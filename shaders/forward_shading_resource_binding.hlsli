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

#ifndef _FORWARD_SHADING_RESOURCE_BINDING_BSLI_
#define _FORWARD_SHADING_RESOURCE_BINDING_BSLI_ 1

#if defined(__STDC__) || defined(__cplusplus)

struct forward_shading_constant_buffer_binding
{
    DirectX::XMFLOAT4X4 view_transform;
    DirectX::XMFLOAT4X4 projection_transform;
    DirectX::XMFLOAT4X4 inverse_view_transform;
    DirectX::XMFLOAT4X4 model_transform;
    DirectX::XMFLOAT4X4 light_view_transform;
    DirectX::XMFLOAT4X4 light_projection_transform;
    DirectX::XMFLOAT3 light_direction;
    float shadow_map_size;
    DirectX::XMFLOAT3 light_color;
    uint32_t _unused_forward_shading_constant_buffer_binding_padding_1;
};

#elif defined(HLSL_VERSION) || defined(__HLSL_VERSION)

cbuffer forward_shading_constant_buffer_binding : register(b0)
{
    float4x4 g_view_transform;
    float4x4 g_projection_transform;
    float4x4 g_inverse_view_transform;
    float4x4 g_model_transform;
    float4x4 g_light_view_transform;
    float4x4 g_light_projection_transform;
    float3 g_light_direction;
    float g_shadow_map_size;
    float3 g_light_color;
    uint _unused_forward_shading_constant_buffer_binding_padding_1;
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

#else
#error Unknown Compiler
#endif

#endif
