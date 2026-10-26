#include <Acolyte/lobby.h>
#include <Acolyte/playerclass.h>
#include <Acolyte/ui.h>
#include <Acolyte/utils.h>
#include <Vagante/client.h>
#include <sdk_registry.h>
#include <safetyhook.hpp>
#include <algorithm>
#include <atomic>
#include <cstdint>
#include "multiplayer/multiplayer.h"

static uintptr_t moduleBaseAddress = (uintptr_t)GetModuleHandleA(NULL);
static int supportedPlayers = 4;
static int maxPlayers = 4;
static bool duplicatePalettes = false;
static std::atomic<int> hostedMaxPlayers = 4;
static NetMenu* hostingMenu;
static GuiButton* maxPlayersButton;
static GuiButton* duplicatePalettesButton;

using RefreshFn = void(__thiscall*)(NetMenu* menuThis, bool force);
using MenuActionFn = void(__thiscall*)(NetMenu* menuThis, bool down);
using MenuUpdateFn = void(__thiscall*)(NetMenu* menuThis);

using LobbyJoinedFn = void(__thiscall*)(Client* clientThis, LobbyEnter_t* result, bool failed);
static LobbyJoinedFn LobbyJoined = nullptr;

static std::string& GetNetworkVersion() {
    // Keep the game's own version for saves and use a separate multiplayer version.
    static std::string version = *reinterpret_cast<std::string*>(moduleBaseAddress + 0x780BD0) + "-acolyte-mp1";
    return version;
}

void networkVersionDetour(SafetyHookContext& ctx) {
    auto& version = GetNetworkVersion();
    // The stream insertion already has its text and length on the stack.
    reinterpret_cast<uintptr_t*>(ctx.esp)[1] = reinterpret_cast<uintptr_t>(version.data());
    reinterpret_cast<uint*>(ctx.esp)[2] = version.size();
}

void lobbyVersionDetour(SafetyHookContext& ctx) {
    ctx.eax = reinterpret_cast<uintptr_t>(GetNetworkVersion().data());
}

void lobbyVersionFilterDetour(SafetyHookContext& ctx) {
    ctx.edx = reinterpret_cast<uintptr_t>(GetNetworkVersion().data());
}

static void UpdateMaxPlayersText() {
    if (maxPlayersButton)
        sdk::UI::SetText(maxPlayersButton, "Max players: " + std::to_string(maxPlayers));
    if (duplicatePalettesButton)
        sdk::UI::SetText(duplicatePalettesButton, duplicatePalettes ? "Duplicates: ON" : "Duplicates: OFF");
}

static void ChangeMaxPlayers(int direction) {
    int players = maxPlayers + direction;
    int limit = sdk::Lobby::GetPlayerLimit();
    if (players > limit) players = 1;
    if (players < 1) players = limit;
    sdk::Lobby::SetMaxPlayers(players);
}

static void OnButtonPressed(GuiButton* button) {
    if (button == maxPlayersButton && !button->disabled && !button->hidden)
        ChangeMaxPlayers(button->rightClick ? -1 : 1);
    if (button == duplicatePalettesButton && !button->disabled && !button->hidden)
        sdk::Lobby::SetDuplicatePalettes(!duplicatePalettes);
}

RefreshFn pOriginalRefresh = nullptr;
void __fastcall refreshDetour(NetMenu* menuThis, void*, bool force) {
    pOriginalRefresh(menuThis, force);
    if (!maxPlayersButton) {
        hostingMenu = menuThis;
        maxPlayersButton = sdk::UI::CreateButton(400, 268, 103, 16, "");
        duplicatePalettesButton = sdk::UI::CreateButton(400, 286, 103, 16, "");
        sdk::UI::SetTooltip(maxPlayersButton, "Maximum players, including the host. Left/right to change.");
        sdk::UI::SetTooltip(duplicatePalettesButton, "Allow up to 99 players. Duplicate class/palette pairs receive a random Overlay tint.");
        UpdateMaxPlayersText();
    }
    auto& elements = menuThis->guiElements;
    if (std::find(elements.begin(), elements.end(), maxPlayersButton) != elements.end()) return;

    size_t index = std::find(elements.begin(), elements.end(), menuThis->hostSteamGameButton) - elements.begin();
    sdk::UI::AddElement(elements, maxPlayersButton);
    sdk::UI::AddElement(elements, duplicatePalettesButton);
    std::rotate(elements.begin() + index, elements.end() - 2, elements.end());

    GuiElement* rows[] = {
        menuThis->gameNameField, menuThis->privateGameCheckbox, menuThis->customGameCheckbox,
        maxPlayersButton, duplicatePalettesButton, menuThis->hostSteamGameButton, menuThis->hostGameButton, menuThis->connectToIpButton,
        menuThis->refreshButton, menuThis->backButton
    };
    for (int i = 0; i < 10; ++i)
        rows[i]->y = 196 + i * 18;
}

static bool HostingOptionSelected(NetMenu* menu) {
    return menu == hostingMenu && menu->selectPos.x == 1 && (menu->selectPos.y == 13 || menu->selectPos.y == 14) &&
        !menu->loading && !menu->selectingCustomWorld && menu->animateInTimer == 0;
}

