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
#include "../thirdparty/Voxel-Cone-Tracing/shaders/brx_voxel_cone_tracing_pack_compute.bsli"

#include "../thirdparty/Brioche-Shader-Language/shaders/brx_shader_language.bsli"

#include "pack_clipmap_resource_binding.hlsli"

brx_uint brx_vct_application_bridge_get_clipmap_mask(in brx_int3 in_mask_texture_coordinates)
{
    return brx_fetch_3d(t_clipmap_texture_mask, brx_int4(in_mask_texture_coordinates, 0)).x;
}

brx_uint brx_vct_application_bridge_get_clipmap_illumination_opacity_r16g16(in brx_int3 in_illumination_opacity_texture_coordinates)
{
    return brx_fetch_3d(t_clipmap_texture_illumination_opacity_r16g16, brx_int4(in_illumination_opacity_texture_coordinates, 0)).x;
}

brx_uint brx_vct_application_bridge_get_clipmap_illumination_opacity_b16a16(in brx_int3 in_illumination_opacity_texture_coordinates)
{
    return brx_fetch_3d(t_clipmap_texture_illumination_opacity_b16a16, brx_int4(in_illumination_opacity_texture_coordinates, 0)).x;
}

void brx_vct_application_bridge_set_clipmap_illumination_opacity_r16g16b16a16(in brx_int3 in_illumination_opacity_texture_coordinates, in brx_float4 in_illumination_opacity)
{
    brx_store_3d_float4(u_clipmap_texture_illumination_opacity_r16g16b16a16, in_illumination_opacity_texture_coordinates, in_illumination_opacity);
}
