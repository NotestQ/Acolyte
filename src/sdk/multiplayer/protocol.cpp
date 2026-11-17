#include <Acolyte/utils.h>
#include <Vagante/client.h>
#include <safetyhook.hpp>
#include "core.h"
#include "multiplayer.h"

static uintptr_t moduleBaseAddress = (uintptr_t)GetModuleHandleA(NULL);
// Vagante displays 1.10.1, but only sends 1.10 when joining a game
static std::string gameVersion = "1.10";
static std::string networkVersion = "1.10-acolyte-mp2";

class ISteamMatchmaking;
using NewGameFn = void(__thiscall*)(Client* clientThis, std::vector<PlayerClass> classes, std::vector<int> palettes);
using WriteBitsFn = void(__thiscall*)(Message* messageThis, int value, int bits, bool uncompressed);
using ReadBitsFn = int(__thiscall*)(Message* messageThis, int bits, bool uncompressed);
using SteamMatchmakingFn = ISteamMatchmaking*(__cdecl*)();
using GetLobbyDataFn = const char*(__cdecl*)(ISteamMatchmaking* matchmaking, ulong64 lobby, const char* key);

static NewGameFn NewGame = nullptr;
static WriteBitsFn WriteBits = nullptr;
static ReadBitsFn ReadBits = nullptr;
static SteamMatchmakingFn SteamMatchmaking = nullptr;
static GetLobbyDataFn GetLobbyData = nullptr;

static void SetNetworkVersion(SafetyHookContext& ctx) {
    // ESP + 4 is the string data, ESP + 8 is its length
    *reinterpret_cast<char**>(ctx.esp + 4) = networkVersion.data();
    *reinterpret_cast<uint*>(ctx.esp + 8) = networkVersion.size();
}

static bool CanBrowseVanillaLobbies() {
    return !modsLoaded && SteamMatchmaking && GetLobbyData;
}

static const char* GetLobbyVersion(CSteamID lobby) {
    if (!lobby.value) return "";
    auto matchmaking = SteamMatchmaking();
    return matchmaking ? GetLobbyData(matchmaking, lobby.value, "gameversion") : "";
}

void lobbyVersionDetour(SafetyHookContext& ctx) {
    if (!Multiplayer::vanillaHost)
        ctx.eax = reinterpret_cast<uintptr_t>(networkVersion.data());
}

void lobbyVersionFilterDetour(SafetyHookContext& ctx) {
    if (CanBrowseVanillaLobbies()) {
        // Don't filter out vanilla lobbies here, checkLobbyVersionDetour checks each lobby's version
        ctx.eip = moduleBaseAddress + 0x2BB9E3;
        return;
    }
    ctx.edx = reinterpret_cast<uintptr_t>(networkVersion.data());
}

void checkLobbyVersionDetour(SafetyHookContext& ctx) {
    if (!CanBrowseVanillaLobbies()) return;
    CSteamID* lobby = reinterpret_cast<CSteamID*>(ctx.ebp - 0xA8); // GetLobbyByIndex writes the ID here
    auto version = GetLobbyVersion(*lobby);
    if (gameVersion != version && networkVersion != version)
        ctx.eip = moduleBaseAddress + 0x2BC170;
}

void serverVersionDetour(SafetyHookContext& ctx) {
    if (!Multiplayer::vanillaHost) SetNetworkVersion(ctx);
}

void steamClientVersionDetour(SafetyHookContext& ctx) {
    Client* client = reinterpret_cast<Client*>(ctx.ebx);
    if (client->server)
        Multiplayer::vanillaClient = Multiplayer::vanillaHost.load();
    else if (client->connectAttempts == 1)
        Multiplayer::vanillaClient = CanBrowseVanillaLobbies() && gameVersion == GetLobbyVersion(client->steamIDLobby);

    if (!Multiplayer::vanillaClient) SetNetworkVersion(ctx);
}

void beginIpConnectionDetour(SafetyHookContext& ctx) {
    Client* client = reinterpret_cast<Client*>(ctx.esi);
    // Reset vanillaClient here, initiateServerConnectionUsingIp can fail before ipClientVersionDetour runs
    if (client->connectAttempts == 0)
        Multiplayer::vanillaClient = client->server && Multiplayer::vanillaHost.load();
}

void ipClientVersionDetour(SafetyHookContext& ctx) {
    if (!Multiplayer::vanillaClient) SetNetworkVersion(ctx);
}

