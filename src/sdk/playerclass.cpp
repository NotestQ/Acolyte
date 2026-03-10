#include <Acolyte/playerclass.h>
#include <Acolyte/resources.h>
#include <Acolyte/utils.h>
#include <Acolyte/logger.h>
#include <filesystem>
#include <optional>
#include <safetyhook.hpp>
#include <sdk_registry.h>
#undef LoadIcon

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
    auto wishSize = baseClassAmount + sdk::PlayerClass::ModdedClassList.size();

    if (vectorSize < wishSize) {
        playerClassVector.reserve(wishSize); // Reserve so we can push back without the vector resizing every time, micro optimization
        // Remove and re-insert 6 (CLASS_RANDOM) at the end
        playerClassVector.pop_back();

        // Insert class based on pre-calculated index
        for (auto i = vectorSize - baseClassAmount; i < sdk::PlayerClass::ModdedClassList.size(); i++) {
            sdk::PlayerClass::ModdedClass newClass = sdk::PlayerClass::ModdedClassMap[sdk::PlayerClass::ModdedClassList.at(i)];
            playerClassVector.push_back(newClass.index);
        }
        playerClassVector.push_back(6);
    }

    return pOriginalGetPlayerClass(menuThis, NULL);
}

sdk::PlayerClass::SetStartingAttributesFn pOriginalSetStartingAttributes = nullptr;
void __cdecl setStartingAttributesDetour(std::shared_ptr<Player> player, void* unused) {
    Player* playerPtr = player.get();
    auto moddedClass = sdk::PlayerClass::GetByIndex(playerPtr->playerClass);
    if (moddedClass) {
        auto& playerStats = playerPtr->stats;
        const ClassStats& wishStats = (*moddedClass)->stats;

        if (wishStats.STR) playerStats[0] = *wishStats.STR;
        if (wishStats.DEX) playerStats[1] = *wishStats.DEX;
        if (wishStats.VIT) playerStats[2] = *wishStats.VIT;
        if (wishStats.SPD) playerStats[3] = *wishStats.SPD;
        if (wishStats.STA) playerStats[4] = *wishStats.STA;
        if (wishStats.INT) playerStats[5] = *wishStats.INT;
        if (wishStats.LUK) playerStats[6] = *wishStats.LUK;
    };

    pOriginalSetStartingAttributes(player, unused);
}

static void InitClassText() {
    sf::Texture* classTextTexture = sdk::Resources::GetTexture(R"(gui/character_menu/class_text.png)", false);
    if (!classTextTexture) {
        playerClassLogger.LogError("Class texture does not exist yet!");
        return;
    }

    sf::Image textImage{};
    sdk::Resources::CopyToImage(classTextTexture, &textImage);

    std::vector<sf::Image> images = sdk::Resources::SeparateImageChunks(textImage, 64);
    TextureData data{};

    data.images = images;
    data.texture = *classTextTexture;
    TextureDataMap[R"(gui/character_menu/class_text.png)"] = data;
}

static void ResourcesInitialized() {
    sdk::Resources::ResourcesInitialized = true;
    InitClassText();

    while (!sdk::PlayerClass::AssetLoadQueue.empty()) {
        auto* moddedClass = sdk::PlayerClass::AssetLoadQueue.front();
        moddedClass->LoadAssets();
        sdk::PlayerClass::AssetLoadQueue.pop();
    }
}

