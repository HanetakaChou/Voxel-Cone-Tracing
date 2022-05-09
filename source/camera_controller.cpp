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

#include "camera_controller.h"

DirectX::XMFLOAT3 g_camera_eye_position = DirectX::XMFLOAT3(0.0F, 0.0F, 0.0F);
DirectX::XMFLOAT3 g_camera_eye_direction = DirectX::XMFLOAT3(0.0F, 0.0F, 0.0F);
DirectX::XMFLOAT3 g_camera_up_direction = DirectX::XMFLOAT3(0.0F, 0.0F, 0.0F);
