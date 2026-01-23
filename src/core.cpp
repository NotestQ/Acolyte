#include "core.h"
#include "Acolyte/logger.h"
#include "parser.h"
#include "sdk_registry.h"
#include "MinHook.h"
#include "windows.h"
#include <string>
#include <filesystem>

namespace fs = std::filesystem;

static Logger mainLogger("Acolyte");
static const uintptr_t moduleBaseAddress = (uintptr_t)GetModuleHandleA(NULL);
static const fs::path cwd = fs::current_path();

// Could be a define but eh.
void EnsureFolder(fs::path parentPath, std::string folderName) {
    if (!fs::exists(parentPath / folderName))
        fs::create_directory(parentPath / folderName);
}

void SetFolders() {
    const fs::path acolyteFolder = cwd / "Acolyte";
    EnsureFolder(cwd, "Acolyte");
    EnsureFolder(acolyteFolder, "mods");
    EnsureFolder(acolyteFolder, "config");
    EnsureFolder(acolyteFolder, "logs");
}

int DisplayRiskBox() {
    HWND hWnd = GetActiveWindow();
    int msgboxID = MessageBox(
        hWnd,
        (LPCSTR)"You are about to run a modded version of Vagante!\n"\
                "Only run mods you trust, they may be malicious!",
        (LPCSTR)"Warning",
        MB_OKCANCEL | MB_ICONWARNING
    );
    return msgboxID;
}
DWORD WINAPI init() {
    // TODO: Settings and log and cache so this warning doesn't display every time
    // It's annoying as hell
    // Not needed for now as there is only one mod, plus big notice in the GitHub
    //int boxResult = DisplayRiskBox();
    //if (boxResult == 2)
    //    return 0;
    AllocConsole(); // TODO: Read from config to not display a console, logs should still work though
    FILE* conOutStr;
    FILE* conErrStr;
    freopen_s(&conOutStr, "CONOUT$", "w", stdout);
    freopen_s(&conErrStr, "CONOUT$", "wb", stderr);

    mainLogger.Log("Acolyte has loaded.");

    SetFolders();

    if (MH_Initialize() != MH_OK) {
        mainLogger.LogError("MinHook couldn't initialize!");
        return 0;
    }
    mainLogger.LogDebug("Attempting to init all SDK components");
    InitAllSDKComponents(); // SDK vs API vs Library go fight

    auto loadOrder = ParseMods(cwd / "Acolyte" / "Mods");
    if (loadOrder) {
        for (auto& manifest : *loadOrder) {
            mainLogger.LogDebug("Loading " + manifest.name);
            for (const auto& dll : fs::directory_iterator(manifest.manifest_path.parent_path())) {
                if (dll.path().extension() == ".dll")
                    LoadLibraryA(dll.path().string().c_str());
            }
        }
        mainLogger.Log("Mods loaded, have fun!");
    }
    else
        mainLogger.Log("No mods were loaded!");

    return 0;
}