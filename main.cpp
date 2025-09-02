#include "renderer.h"
#include <mpi.h>
#include <vector>
#include <algorithm>
#include <chrono>
#include <iostream>
#include <cstring>
#include <thread>
#include <mutex>
#include <map>

using namespace visionaray;

int main(int argc, char** argv)
{
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    using host_ray_type = basic_ray<float>;
    //using host_ray_type = basic_ray<simd::float4>
    //using host_ray_type = basic_ray<simd::float8>;
    //using host_ray_type = basic_ray<simd::float16>


    renderer<host_ray_type> rend;
    rend.init(argc, argv);

    // Ensure each rank has at least 1 sample per pixel
    int original_spp = rend.spp;
    int local_spp = std::max(1, (original_spp + size - 1) / size); // ceil division
    rend.spp = local_spp;

    // Render locally
    auto compute_start_time = std::chrono::high_resolution_clock::now();
    rend.render();
    MPI_Barrier(MPI_COMM_WORLD); // Wait for all ranks to finish
    auto compute_end_time = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> local_compute_duration = compute_end_time - compute_start_time;
    double local_compute_time = local_compute_duration.count();


    // Gather results
    int num_pixels = rend.width * rend.height * 4; // RGBA float buffer
    
    float* local_buffer = nullptr;
    std::vector<float> local_buffer_vec;

    if (rend.alloc_mode == 1) {
    	std::cout << "[WARN] Using malloc() without free\n";
    	local_buffer = (float*)malloc(num_pixels * sizeof(float)); // assign to outer pointer
    } else if (rend.alloc_mode == 2) {
    	std::cout << "[INFO] Using malloc() with free\n";
    	local_buffer = (float*)malloc(num_pixels * sizeof(float));
    } else {
    	std::cout << "[INFO] Using std::vector\n";
    	local_buffer_vec.resize(num_pixels);
    	local_buffer = local_buffer_vec.data();
    }

    // Copy renderer buffer to float array
    auto src = rend.host_rt.color(); // pointer to vector<4, unorm<8>>

    for (int i = 0; i < rend.width * rend.height; ++i)
    {
        local_buffer[4*i + 0] = float(src[i].x);
        local_buffer[4*i + 1] = float(src[i].y);
        local_buffer[4*i + 2] = float(src[i].z);
        local_buffer[4*i + 3] = float(src[i].w);
    }

    std::vector<float> final_buffer;
    float* recv_buf = nullptr;

    if (rank == 0) {
        final_buffer.resize(num_pixels);
        recv_buf = final_buffer.data();
    }

    MPI_Reduce(
        local_buffer,
        recv_buf, // Correctly pass nullptr for non-root ranks
        num_pixels,
        MPI_FLOAT,
        MPI_SUM,
        0,
        MPI_COMM_WORLD
    );

    if (rend.alloc_mode == 2) {
    	free(local_buffer);
    }

    // Rank 0 averages and saves
    if (rank == 0) {
        float scale = 1.0f / size;
        //for (auto& c : final_buffer) c *= scale;i
	for (int i = 0; i < num_pixels; ++i)
	{
		final_buffer[i] = std::min(1.0f, std::max(0.0f, final_buffer[i] * scale));
	}


        // Copy scaled buffer back to renderer as unorm<8>
        auto dst = rend.host_rt.color();
        for (int i = 0; i < rend.width * rend.height; ++i)
        {
            dst[i].x = unorm<8>(final_buffer[4*i + 0]);
            dst[i].y = unorm<8>(final_buffer[4*i + 1]);
            dst[i].z = unorm<8>(final_buffer[4*i + 2]);
            dst[i].w = unorm<8>(final_buffer[4*i + 3]);
        }

        rend.save_as_png();
    }

    // Reporting Performance Metrics
    double max_local_compute_time, min_local_compute_time, sum_local_compute_time;
    MPI_Reduce(&local_compute_time, &max_local_compute_time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    MPI_Reduce(&local_compute_time, &min_local_compute_time, 1, MPI_DOUBLE, MPI_MIN, 0, MPI_COMM_WORLD);
    MPI_Reduce(&local_compute_time, &sum_local_compute_time, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        double avg_local_compute_time = sum_local_compute_time / size;

        std::cout << "\n--- Computational Performance Metrics ---\n";
        std::cout << "Image Size: " << rend.width << "x" << rend.height
                  << ", Num Snowmen: " << 3
                  << ", MPI Processes: " << size << "\n";
        std::cout << "Max Local Computation Time (across all ranks): " << max_local_compute_time << " seconds\n";
        std::cout << "Min Local Computation Time (across all ranks): " << min_local_compute_time << " seconds\n";
        std::cout << "Avg Local Computation Time (across all ranks): " << avg_local_compute_time << " seconds\n";
    }

    MPI_Finalize();
    return 0;
}