void retryVanillaConnectionDetour(SafetyHookContext& ctx) {
    Client* client = reinterpret_cast<Client*>(ctx.edi);
    if (modsLoaded || client->server) return;
    if (client->netState != CLIENT_CONNECTING_SERVER_IP && client->netState != CLIENT_CONNECTING_SERVER_STEAM) return;
    // IP joins don't have lobby data, check the server's version in SV_CONNECT_REJECT
    auto& reason = *reinterpret_cast<std::string*>(ctx.ebp - 0x428);
    static std::string mismatch = "Game version mismatch. Server version: " + gameVersion + " vs. Client: " + networkVersion;
    if (reason != mismatch) return;

    if (!Multiplayer::vanillaClient) {
        Multiplayer::vanillaClient = true;
        client->connectAttempts = 1;
        client->netLoadingFrames = 0;
        client->connectAttemptDelay = 0;
    }
    // Don't disconnect here, Client::processNetworkLogic will retry with the vanilla version
    ctx.eip = moduleBaseAddress + 0x15AF5A;
}

void sendPlayerListLoopDetour(SafetyHookContext& ctx) {
    if (!Multiplayer::vanillaHost && ctx.ebx < Multiplayer::playerSlots * sizeof(ClientConnectionData*))
        ctx.eip = moduleBaseAddress + 0x365550;
}

void findPlayerSlotDetour(SafetyHookContext& ctx) {
    if (!Multiplayer::vanillaHost && ctx.ecx < Multiplayer::playerSlots)
        ctx.eip = moduleBaseAddress + 0x36C120;
}

void readPlayerListLoopDetour(SafetyHookContext& ctx) {
    if (ctx.esi >= Multiplayer::playerSlots * sizeof(PlayerInformation)) return;
    if (Multiplayer::vanillaClient && ctx.esi >= 4 * sizeof(PlayerInformation)) {
        // Vanilla sends 4 slots, use Client::onReceivePlayerList's empty slot branch to clear the rest
        Client* client = reinterpret_cast<Client*>(ctx.edi);
        ctx.eax = reinterpret_cast<uintptr_t>(client->playerInformation.data());
        ctx.eip = moduleBaseAddress + 0x14E725;
        return;
    }
    ctx.eip = moduleBaseAddress + 0x14E690;
}

// Item events and death/victory menus include empty slots too, vanilla only sends 4
void sendPlayerCountDetour(SafetyHookContext& ctx) {
    if (Multiplayer::vanillaHost && ctx.eax > 4) ctx.eax = 4;
}

void readPlayerCountDetour(SafetyHookContext& ctx) {
    if (Multiplayer::vanillaClient && ctx.eax > 4) ctx.eax = 4;
}

void finishJoinResetDetour(SafetyHookContext& ctx) {
    Client* client = reinterpret_cast<Client*>(ctx.ebx);
    if (!client->gameEngine.doNewGame) return;

    // onReceiveServerAuthenticationResponse sets doNewGame, but the level can arrive in the same frame
    // Run newGame first or Client::step will delete the level we just received
    auto& customizeMenu = client->gameEngine.playerCustomizeMenu;
    NewGame(client, customizeMenu.playerClasses, customizeMenu.playerPalettes);
}

void sendPaletteChoiceDetour(SafetyHookContext& ctx) {
    if (Multiplayer::vanillaClient) return;

    // sendInputToServer sends the class, background and affinities, but not the palette
    PlayerMenu* menu = reinterpret_cast<PlayerMenu*>(ctx.ebx);
    Message* message = reinterpret_cast<Message*>(ctx.ebp - 0x5C);
    WriteBits(message, menu->playerPalette, 32, false);
}

void readPaletteChoiceDetour(SafetyHookContext& ctx) {
    if (Multiplayer::vanillaHost) return;

    Message* message = *reinterpret_cast<Message**>(ctx.ebp + 0x14);
    int palette = ReadBits(message, 32, false);
    if (palette < -1 || palette > 3)
        palette = -1; // addPlayer picks a random palette for -1

    Server* server = reinterpret_cast<Server*>(ctx.edi);
    uint slot = *reinterpret_cast<uint*>(ctx.ebp - 0x18);
    server->gameEngine.playerMenus[slot]->playerPalette = palette;
}

namespace Multiplayer {
    std::atomic<bool> vanillaHost = false;
    bool vanillaClient = false;

