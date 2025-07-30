#pragma once

#include <Support/CmdLine.h>
#include <Support/CmdLineUtil.h>

#include <visionaray/bvh.h>
#include <visionaray/kernels.h>
#include <visionaray/point_light.h>
#include <visionaray/sampling.h>
#include <visionaray/scheduler.h>
#include <visionaray/texture/texture.h>

#include <common/image.h>
#include <common/model.h>
#include <common/obj_loader.h>

namespace visionaray
{

template <typename host_ray_type>
renderer<host_ray_type>::renderer()
    : host_sched(8)
{
    using namespace support;

    add_cmdline_option(cl::makeOption<std::string&>(
        cl::Parser<>(),
        "png",
        cl::Desc("Output PNG filename"),
        cl::ArgRequired,
        cl::init(this->png_filename)
    ));

    add_cmdline_option(cl::makeOption<size_t&>(
        cl::Parser<>(),
        "width",
        cl::Desc("Image width"),
        cl::ArgRequired,
        cl::init(this->width)
    ));

    add_cmdline_option(cl::makeOption<size_t&>(
        cl::Parser<>(),
        "height",
        cl::Desc("Image height"),
        cl::ArgRequired,
        cl::init(this->height)
    ));

    add_cmdline_option(cl::makeOption<size_t&>(
        cl::Parser<>(),
        "threads",
        cl::Desc("Number of threads"),
        cl::ArgRequired,
        cl::init(this->num_threads)
    ));

    add_cmdline_option(cl::makeOption<size_t&>(
        cl::Parser<>(),
        "spp",
        cl::Desc("Samples per pixel"),
        cl::ArgRequired,
        cl::init(this->spp)
    ));
}

template <typename host_ray_type>
void renderer<host_ray_type>::add_cmdline_option(cmdline_option option)
{
    options.push_back(option);
}

template <typename host_ray_type>
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

    host_sched.reset(this->num_threads);

    mod.build_snowman();
    using bvh_ref = index_bvh<model::primitive_type>::bvh_ref;
    std::vector<bvh_ref> bvhs{host_bvh.ref()};
    materials = mod.materials;

    cam.look_at({0.0f, 2.5f, 7.0f}, {0.0f, 2.5f, 0.0f}, {0.0f, 1.0f, 0.0f});
    resize(width, height);
}

template <typename host_ray_type>
void renderer<host_ray_type>::resize(int w, int h)
{
    frame_num = 0;
    width = w;
    height = h;
    host_rt.resize(w, h);
    host_rt.clear_color_buffer();

    cam.set_viewport(0, 0, w, h);
    cam.perspective(45.0f * constants::degrees_to_radians<float>(), float(w) / h, 0.1f, 100.0f);
}

template <typename host_ray_type>
void renderer<host_ray_type>::render()
{
    float alpha = 1.0f / ++frame_num;

    pixel_sampler::jittered_blend_type jps;
    jps.spp     = spp;
    jps.sfactor = alpha;
    jps.dfactor = 1.0f - alpha;

    using bvh_ref = index_bvh<model::primitive_type>::bvh_ref;
    std::vector<bvh_ref> bvhs{host_bvh.ref()};

    point_light<float> light;
    light.set_cl(vec3(1.0f, 1.0f, 1.0f));
    light.set_position(cam.eye());
    light.set_constant_attenuation(1.0f);

    std::vector<point_light<float>> lights{light};

    auto kparams = make_kernel_params(
        mod.primitives.data(),
        mod.primitives.data() + mod.primitives.size(),
        materials.data(),
        lights.data(),
        lights.data() + lights.size(),
        4,                          // max bounces
        0.001f,                     // self-intersection epsilon
        vec4(0.7f, 0.8f, 1.0f, 1.0f),  // sky blue background
        vec4(0.0f)
    );

    pathtracing::kernel<decltype(kparams)> kernel;
    kernel.params = kparams;

    auto sparams = make_sched_params(jps, cam, host_rt);
    host_sched.frame(kernel, sparams);
}

template <typename host_ray_type>
void renderer<host_ray_type>::save_as_png()
{
    std::vector<vector<4, unorm<8>>> rgba(width * height);
    memcpy(rgba.data(), host_rt.color(), width * height * 4);

    std::vector<vector<3, unorm<8>>> rgb(width * height);
    for (size_t i = 0; i < rgb.size(); ++i)
    {
        rgb[i] = vector<3, unorm<8>>(rgba[i].x, rgba[i].y, rgba[i].z);
    }

    std::vector<vector<3, unorm<8>>> flipped(width * height);
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            int xx = width - x - 1;
            int yy = height - y - 1;
            flipped[y * width + x] = rgb[yy * width + xx];
        }
    }

    image img(width, height, PF_RGB8, reinterpret_cast<uint8_t const*>(flipped.data()));
    image::save_option opt;
    img.save(png_filename, {opt});
}

} // namespace visionaray