namespace sdk::PlayerClass {
    ModdedClass* ModdedClass::LoadPalettes() {
        for (int paletteI = 1; paletteI <= 4; paletteI++) {
            fs::path palettePath = mod_path / std::to_string(paletteI);
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

                        sdk::Resources::TextureConstructor(&newTexture);
                        sdk::Resources::LoadTextureFromFile(&newTexture, &path, &rect);
                        std::replace(path.begin(), path.end(), '\\', '/'); // Load texture from file is fine with either, map isn't

                        TextureData data{};
                        data.texture = newTexture;
                        data.images = {};
                        TextureDataMap[path] = data;
                    }
                }
            }
        }
        _paletteLoaded = true;
        return this;
    }

    ModdedClass* ModdedClass::LoadIcon() {
        _iconLoaded = true;
        return this;
    }

    ModdedClass* ModdedClass::LoadText() {
        fs::path textPath = mod_path / "text.png";
        if (!fs::exists(textPath)) {
            playerClassLogger.LogError("text.png not found in: " + mod_path.string());
            return this;
        }

        sf::Image textImage{};
        std::string textPathStr = textPath.string();
        if (!sdk::Resources::LoadImageFromFile(&textImage, &textPathStr)) {
            playerClassLogger.LogError("Failed to load text.png for: " + name);
            return this;
        }

        if (textImage.m_size.x != 64 || textImage.m_size.y != 64) {
            playerClassLogger.LogWarning("Text icon for " + name + " is not 64x64 (found " +
                std::to_string(textImage.m_size.x) + "x" +
                std::to_string(textImage.m_size.y) + "), may look incorrect");
        }

        std::string textureKey = "gui/character_menu/class_text.png";
        if (TextureDataMap.contains(textureKey)) {
            TextureDataMap[textureKey].images.push_back(textImage);

            sf::Image stitchedImage = sdk::Resources::StitchImageChunks(TextureDataMap[textureKey].images, 128, 64);

            sf::Texture newTexture{};
            sdk::Resources::TextureConstructor(&newTexture);
            sf::Rect<int> rect{};
            if (!sdk::Resources::LoadTextureFromImage(&newTexture, &stitchedImage, &rect)) {
                playerClassLogger.LogError("Failed to update class texture for: " + name);
                return this;
            }

            TextureDataMap[textureKey].texture = newTexture;
        }
        else {
            playerClassLogger.LogError("Class texture not initialized, something went very wrong");
            return this;
        }
        _textLoaded = true;
        return this;
    }

    ModdedClass* ModdedClass::LoadAssets() {
        if (!sdk::Resources::ResourcesInitialized) {
            AssetLoadQueue.push(this);
            return this;
        }

        if (!_textLoaded)
            LoadText();
        if (!_iconLoaded)
            LoadIcon();
        if (!_paletteLoaded)
            LoadPalettes();
        return this;
    }  
   
    ModdedClass* ModdedClass::SetStats(const ClassStats& stats) {
        if (stats.STR) this->stats.STR = *stats.STR;
        if (stats.DEX) this->stats.DEX = *stats.DEX;
        if (stats.VIT) this->stats.VIT = *stats.VIT;
        if (stats.SPD) this->stats.SPD = *stats.SPD;
        if (stats.STA) this->stats.STA = *stats.STA;
        if (stats.INT) this->stats.INT = *stats.INT;
        if (stats.LUK) this->stats.LUK = *stats.LUK;
        return this;
    }

    static std::optional<ModdedClass*> RegisterInternal(const fs::path& classPath, const std::string& displayName, bool loadAssets) {
        if (!fs::exists(classPath) || !fs::is_directory(classPath)) {
            playerClassLogger.LogError("Class folder does not exist: " + classPath.string());
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
        std::string gfxPath = classPath.string();
        std::replace(gfxPath.begin(), gfxPath.end(), '\\', '/'); // Vagante uses forward slashes so...
        newClass.gfxPathStorage = gfxPath + "/";
        newClass.gfxPath = newClass.gfxPathStorage.c_str(); // Store both as sometimes Vagante needs a pointer with a longer lifetime

        ModdedClassList.push_back(displayName);
        playerClassLogger.LogDebug("Registered class '" + displayName + "' at modded index " +
            std::to_string(newClass.moddedindex) + " and game index " +
            std::to_string(newClass.index));

        if (loadAssets)
            newClass.LoadAssets();

        return &newClass;
    }

    std::optional<ModdedClass*> Register(const std::string& displayName, bool loadAssets) {
        return Register(displayName, displayName, loadAssets);
    }

    std::optional<ModdedClass*> Register(const std::string& className, const std::string& displayName, bool loadAssets) {
        // Get calling module path
        auto callingModulePath = Utils::GetCallingModule();
        if (!callingModulePath) {
            playerClassLogger.LogError("Failed to detect calling module for class: " + displayName);
            return std::nullopt;
        }

        // Extract directory from DLL path
        fs::path modFolder = fs::path(*callingModulePath).parent_path();
        fs::path classPath = modFolder / "classes" / className;

        return RegisterInternal(classPath, displayName, loadAssets);
    }

    std::optional<ModdedClass*> Register(const fs::path& customPath, const std::string& displayName, bool loadAssets) {
        return RegisterInternal(customPath, displayName, loadAssets);
    }

    std::optional<ModdedClass*> GetByName(const std::string& displayName) {
        auto it = ModdedClassMap.find(displayName);
        if (it != ModdedClassMap.end()) {
            return &it->second;
        }
        return std::nullopt;
    }

    std::optional<ModdedClass*> GetByIndex(int gameIndex) {
        return GetByModdedIndex(gameIndex - 8);
    }

    std::optional<ModdedClass*> GetByModdedIndex(int moddedIndex) {
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
        Utils::CreateHook(reinterpret_cast<void**>(reinterpret_cast<SetStartingAttributesFn>(moduleBaseAddress + 0x30C7D0)), &setStartingAttributesDetour, reinterpret_cast<void**>(&pOriginalSetStartingAttributes));
        
        static SafetyHookMid clasGfxPathSwitchHook = safetyhook::create_mid(reinterpret_cast<void*>(moduleBaseAddress + 0x2C7FF7), classGfxPathDetour); // Line 46? of drawGameOverStats
        static SafetyHookMid classNameSwitchHook = safetyhook::create_mid(reinterpret_cast<void*>(moduleBaseAddress + 0x318094), classNameSwitchDetour); // Line 389 of drawGameOverStats
        static SafetyHookMid drawCustomizeMenuFrameHook = safetyhook::create_mid(reinterpret_cast<void*>(moduleBaseAddress + 0x31316C), drawCustomizeMenuFrameDetour); // Line 143 of DrawCustomizeMenu
        static SafetyHookMid setStartingItemHook = safetyhook::create_mid(reinterpret_cast<void*>(moduleBaseAddress + 0x30C56E), setStartingItemDetour); // Line 859 of setStartingItems

        sdk::Resources::OnResourceInitialized.connect(ResourcesInitialized);

        playerClassLogger.LogDebug("PlayerClass system initialized!");
    }
    SDK_REGISTER(InitPlayerClass)
}
