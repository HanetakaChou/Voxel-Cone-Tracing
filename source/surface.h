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

#ifndef _SURFACE_H_
#define _SURFACE_H_ 1

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif
#ifndef NOMINMAX
#define NOMINMAX 1
#endif
#include <sdkddkver.h>
#include <windows.h>
#include <d3d11.h>
#include <cstdint>
#include <vector>

struct surface
{
	ID3D11ShaderResourceView *m_vertex_buffer_position;
	ID3D11ShaderResourceView *m_vertex_buffer_varying;
	ID3D11ShaderResourceView *m_index_buffer;
	ID3D11ShaderResourceView *m_normal_texture;
	ID3D11ShaderResourceView *m_base_color_texture;
	ID3D11ShaderResourceView *m_roughness_metallic_texture;
	ID3D11Buffer *m_factor;
	uint32_t m_index_count;
};

bool load_surfaces(ID3D11Device *device, std::vector<surface> &out_surfaces);

#endif
