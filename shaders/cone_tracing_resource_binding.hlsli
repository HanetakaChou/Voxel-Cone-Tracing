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

#ifndef _CONE_TRACING_RESOURCE_BINDING_HLSLI_
#define _CONE_TRACING_RESOURCE_BINDING_HLSLI_ 1

#if defined(__STDC__) || defined(__cplusplus)

struct cone_tracing_constant_buffer_binding
{
    DirectX::XMFLOAT4X4 inverse_view_transform;
    DirectX::XMFLOAT4X4 inverse_projection_transform;
    DirectX::XMFLOAT3 clipmap_anchor;
    uint32_t _unused_cone_tracing_constant_buffer_binding_padding_1;
    DirectX::XMFLOAT3 clipmap_center;
    uint32_t _unused_cone_tracing_constant_buffer_binding_padding_2;
};

#elif defined(HLSL_VERSION) || defined(__HLSL_VERSION)

cbuffer cone_tracing_constant_buffer_binding : register(b0)
{
    float4x4 g_inverse_view_transform;
    float4x4 g_inverse_projection_transform;
    float3 g_clipmap_anchor;
    uint _unused_cone_tracing_constant_buffer_binding_padding_1;
    float3 g_clipmap_center;
    uint _unused_cone_tracing_constant_buffer_binding_padding_2;
}

Texture2D t_gbuffer_base_color : register(t0);
Texture2D t_gbuffer_roughness_metallic : register(t1);
Texture2D t_gbuffer_normal : register(t2);
Texture2D t_scene_depth : register(t3);

Texture3D t_clipmap_texture_illumination_opacity_r16g16b16a16 : register(t4);

SamplerState s_sampler : register(s0);

RWTexture2D<float4> u_indirect_radiance_and_ambient_occlusion : register(u0);

#else
#error Unknown Compiler
#endif

#endif
