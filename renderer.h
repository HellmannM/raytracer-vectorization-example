// This file is distributed under the MIT license.
// See the LICENSE file for details.
#pragma once

#include <iostream>
#include <memory>
#include <ostream>
#include <string>

#include <Support/CmdLine.h>
#include <Support/CmdLineUtil.h>

#include <visionaray/aligned_vector.h>
#include <visionaray/math/math.h>
#include <visionaray/bvh.h>
#include <visionaray/kernels.h>
#include <visionaray/material.h>
#include <visionaray/pinhole_camera.h>
#include <visionaray/point_light.h>
#include <visionaray/sampling.h>
#include <visionaray/scheduler.h>
#include <visionaray/simple_buffer_rt.h>

#include <common/image.h>
#include <common/make_materials.h>
#include <common/model.h>
#include <common/obj_loader.h>

namespace visionaray
{

using cmdline_options = std::vector<std::shared_ptr<support::cl::OptionBase>>;

//-------------------------------------------------------------------------------------------------
// struct with state variables
//

template <typename host_ray_type>
struct renderer
{
    renderer()
        : host_sched(8)
    {
        using namespace support;

        add_cmdline_option( cl::makeOption<std::string&>(
            cl::Parser<>(),
            "filename",
            cl::Desc("Input file in wavefront obj format"),
            cl::Positional,
            cl::Required,
            cl::init(this->filename)
            ) );

        add_cmdline_option( cl::makeOption<std::string&>(
            cl::Parser<>(),
            "camera",
            cl::Desc("Text file with camera parameters"),
            cl::ArgRequired,
            cl::init(this->initial_camera)
            ) );

        add_cmdline_option( cl::makeOption<bvh_build_strategy&>({
                { "default",            Binned,         "Binned SAH" },
                { "split",              Split,          "Binned SAH with spatial splits" },
                { "lbvh",               LBVH,           "LBVH (CPU)" }
            },
            "bvh",
            cl::Desc("BVH build strategy"),
            cl::ArgRequired,
            cl::init(this->build_strategy)
            ) );

        add_cmdline_option( cl::makeOption<size_t&>(
            cl::Parser<>(),
            "width",
            cl::Desc("Image width in pixels"),
            cl::ArgRequired,
            cl::init(this->width)
            ) );

        add_cmdline_option( cl::makeOption<size_t&>(
            cl::Parser<>(),
            "height",
            cl::Desc("Image height in pixels"),
            cl::ArgRequired,
            cl::init(this->height)
            ) );

        add_cmdline_option( cl::makeOption<size_t&>(
            cl::Parser<>(),
            "threads",
            cl::Desc("Number of threads"),
            cl::ArgRequired,
            cl::init(this->num_threads)
            ) );

        add_cmdline_option( cl::makeOption<size_t&>(
            cl::Parser<>(),
            "spp",
            cl::Desc("Number of samples per pixel"),
            cl::ArgRequired,
            cl::init(this->spp)
            ) );

        add_cmdline_option( cl::makeOption<std::string&>(
            cl::Parser<>(),
            "png",
            cl::Desc("Output PNG filename"),
            cl::ArgRequired,
            cl::init(this->png_filename)
            ) );
    }

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

    cmdline_options                             options;
    support::cl::CmdLine                        cmd;

    void add_cmdline_option( std::shared_ptr<support::cl::OptionBase> option );
    void init(int argc, char** argv);
    void save_as_png();

