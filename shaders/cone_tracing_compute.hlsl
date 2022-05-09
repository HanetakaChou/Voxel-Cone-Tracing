//
// Copyright (C) YuqiaoZhang(HanetakaChou)
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//

#define INTERNAL_BRX_DISABLE_ROOT_SIGNATURE 1
#define BRX_VCT_ENABLE_ILLUMINATION 1
#define BRX_VCT_CONE_TRACING_DIFFUSE_CONE_COUNT 24
#define BRX_VCT_CONE_TRACING_SPECULAR_CONE_COUNT 8
#define BRX_VCT_CONE_TRACING_MAXIMUM_ITERATION_COUNT 128
#include "../thirdparty/Voxel-Cone-Tracing/shaders/brx_voxel_cone_tracing_cone_tracing_compute.bsli"

#include "../thirdparty/Brioche-Shader-Language/shaders/brx_shader_language.bsli"
#include "../thirdparty/Brioche-Shader-Language/shaders/brx_octahedral_mapping.bsli"

#include "cone_tracing_resource_binding.hlsli"

brx_bool brx_vct_application_bridge_get_gbuffer_surface(in brx_int2 in_screen_space_coordinates, out brx_float3 out_surface_position_world_space, out brx_float3 out_surface_shading_normal_world_space, out brx_float3 out_surface_diffuse_color, out brx_float3 out_surface_specular_color, out brx_float out_surface_roughness)
{
    const brx_float INTERNAL_BRX_INVALID_SCENE_DEPTH = 1.0;

    brx_int2 g_screen_width_height = brx_write_only_texture_2d_get_dimension(u_indirect_radiance_and_ambient_occlusion);

    brx_float2 screen_space_uv = (brx_float2(in_screen_space_coordinates) + brx_float2(0.5, 0.5)) / brx_float2(g_screen_width_height);

    brx_float scene_depth = brx_sample_level_2d(t_scene_depth, s_sampler, screen_space_uv, 0).x;

    brx_branch if (INTERNAL_BRX_INVALID_SCENE_DEPTH == scene_depth)
    {
        out_surface_position_world_space = brx_float3(0.0, 0.0, 0.0);
        out_surface_shading_normal_world_space = brx_float3(0.0, 0.0, 0.0);
        out_surface_diffuse_color = brx_float3(0.0, 0.0, 0.0);
        out_surface_specular_color = brx_float3(0.0, 0.0, 0.0);
        out_surface_roughness = 0.0;
        return false;
    }
    else
    {
        {
            // brx_float position_depth = brx_uint_as_float(scene_depth);
            brx_float position_depth = scene_depth;

            brx_float3 position_ndc_space = brx_float3(screen_space_uv * brx_float2(2.0, -2.0) + brx_float2(-1.0, 1.0), position_depth);

            brx_float4 position_view_space_with_w = brx_mul(g_inverse_projection_transform, brx_float4(position_ndc_space, 1.0));

            brx_float3 position_view_space = position_view_space_with_w.xyz / position_view_space_with_w.w;

            brx_float3 position_world_space = brx_mul(g_inverse_view_transform, brx_float4(position_view_space, 1.0)).xyz;

            out_surface_position_world_space = position_world_space;
        }

        {

            brx_float3 base_color = brx_sample_level_2d(t_gbuffer_base_color, s_sampler, screen_space_uv, 0).xyz;

            brx_float2 roughness_metallic = brx_sample_level_2d(t_gbuffer_roughness_metallic, s_sampler, screen_space_uv, 0).xy;

            out_surface_roughness = roughness_metallic.x;

            brx_float metallic = roughness_metallic.y;

            out_surface_shading_normal_world_space = brx_octahedral_unmap(brx_sample_level_2d(t_gbuffer_normal, s_sampler, screen_space_uv, 0).xy);

            // UE4: https://github.com/EpicGames/UnrealEngine/blob/4.21/Engine/Shaders/Private/MobileBasePassPixelShader.usf#L376
            const brx_float dielectric_specular = 0.04;

            out_surface_specular_color = brx_clamp((dielectric_specular - dielectric_specular * metallic) + base_color * metallic, 0.0, 1.0);
            out_surface_diffuse_color = brx_clamp(base_color - base_color * metallic, 0.0, 1.0);
        }

        return true;
    }
}

brx_float3 brx_vct_application_bridge_get_camera_ray_origin()
{
    brx_float3 camera_ray_origin = brx_mul(g_inverse_view_transform, brx_float4(0.0, 0.0, 0.0, 1.0)).xyz;
    return camera_ray_origin;
}

brx_float3 brx_vct_application_bridge_get_clipmap_anchor()
{
    return g_clipmap_anchor.xyz;
}

brx_float3 brx_vct_application_bridge_get_clipmap_center()
{
    return g_clipmap_center.xyz;
}

brx_float4 brx_vct_application_bridge_get_clipmap_illumination_opacity(in brx_float3 in_illumination_opacity_texture_coordinates)
{
    brx_int3 illumination_opacity_texture_dimension = brx_texture_3d_get_dimension(t_clipmap_texture_illumination_opacity_r16g16b16a16, 0);

    brx_float3 normalized_illumination_opacity_texture_coordinates = in_illumination_opacity_texture_coordinates / brx_float3(illumination_opacity_texture_dimension);

    brx_float4 illumination_opacity = brx_sample_level_3d(t_clipmap_texture_illumination_opacity_r16g16b16a16, s_sampler, normalized_illumination_opacity_texture_coordinates, 0);

    return illumination_opacity;
}

void brx_vct_application_bridge_set_radiance_and_ambient_occlusion(in brx_int2 in_screen_space_coordinates, in brx_float4 in_radiance_and_ambient_occlusion)
{
    brx_store_2d(u_indirect_radiance_and_ambient_occlusion, in_screen_space_coordinates, in_radiance_and_ambient_occlusion);
}
