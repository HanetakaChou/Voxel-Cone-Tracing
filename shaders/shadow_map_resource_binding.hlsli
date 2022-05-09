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


#ifndef _SHADOW_MAP_RESOURCE_BINDING_HLSLI_
#define _SHADOW_MAP_RESOURCE_BINDING_HLSLI_ 1

#if defined(__STDC__) || defined(__cplusplus)

struct shadow_map_constant_buffer_binding
{
    DirectX::XMFLOAT4X4 view_transform;
    DirectX::XMFLOAT4X4 projection_transform;
    DirectX::XMFLOAT4X4 model_transform;
};

#elif defined(HLSL_VERSION) || defined(__HLSL_VERSION)

cbuffer shadow_map_constant_buffer_binding : register(b0)
{
    float4x4 g_view_transform;
    float4x4 g_projection_transform;
    float4x4 g_model_transform;
}

ByteAddressBuffer g_index_buffer : register(t1);
ByteAddressBuffer g_vertex_position_buffer : register(t2);

#else
#error Unknown Compiler
#endif

#endif
