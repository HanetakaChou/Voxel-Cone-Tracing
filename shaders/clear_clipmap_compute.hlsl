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
#include "../thirdparty/Voxel-Cone-Tracing/shaders/brx_voxel_cone_tracing_clear_compute.bsli"

#include "../thirdparty/Brioche-Shader-Language/shaders/brx_shader_language.bsli"

#include "clear_clipmap_resource_binding.hlsli"

brx_uint brx_vct_application_bridge_get_clipmap_mask(in brx_int3 in_mask_texture_coordinates)
{
    return brx_load_3d_uint(g_clipmap_texture_mask, in_mask_texture_coordinates);
}

void brx_vct_application_bridge_set_clipmap_opacity(in brx_int3 in_opacity_texture_coordinates, in brx_uint in_packed_opacity)
{
    brx_store_3d_uint(g_clipmap_texture_opacity, in_opacity_texture_coordinates, in_packed_opacity);
}

void brx_vct_application_bridge_set_clipmap_illumination(in brx_int3 in_illumination_texture_coordinates, in brx_uint in_packed_illumination)
{
    brx_store_3d_uint(g_clipmap_texture_illumination, in_illumination_texture_coordinates, in_packed_illumination);
}

void brx_vct_application_bridge_set_clipmap_mask(in brx_int3 in_mask_texture_coordinates, in brx_uint in_mask)
{
    brx_store_3d_uint(g_clipmap_texture_mask, in_mask_texture_coordinates, in_mask);
}
