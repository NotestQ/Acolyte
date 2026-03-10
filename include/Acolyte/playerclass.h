#pragma once
#include "Vagante/entities.h"
#include <Acolyte/exports.h>
#include <string>
#include <optional>
#include <filesystem>
#include <vector>
#include <unordered_map>
#undef LoadIcon

struct ClassStats {
    std::optional<int> STR;
    std::optional<int> DEX;
    std::optional<int> VIT;
    std::optional<int> SPD;
    std::optional<int> STA;
    std::optional<int> INT;
    std::optional<int> LUK;
};

static std::vector<std::string> PlayerClassList{ // Full base class list without enum trickery, index -> string
    "CLASS_KNIGHT",
    "CLASS_ROGUE",
    "CLASS_MAGE",
    "CLASS_DEPRIVED",
    "CLASS_BEASTMASTER",
    "CLASS_SKELETON",
    "CLASS_RANDOM",
};
static std::unordered_map<std::string, int> ClassNameToIndex{ // string -> index helper
    {"CLASS_KNIGHT", 0},
    {"CLASS_ROGUE", 1},
    {"CLASS_MAGE", 2},
    {"CLASS_DEPRIVED", 3},
    {"CLASS_BEASTMASTER", 4},
    {"CLASS_SKELETON", 5},
    {"CLASS_RANDOM", 6},
};

namespace sdk::PlayerClass {
    struct ModdedClass {
        std::string name;
        int index{};
        int moddedindex{};
        ClassStats stats{};

        // I dislike the concept of private variables in a modding environment
        // My mind can and may be changed
        std::filesystem::path mod_path;
        std::string gfxPathStorage;
        const char* gfxPath{};
        bool _debugUseFallbackWeapon = false;
        bool _paletteLoaded = false;
        bool _textLoaded = false;
        bool _iconLoaded = false;

        ACOLYTE_API ModdedClass* LoadPalettes();
        ACOLYTE_API ModdedClass* LoadText();
        ACOLYTE_API ModdedClass* LoadIcon();
        // Calls LoadText, LoadIcon then LoadPalettes. Queues if resources are not ready.
        ACOLYTE_API ModdedClass* LoadAssets();

        // Helper to set multiple stats at once and be able to chain call functions
        ACOLYTE_API ModdedClass* SetStats(const ClassStats& stats);
    };

    using GetClassNameFn = std::string* (__fastcall*)(std::string* __return_storage_ptr__, int classIndex);
    using GetPlayerClassFn = int(__fastcall*)(PlayerMenu* menuThis, void* edx);
    using SetStartingAttributesFn = void(__cdecl*)(std::shared_ptr<Player> player, void* unused);

    inline std::unordered_map<std::string, ModdedClass> ModdedClassMap;
    inline std::vector<std::string> ModdedClassList{}; // Add 7, 6 if not counting skeleton
    inline std::queue<ModdedClass*> AssetLoadQueue{};

    // New player class using displayName for both folder and display
    // Expects: <calling_mod_folder>/classes/<displayName>/text.png and /1 - 4
    ACOLYTE_API std::optional<ModdedClass*> Register(const std::string& displayName, bool loadAssets = true);

    // New player class with separate folder name and display name
    // Expects: <calling_mod_folder>/classes/<className>/text.png and /1 - 4
    ACOLYTE_API std::optional<ModdedClass*> Register(const std::string& className, const std::string& displayName, bool loadAssets = true);

    // New player class with custom path
    // Expects: <customPath>/text.png and /1 - 4
    ACOLYTE_API std::optional<ModdedClass*> Register(const std::filesystem::path& customPath, const std::string& displayName, bool loadAssets = true);

    // Helper functions

    ACOLYTE_API std::optional<ModdedClass*> GetByName(const std::string& displayName);
    ACOLYTE_API std::optional<ModdedClass*> GetByIndex(int gameIndex);
    ACOLYTE_API std::optional<ModdedClass*> GetByModdedIndex(int moddedIndex);
    ACOLYTE_API bool IsModdedClass(int gameIndex);
}