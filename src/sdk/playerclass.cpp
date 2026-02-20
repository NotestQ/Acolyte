#include <Acolyte/playerclass.h>
#include <Acolyte/vaganteutils.h>
#include <Acolyte/utils.h>
#include <Acolyte/logger.h>
#include <filesystem>
#include <optional>
#include <safetyhook.hpp>
#include <sdk_registry.h>

namespace fs = std::filesystem;

static Logger playerClassLogger("Acolyte");
static const uintptr_t moduleBaseAddress = (uintptr_t)GetModuleHandleA(NULL);

void drawCustomizeMenuFrameDetour(SafetyHookContext& ctx) {
    std::vector<int>& playerClassVector = *reinterpret_cast<std::vector<int>*>(ctx.esi + 0x3398);
    int selectionWrap = *reinterpret_cast<int*>(ctx.esi + 0x33C8);

    int playerClassIndex = playerClassVector.at(selectionWrap);
    // Corrects text.png wrapping, player class 6 (random) will always be at the fifth position
    // 8 and above will always be two behind
    if (playerClassIndex == 6) playerClassIndex = 5; 
    else if (playerClassIndex > 5) playerClassIndex -= 2;

    uint32_t* pushedFramePtr = reinterpret_cast<uint32_t*>(ctx.esp);
    *pushedFramePtr = playerClassIndex;
}

void classNameSwitchDetour(SafetyHookContext& ctx) {
    int classIndex = static_cast<int>(ctx.eax);
    auto moddedClass = sdk::PlayerClass::GetByIndex(classIndex);
    if (!moddedClass)
        return;

    std::string replacement = (*moddedClass)->name;

    std::string* local = reinterpret_cast<std::string*>(ctx.ebp - 0x5c);
    *local = replacement;

    // Midhooks are trampolines, so they preserve the original instruction
    // We increase the instruction pointer by the amount of bytes in the original instruction to skip it
    ctx.eip = ctx.eip + 5;
}

void classGfxPathDetour(SafetyHookContext& ctx) {
    int classIndex = static_cast<int>(ctx.esi) - 8;
    if (classIndex < 0)
        return;
    
    auto moddedClass = sdk::PlayerClass::GetByModdedIndex(classIndex);
    if (!moddedClass) {
        playerClassLogger.LogError("Failed to get class with index " + classIndex);
        return;
    }

    *reinterpret_cast<const char**>(ctx.esp + 4) = (*moddedClass)->gfxPath;
}

void setStartingItemDetour(SafetyHookContext& ctx) {
#define breakSwitch ctx.eip = moduleBaseAddress + 0x30B4E0; return; // Equivalent of a break; on the switch
    
    int classIndex = *reinterpret_cast<int*>(ctx.ecx + 0x34c) - 8;
    auto playerClass = sdk::PlayerClass::GetByModdedIndex(classIndex);

    if (!playerClass) {
        playerClassLogger.LogError("Was unexpectedly not able to find index " + std::to_string(classIndex));
        return;
    }

    if (!(*playerClass)->_debugUseFallbackWeapon)
        breakSwitch

#undef breakSwitch
}

sdk::PlayerClass::GetClassNameFn pOriginalGetClassName = nullptr;
std::string* __fastcall getClassNameDetour(std::string* returnStoragePtr, int classIndex) {
    auto ret = pOriginalGetClassName(returnStoragePtr, classIndex);
    auto moddedClass = sdk::PlayerClass::GetByIndex(classIndex);
    if (!moddedClass)
        return ret;

    std::string replacement = (*moddedClass)->name;
    ret->assign(replacement);
    return ret;
}

sdk::PlayerClass::GetPlayerClassFn pOriginalGetPlayerClass = nullptr;
int __fastcall getPlayerClassDetour(PlayerMenu* menuThis, void*) {
    uintptr_t base = (uintptr_t)menuThis;
    std::vector<int>& playerClassVector = *reinterpret_cast<std::vector<int>*>(base + 0x3398);
    int baseClassAmount = 6;
    auto vectorSize = playerClassVector.size();
    auto wishSize = baseClassAmount + ModdedClassList.size();

    if (vectorSize < wishSize) {
        playerClassVector.reserve(wishSize); // Reserve so we can push back without the vector resizing every time, micro optimization
        // Remove and re-insert 6 (CLASS_RANDOM) at the end
        playerClassVector.pop_back();

        // Insert class based on pre-calculated index
        for (auto i = vectorSize - baseClassAmount; i < ModdedClassList.size(); i++) {
            ModdedClass newClass = ModdedClassMap[ModdedClassList.at(i)];
            playerClassVector.push_back(newClass.index);
        }
        playerClassVector.push_back(6);
    }

    return pOriginalGetPlayerClass(menuThis, NULL);
}

