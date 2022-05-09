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

#define _CRT_SECURE_NO_WARNINGS
#include "surface.h"
#include <cmath>
#include <cassert>
#include <string>
#include <stdexcept>
#include <iostream>
#include <DirectXPackedVector.h>
#include "../shaders/surface_resource_binding.hlsli"
#include "../thirdparty/Brioche-Shader-Language/include/brx_packed_vector.h"
#include "../thirdparty/Brioche-Shader-Language/include/brx_octahedral_mapping.h"
#include "SDKmisc.h"
#include "../thirdparty/libpng/png.h"
#define CGLTF_IMPLEMENTATION
#include "../thirdparty/cgltf/cgltf.h"

static cgltf_result _internal_cgltf_custom_read_file(const struct cgltf_memory_options *memory_options, const struct cgltf_file_options *, const char *path, cgltf_size *size, void **data);

static void _internal_cgltf_custom_file_release(const struct cgltf_memory_options *memory_options, const struct cgltf_file_options *, void *data, cgltf_size size);

static void *_internal_cgltf_custom_alloc(void *, cgltf_size size);

static void _internal_cgltf_custom_free(void *, void *ptr);

static void PNGCBAPI _internal_libpng_error_callback(png_structp png_ptr, png_const_charp error_message);

static png_voidp PNGCBAPI _internal_libpng_malloc_callback(png_structp png_ptr, png_alloc_size_t size);

static void PNGCBAPI _internal_libpng_free_ptr(png_structp png_ptr, png_voidp ptr);

struct _internal_libpng_read_data_context
{
    void const *m_data_base;
    size_t m_data_size;
    size_t m_offset;
};

static void PNGCBAPI _internal_libpng_read_data_callback(png_structp png_ptr, png_bytep data, size_t length);

struct surface_vertex_position_buffer_element
{
    // R32G32B32_FLOAT
    float m_position[3];
};

struct surface_vertex_varying_buffer_element
{
    // R16G16_SNORM (octahedron map)
    uint32_t m_normal;
    // R15G15B2_SNORM (octahedron map + tangent w)
    uint32_t m_tangent;
    // use R16G16_SFLOAT instead of R16G16_UNORM to support wrap
    uint32_t m_texcoord;
};

ID3D11ShaderResourceView *internal_load_texture_from_file(ID3D11Device *device, char const *model_path, char const *image_path, bool force_srgb);

