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

#define NOMINMAX 1
#define WIN32_LEAN_AND_MEAN 1
#include <sdkddkver.h>
#include <Windows.h>
#include <windowsx.h>
#include <cstdint>
#include <cassert>
#include <dxgi.h>
#include <d3d11_1.h>
#include "camera_controller.h"
#include "resolution.h"
#include "demo.h"
#include "../thirdparty/DXUT/Optional/DXUTcamera.h"

static bool g_window_running = true;

static CDXUTFirstPersonCamera g_first_person_camera;

static int32_t g_window_width = g_resolution_width;
static int32_t g_window_height = g_resolution_height;

static LRESULT CALLBACK wnd_proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

static inline uint64_t internal_tick_count_per_second();

static inline uint64_t internal_tick_count_now();

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
	ATOM window_class;
	{
		WNDCLASSEXW const window_class_create_info = {
			sizeof(WNDCLASSEX),
			CS_OWNDC,
			wnd_proc,
			0,
			sizeof(LONG_PTR),
			hInstance,
			LoadIconW(NULL, IDI_APPLICATION),
			LoadCursorW(NULL, IDC_ARROW),
			(HBRUSH)(COLOR_WINDOW + 1),
			NULL,
			L"Voxel-Cone-Tracing:0XFFFFFFFF",
			LoadIconW(NULL, IDI_APPLICATION),
		};
		window_class = RegisterClassExW(&window_class_create_info);
		assert(0 != window_class);
	}

	HWND window;
	{
		HWND const desktop_window = GetDesktopWindow();

		constexpr DWORD const dw_style = WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
		constexpr DWORD const dw_ex_style = WS_EX_APPWINDOW;

		RECT rect;
		{
			HMONITOR const monitor = MonitorFromWindow(desktop_window, MONITOR_DEFAULTTONEAREST);

			MONITORINFOEXW monitor_info;
			monitor_info.cbSize = sizeof(MONITORINFOEXW);
			BOOL res_get_monitor_info = GetMonitorInfoW(monitor, &monitor_info);
			assert(FALSE != res_get_monitor_info);

			rect = RECT{(monitor_info.rcWork.left + monitor_info.rcWork.right) / 2 - g_window_width / 2,
						(monitor_info.rcWork.bottom + monitor_info.rcWork.top) / 2 - g_window_height / 2,
						(monitor_info.rcWork.left + monitor_info.rcWork.right) / 2 + g_window_width / 2,
						(monitor_info.rcWork.bottom + monitor_info.rcWork.top) / 2 + g_window_height / 2};

			BOOL const res_adjust_window_rest = AdjustWindowRectEx(&rect, dw_style, FALSE, dw_ex_style);
			assert(FALSE != res_adjust_window_rest);
		}

		window = CreateWindowExW(dw_ex_style, MAKEINTATOM(window_class), L"Voxel Cone Tracing", dw_style, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, desktop_window, NULL, hInstance, NULL);
		assert(NULL != window);
	}

	ID3D11Device *d3d11_device = NULL;
	ID3D11DeviceContext *d3d11_device_context = NULL;
	ID3DUserDefinedAnnotation *d3d11_user_defined_annotation = NULL;
	IDXGISwapChain *dxgi_swap_chain = NULL;
	Demo demo;
	{

		{
			IDXGIFactory1 *dxgi_factory = NULL;
			HRESULT res_create_dxgi_factory = CreateDXGIFactory1(IID_PPV_ARGS(&dxgi_factory));
			assert(SUCCEEDED(res_create_dxgi_factory));

			IDXGIAdapter1 *dxgi_adapter = NULL;
			// "0U" means "default" adapter // perhaps user-select
			HRESULT res_dxgi_factory_enum_adapters = dxgi_factory->EnumAdapters1(0U, &dxgi_adapter);
			assert(SUCCEEDED(res_dxgi_factory_enum_adapters));

			D3D_FEATURE_LEVEL pFeatureLevels[1] = {D3D_FEATURE_LEVEL_11_0};
			HRESULT res_d3d_create_device = D3D11CreateDevice(
				dxgi_adapter,
				D3D_DRIVER_TYPE_UNKNOWN,
				NULL,
#ifndef NDEBUG
				D3D11_CREATE_DEVICE_SINGLETHREADED | D3D11_CREATE_DEVICE_DEBUG,
#else
				D3D11_CREATE_DEVICE_SINGLETHREADED,
#endif
				pFeatureLevels,
				sizeof(pFeatureLevels) / sizeof(pFeatureLevels[0]),
				D3D11_SDK_VERSION,
				&d3d11_device,
				NULL,
				&d3d11_device_context);
			assert(SUCCEEDED(res_d3d_create_device));

			HRESULT res_d3d11_device_context_query_interface = d3d11_device_context->QueryInterface(IID_PPV_ARGS(&d3d11_user_defined_annotation));
			assert(SUCCEEDED(res_d3d11_device_context_query_interface));

			DXGI_SWAP_CHAIN_DESC dxgi_swap_chain_desc;
			dxgi_swap_chain_desc.BufferDesc.Width = g_resolution_width;
			dxgi_swap_chain_desc.BufferDesc.Height = g_resolution_height;
			dxgi_swap_chain_desc.BufferDesc.RefreshRate.Numerator = 0U;
			dxgi_swap_chain_desc.BufferDesc.RefreshRate.Denominator = 0U;
			dxgi_swap_chain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			dxgi_swap_chain_desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
			dxgi_swap_chain_desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
			dxgi_swap_chain_desc.SampleDesc.Count = 1U;
			dxgi_swap_chain_desc.SampleDesc.Quality = 0U;
			dxgi_swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			dxgi_swap_chain_desc.BufferCount = 1U;
			dxgi_swap_chain_desc.OutputWindow = window;
			dxgi_swap_chain_desc.Windowed = TRUE;
			dxgi_swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
			dxgi_swap_chain_desc.Flags = 0U;
			HRESULT res_dxgi_factory_create_swap_chain = dxgi_factory->CreateSwapChain(d3d11_device, &dxgi_swap_chain_desc, &dxgi_swap_chain);
			assert(SUCCEEDED(res_dxgi_factory_create_swap_chain));

			dxgi_adapter->Release();
			dxgi_factory->Release();
		}
	}

	demo.Init(d3d11_device, d3d11_device_context, dxgi_swap_chain);

	g_first_person_camera.SetRotationScaler(DirectX::XM_PIDIV2);
	g_first_person_camera.SetMoveScaler(2.5F);

	g_first_person_camera.SetEyePt(g_camera_eye_position);
	g_first_person_camera.SetEyeDir(g_camera_eye_direction);
	g_first_person_camera.SetUpDir(g_camera_up_direction);

	ShowWindow(window, SW_SHOWDEFAULT);
	{
		BOOL result_update_window = UpdateWindow(window);
		assert(FALSE != result_update_window);
	}

	double tick_count_resolution = 1.0 / static_cast<double>(internal_tick_count_per_second());

	uint64_t tick_count_previous_frame = internal_tick_count_now();

	while (g_window_running)
	{
		MSG msg;
		while (::PeekMessageW(&msg, NULL, 0U, 0U, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}

		if (g_window_running && g_window_width > 0 && g_window_height > 0)
		{
			float interval_time;
			{
				uint64_t const tick_count_current_frame = internal_tick_count_now();
				interval_time = static_cast<float>(static_cast<double>(tick_count_current_frame - tick_count_previous_frame) * tick_count_resolution);
				tick_count_previous_frame = tick_count_current_frame;
			}

			{
				g_first_person_camera.FrameMove(interval_time);
				DirectX::XMStoreFloat3(&g_camera_eye_position, g_first_person_camera.GetEyePt());
				DirectX::XMStoreFloat3(&g_camera_eye_direction, g_first_person_camera.GetEyeDir());
				DirectX::XMStoreFloat3(&g_camera_up_direction, g_first_person_camera.GetUpDir());
			}

			demo.Tick(d3d11_device_context, d3d11_user_defined_annotation, dxgi_swap_chain);
		}
	}

	demo.UnInit();

	dxgi_swap_chain->Release();
	d3d11_user_defined_annotation->Release();
	d3d11_device_context->Release();
	d3d11_device->Release();

	DestroyWindow(window);

	UnregisterClassW(MAKEINTATOM(window_class), hInstance);

	return 0;
}