static void InitTextures() {
    sf::Texture* classTextTexture = VaganteUtils::GetTexture(R"(gui/character_menu/class_text.png)", false);
    while (!classTextTexture) {
        classTextTexture = VaganteUtils::GetTexture(R"(gui/character_menu/class_text.png)", false);

        if (!classTextTexture) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
    
    sf::Image textImage{};
    VaganteUtils::CopyToImage(classTextTexture, &textImage);

    std::vector<sf::Image> images = VaganteUtils::SeparateImageChunks(textImage, 64);
    TextureData data{};

    data.images = images;
    data.texture = *classTextTexture;
    TextureDataMap[R"(gui/character_menu/class_text.png)"] = data;
}

static std::optional<const ModdedClass*> RegisterInternal(const fs::path& classPath, const std::string& displayName) {
    if (!fs::exists(classPath) || !fs::is_directory(classPath)) {
        playerClassLogger.LogError("Class folder does not exist: " + classPath.string());
        return std::nullopt;
    }

    fs::path textPath = classPath / "text.png";
    if (!fs::exists(textPath)) {
        playerClassLogger.LogError("text.png not found in: " + classPath.string());
        return std::nullopt;
    }

    if (ModdedClassMap.contains(displayName)) {
        playerClassLogger.LogWarning("Class already registered: " + displayName);
        return &ModdedClassMap[displayName];
    }

    auto& newClass = ModdedClassMap[displayName];
    newClass.name = displayName;
    newClass.moddedindex = ModdedClassList.size();
    newClass.index = PlayerClassList.size() + newClass.moddedindex + 1; // +1 because 7 is taken
    newClass.mod_path = classPath;
    newClass.unlocked = false;
    std::string gfxPath = classPath.string();
    std::replace(gfxPath.begin(), gfxPath.end(), '\\', '/'); // Vagante uses forward slashes so...
    newClass.gfxPathStorage = gfxPath + "/";
    newClass.gfxPath = newClass.gfxPathStorage.c_str(); // Store both as sometimes Vagante needs a pointer with a longer lifetime

    for (int paletteI = 1; paletteI <= 4; paletteI++) {
        fs::path palettePath = classPath / std::to_string(paletteI);
        if (!fs::exists(palettePath)) {
            playerClassLogger.LogWarning("Palette folder with index " + std::to_string(paletteI) + " not found! Skipping...");
            continue;
        }

        for (const auto& entry : fs::directory_iterator(palettePath)) {
            if (entry.is_regular_file()) {
                fs::path filePath = entry.path();
                std::string extension = filePath.extension().string();

                if (extension == ".png") {
                    sf::Rect<int> rect{};
                    sf::Texture newTexture{};
                    std::string path = filePath.string();

                    VaganteUtils::TextureConstructor(&newTexture);
                    VaganteUtils::LoadTextureFromFile(&newTexture, &path, &rect);
                    std::replace(path.begin(), path.end(), '\\', '/'); // Load texture from file is fine with either, map isn't

                    TextureData data{};
                    data.texture = newTexture;
                    data.images = {};
                    TextureDataMap[path] = data;
                }
            }
        }
    }

    sf::Image textImage{};
    std::string textPathStr = textPath.string();
    if (!VaganteUtils::LoadImageFromFile(&textImage, &textPathStr)) {
        playerClassLogger.LogError("Failed to load text.png for: " + displayName);
        return std::nullopt;
    }

    if (textImage.m_size.x != 64 || textImage.m_size.y != 64) {
        playerClassLogger.LogWarning("Icon for " + displayName + " is not 64x64 (found " +
            std::to_string(textImage.m_size.x) + "x" +
            std::to_string(textImage.m_size.y) + "), may look incorrect");
    }

    std::string textureKey = "gui/character_menu/class_text.png";
    if (!TextureDataMap.contains(textureKey)) // Initialize texture late otherwise the game crashes, band-aid
        InitTextures(); // TODO?: Replace with signal to do it as fast as possible instead of waiting for a mod to require it

    if (TextureDataMap.contains(textureKey)) {
        TextureDataMap[textureKey].images.push_back(textImage);

        sf::Image stitchedImage = VaganteUtils::StitchImageChunks(TextureDataMap[textureKey].images, 128, 64);

        sf::Texture newTexture{};
        VaganteUtils::TextureConstructor(&newTexture);
        sf::Rect<int> rect{};
        if (!VaganteUtils::LoadTextureFromImage(&newTexture, &stitchedImage, &rect)) {
            playerClassLogger.LogError("Failed to update class texture for: " + displayName);
            return std::nullopt;
        }

        TextureDataMap[textureKey].texture = newTexture;
    }
    else {
        playerClassLogger.LogError("Class texture not initialized after init, something went very wrong");
        return std::nullopt;
    }
    
    ModdedClassList.push_back(displayName);

    playerClassLogger.LogDebug("Registered class '" + displayName + "' at modded index " +
        std::to_string(newClass.moddedindex) + " and game index " +
        std::to_string(newClass.index));

    return &newClass;
}

namespace sdk::PlayerClass {
    std::optional<const ModdedClass*> Register(const std::string& displayName) {
        return Register(displayName, displayName);
    }

    std::optional<const ModdedClass*> Register(const std::string& className, const std::string& displayName) {
        // Get calling module path
        auto callingModulePath = Utils::GetCallingModule();
        if (!callingModulePath) {
            playerClassLogger.LogError("Failed to detect calling module for class: " + displayName);
            return std::nullopt;
        }

        // Extract directory from DLL path
        fs::path modFolder = fs::path(*callingModulePath).parent_path();
        fs::path classPath = modFolder / "classes" / className;

        return RegisterInternal(classPath, displayName);
    }

    std::optional<const ModdedClass*> Register(const fs::path& customPath, const std::string& displayName) {
        return RegisterInternal(customPath, displayName);
    }

    std::optional<const ModdedClass*> GetByName(const std::string& displayName) {
        auto it = ModdedClassMap.find(displayName);
        if (it != ModdedClassMap.end()) {
            return &it->second;
        }
        return std::nullopt;
    }

    std::optional<const ModdedClass*> GetByIndex(int gameIndex) {
        return GetByModdedIndex(gameIndex - 8);
    }
    
    std::optional<const ModdedClass*> GetByModdedIndex(int moddedIndex) {
        if (moddedIndex >= 0 && (uint)moddedIndex < ModdedClassList.size()) {
            return &ModdedClassMap[ModdedClassList[moddedIndex]];
        }
        return std::nullopt;
    }

    bool IsModdedClass(int gameIndex) {
        // Base classes are 0-6, modded classes start at 8+
        return gameIndex >= 8;
    }

    void InitPlayerClass() {
        playerClassLogger.LogDebug("Initializing PlayerClass system...");
        Utils::CreateHook(reinterpret_cast<void**>(reinterpret_cast<GetPlayerClassFn>(moduleBaseAddress + 0x31E430)), &getPlayerClassDetour, reinterpret_cast<void**>(&pOriginalGetPlayerClass));
        Utils::CreateHook(reinterpret_cast<void**>(reinterpret_cast<GetClassNameFn>(moduleBaseAddress + 0x2C7EE0)), &getClassNameDetour, reinterpret_cast<void**>(&pOriginalGetClassName));
        
        static SafetyHookMid clasGfxPathSwitchHook = safetyhook::create_mid(reinterpret_cast<void*>(moduleBaseAddress + 0x2C7FF7), classGfxPathDetour); // Line 46? of drawGameOverStats
        static SafetyHookMid classNameSwitchHook = safetyhook::create_mid(reinterpret_cast<void*>(moduleBaseAddress + 0x318094), classNameSwitchDetour); // Line 389 of drawGameOverStats
        static SafetyHookMid drawCustomizeMenuFrameHook = safetyhook::create_mid(reinterpret_cast<void*>(moduleBaseAddress + 0x31316C), drawCustomizeMenuFrameDetour); // Line 143 of DrawCustomizeMenu
        static SafetyHookMid setStartingItemHook = safetyhook::create_mid(reinterpret_cast<void*>(moduleBaseAddress + 0x30C56E), setStartingItemDetour); // Line 859 of setStartingItems

        playerClassLogger.LogDebug("PlayerClass system initialized!");
    }
    SDK_REGISTER(InitPlayerClass)
}
