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
#include "../thirdparty/Voxel-Cone-Tracing/shaders/brx_voxel_cone_tracing_voxelization_vertex.bsli"

#include "../thirdparty/Brioche-Shader-Language/shaders/brx_shader_language.bsli"
#include "../thirdparty/Brioche-Shader-Language/shaders/brx_packed_vector.bsli"
#include "../thirdparty/Brioche-Shader-Language/shaders/brx_octahedral_mapping.bsli"

#include "voxelization_resource_binding.hlsli"

#define SURFACE_VERTEX_POSITION_BUFFER_STRIDE 12u
#define SURFACE_VERTEX_VARYING_BUFFER_STRIDE 12u
#define SURFACE_UINT32_INDEX_BUFFER_STRIDE 4u

void brx_vct_application_bridge_get_triangle_vertices(in brx_uint in_triangle_index, out brx_uint3 out_triangle_vertex_indices, out brx_float3 out_triangle_vertices_position_model_space[3], out brx_float3 out_triangle_vertices_position_world_space[3], out brx_bool out_surface_material_face)
{
    brx_uint3 triangle_vertex_indices;
    {
        brx_uint index_buffer_offset = (SURFACE_UINT32_INDEX_BUFFER_STRIDE * 3u) * in_triangle_index;
        triangle_vertex_indices = brx_byte_address_buffer_load3(g_index_buffer, index_buffer_offset);
    }

    brx_float3 triangle_vertices_position_model_space[3];
    {
        brx_uint3 vertex_position_buffer_offset = SURFACE_VERTEX_POSITION_BUFFER_STRIDE * triangle_vertex_indices;

        brx_uint3 packed_vector_vertices_position_binding[3];
        packed_vector_vertices_position_binding[0] = brx_byte_address_buffer_load3(g_vertex_position_buffer, vertex_position_buffer_offset.x);
        packed_vector_vertices_position_binding[1] = brx_byte_address_buffer_load3(g_vertex_position_buffer, vertex_position_buffer_offset.y);
        packed_vector_vertices_position_binding[2] = brx_byte_address_buffer_load3(g_vertex_position_buffer, vertex_position_buffer_offset.z);

        triangle_vertices_position_model_space[0] = brx_uint_as_float(packed_vector_vertices_position_binding[0]);
        triangle_vertices_position_model_space[1] = brx_uint_as_float(packed_vector_vertices_position_binding[1]);
        triangle_vertices_position_model_space[2] = brx_uint_as_float(packed_vector_vertices_position_binding[2]);
    }

    brx_float3 triangle_vertices_position_world_space[3];
    {
        triangle_vertices_position_world_space[0] = brx_mul(g_model_transform, brx_float4(triangle_vertices_position_model_space[0], 1.0)).xyz;
        triangle_vertices_position_world_space[1] = brx_mul(g_model_transform, brx_float4(triangle_vertices_position_model_space[1], 1.0)).xyz;
        triangle_vertices_position_world_space[2] = brx_mul(g_model_transform, brx_float4(triangle_vertices_position_model_space[2], 1.0)).xyz;
    }

    out_triangle_vertex_indices = triangle_vertex_indices;

    out_triangle_vertices_position_model_space[0] = triangle_vertices_position_model_space[0];
    out_triangle_vertices_position_model_space[1] = triangle_vertices_position_model_space[1];
    out_triangle_vertices_position_model_space[2] = triangle_vertices_position_model_space[2];

    out_triangle_vertices_position_world_space[0] = triangle_vertices_position_world_space[0];
    out_triangle_vertices_position_world_space[1] = triangle_vertices_position_world_space[1];
    out_triangle_vertices_position_world_space[2] = triangle_vertices_position_world_space[2];

    out_surface_material_face = false;
}

void brx_vct_application_bridge_get_vertex(in brx_uint in_triangle_vertex_index, in brx_uint3 in_triangle_vertex_indices, in brx_float3 in_triangle_vertices_position_model_space[3], in brx_float3 in_triangle_vertices_position_world_space[3], in brx_bool in_surface_material_face, out brx_float3 out_vertex_normal_world_space, out brx_float4 out_vertex_tangent_world_space, out brx_float2 out_vertex_texcoord, out brx_float3 out_vertex_position_non_deformed_model_space)
{
    brx_uint vertex_index;
    brx_branch if (0 == in_triangle_vertex_index)
    {
        vertex_index = in_triangle_vertex_indices.x;
    }
    else if (1 == in_triangle_vertex_index)
    {
        vertex_index = in_triangle_vertex_indices.y;
    }
    else
    {
        vertex_index = in_triangle_vertex_indices.z;
    }

    brx_float3 vertex_normal_model_space;
    brx_float4 vertex_tangent_model_space;
    brx_float2 vertex_texcoord;
    {
        brx_uint vertex_varying_buffer_offset = SURFACE_VERTEX_VARYING_BUFFER_STRIDE * vertex_index;
        brx_uint3 packed_vector_vertex_varying_binding = brx_byte_address_buffer_load3(g_vertex_varying_buffer, vertex_varying_buffer_offset);
        vertex_normal_model_space = brx_octahedral_unmap(brx_R16G16_SNORM_to_FLOAT2(packed_vector_vertex_varying_binding.x));
        brx_float3 vertex_mapped_tangent_model_space = brx_R15G15B2_SNORM_to_FLOAT3(packed_vector_vertex_varying_binding.y);
        vertex_tangent_model_space = brx_float4(brx_octahedral_unmap(vertex_mapped_tangent_model_space.xy), vertex_mapped_tangent_model_space.z);
        // https://github.com/AcademySoftwareFoundation/Imath/blob/main/src/Imath/half.h
        // HALF_MAX 65504.0
        vertex_texcoord = brx_clamp(brx_unpack_half2(packed_vector_vertex_varying_binding.z), brx_float2(-65504.0, -65504.0), brx_float2(65504.0, 65504.0));
    }

    out_vertex_normal_world_space = brx_mul(g_model_transform, brx_float4(vertex_normal_model_space, 0.0)).xyz;
    out_vertex_tangent_world_space = brx_float4(brx_mul(g_model_transform, brx_float4(vertex_tangent_model_space.xyz, 0.0)).xyz, vertex_tangent_model_space.w);
    out_vertex_texcoord = vertex_texcoord;
    out_vertex_position_non_deformed_model_space = brx_float3(0.0, 0.0, 0.0);
}

brx_float4x4 brx_vct_application_bridge_get_viewport_depth_direction_view_matrix(in brx_int in_viewport_depth_direction_index)
{
    return g_viewport_depth_direction_view_matrices[in_viewport_depth_direction_index];
}

brx_float4x4 brx_vct_application_bridge_get_clipmap_stack_level_projection_matrix(in brx_int in_clipmap_stack_level_index)
{
    return g_clipmap_stack_level_projection_matrices[in_clipmap_stack_level_index];
}
