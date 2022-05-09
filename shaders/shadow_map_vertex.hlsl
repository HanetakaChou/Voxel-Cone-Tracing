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
#include "../thirdparty/Brioche-Shader-Language/shaders/brx_packed_vector.bsli"
#include "../thirdparty/Brioche-Shader-Language/shaders/brx_octahedral_mapping.bsli"

#include "shadow_map_resource_binding.hlsli"

#define SURFACE_VERTEX_POSITION_BUFFER_STRIDE 12u
#define SURFACE_VERTEX_VARYING_BUFFER_STRIDE 12u
#define SURFACE_UINT32_INDEX_BUFFER_STRIDE 4u

brx_vertex_shader_parameter_begin(main)
brx_vertex_shader_parameter_in_vertex_index brx_vertex_shader_parameter_split
brx_vertex_shader_parameter_out_position
brx_vertex_shader_parameter_end(main)
{
    brx_uint vertex_index;
    {
        brx_uint index_buffer_offset = SURFACE_UINT32_INDEX_BUFFER_STRIDE * brx_uint(brx_vertex_index);
        vertex_index = brx_byte_address_buffer_load(g_index_buffer, index_buffer_offset);
    }

    brx_float3 vertex_position_model_space;
    {
        brx_uint vertex_position_buffer_offset = SURFACE_VERTEX_POSITION_BUFFER_STRIDE * vertex_index;
        brx_uint3 packed_vector_vertex_position_binding = brx_byte_address_buffer_load3(g_vertex_position_buffer, vertex_position_buffer_offset);
        vertex_position_model_space = brx_uint_as_float(packed_vector_vertex_position_binding);
    }

    brx_float3 vertex_position_world_space = brx_mul(g_model_transform, brx_float4(vertex_position_model_space, 1.0)).xyz;
    brx_float3 vertex_position_view_space =  brx_mul(g_view_transform, brx_float4(vertex_position_world_space, 1.0)).xyz;
    brx_float4 vertex_position_clip_space = brx_mul(g_projection_transform, brx_float4(vertex_position_view_space, 1.0));

    brx_position = vertex_position_clip_space;
}
