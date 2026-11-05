#include <Acolyte/logger.h>
#include <Acolyte/playerclass.h>
#include <Acolyte/utils.h>
#include <Acolyte/lobby.h>
#include <safetyhook.hpp>
#include <cstdint>
#include <memory>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <Vagante/client.h>
#include "multiplayer.h"

static uintptr_t moduleBaseAddress = (uintptr_t)GetModuleHandleA(NULL);
static Logger multiplayerLogger("Acolyte");

using UpdateMenuFn = void(__thiscall*)(PlayerMenu* menuThis);
using DrawHudFn = void(__thiscall*)(PlayerMenu* menuThis, sf::RenderTarget* renderTarget);
using RandomPaletteFn = int(__thiscall*)(PlayerCustomizeMenu* customizeThis, unsigned slot, int character);

static sdk::PlayerClass::GetPlayerClassFn GetPlayerClass = nullptr;
static UpdateMenuFn UpdateClassBackgrounds = nullptr;
static UpdateMenuFn UpdateClassPalettes = nullptr;

static unsigned __cdecl CountConnections(Server* server) {
    unsigned count = 0;
    for (unsigned i = 0; i < Multiplayer::playerSlots; ++i) {
        count += server->clientData[i]->active;
        count += server->pendingClientData[i]->active;
    }
    return count;
}

void countSteamConnectionsDetour(SafetyHookContext& ctx) {
    Server* server = reinterpret_cast<Server*>(ctx.ebx);
    ctx.ecx = CountConnections(server);
    ctx.esi = reinterpret_cast<uintptr_t>(server->clientData.front());
    ctx.eip = moduleBaseAddress + 0x36B0F2;
}

void countIpConnectionsDetour(SafetyHookContext& ctx) {
    // Skip the duplicate IP check so two players can join from the same network
    Server* server = reinterpret_cast<Server*>(ctx.edi);
    ctx.ecx = CountConnections(server);
    ctx.esi = reinterpret_cast<uintptr_t>(server->clientData.data());
    ctx.eip = moduleBaseAddress + 0x36B7C4;
}

void finishInputResetDetour(SafetyHookContext& ctx) {
    Input* input = reinterpret_cast<Input*>(ctx.ebx);
    for (unsigned i = 4; i < Multiplayer::playerSlots; ++i) {
        auto& controller = input->controllers[i];
        controller.slot = i;
        controller.enabled = false; // Server::onReceiveInput enables this when the player joins
        controller.binding = false;
    }
}

// Input configs only contain the four local controllers, the extra slots are for network input
void boundInputConfigControllersDetour(SafetyHookContext& ctx) {
    if (ctx.eax > 4) ctx.eax = 4;
}

// Players 5 and up use controller 0 on their own client
void boundLocalControllerDetour(SafetyHookContext& ctx) {
    Client* client = reinterpret_cast<Client*>(ctx.edi);
    if (ctx.ecx >= 4 || ctx.ecx >= client->input->controllers.size())
        ctx.eip = moduleBaseAddress + 0x151543;
}

void initializeMenuAppearanceDetour(SafetyHookContext& ctx) {
    PlayerMenu* menu = reinterpret_cast<PlayerMenu*>(ctx.esi);
    // Only the five base classes are available until GetPlayerClass adds the modded ones
    menu->playerClass = (menu->playerIndex / 4) % 5;
    menu->playerPalette = menu->playerIndex % 4;
}

static unsigned GetUsedPalettes(std::vector<std::shared_ptr<Player>>& players, int character, unsigned ignoredSlot = Multiplayer::playerSlots) {
    unsigned used = 0;
    for (auto& player : players) {
        if (!player || player->playerIndex == ignoredSlot || player->playerClass != character) continue;
        if (player->palette >= 0 && player->palette < 4) used |= 1u << player->palette;
    }
    return used;
}

