#include <Acolyte/logger.h>
#include <Acolyte/utils.h>
#include <Acolyte/playerclass.h>
#include <Vagante/game.h>
#include <safetyhook.hpp>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <map>
#include <memory>
#include <new>
#include <string>
#include <vector>
#include "multiplayer.h"

static uintptr_t moduleBaseAddress = (uintptr_t)GetModuleHandleA(NULL);
static Logger saveLogger("Acolyte");

using SaveConstructorFn = GameSave*(__thiscall*)(GameSave* saveThis);
using SaveDestructorFn = void(__thiscall*)(GameSave* saveThis);
using CaptureSaveFn = void(__thiscall*)(GameSave* saveThis, GameEngine* engine);
using SaveFileFn = bool(__thiscall*)(GameSave* saveThis, std::string* filename);
using InitializeSaveFn = void(__thiscall*)(GameSave* saveThis, GameEngine* engine);
static SaveConstructorFn SaveConstructor = nullptr;
static SaveDestructorFn SaveDestructor = nullptr;

// Extra players are saved in groups of four after the original save data.
struct ExtraFooter {
    char magic[8];
    uint size;
};

using ExtraPlayers = std::vector<std::shared_ptr<GameSave>>;
static ExtraPlayers capturedExtras;
static GameSave* capturedOwner = nullptr;
static unsigned captureOffset = 0;
static ExtraPlayers* restoringExtras = nullptr;
static GameSave* emptyExtra = nullptr;
static std::map<std::string, ExtraPlayers> loadedExtras;

static void DestroySave(GameSave* save) {
    SaveDestructor(save);
    ::operator delete(save);
}

static std::shared_ptr<GameSave> CreateSave() {
    // SaveConstructor initializes the vectors too, so don't construct GameSave twice
    GameSave* save = static_cast<GameSave*>(::operator new(sizeof(GameSave)));
    std::memset(save, 0, sizeof(GameSave));
    SaveConstructor(save);
    for (auto& player : save->m_playerData)
        player.playerClass = NUM_PLAYER_CLASSES;
    return std::shared_ptr<GameSave>(save, DestroySave);
}

void captureExtraSlotsDetour(SafetyHookContext& ctx) {
    // createSaveState reads four players from ECX. Move it to the next group.
    ctx.ecx += captureOffset * sizeof(std::shared_ptr<Player>);
}

CaptureSaveFn pOriginalCaptureSave = nullptr;
void __fastcall captureAllPlayersDetour(GameSave* saveThis, void*, GameEngine* engine) {
    pOriginalCaptureSave(saveThis, engine);
    capturedExtras.clear();
    unsigned lastSlot = (std::min)(static_cast<unsigned>(engine->players.size()), Multiplayer::playerSlots);
    while (lastSlot > 4 && !engine->players[lastSlot - 1]) --lastSlot;
    // Keep gaps between players, but don't capture unused groups at the end.
    for (captureOffset = 4; captureOffset < lastSlot; captureOffset += 4) {
        auto extra = CreateSave();
        pOriginalCaptureSave(extra.get(), engine);
        capturedExtras.push_back(std::move(extra));
    }
    captureOffset = 0;
    capturedOwner = saveThis;
}

SaveFileFn pOriginalSaveFile = nullptr;
static bool WriteExtraPlayers(std::string& mainFile, std::string& extraFile) {
    std::ofstream output(mainFile, std::ios::binary | std::ios::app);
    uint32_t count = static_cast<uint32_t>(capturedExtras.size());
    output.write(reinterpret_cast<char*>(&count), sizeof(count));
    uint64_t total = sizeof(count);
    for (auto& extra : capturedExtras) {
        if (!pOriginalSaveFile(extra.get(), &extraFile)) return false;
        std::ifstream input(extraFile, std::ios::binary | std::ios::ate);
        auto length = input.tellg();
        if (length <= 0 || static_cast<uint64_t>(length) + total + 4 > UINT32_MAX) return false;
        uint32_t size = static_cast<uint32_t>(length);
        total += sizeof(size) + size;
        output.write(reinterpret_cast<char*>(&size), sizeof(size));
        input.seekg(0);
        output << input.rdbuf();
        if (!input || !output) return false;
    }
    ExtraFooter footer{{'V', 'M', 'E', 'P', 'S', 'A', 'V', 'E'}, static_cast<uint>(total)};
    output.write(reinterpret_cast<char*>(&footer), sizeof(footer));
    output.close();
    return output.good();
}

