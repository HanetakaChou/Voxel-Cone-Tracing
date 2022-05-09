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

#ifndef _VOXELIZATION_RESOURCE_BINDING_HLSLI_
#define _VOXELIZATION_RESOURCE_BINDING_HLSLI_ 1

#include "../thirdparty/Voxel-Cone-Tracing/include/brx_voxel_cone_tracing.h"

#if defined(__STDC__) || defined(__cplusplus)

struct voxelization_constant_buffer_binding
{
    DirectX::XMFLOAT4X4 viewport_depth_direction_view_matrices[BRX_VCT_VIEWPORT_DEPTH_DIRECTION_COUNT];
    DirectX::XMFLOAT4X4 clipmap_stack_level_projection_matrices[BRX_VCT_CLIPMAP_STACK_LEVEL_COUNT];
    DirectX::XMFLOAT4X4 model_transform;
    DirectX::XMFLOAT4X4 light_view_transform;
    DirectX::XMFLOAT4X4 light_projection_transform;
    DirectX::XMFLOAT3 light_direction;
    float shadow_map_size;
    DirectX::XMFLOAT3 light_color;
    uint32_t _unused_voxelization_constant_buffer_binding_padding_1;
};

#elif defined(HLSL_VERSION) || defined(__HLSL_VERSION)

cbuffer voxelization_constant_buffer_binding : register(b0)
{
    float4x4 g_viewport_depth_direction_view_matrices[BRX_VCT_VIEWPORT_DEPTH_DIRECTION_COUNT];
    float4x4 g_clipmap_stack_level_projection_matrices[BRX_VCT_CLIPMAP_STACK_LEVEL_COUNT];
    float4x4 g_model_transform;
    float4x4 g_light_view_transform;
    float4x4 g_light_projection_transform;
    float3 g_light_direction;
    float g_shadow_map_size;
    float3 g_light_color;
    uint _unused_voxelization_constant_buffer_binding_padding_1;
}

Texture2D g_shadow_map : register(t0);

ByteAddressBuffer g_index_buffer : register(t1);
ByteAddressBuffer g_vertex_position_buffer : register(t2);
ByteAddressBuffer g_vertex_varying_buffer : register(t3);

Texture2D g_normal_texture : register(t4);
Texture2D g_base_color_texture : register(t5);
Texture2D g_roughness_metallic_texture : register(t6);

SamplerState g_surface_sampler : register(s0);
SamplerComparisonState g_shadow_sampler : register(s1);

RWTexture3D<uint> u_clipmap_texture_mask : register(u0);
RWTexture3D<uint> u_clipmap_texture_illumination_opacity_red_green : register(u1);
RWTexture3D<uint> u_clipmap_texture_illumination_opacity_blue_alpha : register(u2);

#else
#error Unknown Compiler
#endif

#endif
