#pragma once

#include <vector>
#include <visionaray/generic_primitive.h>
#include <visionaray/aligned_vector.h>
#include <visionaray/material.h>
#include <visionaray/math/vector.h>

namespace visionaray
{

struct model
{
    using primitive_type = generic_primitive<basic_sphere<float>,basic_plane<3, float>>;
    using material_type  = plastic<float>;

    aligned_vector<primitive_type> primitives;
    aligned_vector<material_type>  materials;

    void build_snowman()
    {
        primitives.clear();
        materials.clear();

        // Snowman body spheres
        vec3 pos[] = {
            {0.0f, 1.0f, 0.0f}, // base
            {0.0f, 2.25f, 0.0f}, // mid
            {0.0f, 3.25f, 0.0f}  // head
        };

        float r[] = {1.0f, 0.75f, 0.5f};

        for (int i = 0; i < 3; ++i)
        {
            basic_sphere<float> s(pos[i], r[i]);
            primitives.emplace_back(s);

            plastic<float> mat;
	    
	    mat.cd() = from_rgb(1.0f, 1.0f, 1.0f);       // White diffuse
	    mat.kd() = 0.8f;             // Diffuse weight
	    mat.cs() = from_rgb(0.3f, 0.3f, 0.3f);       // Specular highlight
	    mat.ks() = 0.2f;             // Specular weight
	    mat.specular_exp() = 50.0f;  // Shininess
	    materials.push_back(mat);
        }

        // Eyes
        vec3 eye_L = { -0.15f, 3.4f, 0.45f };
        vec3 eye_R = {  0.15f, 3.4f, 0.45f };
        float eye_radius = 0.05f;

        for (auto eye_pos : { eye_L, eye_R })
        {
            primitives.emplace_back(basic_sphere<float>(eye_pos, eye_radius));

            plastic<float> eye_mat;
            eye_mat.cd()  = from_rgb(0.0f, 0.0f, 0.0f);
            eye_mat.ks() = 0.1f;
            eye_mat.specular_exp() = 100.0f;
            materials.push_back(eye_mat);
        }
    }
};

} // namespace visionaray