    void render();
    void resize(int w, int h);

};

//-------------------------------------------------------------------------------------------------
// I/O utility for camera lookat only - not fit for the general case!
//

std::istream& operator>>(std::istream& in, pinhole_camera& cam)
{
    vec3 eye;
    vec3 center;
    vec3 up;

    in >> eye >> std::ws >> center >> std::ws >> up >> std::ws;
    cam.look_at(eye, center, up);

    return in;
}

std::ostream& operator<<(std::ostream& out, pinhole_camera const& cam)
{
    out << cam.eye() << '\n';
    out << cam.center() << '\n';
    out << cam.up() << '\n';
    return out;
}

//-------------------------------------------------------------------------------------------------
// CmdLine args
//

template<typename host_ray_type>
void renderer<host_ray_type>::init(int argc, char** argv)
{
    using namespace support;

    for (auto& opt : options)
    {
        cmd.add(*opt);
    }

    auto args = std::vector<std::string>(argv + 1, argv + argc);
    cl::expandWildcards(args);
    cl::expandResponseFiles(args, cl::TokenizeUnix());

    cmd.parse(args, false);

    host_sched.reset(num_threads);

    resize(width, height);
}

template<typename host_ray_type>
void renderer<host_ray_type>::add_cmdline_option( std::shared_ptr<support::cl::OptionBase> option )
{
    options.emplace_back(option);
}

//-------------------------------------------------------------------------------------------------
// Render function, implements the kernel
//

template<typename host_ray_type>
void renderer<host_ray_type>::render()
{
    float alpha = 1.0f / ++frame_num;
    pixel_sampler::jittered_blend_type jps;
    jps.spp = 1;
    jps.sfactor = alpha;
    jps.dfactor = 1.0f - alpha;
    auto sparams = make_sched_params(
            jps,
            cam,
            host_rt
            );

    using bvh_ref = index_bvh<model::triangle_type>::bvh_ref;
    std::vector<bvh_ref> bvhs;
    bvhs.push_back(host_bvh.ref());

    // headlight
    point_light<float> headlight;
    headlight.set_cl( vec3(1.0f, 1.0f, 1.0f) );
    headlight.set_kl( 1.0f );
    headlight.set_position( cam.eye() );
    headlight.set_constant_attenuation(1.0f);
    headlight.set_linear_attenuation(0.0f);
    headlight.set_quadratic_attenuation(0.0f);
    std::vector<point_light<float>> lights{headlight};

    auto kparams = make_kernel_params(
            mod.primitives.data(),
            mod.primitives.data() + mod.primitives.size(),
            materials.data(),
            lights.data(),
            lights.data() + lights.size(),
            4,                          // max bounces
            0.001f,                     // self-intersection
            vec4(0.2, 0.2, 0.5, 1.0),   // bg-color
            vec4(0.0)
            );

    pathtracing::kernel<decltype(kparams)> kernel;
    kernel.params = kparams;

    host_sched.frame(
        kernel,
        sparams
        );
}

//-------------------------------------------------------------------------------------------------
// resize event
//

template<typename host_ray_type>
void renderer<host_ray_type>::resize(int w, int h)
{
    frame_num = 0;
    host_rt.clear_color_buffer();

    cam.set_viewport(0, 0, w, h);
    float aspect = w / static_cast<float>(h);
    cam.perspective(45.0f * constants::degrees_to_radians<float>(), aspect, 0.001f, 1000.0f);
    host_rt.resize(w, h);
}

//-------------------------------------------------------------------------------------------------
// write fb to png
//

template<typename host_ray_type>
void renderer<host_ray_type>::save_as_png()
{
    // Swizzle to RGB8 for compatibility with pnm image
    std::vector<vector<4, unorm<8>>> rgba(width * height);
    memcpy(rgba.data(), host_rt.color(), width * height * 4);
    std::vector<vector<3, unorm<8>>> rgb(width * height);
    for (size_t i=0; i<rgb.size(); ++i)
    {
        rgb[i] = vector<3, unorm<8>>(rgba[i].x, rgba[i].y, rgba[i].z);
    }

    // Flip
    std::vector<vector<3, unorm<8>>> flipped(width * height);
    for (int y = 0; y < height; ++y)
    {
      for (int x = 0; x < width; ++x)
      {
        auto xx = width - x - 1;
        auto yy = height - y - 1;
        flipped[y * width + x] = rgb[yy * width + xx];
      }
    }

    image img(
        width,
        height,
        PF_RGB8,
        reinterpret_cast<uint8_t const*>(flipped.data())
        );

    image::save_option opt;
    img.save(png_filename, {opt});
}

} // namespace visionaray
