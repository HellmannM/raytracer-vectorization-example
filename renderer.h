#pragma once

#include <memory>
#include <string>
#include <iostream>
#include <ostream>
#include <vector>

#include <Support/CmdLine.h>

#include <visionaray/aligned_vector.h>
#include <visionaray/math/math.h>
#include <visionaray/bvh.h>
#include <visionaray/material.h>
#include <visionaray/pinhole_camera.h>
#include <visionaray/scheduler.h>
#include <visionaray/simple_buffer_rt.h>

#include <common/model.h>

#ifdef __CUDACC__
#include <cuda_runtime_api.h>
#include <thrust/device_vector.h>
#include <common/gpu_buffer_rt.h>
#endif

namespace visionaray
{

template <typename host_ray_type>
struct renderer
{
    using cmdline_option = std::shared_ptr<support::cl::OptionBase>;

    pinhole_camera cam;
    simple_buffer_rt<PF_RGBA8, PF_UNSPECIFIED, PF_RGBA32F> host_rt;
    tiled_sched<host_ray_type> host_sched;

    model mod;
    aligned_vector<plastic<float>> materials;
    index_bvh<model::primitive_type> host_bvh;

#ifdef __CUDACC__
    enum device_type
    {
	    CPU = 0,
	    GPU
    };

    gpu_buffer_rt<PF_RGBA8, PF_UNSPECIFIED, PF_RGBA32F> device_rt;
    cuda_sched<host_ray_type> device_sched;

    thrust::device_vector<model::primitive_type> device_primitives;
    thrust::device_vector<plastic<float>> device_materials;

    cuda_index_bvh<model::primitive_type> device_bvh;

    device_type dev_type = GPU;
#endif
    
    std::string png_filename = "rendered_snowman.png";

    size_t width = 512;
    size_t height = 512;
    size_t spp = 128;
    size_t num_threads = 1;
    unsigned frame_num = 0;
    size_t alloc_mode = 3;

    std::vector<cmdline_option> options;
    support::cl::CmdLine cmd;

    renderer();

    void add_cmdline_option(cmdline_option option);
    void init(int argc, char** argv);
    void save_as_png();
    void render();
    void resize(int w, int h);
    
};

} // namespace visionaray

#include "renderer.inl"

