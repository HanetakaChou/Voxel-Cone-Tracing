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

#ifndef _PACK_CLIPMAP_RESOURCE_BINDING_HLSLI_
#define _PACK_CLIPMAP_RESOURCE_BINDING_HLSLI_ 1

#if defined(__STDC__) || defined(__cplusplus)

#elif defined(HLSL_VERSION) || defined(__HLSL_VERSION)

Texture3D<uint4> t_clipmap_texture_mask : register(t0);
Texture3D t_clipmap_texture_opacity_r32 : register(t1);
Texture3D t_clipmap_texture_illumination_r32 : register(t2);
Texture3D t_clipmap_texture_illumination_g32 : register(t3);
Texture3D t_clipmap_texture_illumination_b32 : register(t4);

RWTexture3D<float4> u_clipmap_texture_opacity_r16 : register(u0);
RWTexture3D<float4> u_clipmap_texture_illumination_r16g16b16 : register(u1);

#else
#error Unknown Compiler
#endif

#endif
