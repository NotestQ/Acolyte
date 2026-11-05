// https://github.com/mrexodia/perfect-dll-proxy
#include "inc/core.h"
#include <Windows.h>
#include <safetyhook.hpp>

#ifdef _WIN64
#define DLLPATH "\\\\.\\GLOBALROOT\\SystemRoot\\System32\\version.dll"
#else
#define DLLPATH "\\\\.\\GLOBALROOT\\SystemRoot\\SysWOW64\\version.dll"
#endif // _WIN64

#pragma comment(linker, "/EXPORT:GetFileVersionInfoA=" DLLPATH ".GetFileVersionInfoA")
#pragma comment(linker, "/EXPORT:GetFileVersionInfoByHandle=" DLLPATH ".GetFileVersionInfoByHandle")
#pragma comment(linker, "/EXPORT:GetFileVersionInfoExA=" DLLPATH ".GetFileVersionInfoExA")
#pragma comment(linker, "/EXPORT:GetFileVersionInfoExW=" DLLPATH ".GetFileVersionInfoExW")
#pragma comment(linker, "/EXPORT:GetFileVersionInfoSizeA=" DLLPATH ".GetFileVersionInfoSizeA")
#pragma comment(linker, "/EXPORT:GetFileVersionInfoSizeExA=" DLLPATH ".GetFileVersionInfoSizeExA")
#pragma comment(linker, "/EXPORT:GetFileVersionInfoSizeExW=" DLLPATH ".GetFileVersionInfoSizeExW")
#pragma comment(linker, "/EXPORT:GetFileVersionInfoSizeW=" DLLPATH ".GetFileVersionInfoSizeW")
#pragma comment(linker, "/EXPORT:GetFileVersionInfoW=" DLLPATH ".GetFileVersionInfoW")
#pragma comment(linker, "/EXPORT:VerFindFileA=" DLLPATH ".VerFindFileA")
#pragma comment(linker, "/EXPORT:VerFindFileW=" DLLPATH ".VerFindFileW")
#pragma comment(linker, "/EXPORT:VerInstallFileA=" DLLPATH ".VerInstallFileA")
#pragma comment(linker, "/EXPORT:VerInstallFileW=" DLLPATH ".VerInstallFileW")
#pragma comment(linker, "/EXPORT:VerLanguageNameA=" DLLPATH ".VerLanguageNameA")
#pragma comment(linker, "/EXPORT:VerLanguageNameW=" DLLPATH ".VerLanguageNameW")
#pragma comment(linker, "/EXPORT:VerQueryValueA=" DLLPATH ".VerQueryValueA")
#pragma comment(linker, "/EXPORT:VerQueryValueW=" DLLPATH ".VerQueryValueW")

void initializeBeforeMainDetour(SafetyHookContext& ctx) {
    init();
}

BOOL WINAPI DllMain(HMODULE hModule, DWORD callReason, LPVOID lpvReserved) {

    if (callReason == DLL_PROCESS_ATTACH) {
        // Initialize before the game allocates its player arrays
        auto moduleBaseAddress = (uintptr_t)GetModuleHandleA(NULL);
        static SafetyHookMid mainHook = safetyhook::create_mid(reinterpret_cast<void*>(moduleBaseAddress + 0x2A0570), initializeBeforeMainDetour);
        if (!mainHook) return FALSE;
    }

    return TRUE;
}
