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


#ifndef _SURFACE_RESOURCE_BINDING_HLSLI_
#define _SURFACE_RESOURCE_BINDING_HLSLI_ 1

#if defined(__STDC__) || defined(__cplusplus)

struct surface_constant_buffer_binding
{
    DirectX::XMFLOAT4 base_color_factor;
    float roughness_factor;
    float metallic_factor;
    uint32_t _unused_surface_constant_buffer_binding_padding_1;
    uint32_t _unused_surface_constant_buffer_binding_padding_2;
};

#elif defined(HLSL_VERSION) || defined(__HLSL_VERSION)

cbuffer surface_constant_buffer_binding : register(b1)
{
    float4 g_base_color_factor;
    float g_roughness_factor;
    float g_metallic_factor;
    uint _unused_surface_constant_buffer_binding_padding_1;
    uint _unused_surface_constant_buffer_binding_padding_2;
}

#else
#error Unknown Compiler
#endif

#endif