bool load_surfaces(ID3D11Device *device, std::vector<surface> &out_surfaces)
{
    char surface_group_path[2 * MAX_PATH];
    {
        wchar_t surface_group_path_w[MAX_PATH];
        HRESULT res_dxut_find_dxsdk_media_file_cch = DXUTFindDXSDKMediaFileCch(surface_group_path_w, MAX_PATH, L"thirdparty/sponza/sponza.gltf");
        assert(SUCCEEDED(res_dxut_find_dxsdk_media_file_cch));

        int written = WideCharToMultiByte(CP_UTF8, 0U, surface_group_path_w, std::wcslen(surface_group_path_w), surface_group_path, 2 * MAX_PATH, NULL, NULL);
        assert(written > 0);
        assert(written < (2 * MAX_PATH));
        surface_group_path[written] = '\0';
    }

    cgltf_data *data = NULL;
    {
        cgltf_options options = {};
        options.memory.alloc_func = _internal_cgltf_custom_alloc;
        options.memory.free_func = _internal_cgltf_custom_free;
        options.file.read = _internal_cgltf_custom_read_file;
        options.file.release = _internal_cgltf_custom_file_release;
        options.file.user_data = NULL;

        cgltf_result result_parse_file = cgltf_parse_file(&options, surface_group_path, &data);
        if (cgltf_result_success != result_parse_file)
        {
            return false;
        }

        cgltf_result result_load_buffers = cgltf_load_buffers(&options, data, surface_group_path);
        if (cgltf_result_success != result_load_buffers)
        {
            cgltf_free(data);
            return false;
        }
    }

    assert(1U == data->meshes_count);

    cgltf_mesh const *mesh = &data->meshes[0];

    assert(out_surfaces.empty());
    out_surfaces.resize(mesh->primitives_count);

    for (size_t primitive_index = 0U; primitive_index < mesh->primitives_count; ++primitive_index)
    {
        std::vector<uint32_t> raw_indices;
        std::vector<DirectX::XMFLOAT3> raw_positions;
        std::vector<DirectX::XMFLOAT3> raw_normals;
        std::vector<DirectX::XMFLOAT2> raw_texcoords;
        std::vector<DirectX::XMFLOAT4> raw_tangents;
        {
            cgltf_primitive const *const primitive = &mesh->primitives[primitive_index];

            cgltf_accessor const *position_accessor = NULL;
            cgltf_accessor const *normal_accessor = NULL;
            cgltf_accessor const *tangent_accessor = NULL;
            cgltf_accessor const *texcoord_accessor = NULL;
            {
                cgltf_int position_index = -1;
                cgltf_int normal_index = -1;
                cgltf_int tangent_index = -1;
                cgltf_int texcoord_index = -1;

                for (size_t vertex_attribute_index = 0; vertex_attribute_index < primitive->attributes_count; ++vertex_attribute_index)
                {
                    cgltf_attribute const *vertex_attribute = &primitive->attributes[vertex_attribute_index];

                    switch (vertex_attribute->type)
                    {
                    case cgltf_attribute_type_position:
                    {
                        assert(cgltf_attribute_type_position == vertex_attribute->type);

                        if (NULL == position_accessor || vertex_attribute->index < position_index)
                        {
                            position_accessor = vertex_attribute->data;
                            position_index = vertex_attribute->index;
                        }
                    }
                    break;
                    case cgltf_attribute_type_normal:
                    {
                        assert(cgltf_attribute_type_normal == vertex_attribute->type);

                        if (NULL == normal_accessor || vertex_attribute->index < normal_index)
                        {
                            normal_accessor = vertex_attribute->data;
                            normal_index = vertex_attribute->index;
                        }
                    }
                    break;
                    case cgltf_attribute_type_tangent:
                    {
                        assert(cgltf_attribute_type_tangent == vertex_attribute->type);

                        if (NULL == tangent_accessor || vertex_attribute->index < tangent_index)
                        {
                            tangent_accessor = vertex_attribute->data;
                            tangent_index = vertex_attribute->index;
                        }
                    }
                    break;
                    case cgltf_attribute_type_texcoord:
                    {
                        assert(cgltf_attribute_type_texcoord == vertex_attribute->type);

                        if (NULL == texcoord_accessor || vertex_attribute->index < texcoord_index)
                        {
                            texcoord_accessor = vertex_attribute->data;
                            texcoord_index = vertex_attribute->index;
                        }
                    }
                    break;
                    default:
                    {
                        // Do Nothing
                    }
                    }
                }
            }

            assert(NULL != position_accessor);
            assert(NULL != normal_accessor);
            assert(NULL != tangent_accessor);
            assert(NULL != texcoord_accessor);

            cgltf_accessor const *const index_accessor = primitive->indices;

            assert(NULL != index_accessor);

            size_t const vertex_count = position_accessor->count;
            size_t const index_count = index_accessor->count;

            assert(cgltf_primitive_type_triangles == primitive->type);
            assert(0U == (index_count % 3U));

            assert(raw_indices.empty());
            raw_indices.resize(index_count);
            assert(raw_positions.empty());
            raw_positions.resize(vertex_count);
            assert(raw_normals.empty());
            raw_normals.resize(vertex_count);
            assert(raw_texcoords.empty());
            raw_texcoords.resize(vertex_count);
            assert(raw_tangents.empty());
            raw_tangents.resize(vertex_count);

            // Index
            {
                uintptr_t index_base = -1;
                size_t index_stride = -1;
                {
                    cgltf_buffer_view const *const index_buffer_view = index_accessor->buffer_view;
                    index_base = reinterpret_cast<uintptr_t>(index_buffer_view->buffer->data) + index_buffer_view->offset + index_accessor->offset;
                    index_stride = (0 != index_buffer_view->stride) ? index_buffer_view->stride : index_accessor->stride;
                }

                assert(cgltf_type_scalar == index_accessor->type);

                switch (index_accessor->component_type)
                {
                case cgltf_component_type_r_8u:
                {
                    assert(cgltf_component_type_r_8u == index_accessor->component_type);

                    for (size_t index_index = 0; index_index < index_accessor->count; ++index_index)
                    {
                        uint8_t const *const index_ubyte = reinterpret_cast<uint8_t const *>(index_base + index_stride * index_index);

                        uint32_t const raw_index = static_cast<uint32_t>(*index_ubyte);

                        raw_indices[index_index] = raw_index;
                    }
                }
                break;
                case cgltf_component_type_r_16u:
                {
                    assert(cgltf_component_type_r_16u == index_accessor->component_type);

                    for (size_t index_index = 0; index_index < index_accessor->count; ++index_index)
                    {
                        uint16_t const *const index_ushort = reinterpret_cast<uint16_t const *>(index_base + index_stride * index_index);

                        uint32_t const raw_index = static_cast<uint32_t>(*index_ushort);

                        raw_indices[index_index] = raw_index;
                    }
                }
                break;
                case cgltf_component_type_r_32u:
                {
                    assert(cgltf_component_type_r_32u == index_accessor->component_type);

                    for (size_t index_index = 0; index_index < index_accessor->count; ++index_index)
                    {
                        uint32_t const *const index_uint = reinterpret_cast<uint32_t const *>(index_base + index_stride * index_index);

                        uint32_t const raw_index = (*index_uint);

                        raw_indices[index_index] = raw_index;
                    }
                }
                break;
                default:
                    assert(0);
                }
            }

            // Position
            {
                uintptr_t position_base = -1;
                size_t position_stride = -1;
                {
                    cgltf_buffer_view const *const position_buffer_view = position_accessor->buffer_view;
                    position_base = reinterpret_cast<uintptr_t>(position_buffer_view->buffer->data) + position_buffer_view->offset + position_accessor->offset;
                    position_stride = (0 != position_buffer_view->stride) ? position_buffer_view->stride : position_accessor->stride;
                }

                assert(cgltf_type_vec3 == position_accessor->type);
                assert(cgltf_component_type_r_32f == position_accessor->component_type);

                for (size_t vertex_index = 0; vertex_index < position_accessor->count; ++vertex_index)
                {
                    float const *const position_float3 = reinterpret_cast<float const *>(position_base + position_stride * vertex_index);

                    raw_positions[vertex_index] = DirectX::XMFLOAT3(position_float3[0], position_float3[1], position_float3[2]);
                }
            }

            // Normal
            {
                uintptr_t normal_base = -1;
                size_t normal_stride = -1;
                {
                    cgltf_buffer_view const *const normal_buffer_view = normal_accessor->buffer_view;
                    normal_base = reinterpret_cast<uintptr_t>(normal_buffer_view->buffer->data) + normal_buffer_view->offset + normal_accessor->offset;
                    normal_stride = (0 != normal_buffer_view->stride) ? normal_buffer_view->stride : normal_accessor->stride;
                }

                assert(normal_accessor->count == vertex_count);

                assert(cgltf_type_vec3 == normal_accessor->type);
                assert(cgltf_component_type_r_32f == normal_accessor->component_type);

                for (size_t vertex_index = 0; vertex_index < normal_accessor->count; ++vertex_index)
                {
                    float const *const normal_float3 = reinterpret_cast<float const *>(normal_base + normal_stride * vertex_index);

                    raw_normals[vertex_index] = DirectX::XMFLOAT3(normal_float3[0], normal_float3[1], normal_float3[2]);
                }
            }

            // Texcoord
            {
                uintptr_t texcoord_base = -1;
                size_t texcoord_stride = -1;
                {
                    cgltf_buffer_view const *const texcoord_buffer_view = texcoord_accessor->buffer_view;
                    texcoord_base = reinterpret_cast<uintptr_t>(texcoord_buffer_view->buffer->data) + texcoord_buffer_view->offset + texcoord_accessor->offset;
                    texcoord_stride = (0 != texcoord_buffer_view->stride) ? texcoord_buffer_view->stride : texcoord_accessor->stride;
                }

                assert(texcoord_accessor->count == vertex_count);

                assert(cgltf_type_vec2 == texcoord_accessor->type);

                switch (texcoord_accessor->component_type)
                {
                case cgltf_component_type_r_8u:
                {
                    assert(cgltf_component_type_r_8u == texcoord_accessor->component_type);

                    for (size_t vertex_index = 0; vertex_index < texcoord_accessor->count; ++vertex_index)
                    {
                        uint8_t const *const texcoord_ubyte2 = reinterpret_cast<uint8_t const *>(texcoord_base + texcoord_stride * vertex_index);

                        DirectX::PackedVector::XMUBYTEN2 packed_vector_ubyten2(texcoord_ubyte2[0], texcoord_ubyte2[1]);

                        DirectX::XMVECTOR unpacked_vector = DirectX::PackedVector::XMLoadUByteN2(&packed_vector_ubyten2);

                        DirectX::XMStoreFloat2(&raw_texcoords[vertex_index], unpacked_vector);
                    }
                }
                break;
                case cgltf_component_type_r_16u:
                {
                    assert(cgltf_component_type_r_16u == texcoord_accessor->component_type);

                    for (size_t vertex_index = 0; vertex_index < texcoord_accessor->count; ++vertex_index)
                    {
                        uint16_t const *const texcoord_ushortn2 = reinterpret_cast<uint16_t const *>(texcoord_base + texcoord_stride * vertex_index);

                        DirectX::PackedVector::XMUSHORTN2 packed_vector_ushortn2(texcoord_ushortn2[0], texcoord_ushortn2[1]);

                        DirectX::XMVECTOR unpacked_vector = DirectX::PackedVector::XMLoadUShortN2(&packed_vector_ushortn2);

                        DirectX::XMStoreFloat2(&raw_texcoords[vertex_index], unpacked_vector);
                    }
                }
                break;
                case cgltf_component_type_r_32f:
                {
                    assert(cgltf_component_type_r_32f == texcoord_accessor->component_type);

                    for (size_t vertex_index = 0; vertex_index < texcoord_accessor->count; ++vertex_index)
                    {
                        float const *const texcoord_float2 = reinterpret_cast<float const *>(texcoord_base + texcoord_stride * vertex_index);

                        raw_texcoords[vertex_index] = DirectX::XMFLOAT2(texcoord_float2[0], texcoord_float2[1]);
                    }
                }
                break;
                default:
                    assert(0);
                }
            }

            // Tangent
            {
                uintptr_t tangent_base = -1;
                size_t tangent_stride = -1;
                {
                    cgltf_buffer_view const *const tangent_buffer_view = tangent_accessor->buffer_view;
                    tangent_base = reinterpret_cast<uintptr_t>(tangent_buffer_view->buffer->data) + tangent_buffer_view->offset + tangent_accessor->offset;
                    tangent_stride = (0 != tangent_buffer_view->stride) ? tangent_buffer_view->stride : tangent_accessor->stride;
                }

                assert(tangent_accessor->count == vertex_count);

                assert(cgltf_type_vec4 == tangent_accessor->type);
                assert(cgltf_component_type_r_32f == tangent_accessor->component_type);

                for (size_t vertex_index = 0; vertex_index < tangent_accessor->count; ++vertex_index)
                {
                    float const *const tangent_float4 = reinterpret_cast<float const *>(tangent_base + tangent_stride * vertex_index);

                    raw_tangents[vertex_index] = DirectX::XMFLOAT4(tangent_float4[0], tangent_float4[1], tangent_float4[2], tangent_float4[3]);
                }
            }
        }

        float normal_texture_scale = 1.0F;
        std::string normal_texture_image_uri;
        DirectX::XMFLOAT3 emissive_factor;
        std::string emissive_texture_image_uri;
        DirectX::XMFLOAT4 base_color_factor;
        std::string base_color_texture_image_uri;
        float metallic_factor = 0.0F;
        float roughness_factor = 0.0F;
        std::string metallic_roughness_texture_image_uri;
        {
            cgltf_primitive const *const primitive = &mesh->primitives[primitive_index];

            cgltf_material const *const material = primitive->material;

            if (NULL != material->normal_texture.texture)
            {
                cgltf_image const *const normal_texture_image = material->normal_texture.texture->image;
                assert(NULL != normal_texture_image);
                assert(NULL == normal_texture_image->buffer_view);
                assert(NULL != normal_texture_image->uri);

                normal_texture_image_uri = normal_texture_image->uri;
                cgltf_decode_uri(&normal_texture_image_uri[0]);
                size_t null_terminator_pos = normal_texture_image_uri.find('\0');
                if (std::string::npos != null_terminator_pos)
                {
                    normal_texture_image_uri.resize(null_terminator_pos);
                }

                normal_texture_scale = material->normal_texture.scale;
            }

            if (material->has_emissive_strength)
            {
                emissive_factor = DirectX::XMFLOAT3(material->emissive_factor[0] * material->emissive_strength.emissive_strength, material->emissive_factor[1] * material->emissive_strength.emissive_strength, material->emissive_factor[2] * material->emissive_strength.emissive_strength);
            }
            else
            {
                emissive_factor = DirectX::XMFLOAT3(material->emissive_factor[0], material->emissive_factor[1], material->emissive_factor[2]);
            }

            if (NULL != material->emissive_texture.texture)
            {
                cgltf_image const *const emissive_texture_image = material->emissive_texture.texture->image;
                assert(NULL != emissive_texture_image);
                assert(NULL == emissive_texture_image->buffer_view);
                assert(NULL != emissive_texture_image->uri);

                emissive_texture_image_uri = emissive_texture_image->uri;
                cgltf_decode_uri(&emissive_texture_image_uri[0]);
                size_t null_terminator_pos = emissive_texture_image_uri.find('\0');
                if (std::string::npos != null_terminator_pos)
                {
                    emissive_texture_image_uri.resize(null_terminator_pos);
                }
            }

            if (material->has_pbr_metallic_roughness)
            {
                base_color_factor = DirectX::XMFLOAT4(material->pbr_metallic_roughness.base_color_factor[0], material->pbr_metallic_roughness.base_color_factor[1], material->pbr_metallic_roughness.base_color_factor[2], material->pbr_metallic_roughness.base_color_factor[3]);

                if (NULL != material->pbr_metallic_roughness.base_color_texture.texture)
                {
                    cgltf_image const *const base_color_texture_image = material->pbr_metallic_roughness.base_color_texture.texture->image;
                    assert(NULL != base_color_texture_image);
                    assert(NULL == base_color_texture_image->buffer_view);
                    assert(NULL != base_color_texture_image->uri);

                    base_color_texture_image_uri = base_color_texture_image->uri;
                    cgltf_decode_uri(&base_color_texture_image_uri[0]);
                    size_t null_terminator_pos = base_color_texture_image_uri.find('\0');
                    if (std::string::npos != null_terminator_pos)
                    {
                        base_color_texture_image_uri.resize(null_terminator_pos);
                    }
                }

                metallic_factor = material->pbr_metallic_roughness.metallic_factor;

                roughness_factor = material->pbr_metallic_roughness.roughness_factor;

                if (NULL != material->pbr_metallic_roughness.metallic_roughness_texture.texture)
                {
                    cgltf_image const *const metallic_roughness_texture_image = material->pbr_metallic_roughness.metallic_roughness_texture.texture->image;
                    assert(NULL != metallic_roughness_texture_image);
                    assert(NULL == metallic_roughness_texture_image->buffer_view);
                    assert(NULL != metallic_roughness_texture_image->uri);

                    metallic_roughness_texture_image_uri = metallic_roughness_texture_image->uri;
                    cgltf_decode_uri(&metallic_roughness_texture_image_uri[0]);
                    size_t null_terminator_pos = metallic_roughness_texture_image_uri.find('\0');
                    if (std::string::npos != null_terminator_pos)
                    {
                        metallic_roughness_texture_image_uri.resize(null_terminator_pos);
                    }
                }
            }
        }

        uint32_t const mesh_id = primitive_index;

        size_t const index_count = raw_indices.size();
        size_t const vertex_count = raw_positions.size();

        out_surfaces[mesh_id].m_index_count = index_count;

        std::vector<uint32_t> indices(static_cast<size_t>(index_count));
        std::vector<surface_vertex_position_buffer_element> vertices_position(static_cast<size_t>(vertex_count));
        std::vector<surface_vertex_varying_buffer_element> vertices_varying(static_cast<size_t>(vertex_count));

        for (size_t index_index = 0; index_index < index_count; ++index_index)
        {
            indices[index_index] = raw_indices[index_index];
        }

        for (size_t vertex_index = 0; vertex_index < vertex_count; ++vertex_index)
        {
            DirectX::XMFLOAT3 vertex_position;
            {
                DirectX::XMFLOAT3 raw_position = raw_positions[vertex_index];

                vertex_position = raw_position;
            }

            uint32_t vertex_packed_normal;
            {
                DirectX::XMFLOAT3 raw_normal = raw_normals[vertex_index];

                DirectX::XMFLOAT3 normalized_raw_normal;
                DirectX::XMStoreFloat3(&normalized_raw_normal, DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&raw_normal)));

                DirectX::XMFLOAT2 const mapped_normal = brx_octahedral_map(normalized_raw_normal);

                DirectX::PackedVector::XMSHORTN2 packed_normal;
                DirectX::PackedVector::XMStoreShortN2(&packed_normal, DirectX::XMLoadFloat2(&mapped_normal));

                vertex_packed_normal = packed_normal.v;
            }

            uint32_t vertex_packed_tangent;
            {
                DirectX::XMFLOAT3 raw_tangent_xyz(raw_tangents[vertex_index].x, raw_tangents[vertex_index].y, raw_tangents[vertex_index].z);
                float const raw_tangent_w = raw_tangents[vertex_index].w;

                DirectX::XMFLOAT3 normalized_raw_tangent_xyz;
                {
                    DirectX::XMVECTOR simd_raw_tangent_xyz = DirectX::XMLoadFloat3(&raw_tangent_xyz);
                    float length_raw_tangent_xyz = DirectX::XMVectorGetX(DirectX::XMVector3Length(simd_raw_tangent_xyz));
                    if (length_raw_tangent_xyz > 1E-5F)
                    {
                        DirectX::XMStoreFloat3(&normalized_raw_tangent_xyz, DirectX::XMVectorScale(simd_raw_tangent_xyz, 1.0F / length_raw_tangent_xyz));
                    }
                    else
                    {
                        normalized_raw_tangent_xyz = DirectX::XMFLOAT3(1.0F, 0.0F, 0.0F);
                    }
                }

                vertex_packed_tangent = brx_FLOAT3_to_R15G15B2_SNORM(brx_octahedral_map(normalized_raw_tangent_xyz), raw_tangent_w);
            }

            uint32_t vertex_packed_texcoord;
            {
                DirectX::XMFLOAT2 raw_texcoord = raw_texcoords[vertex_index];

                DirectX::PackedVector::XMHALF2 packed_texcoord;
                DirectX::PackedVector::XMStoreHalf2(&packed_texcoord, DirectX::XMLoadFloat2(&raw_texcoord));

                vertex_packed_texcoord = packed_texcoord.v;
            }

            vertices_position[vertex_index] = surface_vertex_position_buffer_element{{vertex_position.x, vertex_position.y, vertex_position.z}};
            vertices_varying[vertex_index] = surface_vertex_varying_buffer_element{vertex_packed_normal, vertex_packed_tangent, vertex_packed_texcoord};
        }

        {
            D3D11_BUFFER_DESC buffer_desc;
            buffer_desc.ByteWidth = sizeof(surface_vertex_position_buffer_element) * vertex_count;
            buffer_desc.Usage = D3D11_USAGE_IMMUTABLE;
            buffer_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
            buffer_desc.CPUAccessFlags = 0U;
            buffer_desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
            buffer_desc.StructureByteStride = 0U;

            D3D11_SUBRESOURCE_DATA subresource_data;
            subresource_data.pSysMem = vertices_position.data();
            subresource_data.SysMemPitch = sizeof(surface_vertex_position_buffer_element);
            subresource_data.SysMemSlicePitch = sizeof(surface_vertex_position_buffer_element);

            ID3D11Buffer *vertex_buffer_position = NULL;

            HRESULT res_device_create_buffer = device->CreateBuffer(&buffer_desc, &subresource_data, &vertex_buffer_position);
            assert(SUCCEEDED(res_device_create_buffer));

            D3D11_SHADER_RESOURCE_VIEW_DESC view_desc;
            view_desc.Format = DXGI_FORMAT_R32_TYPELESS;
            view_desc.ViewDimension = D3D_SRV_DIMENSION_BUFFEREX;
            view_desc.BufferEx.FirstElement = 0U;
            assert(0U == ((sizeof(surface_vertex_position_buffer_element) * vertex_count) % sizeof(uint32_t)));
            view_desc.BufferEx.NumElements = sizeof(surface_vertex_position_buffer_element) * vertex_count / sizeof(uint32_t);
            view_desc.BufferEx.Flags = D3D11_BUFFEREX_SRV_FLAG_RAW;
            HRESULT res_device_create_view = device->CreateShaderResourceView(vertex_buffer_position, &view_desc, &out_surfaces[mesh_id].m_vertex_buffer_position);
            assert(SUCCEEDED(res_device_create_view));

            vertex_buffer_position->Release();
        }

        {
            D3D11_BUFFER_DESC buffer_desc;
            buffer_desc.ByteWidth = sizeof(surface_vertex_varying_buffer_element) * vertex_count;
            buffer_desc.Usage = D3D11_USAGE_IMMUTABLE;
            buffer_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
            buffer_desc.CPUAccessFlags = 0U;
            buffer_desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
            buffer_desc.StructureByteStride = 0U;

            D3D11_SUBRESOURCE_DATA subresource_data;
            subresource_data.pSysMem = vertices_varying.data();
            subresource_data.SysMemPitch = sizeof(surface_vertex_varying_buffer_element);
            subresource_data.SysMemSlicePitch = sizeof(surface_vertex_varying_buffer_element);

            ID3D11Buffer *vertex_buffer_varying = NULL;

            HRESULT res_device_create_buffer = device->CreateBuffer(&buffer_desc, &subresource_data, &vertex_buffer_varying);
            assert(SUCCEEDED(res_device_create_buffer));

            D3D11_SHADER_RESOURCE_VIEW_DESC view_desc;
            view_desc.Format = DXGI_FORMAT_R32_TYPELESS;
            view_desc.ViewDimension = D3D_SRV_DIMENSION_BUFFEREX;
            view_desc.BufferEx.FirstElement = 0U;
            assert(0U == ((sizeof(surface_vertex_varying_buffer_element) * vertex_count) % sizeof(uint32_t)));
            view_desc.BufferEx.NumElements = sizeof(surface_vertex_varying_buffer_element) * vertex_count / sizeof(uint32_t);
            view_desc.BufferEx.Flags = D3D11_BUFFEREX_SRV_FLAG_RAW;
            HRESULT res_device_create_view = device->CreateShaderResourceView(vertex_buffer_varying, &view_desc, &out_surfaces[mesh_id].m_vertex_buffer_varying);
            assert(SUCCEEDED(res_device_create_view));

            vertex_buffer_varying->Release();
        }

        {
            D3D11_BUFFER_DESC buffer_desc;
            buffer_desc.ByteWidth = sizeof(uint32_t) * index_count;
            buffer_desc.Usage = D3D11_USAGE_IMMUTABLE;
            buffer_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
            buffer_desc.CPUAccessFlags = 0U;
            buffer_desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
            buffer_desc.StructureByteStride = 0U;

            D3D11_SUBRESOURCE_DATA subresource_data;
            subresource_data.pSysMem = indices.data();
            subresource_data.SysMemPitch = sizeof(uint32_t);
            subresource_data.SysMemSlicePitch = sizeof(uint32_t);

            ID3D11Buffer *index_buffer = NULL;

            HRESULT res_device_create_buffer = device->CreateBuffer(&buffer_desc, &subresource_data, &index_buffer);
            assert(SUCCEEDED(res_device_create_buffer));

            D3D11_SHADER_RESOURCE_VIEW_DESC view_desc;
            view_desc.Format = DXGI_FORMAT_R32_TYPELESS;
            view_desc.ViewDimension = D3D_SRV_DIMENSION_BUFFEREX;
            view_desc.BufferEx.FirstElement = 0U;
            assert(0U == ((sizeof(uint32_t) * index_count) % sizeof(uint32_t)));
            view_desc.BufferEx.NumElements = sizeof(uint32_t) * index_count / sizeof(uint32_t);
            view_desc.BufferEx.Flags = D3D11_BUFFEREX_SRV_FLAG_RAW;
            HRESULT res_device_create_view = device->CreateShaderResourceView(index_buffer, &view_desc, &out_surfaces[mesh_id].m_index_buffer);
            assert(SUCCEEDED(res_device_create_view));

            index_buffer->Release();
        }

        assert(1.0 == normal_texture_scale);

        assert(!normal_texture_image_uri.empty());
        out_surfaces[mesh_id].m_normal_texture = internal_load_texture_from_file(device, surface_group_path, normal_texture_image_uri.c_str(), false);

        assert(emissive_texture_image_uri.empty());

        assert(!base_color_texture_image_uri.empty());
        out_surfaces[mesh_id].m_base_color_texture = internal_load_texture_from_file(device, surface_group_path, base_color_texture_image_uri.c_str(), true);

        assert(!metallic_roughness_texture_image_uri.empty());
        out_surfaces[mesh_id].m_roughness_metallic_texture = internal_load_texture_from_file(device, surface_group_path, metallic_roughness_texture_image_uri.c_str(), false);

        {
            surface_constant_buffer_binding constant;
            constant.base_color_factor = base_color_factor;
            constant.roughness_factor = roughness_factor;
            constant.metallic_factor = metallic_factor;

            D3D11_BUFFER_DESC buffer_desc;
            buffer_desc.ByteWidth = sizeof(surface_constant_buffer_binding);
            buffer_desc.Usage = D3D11_USAGE_IMMUTABLE;
            buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
            buffer_desc.CPUAccessFlags = 0U;
            buffer_desc.MiscFlags = 0U;
            buffer_desc.StructureByteStride = 0U;

            D3D11_SUBRESOURCE_DATA subresource_data;
            subresource_data.pSysMem = &constant;
            subresource_data.SysMemPitch = sizeof(surface_constant_buffer_binding);
            subresource_data.SysMemSlicePitch = sizeof(surface_constant_buffer_binding);

            HRESULT res_device_create_buffer = device->CreateBuffer(&buffer_desc, &subresource_data, &out_surfaces[mesh_id].m_factor);
            assert(SUCCEEDED(res_device_create_buffer));
        }
    }

    cgltf_free(data);

    return true;
}

