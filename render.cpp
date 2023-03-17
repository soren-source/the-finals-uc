#include <render.hpp>
#include <menu.hpp>

#include <custom.hpp>
#include <font.hpp>
#include <tahoma.hpp>

#include <imgui_impl_win32.h>
#include <imgui_impl_dx12.h>
#include <imgui.h>
#include <dxgitype.h>
#include <dxgi.h>
#include <MinHook.h>
#include <esp.hpp>
#include <aimbot.hpp>

using namespace uknowncheats;

struct FrameContext {
	ID3D12CommandAllocator* commandAllocator = nullptr;
	ID3D12Resource* main_render_target_resource = nullptr;
	D3D12_CPU_DESCRIPTOR_HANDLE main_render_target_descriptor;
};

int32_t buffersCounts = -1;
FrameContext* frameContext;
ID3D12Device* d3d12Device = nullptr;
ID3D12DescriptorHeap* d3d12DescriptorHeapBackBuffers = nullptr;
ID3D12DescriptorHeap* d3d12DescriptorHeapImGuiRender = nullptr;
ID3D12GraphicsCommandList* d3d12CommandList = nullptr;
ID3D12Fence* d3d12Fence = nullptr;
UINT64 d3d12FenceValue = 0;

typedef long(__fastcall* PresentD3D12) (IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
extern PresentD3D12 oPresentD3D12;

PresentD3D12 oPresentD3D12;
HRESULT(*oSignalD3D12)(ID3D12CommandQueue*, ID3D12Fence*, UINT64);

struct __declspec(uuid("189819f1-1db6-4b57-be54-1821339b85f7")) ID3D12Device;
static uint64_t* g_methodsTable = NULL;

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT __stdcall wnd_proc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
        return true;

    return CallWindowProc(g_render.m_wnd_proc, hWnd, uMsg, wParam, lParam);
}