static int ChooseJoinAppearance(PlayerMenu* menu, int* randomizedClass = nullptr) {
    int menuClass = GetPlayerClass(menu, nullptr);
    int currentClass = randomizedClass ? *randomizedClass : menuClass;
    int palette = randomizedClass ? -1 : menu->playerPalette;
    if (sdk::Lobby::GetDuplicatePalettes()) return palette < 0 ? palette : palette % 4;
    if (currentClass == CLASS_RANDOM) return palette;

    auto& classes = menu->availableClasses;
    if (classes.empty()) return palette;
    unsigned classCount = classes.size();
    unsigned firstClass = static_cast<unsigned>(menu->playerClass) % classCount;
    if (randomizedClass) {
        auto found = std::find(classes.begin(), classes.end(), currentClass);
        firstClass = found == classes.end() ? 0 : static_cast<unsigned>(found - classes.begin());
    }
    auto& players = *menu->playerCustomizeMenu->players;

    for (unsigned step = 0; step < classCount; ++step) {
        auto index = (firstClass + step) % classCount;
        int character = classes[index];
        if (character == CLASS_RANDOM) continue;
        unsigned used = GetUsedPalettes(players, character);
        if (used == 0xF) continue;
        // A negative palette means random
        if (step == 0 && palette < 0) return palette;
        unsigned candidate = step == 0 ? static_cast<unsigned>(palette) % 4 : 0;
        while (used & (1u << candidate)) candidate = (candidate + 1) % 4;
        palette = static_cast<int>(candidate);
        if (randomizedClass) *randomizedClass = character;
        else {
            menu->playerPalette = palette;
            if (step != 0) {
                menu->playerClass = index;
                menu->updateTextContainer = true;
                UpdateClassBackgrounds(menu);
                UpdateClassPalettes(menu);
            }
        }
        return palette;
    }
    return palette < 0 ? palette : palette % 4;
}

RandomPaletteFn pOriginalRandomPalette = nullptr;
int __fastcall selectRandomPaletteDetour(PlayerCustomizeMenu* customizeThis, void*, unsigned slot, int character) {
    if (sdk::Lobby::GetDuplicatePalettes()) return std::rand() % 4;
    auto& players = *customizeThis->players;
    unsigned used = GetUsedPalettes(players, character, slot);
    // If all four palettes are taken just pick one, randomPalette would divide by zero
    return used == 15 ? std::rand() % 4 : pOriginalRandomPalette(customizeThis, slot, character);
}

void selectPaletteDetour(SafetyHookContext& ctx) {
    // ECX becomes the player count, zero skips the duplicate check
    if (sdk::Lobby::GetDuplicatePalettes()) ctx.ecx = 0;
}

void selectRandomClassDetour(SafetyHookContext& ctx) {
    PlayerCustomizeMenu* customize = *reinterpret_cast<PlayerCustomizeMenu**>(ctx.ebp - 0x14);
    unsigned slot = *reinterpret_cast<unsigned*>(ctx.ebp + 8);
    auto& menus = *customize->guis;
    if (slot < menus.size() && menus[slot])
        ChooseJoinAppearance(menus[slot].get(), reinterpret_cast<int*>(ctx.ebp - 0x1C));
}

// The game gives up after four palettes, try the next class if they're all taken
void selectJoinAppearanceDetour(SafetyHookContext& ctx) {
    ctx.edi = ChooseJoinAppearance(reinterpret_cast<PlayerMenu*>(ctx.ebx)); // addPlayer reads the palette from EDI
    *reinterpret_cast<unsigned*>(ctx.ebp - 0x10) = 0; // Reused for the background later
    ctx.eip = moduleBaseAddress + 0x3091E8;
}

DrawHudFn pOriginalDrawHud = nullptr;
void __fastcall selectVisibleHudDetour(PlayerMenu* menuThis, void*, sf::RenderTarget* renderTarget) {
    auto playerIndex = menuThis->playerIndex;
    auto localPlayer = static_cast<unsigned>(menuThis->networkedPlayerIndex);
    if (menuThis->networked) {
        // Players 5 and up see their own HUD in player 1's place
        if (playerIndex >= 4 && playerIndex != localPlayer) return;
        if (playerIndex == 0 && localPlayer >= 4) return;
    } else if (playerIndex >= 4) {
        return;
    }
    pOriginalDrawHud(menuThis, renderTarget);
}

