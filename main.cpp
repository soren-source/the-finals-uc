#include <common.h>
#include <main.hpp>
#include <hooking.hpp>
#include <render.hpp>
#include <thread>
#include <MinHook.h>
#include <pointers.hpp>

#include <ue4.h>

using namespace uknowncheats;

DWORD WINAPI initialize(LPVOID lpParam) {

    std::cout << "[+] Initializing Render..." << std::endl;    
    if (!g_render.initialize()) return EXIT_SUCCESS;

    std::cout << "[+] Initializing Unreal Engine 5 SDK..." << std::endl;
    //if (!init_sdk()) return EXIT_SUCCESS;

    std::cout << "[+] Initializing Hooking..." << std::endl;
    g_hooking.hook_present();


    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        if (GetAsyncKeyState(VK_END)) {
            fclose((FILE*)stdout); 
            FreeConsole();
            MH_RemoveHook(MH_ALL_HOOKS);
            MH_Uninitialize();
            FreeLibraryAndExitThread(g_main.module, 0);
        }
    }

    return EXIT_SUCCESS;
}

DWORD get_module_size(DWORD64 base)
{
    IMAGE_DOS_HEADER dos_header = { 0 };
    IMAGE_NT_HEADERS nt_headers = { 0 };
    if (!base)return -1;
    dos_header = *(IMAGE_DOS_HEADER*)base;
    nt_headers = *(IMAGE_NT_HEADERS*)(base + dos_header.e_lfanew);
    return nt_headers.OptionalHeader.SizeOfImage;
}

typedef struct
{
    DWORD64 dwEP;
    void* pParam;
}CALL_MYFUNCTION, * PCALL_MYFUNCTION;
typedef DWORD(*_Function)(VOID* p);

void WINAPI my_fn_thread(PCALL_MYFUNCTION pCMF)
{
    if (pCMF != NULL && pCMF->dwEP != NULL)
    {
        _Function Function = (_Function)pCMF->dwEP;
        Function(pCMF->pParam);
    }
}

HANDLE my_create_thread(LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter, LPDWORD lpThreadId)
{
    HMODULE hNtDll = GetModuleHandleA("ntdll.dll");
    if (hNtDll != NULL)
    {
        DWORD dwImageSize = get_module_size((DWORD64)hNtDll);
        BYTE* pMemoryData = (BYTE*)hNtDll + dwImageSize - 0x400;

        if (pMemoryData != NULL)
        {
            DWORD dwProtect;
            VirtualProtect(pMemoryData, 0x100, PAGE_EXECUTE_READWRITE, &dwProtect);
            CALL_MYFUNCTION* pCMF = (CALL_MYFUNCTION*)VirtualAlloc(NULL, 0x100, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
            pCMF->dwEP = (DWORD64)(lpStartAddress);
            pCMF->pParam = lpParameter;
            memcpy((LPVOID)pMemoryData, (LPVOID)my_fn_thread, 0x100);
            HANDLE hHandle = CreateRemoteThread(GetCurrentProcess(), NULL, 0, (LPTHREAD_START_ROUTINE)pMemoryData, pCMF, NULL, lpThreadId);
            return hHandle;
        }
    }
    return 0;
}

BOOL APIENTRY DllMain(HMODULE h_module, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(h_module);
        g_main.module = h_module;
        g_main.handle = my_create_thread(&initialize, 0, 0);
        if (g_main.handle) CloseHandle(g_main.handle);
    }
    return TRUE;
}