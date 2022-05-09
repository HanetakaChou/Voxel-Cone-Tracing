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
#include "../thirdparty/Voxel-Cone-Tracing/shaders/brx_voxel_cone_tracing_zero_compute.bsli"

#include "../thirdparty/Brioche-Shader-Language/shaders/brx_shader_language.bsli"

#include "zero_clipmap_resource_binding.hlsli"

brx_int3 brx_vct_application_bridge_get_clipmap_mask_dimension()
{
    return brx_read_write_texture_3d_uint_get_dimension(u_clipmap_texture_mask);
}

void brx_vct_application_bridge_set_clipmap_mask(in brx_int3 in_mask_texture_coordinates, in brx_uint in_mask)
{
    brx_store_3d_uint(u_clipmap_texture_mask, in_mask_texture_coordinates, in_mask);
}

brx_int3 brx_vct_application_bridge_get_clipmap_opacity_r32_dimension()
{
    return brx_write_only_texture_3d_uint_get_dimension(u_clipmap_texture_opacity_r32);
}

void brx_vct_application_bridge_set_clipmap_opacity_r32(in brx_int3 in_opacity_texture_coordinates, in brx_uint in_packed_opacity)
{
    brx_store_3d_uint(u_clipmap_texture_opacity_r32, in_opacity_texture_coordinates, in_packed_opacity);
}

brx_int3 brx_vct_application_bridge_get_clipmap_opacity_r16_dimension()
{
    return brx_write_only_texture_3d_float4_get_dimension(u_clipmap_texture_opacity_r16);
}

void brx_vct_application_bridge_set_clipmap_opacity_r16(in brx_int3 in_opacity_texture_coordinates, in brx_float in_packed_opacity)
{
    brx_store_3d_float4(u_clipmap_texture_opacity_r16, in_opacity_texture_coordinates, brx_float4(in_packed_opacity, 0.0, 0.0, 0.0));
}

brx_int3 brx_vct_application_bridge_get_clipmap_illumination_r32_dimension()
{
    return brx_write_only_texture_3d_uint_get_dimension(u_clipmap_texture_illumination_r32);
}

void brx_vct_application_bridge_set_clipmap_illumination_r32(in brx_int3 in_illumination_texture_coordinates, in brx_uint in_packed_illumination)
{
    brx_store_3d_uint(u_clipmap_texture_illumination_r32, in_illumination_texture_coordinates, in_packed_illumination);
}

brx_int3 brx_vct_application_bridge_get_clipmap_illumination_g32_dimension()
{
    return brx_write_only_texture_3d_uint_get_dimension(u_clipmap_texture_illumination_g32);
}

void brx_vct_application_bridge_set_clipmap_illumination_g32(in brx_int3 in_illumination_texture_coordinates, in brx_uint in_packed_illumination)
{
    brx_store_3d_uint(u_clipmap_texture_illumination_g32, in_illumination_texture_coordinates, in_packed_illumination);
}

brx_int3 brx_vct_application_bridge_get_clipmap_illumination_b32_dimension()
{
    return brx_write_only_texture_3d_uint_get_dimension(u_clipmap_texture_illumination_b32);
}

void brx_vct_application_bridge_set_clipmap_illumination_b32(in brx_int3 in_illumination_texture_coordinates, in brx_uint in_packed_illumination)
{
    brx_store_3d_uint(u_clipmap_texture_illumination_b32, in_illumination_texture_coordinates, in_packed_illumination);
}

brx_int3 brx_vct_application_bridge_get_clipmap_illumination_r16g16b16_dimension()
{
    return brx_write_only_texture_3d_float4_get_dimension(u_clipmap_texture_illumination_r16g16b16);
}

void brx_vct_application_bridge_set_clipmap_illumination_r16g16b16(in brx_int3 in_illumination_texture_coordinates, in brx_float3 in_packed_illumination)
{
    brx_store_3d_float4(u_clipmap_texture_illumination_r16g16b16, in_illumination_texture_coordinates, brx_float4(0.0, 0.0, 0.0, 0.0));
}
