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
#include <DirectXMath.h>
#include <cstddef>
#include <cstdint>
#include <cassert>
#include <vector>
#include "resolution.h"
#include "camera_controller.h"
#include "demo.h"
#include "surface.h"
#ifndef NDEBUG
#include "../shaders/dxbc/Debug/shadow_map_vertex.inl"
#include "../shaders/dxbc/Debug/forward_shading_vertex.inl"
#include "../shaders/dxbc/Debug/forward_shading_fragment.inl"
#include "../shaders/dxbc/Debug/zero_clipmap_compute.inl"
#include "../shaders/dxbc/Debug/clear_clipmap_compute.inl"
#include "../shaders/dxbc/Debug/pack_clipmap_compute.inl"
#include "../shaders/dxbc/Debug/voxelization_vertex.inl"
#include "../shaders/dxbc/Debug/voxelization_fragment.inl"
#include "../shaders/dxbc/Debug/cone_tracing_compute.inl"
#include "../shaders/dxbc/Debug/post_processing_vertex.inl"
#include "../shaders/dxbc/Debug/post_processing_fragment.inl"
#else
#include "../shaders/dxbc/Release/shadow_map_vertex.inl"
#include "../shaders/dxbc/Release/forward_shading_vertex.inl"
#include "../shaders/dxbc/Release/forward_shading_fragment.inl"
#include "../shaders/dxbc/Release/zero_clipmap_compute.inl"
#include "../shaders/dxbc/Release/clear_clipmap_compute.inl"
#include "../shaders/dxbc/Release/pack_clipmap_compute.inl"
#include "../shaders/dxbc/Release/voxelization_vertex.inl"
#include "../shaders/dxbc/Release/voxelization_fragment.inl"
#include "../shaders/dxbc/Release/cone_tracing_compute.inl"
#include "../shaders/dxbc/Release/post_processing_vertex.inl"
#include "../shaders/dxbc/Release/post_processing_fragment.inl"
#endif
#include "../shaders/shadow_map_resource_binding.hlsli"
#include "../shaders/forward_shading_resource_binding.hlsli"
#include "../shaders/voxelization_resource_binding.hlsli"
#include "../shaders/cone_tracing_resource_binding.hlsli"
#include "../thirdparty/Brioche-Shader-Language/include/brx_reversed_z.h"
#include "../thirdparty/Voxel-Cone-Tracing/include/brx_voxel_cone_tracing.h"
#include "../thirdparty/Voxel-Cone-Tracing/include/brx_voxel_cone_tracing_zero.h"
#include "../thirdparty/Voxel-Cone-Tracing/include/brx_voxel_cone_tracing_clear.h"
#include "../thirdparty/Voxel-Cone-Tracing/include/brx_voxel_cone_tracing_pack.h"
#include "../thirdparty/Voxel-Cone-Tracing/include/brx_voxel_cone_tracing_resource.h"
#include "../thirdparty/Voxel-Cone-Tracing/include/brx_voxel_cone_tracing_voxelization.h"

static DirectX::XMFLOAT3 const g_light_direction(0.0553847104F, 0.830773175F, 0.553848922F);
static float g_light_size = 20.0F;
static uint32_t const g_shadow_map_size = 2048U;
static DirectX::XMFLOAT3 g_light_color(5.0F, 5.0F, 5.0F);

Demo::Demo()
	: m_shadow_map_dsv(NULL),
	  m_shadow_map_srv(NULL),
	  m_shadow_map_constant(NULL),
	  m_shadow_map_vertex_shader(NULL),
	  m_shadow_map_rasterizer_state(NULL),
	  m_shadow_map_depth_stencil_state(NULL),
	  m_direct_radiance_rtv(NULL),
	  m_direct_radiance_srv(NULL),
	  m_gbuffer_base_color_rtv(NULL),
	  m_gbuffer_base_color_srv(NULL),
	  m_gbuffer_roughness_metallic_rtv(NULL),
	  m_gbuffer_roughness_metallic_srv(NULL),
	  m_gbuffer_normal_rtv(NULL),
	  m_gbuffer_normal_srv(NULL),
	  m_scene_depth_dsv(NULL),
	  m_scene_depth_srv(NULL),
	  m_forward_shading_constant(NULL),
	  m_forward_shading_vertex_shader(NULL),
	  m_forward_shading_pixel_shader(NULL),
	  m_forward_shading_rasterizer_state(NULL),
	  m_forward_shading_depth_stencil_state(NULL),
	  m_clipmap_texture_mask_uav(NULL),
	  m_clipmap_texture_mask_srv(NULL),
	  m_clipmap_texture_illumination_opacity_r16g16_uav(NULL),
	  m_clipmap_texture_illumination_opacity_r16g16_srv(NULL),
	  m_clipmap_texture_illumination_opacity_b16a16_uav(NULL),
	  m_clipmap_texture_illumination_opacity_b16a16_srv(NULL),
	  m_clipmap_texture_illumination_opacity_r16g16b16a16_uav(NULL),
	  m_clipmap_texture_illumination_opacity_r16g16b16a16_srv(NULL),
	  m_zero_clipmap_compute_shader(NULL),
	  m_clear_clipmap_compute_shader(NULL),
	  m_pack_clipmap_compute_shader(NULL),
	  m_voxelization_constant(NULL),
	  m_voxelization_depth_dsv(NULL),
	  m_voxelization_vertex_shader(NULL),
	  m_voxelization_pixel_shader(NULL),
	  m_voxelization_rasterizer_state(NULL),
	  m_voxelization_depth_stencil_state(NULL),
	  m_indirect_radiance_and_ambient_occlusion_uav(NULL),
	  m_indirect_radiance_and_ambient_occlusion_srv(NULL),
	  m_cone_tracing_constant(NULL),
	  m_cone_tracing_compute_shader(NULL),
	  m_post_processing_vertex_shader(NULL),
	  m_post_processing_pixel_shader(NULL),
	  m_post_processing_rasterizer_state(NULL),
	  m_post_processing_depth_stencil_state(NULL),
	  m_swap_chain_texture_rtv(NULL),
	  m_wrap_anisotrop_sampler(NULL),
	  m_clamp_linear_sampler(NULL),
	  m_shadow_comparison_border_linear_sampler(NULL)
{
}

