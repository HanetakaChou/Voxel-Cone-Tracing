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

#ifndef _BRX_VOXEL_CONE_TRACING_RESOURCE_H_
#define _BRX_VOXEL_CONE_TRACING_RESOURCE_H_ 1

#include "brx_voxel_cone_tracing.h"

static inline DirectX::XMUINT3 brx_voxel_cone_tracing_resource_clipmap_mask_texture_extent()
{
    return DirectX::XMUINT3(
        static_cast<uint32_t>(BRX_VCT_CLIPMAP_MAP_SIZE),
        static_cast<uint32_t>(BRX_VCT_CLIPMAP_MAP_SIZE),
        static_cast<uint32_t>(BRX_VCT_CLIPMAP_MAP_SIZE));
}

static inline DirectX::XMUINT3 brx_voxel_cone_tracing_resource_clipmap_illumination_opacity_texture_extent()
{
    return DirectX::XMUINT3(
        (static_cast<uint32_t>(BRX_VCT_CLIPMAP_MAP_SIZE) + static_cast<uint32_t>(BRX_VCT_CLIPMAP_MARGIN) + static_cast<uint32_t>(BRX_VCT_CLIPMAP_MARGIN)) * BRX_VCT_CLIPMAP_ILLUMINATION_TEXTURE_DIRECTION_WIDTH_COUNT,
        (static_cast<uint32_t>(BRX_VCT_CLIPMAP_MAP_SIZE) + static_cast<uint32_t>(BRX_VCT_CLIPMAP_MARGIN) + static_cast<uint32_t>(BRX_VCT_CLIPMAP_MARGIN)) * BRX_VCT_CLIPMAP_ILLUMINATION_TEXTURE_DIRECTION_HEIGHT_COUNT,
        ((static_cast<uint32_t>(BRX_VCT_CLIPMAP_MAP_SIZE) + static_cast<uint32_t>(BRX_VCT_CLIPMAP_MARGIN) + static_cast<uint32_t>(BRX_VCT_CLIPMAP_MARGIN)) * (static_cast<uint32_t>(BRX_VCT_CLIPMAP_STACK_LEVEL_COUNT) - 1) + ((static_cast<uint32_t>(BRX_VCT_CLIPMAP_MAP_SIZE) - (static_cast<uint32_t>(BRX_VCT_CLIPMAP_MAP_SIZE) >> (static_cast<uint32_t>(BRX_VCT_CLIPMAP_MIP_LEVEL_COUNT) + 1))) << 1) + (static_cast<uint32_t>(BRX_VCT_CLIPMAP_MARGIN) + static_cast<uint32_t>(BRX_VCT_CLIPMAP_MARGIN)) * (static_cast<uint32_t>(BRX_VCT_CLIPMAP_MIP_LEVEL_COUNT) + 1)));
}

static inline DirectX::XMFLOAT3 brx_voxel_cone_tracing_resource_clipmap_anchor(DirectX::XMFLOAT3 const &in_eye_position, DirectX::XMFLOAT3 const &in_eye_direction)
{
    DirectX::XMFLOAT3 clipmap_anchor;
    DirectX::XMStoreFloat3(&clipmap_anchor, DirectX::XMVectorAdd(DirectX::XMLoadFloat3(&in_eye_position), DirectX::XMVectorScale(DirectX::XMLoadFloat3(&in_eye_direction), (static_cast<float>(BRX_VCT_CLIPMAP_FINEST_VOXEL_SIZE) * static_cast<float>(BRX_VCT_CLIPMAP_MAP_SIZE) * 0.5F))));
    return clipmap_anchor;
}

static inline DirectX::XMFLOAT3 brx_voxel_cone_tracing_resource_clipmap_center(DirectX::XMFLOAT3 const &in_clipmap_anchor)
{
    float const coarsest_stack_level_voxel_size = static_cast<float>(BRX_VCT_CLIPMAP_FINEST_VOXEL_SIZE) * static_cast<float>(1U << (static_cast<uint32_t>(BRX_VCT_CLIPMAP_STACK_LEVEL_COUNT) - 1U));

    DirectX::XMFLOAT3 clipmap_center;
    DirectX::XMStoreFloat3(&clipmap_center, DirectX::XMVectorScale(DirectX::XMVectorRound(DirectX::XMVectorScale(DirectX::XMLoadFloat3(&in_clipmap_anchor), 1.0F / coarsest_stack_level_voxel_size)), coarsest_stack_level_voxel_size));
    return clipmap_center;
}

#endif
