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

#include "post_processing_resource_binding.hlsli"

brx_early_depth_stencil
brx_pixel_shader_parameter_begin(main)
brx_pixel_shader_parameter_in_frag_coord brx_pixel_shader_parameter_split
brx_pixel_shader_parameter_in(brx_float2, in_interpolated_uv, 0) brx_pixel_shader_parameter_split
brx_pixel_shader_parameter_out(brx_float4, out_display_color, 0)
brx_pixel_shader_parameter_end(main)
{
    brx_float3 direct_radiance = brx_fetch_2d(g_direct_radiance, brx_int3(brx_frag_coord.xy, 0)).xyz;

    brx_float3 indirect_radiance;
    brx_float ambient_occlusion;
    {
        brx_float4 indirect_radiance_and_ambient_occlusion = brx_sample_2d(g_indirect_radiance_and_ambient_occlusion, g_sampler, in_interpolated_uv);
        indirect_radiance = indirect_radiance_and_ambient_occlusion.xyz;
        ambient_occlusion = indirect_radiance_and_ambient_occlusion.w;
    }

    // TODO
    brx_float3 diffuse_environment_lighting = brx_float3(0.01, 0.01, 0.01);

    brx_float3 scene_color = direct_radiance + indirect_radiance + diffuse_environment_lighting * ambient_occlusion;

    out_display_color = brx_float4(brx_pow(brx_max(brx_float3(0.0, 0.0, 0.0), scene_color), (1.0 / 2.2)), 1.0);
}