void render::init_imgui()
{
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags = ImGuiConfigFlags_NoMouseCursorChange;
	io.IniFilename = NULL;
	ImGui_ImplWin32_Init(m_window);
	ImGui_ImplDX12_Init(m_device, buffersCounts,
		DXGI_FORMAT_R8G8B8A8_UNORM, d3d12DescriptorHeapImGuiRender,
		d3d12DescriptorHeapImGuiRender->GetCPUDescriptorHandleForHeapStart(),
		d3d12DescriptorHeapImGuiRender->GetGPUDescriptorHandleForHeapStart());

	ImGuiStyle* style = &ImGui::GetStyle();
	style->GrabRounding = 5.f;
	style->GrabMinSize = 5.f;
	style->ScrollbarSize = 5.f;

	font_menu = io.Fonts->AddFontFromMemoryTTF((void*)font_binary, sizeof(font_binary), 12.f);
	icons = io.Fonts->AddFontFromMemoryTTF((void*)icons_binary, sizeof(icons_binary), 12.f);
	icons2 = io.Fonts->AddFontFromMemoryTTF((void*)icons_binary, sizeof(icons_binary), 16.f);
	tahoma = io.Fonts->AddFontFromMemoryTTF((void*)tahoma_binary, sizeof(tahoma_binary), 12.f);
}
HRESULT render::render_window(IDXGISwapChain3* dx_swapchain, const UINT sync_interval, UINT flags) {
	if (!m_rendering_initialized)
	{
		if (SUCCEEDED(dx_swapchain->GetDevice(__uuidof(ID3D12Device), (void**)&d3d12Device)))
		{
			CreateEvent(nullptr, false, false, nullptr);

			this->m_window = FindWindowA("UnrealWindow", 0);

			DXGI_SWAP_CHAIN_DESC sdesc;
			dx_swapchain->GetDesc(&sdesc);
			sdesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
			sdesc.OutputWindow = this->m_window;
			sdesc.Windowed = ((GetWindowLongPtr(this->m_window, GWL_STYLE) & WS_POPUP) != 0) ? false : true;

			buffersCounts = sdesc.BufferCount;
			frameContext = new FrameContext[buffersCounts];

			D3D12_DESCRIPTOR_HEAP_DESC descriptorImGuiRender = {};
			descriptorImGuiRender.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			descriptorImGuiRender.NumDescriptors = buffersCounts;
			descriptorImGuiRender.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

			if (d3d12Device->CreateDescriptorHeap(&descriptorImGuiRender, IID_PPV_ARGS(&d3d12DescriptorHeapImGuiRender)) != S_OK)
				return false;

			ID3D12CommandAllocator* allocator;
			if (d3d12Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocator)) != S_OK)
				return false;

			for (size_t i = 0; i < buffersCounts; i++) {
				frameContext[i].commandAllocator = allocator;
			}

			if (d3d12Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator, NULL, IID_PPV_ARGS(&d3d12CommandList)) != S_OK ||
				d3d12CommandList->Close() != S_OK)
				return false;

			D3D12_DESCRIPTOR_HEAP_DESC descriptorBackBuffers;
			descriptorBackBuffers.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
			descriptorBackBuffers.NumDescriptors = buffersCounts;
			descriptorBackBuffers.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			descriptorBackBuffers.NodeMask = 1;

			if (d3d12Device->CreateDescriptorHeap(&descriptorBackBuffers, IID_PPV_ARGS(&d3d12DescriptorHeapBackBuffers)) != S_OK)
				return false;

			const auto rtvDescriptorSize = d3d12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
			D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = d3d12DescriptorHeapBackBuffers->GetCPUDescriptorHandleForHeapStart();

			for (size_t i = 0; i < buffersCounts; i++) {
				ID3D12Resource* pBackBuffer = nullptr;

				frameContext[i].main_render_target_descriptor = rtvHandle;
				dx_swapchain->GetBuffer(i, IID_PPV_ARGS(&pBackBuffer));
				d3d12Device->CreateRenderTargetView(pBackBuffer, nullptr, rtvHandle);
				frameContext[i].main_render_target_resource = pBackBuffer;
				rtvHandle.ptr += rtvDescriptorSize;
			}

			ImGui::CreateContext();
			ImGuiIO& io = ImGui::GetIO();
			io.ConfigFlags = ImGuiConfigFlags_NoMouseCursorChange;
			io.IniFilename = NULL;
			ImGui_ImplWin32_Init(this->m_window);
			ImGui_ImplDX12_Init(d3d12Device, buffersCounts,
				DXGI_FORMAT_R8G8B8A8_UNORM, d3d12DescriptorHeapImGuiRender,
				d3d12DescriptorHeapImGuiRender->GetCPUDescriptorHandleForHeapStart(),
				d3d12DescriptorHeapImGuiRender->GetGPUDescriptorHandleForHeapStart());

			this->m_wnd_proc = (WNDPROC)SetWindowLongPtr(sdesc.OutputWindow, GWLP_WNDPROC, (__int3264)(LONG_PTR)wnd_proc);

			ImGuiStyle* style = &ImGui::GetStyle();
			style->GrabRounding = 5.f;
			style->GrabMinSize = 5.f;
			style->ScrollbarSize = 5.f;

			font_menu = io.Fonts->AddFontFromMemoryTTF((void*)font_binary, sizeof(font_binary), 12.f);
			icons = io.Fonts->AddFontFromMemoryTTF((void*)icons_binary, sizeof(icons_binary), 12.f);
			icons2 = io.Fonts->AddFontFromMemoryTTF((void*)icons_binary, sizeof(icons_binary), 16.f);
			tahoma = io.Fonts->AddFontFromMemoryTTF((void*)tahoma_binary, sizeof(tahoma_binary), 12.f);

			ImGui_ImplDX12_CreateDeviceObjects();
			m_rendering_initialized = true;
		}
		else
		{
			printf("error");
		}
	}

	if (this->d3d12CommandQueue == nullptr)
		return this->original_present(dx_swapchain, sync_interval, flags);

	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	g_esp.render();

	FrameContext& currentFrameContext = frameContext[dx_swapchain->GetCurrentBackBufferIndex()];
	currentFrameContext.commandAllocator->Reset();

	D3D12_RESOURCE_BARRIER barrier;
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = currentFrameContext.main_render_target_resource;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

	d3d12CommandList->Reset(currentFrameContext.commandAllocator, nullptr);
	d3d12CommandList->ResourceBarrier(1, &barrier);
	d3d12CommandList->OMSetRenderTargets(1, &currentFrameContext.main_render_target_descriptor, FALSE, nullptr);
	d3d12CommandList->SetDescriptorHeaps(1, &d3d12DescriptorHeapImGuiRender);

	ImGui::Render();
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), d3d12CommandList);

	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;

	d3d12CommandList->ResourceBarrier(1, &barrier);
	d3d12CommandList->Close();

	this->d3d12CommandQueue->ExecuteCommandLists(1, reinterpret_cast<ID3D12CommandList* const*>(&d3d12CommandList));

	return this->original_present(dx_swapchain, sync_interval, flags);
}


