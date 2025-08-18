#pragma once

#include <Support/CmdLine.h>
#include <Support/CmdLineUtil.h>

#include <visionaray/bvh.h>
#include <visionaray/kernels.h>
//#include <visionaray/directional_light.h>
#include <visionaray/point_light.h>
#include <visionaray/sampling.h>
#include <visionaray/scheduler.h>

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
#ifdef __CUDACC__
    using namespace support;
    
    add_cmdline_option( cl::makeOption<device_type&>({{ "cpu", CPU, "Rendering on the CPU" }, { "gpu", GPU, "Rendering on the GPU" },},
	"device",
        cl::Desc("Rendering device"),
        cl::ArgRequired,
        cl::init(this->dev_type)
    ) );
#endif
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
    std::cout << "Scene bbox min: " << mod.bbox.min << " max: " << mod.bbox.max << "\n";
    for (unsigned i = 0; i < mod.primitives.size(); ++i)
    {
	    mod.primitives[i].prim_id = i;
    }
    
    if (host_bvh.num_nodes() == 0)
    {
	    lbvh_builder builder;
	    host_bvh = builder.build(index_bvh<model::primitive_type>{},mod.primitives.data(),mod.primitives.size());
    }

    materials = mod.materials;

    cam.look_at({0.0f, 10.5f, 10.0f}, {0.0f, 2.5f, 0.0f}, {0.0f, 1.5f, 0.0f});
    resize(width, height);

#ifdef __CUDACC__
    // Copy scene to GPU
    device_spheres = mod.primitives;
    device_materials = materials;

    // Resize GPU render target
    device_rt.resize(width, height);
#endif

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
    cam.perspective(60.0f * constants::degrees_to_radians<float>(), float(w) / h, 0.1f, 100.0f);
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
    bvhs.push_back(host_bvh.ref());

    //directional_light<float> sunlight;
    //sunlight.set_cl(vec3(1.0f, 1.0f, 1.0f));
    //sunlight.set_direction(normalize(vec3(-1.0f, 1.0f, -1.0f)));
    //std::vector<directional_light<float>> lights{sunlight};
    
    point_light<float> headlight;
    headlight.set_cl(vec3(0.9f, 0.9f, 0.9f));
    headlight.set_kl(0.3f);
    vec3f dir = cam.center() - cam.eye();
    vec3f back = -65.f * dir;
    vec3f up = 50.f * cam.up();
    vec3f side = 50.f * norm(dir) * cross(normalize(dir), cam.up());
    vec3f pos = cam.eye() + back + up + side;
    headlight.set_position(pos);
    headlight.set_constant_attenuation(1.0f);
    headlight.set_linear_attenuation(0.0f);
    headlight.set_quadratic_attenuation(0.0f);
    std::vector<point_light<float>> lights{headlight};

    vec3* dummies = nullptr;
    aligned_vector<vec3> dummy_textures;

    aligned_vector<index_bvh<basic_sphere<float>>::bvh_ref> refs;
    refs.push_back(host_bvh.ref());

    auto kparams = make_kernel_params(
            normals_per_face_binding{},
	    mod.primitives.data(),
            mod.primitives.data() + mod.primitives.size(),
	    (vec3*)nullptr,
	    (vec3*)nullptr,
            materials.data(),
            lights.data(),
            lights.data() + lights.size(),
            4,                          // number of reflective bounces
            0.0001f,                     // epsilon to avoid self intersection by secondary rays
            vec4(0.8f, 0.6f, 0.8f, 1.0f),
            vec4(1.0f)
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

