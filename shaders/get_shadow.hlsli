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

#ifndef _GET_SHADOW_MAP_HLSLI_
#define _GET_SHADOW_MAP_HLSLI_ 1

brx_float get_shadow(brx_float3 surface_position_world_space)
{
    // Poisson
    const brx_float2 g_sample_positions[16] = 
        brx_array_constructor_begin(brx_float2, 16)
        brx_float2(-0.3935238, 0.7530643) brx_array_constructor_split
        brx_float2(-0.3022015, 0.297664) brx_array_constructor_split
        brx_float2(0.09813362, 0.192451) brx_array_constructor_split
        brx_float2(-0.7593753, 0.518795) brx_array_constructor_split
        brx_float2(0.2293134, 0.7607011) brx_array_constructor_split
        brx_float2(0.6505286, 0.6297367) brx_array_constructor_split
        brx_float2(0.5322764, 0.2350069) brx_array_constructor_split
        brx_float2(0.8581018, -0.01624052) brx_array_constructor_split
        brx_float2(-0.6928226, 0.07119545) brx_array_constructor_split
        brx_float2(-0.3114384, -0.3017288) brx_array_constructor_split
        brx_float2(0.2837671, -0.179743) brx_array_constructor_split
        brx_float2(-0.3093514, -0.749256) brx_array_constructor_split
        brx_float2(-0.7386893, -0.5215692) brx_array_constructor_split
        brx_float2(0.3988827, -0.617012) brx_array_constructor_split
        brx_float2(0.8114883, -0.458026) brx_array_constructor_split
        brx_float2(0.08265103, -0.8939569)
        brx_array_constructor_end;

    // https://github.com/KhronosGroup/glTF-Sample-Models/blob/main/2.0/Sponza/glTF/Sponza.gltf#L8558
    // https://github.com/KhronosGroup/glTF-Sample-Assets/blob/main/Models/Sponza/glTF/Sponza.gltf#L8558
    surface_position_world_space += g_light_direction * 0.00800000037997961F;

    brx_float3 view_position = brx_mul(g_light_view_transform, brx_float4(surface_position_world_space, 1.0)).xyz;
    brx_float4 clip_position = brx_mul(g_light_projection_transform, brx_float4(view_position, 1.0));

    brx_float3 shadow_map_position;
    {
        brx_float3 temp = clip_position.xyz / clip_position.w;
        shadow_map_position = brx_float3(temp.xy * brx_float2(0.5F, -0.5F) + brx_float2(0.5F, 0.5F), temp.z);
    }

    brx_float shadow = 0.0;
    brx_unroll for (brx_int sample_index = 0; sample_index < 16; ++sample_index)
    {
        brx_float2 offset = g_sample_positions[sample_index] * (2.0 / g_shadow_map_size);
        brx_float sample = g_shadow_map.SampleCmpLevelZero(g_shadow_sampler, shadow_map_position.xy + offset, shadow_map_position.z);
        shadow += (sample * (1.0 / 16.0));
    }

    return shadow;
}


#endif
