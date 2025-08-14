#pragma once

#include <vector>
#include <visionaray/aligned_vector.h>
#include <visionaray/material.h>
#include <visionaray/math/vector.h>
#include <visionaray/generic_primitive.h>

namespace visionaray
{

struct model
{
    using primitive_type = basic_sphere<float>;
    using material_type  = plastic<float>;

    aligned_vector<primitive_type> primitives;
    aligned_vector<material_type>  materials;
    aabb bbox;

    void build_snowman()
    {
        primitives.clear();
        materials.clear();
        bbox.invalidate();

        // White material for snowman body
        material_type white_mat;
        white_mat.cd() = from_rgb(1.0f, 1.0f, 1.0f);
        white_mat.kd() = 1.0f;
        white_mat.cs() = from_rgb(0.3f, 0.3f, 0.3f);
        white_mat.ks() = 0.0f;
        white_mat.specular_exp() = 50.0f;

        // Black material for eyes
        material_type eye_mat;
        eye_mat.cd() = from_rgb(0.0f, 0.0f, 0.0f);
        eye_mat.ks() = 0.0f;
        eye_mat.kd() = 1.0f;
        eye_mat.specular_exp() = 100.0f;

	// material for floor
	material_type floor_mat;
	floor_mat.cd() = from_rgb(0.92f, 0.95f, 1.0f);
        floor_mat.ks() = 0.0f;
        floor_mat.kd() = 1.0f;
	floor_mat.cs() = from_rgb(0.2f, 0.2f, 0.2f);
        floor_mat.specular_exp() = 20.0f;

	// material for nose
	material_type orange_mat;
	orange_mat.cd() = from_rgb(1.0f, 0.5f, 0.0f);
	orange_mat.kd() = 1.0f;
	orange_mat.ks() = 0.0f;
	orange_mat.specular_exp() = 50.0f;
	
	// material for stick hands
	material_type brown_mat;
	brown_mat.cd() = from_rgb(0.4f, 0.25f, 0.1f);
	brown_mat.kd() = 1.0f;
	brown_mat.ks() = 0.0f;
	brown_mat.specular_exp() = 30.0f;

        materials.push_back(white_mat); // material 0
        materials.push_back(eye_mat);   // material 1
	materials.push_back(floor_mat); // material 2
	materials.push_back(orange_mat); // material 3
	materials.push_back(brown_mat); //material 4	


        int prim_counter = 0;
        const float spacing = 4.0f;
        const float start_x = -((3 -1) * spacing) / 2.0f;

	// Floor
	vec3 floor_pos = {1.0f, -199.3f, 0.0f};
	float floor_radius = 200.0f;
	primitive_type floor_sphere(floor_pos, floor_radius);	
	floor_sphere.prim_id = prim_counter++;
	floor_sphere.geom_id = 2; // floor material
	primitives.push_back(floor_sphere);
	bbox.insert(aabb(floor_pos - floor_radius, floor_pos + floor_radius));

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

                bbox.insert(aabb(pos[i] - radii[i], pos[i] + radii[i]));
            }

	    // Nose
	    vec3 nose_pos = {base_x, 3.25f, 0.65f};
	    primitive_type nose(nose_pos, 0.12f);
	    nose.prim_id = prim_counter++;
	    nose.geom_id = 3; // orange
	    primitives.push_back(nose);
	    bbox.insert(aabb(nose_pos - 0.12f, nose_pos + 0.12f));

            // Eyes on head (relative to the last sphere)
            vec3 eye_L = {base_x - 0.15f, 3.4f, 0.45f};
            vec3 eye_R = {base_x + 0.15f, 3.4f, 0.45f};
            float eye_radius = 0.1f;

            for (auto eye_pos : {eye_L, eye_R})
            {
                primitive_type eye_sphere(eye_pos, eye_radius);
                eye_sphere.prim_id = prim_counter++;
                eye_sphere.geom_id = 1; // black eyes material
                primitives.push_back(eye_sphere);

                bbox.insert(aabb(eye_pos - eye_radius, eye_pos + eye_radius));
            }

	    // Buttons on middle sphere
	    float button_z = 0.85f;
	    float button_spacing = 0.25f;
	    for (int b = 0; b < 3; ++b)
	    {
		    vec3 button_pos = {base_x, 2.25f + 0.3f - b * button_spacing, button_z};
		    primitive_type button(button_pos, 0.1f);
		    button.prim_id = prim_counter++;
		    button.geom_id = 1; // black
		    primitives.push_back(button);
		    bbox.insert(aabb(button_pos - 0.1f, button_pos + 0.1f));
	    }

	    // Hat
	    vec3 hat_base = {base_x, 3.25f + 0.55f, 0.0f};
	    vec3 hat_top  = {base_x, hat_base.y + 0.3f, 0.0f};
	    primitive_type brim(hat_base, 0.35f);
	    brim.prim_id = prim_counter++;
	    brim.geom_id = 1; // black
	    primitives.push_back(brim);
	    bbox.insert(aabb(hat_base - 0.35f, hat_base + 0.35f));
	    
	    primitive_type top(hat_top, 0.25f);
	    top.prim_id = prim_counter++;
	    top.geom_id = 1; // black
	    primitives.push_back(top);
	    bbox.insert(aabb(hat_top - 0.25f, hat_top + 0.25f));
	    
	    // Stick hands (chains of brown spheres)
	    float hand_y = 2.25f;
	    for (int side : {-1, 1})
	    {
		    vec3 start = {base_x + side * 0.9f, hand_y, 0.0f};

		    // Main branch
		    vec3 tip_pos;
		    for (int h = 0; h < 5; ++h)
		    {
			    float r = 0.12f - h * 0.02f;
			    vec3 hand_pos = {start.x  + side * h * 0.10f, hand_y + h * 0.03f, 0.0f};
			    primitive_type hand(hand_pos, r);
			    hand.prim_id = prim_counter++;
			    hand.geom_id = 4; // brown
			    primitives.push_back(hand);
			    bbox.insert(aabb(hand_pos - r, hand_pos + r));

			    if (h == 4) tip_pos = hand_pos; // save end position
		    }

		    for (int branch = 0; branch < 2; ++branch)
		    {
			  vec3 dir = {side * 0.1f, branch == 0 ? 0.1f : -0.1f , 0.03f};
			  for (int f = 0; f < 3; ++f)
			  {
				  float r = 0.05f - f * 0.01f;
				  vec3 fork_pos = tip_pos + dir * float(f + 0.8f);
				  primitive_type fork(fork_pos, r);
				  fork.prim_id = prim_counter++;
				  fork.geom_id = 4; // brown
				  primitives.push_back(fork);
				  bbox.insert(aabb(fork_pos - r, fork_pos + r));
			  }
		    }

	    }
        }
    }
};

} // namespace visionaray