// Only change the HUD position, inventory and customization still need playerIndex
void positionExtraHudDetour(SafetyHookContext& ctx) {
    if (ctx.eax >= 4) ctx.eax = 0;
}

// The game reads graves[deadCount] even when the player is alive
void boundGraveAccessDetour(SafetyHookContext& ctx) {
    GameEngine* engine = reinterpret_cast<GameEngine*>(ctx.ebx);
    if (ctx.edx >= engine->graves.size()) ctx.eip = moduleBaseAddress + 0x1C91E8;
}

void finishGravesDetour(SafetyHookContext& ctx) {
    GameEngine* engine = reinterpret_cast<GameEngine*>(ctx.ebx);
    for (unsigned i = ctx.edx; i < engine->graves.size(); ++i) {
        Grave* grave = engine->graves[i].get();
        grave->visible = false;
        grave->palpable = false;
    }
    ctx.eip = moduleBaseAddress + 0x1C91E8;
}

namespace Multiplayer {
    bool ApplyPlayerLimits() {
        int success = 1;
        unsigned connectionPointerBytes = playerSlots * sizeof(ClientConnectionData*);
        unsigned sharedMenuBytes = playerSlots * sizeof(std::shared_ptr<PlayerMenu>);
        unsigned runStatsBytes = playerSlots * sizeof(std::map<std::string, int>);
        unsigned controlSchemeBytes = playerSlots * sizeof(ControlScheme);
        unsigned playerInformationBytes = playerSlots * sizeof(PlayerInformation);
        unsigned healingRecordBytes = playerSlots * sizeof(Bonfire::HealingPlayerState);
        unsigned triggerCountBytes = playerSlots * sizeof(int);
        unsigned lastPlayerIndex = playerSlots - 1;

        // Server::receiveNetworkData, includes the index used when no connection is found
        success &= Utils::PatchByteAt(moduleBaseAddress + 0x369298, 2, playerSlots);
        success &= Utils::PatchByteAt(moduleBaseAddress + 0x3692C6, 2, playerSlots);
        success &= Utils::PatchByteAt(moduleBaseAddress + 0x36940E, 2, playerSlots);

        // Server::start: clientData and pendingClientData sizes and construction
        success &= Utils::PatchByteAt(moduleBaseAddress + 0x363B6E, 2, playerSlots);
        success &= Utils::PatchByteAt(moduleBaseAddress + 0x363B81, 1, playerSlots);
        success &= Utils::PatchByteAt(moduleBaseAddress + 0x363B9D, 2, playerSlots);
        success &= Utils::PatchByteAt(moduleBaseAddress + 0x363BB1, 2, playerSlots);

        // Server::start: pending vector fill
        success &= Utils::PatchMemory(reinterpret_cast<void*>(moduleBaseAddress + 0x363BC4 + 1), &playerSlots, sizeof(playerSlots));

        // clientData and pendingClientData reallocation
        success &= Utils::PatchByteAt(moduleBaseAddress + 0x3721C2, 2, playerSlots);
        success &= Utils::PatchMemory(reinterpret_cast<void*>(moduleBaseAddress + 0x3721C7 + 1), &connectionPointerBytes, sizeof(connectionPointerBytes));
        success &= Utils::PatchMemory(reinterpret_cast<void*>(moduleBaseAddress + 0x37222D + 1), &playerSlots, sizeof(playerSlots));

        // Server: new game, level transition, disconnect, authentication and broadcasts
        success &= Utils::PatchByteAt(moduleBaseAddress + 0x3640EF, 2, playerSlots);
        success &= Utils::PatchByteAt(moduleBaseAddress + 0x3643AC, 2, playerSlots);
        success &= Utils::PatchByteAt(moduleBaseAddress + 0x36524A, 2, playerSlots);
        success &= Utils::PatchByteAt(moduleBaseAddress + 0x362D5A, 2, playerSlots);
        success &= Utils::PatchByteAt(moduleBaseAddress + 0x36B3EF, 2, playerSlots);
        success &= Utils::PatchByteAt(moduleBaseAddress + 0x36BC6A, 2, playerSlots);
        success &= Utils::PatchByteAt(moduleBaseAddress + 0x36C149, 2, playerSlots);
        success &= Utils::PatchByteAt(moduleBaseAddress + 0x36C75A, 2, playerSlots);
        success &= Utils::PatchByteAt(moduleBaseAddress + 0x36EF68, 2, playerSlots);
        success &= Utils::PatchByteAt(moduleBaseAddress + 0x3701AC, 2, playerSlots);
        success &= Utils::PatchByteAt(moduleBaseAddress + 0x370EBA, 2, playerSlots);
        success &= Utils::PatchByteAt(moduleBaseAddress + 0x3710D2, 2, playerSlots);

        // Inventory events need an acknowledgement bit for every slot, including disconnected players
        success &= Utils::PatchByteAt(moduleBaseAddress + 0x208B9D, 1, playerSlots);

        // GameEngine::GameEngine: PlayerMenu vector
        success &= Utils::PatchByteAt(moduleBaseAddress + 0x1C0CC9, 2, playerSlots);
        success &= Utils::PatchByteAt(moduleBaseAddress + 0x1C0CF4, 2, playerSlots);
        success &= Utils::PatchMemory(reinterpret_cast<void*>(moduleBaseAddress + 0x1C0D08 + 1), &playerSlots, sizeof(playerSlots));

        // PlayerMenu vector reallocation
        success &= Utils::PatchByteAt(moduleBaseAddress + 0x1CC1B7, 2, playerSlots);
        success &= Utils::PatchMemory(reinterpret_cast<void*>(moduleBaseAddress + 0x1CC1BC + 1), &sharedMenuBytes, sizeof(sharedMenuBytes));
        success &= Utils::PatchMemory(reinterpret_cast<void*>(moduleBaseAddress + 0x1CC222 + 1), &playerSlots, sizeof(playerSlots));

        // GameEngine::cleanUp: player vector and reallocation
        success &= Utils::PatchByteAt(moduleBaseAddress + 0x1C1376, 2, playerSlots);
        success &= Utils::PatchByteAt(moduleBaseAddress + 0x1C139D, 2, playerSlots);
        success &= Utils::PatchByteAt(moduleBaseAddress + 0x1CC164, 1, playerSlots);
        success &= Utils::PatchMemory(reinterpret_cast<void*>(moduleBaseAddress + 0x1C13B2 + 1), &playerSlots, sizeof(playerSlots));
        success &= Utils::PatchMemory(reinterpret_cast<void*>(moduleBaseAddress + 0x1CC106 + 1), &playerSlots, sizeof(playerSlots));

        // GameEngine::newGame: runStats and recreating players
        success &= Utils::PatchByteAt(moduleBaseAddress + 0x1C1619, 2, playerSlots);
        success &= Utils::PatchByteAt(moduleBaseAddress + 0x1C1640, 2, playerSlots);
        success &= Utils::PatchByteAt(moduleBaseAddress + 0x1C1F1F, 2, playerSlots);
        success &= Utils::PatchMemory(reinterpret_cast<void*>(moduleBaseAddress + 0x1C1651 + 1), &playerSlots, sizeof(playerSlots));

        // runStats reallocation
        success &= Utils::PatchByteAt(moduleBaseAddress + 0x3BE972, 2, playerSlots);
        success &= Utils::PatchMemory(reinterpret_cast<void*>(moduleBaseAddress + 0x3BE977 + 1), &playerSlots, sizeof(playerSlots));
        success &= Utils::PatchMemory(reinterpret_cast<void*>(moduleBaseAddress + 0x3BE9EE + 1), &playerSlots, sizeof(playerSlots));
        success &= Utils::PatchMemory(reinterpret_cast<void*>(moduleBaseAddress + 0x3BE982 + 1), &runStatsBytes, sizeof(runStatsBytes));

        // PlayerCustomizeMenu: per-player palettes, classes and reset
        success &= Utils::PatchByteAt(moduleBaseAddress + 0x307D2B, 2, playerSlots);
        success &= Utils::PatchByteAt(moduleBaseAddress + 0x307D3F, 2, playerSlots);
        success &= Utils::PatchByteAt(moduleBaseAddress + 0x307D48, 1, playerSlots);
        success &= Utils::PatchByteAt(moduleBaseAddress + 0x307D7B, 2, playerSlots);
        success &= Utils::PatchByteAt(moduleBaseAddress + 0x307D92, 2, playerSlots);
        success &= Utils::PatchByteAt(moduleBaseAddress + 0x3083A6, 2, playerSlots);
        success &= Utils::PatchByteAt(moduleBaseAddress + 0x3083CF, 1, playerSlots);
        success &= Utils::PatchByteAt(moduleBaseAddress + 0x309921, 1, playerSlots);
        success &= Utils::PatchMemory(reinterpret_cast<void*>(moduleBaseAddress + 0x307D53 + 1), &playerSlots, sizeof(playerSlots));
        success &= Utils::PatchMemory(reinterpret_cast<void*>(moduleBaseAddress + 0x307DA5 + 1), &playerSlots, sizeof(playerSlots));
        success &= Utils::PatchMemory(reinterpret_cast<void*>(moduleBaseAddress + 0x3098D4 + 1), &playerSlots, sizeof(playerSlots));
        success &= Utils::PatchMemory(reinterpret_cast<void*>(moduleBaseAddress + 0x3098F8 + 1), &playerSlots, sizeof(playerSlots));

        // Input::reset: controller timers, still only four local joysticks
        success &= Utils::PatchMemory(reinterpret_cast<void*>(moduleBaseAddress + 0x1FD497 + 1), &playerSlots, sizeof(playerSlots));
        success &= Utils::PatchMemory(reinterpret_cast<void*>(moduleBaseAddress + 0x1FD4D3 + 1), &playerSlots, sizeof(playerSlots));
        success &= Utils::PatchMemory(reinterpret_cast<void*>(moduleBaseAddress + 0x1FD4E5 + 1), &playerSlots, sizeof(playerSlots));
        success &= Utils::PatchByteAt(moduleBaseAddress + 0x1FD4BE, 2, playerSlots);
        success &= Utils::PatchByteAt(moduleBaseAddress + 0x1FD4CA, 1, playerSlots);
        success &= Utils::PatchByteAt(moduleBaseAddress + 0x1FD50A, 2, playerSlots);
        success &= Utils::PatchByteAt(moduleBaseAddress + 0x1FD51E, 2, playerSlots);
        success &= Utils::PatchByteAt(moduleBaseAddress + 0x1FD52A, 1, playerSlots);

        // Input::reset: ControlScheme vector for remote players on the server
        success &= Utils::PatchByteAt(moduleBaseAddress + 0x1FD60E, 2, playerSlots);
        success &= Utils::PatchByteAt(moduleBaseAddress + 0x1FD649, 2, playerSlots);
        success &= Utils::PatchByteAt(moduleBaseAddress + 0x207795, 2, playerSlots);
        success &= Utils::PatchMemory(reinterpret_cast<void*>(moduleBaseAddress + 0x1FD65F + 1), &playerSlots, sizeof(playerSlots));
        success &= Utils::PatchMemory(reinterpret_cast<void*>(moduleBaseAddress + 0x20781B + 1), &playerSlots, sizeof(playerSlots));
        success &= Utils::PatchMemory(reinterpret_cast<void*>(moduleBaseAddress + 0x1FD61B + 2), &controlSchemeBytes, sizeof(controlSchemeBytes));
        success &= Utils::PatchMemory(reinterpret_cast<void*>(moduleBaseAddress + 0x2078CE + 2), &controlSchemeBytes, sizeof(controlSchemeBytes));

        // Client::PlayerInformation vector: resize and reallocation
        success &= Utils::PatchByteAt(moduleBaseAddress + 0x169F32, 2, playerSlots);
        success &= Utils::PatchByteAt(moduleBaseAddress + 0x169F84, 2, playerSlots);
        success &= Utils::PatchByteAt(moduleBaseAddress + 0x171288, 2, playerSlots);
        success &= Utils::PatchMemory(reinterpret_cast<void*>(moduleBaseAddress + 0x169F98 + 1), &playerSlots, sizeof(playerSlots));
        success &= Utils::PatchMemory(reinterpret_cast<void*>(moduleBaseAddress + 0x171303 + 1), &playerSlots, sizeof(playerSlots));
        success &= Utils::PatchMemory(reinterpret_cast<void*>(moduleBaseAddress + 0x169F37 + 2), &playerInformationBytes, sizeof(playerInformationBytes));
        success &= Utils::PatchMemory(reinterpret_cast<void*>(moduleBaseAddress + 0x171409 + 2), &playerInformationBytes, sizeof(playerInformationBytes));

        // Client: read all players and update their names and avatars
        success &= Utils::PatchMemory(reinterpret_cast<void*>(moduleBaseAddress + 0x14E7A8 + 2), &playerInformationBytes, sizeof(playerInformationBytes));
        success &= Utils::PatchMemory(reinterpret_cast<void*>(moduleBaseAddress + 0x14D817 + 2), &playerInformationBytes, sizeof(playerInformationBytes));

        // Give each player a healing slot at the bonfire
        success &= Utils::PatchByteAt(moduleBaseAddress + 0xFF6B6, 2, playerSlots);
        success &= Utils::PatchByteAt(moduleBaseAddress + 0xFF6DD, 2, playerSlots);
        success &= Utils::PatchMemory(reinterpret_cast<void*>(moduleBaseAddress + 0xFF6F0 + 1), &playerSlots, sizeof(playerSlots));
        success &= Utils::PatchByteAt(moduleBaseAddress + 0x103CA6, 2, playerSlots);
        success &= Utils::PatchMemory(reinterpret_cast<void*>(moduleBaseAddress + 0x103CAB + 1), &healingRecordBytes, sizeof(healingRecordBytes));
        success &= Utils::PatchMemory(reinterpret_cast<void*>(moduleBaseAddress + 0x103D1A + 1), &playerSlots, sizeof(playerSlots));
        success &= Utils::PatchByteAt(moduleBaseAddress + 0xFF742, 2, playerSlots);
        success &= Utils::PatchByteAt(moduleBaseAddress + 0xFF756, 2, playerSlots);
        success &= Utils::PatchByteAt(moduleBaseAddress + 0xFF75F, 1, playerSlots);
        success &= Utils::PatchMemory(reinterpret_cast<void*>(moduleBaseAddress + 0xFF76A + 1), &playerSlots, sizeof(playerSlots));
        success &= Utils::PatchByteAt(moduleBaseAddress + 0x1023FF, 2, playerSlots);

        // Use playerSlots for "not found" here, otherwise Bonfire::step makes player 5 stand back up
        success &= Utils::PatchByteAt(moduleBaseAddress + 0x101B01, 2, playerSlots);
        success &= Utils::PatchMemory(reinterpret_cast<void*>(moduleBaseAddress + 0x101B06 + 1), &playerSlots, sizeof(playerSlots));
        success &= Utils::PatchByteAt(moduleBaseAddress + 0xFFC6D, 2, playerSlots);

        // Grave revival cost and knockback on the client
        success &= Utils::PatchByteAt(moduleBaseAddress + 0x761F7, 2, playerSlots);

        // PlayerMod: extra players need their own trigger counts
        success &= Utils::PatchByteAt(moduleBaseAddress + 0xD5A67, 2, playerSlots);
        success &= Utils::PatchMemory(reinterpret_cast<void*>(moduleBaseAddress + 0xD5A6C + 3), &triggerCountBytes, sizeof(triggerCountBytes));
        success &= Utils::PatchByteAt(moduleBaseAddress + 0xD5A8E, 2, playerSlots);
        success &= Utils::PatchByteAt(moduleBaseAddress + 0xD5A97, 1, playerSlots);
        success &= Utils::PatchMemory(reinterpret_cast<void*>(moduleBaseAddress + 0xD5AB3 + 1), &playerSlots, sizeof(playerSlots));
        success &= Utils::PatchMemory(reinterpret_cast<void*>(moduleBaseAddress + 0xD5FC4 + 1), &lastPlayerIndex, sizeof(lastPlayerIndex));
        return success != 0;
    }

