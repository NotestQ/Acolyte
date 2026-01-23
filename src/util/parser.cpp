#include "Acolyte/logger.h"
#include "parser.h"
#include "toml.hpp"
#include <vector>
#include <unordered_map>
#include <string>
#include <filesystem>
#include <iostream>
#include <unordered_set>
#include <regex>

static Logger mainLogger("Acolyte");
namespace fs = std::filesystem;

// Implements https://en.wikipedia.org/wiki/Topological_sorting#Depth-first_search
static bool DepthFirstSearch(const std::string& modName,
    const Graph& graph,
    std::unordered_set<std::string>& visiting,
    std::unordered_set<std::string>& visited,
    std::vector<std::string>& result,
    std::vector<std::string>& cycle)
{
    if (visited.contains(modName)) return false;
    if (visiting.contains(modName)) {
        cycle.insert(cycle.begin(), modName);
        return true; // found cycle
    }

    visiting.insert(modName);
    auto dependencies = graph.find(modName);
    if (dependencies != graph.end()) {
        // Gets the std::unordered_set then loops through it
        // Loops through all the dependencies in this mod, trying to find a mod we're currently checking/recursing through/visiting
        // When it returns true it goes all the way back to the original mod, so the cycle vector will have the full path.
        for (const auto& dep : dependencies->second) {
            if (DepthFirstSearch(dep, graph, visiting, visited, result, cycle)) {
                // Mod in this instance would be the dependency
                // with dep being what the search was called for originally
                if (cycle.back() != modName)
                    cycle.insert(cycle.begin(), modName);
                return true;
            }
        }
    }

    visiting.erase(modName);
    visited.insert(modName);
    result.insert(result.begin(), modName);
    return false;
}

static std::optional<std::vector<std::string>> TopologicalSort(Graph& graph) {
    std::unordered_set<std::string> visiting;
    std::unordered_set<std::string> visited;
    std::vector<std::string> result;
    std::vector<std::string> cycle;

    for (auto& mod : graph) {
        if (!visited.contains(mod.first)) {
            if (DepthFirstSearch(mod.first, graph, visiting, visited, result, cycle)) {
                std::string cycleStr = "";
                for (auto& cycleMod : cycle) {
                    cycleStr = cycleStr + cycleMod;
                    if (cycleMod != cycle.front())
                        cycleStr = cycleStr + " -> ";
                }
                mainLogger.LogError((std::string)"Mods will not load, circular dependencies detected:\n" + "    ", cycleStr);
                return std::nullopt;
            }
        }
    }

    return result;
}

static std::optional<ModManifest> ReadManifest(const fs::path& filePath) {
    try {
        toml::table manifestToml = toml::parse_file(filePath.string());
        ModManifest manifest;

        mainLogger.LogDebug("Reading name");
        std::optional<std::string> name = manifestToml["name"].value<std::string>();
        if (name)
            manifest.name = *name;
        else {
            mainLogger.LogError(filePath.parent_path().filename().string() + "'s manifest has no name! Skipping...");
            return std::nullopt;
        }

        mainLogger.LogDebug("Reading version");
        std::optional<std::string> version = manifestToml["version"].value<std::string>();
        if (version) {
            //TODO: Enforce SemVer
            manifest.version = *version;
        }
        else {
            mainLogger.LogWarning(filePath.parent_path().filename().string() + "'s manifest has no version! Setting to 0.0.0...");
            manifest.version = "0.0.0";
        }

        mainLogger.LogDebug("Reading dependencies");
        if (manifestToml["dependencies"].is_array()) {
            toml::array& dependencyArray = *manifestToml.get_as<toml::array>("dependencies");
            for (auto&& elem : dependencyArray) {
                std::optional<std::string> dependency = elem.value<std::string>();
                if (dependency)
                    manifest.dependencies.insert(*dependency);
            }
        }

        mainLogger.LogDebug("Reading incompatibilities");
        if (manifestToml["incompatible"].is_array()) {
            toml::array& incompatibleArray = *manifestToml.get_as<toml::array>("incompatible");
            for (auto&& elem : incompatibleArray) {
                std::optional<std::string> incompatibility = elem.value<std::string>();
                if (incompatibility)
                    manifest.incompatible.insert(*incompatibility);
            }
        }

        manifest.manifest_path = filePath;
        return manifest;
    }
    catch (const toml::parse_error& error) {
        std::stringstream ss;
        ss << "Failed to parse " << filePath << "\n" << error;
        mainLogger.LogError(ss.str());
        return std::nullopt;
    }
}

std::optional<std::vector<ModManifest>> ParseMods(const fs::path &modsFolderPath) {
    for (const auto& mod : fs::directory_iterator(modsFolderPath)) {
        if (!mod.is_directory())
            continue;
        
        for (const auto& file : fs::directory_iterator(mod)) {
            auto filePath = file.path();
            if (filePath.filename() == "manifest.toml") {
                mainLogger.LogDebug("Reading " + mod.path().filename().string() + "'s manifest");
                auto manifest = ReadManifest(filePath);
                if (manifest)
                    ManifestMap[manifest->name] = *manifest;
            }
        }
    }
    mainLogger.LogDebug("Building graph");
    Graph graph;
    std::vector<ModManifest> finalList;
    for (const auto& manifestKV : ManifestMap) {
        if (manifestKV.second.dependencies.empty()) {
            finalList.push_back(manifestKV.second);
            continue;
        }  
        graph[manifestKV.first] = manifestKV.second.dependencies;
    }
    mainLogger.LogDebug("Sorting graph");
    auto sortedList = TopologicalSort(graph);
    
    if (sortedList) {
        for (auto& modName : *sortedList) {
            finalList.push_back(ManifestMap[modName]);
        }

        return finalList;
    }
    return std::nullopt;
}