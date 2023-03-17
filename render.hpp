#pragma once
#include <common.h>
#include <d3d12.h>
#include <dxgi1_4.h>

namespace uknowncheats
{
	class render
	{
	public:
		explicit render() = default;
		~render() = default;

		bool initialize();
		HRESULT render_window(IDXGISwapChain3*, const UINT, UINT);

		void* present_func;
		void* execute_command_list_func;
		uint64_t* swapchain_vtable = NULL;
		DWORD_PTR* context_vtable = NULL;
		DWORD_PTR* device_vtable = NULL;
		WNDPROC m_wnd_proc = NULL;
		ID3D12CommandQueue* d3d12CommandQueue = nullptr;

		typedef HRESULT(__stdcall* present_t)(IDXGISwapChain* dx_swapchain, UINT sync_interval, UINT flags);
		present_t original_present = NULL;

	private:
		void init_imgui();

		bool m_rendering_initialized = false;

		ID3D12Device* m_device = NULL;
		HWND m_window = NULL;
	};

	inline render g_render;
}