ID3D11ShaderResourceView *internal_load_texture_from_file(ID3D11Device *device, char const *model_path, char const *image_path, bool force_srgb)
{
    std::vector<uint32_t> pixel_data;
    uint32_t pixel_data_width;
    uint32_t pixel_data_height;
    {
        std::vector<uint8_t> file_data;
        {
            wchar_t path_w[2 * MAX_PATH];
            {
                std::string path = model_path;
                {
                    size_t pos = path.find_last_of("\\/");
                    path = (pos > 0U) ? path.substr(0, pos) : path;
                    path += '/';
                    path += image_path;
                }
                assert(path.length() < (2 * MAX_PATH));

                int written = MultiByteToWideChar(CP_UTF8, 0U, path.c_str(), path.length(), path_w, 2 * MAX_PATH);
                assert(written > 0);
                assert(written < (2 * MAX_PATH));
                path_w[written] = L'\0';
            }

            HANDLE file = CreateFileW(path_w, FILE_GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
            assert(INVALID_HANDLE_VALUE != file);

            LARGE_INTEGER length;
            BOOL res_get_file_size_ex = GetFileSizeEx(file, &length);
            assert(FALSE != res_get_file_size_ex);

            assert(file_data.empty());
            file_data.resize(static_cast<int64_t>(length.QuadPart));

            DWORD read_size;
            BOOL res_read_file = ReadFile(file, file_data.data(), static_cast<DWORD>(length.QuadPart), &read_size, NULL);
            assert(FALSE != res_read_file);

            BOOL res_close_handle = CloseHandle(file);
            assert(FALSE != res_close_handle);
        }

        // pixel_data
        {
            static constexpr size_t const k_max_image_width_or_height = 16384U;
            static constexpr int const k_albedo_image_channel_size = sizeof(uint8_t);
            static constexpr int const k_albedo_image_num_channels = 4U;

            void const *const data_base = file_data.data();
            size_t const data_size = file_data.size();

            png_structp png_ptr = NULL;
            png_infop header_info_ptr = NULL;
            bool has_error = false;
            try
            {
                png_ptr = png_create_read_struct_2(PNG_LIBPNG_VER_STRING, NULL, _internal_libpng_error_callback, NULL, NULL, _internal_libpng_malloc_callback, _internal_libpng_free_ptr);

                if ((png_get_chunk_malloc_max(png_ptr) < data_size) && (data_size < (static_cast<uint32_t>(1U) << static_cast<uint32_t>(24U))))
                {
                    png_set_chunk_malloc_max(png_ptr, data_size);
                }

                _internal_libpng_read_data_context read_data_context = {data_base, data_size, 0};

                png_set_read_fn(png_ptr, &read_data_context, _internal_libpng_read_data_callback);

                header_info_ptr = png_create_info_struct(png_ptr);
                png_read_info(png_ptr, header_info_ptr);

                png_uint_32 width;
                png_uint_32 height;
                int bit_depth;
                int color_type;
                int interlaced;

                if (!png_get_IHDR(png_ptr, header_info_ptr, &width, &height, &bit_depth, &color_type, &interlaced, NULL, NULL))
                {
                    throw std::runtime_error("png get IHDR");
                }

                if (bit_depth > 8)
                {
                    assert(16 == bit_depth);

                    png_set_scale_16(png_ptr);
                }

                // https://github.com/pnggroup/libpng/blob/libpng16/libpng-manual.txt
                if (PNG_COLOR_TYPE_GRAY == color_type)
                {
                    // 01 -> 6A: CA
                    // 0  -> 6A: CA
                    // 0T -> 6A: C
                    // 0O -> 6A: C

                    png_set_gray_to_rgb(png_ptr);

                    if (!png_get_valid(png_ptr, header_info_ptr, PNG_INFO_tRNS))
                    {
                        png_set_add_alpha(png_ptr, 0XFFFFU, PNG_FILLER_AFTER);
                    }
                }
                else if (PNG_COLOR_TYPE_GRAY_ALPHA == color_type)
                {
                    // 4A -> 6A: C
                    // 4O -> 6O: C

                    png_set_gray_to_rgb(png_ptr);
                }
                else if (PNG_COLOR_TYPE_PALETTE == color_type)
                {
                    // 31 -> 6A: PA
                    // 3  -> 6A: PA
                    // 3T -> 6A: P
                    // 3O -> 6A: P

                    png_set_expand(png_ptr);
                    png_set_palette_to_rgb(png_ptr);

                    if (!png_get_valid(png_ptr, header_info_ptr, PNG_INFO_tRNS))
                    {
                        png_set_add_alpha(png_ptr, 0XFFFFU, PNG_FILLER_AFTER);
                    }
                }
                else if (PNG_COLOR_TYPE_RGB == color_type)
                {
                    // 2  -> 6A: A
                    // 2T -> 6A: T
                    // 2O -> 6O: T

                    if (!png_get_valid(png_ptr, header_info_ptr, PNG_INFO_tRNS))
                    {
                        png_set_add_alpha(png_ptr, 0XFFFFU, PNG_FILLER_AFTER);
                    }
                    else
                    {
                        png_set_tRNS_to_alpha(png_ptr);
                    }
                }

                {
                    double file_gamma = 1 / 2.2;
                    double screen_gamma = 2.2;
                    if (png_get_gAMA(png_ptr, header_info_ptr, &file_gamma))
                    {
                        png_set_gamma(png_ptr, screen_gamma, file_gamma);
                    }
                    else
                    {
                        png_set_gamma(png_ptr, screen_gamma, file_gamma);
                    }
                }

                int const num_passes = png_set_interlace_handling(png_ptr);

                // perform all transforms
                png_read_update_info(png_ptr, header_info_ptr);

                if (!png_get_IHDR(png_ptr, header_info_ptr, &width, &height, &bit_depth, &color_type, &interlaced, NULL, NULL))
                {
                    throw std::runtime_error("png_get_IHDR");
                }

                if ((width > k_max_image_width_or_height) || (height > k_max_image_width_or_height))
                {
                    throw std::runtime_error("Size Overflow");
                }

                if ((width < 1U) || (height < 1U))
                {
                    throw std::runtime_error("Size Zero");
                }

                if (!((PNG_COLOR_TYPE_RGB_ALPHA == color_type) && (k_albedo_image_num_channels == png_get_channels(png_ptr, header_info_ptr)) && ((8 * k_albedo_image_channel_size) == bit_depth)))
                {
                    throw std::runtime_error("NOT RGBA8 Format");
                }

                uint64_t const _uint64_stride = static_cast<uint64_t>(k_albedo_image_channel_size) * static_cast<uint64_t>(k_albedo_image_num_channels) * static_cast<uint64_t>(width);
                uint64_t const _uint64_num_pixels = static_cast<uint64_t>(width) * static_cast<uint64_t>(height);

                uintptr_t const stride = static_cast<uintptr_t>(_uint64_stride);
                size_t const num_pixels = static_cast<size_t>(_uint64_num_pixels);

                if (!((stride == _uint64_stride) && (num_pixels == _uint64_num_pixels)))
                {
                    throw std::runtime_error("Size Arguments Overflow");
                }

                pixel_data.resize(num_pixels);

                for (int pass_index = 0; pass_index < num_passes; ++pass_index)
                {
                    for (png_uint_32 height_index = 0U; height_index < height; ++height_index)
                    {
                        png_bytep row = reinterpret_cast<png_bytep>(reinterpret_cast<uintptr_t>(pixel_data.data()) + stride * height_index);
                        png_read_rows(png_ptr, &row, NULL, 1);
                    }
                }

                // we only need the header info
                // we do NOT need the end info
                // png_read_end(st, end_info);

                pixel_data_width = width;
                pixel_data_height = height;
            }
            catch (std::runtime_error exception)
            {
                std::cout << exception.what() << std::endl;

                has_error = true;
            }

            png_destroy_info_struct(png_ptr, &header_info_ptr);

            png_destroy_read_struct(&png_ptr, &header_info_ptr, NULL);

            assert(!has_error);
        }
    }

    ID3D11ShaderResourceView *texture_view = NULL;
    {
        D3D11_TEXTURE2D_DESC texture2d_desc;
        texture2d_desc.Width = pixel_data_width;
        texture2d_desc.Height = pixel_data_height;
        texture2d_desc.MipLevels = 1U;
        texture2d_desc.ArraySize = 1U;
        texture2d_desc.Format = force_srgb ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;
        texture2d_desc.SampleDesc.Count = 1U;
        texture2d_desc.SampleDesc.Quality = 0U;
        texture2d_desc.Usage = D3D11_USAGE_IMMUTABLE;
        texture2d_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        texture2d_desc.CPUAccessFlags = 0U;
        texture2d_desc.MiscFlags = 0U;

        D3D11_SUBRESOURCE_DATA subresource_data;
        subresource_data.pSysMem = pixel_data.data();
        subresource_data.SysMemPitch = sizeof(uint32_t) * pixel_data_width;
        subresource_data.SysMemSlicePitch = sizeof(uint32_t) * pixel_data_width * pixel_data_height;

        ID3D11Texture2D *texture = NULL;

        HRESULT res_device_create_texture2d = device->CreateTexture2D(&texture2d_desc, &subresource_data, &texture);
        assert(SUCCEEDED(res_device_create_texture2d));

        D3D11_SHADER_RESOURCE_VIEW_DESC shader_resource_view_desc;
        shader_resource_view_desc.Format = force_srgb ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;
        shader_resource_view_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        shader_resource_view_desc.Texture2D.MostDetailedMip = 0U;
        shader_resource_view_desc.Texture2D.MipLevels = 1U;

        HRESULT res_device_create_shader_resource_view = device->CreateShaderResourceView(texture, &shader_resource_view_desc, &texture_view);
        assert(SUCCEEDED(res_device_create_shader_resource_view));

        texture->Release();
    }

    return texture_view;
}

static cgltf_result _internal_cgltf_custom_read_file(const struct cgltf_memory_options *memory_options, const struct cgltf_file_options *file_options, const char *path, cgltf_size *size, void **data)
{
    void *(*const memory_alloc)(void *, cgltf_size) = memory_options->alloc_func;
    assert(NULL != memory_alloc);

    void (*const memory_free)(void *, void *) = memory_options->free_func;
    assert(NULL != memory_free);

    wchar_t path_w[2 * MAX_PATH];
    {
        size_t path_len = std::strlen(path);
        assert(path_len < (2 * MAX_PATH));

        int written = MultiByteToWideChar(CP_UTF8, 0U, path, path_len, path_w, 2 * MAX_PATH);
        assert(written > 0);
        assert(written < (2 * MAX_PATH));
        path_w[written] = L'\0';
    }

    HANDLE file = CreateFileW(path_w, FILE_GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    assert(INVALID_HANDLE_VALUE != file);

    if (INVALID_HANDLE_VALUE == file)
    {
        return cgltf_result_file_not_found;
    }

    cgltf_size file_size = (NULL != size) ? (*size) : 0;
    if (file_size == 0)
    {
        LARGE_INTEGER length;
        BOOL res_get_file_size_ex = GetFileSizeEx(file, &length);
        if (FALSE == res_get_file_size_ex)
        {
            BOOL res_close_handle = CloseHandle(file);
            assert(FALSE != res_close_handle);
            return cgltf_result_io_error;
        }

        file_size = static_cast<int64_t>(length.QuadPart);
    }

    void *file_data = memory_alloc(memory_options->user_data, file_size);
    if (NULL == file_data)
    {
        BOOL res_close_handle = CloseHandle(file);
        assert(FALSE != res_close_handle);
        return cgltf_result_out_of_memory;
    }

    {
        DWORD read_size;
        BOOL res_read_file = ReadFile(file, file_data, file_size, &read_size, NULL);
        assert(FALSE != res_read_file);

        BOOL res_close_handle = CloseHandle(file);
        assert(FALSE != res_close_handle);

        if (FALSE == res_read_file || read_size != file_size)
        {
            memory_free(memory_options->user_data, file_data);
            return cgltf_result_io_error;
        }
    }

    if (NULL != size)
    {
        *size = file_size;
    }
    if (NULL != data)
    {
        *data = file_data;
    }

    return cgltf_result_success;
}

static void _internal_cgltf_custom_file_release(const struct cgltf_memory_options *memory_options, const struct cgltf_file_options *, void *data, cgltf_size size)
{
    void (*const memory_free)(void *, void *) = memory_options->free_func;
    assert(NULL != memory_free);

    memory_free(memory_options->user_data, data);
}

static void *_internal_cgltf_custom_alloc(void *, cgltf_size size)
{
    return _aligned_malloc(size, 16U);
}

static void _internal_cgltf_custom_free(void *, void *ptr)
{
    return _aligned_free(ptr);
}

static void PNGCBAPI _internal_libpng_error_callback(png_structp, png_const_charp error_message)
{
    // TODO: is it safe to throw exception crossing the boundary of different DLLs?
    // compile libpng into static library?
    throw std::runtime_error(error_message);
}

static png_voidp PNGCBAPI _internal_libpng_malloc_callback(png_structp, png_alloc_size_t size)
{
    return _aligned_malloc(size, 16U);
}

static void PNGCBAPI _internal_libpng_free_ptr(png_structp, png_voidp ptr)
{
    return _aligned_free(ptr);
}

static void PNGCBAPI _internal_libpng_read_data_callback(png_structp png_ptr, png_bytep data, size_t length)
{
    // pngrio.c: png_default_read_data

    if (png_ptr != NULL)
    {
        _internal_libpng_read_data_context *const read_data_context = static_cast<_internal_libpng_read_data_context *>(png_get_io_ptr(png_ptr));

        if ((read_data_context->m_offset + length) <= read_data_context->m_data_size)
        {
            std::memcpy(data, reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(read_data_context->m_data_base) + read_data_context->m_offset), length);
            read_data_context->m_offset += length;
        }
        else
        {
            throw std::runtime_error("Read Data Overflow");
        }
    }
}
