// General utils not specifically Vagante related
#include <Acolyte/utils.h>
#include "MinHook.h"
#include <vector>
#include <Acolyte/logger.h>

static Logger utilsLogger("Acolyte");

namespace Utils {
    void CreateHook(LPVOID pTarget, LPVOID pDetour, LPVOID* ppOriginal) {
        MH_STATUS hook = MH_CreateHook(pTarget, pDetour, ppOriginal);
        if (hook != MH_OK) {
            utilsLogger.LogError("Couldn't hook! Status: ", std::to_string(hook));
        }
        MH_EnableHook(pTarget);
    }

    // Rewrite memory directly at address
    int PatchMemory(void* address, const void* data, SIZE_T len) {
        DWORD oldProt;
        if (!VirtualProtect(address, len, PAGE_EXECUTE_READWRITE, &oldProt)) return 0;

        memcpy(address, data, len);
        FlushInstructionCache(GetCurrentProcess(), address, len);

        DWORD tmp;
        VirtualProtect(address, len, oldProt, &tmp);
        return 1;
    }

    // Convenience to patch a byte in the instruction address
    int PatchByteAt(uintptr_t addr, size_t offset, uint8_t value) {
        void* target = reinterpret_cast<void*>(addr + offset);
        return PatchMemory(target, &value, sizeof(value));
    }

    std::optional<std::string> GetCallingModule()
    {
        const int MAX_FRAMES = 64;
        void* frames[MAX_FRAMES] = { 0 };

        USHORT capture = CaptureStackBackTrace(1, MAX_FRAMES, frames, nullptr);
        auto moduleHandleBitflag = GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT;

        // Current module path for comparison
        HMODULE hCur = nullptr;
        if (!GetModuleHandleExA(
            moduleHandleBitflag,
            reinterpret_cast<LPCSTR>(&GetCallingModule),
            &hCur)) {
            return std::nullopt;
        }

        char curPath[MAX_PATH] = { 0 };
        DWORD curLen = GetModuleFileNameA(hCur, curPath, (DWORD)sizeof(curPath));
        if (curLen == 0) return {};

        // Walk captured frames and return the first module path that differs from current module
        for (USHORT i = 0; i < capture; ++i)
        {
            HMODULE hMod = nullptr;
            if (!GetModuleHandleExA(moduleHandleBitflag, reinterpret_cast<LPCSTR>(frames[i]), &hMod))
                continue;

            char path[MAX_PATH] = { 0 };
            DWORD len = GetModuleFileNameA(hMod, path, (DWORD)sizeof(path));
            if (len == 0)
                continue;

            if (_stricmp(path, curPath) != 0)
                return std::string(path, len);
        }

        return std::nullopt;
    }
}