bool render::initialize() {
    WNDCLASSEX windowClass;
    windowClass.cbSize = sizeof(WNDCLASSEX);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = DefWindowProc;
    windowClass.cbClsExtra = 0;
    windowClass.cbWndExtra = 0;
    windowClass.hInstance = GetModuleHandle(NULL);
    windowClass.hIcon = NULL;
    windowClass.hCursor = NULL;
    windowClass.hbrBackground = NULL;
    windowClass.lpszMenuName = NULL;
    windowClass.lpszClassName = L"UC";
    windowClass.hIconSm = NULL;

    ::RegisterClassEx(&windowClass);

    HWND window = ::CreateWindow(windowClass.lpszClassName, L"UC", WS_OVERLAPPEDWINDOW, 0, 0, 100, 100, NULL, NULL, windowClass.hInstance, NULL);
	this->m_window = window;

	HMODULE libDXGI;
	HMODULE libD3D12;
	if ((libDXGI = ::GetModuleHandleA(("dxgi.dll"))) == NULL || (libD3D12 = ::GetModuleHandleA(("d3d12.dll"))) == NULL)
	{
		::DestroyWindow(window);
		::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
		return 0;
	}

	void* CreateDXGIFactory;
	if ((CreateDXGIFactory = ::GetProcAddress(libDXGI, "CreateDXGIFactory")) == NULL)
	{
		::DestroyWindow(window);
		::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
		return 0;
	}

	IDXGIFactory* factory;
	if (((long(__stdcall*)(const IID&, void**))(CreateDXGIFactory))(__uuidof(IDXGIFactory), (void**)&factory) < 0)
	{
		::DestroyWindow(window);
		::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
		return 0;
	}

	IDXGIAdapter* adapter;
	if (factory->EnumAdapters(0, &adapter) == DXGI_ERROR_NOT_FOUND)
	{
		::DestroyWindow(window);
		::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
		return 0;
	}

	void* D3D12CreateDevice;
	if ((D3D12CreateDevice = ::GetProcAddress(libD3D12, "D3D12CreateDevice")) == NULL)
	{
		::DestroyWindow(window);
		::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
		return 0;
	}

	ID3D12Device* device;
	if (((long(__stdcall*)(IUnknown*, D3D_FEATURE_LEVEL, const IID&, void**))(D3D12CreateDevice))(adapter, D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), (void**)&device) < 0)
	{
		::DestroyWindow(window);
		::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
		return 0;
	}

	D3D12_COMMAND_QUEUE_DESC queueDesc;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Priority = 0;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.NodeMask = 0;

	ID3D12CommandQueue* commandQueue;
	if (device->CreateCommandQueue(&queueDesc, __uuidof(ID3D12CommandQueue), (void**)&commandQueue) < 0)
	{
		::DestroyWindow(window);
		::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
		return 0;
	}

	ID3D12CommandAllocator* commandAllocator;
	if (device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, __uuidof(ID3D12CommandAllocator), (void**)&commandAllocator) < 0)
	{
		::DestroyWindow(window);
		::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
		return 0;
	}

	ID3D12GraphicsCommandList* commandList;
	if (device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator, NULL, __uuidof(ID3D12GraphicsCommandList), (void**)&commandList) < 0)
	{
		::DestroyWindow(window);
		::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
		return 0;
	}

	DXGI_RATIONAL refreshRate;
	refreshRate.Numerator = 60;
	refreshRate.Denominator = 1;

	DXGI_MODE_DESC bufferDesc;
	bufferDesc.Width = 100;
	bufferDesc.Height = 100;
	bufferDesc.RefreshRate = refreshRate;
	bufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	bufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	bufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

	DXGI_SAMPLE_DESC sampleDesc;
	sampleDesc.Count = 1;
	sampleDesc.Quality = 0;

	DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
	swapChainDesc.BufferDesc = bufferDesc;
	swapChainDesc.SampleDesc = sampleDesc;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = 2;
	swapChainDesc.OutputWindow = window;
	swapChainDesc.Windowed = 1;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	IDXGISwapChain* swapChain;
	if (factory->CreateSwapChain(commandQueue, &swapChainDesc, &swapChain) < 0)
	{
		::DestroyWindow(window);
		::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
		return 0;
	}

	g_methodsTable = (uint64_t*)::calloc(150, sizeof(uint64_t));
	::memcpy(g_methodsTable, *(uint64_t**)device, 44 * sizeof(uint64_t));
	::memcpy(g_methodsTable + 44, *(uint64_t**)commandQueue, 19 * sizeof(uint64_t));
	::memcpy(g_methodsTable + 44 + 19, *(uint64_t**)commandAllocator, 9 * sizeof(uint64_t));
	::memcpy(g_methodsTable + 44 + 19 + 9, *(uint64_t**)commandList, 60 * sizeof(uint64_t));
	::memcpy(g_methodsTable + 44 + 19 + 9 + 60, *(uint64_t**)swapChain, 18 * sizeof(uint64_t));

	this->present_func = (void*)g_methodsTable[140];
	this->execute_command_list_func = (void*)g_methodsTable[54];

	device->Release();
	device = NULL;

	commandQueue->Release();
	commandQueue = NULL;

	commandAllocator->Release();
	commandAllocator = NULL;

	commandList->Release();
	commandList = NULL;

	swapChain->Release();
	swapChain = NULL;

	::DestroyWindow(window);
	::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);

	return true;
}