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
#define BRX_VCT_VOXELIZATION_MAXIMUM_ENVIRONMENT_LIGHTING_ILLUMINATION_FLOAT_COUNT 0
#define BRX_VCT_VOXELIZATION_MAXIMUM_DIRECT_LIGHTING_COUNT 1
#include "../thirdparty/Voxel-Cone-Tracing/shaders/brx_voxel_cone_tracing_voxelization_fragment.bsli"

#include "voxelization_resource_binding.hlsli"
#include "surface_resource_binding.hlsli"
#include "get_shadow.hlsli"
#include "../thirdparty/Brioche-Shader-Language/shaders/brx_brdf.bsli"

brx_bool brx_vct_application_bridge_get_surface(in brx_float3 in_normal, in brx_float4 in_tangent, in brx_float2 in_texcoord, out brx_float3 out_surface_shading_normal_world_space, out brx_float3 out_surface_diffuse_color, out brx_float3 out_surface_specular_color, out brx_float out_surface_roughness, out brx_float out_surface_opacity, out brx_float3 out_surface_emissive)
{
    {
        brx_float3 geometry_normal_world_space = brx_normalize(in_normal);

        brx_float3 tangent_world_space = brx_normalize(in_tangent.xyz);

        brx_float3 bitangent_world_space = brx_cross(geometry_normal_world_space, tangent_world_space) * ((in_tangent.w >= 0.0) ? 1.0 : -1.0);

        brx_float3 shading_normal_tangent_space = brx_normalize(brx_sample_2d(g_normal_texture, g_surface_sampler, in_texcoord).xyz * 2.0 - float3(1.0, 1.0, 1.0));

        out_surface_shading_normal_world_space = brx_normalize(tangent_world_space * shading_normal_tangent_space.x + bitangent_world_space * shading_normal_tangent_space.y + geometry_normal_world_space * shading_normal_tangent_space.z);
    }

    {
        brx_float4 base_color_and_opacity = brx_sample_2d(g_base_color_texture, g_surface_sampler, in_texcoord) * g_base_color_factor;

        brx_float3 base_color = base_color_and_opacity.xyz;

        out_surface_opacity = base_color_and_opacity.w;

        brx_float2 surface_roughness_metallic = brx_sample_2d(g_roughness_metallic_texture, g_surface_sampler, in_texcoord).yz * brx_float2(g_roughness_factor, g_metallic_factor);

        out_surface_roughness = surface_roughness_metallic.x;

        brx_float metallic = surface_roughness_metallic.y;

        // UE4: https://github.com/EpicGames/UnrealEngine/blob/4.21/Engine/Shaders/Private/MobileBasePassPixelShader.usf#L376
        const float dielectric_specular = 0.04;

        out_surface_specular_color = brx_clamp((dielectric_specular - dielectric_specular * metallic) + base_color * metallic, 0.0, 1.0);
        out_surface_diffuse_color = brx_clamp(base_color - base_color * metallic, 0.0, 1.0);
    }

    out_surface_emissive = brx_float3(0.0, 0.0, 0.0);

    return (out_surface_opacity > 0.5);
}

brx_int brx_vct_application_bridge_get_direct_lighting_count(in brx_float3 in_surface_position_world_space, in brx_float3 in_surface_shading_normal_world_space, in brx_float3 in_surface_diffuse_color, in brx_float3 in_surface_specular_color, in brx_float in_surface_roughness)
{
    return 1;
}

brx_float3 brx_vct_application_bridge_get_direct_lighting_illumination(in brx_int in_direct_lighting_index, in brx_float3 in_surface_position_world_space, in brx_float3 in_surface_shading_normal_world_space, in brx_float3 in_surface_diffuse_color, in brx_float3 in_surface_specular_color, in brx_float in_surface_roughness)
{
    brx_float3 incident_direction = brx_normalize(g_light_direction);

    brx_float3 N = in_surface_shading_normal_world_space;
    brx_float3 L = incident_direction;
    brx_float NdotL = brx_dot(N, L);

    brx_float3 incident_illumination;
    brx_branch if (NdotL > 0.0)
    {
        incident_illumination = g_light_color * get_shadow(in_surface_position_world_space);
    }
    else
    {
        incident_illumination = brx_float3(0.0, 0.0, 0.0);
    }

    return incident_illumination;
}