bool __fastcall saveAllPlayersDetour(GameSave* saveThis, void*, std::string* filename) {
    if (saveThis != capturedOwner) {
        saveLogger.LogError("Save has no captured extra-player records.");
        return false;
    }

    // Replace the old save only after all player records have been written
    std::string destination = *filename;
    std::string mainFile = destination + ".players-writing";
    std::string extraFile = destination + ".players-extra";
    bool saved = pOriginalSaveFile(saveThis, &mainFile);
    saveThis->m_loadedFileName = destination;
    if (saved && !capturedExtras.empty())
        saved = WriteExtraPlayers(mainFile, extraFile);
    if (saved)
        saved = MoveFileExA(mainFile.c_str(), destination.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
    DeleteFileA(mainFile.c_str());
    DeleteFileA(extraFile.c_str());
    if (saved) {
        if (capturedExtras.empty())
            loadedExtras.erase(destination);
        else
            loadedExtras[destination] = capturedExtras;
    }
    return saved;
}

SaveFileFn pOriginalLoadFile = nullptr;
// 0: ordinary four-player save; 1: extended save; -1: unreadable extension.
static int ReadExtraPlayers(std::string& filename, std::string& extractedFile, ExtraPlayers& extras) {
    std::ifstream input(filename, std::ios::binary | std::ios::ate);
    if (!input) return -1;
    auto length = input.tellg();
    if (length < static_cast<std::streamoff>(sizeof(ExtraFooter))) return 0;
    ExtraFooter footer{};
    input.seekg(-static_cast<std::streamoff>(sizeof(footer)), std::ios::end);
    input.read(reinterpret_cast<char*>(&footer), sizeof(footer));
    if (!input) return -1;
    bool legacy = std::memcmp(footer.magic, "VME8SAVE", 8) == 0;
    if (!legacy && std::memcmp(footer.magic, "VMEPSAVE", 8) != 0) return 0;
    if (!footer.size || footer.size > static_cast<uint64_t>(length) - sizeof(footer)) return -1;
    input.seekg(length - static_cast<std::streamoff>(sizeof(footer)) - static_cast<std::streamoff>(footer.size));
    uint32_t remaining = footer.size;
    uint32_t count = 1;
    if (!legacy) {
        if (remaining < sizeof(count)) return -1;
        input.read(reinterpret_cast<char*>(&count), sizeof(count));
        remaining -= sizeof(count);
    }
    if (!count || count > Multiplayer::playerSlots / 4 - 1) return -1;
    for (uint32_t i = 0; i < count; ++i) {
        uint32_t size = remaining;
        if (!legacy) {
            if (remaining < sizeof(size)) return -1;
            input.read(reinterpret_cast<char*>(&size), sizeof(size));
            remaining -= sizeof(size);
        }
        if (!input || !size || size > remaining) return -1;
        std::vector<char> bytes(size);
        input.read(bytes.data(), bytes.size());
        if (!input) return -1;
        remaining -= size;
        std::ofstream output(extractedFile, std::ios::binary | std::ios::trunc);
        output.write(bytes.data(), bytes.size());
        output.close();
        if (!output) return -1;
        auto extra = CreateSave();
        if (!pOriginalLoadFile(extra.get(), &extractedFile)) return -1;
        extras.push_back(std::move(extra));
    }
    return remaining == 0 ? 1 : -1;
}

bool __fastcall loadAllPlayersDetour(GameSave* saveThis, void*, std::string* filename) {
    if (!pOriginalLoadFile(saveThis, filename)) return false;
    ExtraPlayers extras;
    std::string extractedFile = *filename + ".players-reading";
    int result = ReadExtraPlayers(*filename, extractedFile, extras);
    DeleteFileA(extractedFile.c_str());
    if (result < 0) {
        saveLogger.LogError("Could not read extra players from " + *filename);
        loadedExtras.erase(*filename);
        return false;
    }
    if (result)
        loadedExtras[*filename] = std::move(extras);
    else
        loadedExtras.erase(*filename);
    return true;
}

InitializeSaveFn pOriginalInitializeSave = nullptr;
void __fastcall initializeAllPlayersDetour(GameSave* saveThis, void*, GameEngine* engine) {
    // The load menu copies GameSave, so use its filename to find the extra players.
    auto found = loadedExtras.find(saveThis->m_loadedFileName);
    auto empty = CreateSave();
    restoringExtras = found == loadedExtras.end() ? nullptr : &found->second;
    emptyExtra = empty.get();
    pOriginalInitializeSave(saveThis, engine);
    restoringExtras = nullptr;
    emptyExtra = nullptr;
}

static GameSave::PlayerData* ExtraRecord(unsigned slot) {
    unsigned group = slot / 4 - 1;
    GameSave* save = restoringExtras && group < restoringExtras->size() ? (*restoringExtras)[group].get() : emptyExtra;
    return &save->m_playerData[slot % 4];
}

void restoreExtraPlayerDetour(SafetyHookContext& ctx) {
    unsigned slot = ctx.ebx / sizeof(std::shared_ptr<Player>);
    auto save = *reinterpret_cast<GameSave**>(ctx.ebp - 0x44);
    auto record = slot < 4 ? &save->m_playerData[slot] : ExtraRecord(slot);
    ctx.esi = reinterpret_cast<uintptr_t>(&record->bg);
    *reinterpret_cast<uintptr_t*>(ctx.ebp - 0x3C) = ctx.esi;
    // The game's < NUM_PLAYER_CLASSES check skips modded classes, allow registered ones too
    bool available = (record->playerClass >= CLASS_KNIGHT && record->playerClass < NUM_PLAYER_CLASSES) ||
        sdk::PlayerClass::GetByIndex(record->playerClass).has_value();
    ctx.eip = moduleBaseAddress + (available ? 0x1D2219 : 0x1D21B6);
}

void restoreExtraInventoryDetour(SafetyHookContext& ctx) {
    unsigned slot = ctx.ebx / sizeof(std::shared_ptr<Player>);
    if (slot < 4) return;
    ctx.eax = reinterpret_cast<uintptr_t>(ExtraRecord(slot));
    *reinterpret_cast<uintptr_t*>(ctx.ebp - 0x44) = ctx.eax;
}

namespace Multiplayer {
    bool InitSaves() {
        SaveConstructor = reinterpret_cast<SaveConstructorFn>(moduleBaseAddress + 0x14A880);
        SaveDestructor = reinterpret_cast<SaveDestructorFn>(moduleBaseAddress + 0x14AB20);

        static SafetyHookMid captureSlotsHook = safetyhook::create_mid(reinterpret_cast<void*>(moduleBaseAddress + 0x1D0284), captureExtraSlotsDetour);
        static SafetyHookMid restorePlayerHook = safetyhook::create_mid(reinterpret_cast<void*>(moduleBaseAddress + 0x1D21B0), restoreExtraPlayerDetour);
        static SafetyHookMid restoreInventoryHook = safetyhook::create_mid(reinterpret_cast<void*>(moduleBaseAddress + 0x1D23F7), restoreExtraInventoryDetour);
        if (!captureSlotsHook || !restorePlayerHook || !restoreInventoryHook) return false;
        Utils::CreateHook(reinterpret_cast<void*>(moduleBaseAddress + 0x1D0180),
            &captureAllPlayersDetour, reinterpret_cast<void**>(&pOriginalCaptureSave));
        Utils::CreateHook(reinterpret_cast<void*>(moduleBaseAddress + 0x1D4A80),
            &saveAllPlayersDetour, reinterpret_cast<void**>(&pOriginalSaveFile));
        Utils::CreateHook(reinterpret_cast<void*>(moduleBaseAddress + 0x1D3400),
            &loadAllPlayersDetour, reinterpret_cast<void**>(&pOriginalLoadFile));
        Utils::CreateHook(reinterpret_cast<void*>(moduleBaseAddress + 0x1D1A10),
            &initializeAllPlayersDetour, reinterpret_cast<void**>(&pOriginalInitializeSave));
        return pOriginalCaptureSave && pOriginalSaveFile && pOriginalLoadFile && pOriginalInitializeSave;
    }

}