MenuActionFn pOriginalAction = nullptr;
void __fastcall actionDetour(NetMenu* menuThis, void*, bool down) {
    if (HostingOptionSelected(menuThis)) {
        auto button = menuThis->selectPos.y == 13 ? maxPlayersButton : duplicatePalettesButton;
        button->rightClick = false;
        sdk::UI::Action(button, down);
        return;
    }
    // The game's switch still uses the old row numbers, subtract our two rows before calling it
    int row = menuThis->selectPos.y;
    if (menuThis == hostingMenu && menuThis->selectPos.x == 1 && row > 14)
        menuThis->selectPos.y -= 2;
    pOriginalAction(menuThis, down);
    menuThis->selectPos.y = row;
}

MenuActionFn pOriginalLeft = nullptr;
void __fastcall leftDetour(NetMenu* menuThis, void*, bool repeat) {
    if (HostingOptionSelected(menuThis)) {
        if (menuThis->selectPos.y == 13)
            ChangeMaxPlayers(-1);
        else if (!repeat)
            sdk::Lobby::SetDuplicatePalettes(!duplicatePalettes);
        return;
    }
    pOriginalLeft(menuThis, repeat);
}

MenuActionFn pOriginalRight = nullptr;
void __fastcall rightDetour(NetMenu* menuThis, void*, bool repeat) {
    if (HostingOptionSelected(menuThis)) {
        if (menuThis->selectPos.y == 13)
            ChangeMaxPlayers(1);
        else if (!repeat)
            sdk::Lobby::SetDuplicatePalettes(!duplicatePalettes);
        return;
    }
    pOriginalRight(menuThis, repeat);
}

MenuUpdateFn pOriginalStepGuiEnabled = nullptr;
void __fastcall stepGuiEnabledDetour(NetMenu* menuThis, void*) {
    pOriginalStepGuiEnabled(menuThis);
    if (menuThis == hostingMenu) {
        maxPlayersButton->disabled = menuThis->loading || menuThis->selectingCustomWorld;
        duplicatePalettesButton->disabled = maxPlayersButton->disabled || supportedPlayers <= 4;
    }
}

MenuUpdateFn pOriginalDestroyMenu = nullptr;
void __fastcall destroyMenuDetour(NetMenu* menuThis, void*) {
    if (menuThis == hostingMenu) {
        // The menu deletes these buttons with its other elements
        maxPlayersButton = nullptr;
        duplicatePalettesButton = nullptr;
        hostingMenu = nullptr;
    }
    pOriginalDestroyMenu(menuThis);
}

void startServerDetour(SafetyHookContext&) {
    hostedMaxPlayers.store((std::min)(maxPlayers, sdk::Lobby::GetPlayerLimit()));
}

void createLobbyDetour(SafetyHookContext& ctx) {
    // ESP + 4 is the maximum member count passed to CreateLobby.
    reinterpret_cast<int*>(ctx.esp)[1] = hostedMaxPlayers.load();
}

void setLobbyMemberLimitDetour(SafetyHookContext& ctx) {
    // The member limit is already on the stack. The Steam ID is pushed next.
    *reinterpret_cast<int*>(ctx.esp) = hostedMaxPlayers.load();
}

void connectHostLobbyDetour(SafetyHookContext& ctx) {
    Client* client = reinterpret_cast<Client*>(ctx.esi);
    Server* server = client->server;
    auto lobby = *reinterpret_cast<uint64_t*>(ctx.ebp + 8);
    if (!server || !lobby || server->state != STATE_RUNNING || server->steamIDLobby.value != lobby) return;

    // CreateLobby already joined the host. Joining again fails for a one-player lobby.
    LobbyEnter_t result{lobby, 0, false, 1}; // k_EChatRoomEnterResponseSuccess
    LobbyJoined(client, &result, false);
    // Retry through processNetworkLogic if the server metadata hasn't arrived yet.
    if (client->netState == CLIENT_CONNECTING_LOBBY) client->netState = CLIENT_INITIALIZING;
    ctx.eip = moduleBaseAddress + 0x14D0AA;
}

void checkSteamAdmissionDetour(SafetyHookContext& ctx) {
    // Continue to the game's accepted or full-server response.
    ctx.eip = moduleBaseAddress + (ctx.ecx < static_cast<uint>(hostedMaxPlayers.load()) ? 0x36B139 : 0x36B0F7);
}

void checkIpAdmissionDetour(SafetyHookContext& ctx) {
    ctx.eip = moduleBaseAddress + (ctx.ecx < static_cast<uint>(hostedMaxPlayers.load()) ? 0x36B87E : 0x36B7CD);
}

namespace sdk::Lobby {
    int GetMaxPlayers() {
        return maxPlayers;
    }

    int GetSupportedPlayers() {
        return supportedPlayers;
    }

    int GetPlayerLimit() {
        return (std::min)(supportedPlayers, duplicatePalettes ? static_cast<int>(Multiplayer::maxPlayers) : sdk::PlayerClass::GetPlayableClassCount() * 4);
    }

