#include <hooking.hpp>
#include <render.hpp>
#include <pointers.hpp>

using namespace uknowncheats;

void(*execute_command_lists_o)(ID3D12CommandQueue*, UINT, ID3D12CommandList*);
void hk_execute_command_lists(ID3D12CommandQueue* queue, UINT NumCommandLists, ID3D12CommandList* ppCommandLists) {
	if (!g_render.d3d12CommandQueue)
		g_render.d3d12CommandQueue = queue;

	execute_command_lists_o(queue, NumCommandLists, ppCommandLists);
}


HRESULT intercept_window_render(IDXGISwapChain3* swapchain_ptr, const UINT sync_interval, UINT flags) {
	return g_render.render_window(swapchain_ptr, sync_interval, flags);
}

void hooking::hook_present() {
	MH_Initialize();

	MH_CreateHook((void*)g_render.present_func, &intercept_window_render, reinterpret_cast<void**>(&g_render.original_present));
	MH_CreateHook((void*)g_render.execute_command_list_func, &hk_execute_command_lists, reinterpret_cast<void**>(&execute_command_lists_o));
	//MH_CreateHook((void*)g_render.swapchain_vtable[8], &intercept_window_render, reinterpret_cast<void**>(&g_render.original_present));
	MH_EnableHook(MH_ALL_HOOKS);
}