    void InitProtocol() {
        NewGame = reinterpret_cast<NewGameFn>(moduleBaseAddress + 0x150190);
        WriteBits = reinterpret_cast<WriteBitsFn>(moduleBaseAddress + 0x2AF5E0);
        ReadBits = reinterpret_cast<ReadBitsFn>(moduleBaseAddress + 0x2AF840);

        auto steam = GetModuleHandleA("steam_api.dll");
        if (steam) {
            SteamMatchmaking = reinterpret_cast<SteamMatchmakingFn>(GetProcAddress(steam, "SteamAPI_SteamMatchmaking_v009"));
            GetLobbyData = reinterpret_cast<GetLobbyDataFn>(GetProcAddress(steam, "SteamAPI_ISteamMatchmaking_GetLobbyData"));
        }

        static SafetyHookMid lobbyVersionHook = Utils::CreateMidHook(reinterpret_cast<void*>(moduleBaseAddress + 0x36AEA9), lobbyVersionDetour);
        static SafetyHookMid lobbyVersionFilterHook = Utils::CreateMidHook(reinterpret_cast<void*>(moduleBaseAddress + 0x2BB9D6), lobbyVersionFilterDetour);
        static SafetyHookMid lobbyResultHook = Utils::CreateMidHook(reinterpret_cast<void*>(moduleBaseAddress + 0x2BBB9D), checkLobbyVersionDetour);

        static SafetyHookMid serverVersionHook = Utils::CreateMidHook(reinterpret_cast<void*>(moduleBaseAddress + 0x36377E), serverVersionDetour);
        static SafetyHookMid connectionVersionHook = Utils::CreateMidHook(reinterpret_cast<void*>(moduleBaseAddress + 0x3695DB), serverVersionDetour);
        static SafetyHookMid steamVersionHook = Utils::CreateMidHook(reinterpret_cast<void*>(moduleBaseAddress + 0x14D276), steamClientVersionDetour);
        static SafetyHookMid beginIpConnectionHook = Utils::CreateMidHook(reinterpret_cast<void*>(moduleBaseAddress + 0x14CB1F), beginIpConnectionDetour);
        static SafetyHookMid ipVersionHook = Utils::CreateMidHook(reinterpret_cast<void*>(moduleBaseAddress + 0x14C978), ipClientVersionDetour);
        static SafetyHookMid rejectVersionHook = Utils::CreateMidHook(reinterpret_cast<void*>(moduleBaseAddress + 0x15ACF6), retryVanillaConnectionDetour);

        static SafetyHookMid sendPlayerListHook = Utils::CreateMidHook(reinterpret_cast<void*>(moduleBaseAddress + 0x36560B), sendPlayerListLoopDetour);
        static SafetyHookMid findPlayerSlotHook = Utils::CreateMidHook(reinterpret_cast<void*>(moduleBaseAddress + 0x36C149), findPlayerSlotDetour);
        static SafetyHookMid readPlayerListHook = Utils::CreateMidHook(reinterpret_cast<void*>(moduleBaseAddress + 0x14E7A8), readPlayerListLoopDetour);
        static SafetyHookMid sendVictoryMenusHook = Utils::CreateMidHook(reinterpret_cast<void*>(moduleBaseAddress + 0x366439), sendPlayerCountDetour);
        static SafetyHookMid sendDeathMenusHook = Utils::CreateMidHook(reinterpret_cast<void*>(moduleBaseAddress + 0x366567), sendPlayerCountDetour);
        static SafetyHookMid readVictoryMenusHook = Utils::CreateMidHook(reinterpret_cast<void*>(moduleBaseAddress + 0x158D51), readPlayerCountDetour);
        static SafetyHookMid readDeathMenusHook = Utils::CreateMidHook(reinterpret_cast<void*>(moduleBaseAddress + 0x158EDF), readPlayerCountDetour);
        static SafetyHookMid sendItemEventsHook = Utils::CreateMidHook(reinterpret_cast<void*>(moduleBaseAddress + 0x3677A2), sendPlayerCountDetour);
        static SafetyHookMid readItemEventsHook = Utils::CreateMidHook(reinterpret_cast<void*>(moduleBaseAddress + 0x1589E6), readPlayerCountDetour);
        static SafetyHookMid joinResetHook = Utils::CreateMidHook(reinterpret_cast<void*>(moduleBaseAddress + 0x14DF2D), finishJoinResetDetour);
        static SafetyHookMid sendPaletteChoiceHook = Utils::CreateMidHook(reinterpret_cast<void*>(moduleBaseAddress + 0x15C8C2), sendPaletteChoiceDetour);
        static SafetyHookMid readPaletteChoiceHook = Utils::CreateMidHook(reinterpret_cast<void*>(moduleBaseAddress + 0x36CE12), readPaletteChoiceDetour);
    }
}