    bool InitMultiplayer() {
        GetPlayerClass = reinterpret_cast<sdk::PlayerClass::GetPlayerClassFn>(moduleBaseAddress + 0x31E430);
        UpdateClassBackgrounds = reinterpret_cast<UpdateMenuFn>(moduleBaseAddress + 0x31CE80);
        UpdateClassPalettes = reinterpret_cast<UpdateMenuFn>(moduleBaseAddress + 0x31D030);

        static SafetyHookMid steamConnectionsHook = Utils::CreateMidHook(reinterpret_cast<void*>(moduleBaseAddress + 0x36B076), countSteamConnectionsDetour);
        static SafetyHookMid ipConnectionsHook = Utils::CreateMidHook(reinterpret_cast<void*>(moduleBaseAddress + 0x36B70F), countIpConnectionsDetour);
        static SafetyHookMid inputResetHook = Utils::CreateMidHook(reinterpret_cast<void*>(moduleBaseAddress + 0x1FD91C), finishInputResetDetour);
        static SafetyHookMid inputLoadHook = Utils::CreateMidHook(reinterpret_cast<void*>(moduleBaseAddress + 0x200BF4), boundInputConfigControllersDetour);
        static SafetyHookMid inputSaveHook = Utils::CreateMidHook(reinterpret_cast<void*>(moduleBaseAddress + 0x1FFFBF), boundInputConfigControllersDetour);
        static SafetyHookMid inputLegacyLoadHook = Utils::CreateMidHook(reinterpret_cast<void*>(moduleBaseAddress + 0x20182F), boundInputConfigControllersDetour);
        static SafetyHookMid localControllerHook = Utils::CreateMidHook(reinterpret_cast<void*>(moduleBaseAddress + 0x1514FE), boundLocalControllerDetour);
        static SafetyHookMid menuAppearanceHook = Utils::CreateMidHook(reinterpret_cast<void*>(moduleBaseAddress + 0x30DDA6), initializeMenuAppearanceDetour);
        static SafetyHookMid joinAppearanceHook = Utils::CreateMidHook(reinterpret_cast<void*>(moduleBaseAddress + 0x309191), selectJoinAppearanceDetour);
        static SafetyHookMid selectPaletteHook = Utils::CreateMidHook(reinterpret_cast<void*>(moduleBaseAddress + 0x31DF92), selectPaletteDetour);
        static SafetyHookMid randomClassHook = Utils::CreateMidHook(reinterpret_cast<void*>(moduleBaseAddress + 0x3088B5), selectRandomClassDetour);
        static SafetyHookMid hudPositionHook = Utils::CreateMidHook(reinterpret_cast<void*>(moduleBaseAddress + 0x30E614), positionExtraHudDetour);
        static SafetyHookMid graveAccessHook = Utils::CreateMidHook(reinterpret_cast<void*>(moduleBaseAddress + 0x1C9015), boundGraveAccessDetour);
        static SafetyHookMid finishGravesHook = Utils::CreateMidHook(reinterpret_cast<void*>(moduleBaseAddress + 0x1C911D), finishGravesDetour);
        // With no players, the game jumps straight to the cleanup loop
        static SafetyHookMid emptyGravesHook = Utils::CreateMidHook(reinterpret_cast<void*>(moduleBaseAddress + 0x1C9129), finishGravesDetour);

        InitGameplay();
        InitStorage();

        Utils::CreateHook(reinterpret_cast<void*>(moduleBaseAddress + 0x30E4A0),
            &selectVisibleHudDetour, reinterpret_cast<void**>(&pOriginalDrawHud));
        Utils::CreateHook(reinterpret_cast<void*>(moduleBaseAddress + 0x309510),
            &selectRandomPaletteDetour, reinterpret_cast<void**>(&pOriginalRandomPalette));
        if (!pOriginalDrawHud || !pOriginalRandomPalette || !InitPalettes() || !InitSaves()) return false;

        return true;
    }

    void FailInitialization() {
        multiplayerLogger.LogError("Expanded multiplayer could not initialize. Stopping before the game uses incomplete hooks.");
        MessageBoxA(nullptr, "Acolyte could not initialize multiplayer. Check Acolyte/logs for details.\nThe game will close because only part of the required hooks were installed.", "Acolyte", MB_OK | MB_ICONERROR);
        ExitProcess(1);
    }
}
