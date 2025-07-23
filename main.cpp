// This file is distributed under the MIT license.
// See the LICENSE file for details.

#include <exception>
#include <fstream>
#include <iostream>
#include <string>

#include <visionaray/math/math.h>
#include <visionaray/bvh.h>

#include <common/timer.h>

#include "renderer.h"

using namespace visionaray;

//-------------------------------------------------------------------------------------------------
// Main function, performs initialization
//

int main(int argc, char** argv)
{
    //using host_ray_type = basic_ray<float>;
    //using host_ray_type = basic_ray<simd::float4>;
    using host_ray_type = basic_ray<simd::float8>;
    //using host_ray_type = basic_ray<simd::float16>;

    renderer<host_ray_type> rend;

    try
    {
        rend.init(argc, argv);
    }
    catch (std::exception const& e)
    {
        std::cerr << e.what() << '\n';
        return EXIT_FAILURE;
    }

    if (!rend.mod.load(rend.filename))
    {
        std::cerr << "Failed loading obj model\n";
        return EXIT_FAILURE;
    }

    std::cout << "Creating BVH...\n";

    binned_sah_builder builder;
    builder.enable_spatial_splits(rend.build_strategy == renderer<host_ray_type>::Split);

    rend.host_bvh = builder.build(
            index_bvh<model::triangle_type>{},
            rend.mod.primitives.data(),
            rend.mod.primitives.size()
            );
    rend.materials = make_materials(plastic<float>{}, rend.mod.materials);

    std::cout << "Ready\n";

    float aspect = rend.width / static_cast<float>(rend.height);

    rend.cam.perspective(45.0f * constants::degrees_to_radians<float>(), aspect, 0.001f, 1000.0f);

    // Load camera from file or set view-all
    std::ifstream file(rend.initial_camera);
    if (file.good())
    {
        file >> rend.cam;
    }
    else
    {
        rend.cam.view_all( rend.mod.bbox );
    }

    timer t;
    for (size_t sample = 1; sample <= rend.spp; ++sample)
    {
        rend.render();
        std::cout << "sample " << sample << ": " << t.elapsed() << "ms\n";
        t.reset();
    }

    rend.save_as_png();

    return EXIT_SUCCESS;
}
