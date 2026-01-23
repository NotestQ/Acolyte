#pragma once
#include "vagante.h"
#include <Acolyte/exports.h>
#include <string>
#include <optional>
#include <filesystem>
#include <vector>
#include <unordered_map>

// Player class data structure
struct ModdedClass {
    std::string name;
    bool unlocked{};
    int index{};
    int moddedindex{};
    std::filesystem::path mod_path;
    bool _debugUseFallbackWeapon = false;
    std::string gfxPathStorage;
    const char* gfxPath;
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

inline std::unordered_map<std::string, ModdedClass> ModdedClassMap;
inline std::vector<std::string> ModdedClassList{}; // Add 7, 6 if not counting skeleton

namespace sdk::PlayerClass {
    using GetClassNameFn = std::string* (__fastcall*)(std::string* __return_storage_ptr__, int classIndex);
    using GetPlayerClassFn = int(__fastcall*)(PlayerMenu* menuThis, void* edx);

    // Register a new player class using displayName for both folder and display
    // Expects: <calling_mod_folder>/classes/<displayName>/text.png and /1 - 4
    ACOLYTE_API std::optional<const ModdedClass*> Register(const std::string& displayName);

    // Register a new player class with separate folder name and display name
    // Expects: <calling_mod_folder>/classes/<className>/text.png and /1 - 4
    ACOLYTE_API std::optional<const ModdedClass*> Register(const std::string& className, const std::string& displayName);

    // Register a new player class with custom path
    // Expects: <customPath>/text.png and /1 - 4
    ACOLYTE_API std::optional<const ModdedClass*> Register(const std::filesystem::path& customPath, const std::string& displayName);

    // Helper functions
    ACOLYTE_API std::optional<const ModdedClass*> GetByName(const std::string& displayName);
    ACOLYTE_API std::optional<const ModdedClass*> GetByIndex(int gameIndex);
    ACOLYTE_API std::optional<const ModdedClass*> GetByModdedIndex(int moddedIndex);
    ACOLYTE_API bool IsModdedClass(int gameIndex);
}