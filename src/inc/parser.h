#pragma once
#include <string>
#include <unordered_set>
#include <filesystem>

typedef std::unordered_map<std::string, std::unordered_set<std::string>> Graph; // ModName, deps

struct ModManifest {
    std::string name;
    std::string version; // TODO: SemVer regex
    std::unordered_set<std::string> dependencies; // TODO: SemVer matching
    std::unordered_set<std::string> incompatible; // TODO: SemVer matching
    std::filesystem::path manifest_path;
};
static std::unordered_map<std::string, ModManifest> ManifestMap;
std::optional<std::vector<ModManifest>> ParseMods(const std::filesystem::path& modsFolderPath);