void Demo::Init(ID3D11Device *device, ID3D11DeviceContext *device_context, IDXGISwapChain *swap_chain)
{
	{
		D3D11_TEXTURE2D_DESC texture2d_desc;
		texture2d_desc.Width = static_cast<uint32_t>(g_shadow_map_size);
		texture2d_desc.Height = static_cast<uint32_t>(g_shadow_map_size);
		texture2d_desc.MipLevels = 1U;
		texture2d_desc.ArraySize = 1U;
		texture2d_desc.Format = DXGI_FORMAT_R32_TYPELESS;
		texture2d_desc.SampleDesc.Count = 1U;
		texture2d_desc.SampleDesc.Quality = 0U;
		texture2d_desc.Usage = D3D11_USAGE_DEFAULT;
		texture2d_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
		texture2d_desc.CPUAccessFlags = 0U;
		texture2d_desc.MiscFlags = 0U;

		ID3D11Texture2D *shadow_map_texture = NULL;

		HRESULT res_device_create_texture2d = device->CreateTexture2D(&texture2d_desc, NULL, &shadow_map_texture);
		assert(SUCCEEDED(res_device_create_texture2d));

		D3D11_DEPTH_STENCIL_VIEW_DESC depth_stencil_view_desc;
		depth_stencil_view_desc.Format = DXGI_FORMAT_D32_FLOAT;
		depth_stencil_view_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		depth_stencil_view_desc.Flags = 0U;
		depth_stencil_view_desc.Texture2D.MipSlice = 0U;

		assert(NULL == this->m_shadow_map_dsv);

		HRESULT res_device_create_depth_stencil_view = device->CreateDepthStencilView(shadow_map_texture, &depth_stencil_view_desc, &this->m_shadow_map_dsv);
		assert(SUCCEEDED(res_device_create_depth_stencil_view));

		D3D11_SHADER_RESOURCE_VIEW_DESC shader_resource_view_desc;
		shader_resource_view_desc.Format = DXGI_FORMAT_R32_FLOAT;
		shader_resource_view_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		shader_resource_view_desc.Texture2D.MostDetailedMip = 0U;
		shader_resource_view_desc.Texture2D.MipLevels = 1U;

		assert(NULL == this->m_shadow_map_srv);

		HRESULT res_device_create_shader_resource_view = device->CreateShaderResourceView(shadow_map_texture, &shader_resource_view_desc, &this->m_shadow_map_srv);
		assert(SUCCEEDED(res_device_create_shader_resource_view));

		shadow_map_texture->Release();
	}

	{
		D3D11_BUFFER_DESC buffer_desc;
		buffer_desc.ByteWidth = sizeof(shadow_map_constant_buffer_binding);
		buffer_desc.Usage = D3D11_USAGE_DEFAULT;
		buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		buffer_desc.CPUAccessFlags = 0U;
		buffer_desc.MiscFlags = 0U;
		buffer_desc.StructureByteStride = 0U;

		assert(NULL == this->m_shadow_map_constant);

		HRESULT res_device_create_buffer = device->CreateBuffer(&buffer_desc, NULL, &this->m_shadow_map_constant);
		assert(SUCCEEDED(res_device_create_buffer));
	}

	{
		assert(NULL == this->m_shadow_map_vertex_shader);

		HRESULT res_device_create_vertex_shader = device->CreateVertexShader(code_shader_shadow_map_vertex, sizeof(code_shader_shadow_map_vertex), NULL, &this->m_shadow_map_vertex_shader);
		assert(SUCCEEDED(res_device_create_vertex_shader));
	}

	{
		assert(NULL == this->m_shadow_map_rasterizer_state);

		D3D11_RASTERIZER_DESC rasterizer_desc;
		rasterizer_desc.FillMode = D3D11_FILL_SOLID;
		rasterizer_desc.CullMode = D3D11_CULL_NONE;
		rasterizer_desc.FrontCounterClockwise = TRUE;
		rasterizer_desc.DepthBias = 16;
		rasterizer_desc.DepthBiasClamp = 0.0F;
		rasterizer_desc.SlopeScaledDepthBias = 4.0F;
		rasterizer_desc.DepthClipEnable = TRUE;
		rasterizer_desc.ScissorEnable = FALSE;
		rasterizer_desc.MultisampleEnable = FALSE;
		rasterizer_desc.AntialiasedLineEnable = FALSE;
		HRESULT res_device_create_rasterizer_state = device->CreateRasterizerState(&rasterizer_desc, &this->m_shadow_map_rasterizer_state);
		assert(SUCCEEDED(res_device_create_rasterizer_state));
	}

	{
		D3D11_DEPTH_STENCIL_DESC depth_stencil_desc;
		depth_stencil_desc.DepthEnable = TRUE;
		depth_stencil_desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		depth_stencil_desc.DepthFunc = D3D11_COMPARISON_LESS;
		depth_stencil_desc.StencilEnable = FALSE;
		depth_stencil_desc.StencilReadMask = 0U;
		depth_stencil_desc.StencilWriteMask = 0U;
		depth_stencil_desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
		depth_stencil_desc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
		depth_stencil_desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
		depth_stencil_desc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
		depth_stencil_desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
		depth_stencil_desc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
		depth_stencil_desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
		depth_stencil_desc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;

		assert(NULL == this->m_shadow_map_depth_stencil_state);

		HRESULT res_device_create_depth_stencil_state = device->CreateDepthStencilState(&depth_stencil_desc, &this->m_shadow_map_depth_stencil_state);
		assert(SUCCEEDED(res_device_create_depth_stencil_state));
	}

	{
		D3D11_TEXTURE2D_DESC texture2d_desc;
		texture2d_desc.Width = static_cast<uint32_t>(g_resolution_width);
		texture2d_desc.Height = static_cast<uint32_t>(g_resolution_height);
		texture2d_desc.MipLevels = 1U;
		texture2d_desc.ArraySize = 1U;
		texture2d_desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		texture2d_desc.SampleDesc.Count = 1U;
		texture2d_desc.SampleDesc.Quality = 0U;
		texture2d_desc.Usage = D3D11_USAGE_DEFAULT;
		texture2d_desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
		texture2d_desc.CPUAccessFlags = 0U;
		texture2d_desc.MiscFlags = 0U;

		ID3D11Texture2D *direct_radiance_texture = NULL;

		HRESULT res_device_create_texture2d = device->CreateTexture2D(&texture2d_desc, NULL, &direct_radiance_texture);
		assert(SUCCEEDED(res_device_create_texture2d));

		D3D11_RENDER_TARGET_VIEW_DESC render_target_view_desc;
		render_target_view_desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		render_target_view_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		render_target_view_desc.Texture2D.MipSlice = 0U;

		assert(NULL == this->m_direct_radiance_rtv);

		HRESULT res_device_create_render_target_view = device->CreateRenderTargetView(direct_radiance_texture, &render_target_view_desc, &this->m_direct_radiance_rtv);
		assert(SUCCEEDED(res_device_create_render_target_view));

		D3D11_SHADER_RESOURCE_VIEW_DESC shader_resource_view_desc;
		shader_resource_view_desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		shader_resource_view_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		shader_resource_view_desc.Texture2D.MostDetailedMip = 0U;
		shader_resource_view_desc.Texture2D.MipLevels = 1U;

		assert(NULL == this->m_direct_radiance_srv);

		HRESULT res_device_create_shader_resource_view = device->CreateShaderResourceView(direct_radiance_texture, &shader_resource_view_desc, &this->m_direct_radiance_srv);
		assert(SUCCEEDED(res_device_create_shader_resource_view));

		direct_radiance_texture->Release();
	}

	{
		D3D11_TEXTURE2D_DESC texture2d_desc;
		texture2d_desc.Width = static_cast<uint32_t>(g_resolution_width);
		texture2d_desc.Height = static_cast<uint32_t>(g_resolution_height);
		texture2d_desc.MipLevels = 1U;
		texture2d_desc.ArraySize = 1U;
		texture2d_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		texture2d_desc.SampleDesc.Count = 1U;
		texture2d_desc.SampleDesc.Quality = 0U;
		texture2d_desc.Usage = D3D11_USAGE_DEFAULT;
		texture2d_desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
		texture2d_desc.CPUAccessFlags = 0U;
		texture2d_desc.MiscFlags = 0U;

		ID3D11Texture2D *gbuffer_base_color_texture = NULL;

		HRESULT res_device_create_texture2d = device->CreateTexture2D(&texture2d_desc, NULL, &gbuffer_base_color_texture);
		assert(SUCCEEDED(res_device_create_texture2d));

		D3D11_RENDER_TARGET_VIEW_DESC render_target_view_desc;
		render_target_view_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		render_target_view_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		render_target_view_desc.Texture2D.MipSlice = 0U;

		assert(NULL == this->m_gbuffer_base_color_rtv);

		HRESULT res_device_create_render_target_view = device->CreateRenderTargetView(gbuffer_base_color_texture, &render_target_view_desc, &this->m_gbuffer_base_color_rtv);
		assert(SUCCEEDED(res_device_create_render_target_view));

		D3D11_SHADER_RESOURCE_VIEW_DESC shader_resource_view_desc;
		shader_resource_view_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		shader_resource_view_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		shader_resource_view_desc.Texture2D.MostDetailedMip = 0U;
		shader_resource_view_desc.Texture2D.MipLevels = 1U;

		assert(NULL == this->m_gbuffer_base_color_srv);

		HRESULT res_device_create_shader_resource_view = device->CreateShaderResourceView(gbuffer_base_color_texture, &shader_resource_view_desc, &this->m_gbuffer_base_color_srv);
		assert(SUCCEEDED(res_device_create_shader_resource_view));

		gbuffer_base_color_texture->Release();
	}

	{
		D3D11_TEXTURE2D_DESC texture2d_desc;
		texture2d_desc.Width = static_cast<uint32_t>(g_resolution_width);
		texture2d_desc.Height = static_cast<uint32_t>(g_resolution_height);
		texture2d_desc.MipLevels = 1U;
		texture2d_desc.ArraySize = 1U;
		texture2d_desc.Format = DXGI_FORMAT_R8G8_UNORM;
		texture2d_desc.SampleDesc.Count = 1U;
		texture2d_desc.SampleDesc.Quality = 0U;
		texture2d_desc.Usage = D3D11_USAGE_DEFAULT;
		texture2d_desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
		texture2d_desc.CPUAccessFlags = 0U;
		texture2d_desc.MiscFlags = 0U;

		ID3D11Texture2D *gbuffer_roughness_metallic_texture = NULL;

		HRESULT res_device_create_texture2d = device->CreateTexture2D(&texture2d_desc, NULL, &gbuffer_roughness_metallic_texture);
		assert(SUCCEEDED(res_device_create_texture2d));

		D3D11_RENDER_TARGET_VIEW_DESC render_target_view_desc;
		render_target_view_desc.Format = DXGI_FORMAT_R8G8_UNORM;
		render_target_view_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		render_target_view_desc.Texture2D.MipSlice = 0U;

		assert(NULL == this->m_gbuffer_roughness_metallic_rtv);

		HRESULT res_device_create_render_target_view = device->CreateRenderTargetView(gbuffer_roughness_metallic_texture, &render_target_view_desc, &this->m_gbuffer_roughness_metallic_rtv);
		assert(SUCCEEDED(res_device_create_render_target_view));

		D3D11_SHADER_RESOURCE_VIEW_DESC shader_resource_view_desc;
		shader_resource_view_desc.Format = DXGI_FORMAT_R8G8_UNORM;
		shader_resource_view_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		shader_resource_view_desc.Texture2D.MostDetailedMip = 0U;
		shader_resource_view_desc.Texture2D.MipLevels = 1U;

		assert(NULL == this->m_gbuffer_roughness_metallic_srv);

		HRESULT res_device_create_shader_resource_view = device->CreateShaderResourceView(gbuffer_roughness_metallic_texture, &shader_resource_view_desc, &this->m_gbuffer_roughness_metallic_srv);
		assert(SUCCEEDED(res_device_create_shader_resource_view));

		gbuffer_roughness_metallic_texture->Release();
	}

	{
		D3D11_TEXTURE2D_DESC texture2d_desc;
		texture2d_desc.Width = static_cast<uint32_t>(g_resolution_width);
		texture2d_desc.Height = static_cast<uint32_t>(g_resolution_height);
		texture2d_desc.MipLevels = 1U;
		texture2d_desc.ArraySize = 1U;
		texture2d_desc.Format = DXGI_FORMAT_R16G16_SNORM;
		texture2d_desc.SampleDesc.Count = 1U;
		texture2d_desc.SampleDesc.Quality = 0U;
		texture2d_desc.Usage = D3D11_USAGE_DEFAULT;
		texture2d_desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
		texture2d_desc.CPUAccessFlags = 0U;
		texture2d_desc.MiscFlags = 0U;

		ID3D11Texture2D *gbuffer_normal_texture = NULL;

		HRESULT res_device_create_texture2d = device->CreateTexture2D(&texture2d_desc, NULL, &gbuffer_normal_texture);
		assert(SUCCEEDED(res_device_create_texture2d));

		D3D11_RENDER_TARGET_VIEW_DESC render_target_view_desc;
		render_target_view_desc.Format = DXGI_FORMAT_R16G16_SNORM;
		render_target_view_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		render_target_view_desc.Texture2D.MipSlice = 0U;

		assert(NULL == this->m_gbuffer_normal_rtv);

		HRESULT res_device_create_render_target_view = device->CreateRenderTargetView(gbuffer_normal_texture, &render_target_view_desc, &this->m_gbuffer_normal_rtv);
		assert(SUCCEEDED(res_device_create_render_target_view));

		D3D11_SHADER_RESOURCE_VIEW_DESC shader_resource_view_desc;
		shader_resource_view_desc.Format = DXGI_FORMAT_R16G16_SNORM;
		shader_resource_view_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		shader_resource_view_desc.Texture2D.MostDetailedMip = 0U;
		shader_resource_view_desc.Texture2D.MipLevels = 1U;

		assert(NULL == this->m_gbuffer_normal_srv);

		HRESULT res_device_create_shader_resource_view = device->CreateShaderResourceView(gbuffer_normal_texture, &shader_resource_view_desc, &this->m_gbuffer_normal_srv);
		assert(SUCCEEDED(res_device_create_shader_resource_view));

		gbuffer_normal_texture->Release();
	}

	{
		D3D11_TEXTURE2D_DESC texture2d_desc;
		texture2d_desc.Width = static_cast<uint32_t>(g_resolution_width);
		texture2d_desc.Height = static_cast<uint32_t>(g_resolution_height);
		texture2d_desc.MipLevels = 1U;
		texture2d_desc.ArraySize = 1U;
		texture2d_desc.Format = DXGI_FORMAT_R32_TYPELESS;
		texture2d_desc.SampleDesc.Count = 1U;
		texture2d_desc.SampleDesc.Quality = 0U;
		texture2d_desc.Usage = D3D11_USAGE_DEFAULT;
		texture2d_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
		texture2d_desc.CPUAccessFlags = 0U;
		texture2d_desc.MiscFlags = 0U;

		ID3D11Texture2D *scene_depth_texture = NULL;

		HRESULT res_device_create_texture2d = device->CreateTexture2D(&texture2d_desc, NULL, &scene_depth_texture);
		assert(SUCCEEDED(res_device_create_texture2d));

		D3D11_DEPTH_STENCIL_VIEW_DESC depth_stencil_view_desc;
		depth_stencil_view_desc.Format = DXGI_FORMAT_D32_FLOAT;
		depth_stencil_view_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		depth_stencil_view_desc.Flags = 0U;
		depth_stencil_view_desc.Texture2D.MipSlice = 0U;

		assert(NULL == this->m_scene_depth_dsv);

		HRESULT res_device_create_depth_stencil_view = device->CreateDepthStencilView(scene_depth_texture, &depth_stencil_view_desc, &this->m_scene_depth_dsv);
		assert(SUCCEEDED(res_device_create_depth_stencil_view));

		D3D11_SHADER_RESOURCE_VIEW_DESC shader_resource_view_desc;
		shader_resource_view_desc.Format = DXGI_FORMAT_R32_FLOAT;
		shader_resource_view_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		shader_resource_view_desc.Texture2D.MostDetailedMip = 0U;
		shader_resource_view_desc.Texture2D.MipLevels = 1U;

		assert(NULL == this->m_scene_depth_srv);

		HRESULT res_device_create_shader_resource_view = device->CreateShaderResourceView(scene_depth_texture, &shader_resource_view_desc, &this->m_scene_depth_srv);
		assert(SUCCEEDED(res_device_create_shader_resource_view));

		scene_depth_texture->Release();
	}

	{
		D3D11_BUFFER_DESC buffer_desc;
		buffer_desc.ByteWidth = sizeof(forward_shading_constant_buffer_binding);
		buffer_desc.Usage = D3D11_USAGE_DEFAULT;
		buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		buffer_desc.CPUAccessFlags = 0U;
		buffer_desc.MiscFlags = 0U;
		buffer_desc.StructureByteStride = 0U;

		assert(NULL == this->m_forward_shading_constant);

		HRESULT res_device_create_buffer = device->CreateBuffer(&buffer_desc, NULL, &this->m_forward_shading_constant);
		assert(SUCCEEDED(res_device_create_buffer));
	}

	{
		assert(NULL == this->m_forward_shading_vertex_shader);

		HRESULT res_device_create_vertex_shader = device->CreateVertexShader(code_shader_forward_shading_vertex, sizeof(code_shader_forward_shading_vertex), NULL, &this->m_forward_shading_vertex_shader);
		assert(SUCCEEDED(res_device_create_vertex_shader));
	}

	{
		assert(NULL == this->m_forward_shading_pixel_shader);

		HRESULT res_device_create_pixel_shader = device->CreatePixelShader(code_shader_forward_shading_fragment, sizeof(code_shader_forward_shading_fragment), NULL, &this->m_forward_shading_pixel_shader);
		assert(SUCCEEDED(res_device_create_pixel_shader));
	}

	{
		assert(NULL == this->m_forward_shading_rasterizer_state);

		D3D11_RASTERIZER_DESC rasterizer_desc;
		rasterizer_desc.FillMode = D3D11_FILL_SOLID;
		rasterizer_desc.CullMode = D3D11_CULL_BACK;
		rasterizer_desc.FrontCounterClockwise = TRUE;
		rasterizer_desc.DepthBias = 0;
		rasterizer_desc.DepthBiasClamp = 0.0F;
		rasterizer_desc.SlopeScaledDepthBias = 0.0F;
		rasterizer_desc.DepthClipEnable = TRUE;
		rasterizer_desc.ScissorEnable = FALSE;
		rasterizer_desc.MultisampleEnable = FALSE;
		rasterizer_desc.AntialiasedLineEnable = FALSE;
		HRESULT res_device_create_rasterizer_state = device->CreateRasterizerState(&rasterizer_desc, &this->m_forward_shading_rasterizer_state);
		assert(SUCCEEDED(res_device_create_rasterizer_state));
	}

	{
		D3D11_DEPTH_STENCIL_DESC depth_stencil_desc;
		depth_stencil_desc.DepthEnable = TRUE;
		depth_stencil_desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		depth_stencil_desc.DepthFunc = D3D11_COMPARISON_GREATER;
		depth_stencil_desc.StencilEnable = FALSE;
		depth_stencil_desc.StencilReadMask = 0U;
		depth_stencil_desc.StencilWriteMask = 0U;
		depth_stencil_desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
		depth_stencil_desc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
		depth_stencil_desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
		depth_stencil_desc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
		depth_stencil_desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
		depth_stencil_desc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
		depth_stencil_desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
		depth_stencil_desc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;

		assert(NULL == this->m_forward_shading_depth_stencil_state);

		HRESULT res_device_create_depth_stencil_state = device->CreateDepthStencilState(&depth_stencil_desc, &this->m_forward_shading_depth_stencil_state);
		assert(SUCCEEDED(res_device_create_depth_stencil_state));
	}

	{
		DirectX::XMUINT3 clipmap_mask_texture_extent = brx_voxel_cone_tracing_resource_clipmap_mask_texture_extent();

		D3D11_TEXTURE3D_DESC texture3d_desc;
		texture3d_desc.Width = clipmap_mask_texture_extent.x;
		texture3d_desc.Height = clipmap_mask_texture_extent.y;
		texture3d_desc.Depth = clipmap_mask_texture_extent.z;
		texture3d_desc.MipLevels = 1U;
		texture3d_desc.Format = DXGI_FORMAT_R32_TYPELESS;
		texture3d_desc.Usage = D3D11_USAGE_DEFAULT;
		texture3d_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
		texture3d_desc.CPUAccessFlags = 0U;
		texture3d_desc.MiscFlags = 0U;

		ID3D11Texture3D *clipmap_texture_mask = NULL;

		HRESULT res_device_create_buffer = device->CreateTexture3D(&texture3d_desc, NULL, &clipmap_texture_mask);
		assert(SUCCEEDED(res_device_create_buffer));

		D3D11_UNORDERED_ACCESS_VIEW_DESC unordered_access_view_desc;
		unordered_access_view_desc.Format = DXGI_FORMAT_R32_UINT;
		unordered_access_view_desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE3D;
		unordered_access_view_desc.Texture3D.MipSlice = 0U;
		unordered_access_view_desc.Texture3D.FirstWSlice = 0U;
		unordered_access_view_desc.Texture3D.WSize = clipmap_mask_texture_extent.z;

		assert(NULL == this->m_clipmap_texture_mask_uav);

		HRESULT res_device_create_unordered_access_view = device->CreateUnorderedAccessView(clipmap_texture_mask, &unordered_access_view_desc, &this->m_clipmap_texture_mask_uav);
		assert(SUCCEEDED(res_device_create_unordered_access_view));

		D3D11_SHADER_RESOURCE_VIEW_DESC shader_resource_view_desc;
		shader_resource_view_desc.Format = DXGI_FORMAT_R32_UINT;
		shader_resource_view_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
		shader_resource_view_desc.Texture3D.MostDetailedMip = 0U;
		shader_resource_view_desc.Texture3D.MipLevels = 1U;

		assert(NULL == this->m_clipmap_texture_mask_srv);

		HRESULT res_device_create_shader_resource_view = device->CreateShaderResourceView(clipmap_texture_mask, &shader_resource_view_desc, &this->m_clipmap_texture_mask_srv);
		assert(SUCCEEDED(res_device_create_shader_resource_view));

		clipmap_texture_mask->Release();
	}

	{
		DirectX::XMUINT3 clipmap_illumination_opacity_texture_extent = brx_voxel_cone_tracing_resource_clipmap_illumination_opacity_texture_extent();

		D3D11_TEXTURE3D_DESC texture3d_desc;
		texture3d_desc.Width = clipmap_illumination_opacity_texture_extent.x;
		texture3d_desc.Height = clipmap_illumination_opacity_texture_extent.y;
		texture3d_desc.Depth = clipmap_illumination_opacity_texture_extent.z;
		texture3d_desc.MipLevels = 1U;
		texture3d_desc.Format = DXGI_FORMAT_R32_UINT;
		texture3d_desc.Usage = D3D11_USAGE_DEFAULT;
		texture3d_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
		texture3d_desc.CPUAccessFlags = 0U;
		texture3d_desc.MiscFlags = 0U;

		ID3D11Texture3D *clipmap_texture_illumination_opacity = NULL;

		HRESULT res_device_create_texture = device->CreateTexture3D(&texture3d_desc, NULL, &clipmap_texture_illumination_opacity);
		assert(SUCCEEDED(res_device_create_texture));

		D3D11_UNORDERED_ACCESS_VIEW_DESC unordered_access_view_desc;
		unordered_access_view_desc.Format = DXGI_FORMAT_R32_UINT;
		unordered_access_view_desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE3D;
		unordered_access_view_desc.Texture3D.MipSlice = 0U;
		unordered_access_view_desc.Texture3D.FirstWSlice = 0U;
		unordered_access_view_desc.Texture3D.WSize = clipmap_illumination_opacity_texture_extent.z;

		assert(NULL == this->m_clipmap_texture_illumination_opacity_r16g16_uav);

		HRESULT res_device_create_unordered_access_view = device->CreateUnorderedAccessView(clipmap_texture_illumination_opacity, &unordered_access_view_desc, &this->m_clipmap_texture_illumination_opacity_r16g16_uav);
		assert(SUCCEEDED(res_device_create_unordered_access_view));

		D3D11_SHADER_RESOURCE_VIEW_DESC shader_resource_view_desc;
		shader_resource_view_desc.Format = DXGI_FORMAT_R32_UINT;
		shader_resource_view_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
		shader_resource_view_desc.Texture3D.MostDetailedMip = 0U;
		shader_resource_view_desc.Texture3D.MipLevels = 1U;

		assert(NULL == this->m_clipmap_texture_illumination_opacity_r16g16_srv);

		HRESULT res_device_create_shader_resource_view = device->CreateShaderResourceView(clipmap_texture_illumination_opacity, &shader_resource_view_desc, &this->m_clipmap_texture_illumination_opacity_r16g16_srv);
		assert(SUCCEEDED(res_device_create_shader_resource_view));

		clipmap_texture_illumination_opacity->Release();
	}

	{
		DirectX::XMUINT3 clipmap_illumination_opacity_texture_extent = brx_voxel_cone_tracing_resource_clipmap_illumination_opacity_texture_extent();

		D3D11_TEXTURE3D_DESC texture3d_desc;
		texture3d_desc.Width = clipmap_illumination_opacity_texture_extent.x;
		texture3d_desc.Height = clipmap_illumination_opacity_texture_extent.y;
		texture3d_desc.Depth = clipmap_illumination_opacity_texture_extent.z;
		texture3d_desc.MipLevels = 1U;
		texture3d_desc.Format = DXGI_FORMAT_R32_UINT;
		texture3d_desc.Usage = D3D11_USAGE_DEFAULT;
		texture3d_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
		texture3d_desc.CPUAccessFlags = 0U;
		texture3d_desc.MiscFlags = 0U;

		ID3D11Texture3D *clipmap_texture_illumination_opacity = NULL;

		HRESULT res_device_create_texture = device->CreateTexture3D(&texture3d_desc, NULL, &clipmap_texture_illumination_opacity);
		assert(SUCCEEDED(res_device_create_texture));

		D3D11_UNORDERED_ACCESS_VIEW_DESC unordered_access_view_desc;
		unordered_access_view_desc.Format = DXGI_FORMAT_R32_UINT;
		unordered_access_view_desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE3D;
		unordered_access_view_desc.Texture3D.MipSlice = 0U;
		unordered_access_view_desc.Texture3D.FirstWSlice = 0U;
		unordered_access_view_desc.Texture3D.WSize = clipmap_illumination_opacity_texture_extent.z;

		assert(NULL == this->m_clipmap_texture_illumination_opacity_b16a16_uav);

		HRESULT res_device_create_unordered_access_view = device->CreateUnorderedAccessView(clipmap_texture_illumination_opacity, &unordered_access_view_desc, &this->m_clipmap_texture_illumination_opacity_b16a16_uav);
		assert(SUCCEEDED(res_device_create_unordered_access_view));

		D3D11_SHADER_RESOURCE_VIEW_DESC shader_resource_view_desc;
		shader_resource_view_desc.Format = DXGI_FORMAT_R32_UINT;
		shader_resource_view_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
		shader_resource_view_desc.Texture3D.MostDetailedMip = 0U;
		shader_resource_view_desc.Texture3D.MipLevels = 1U;

		assert(NULL == this->m_clipmap_texture_illumination_opacity_b16a16_srv);

		HRESULT res_device_create_shader_resource_view = device->CreateShaderResourceView(clipmap_texture_illumination_opacity, &shader_resource_view_desc, &this->m_clipmap_texture_illumination_opacity_b16a16_srv);
		assert(SUCCEEDED(res_device_create_shader_resource_view));

		clipmap_texture_illumination_opacity->Release();
	}

	{
		DirectX::XMUINT3 clipmap_illumination_opacity_texture_extent = brx_voxel_cone_tracing_resource_clipmap_illumination_opacity_texture_extent();

		D3D11_TEXTURE3D_DESC texture3d_desc;
		texture3d_desc.Width = clipmap_illumination_opacity_texture_extent.x;
		texture3d_desc.Height = clipmap_illumination_opacity_texture_extent.y;
		texture3d_desc.Depth = clipmap_illumination_opacity_texture_extent.z;
		texture3d_desc.MipLevels = 1U;
		texture3d_desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		texture3d_desc.Usage = D3D11_USAGE_DEFAULT;
		texture3d_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
		texture3d_desc.CPUAccessFlags = 0U;
		texture3d_desc.MiscFlags = 0U;

		ID3D11Texture3D *clipmap_texture_illumination_opacity = NULL;

		HRESULT res_device_create_texture = device->CreateTexture3D(&texture3d_desc, NULL, &clipmap_texture_illumination_opacity);
		assert(SUCCEEDED(res_device_create_texture));

		D3D11_UNORDERED_ACCESS_VIEW_DESC unordered_access_view_desc;
		unordered_access_view_desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		unordered_access_view_desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE3D;
		unordered_access_view_desc.Texture3D.MipSlice = 0U;
		unordered_access_view_desc.Texture3D.FirstWSlice = 0U;
		unordered_access_view_desc.Texture3D.WSize = clipmap_illumination_opacity_texture_extent.z;

		assert(NULL == this->m_clipmap_texture_illumination_opacity_r16g16b16a16_uav);

		HRESULT res_device_create_unordered_access_view = device->CreateUnorderedAccessView(clipmap_texture_illumination_opacity, &unordered_access_view_desc, &this->m_clipmap_texture_illumination_opacity_r16g16b16a16_uav);
		assert(SUCCEEDED(res_device_create_unordered_access_view));

		D3D11_SHADER_RESOURCE_VIEW_DESC shader_resource_view_desc;
		shader_resource_view_desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		shader_resource_view_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
		shader_resource_view_desc.Texture3D.MostDetailedMip = 0U;
		shader_resource_view_desc.Texture3D.MipLevels = 1U;

		assert(NULL == this->m_clipmap_texture_illumination_opacity_r16g16b16a16_srv);

		HRESULT res_device_create_shader_resource_view = device->CreateShaderResourceView(clipmap_texture_illumination_opacity, &shader_resource_view_desc, &this->m_clipmap_texture_illumination_opacity_r16g16b16a16_srv);
		assert(SUCCEEDED(res_device_create_shader_resource_view));

		clipmap_texture_illumination_opacity->Release();
	}

	{
		assert(NULL == this->m_zero_clipmap_compute_shader);

		HRESULT res_device_create_compute_shader = device->CreateComputeShader(code_shader_zero_clipmap_compute, sizeof(code_shader_zero_clipmap_compute), NULL, &this->m_zero_clipmap_compute_shader);
		assert(SUCCEEDED(res_device_create_compute_shader));
	}

	{
		assert(NULL == this->m_clear_clipmap_compute_shader);

		HRESULT res_device_create_compute_shader = device->CreateComputeShader(code_shader_clear_clipmap_compute, sizeof(code_shader_clear_clipmap_compute), NULL, &this->m_clear_clipmap_compute_shader);
		assert(SUCCEEDED(res_device_create_compute_shader));
	}

	{
		assert(NULL == this->m_pack_clipmap_compute_shader);

		HRESULT res_device_create_compute_shader = device->CreateComputeShader(code_shader_pack_clipmap_compute, sizeof(code_shader_pack_clipmap_compute), NULL, &this->m_pack_clipmap_compute_shader);
		assert(SUCCEEDED(res_device_create_compute_shader));
	}

	{
		D3D11_BUFFER_DESC buffer_desc;
		buffer_desc.ByteWidth = sizeof(voxelization_constant_buffer_binding);
		buffer_desc.Usage = D3D11_USAGE_DEFAULT;
		buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		buffer_desc.CPUAccessFlags = 0U;
		buffer_desc.MiscFlags = 0U;
		buffer_desc.StructureByteStride = 0U;

		assert(NULL == this->m_voxelization_constant);

		HRESULT res_device_create_buffer = device->CreateBuffer(&buffer_desc, NULL, &this->m_voxelization_constant);
		assert(SUCCEEDED(res_device_create_buffer));
	}

	{
		D3D11_TEXTURE2D_DESC texture2d_desc;
		texture2d_desc.Width = static_cast<uint32_t>(BRX_VCT_CLIPMAP_MAP_SIZE);
		texture2d_desc.Height = static_cast<uint32_t>(BRX_VCT_CLIPMAP_MAP_SIZE);
		texture2d_desc.MipLevels = 1U;
		texture2d_desc.ArraySize = 1U;
		texture2d_desc.Format = DXGI_FORMAT_D32_FLOAT;
		texture2d_desc.SampleDesc.Count = static_cast<uint32_t>(BRX_VCT_VOXELIZATION_PIXEL_SAMPLE_COUNT);
		texture2d_desc.SampleDesc.Quality = D3D11_STANDARD_MULTISAMPLE_PATTERN;
		texture2d_desc.Usage = D3D11_USAGE_DEFAULT;
		texture2d_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		texture2d_desc.CPUAccessFlags = 0U;
		texture2d_desc.MiscFlags = 0U;

		ID3D11Texture2D *voxelization_depth_texture = NULL;

		HRESULT res_device_create_texture2d = device->CreateTexture2D(&texture2d_desc, NULL, &voxelization_depth_texture);
		assert(SUCCEEDED(res_device_create_texture2d));

		D3D11_DEPTH_STENCIL_VIEW_DESC depth_stencil_view_desc;
		depth_stencil_view_desc.Flags = 0U;
		depth_stencil_view_desc.Format = DXGI_FORMAT_D32_FLOAT;
		depth_stencil_view_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
		// depth_stencil_view_desc.Texture2DMS.UnusedField_NothingToDefine = 0U;

		assert(NULL == this->m_voxelization_depth_dsv);

		HRESULT res_device_create_depth_stencil_view = device->CreateDepthStencilView(voxelization_depth_texture, &depth_stencil_view_desc, &this->m_voxelization_depth_dsv);
		assert(SUCCEEDED(res_device_create_depth_stencil_view));

		voxelization_depth_texture->Release();
	}

	{
		assert(NULL == this->m_voxelization_vertex_shader);

		HRESULT res_device_create_vertex_shader = device->CreateVertexShader(code_shader_voxelization_vertex, sizeof(code_shader_voxelization_vertex), NULL, &this->m_voxelization_vertex_shader);
		assert(SUCCEEDED(res_device_create_vertex_shader));
	}

	{
		assert(NULL == this->m_voxelization_pixel_shader);

		HRESULT res_device_create_pixel_shader = device->CreatePixelShader(code_shader_voxelization_fragment, sizeof(code_shader_voxelization_fragment), NULL, &this->m_voxelization_pixel_shader);
		assert(SUCCEEDED(res_device_create_pixel_shader));
	}

	{
		assert(NULL == this->m_voxelization_rasterizer_state);

		D3D11_RASTERIZER_DESC rasterizer_desc;
		rasterizer_desc.FillMode = D3D11_FILL_SOLID;
		rasterizer_desc.CullMode = D3D11_CULL_NONE;
		rasterizer_desc.FrontCounterClockwise = TRUE;
		rasterizer_desc.DepthBias = 0;
		rasterizer_desc.DepthBiasClamp = 0.0F;
		rasterizer_desc.SlopeScaledDepthBias = 0.0F;
		rasterizer_desc.DepthClipEnable = TRUE; 
		rasterizer_desc.ScissorEnable = FALSE;
		rasterizer_desc.MultisampleEnable = TRUE;
		rasterizer_desc.AntialiasedLineEnable = FALSE;
		HRESULT res_device_create_rasterizer_state = device->CreateRasterizerState(&rasterizer_desc, &this->m_voxelization_rasterizer_state);
		assert(SUCCEEDED(res_device_create_rasterizer_state));
	}

	{
		D3D11_DEPTH_STENCIL_DESC depth_stencil_desc;
		depth_stencil_desc.DepthEnable = FALSE;
		depth_stencil_desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
		depth_stencil_desc.DepthFunc = D3D11_COMPARISON_ALWAYS;
		depth_stencil_desc.StencilEnable = FALSE;
		depth_stencil_desc.StencilReadMask = 0U;
		depth_stencil_desc.StencilWriteMask = 0U;
		depth_stencil_desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
		depth_stencil_desc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
		depth_stencil_desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
		depth_stencil_desc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
		depth_stencil_desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
		depth_stencil_desc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
		depth_stencil_desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
		depth_stencil_desc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;

		assert(NULL == this->m_voxelization_depth_stencil_state);

		HRESULT res_device_create_depth_stencil_state = device->CreateDepthStencilState(&depth_stencil_desc, &this->m_voxelization_depth_stencil_state);
		assert(SUCCEEDED(res_device_create_depth_stencil_state));
	}

	{
		D3D11_TEXTURE2D_DESC texture2d_desc;
		texture2d_desc.Width = std::max(1U, static_cast<uint32_t>(g_resolution_width) / 2U);
		texture2d_desc.Height = std::max(1U, static_cast<uint32_t>(g_resolution_height) / 2U);
		texture2d_desc.MipLevels = 1U;
		texture2d_desc.ArraySize = 1U;
		texture2d_desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		texture2d_desc.SampleDesc.Count = 1U;
		texture2d_desc.SampleDesc.Quality = 0U;
		texture2d_desc.Usage = D3D11_USAGE_DEFAULT;
		texture2d_desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
		texture2d_desc.CPUAccessFlags = 0U;
		texture2d_desc.MiscFlags = 0U;

		ID3D11Texture2D *indirect_radiance_and_ambient_occlusion_texture = NULL;

		HRESULT res_device_create_texture2d = device->CreateTexture2D(&texture2d_desc, NULL, &indirect_radiance_and_ambient_occlusion_texture);
		assert(SUCCEEDED(res_device_create_texture2d));

		D3D11_UNORDERED_ACCESS_VIEW_DESC unordered_access_view_desc;
		unordered_access_view_desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		unordered_access_view_desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
		unordered_access_view_desc.Texture2D.MipSlice = 0U;

		assert(NULL == this->m_indirect_radiance_and_ambient_occlusion_uav);

		HRESULT res_device_create_render_target_view = device->CreateUnorderedAccessView(indirect_radiance_and_ambient_occlusion_texture, &unordered_access_view_desc, &this->m_indirect_radiance_and_ambient_occlusion_uav);
		assert(SUCCEEDED(res_device_create_render_target_view));

		D3D11_SHADER_RESOURCE_VIEW_DESC shader_resource_view_desc;
		shader_resource_view_desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		shader_resource_view_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		shader_resource_view_desc.Texture2D.MostDetailedMip = 0U;
		shader_resource_view_desc.Texture2D.MipLevels = 1U;

		assert(NULL == this->m_indirect_radiance_and_ambient_occlusion_srv);

		HRESULT res_device_create_shader_resource_view = device->CreateShaderResourceView(indirect_radiance_and_ambient_occlusion_texture, &shader_resource_view_desc, &this->m_indirect_radiance_and_ambient_occlusion_srv);
		assert(SUCCEEDED(res_device_create_shader_resource_view));

		indirect_radiance_and_ambient_occlusion_texture->Release();
	}

	{
		assert(NULL == this->m_cone_tracing_compute_shader);

		HRESULT res_device_create_compute_shader = device->CreateComputeShader(code_shader_cone_tracing_compute, sizeof(code_shader_cone_tracing_compute), NULL, &this->m_cone_tracing_compute_shader);
		assert(SUCCEEDED(res_device_create_compute_shader));
	}

	{
		D3D11_BUFFER_DESC buffer_desc;
		buffer_desc.ByteWidth = sizeof(cone_tracing_constant_buffer_binding);
		buffer_desc.Usage = D3D11_USAGE_DEFAULT;
		buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		buffer_desc.CPUAccessFlags = 0U;
		buffer_desc.MiscFlags = 0U;
		buffer_desc.StructureByteStride = 0U;

		assert(NULL == this->m_cone_tracing_constant);

		HRESULT res_device_create_buffer = device->CreateBuffer(&buffer_desc, NULL, &this->m_cone_tracing_constant);
		assert(SUCCEEDED(res_device_create_buffer));
	}

	{
		assert(NULL == this->m_post_processing_vertex_shader);

		HRESULT res_device_create_vertex_shader = device->CreateVertexShader(code_shader_post_processing_vertex, sizeof(code_shader_post_processing_vertex), NULL, &this->m_post_processing_vertex_shader);
		assert(SUCCEEDED(res_device_create_vertex_shader));
	}

	{
		assert(NULL == this->m_post_processing_pixel_shader);

		HRESULT res_device_create_pixel_shader = device->CreatePixelShader(code_shader_post_processing_fragment, sizeof(code_shader_post_processing_fragment), NULL, &this->m_post_processing_pixel_shader);
		assert(SUCCEEDED(res_device_create_pixel_shader));
	}

	{
		assert(NULL == this->m_post_processing_rasterizer_state);

		D3D11_RASTERIZER_DESC rasterizer_desc;
		rasterizer_desc.FillMode = D3D11_FILL_SOLID;
		rasterizer_desc.CullMode = D3D11_CULL_BACK;
		rasterizer_desc.FrontCounterClockwise = TRUE;
		rasterizer_desc.DepthBias = 0;
		rasterizer_desc.DepthBiasClamp = 0.0F;
		rasterizer_desc.SlopeScaledDepthBias = 0.0F;
		rasterizer_desc.DepthClipEnable = TRUE;
		rasterizer_desc.ScissorEnable = FALSE;
		rasterizer_desc.MultisampleEnable = TRUE;
		rasterizer_desc.AntialiasedLineEnable = FALSE;
		HRESULT res_device_create_rasterizer_state = device->CreateRasterizerState(&rasterizer_desc, &this->m_post_processing_rasterizer_state);
		assert(SUCCEEDED(res_device_create_rasterizer_state));
	}

	{
		D3D11_DEPTH_STENCIL_DESC depth_stencil_desc;
		depth_stencil_desc.DepthEnable = FALSE;
		depth_stencil_desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
		depth_stencil_desc.DepthFunc = D3D11_COMPARISON_ALWAYS;
		depth_stencil_desc.StencilEnable = FALSE;
		depth_stencil_desc.StencilReadMask = 0U;
		depth_stencil_desc.StencilWriteMask = 0U;
		depth_stencil_desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
		depth_stencil_desc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
		depth_stencil_desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
		depth_stencil_desc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
		depth_stencil_desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
		depth_stencil_desc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
		depth_stencil_desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
		depth_stencil_desc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;

		assert(NULL == this->m_post_processing_depth_stencil_state);

		HRESULT res_device_create_depth_stencil_state = device->CreateDepthStencilState(&depth_stencil_desc, &this->m_post_processing_depth_stencil_state);
		assert(SUCCEEDED(res_device_create_depth_stencil_state));
	}

	{
		ID3D11Texture2D *swap_chain_back_buffer = NULL;
		HRESULT res_swap_chain_get_buffer = swap_chain->GetBuffer(0U, IID_PPV_ARGS(&swap_chain_back_buffer));
		assert(SUCCEEDED(res_swap_chain_get_buffer));

		D3D11_RENDER_TARGET_VIEW_DESC render_target_view_desc;
		render_target_view_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		render_target_view_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		render_target_view_desc.Texture2D.MipSlice = 0U;

		assert(NULL == this->m_swap_chain_texture_rtv);

		HRESULT res_device_create_render_target_view = device->CreateRenderTargetView(swap_chain_back_buffer, &render_target_view_desc, &this->m_swap_chain_texture_rtv);
		assert(SUCCEEDED(res_device_create_render_target_view));

		swap_chain_back_buffer->Release();
	}

	{
		D3D11_SAMPLER_DESC sampler_desc;
		sampler_desc.Filter = D3D11_FILTER_ANISOTROPIC;
		sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		sampler_desc.MipLODBias = 0.0;
		sampler_desc.MaxAnisotropy = D3D11_MAX_MAXANISOTROPY;
		sampler_desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
		sampler_desc.BorderColor[0] = 1.0;
		sampler_desc.BorderColor[1] = 1.0;
		sampler_desc.BorderColor[2] = 1.0;
		sampler_desc.BorderColor[3] = 1.0;
		sampler_desc.MinLOD = 0.0;
		sampler_desc.MaxLOD = FLT_MAX;

		assert(NULL == this->m_wrap_anisotrop_sampler);

		HRESULT res_device_create_sampler = device->CreateSamplerState(&sampler_desc, &this->m_wrap_anisotrop_sampler);
		assert(SUCCEEDED(res_device_create_sampler));
	}

	{
		D3D11_SAMPLER_DESC sampler_desc;
		sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
		sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
		sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		sampler_desc.MipLODBias = 0.0;
		sampler_desc.MaxAnisotropy = 1U;
		sampler_desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
		sampler_desc.BorderColor[0] = 1.0;
		sampler_desc.BorderColor[1] = 1.0;
		sampler_desc.BorderColor[2] = 1.0;
		sampler_desc.BorderColor[3] = 1.0;
		sampler_desc.MinLOD = 0.0;
		sampler_desc.MaxLOD = FLT_MAX;

		assert(NULL == this->m_clamp_linear_sampler);

		HRESULT res_device_create_sampler = device->CreateSamplerState(&sampler_desc, &this->m_clamp_linear_sampler);
		assert(SUCCEEDED(res_device_create_sampler));
	}

	{
		D3D11_SAMPLER_DESC sampler_desc;
		sampler_desc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
		sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
		sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
		sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
		sampler_desc.MipLODBias = 0.0;
		sampler_desc.MaxAnisotropy = 1U;
		sampler_desc.ComparisonFunc = D3D11_COMPARISON_LESS;
		sampler_desc.BorderColor[0] = 1.0;
		sampler_desc.BorderColor[1] = 1.0;
		sampler_desc.BorderColor[2] = 1.0;
		sampler_desc.BorderColor[3] = 1.0;
		sampler_desc.MinLOD = 0.0;
		sampler_desc.MaxLOD = FLT_MAX;

		assert(NULL == this->m_shadow_comparison_border_linear_sampler);

		HRESULT res_device_create_sampler = device->CreateSamplerState(&sampler_desc, &this->m_shadow_comparison_border_linear_sampler);
		assert(SUCCEEDED(res_device_create_sampler));
	}

	bool res_load_surfaces = load_surfaces(device, this->m_surfaces);
	assert(res_load_surfaces);

	g_camera_eye_position = DirectX::XMFLOAT3(-0.709524453F, 1.81154013F, 0.484151244F);
	g_camera_eye_direction = DirectX::XMFLOAT3(0.0F, 0.0F, -1.00000000F);
	g_camera_up_direction = DirectX::XMFLOAT3(0.0, 1.0, 0.0);

	// initialize
	{
		device_context->CSSetShader(this->m_zero_clipmap_compute_shader, NULL, 0U);

		{
			ID3D11UnorderedAccessView *const zero_clipmap_uavs[4] = {this->m_clipmap_texture_mask_uav, this->m_clipmap_texture_illumination_opacity_r16g16_uav, this->m_clipmap_texture_illumination_opacity_b16a16_uav, this->m_clipmap_texture_illumination_opacity_r16g16b16a16_uav};
			device_context->CSSetUnorderedAccessViews(0U, sizeof(zero_clipmap_uavs) / sizeof(zero_clipmap_uavs[0]), zero_clipmap_uavs, NULL);
		}

		{
			DirectX::XMUINT3 dispatch_extent = brx_voxel_cone_tracing_zero_dispatch_extent();

			device_context->Dispatch(dispatch_extent.x, dispatch_extent.y, dispatch_extent.z);
		}

		{
			ID3D11UnorderedAccessView *const null_uavs[4] = {NULL, NULL, NULL, NULL};
			device_context->CSSetUnorderedAccessViews(0U, sizeof(null_uavs) / sizeof(null_uavs[0]), null_uavs, NULL);
		}
	}
}

