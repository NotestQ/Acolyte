#pragma once
#include <atomic>

namespace Multiplayer {
    // Saves work in groups of four, so leave one spare slot for the 99 player limit
    constexpr unsigned playerSlots = 100;
    constexpr unsigned maxPlayers = 99;
    extern std::atomic<bool> vanillaHost;
    extern bool vanillaClient;

    bool InitMultiplayer();
    void InitGameplay();
    bool InitPalettes();
    bool InitSaves();
    void InitStorage();
    void InitProtocol();
    bool ApplyPlayerLimits();
    void FailInitialization();
}
