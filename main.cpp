#include "renderer.h"

using namespace visionaray;

int main(int argc, char** argv)
{
    using host_ray_type = basic_ray<simd::float8>;

    renderer<host_ray_type> rend;
    rend.init(argc, argv);
    rend.render();
    rend.save_as_png();

    return 0;
}