static LRESULT CALLBACK wnd_proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_SIZE:
	{
		WORD const new_width = LOWORD(lParam);
		WORD const new_height = HIWORD(lParam);

		if (g_window_width != new_width || g_window_height != new_height)
		{
			g_window_width = new_width;
			g_window_height = new_height;

			assert(g_resolution_width == g_window_width || 0 == g_window_width);
			assert(g_resolution_height == g_window_height || 0 == g_window_height);
		}
	}
		return 0;
	case WM_ERASEBKGND:
		return 1;
	case WM_KEYDOWN:
	{
		D3DUtil_CameraKeys mappedKey;
		switch (wParam)
		{
		case 'W':
		{
			mappedKey = CAM_MOVE_FORWARD;
		}
		break;
		case 'S':
		{
			mappedKey = CAM_MOVE_BACKWARD;
		}
		break;
		case 'A':
		{
			mappedKey = CAM_STRAFE_LEFT;
		}
		break;
		case 'D':
		{
			mappedKey = CAM_STRAFE_RIGHT;
		}
		break;
		case 'Q':
		{
			mappedKey = CAM_MOVE_UP;
		}
		break;
		case 'E':
		{
			mappedKey = CAM_MOVE_DOWN;
		}
		break;
		default:
		{
			mappedKey = CAM_UNKNOWN;
		}
		};

		g_first_person_camera.HandleKeyDownMessage(mappedKey);
	}
		return 0;
	case WM_KEYUP:
	{
		D3DUtil_CameraKeys mappedKey;
		switch (wParam)
		{
		case 'W':
		{
			mappedKey = CAM_MOVE_FORWARD;
		}
		break;
		case 'S':
		{
			mappedKey = CAM_MOVE_BACKWARD;
		}
		break;
		case 'A':
		{
			mappedKey = CAM_STRAFE_LEFT;
		}
		break;
		case 'D':
		{
			mappedKey = CAM_STRAFE_RIGHT;
		}
		break;
		case 'Q':
		{
			mappedKey = CAM_MOVE_UP;
		}
		break;
		case 'E':
		{
			mappedKey = CAM_MOVE_DOWN;
		}
		break;
		default:
		{
			mappedKey = CAM_UNKNOWN;
		}
		};

		g_first_person_camera.HandleKeyUpMessage(mappedKey);
	}
		return 0;
	case WM_MOUSEMOVE:
	{

		int window_x = GET_X_LPARAM(lParam);
		int window_y = GET_Y_LPARAM(lParam);

		float normalized_x = static_cast<float>(static_cast<double>(window_x) / static_cast<double>(g_window_width));
		float normalizedY = static_cast<float>(static_cast<double>(window_y) / static_cast<double>(g_window_height));

		bool leftButton = (0U != (wParam & MK_LBUTTON));
		bool middleButton = (0U != (wParam & MK_MBUTTON));
		bool rightButton = (0U != (wParam & MK_RBUTTON));

		g_first_person_camera.HandleMouseMoveMessage(normalized_x, normalizedY, leftButton, middleButton, rightButton);
	}
		return 0;
	case WM_DESTROY:
	{
		g_window_running = false;
	}
		return 0;
	default:
		return DefWindowProcW(hWnd, message, wParam, lParam);
	}
}

static inline uint64_t internal_tick_count_per_second()
{
	LARGE_INTEGER int64_frequency;
	BOOL result_query_performance_frequency = QueryPerformanceFrequency(&int64_frequency);
	assert(NULL != result_query_performance_frequency);

	uint64_t const tick_count_per_second = static_cast<uint64_t>(int64_frequency.QuadPart);
	return tick_count_per_second;
}

static inline uint64_t internal_tick_count_now()
{
	LARGE_INTEGER int64_performance_count;
	BOOL result_query_performance_counter = QueryPerformanceCounter(&int64_performance_count);
	assert(NULL != result_query_performance_counter);

	uint64_t const tick_count_now = static_cast<uint64_t>(int64_performance_count.QuadPart);
	return tick_count_now;
}
