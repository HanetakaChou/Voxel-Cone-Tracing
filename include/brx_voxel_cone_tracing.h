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

#ifndef _BRX_VOXEL_CONE_TRACING_H_
#define _BRX_VOXEL_CONE_TRACING_H_ 1

#define BRX_VCT_VIEWPORT_DEPTH_DIRECTION_COUNT 3
#define BRX_VCT_VIEWPORT_DEPTH_DIRECTION_AXIS_Z 0 // xyz -> xyz
#define BRX_VCT_VIEWPORT_DEPTH_DIRECTION_AXIS_Y 1 // xyz -> zxy
#define BRX_VCT_VIEWPORT_DEPTH_DIRECTION_AXIS_X 2 // xyz -> yzx

#define BRX_VCT_CLIPMAP_STACK_LEVEL_COUNT 5
#define BRX_VCT_CLIPMAP_MIP_LEVEL_COUNT 0
#define BRX_VCT_CLIPMAP_MAP_SIZE 128
#define BRX_VCT_CLIPMAP_MARGIN 1
// https://github.com/KhronosGroup/glTF-Sample-Models/blob/main/2.0/Sponza/glTF/Sponza.gltf#L8558
// https://github.com/KhronosGroup/glTF-Sample-Assets/blob/main/Models/Sponza/glTF/Sponza.gltf#L8558
#define BRX_VCT_CLIPMAP_FINEST_VOXEL_SIZE (8.0 * 0.00800000037997961)

// TODO: do we really need 16 directions?
#define BRX_VCT_CLIPMAP_ILLUMINATION_TEXTURE_DIRECTION_WIDTH_COUNT 4
#define BRX_VCT_CLIPMAP_ILLUMINATION_TEXTURE_DIRECTION_HEIGHT_COUNT 4

#define BRX_VCT_VOXELIZATION_PIXEL_SAMPLE_COUNT 8

#define BRX_VOXEL_CONE_TRACING_ZERO_THREADGROUP_X 8
#define BRX_VOXEL_CONE_TRACING_ZERO_THREADGROUP_Y 8
#define BRX_VOXEL_CONE_TRACING_ZERO_THREADGROUP_Z 8

#define BRX_VOXEL_CONE_TRACING_CLEAR_THREADGROUP_X 8
#define BRX_VOXEL_CONE_TRACING_CLEAR_THREADGROUP_Y 8
#define BRX_VOXEL_CONE_TRACING_CLEAR_THREADGROUP_Z 8

#define BRX_VOXEL_CONE_TRACING_PACK_THREADGROUP_X 8
#define BRX_VOXEL_CONE_TRACING_PACK_THREADGROUP_Y 8
#define BRX_VOXEL_CONE_TRACING_PACK_THREADGROUP_Z 8

#if (BRX_VCT_CLIPMAP_ILLUMINATION_TEXTURE_DIRECTION_WIDTH_COUNT * BRX_VCT_CLIPMAP_ILLUMINATION_TEXTURE_DIRECTION_HEIGHT_COUNT + BRX_VCT_CLIPMAP_STACK_LEVEL_COUNT) > 32
#error "mask 32 bit"
#endif

#endif