void Demo::Tick(ID3D11DeviceContext *device_context, ID3DUserDefinedAnnotation *user_defined_annotation, IDXGISwapChain *swap_chain)
{
	DirectX::XMFLOAT4X4 light_view_transform;
	DirectX::XMFLOAT4X4 light_projection_transform;
	{
		float const light_range = g_light_size * 10.0F;

		// TODO: we need to ensure all voxels are within the shadow map
#if 1
		DirectX::XMFLOAT3 const g_light_projection_center_position(0.0104755750F, 11.4115410F, 0.0841513649F);
#else
		DirectX::XMFLOAT3 g_light_projection_center_position = g_camera_eye_position;

#endif
		DirectX::XMVECTOR light_eye_position = DirectX::XMVectorAdd(DirectX::XMLoadFloat3(&g_light_projection_center_position), DirectX::XMVectorScale(DirectX::XMLoadFloat3(&g_light_direction), light_range * 0.5F));

		DirectX::XMVECTOR light_eye_direction = DirectX::XMVectorNegate(DirectX::XMLoadFloat3(&g_light_direction));

		DirectX::XMFLOAT3 light_up_direction(0.0F, 1.0F, 0.0F);

		DirectX::XMStoreFloat4x4(&light_view_transform, DirectX::XMMatrixLookToRH(light_eye_position, light_eye_direction, DirectX::XMLoadFloat3(&light_up_direction)));

		DirectX::XMStoreFloat4x4(&light_projection_transform, DirectX::XMMatrixOrthographicRH(g_light_size, g_light_size, light_range * 0.01F, light_range));
	}

	DirectX::XMFLOAT4X4 view_transform;
	DirectX::XMFLOAT4X4 projection_transform;
	DirectX::XMFLOAT4X4 inverse_view_transform;
	DirectX::XMFLOAT4X4 inverse_projection_transform;
	{
		DirectX::XMMATRIX simd_view_transform = DirectX::XMMatrixLookToRH(DirectX::XMLoadFloat3(&g_camera_eye_position), DirectX::XMLoadFloat3(&g_camera_eye_direction), DirectX::XMLoadFloat3(&g_camera_up_direction));

		DirectX::XMMATRIX simd_projection_transform = brx_DirectX_Math_Matrix_PerspectiveFovRH_ReversedZ(DirectX::XM_PIDIV4, (static_cast<float>(g_resolution_width) / static_cast<float>(g_resolution_height)), 1.0F, 10000.0F);

		DirectX::XMStoreFloat4x4(&view_transform, simd_view_transform);

		DirectX::XMStoreFloat4x4(&projection_transform, simd_projection_transform);

		DirectX::XMStoreFloat4x4(&inverse_view_transform, DirectX::XMMatrixInverse(NULL, simd_view_transform));

		DirectX::XMStoreFloat4x4(&inverse_projection_transform, DirectX::XMMatrixInverse(NULL, simd_projection_transform));
	}

	{
		user_defined_annotation->BeginEvent(L"Shadow Map");

		{
			shadow_map_constant_buffer_binding constant;

			constant.view_transform = light_view_transform;

			constant.projection_transform = light_projection_transform;

			DirectX::XMStoreFloat4x4(&constant.model_transform, DirectX::XMMatrixIdentity());

			device_context->UpdateSubresource(this->m_shadow_map_constant, 0U, NULL, &constant, sizeof(shadow_map_constant_buffer_binding), sizeof(shadow_map_constant_buffer_binding));
		}

		device_context->OMSetRenderTargets(0U, NULL, this->m_shadow_map_dsv);

		{
			FLOAT const depth = 1.0F;
			device_context->ClearDepthStencilView(this->m_shadow_map_dsv, D3D11_CLEAR_DEPTH, depth, 0U);
		}

		device_context->IASetInputLayout(NULL);
		device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		device_context->VSSetShader(this->m_shadow_map_vertex_shader, NULL, 0U);
		device_context->PSSetShader(NULL, NULL, 0U);
		device_context->RSSetState(this->m_shadow_map_rasterizer_state);
		device_context->OMSetDepthStencilState(this->m_shadow_map_depth_stencil_state, 0U);

		{
			D3D11_VIEWPORT viewports[1] = {{0.0F, 0.0F, static_cast<float>(g_shadow_map_size), static_cast<float>(g_shadow_map_size), 0.0F, 1.0F}};
			device_context->RSSetViewports(1U, viewports);
		}

		device_context->VSSetConstantBuffers(0U, 1U, &this->m_shadow_map_constant);

		for (auto const &surface : this->m_surfaces)
		{
			ID3D11ShaderResourceView *const surface_srvs[] = {surface.m_index_buffer, surface.m_vertex_buffer_position};
			device_context->VSSetShaderResources(1U, sizeof(surface_srvs) / sizeof(surface_srvs[0]), surface_srvs);

			device_context->DrawInstanced(surface.m_index_count, 1U, 0U, 0U);
		}

		device_context->OMSetRenderTargets(0U, NULL, NULL);

		user_defined_annotation->EndEvent();
	}

	{
		user_defined_annotation->BeginEvent(L"Forward Shading");

		{
			forward_shading_constant_buffer_binding constant;

			constant.view_transform = view_transform;

			constant.projection_transform = projection_transform;

			constant.inverse_view_transform = inverse_view_transform;

			constant.light_view_transform = light_view_transform;

			constant.light_projection_transform = light_projection_transform;

			constant.light_direction = g_light_direction;

			constant.light_color = g_light_color;

			constant.shadow_map_size = g_shadow_map_size;

			DirectX::XMStoreFloat4x4(&constant.model_transform, DirectX::XMMatrixIdentity());

			device_context->UpdateSubresource(this->m_forward_shading_constant, 0U, NULL, &constant, sizeof(forward_shading_constant_buffer_binding), sizeof(forward_shading_constant_buffer_binding));
		}

		{
			ID3D11RenderTargetView *const forward_shading_rtvs[4] = {this->m_direct_radiance_rtv, this->m_gbuffer_base_color_rtv, this->m_gbuffer_roughness_metallic_rtv, this->m_gbuffer_normal_rtv};
			device_context->OMSetRenderTargets(sizeof(forward_shading_rtvs) / sizeof(forward_shading_rtvs[0]), forward_shading_rtvs, this->m_scene_depth_dsv);
		}

		{
			FLOAT const color[4] = {0.0F, 0.0F, 0.0F, 0.0F};
			device_context->ClearRenderTargetView(this->m_direct_radiance_rtv, color);
			device_context->ClearRenderTargetView(this->m_gbuffer_base_color_rtv, color);
			device_context->ClearRenderTargetView(this->m_gbuffer_roughness_metallic_rtv, color);
			device_context->ClearRenderTargetView(this->m_gbuffer_normal_rtv, color);

			FLOAT const depth = 0.0F;
			device_context->ClearDepthStencilView(this->m_scene_depth_dsv, D3D11_CLEAR_DEPTH, depth, 0U);
		}

		device_context->IASetInputLayout(NULL);
		device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		device_context->VSSetShader(this->m_forward_shading_vertex_shader, NULL, 0U);
		device_context->PSSetShader(this->m_forward_shading_pixel_shader, NULL, 0U);
		device_context->RSSetState(this->m_forward_shading_rasterizer_state);
		device_context->OMSetDepthStencilState(this->m_forward_shading_depth_stencil_state, 0U);

		{
			D3D11_VIEWPORT viewports[1] = {{0.0F, 0.0F, static_cast<float>(g_resolution_width), static_cast<float>(g_resolution_height), 0.0F, 1.0F}};
			device_context->RSSetViewports(1U, viewports);
		}

		device_context->VSSetConstantBuffers(0U, 1U, &this->m_forward_shading_constant);
		device_context->PSSetConstantBuffers(0U, 1U, &this->m_forward_shading_constant);

		{
			ID3D11ShaderResourceView *const forward_shading_srvs[1] = {this->m_shadow_map_srv};
			device_context->VSSetShaderResources(0U, sizeof(forward_shading_srvs) / sizeof(forward_shading_srvs[0]), forward_shading_srvs);
			device_context->PSSetShaderResources(0U, sizeof(forward_shading_srvs) / sizeof(forward_shading_srvs[0]), forward_shading_srvs);
		}

		for (auto const &surface : this->m_surfaces)
		{
			device_context->VSSetConstantBuffers(1U, 1U, &surface.m_factor);
			device_context->PSSetConstantBuffers(1U, 1U, &surface.m_factor);

			ID3D11ShaderResourceView *const surface_srvs[] = {surface.m_index_buffer, surface.m_vertex_buffer_position, surface.m_vertex_buffer_varying, surface.m_normal_texture, surface.m_base_color_texture, surface.m_roughness_metallic_texture};
			device_context->VSSetShaderResources(1U, sizeof(surface_srvs) / sizeof(surface_srvs[0]), surface_srvs);
			device_context->PSSetShaderResources(1U, sizeof(surface_srvs) / sizeof(surface_srvs[0]), surface_srvs);

			ID3D11SamplerState *const surface_samplers[] = {this->m_wrap_anisotrop_sampler, this->m_shadow_comparison_border_linear_sampler};
			device_context->VSSetSamplers(0U, sizeof(surface_samplers) / sizeof(surface_samplers[0]), surface_samplers);
			device_context->PSSetSamplers(0U, sizeof(surface_samplers) / sizeof(surface_samplers[0]), surface_samplers);

			device_context->DrawInstanced(surface.m_index_count, 1U, 0U, 0U);
		}

		{
			ID3D11ShaderResourceView *const null_srvs[1] = {NULL};
			device_context->VSSetShaderResources(0U, sizeof(null_srvs) / sizeof(null_srvs[0]), null_srvs);
			device_context->PSSetShaderResources(0U, sizeof(null_srvs) / sizeof(null_srvs[0]), null_srvs);
		}

		{
			ID3D11RenderTargetView *const null_rtvs[4] = {NULL, NULL, NULL, NULL};
			device_context->OMSetRenderTargets(sizeof(null_rtvs) / sizeof(null_rtvs[0]), null_rtvs, this->m_scene_depth_dsv);
		}

		user_defined_annotation->EndEvent();
	}

	{
		user_defined_annotation->BeginEvent(L"Clear");

		device_context->CSSetShader(this->m_clear_clipmap_compute_shader, NULL, 0U);

		{
			ID3D11UnorderedAccessView *const clear_clipmap_uavs[4] = {this->m_clipmap_texture_mask_uav, this->m_clipmap_texture_illumination_opacity_r16g16_uav, this->m_clipmap_texture_illumination_opacity_b16a16_uav, this->m_clipmap_texture_illumination_opacity_r16g16b16a16_uav};
			device_context->CSSetUnorderedAccessViews(0U, sizeof(clear_clipmap_uavs) / sizeof(clear_clipmap_uavs[0]), clear_clipmap_uavs, NULL);
		}

		{
			DirectX::XMUINT3 dispatch_extent = brx_voxel_cone_tracing_clear_dispatch_extent();

			device_context->Dispatch(dispatch_extent.x, dispatch_extent.y, dispatch_extent.z);
		}

		{
			ID3D11UnorderedAccessView *const null_uavs[4] = {NULL, NULL, NULL, NULL};
			device_context->CSSetUnorderedAccessViews(0U, sizeof(null_uavs) / sizeof(null_uavs[0]), null_uavs, NULL);
		}

		user_defined_annotation->EndEvent();
	}

	{
		user_defined_annotation->BeginEvent(L"Voxelization");

		{
			voxelization_constant_buffer_binding constant;

			DirectX::XMFLOAT3 clipmap_anchor = brx_voxel_cone_tracing_resource_clipmap_anchor(g_camera_eye_position, g_camera_eye_direction);

			DirectX::XMFLOAT3 clipmap_center = brx_voxel_cone_tracing_resource_clipmap_center(clipmap_anchor);

			for (uint32_t viewport_depth_direction_index = 0; viewport_depth_direction_index < BRX_VCT_VIEWPORT_DEPTH_DIRECTION_COUNT; ++viewport_depth_direction_index)
			{
				constant.viewport_depth_direction_view_matrices[viewport_depth_direction_index] = brx_voxel_cone_tracing_voxelization_compute_viewport_depth_direction_view_matrix(clipmap_center, viewport_depth_direction_index);
			}

			for (uint32_t stack_level = 0; stack_level < BRX_VCT_CLIPMAP_STACK_LEVEL_COUNT; ++stack_level)
			{
				constant.clipmap_stack_level_projection_matrices[stack_level] = brx_voxel_cone_tracing_voxelization_compute_clipmap_stack_level_projection_matrix(stack_level);
			}

			DirectX::XMStoreFloat4x4(&constant.model_transform, DirectX::XMMatrixIdentity());

			constant.light_view_transform = light_view_transform;

			constant.light_projection_transform = light_projection_transform;

			constant.light_direction = g_light_direction;

			constant.light_color = g_light_color;

			constant.shadow_map_size = g_shadow_map_size;

			device_context->UpdateSubresource(this->m_voxelization_constant, 0U, NULL, &constant, sizeof(voxelization_constant_buffer_binding), sizeof(voxelization_constant_buffer_binding));
		}

		{
			ID3D11UnorderedAccessView *const voxelization_uavs[3] = {this->m_clipmap_texture_mask_uav, this->m_clipmap_texture_illumination_opacity_r16g16_uav, this->m_clipmap_texture_illumination_opacity_b16a16_uav};
			device_context->OMSetRenderTargetsAndUnorderedAccessViews(0U, NULL, this->m_voxelization_depth_dsv, 0U, sizeof(voxelization_uavs) / sizeof(voxelization_uavs[0]), voxelization_uavs, NULL);
		}

		device_context->IASetInputLayout(NULL);
		device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		device_context->VSSetShader(this->m_voxelization_vertex_shader, NULL, 0U);
		device_context->PSSetShader(this->m_voxelization_pixel_shader, NULL, 0U);
		device_context->RSSetState(this->m_voxelization_rasterizer_state);
		device_context->OMSetDepthStencilState(this->m_voxelization_depth_stencil_state, 0U);

		{
			D3D11_VIEWPORT viewports[1] = {{0.0F, 0.0F, static_cast<float>(BRX_VCT_CLIPMAP_MAP_SIZE), static_cast<float>(BRX_VCT_CLIPMAP_MAP_SIZE), 0.0F, 1.0F}};
			device_context->RSSetViewports(1U, viewports);
		}

		device_context->VSSetConstantBuffers(0U, 1U, &this->m_voxelization_constant);
		device_context->PSSetConstantBuffers(0U, 1U, &this->m_voxelization_constant);

		{
			ID3D11ShaderResourceView *const voxelization_srvs[1] = {this->m_shadow_map_srv};
			device_context->VSSetShaderResources(0U, sizeof(voxelization_srvs) / sizeof(voxelization_srvs[0]), voxelization_srvs);
			device_context->PSSetShaderResources(0U, sizeof(voxelization_srvs) / sizeof(voxelization_srvs[0]), voxelization_srvs);
		}

		for (auto const &surface : this->m_surfaces)
		{
			device_context->VSSetConstantBuffers(1U, 1U, &surface.m_factor);
			device_context->PSSetConstantBuffers(1U, 1U, &surface.m_factor);

			ID3D11ShaderResourceView *const surface_srvs[] = {surface.m_index_buffer, surface.m_vertex_buffer_position, surface.m_vertex_buffer_varying, surface.m_normal_texture, surface.m_base_color_texture, surface.m_roughness_metallic_texture};
			device_context->VSSetShaderResources(1U, sizeof(surface_srvs) / sizeof(surface_srvs[0]), surface_srvs);
			device_context->PSSetShaderResources(1U, sizeof(surface_srvs) / sizeof(surface_srvs[0]), surface_srvs);

			ID3D11SamplerState *const surface_samplers[] = {this->m_wrap_anisotrop_sampler, this->m_shadow_comparison_border_linear_sampler};
			device_context->VSSetSamplers(0U, sizeof(surface_samplers) / sizeof(surface_samplers[0]), surface_samplers);
			device_context->PSSetSamplers(0U, sizeof(surface_samplers) / sizeof(surface_samplers[0]), surface_samplers);

			device_context->DrawInstanced(surface.m_index_count, static_cast<uint32_t>(BRX_VCT_CLIPMAP_STACK_LEVEL_COUNT), 0U, 0U);
		}

		{
			ID3D11ShaderResourceView *const null_srvs[1] = {NULL};
			device_context->VSSetShaderResources(0U, sizeof(null_srvs) / sizeof(null_srvs[0]), null_srvs);
			device_context->PSSetShaderResources(0U, sizeof(null_srvs) / sizeof(null_srvs[0]), null_srvs);
		}

		{
			ID3D11UnorderedAccessView *const null_uavs[3] = {NULL, NULL, NULL};
			device_context->OMSetRenderTargetsAndUnorderedAccessViews(0U, NULL, NULL, 0U, sizeof(null_uavs) / sizeof(null_uavs[0]), null_uavs, NULL);
		}

		user_defined_annotation->EndEvent();
	}

	{
		user_defined_annotation->BeginEvent(L"Pack");

		device_context->CSSetShader(this->m_pack_clipmap_compute_shader, NULL, 0U);

		{
			ID3D11UnorderedAccessView *const pack_clipmap_uavs[1] = {this->m_clipmap_texture_illumination_opacity_r16g16b16a16_uav};
			device_context->CSSetUnorderedAccessViews(0U, sizeof(pack_clipmap_uavs) / sizeof(pack_clipmap_uavs[0]), pack_clipmap_uavs, NULL);
		}

		{
			ID3D11ShaderResourceView *const pack_tracing_srvs[3] = {this->m_clipmap_texture_mask_srv, this->m_clipmap_texture_illumination_opacity_r16g16_srv, this->m_clipmap_texture_illumination_opacity_b16a16_srv};
			device_context->CSSetShaderResources(0U, sizeof(pack_tracing_srvs) / sizeof(pack_tracing_srvs[0]), pack_tracing_srvs);
		}

		{
			DirectX::XMUINT3 dispatch_extent = brx_voxel_cone_tracing_pack_dispatch_extent();

			device_context->Dispatch(dispatch_extent.x, dispatch_extent.y, dispatch_extent.z);
		}

		{
			ID3D11ShaderResourceView *const null_srvs[3] = {NULL, NULL, NULL};
			device_context->CSSetShaderResources(0U, sizeof(null_srvs) / sizeof(null_srvs[0]), null_srvs);
		}

		{
			ID3D11UnorderedAccessView *const null_uavs[1] = {NULL};
			device_context->CSSetUnorderedAccessViews(0U, sizeof(null_uavs) / sizeof(null_uavs[0]), null_uavs, NULL);
		}

		user_defined_annotation->EndEvent();
	}

	{
		user_defined_annotation->BeginEvent(L"Cone Tracing");

		{
			cone_tracing_constant_buffer_binding constant;

			constant.inverse_view_transform = inverse_view_transform;

			constant.inverse_projection_transform = inverse_projection_transform;

			constant.clipmap_anchor = brx_voxel_cone_tracing_resource_clipmap_anchor(g_camera_eye_position, g_camera_eye_direction);

			constant.clipmap_center = brx_voxel_cone_tracing_resource_clipmap_center(constant.clipmap_anchor);

			device_context->UpdateSubresource(this->m_cone_tracing_constant, 0U, NULL, &constant, sizeof(cone_tracing_constant_buffer_binding), sizeof(cone_tracing_constant_buffer_binding));
		}

		device_context->CSSetShader(this->m_cone_tracing_compute_shader, NULL, 0U);

		device_context->CSSetConstantBuffers(0U, 1U, &this->m_cone_tracing_constant);

		{
			ID3D11UnorderedAccessView *const cone_tracing_uavs[1] = {this->m_indirect_radiance_and_ambient_occlusion_uav};
			device_context->CSSetUnorderedAccessViews(0U, sizeof(cone_tracing_uavs) / sizeof(cone_tracing_uavs[0]), cone_tracing_uavs, NULL);
		}

		{
			ID3D11ShaderResourceView *const cone_tracing_srvs[5] = {this->m_gbuffer_base_color_srv, this->m_gbuffer_roughness_metallic_srv, this->m_gbuffer_normal_srv, this->m_scene_depth_srv, this->m_clipmap_texture_illumination_opacity_r16g16b16a16_srv};
			device_context->CSSetShaderResources(0U, sizeof(cone_tracing_srvs) / sizeof(cone_tracing_srvs[0]), cone_tracing_srvs);
		}

		{
			ID3D11SamplerState *const cone_tracing_samplers[] = {this->m_clamp_linear_sampler};
			device_context->CSSetSamplers(0U, sizeof(cone_tracing_samplers) / sizeof(cone_tracing_samplers[0]), cone_tracing_samplers);
		}

		device_context->Dispatch(std::max(1U, static_cast<uint32_t>(g_resolution_width) / 2U), std::max(1U, static_cast<uint32_t>(g_resolution_height) / 2U), 1U);

		{
			ID3D11ShaderResourceView *const null_srvs[5] = {NULL, NULL, NULL, NULL, NULL};
			device_context->CSSetShaderResources(0U, sizeof(null_srvs) / sizeof(null_srvs[0]), null_srvs);
		}

		{
			ID3D11UnorderedAccessView *const null_uavs[1] = {NULL};
			device_context->CSSetUnorderedAccessViews(0U, sizeof(null_uavs) / sizeof(null_uavs[0]), null_uavs, NULL);
		}

		user_defined_annotation->EndEvent();
	}

	{
		user_defined_annotation->BeginEvent(L"Post Processing");

		{
			ID3D11RenderTargetView *const post_processing_rtvs[1] = {this->m_swap_chain_texture_rtv};
			device_context->OMSetRenderTargets(sizeof(post_processing_rtvs) / sizeof(post_processing_rtvs[0]), post_processing_rtvs, NULL);
		}

		device_context->IASetInputLayout(NULL);
		device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		device_context->VSSetShader(this->m_post_processing_vertex_shader, NULL, 0U);
		device_context->PSSetShader(this->m_post_processing_pixel_shader, NULL, 0U);
		device_context->RSSetState(this->m_post_processing_rasterizer_state);
		device_context->OMSetDepthStencilState(this->m_post_processing_depth_stencil_state, 0U);

		{
			D3D11_VIEWPORT viewports[1] = {{0.0F, 0.0F, static_cast<float>(g_resolution_width), static_cast<float>(g_resolution_height), 0.0F, 1.0F}};
			device_context->RSSetViewports(1U, viewports);
		}

		{
			ID3D11ShaderResourceView *const post_processing_srvs[2] = {this->m_direct_radiance_srv, this->m_indirect_radiance_and_ambient_occlusion_srv};
			device_context->VSSetShaderResources(0U, sizeof(post_processing_srvs) / sizeof(post_processing_srvs[0]), post_processing_srvs);
			device_context->PSSetShaderResources(0U, sizeof(post_processing_srvs) / sizeof(post_processing_srvs[0]), post_processing_srvs);
		}

		{

			ID3D11SamplerState *const surface_samplers[] = {this->m_clamp_linear_sampler};
			device_context->VSSetSamplers(0U, sizeof(surface_samplers) / sizeof(surface_samplers[0]), surface_samplers);
			device_context->PSSetSamplers(0U, sizeof(surface_samplers) / sizeof(surface_samplers[0]), surface_samplers);
		}

		device_context->DrawInstanced(3U, 1U, 0U, 0U);

		{
			ID3D11ShaderResourceView *const null_srvs[2] = {NULL};
			device_context->VSSetShaderResources(0U, sizeof(null_srvs) / sizeof(null_srvs[0]), null_srvs);
			device_context->PSSetShaderResources(0U, sizeof(null_srvs) / sizeof(null_srvs[0]), null_srvs);
		}

		{
			ID3D11RenderTargetView *const null_rtvs[1] = {NULL};

			device_context->OMSetRenderTargets(sizeof(null_rtvs) / sizeof(null_rtvs[0]), null_rtvs, NULL);
		}

		user_defined_annotation->EndEvent();
	}

	{
		HRESULT res_swap_chain_present = swap_chain->Present(1U, 0U);
		assert(SUCCEEDED(res_swap_chain_present));
	}
}

