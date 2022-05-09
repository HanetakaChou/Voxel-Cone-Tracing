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

#ifndef _POST_PROCESSING_RESOURCE_BINDING_HLSLI_
#define _POST_PROCESSING_RESOURCE_BINDING_HLSLI_ 1

#if defined(__STDC__) || defined(__cplusplus)

#elif defined(HLSL_VERSION) || defined(__HLSL_VERSION)

Texture2D<float4> g_direct_radiance : register(t0);
Texture2D<float4> g_indirect_radiance_and_ambient_occlusion : register(t1);

SamplerState g_sampler : register(s0);

#else
#error Unknown Compiler
#endif

#endif
