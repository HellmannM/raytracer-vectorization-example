// This file is distributed under the MIT license.
// See the LICENSE file for details.
#pragma once

#include <iostream>
#include <memory>
#include <ostream>
#include <string>

#include <Support/CmdLine.h>

#include <visionaray/aligned_vector.h>
#include <visionaray/math/math.h>
#include <visionaray/bvh.h>
#include <visionaray/material.h>
#include <visionaray/pinhole_camera.h>
#include <visionaray/scheduler.h>
#include <visionaray/simple_buffer_rt.h>

#include <common/model.h>

namespace visionaray
{

//-------------------------------------------------------------------------------------------------
// struct with state variables
//

template <typename host_ray_type>
struct renderer
{
    using cmdline_option = std::shared_ptr<support::cl::OptionBase>;

    enum bvh_build_strategy
    {
        Binned = 0, // Binned SAH builder, no spatial splits
        Split,      // Split BVH, also binned and with SAH
        LBVH,       // LBVH builder on the CPU
    };

    pinhole_camera                              cam;
    simple_buffer_rt<PF_RGBA8, PF_UNSPECIFIED, PF_RGBA32F> host_rt;
    tiled_sched<host_ray_type>                  host_sched;
    bvh_build_strategy                          build_strategy  = Binned;

    std::string                                 filename;
    std::string                                 png_filename{"rendered_image.png"};
    std::string                                 initial_camera;

    model                                       mod;
    aligned_vector<plastic<float>>              materials;
    index_bvh<model::triangle_type>             host_bvh;
    unsigned                                    frame_num       = 0;

    size_t                                      width           = 512;
    size_t                                      height          = 512;
    size_t                                      num_threads     = 8;
    size_t                                      spp             = 8;

    std::vector<cmdline_option>                 options;
    support::cl::CmdLine                        cmd;

    renderer();

    void add_cmdline_option(cmdline_option option);
    void init(int argc, char** argv);
    void save_as_png();

    void render();
    void resize(int w, int h);

};

} // namespace visionaray

#include "renderer.inl"

