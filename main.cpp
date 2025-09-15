#include "renderer.h"
#include <mpi.h>
#include <vector>
#include <algorithm>
#include <chrono>
#include <iostream>
#include <cstring>

using namespace visionaray;

int main(int argc, char** argv)
{
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

#ifdef __CUDACC__
    int device_count = 0;
    cudaGetDeviceCount(&device_count);

    int local_rank = 0;
    char* env = getenv("OMPI_COMM_WORLD_LOCAL_RANK");  // OpenMPI specific
    if (env) {
        local_rank = std::atoi(env);
    }

    int device_id = local_rank % device_count;
    cudaSetDevice(device_id);

    if (rank == 0) {
        std::cout << "CUDA devices available: " << device_count << std::endl;
    }
    std::cout << "MPI rank " << rank << " mapped to GPU " << device_id << std::endl;
#endif

    using host_ray_type = basic_ray<float>;
#ifdef __CUDACC__
    using device_ray_type = basic_ray<float>;
#endif
    //using host_ray_type = basic_ray<simd::float4>;
    //using host_ray_type = basic_ray<simd::float8>;
    //using host_ray_type = basic_ray<simd::float16>;


    renderer<host_ray_type> rend;
    rend.init(argc, argv);

    // Ensure each rank has at least 1 sample per pixel
    int original_spp = rend.spp;
    int local_spp = std::max(1, (original_spp + size - 1) / size); // ceil division
    rend.spp = local_spp;

    // Render locally
    auto compute_start_time = std::chrono::high_resolution_clock::now();
    std::cout << "Render started on Rank: " << rank << std::endl;
    rend.render();
    MPI_Barrier(MPI_COMM_WORLD); // Wait for all ranks to finish
    std::cout << "Renderer completed on Rank: " << rank << std::endl;
    auto compute_end_time = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> local_compute_duration = compute_end_time - compute_start_time;
    double local_compute_time = local_compute_duration.count();

    // Gather results
    int num_pixels = rend.width * rend.height * 4; // RGBA float buffer
    std::vector<float> local_buffer(num_pixels);

#ifdef __CUDACC__
    if (rend.dev_type == rend.GPU)
    {
        // Copy the rendered image data from the GPU to the host buffer
        std::vector<vector<4, unorm<8>>> host_rgba(rend.width * rend.height);
        cudaMemcpy(host_rgba.data(), rend.device_rt.color(), rend.width * rend.height * 4, cudaMemcpyDeviceToHost);

        for (int i = 0; i < rend.width * rend.height; ++i)
        {
            local_buffer[4*i + 0] = float(host_rgba[i].x);
            local_buffer[4*i + 1] = float(host_rgba[i].y);
            local_buffer[4*i + 2] = float(host_rgba[i].z);
            local_buffer[4*i + 3] = float(host_rgba[i].w);
        }

	std::cout << "Rendered image from  GPU to CPU" << std::endl;
    }
    else
#endif
    {
        auto src = rend.host_rt.color(); // pointer to vector<4, unorm<8>>
        for (int i = 0; i < rend.width * rend.height; ++i)
        {
            local_buffer[4*i + 0] = float(src[i].x);
            local_buffer[4*i + 1] = float(src[i].y);
            local_buffer[4*i + 2] = float(src[i].z);
            local_buffer[4*i + 3] = float(src[i].w);
        }

	std::cout << "Rendered image to CPU" << std::endl;
    }

    std::vector<float> final_buffer;
    float* recv_buf = nullptr;

    if (rank == 0) {
        final_buffer.resize(num_pixels);
        recv_buf = final_buffer.data();
    }

    MPI_Reduce(
        local_buffer.data(),
        recv_buf, // Correctly pass nullptr for non-root ranks
        num_pixels,
        MPI_FLOAT,
        MPI_SUM,
        0,
        MPI_COMM_WORLD
    );

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