void Demo::UnInit()
{
	this->m_shadow_map_dsv->Release();
	this->m_shadow_map_srv->Release();

	this->m_shadow_map_constant->Release();
	this->m_shadow_map_vertex_shader->Release();
	this->m_shadow_map_rasterizer_state->Release();
	this->m_shadow_map_depth_stencil_state->Release();

	this->m_direct_radiance_rtv->Release();
	this->m_direct_radiance_srv->Release();
	this->m_gbuffer_base_color_rtv->Release();
	this->m_gbuffer_base_color_srv->Release();
	this->m_gbuffer_roughness_metallic_rtv->Release();
	this->m_gbuffer_roughness_metallic_srv->Release();
	this->m_gbuffer_normal_rtv->Release();
	this->m_gbuffer_normal_srv->Release();
	this->m_scene_depth_dsv->Release();
	this->m_scene_depth_srv->Release();

	this->m_forward_shading_constant->Release();
	this->m_forward_shading_vertex_shader->Release();
	this->m_forward_shading_pixel_shader->Release();
	this->m_forward_shading_rasterizer_state->Release();
	this->m_forward_shading_depth_stencil_state->Release();

	this->m_clipmap_texture_mask_uav->Release();
	this->m_clipmap_texture_mask_srv->Release();
	this->m_clipmap_texture_illumination_opacity_r16g16_uav->Release();
	this->m_clipmap_texture_illumination_opacity_r16g16_srv->Release();
	this->m_clipmap_texture_illumination_opacity_b16a16_uav->Release();
	this->m_clipmap_texture_illumination_opacity_b16a16_srv->Release();
	this->m_clipmap_texture_illumination_opacity_r16g16b16a16_uav->Release();
	this->m_clipmap_texture_illumination_opacity_r16g16b16a16_srv->Release();

	this->m_zero_clipmap_compute_shader->Release();
	this->m_clear_clipmap_compute_shader->Release();
	this->m_pack_clipmap_compute_shader->Release();

	this->m_voxelization_constant->Release();
	this->m_voxelization_depth_dsv->Release();
	this->m_voxelization_vertex_shader->Release();
	this->m_voxelization_pixel_shader->Release();
	this->m_voxelization_rasterizer_state->Release();
	this->m_voxelization_depth_stencil_state->Release();

	this->m_indirect_radiance_and_ambient_occlusion_uav->Release();
	this->m_indirect_radiance_and_ambient_occlusion_srv->Release();

	this->m_cone_tracing_constant->Release();
	this->m_cone_tracing_compute_shader->Release();

	this->m_post_processing_vertex_shader->Release();
	this->m_post_processing_pixel_shader->Release();
	this->m_post_processing_rasterizer_state->Release();
	this->m_post_processing_depth_stencil_state->Release();

	this->m_swap_chain_texture_rtv->Release();

	this->m_wrap_anisotrop_sampler->Release();
	this->m_clamp_linear_sampler->Release();
	this->m_shadow_comparison_border_linear_sampler->Release();

	for (auto const &surface : this->m_surfaces)
	{
		surface.m_vertex_buffer_position->Release();
		surface.m_vertex_buffer_varying->Release();
		surface.m_index_buffer->Release();
		surface.m_normal_texture->Release();
		surface.m_base_color_texture->Release();
		surface.m_roughness_metallic_texture->Release();
		surface.m_factor->Release();
	}
}
