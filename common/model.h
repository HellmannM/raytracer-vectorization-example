#pragma once

#include <vector>
#include <visionaray/aligned_vector.h>
#include <visionaray/material.h>
#include <visionaray/math/vector.h>
#include<visionaray/generic_primitive.h>

namespace visionaray
{

struct model
{
    using primitive_type = basic_sphere<float>;
    using material_type  = plastic<float>;

    aligned_vector<primitive_type> primitives;
    aligned_vector<material_type>  materials;

    void build_snowman()
    {
        primitives.clear();
        materials.clear();

        // White material for snowman body
        material_type white_mat;
        white_mat.cd() = from_rgb(1.0f, 1.0f, 1.0f);
        white_mat.kd() = 0.8f;
        white_mat.cs() = from_rgb(0.3f, 0.3f, 0.3f);
        white_mat.ks() = 0.2f;
        white_mat.specular_exp() = 50.0f;

        // Black material for eyes
        material_type eye_mat;
        eye_mat.cd() = from_rgb(0.0f, 0.0f, 0.0f);
        eye_mat.ks() = 0.1f;
        eye_mat.specular_exp() = 100.0f;

        materials.push_back(white_mat); // material 0
        materials.push_back(eye_mat);   // material 1


        int prim_counter = 0;
	const float spacing = 4.0f;
	const float start_x = -((3 -1) * spacing) / 2.0f;

        for (int sn = 0; sn < 3; ++sn) // three snowmen
        {
            float base_x = start_x + sn * spacing;

            // Snowman body positions (stacked vertically)
            vec3 pos[] = {
                {base_x, 1.0f, 0.0f},   // base sphere
                {base_x, 2.25f, 0.0f},  // middle sphere
                {base_x, 3.25f, 0.0f}   // head sphere
            };

            float radii[] = {1.0f, 0.75f, 0.5f};

            for (int i = 0; i < 3; ++i)
            {
                primitive_type s(pos[i], radii[i]);
                s.prim_id = prim_counter++;
                s.geom_id = 0; // white material
                primitives.push_back(s);
            }

            // Eyes on head (relative to the last sphere)
            vec3 eye_L = {base_x - 0.15f, 3.4f, 0.45f};
            vec3 eye_R = {base_x + 0.15f, 3.4f, 0.45f};
            float eye_radius = 0.05f;

            for (auto eye_pos : {eye_L, eye_R})
            {
                primitive_type eye_sphere(eye_pos, eye_radius);
                eye_sphere.prim_id = prim_counter++;
                eye_sphere.geom_id = 1; // black eyes material
                primitives.push_back(eye_sphere);
            }
        }
    }
};

} // namespace visionaray

