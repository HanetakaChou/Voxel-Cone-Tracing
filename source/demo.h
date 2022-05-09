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

#ifndef _DEMO_H_
#define _DEMO_H_ 1

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif
#ifndef NOMINMAX
#define NOMINMAX 1
#endif
#include <sdkddkver.h>
#include <windows.h>
#include <dxgi.h>
#include <d3d11_1.h>
#include <vector>
#include "surface.h"

class Demo
{
	// Shadow Map
	ID3D11DepthStencilView *m_shadow_map_dsv;
	ID3D11ShaderResourceView *m_shadow_map_srv;

	//
	ID3D11Buffer *m_shadow_map_constant;
	ID3D11VertexShader *m_shadow_map_vertex_shader;
	ID3D11RasterizerState *m_shadow_map_rasterizer_state;
	ID3D11DepthStencilState *m_shadow_map_depth_stencil_state;

	//
	ID3D11RenderTargetView *m_direct_radiance_rtv;
	ID3D11ShaderResourceView *m_direct_radiance_srv;
	ID3D11RenderTargetView *m_gbuffer_base_color_rtv;
	ID3D11ShaderResourceView *m_gbuffer_base_color_srv;
	ID3D11RenderTargetView *m_gbuffer_roughness_metallic_rtv;
	ID3D11ShaderResourceView *m_gbuffer_roughness_metallic_srv;
	ID3D11RenderTargetView *m_gbuffer_normal_rtv;
	ID3D11ShaderResourceView *m_gbuffer_normal_srv;
	ID3D11DepthStencilView *m_scene_depth_dsv;
	ID3D11ShaderResourceView *m_scene_depth_srv;

	// Forward Shading
	ID3D11Buffer *m_forward_shading_constant;
	ID3D11VertexShader *m_forward_shading_vertex_shader;
	ID3D11PixelShader *m_forward_shading_pixel_shader;
	ID3D11RasterizerState *m_forward_shading_rasterizer_state;
	ID3D11DepthStencilState *m_forward_shading_depth_stencil_state;

	// Clipmap
	ID3D11UnorderedAccessView *m_clipmap_texture_mask_uav;
	ID3D11ShaderResourceView *m_clipmap_texture_mask_srv;
	ID3D11UnorderedAccessView *m_clipmap_texture_illumination_opacity_r16g16_uav;
	ID3D11ShaderResourceView *m_clipmap_texture_illumination_opacity_r16g16_srv;
	ID3D11UnorderedAccessView *m_clipmap_texture_illumination_opacity_b16a16_uav;
	ID3D11ShaderResourceView *m_clipmap_texture_illumination_opacity_b16a16_srv;
	ID3D11UnorderedAccessView *m_clipmap_texture_illumination_opacity_r16g16b16a16_uav;
	ID3D11ShaderResourceView *m_clipmap_texture_illumination_opacity_r16g16b16a16_srv;

	ID3D11ComputeShader *m_zero_clipmap_compute_shader;
	ID3D11ComputeShader *m_clear_clipmap_compute_shader;
	ID3D11ComputeShader *m_pack_clipmap_compute_shader;

	// Voxelization
	ID3D11Buffer *m_voxelization_constant;
	ID3D11DepthStencilView *m_voxelization_depth_dsv;
	ID3D11VertexShader *m_voxelization_vertex_shader;
	ID3D11PixelShader *m_voxelization_pixel_shader;
	ID3D11RasterizerState *m_voxelization_rasterizer_state;
	ID3D11DepthStencilState *m_voxelization_depth_stencil_state;

	//
	ID3D11UnorderedAccessView *m_indirect_radiance_and_ambient_occlusion_uav;
	ID3D11ShaderResourceView *m_indirect_radiance_and_ambient_occlusion_srv;

	// Cone Tracing
	ID3D11Buffer *m_cone_tracing_constant;
	ID3D11ComputeShader *m_cone_tracing_compute_shader;

	// Post Processing
	ID3D11VertexShader *m_post_processing_vertex_shader;
	ID3D11PixelShader *m_post_processing_pixel_shader;
	ID3D11RasterizerState *m_post_processing_rasterizer_state;
	ID3D11DepthStencilState *m_post_processing_depth_stencil_state;

	// Swap Chain
	ID3D11RenderTargetView *m_swap_chain_texture_rtv;

	// Sampler
	ID3D11SamplerState *m_wrap_anisotrop_sampler;
	ID3D11SamplerState *m_clamp_linear_sampler;
	ID3D11SamplerState *m_shadow_comparison_border_linear_sampler;

	// Asset
	std::vector<surface> m_surfaces;

public:
	Demo();
	void Init(ID3D11Device *device, ID3D11DeviceContext *device_context, IDXGISwapChain *swap_chain);
	void Tick(ID3D11DeviceContext *device_context, ID3DUserDefinedAnnotation *user_defined_annotation, IDXGISwapChain *swap_chain);
	void UnInit();
};

#endif