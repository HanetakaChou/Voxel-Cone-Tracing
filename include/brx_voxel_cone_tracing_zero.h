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

#ifndef _BRX_VOXEL_CONE_TRACING_ZERO_H_
#define _BRX_VOXEL_CONE_TRACING_ZERO_H_ 1

#include "brx_voxel_cone_tracing.h"
#include "brx_voxel_cone_tracing_resource.h"

static inline DirectX::XMUINT3 brx_voxel_cone_tracing_zero_dispatch_extent()
{
    DirectX::XMUINT3 const clipmap_mask_texture_extent = brx_voxel_cone_tracing_resource_clipmap_mask_texture_extent();

    DirectX::XMUINT3 const clipmap_illumination_opacity_texture_extent = brx_voxel_cone_tracing_resource_clipmap_illumination_opacity_texture_extent();

    uint32_t const clipmap_maximum_extent_x = std::max(clipmap_mask_texture_extent.x, clipmap_illumination_opacity_texture_extent.x);

    uint32_t const clipmap_maximum_extent_y = std::max(clipmap_mask_texture_extent.y, clipmap_illumination_opacity_texture_extent.y);

    uint32_t const clipmap_maximum_extent_z = std::max(clipmap_mask_texture_extent.z, clipmap_illumination_opacity_texture_extent.z);

    return DirectX::XMUINT3(
        (clipmap_maximum_extent_x + static_cast<uint32_t>(BRX_VOXEL_CONE_TRACING_ZERO_THREADGROUP_X) - 1U) / static_cast<uint32_t>(BRX_VOXEL_CONE_TRACING_ZERO_THREADGROUP_X),
        (clipmap_maximum_extent_y + static_cast<uint32_t>(BRX_VOXEL_CONE_TRACING_ZERO_THREADGROUP_Y) - 1U) / static_cast<uint32_t>(BRX_VOXEL_CONE_TRACING_ZERO_THREADGROUP_Y),
        (clipmap_maximum_extent_z + static_cast<uint32_t>(BRX_VOXEL_CONE_TRACING_ZERO_THREADGROUP_Z) - 1U) / static_cast<uint32_t>(BRX_VOXEL_CONE_TRACING_ZERO_THREADGROUP_Z));
}

#endif
