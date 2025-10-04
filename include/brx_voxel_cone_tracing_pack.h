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

#ifndef _BRX_VOXEL_CONE_TRACING_PACK_H_
#define _BRX_VOXEL_CONE_TRACING_PACK_H_ 1

#include "brx_voxel_cone_tracing.h"

static inline DirectX::XMUINT3 brx_voxel_cone_tracing_pack_dispatch_extent()
{
    return DirectX::XMUINT3(
        static_cast<uint32_t>(BRX_VCT_CLIPMAP_MAP_SIZE) / static_cast<uint32_t>(BRX_VOXEL_CONE_TRACING_PACK_THREADGROUP_X),
        static_cast<uint32_t>(BRX_VCT_CLIPMAP_MAP_SIZE) / static_cast<uint32_t>(BRX_VOXEL_CONE_TRACING_PACK_THREADGROUP_Y),
        static_cast<uint32_t>(BRX_VCT_CLIPMAP_MAP_SIZE) / static_cast<uint32_t>(BRX_VOXEL_CONE_TRACING_PACK_THREADGROUP_Z));
}

#endif