brx_float3 brx_vct_application_bridge_get_direct_lighting_radiance(in brx_int in_direct_lighting_index, in brx_float3 in_direct_lighting_illumination, in brx_float3 in_outgoing_direction_world_space, in brx_float3 in_surface_position_world_space, in brx_float3 in_surface_shading_normal_world_space, in brx_float3 in_surface_diffuse_color, in brx_float3 in_surface_specular_color, in brx_float in_surface_roughness)
{
    brx_float3 direct_lighting_radiance = brx_float3(0.0, 0.0, 0.0);

    brx_float3 incident_direction = brx_normalize(g_light_direction);

    brx_float3 N = in_surface_shading_normal_world_space;
    brx_float3 L = incident_direction;
    brx_float NdotL = brx_dot(N, L);

    brx_branch if (NdotL > 0.0)
    {
        brx_float3 incident_illumination = in_direct_lighting_illumination;

        const brx_float INTERNAL_BRX_ILLUMINATION_THRESHOLD = 1e-4;

        brx_branch if (brx_any(brx_greater_than(incident_illumination, brx_float3(INTERNAL_BRX_ILLUMINATION_THRESHOLD, INTERNAL_BRX_ILLUMINATION_THRESHOLD, INTERNAL_BRX_ILLUMINATION_THRESHOLD))))
        {
            brx_float3 V = in_outgoing_direction_world_space;
            brx_float NdotV = brx_dot(N, V);

            brx_float3 diffuse_brdf = brx_lambert_brdf(in_surface_diffuse_color);

            brx_float3 specular_brdf;
            brx_branch if (NdotV > 0.0)
            {
                brx_float VdotL = brx_dot(V, L);

                // Real-Time Rendering Fourth Edition / 9.8 BRDF Models for Surface Reflection / [Hammon 2017]
                // UE4: [Init](https://github.com/EpicGames/UnrealEngine/blob/4.27/Engine/Shaders/Private/BRDF.ush#L31)
                // U3D: [GetBSDFAngle](https://github.com/Unity-Technologies/Graphics/blob/v10.8.0/com.unity.render-pipelines.core/ShaderLibrary/CommonLighting.hlsl#L361)
                brx_float invLenH = brx_rsqrt(2.0 + 2.0 * VdotL);
                brx_float NdotH = brx_clamp((NdotL + NdotV) * invLenH, 0.0, 1.0);
                brx_float LdotH = brx_clamp(invLenH * VdotL + invLenH, 0.0, 1.0);

                brx_float VdotH = LdotH;

                // Real-Time Rendering Fourth Edition / 9.8.1 Normal Distribution Functions: "In the Disney principled shading model, Burley[214] exposes the surface_roughness control to users as g = r2, where r is the user-interface surface_roughness parameter value between 0 and 1."
                brx_float alpha = in_surface_roughness * in_surface_roughness;

                brx_float3 f0 = in_surface_specular_color;
                const brx_float3 f90 = brx_float3(1.0, 1.0, 1.0);

                specular_brdf = brx_trowbridge_reitz_brdf(alpha, NdotH, NdotV, NdotL, f0, f90, VdotH);
            }
            else
            {
                specular_brdf = brx_float3(0.0, 0.0, 0.0);
            }

            direct_lighting_radiance += (diffuse_brdf + specular_brdf) * (incident_illumination * NdotL);
        }
    }

    return direct_lighting_radiance;
}

brx_uint brx_vct_application_bridge_get_clipmap_mask(in brx_int3 in_mask_texture_coordinates)
{
    return brx_load_3d_uint(u_clipmap_texture_mask, in_mask_texture_coordinates);
}

brx_uint brx_vct_application_bridge_compare_and_swap_clipmap_mask(in brx_int3 in_mask_texture_coordinates, in brx_uint in_old_value, in brx_uint in_new_value)
{
    return brx_texture_3d_uint_interlocked_compare_exchange(u_clipmap_texture_mask, in_mask_texture_coordinates, in_old_value, in_new_value);
}

brx_uint brx_vct_application_bridge_get_clipmap_illumination_opacity_red_green(in brx_int3 in_illumination_texture_coordinates)
{
    return brx_load_3d_uint(u_clipmap_texture_illumination_opacity_red_green, in_illumination_texture_coordinates);
}

brx_uint brx_vct_application_bridge_compare_and_swap_clipmap_illumination_opacity_red_green(in brx_int3 in_illumination_texture_coordinates, in brx_uint in_old_value, in brx_uint in_new_value)
{
    return brx_texture_3d_uint_interlocked_compare_exchange(u_clipmap_texture_illumination_opacity_red_green, in_illumination_texture_coordinates, in_old_value, in_new_value);
}

brx_uint brx_vct_application_bridge_get_clipmap_illumination_opacity_blue_alpha(in brx_int3 in_illumination_texture_coordinates)
{
    return brx_load_3d_uint(u_clipmap_texture_illumination_opacity_blue_alpha, in_illumination_texture_coordinates);
}

brx_uint brx_vct_application_bridge_compare_and_swap_clipmap_illumination_opacity_blue_alpha(in brx_int3 in_illumination_texture_coordinates, in brx_uint in_old_value, in brx_uint in_new_value)
{
    return brx_texture_3d_uint_interlocked_compare_exchange(u_clipmap_texture_illumination_opacity_blue_alpha, in_illumination_texture_coordinates, in_old_value, in_new_value);
}
