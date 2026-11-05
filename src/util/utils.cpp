// General utils not specifically Vagante related
#include <Acolyte/utils.h>
#include <MinHook.h>
#include <stacktrace>
#include <filesystem>

static Logger utilsLogger("Acolyte");

namespace Utils {
    std::optional<std::string> CreateTemporaryFile(std::string directory) {
        if (directory.empty()) directory = ".";
        std::error_code error;
        std::filesystem::create_directories(directory, error);
        if (error) {
            utilsLogger.LogError("Couldn't create temporary directory " + directory + ": " + error.message());
            return std::nullopt;
        }
        char path[MAX_PATH]{};
        if (!GetTempFileNameA(directory.c_str(), "aco", 0, path)) {
            utilsLogger.LogError("Couldn't create temporary file in " + directory + ": " + std::to_string(GetLastError()));
            return std::nullopt;
        }
        return std::string(path);
    }

    void CreateHook(LPVOID pTarget, LPVOID pDetour, LPVOID* ppOriginal) {
        MH_STATUS hook = MH_CreateHook(pTarget, pDetour, ppOriginal);
        if (hook != MH_OK) {
            utilsLogger.LogError("Couldn't hook! Status: ", std::to_string(hook));
            if (ppOriginal) *ppOriginal = nullptr;
            return;
        }
        hook = MH_EnableHook(pTarget);
        if (hook != MH_OK) {
            utilsLogger.LogError("Couldn't enable hook! Status: ", std::to_string(hook));
            MH_RemoveHook(pTarget);
            if (ppOriginal) *ppOriginal = nullptr;
        }
    }

    SafetyHookMid CreateMidHook(void* target, safetyhook::MidHookFn detour) {
        auto hook = SafetyHookMid::create(target, detour);
        if (!hook) {
            utilsLogger.LogError("Couldn't create mid hook at " + std::to_string(reinterpret_cast<uintptr_t>(target)) + "! Status: ",
                std::to_string(hook.error().type));
            ExitProcess(1);
        }

        return std::move(*hook);
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
