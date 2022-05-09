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

#include "../thirdparty/Brioche-Shader-Language/shaders/brx_shader_language.bsli"
#include "../thirdparty/Brioche-Shader-Language/shaders/brx_brdf.bsli"
#include "../thirdparty/Brioche-Shader-Language/shaders/brx_octahedral_mapping.bsli"

#include "forward_shading_resource_binding.hlsli"
#include "surface_resource_binding.hlsli"
#include "get_shadow.hlsli"

brx_early_depth_stencil
brx_pixel_shader_parameter_begin(main)
brx_pixel_shader_parameter_in_frag_coord brx_pixel_shader_parameter_split
brx_pixel_shader_parameter_in(brx_float3, in_interpolated_position_world_space, 0) brx_pixel_shader_parameter_split
brx_pixel_shader_parameter_in(brx_float3, in_interpolated_normal, 1) brx_pixel_shader_parameter_split
brx_pixel_shader_parameter_in(brx_float4, in_interpolated_tangent, 2) brx_pixel_shader_parameter_split
brx_pixel_shader_parameter_in(brx_float2, in_interpolated_texcoord, 3) brx_pixel_shader_parameter_split
brx_pixel_shader_parameter_out(brx_float4, out_direct_radiance, 0) brx_pixel_shader_parameter_split
brx_pixel_shader_parameter_out(brx_float4, out_gbuffer_base_color, 1) brx_pixel_shader_parameter_split
brx_pixel_shader_parameter_out(brx_float4, out_gbuffer_roughness_metallic, 2) brx_pixel_shader_parameter_split
brx_pixel_shader_parameter_out(brx_float4, out_gbuffer_normal, 3)
brx_pixel_shader_parameter_end(main)
{
    brx_float3 surface_shading_normal_world_space;
    brx_float3 surface_diffuse_color;
    brx_float3 surface_specular_color;
    brx_float3 surface_base_color;
    brx_float surface_metallic;
    brx_float surface_roughness;
    brx_float surface_opacity;
    {
        {
            brx_float3 geometry_normal_world_space = brx_normalize(in_interpolated_normal);

            brx_float3 tangent_world_space = brx_normalize(in_interpolated_tangent.xyz);

            brx_float3 bitangent_world_space = brx_cross(geometry_normal_world_space, tangent_world_space) * ((in_interpolated_tangent.w >= 0.0) ? 1.0 : -1.0);

            brx_float3 shading_normal_tangent_space = brx_normalize(brx_sample_2d(g_normal_texture, g_surface_sampler, in_interpolated_texcoord).xyz * 2.0 - brx_float3(1.0, 1.0, 1.0));

            surface_shading_normal_world_space = brx_normalize(tangent_world_space * shading_normal_tangent_space.x + bitangent_world_space * shading_normal_tangent_space.y + geometry_normal_world_space * shading_normal_tangent_space.z);
        }

        {
            brx_float4 base_color_and_opacity = brx_sample_2d(g_base_color_texture, g_surface_sampler, in_interpolated_texcoord) * g_base_color_factor;

            surface_base_color = base_color_and_opacity.xyz;

            surface_opacity = base_color_and_opacity.w;
        }

        {
            brx_float2 roughness_metallic = brx_sample_2d(g_roughness_metallic_texture, g_surface_sampler, in_interpolated_texcoord).yz * brx_float2(g_roughness_factor, g_metallic_factor);

            surface_roughness = roughness_metallic.x;

            surface_metallic = roughness_metallic.y;
        }

        {
            // UE4: https://github.com/EpicGames/UnrealEngine/blob/4.21/Engine/Shaders/Private/MobileBasePassPixelShader.usf#L376
            const brx_float dielectric_specular = 0.04;

            surface_specular_color = brx_clamp((dielectric_specular - dielectric_specular * surface_metallic) + surface_base_color * surface_metallic, 0.0, 1.0);
            surface_diffuse_color = brx_clamp(surface_base_color - surface_base_color * surface_metallic, 0.0, 1.0);
        }
    }

    // TODO: can we still use brx_early_depth_stencil
    brx_branch if (surface_opacity < 0.5)
    {
        discard;
    }

    brx_float3 surface_position_world_space = in_interpolated_position_world_space;

    brx_float3 camera_ray_direction;
    {
        brx_float3 camera_ray_origin = brx_mul(g_inverse_view_transform, brx_float4(0.0, 0.0, 0.0, 1.0)).xyz;
        camera_ray_direction = brx_normalize(surface_position_world_space - camera_ray_origin);
    }

    brx_float3 radiance;
    {
        const brx_float INTERNAL_BRX_ILLUMINATION_THRESHOLD = 1e-4;
        const brx_float INTERNAL_HALF_VECTOR_LENGTH_SQUARE_MINIMUM = 1e-8;

        brx_float3 N = surface_shading_normal_world_space;
        brx_float3 L = brx_normalize(g_light_direction);
        brx_float NdotL = brx_dot(N, L);

        brx_branch if (NdotL > 0.0)
        {
            brx_float3 E_l = g_light_color * get_shadow(surface_position_world_space);
            brx_float3 E_n = E_l * NdotL;

            brx_branch if (brx_all(E_n > brx_float3(INTERNAL_BRX_ILLUMINATION_THRESHOLD, INTERNAL_BRX_ILLUMINATION_THRESHOLD, INTERNAL_BRX_ILLUMINATION_THRESHOLD)))
            {
                brx_float3 V = -camera_ray_direction;
                brx_float NdotV = brx_dot(N, V);

                brx_branch if (NdotV > 0.0)
                {
                    brx_float3 diffuse_brdf = brx_lambert_brdf(surface_diffuse_color);

                    brx_float3 specular_brdf;
                    brx_branch if (NdotV > 0.0)
                    {
                        brx_float square_NdotH;
                        brx_float LdotH;
                        {
#if 0
                            brx_float VdotL = brx_dot(V, L);

                            // Real-Time Rendering Fourth Edition / 9.8 BRDF Models for Surface Reflection / [Hammon 2017]
                            // UE4: [Init](https://github.com/EpicGames/UnrealEngine/blob/4.27/Engine/Shaders/Private/BRDF.ush#L31)
                            // U3D: [GetBSDFAngle](https://github.com/Unity-Technologies/Graphics/blob/v10.8.0/com.unity.render-pipelines.core/ShaderLibrary/CommonLighting.hlsl#L361)
                            brx_float invLenH = brx_rsqrt(brx_max(INTERNAL_HALF_VECTOR_LENGTH_SQUARE_MINIMUM, 2.0 + 2.0 * VdotL));
                            brx_float NdotH = brx_clamp((NdotL + NdotV) * invLenH, 0.0, 1.0);
                            LdotH = brx_clamp(invLenH * VdotL + invLenH, 0.0, 1.0);

                            square_NdotH = NdotH * NdotH;
#else
                            // [BrdfGGX::eval](https://github.com/selfshadow/ltc_code/blob/master/fit/brdf_ggx.h#L34)
                            // The precision is higher, likely because "normalize" and "sqrt" are omitted
                            brx_float3 not_normalized_H = V + L;
                            brx_float not_normalized_NdotH = brx_dot(N, not_normalized_H);
                            brx_float square_not_normalized_NdotH = not_normalized_NdotH * not_normalized_NdotH;
                            brx_float square_length_not_normalized_H = brx_dot(not_normalized_H, not_normalized_H);

                            brx_branch if ((not_normalized_NdotH > 0.0) && (square_length_not_normalized_H > INTERNAL_HALF_VECTOR_LENGTH_SQUARE_MINIMUM))
                            {
                                square_NdotH = square_not_normalized_NdotH / square_length_not_normalized_H;
                            }
                            else
                            {
                                square_NdotH = 0.0F;
                            }

                            brx_branch if (square_length_not_normalized_H > INTERNAL_HALF_VECTOR_LENGTH_SQUARE_MINIMUM)
                            {
                                LdotH = brx_clamp(brx_dot(L, not_normalized_H * brx_rsqrt(square_length_not_normalized_H)), 0.0, 1.0);
                            }
                            else
                            {
                                LdotH = 0.0F;
                            }
#endif
                        }

                        brx_float VdotH = LdotH;

                        // Real-Time Rendering Fourth Edition / 9.8.1 Normal Distribution Functions: "In the Disney principled shading model, Burley[214] exposes the surface_roughness control to users as g = r2, where r is the user-interface surface_roughness parameter value between 0 and 1."
                        brx_float alpha = surface_roughness * surface_roughness;

                        brx_float3 f0 = surface_specular_color;
                        const brx_float3 f90 = brx_float3(1.0, 1.0, 1.0);

                        specular_brdf = brx_trowbridge_reitz_brdf_square_version(alpha, square_NdotH, NdotV, NdotL, f0, f90, VdotH);
                    }
                    else
                    {
                        specular_brdf = brx_float3(0.0, 0.0, 0.0);
                    }

                    radiance = (diffuse_brdf + specular_brdf) * E_n;
                }
                else
                {
                    radiance = brx_float3(0.0, 0.0, 0.0);
                }
            }
            else
            {
                radiance = brx_float3(0.0, 0.0, 0.0);
            }
        }
        else
        {
            radiance = brx_float3(0.0, 0.0, 0.0);
        }
    }

    out_direct_radiance = brx_float4(radiance, 1.0);

    out_gbuffer_base_color = brx_float4(surface_base_color, 1.0);
    out_gbuffer_roughness_metallic = brx_float4(surface_roughness, surface_metallic, 0.0, 0.0);
    out_gbuffer_normal = brx_float4(brx_octahedral_map(surface_shading_normal_world_space), 0.0, 0.0);
}