    bool GetDuplicatePalettes() {
        return duplicatePalettes;
    }

    void SetDuplicatePalettes(bool enabled) {
        duplicatePalettes = enabled;
        SetMaxPlayers(maxPlayers);
    }

    void SetMaxPlayers(int players) {
        maxPlayers = std::clamp(players, 1, GetPlayerLimit());
        UpdateMaxPlayersText();
    }

    void InitLobby() {
        LobbyJoined = reinterpret_cast<LobbyJoinedFn>(moduleBaseAddress + 0x14CC50);
        if (!Multiplayer::InitMultiplayer()) Multiplayer::FailInitialization();
        sdk::UI::OnButtonPressed.connect(&OnButtonPressed);
        Utils::CreateHook(reinterpret_cast<void*>(moduleBaseAddress + 0x2BB5F0),
            &refreshDetour, reinterpret_cast<void**>(&pOriginalRefresh));
        Utils::CreateHook(reinterpret_cast<void*>(moduleBaseAddress + 0x2B9080),
            &actionDetour, reinterpret_cast<void**>(&pOriginalAction));
        Utils::CreateHook(reinterpret_cast<void*>(moduleBaseAddress + 0x2B9000),
            &leftDetour, reinterpret_cast<void**>(&pOriginalLeft));
        Utils::CreateHook(reinterpret_cast<void*>(moduleBaseAddress + 0x2B9040),
            &rightDetour, reinterpret_cast<void**>(&pOriginalRight));
        Utils::CreateHook(reinterpret_cast<void*>(moduleBaseAddress + 0x2BA620),
            &stepGuiEnabledDetour, reinterpret_cast<void**>(&pOriginalStepGuiEnabled));
        Utils::CreateHook(reinterpret_cast<void*>(moduleBaseAddress + 0x2B7ED0),
            &destroyMenuDetour, reinterpret_cast<void**>(&pOriginalDestroyMenu));

        static SafetyHookMid startServerHook = safetyhook::create_mid(reinterpret_cast<void*>(moduleBaseAddress + 0x363680), startServerDetour);
        static SafetyHookMid createLobbyHook = safetyhook::create_mid(reinterpret_cast<void*>(moduleBaseAddress + 0x36A6F6), createLobbyDetour);
        static SafetyHookMid lobbyMemberLimitHook = safetyhook::create_mid(reinterpret_cast<void*>(moduleBaseAddress + 0x36AF35), setLobbyMemberLimitDetour);
        static SafetyHookMid hostLobbyConnectionHook = safetyhook::create_mid(reinterpret_cast<void*>(moduleBaseAddress + 0x14CFD4), connectHostLobbyDetour);
        static SafetyHookMid steamAdmissionHook = safetyhook::create_mid(reinterpret_cast<void*>(moduleBaseAddress + 0x36B0F2), checkSteamAdmissionDetour);
        static SafetyHookMid ipAdmissionHook = safetyhook::create_mid(reinterpret_cast<void*>(moduleBaseAddress + 0x36B7C4), checkIpAdmissionDetour);
        // Vanilla uses the same gameversion filter and rejects a different connection version.
        static SafetyHookMid lobbyVersionHook = safetyhook::create_mid(reinterpret_cast<void*>(moduleBaseAddress + 0x36AEA9), lobbyVersionDetour);
        static SafetyHookMid lobbyVersionFilterHook = safetyhook::create_mid(reinterpret_cast<void*>(moduleBaseAddress + 0x2BB9D6), lobbyVersionFilterDetour);
        static SafetyHookMid steamVersionHook = safetyhook::create_mid(reinterpret_cast<void*>(moduleBaseAddress + 0x14D276), networkVersionDetour);
        static SafetyHookMid ipVersionHook = safetyhook::create_mid(reinterpret_cast<void*>(moduleBaseAddress + 0x14C978), networkVersionDetour);
        static SafetyHookMid serverVersionHook = safetyhook::create_mid(reinterpret_cast<void*>(moduleBaseAddress + 0x36377E), networkVersionDetour);
        static SafetyHookMid connectionVersionHook = safetyhook::create_mid(reinterpret_cast<void*>(moduleBaseAddress + 0x3695DB), networkVersionDetour);

        if (!pOriginalRefresh || !pOriginalAction || !pOriginalLeft || !pOriginalRight ||
            !pOriginalStepGuiEnabled || !pOriginalDestroyMenu || !startServerHook || !createLobbyHook ||
            !lobbyMemberLimitHook || !hostLobbyConnectionHook || !steamAdmissionHook || !ipAdmissionHook ||
            !lobbyVersionHook || !lobbyVersionFilterHook || !steamVersionHook || !ipVersionHook ||
            !serverVersionHook || !connectionVersionHook)
            Multiplayer::FailInitialization();

        // All required hooks must be ready before the game starts using the larger arrays.
        if (!Multiplayer::ApplyPlayerLimits() || !Utils::PatchByteAt(moduleBaseAddress + 0x2BAD35, 2, 19))
            Multiplayer::FailInitialization();
        supportedPlayers = Multiplayer::maxPlayers;
    }
    SDK_REGISTER(InitLobby);
}
