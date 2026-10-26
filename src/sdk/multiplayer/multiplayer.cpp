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

using GameAllocateFn = void*(__cdecl*)(size_t size);
using AddEntityFn = void(__thiscall*)(Level* levelThis, std::shared_ptr<Entity>& entity);
using UpdateMenuFn = void(__thiscall*)(PlayerMenu* menuThis);
using DrawHudFn = void(__thiscall*)(PlayerMenu* menuThis, sf::RenderTarget* renderTarget);
// This constructor takes this in ECX, x/y in XMM2/XMM3 and pops one stack argument.
// The unused arguments put x/y in the right registers.
using GraveConstructorFn = Grave*(__vectorcall*)(Grave* graveThis, void*, void*, float, float, float x, float y);
using RandomPaletteFn = int(__thiscall*)(PlayerCustomizeMenu* customizeThis, unsigned slot, int character);

static GameAllocateFn GameAllocate = nullptr;
static AddEntityFn AddEntity = nullptr;
static sdk::PlayerClass::GetPlayerClassFn GetPlayerClass = nullptr;
static UpdateMenuFn UpdateClassBackgrounds = nullptr;
static UpdateMenuFn UpdateClassPalettes = nullptr;
static GraveConstructorFn GraveConstructor = nullptr;

static void DestroyGrave(Grave* grave) {
    using DestroyFn = void*(__thiscall*)(Grave* graveThis, uint flags);
    // The game's destructor also releases the sprite and frees the allocation.
    auto vtable = static_cast<void**>(grave->vtable);
    reinterpret_cast<DestroyFn>(vtable[0])(grave, 1);
}

static unsigned __cdecl CountConnections(Server* server) {
    unsigned count = 0;
    for (unsigned i = 0; i < Multiplayer::playerSlots; ++i) {
        count += server->clientData[i]->active;
        count += server->pendingClientData[i]->active;
    }
    return count;
}

// We replace the counting loop here, so resume at the game's connection limit check
void countSteamConnectionsDetour(SafetyHookContext& ctx) {
    Server* server = reinterpret_cast<Server*>(ctx.ebx);
    ctx.ecx = CountConnections(server);
    ctx.esi = reinterpret_cast<uintptr_t>(server->clientData.front());
    ctx.eip = moduleBaseAddress + 0x36B0F2;
}

void countIpConnectionsDetour(SafetyHookContext& ctx) {
    // Skip the same-IP rejection too, so players can share a network.
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
        controller.enabled = false; // Server::onReceiveInput enables this later.
        controller.binding = false; // Remote slots must not bind local devices.
    }
}

// Local Input may have been created with four controllers before Acolyte loads.
// Online clients use slot zero for their own input.
void boundLocalControllerDetour(SafetyHookContext& ctx) {
    Client* client = reinterpret_cast<Client*>(ctx.edi);
    if (ctx.ecx >= 4 || ctx.ecx >= client->input->controllers.size())
        ctx.eip = moduleBaseAddress + 0x151543;
}

void initializeMenuAppearanceDetour(SafetyHookContext& ctx) {
    PlayerMenu* menu = reinterpret_cast<PlayerMenu*>(ctx.esi);
    // Only the five base classes exist here. GetPlayerClass adds the modded classes later.
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
    // GetPlayerClass adds any modded classes before we read the list.
    int menuClass = GetPlayerClass(menu, nullptr);
    int currentClass = randomizedClass ? *randomizedClass : menuClass;
    int randomPalette = -1;
    auto& palette = randomizedClass ? randomPalette : menu->playerPalette;
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
        // Keep the random palette option if this class still has room.
        if (step == 0 && palette < 0) return palette;
        unsigned candidate = step == 0 ? static_cast<unsigned>(palette) % 4 : 0;
        while (used & (1u << candidate)) candidate = (candidate + 1) % 4;
        palette = static_cast<int>(candidate);
        if (randomizedClass) *randomizedClass = character;
        else if (step != 0) {
            menu->playerClass = index;
            menu->updateTextContainer = true;
            UpdateClassBackgrounds(menu);
            UpdateClassPalettes(menu);
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
    // randomPalette would divide by zero if all four palettes were taken.
    return used == 15 ? std::rand() % 4 : pOriginalRandomPalette(customizeThis, slot, character);
}

void selectRandomClassDetour(SafetyHookContext& ctx) {
    PlayerCustomizeMenu* customize = *reinterpret_cast<PlayerCustomizeMenu**>(ctx.ebp - 0x14);
    unsigned slot = *reinterpret_cast<unsigned*>(ctx.ebp + 8);
    auto& menus = *customize->guis;
    if (slot < menus.size() && menus[slot])
        ChooseJoinAppearance(menus[slot].get(), reinterpret_cast<int*>(ctx.ebp - 0x1C));
}

// The original loop gives up after four palettes. Check the next class too.
void selectJoinAppearanceDetour(SafetyHookContext& ctx) {
    ctx.edi = ChooseJoinAppearance(reinterpret_cast<PlayerMenu*>(ctx.ebx)); // addPlayer reads the palette from EDI.
    *reinterpret_cast<unsigned*>(ctx.ebp - 0x10) = 0; // Later reused for the background argument.
    ctx.eip = moduleBaseAddress + 0x3091E8;
}

DrawHudFn pOriginalDrawHud = nullptr;
// Extra players use the first HUD position on their own screen.
void __fastcall selectVisibleHudDetour(PlayerMenu* menuThis, void*, sf::RenderTarget* renderTarget) {
    auto playerIndex = menuThis->playerIndex;
    auto localPlayer = static_cast<unsigned>(menuThis->networkedPlayerIndex);
    if (menuThis->networked) {
        if (playerIndex >= 4 && playerIndex != localPlayer) return;
        if (playerIndex == 0 && localPlayer >= 4) return;
    } else if (playerIndex >= 4) {
        return;
    }
    pOriginalDrawHud(menuThis, renderTarget);
}

// Change the position calculation, keeping playerIndex for inventory and customization.
void positionExtraHudDetour(SafetyHookContext& ctx) {
    if (ctx.eax >= 4) ctx.eax = 0;
}

void addRoomGraveDetour(SafetyHookContext& ctx) {
    // The original shared_ptr has been constructed at this point.
    Level* level = *reinterpret_cast<Level**>(ctx.ebp - 0x70);
    Entity* original = *reinterpret_cast<Entity**>(ctx.ebp - 0x6C);
    if (!level || !original || original->type != TYPE_GRAVE) return;

    unsigned count = 0;
    if (level->players) {
        for (auto& player : *level->players)
            count += player != nullptr;
    }
    // Add another grave at each marker for every extra group of four players.
    for (unsigned group = 1; group * 4 < count; ++group) {
        auto grave = static_cast<Grave*>(GameAllocate(sizeof(Grave)));
        // Keep the extra graves beside the original, so they don't end up inside walls.
        GraveConstructor(grave, nullptr, nullptr, 0.0f, 0.0f, original->x + 8.0f, original->y);
        std::shared_ptr<Entity> shared(grave, DestroyGrave);
        AddEntity(level, shared);
    }
}

// The original loop reads graves[deadCount] even for living players.
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

        // Native ClientConnectionData* vector reallocation (both server arrays)
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

        // GameEngine::GameEngine: PlayerMenu vector
        success &= Utils::PatchByteAt(moduleBaseAddress + 0x1C0CC9, 2, playerSlots);
        success &= Utils::PatchByteAt(moduleBaseAddress + 0x1C0CF4, 2, playerSlots);
        success &= Utils::PatchMemory(reinterpret_cast<void*>(moduleBaseAddress + 0x1C0D08 + 1), &playerSlots, sizeof(playerSlots));

        // Native PlayerMenu vector reallocation
        success &= Utils::PatchByteAt(moduleBaseAddress + 0x1CC1B7, 2, playerSlots);
        success &= Utils::PatchMemory(reinterpret_cast<void*>(moduleBaseAddress + 0x1CC1BC + 1), &sharedMenuBytes, sizeof(sharedMenuBytes));
        success &= Utils::PatchMemory(reinterpret_cast<void*>(moduleBaseAddress + 0x1CC222 + 1), &playerSlots, sizeof(playerSlots));

        // GameEngine::cleanUp: player vector, including native reallocation
        success &= Utils::PatchByteAt(moduleBaseAddress + 0x1C1376, 2, playerSlots);
        success &= Utils::PatchByteAt(moduleBaseAddress + 0x1C139D, 2, playerSlots);
        success &= Utils::PatchByteAt(moduleBaseAddress + 0x1CC164, 1, playerSlots);
        success &= Utils::PatchMemory(reinterpret_cast<void*>(moduleBaseAddress + 0x1C13B2 + 1), &playerSlots, sizeof(playerSlots));
        success &= Utils::PatchMemory(reinterpret_cast<void*>(moduleBaseAddress + 0x1CC106 + 1), &playerSlots, sizeof(playerSlots));

        // GameEngine::newGame: run statistics and existing player reconstruction
        success &= Utils::PatchByteAt(moduleBaseAddress + 0x1C1619, 2, playerSlots);
        success &= Utils::PatchByteAt(moduleBaseAddress + 0x1C1640, 2, playerSlots);
        success &= Utils::PatchByteAt(moduleBaseAddress + 0x1C1F1F, 2, playerSlots);
        success &= Utils::PatchMemory(reinterpret_cast<void*>(moduleBaseAddress + 0x1C1651 + 1), &playerSlots, sizeof(playerSlots));

        // Native runStats vector reallocation
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

        // Input::reset: per-controller timers (hardware joystick limits stay at four)
        success &= Utils::PatchMemory(reinterpret_cast<void*>(moduleBaseAddress + 0x1FD497 + 1), &playerSlots, sizeof(playerSlots));
        success &= Utils::PatchMemory(reinterpret_cast<void*>(moduleBaseAddress + 0x1FD4D3 + 1), &playerSlots, sizeof(playerSlots));
        success &= Utils::PatchMemory(reinterpret_cast<void*>(moduleBaseAddress + 0x1FD4E5 + 1), &playerSlots, sizeof(playerSlots));
        success &= Utils::PatchByteAt(moduleBaseAddress + 0x1FD4BE, 2, playerSlots);
        success &= Utils::PatchByteAt(moduleBaseAddress + 0x1FD4CA, 1, playerSlots);
        success &= Utils::PatchByteAt(moduleBaseAddress + 0x1FD50A, 2, playerSlots);
        success &= Utils::PatchByteAt(moduleBaseAddress + 0x1FD51E, 2, playerSlots);
        success &= Utils::PatchByteAt(moduleBaseAddress + 0x1FD52A, 1, playerSlots);

        // Input::reset: ControlScheme vector for server-side remote input
        success &= Utils::PatchByteAt(moduleBaseAddress + 0x1FD60E, 2, playerSlots);
        success &= Utils::PatchByteAt(moduleBaseAddress + 0x1FD649, 2, playerSlots);
        success &= Utils::PatchByteAt(moduleBaseAddress + 0x207795, 2, playerSlots);
        success &= Utils::PatchMemory(reinterpret_cast<void*>(moduleBaseAddress + 0x1FD65F + 1), &playerSlots, sizeof(playerSlots));
        success &= Utils::PatchMemory(reinterpret_cast<void*>(moduleBaseAddress + 0x20781B + 1), &playerSlots, sizeof(playerSlots));
        success &= Utils::PatchMemory(reinterpret_cast<void*>(moduleBaseAddress + 0x1FD61B + 2), &controlSchemeBytes, sizeof(controlSchemeBytes));
        success &= Utils::PatchMemory(reinterpret_cast<void*>(moduleBaseAddress + 0x2078CE + 2), &controlSchemeBytes, sizeof(controlSchemeBytes));

        // Client::PlayerInformation vector: resize and native reallocation
        success &= Utils::PatchByteAt(moduleBaseAddress + 0x169F32, 2, playerSlots);
        success &= Utils::PatchByteAt(moduleBaseAddress + 0x169F84, 2, playerSlots);
        success &= Utils::PatchByteAt(moduleBaseAddress + 0x171288, 2, playerSlots);
        success &= Utils::PatchMemory(reinterpret_cast<void*>(moduleBaseAddress + 0x169F98 + 1), &playerSlots, sizeof(playerSlots));
        success &= Utils::PatchMemory(reinterpret_cast<void*>(moduleBaseAddress + 0x171303 + 1), &playerSlots, sizeof(playerSlots));
        success &= Utils::PatchMemory(reinterpret_cast<void*>(moduleBaseAddress + 0x169F37 + 2), &playerInformationBytes, sizeof(playerInformationBytes));
        success &= Utils::PatchMemory(reinterpret_cast<void*>(moduleBaseAddress + 0x171409 + 2), &playerInformationBytes, sizeof(playerInformationBytes));

        // Client: decode the expanded roster and refresh names/avatars
        success &= Utils::PatchMemory(reinterpret_cast<void*>(moduleBaseAddress + 0x14E7A8 + 2), &playerInformationBytes, sizeof(playerInformationBytes));
        success &= Utils::PatchMemory(reinterpret_cast<void*>(moduleBaseAddress + 0x14D817 + 2), &playerInformationBytes, sizeof(playerInformationBytes));

        // Bonfire: healing records, personal healing totals and player acceptance.
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

        // Client-side grave revival cost and knockback accept all online slots.
        success &= Utils::PatchByteAt(moduleBaseAddress + 0x761F7, 2, playerSlots);

        // PlayerMod: independent trigger counts instead of aliasing player four.
        success &= Utils::PatchByteAt(moduleBaseAddress + 0xD5A67, 2, playerSlots);
        success &= Utils::PatchMemory(reinterpret_cast<void*>(moduleBaseAddress + 0xD5A6C + 3), &triggerCountBytes, sizeof(triggerCountBytes));
        success &= Utils::PatchByteAt(moduleBaseAddress + 0xD5A8E, 2, playerSlots);
        success &= Utils::PatchByteAt(moduleBaseAddress + 0xD5A97, 1, playerSlots);
        success &= Utils::PatchMemory(reinterpret_cast<void*>(moduleBaseAddress + 0xD5AB3 + 1), &playerSlots, sizeof(playerSlots));
        success &= Utils::PatchMemory(reinterpret_cast<void*>(moduleBaseAddress + 0xD5FC4 + 1), &lastPlayerIndex, sizeof(lastPlayerIndex));
        return success != 0;
    }

    bool InitMultiplayer() {
        GraveConstructor = reinterpret_cast<GraveConstructorFn>(moduleBaseAddress + 0x75540);
        GameAllocate = reinterpret_cast<GameAllocateFn>(moduleBaseAddress + 0x4CCFA2);
        AddEntity = reinterpret_cast<AddEntityFn>(moduleBaseAddress + 0x24AFC0);
        GetPlayerClass = reinterpret_cast<sdk::PlayerClass::GetPlayerClassFn>(moduleBaseAddress + 0x31E430);
        UpdateClassBackgrounds = reinterpret_cast<UpdateMenuFn>(moduleBaseAddress + 0x31CE80);
        UpdateClassPalettes = reinterpret_cast<UpdateMenuFn>(moduleBaseAddress + 0x31D030);

        static SafetyHookMid steamConnectionsHook = safetyhook::create_mid(reinterpret_cast<void*>(moduleBaseAddress + 0x36B076), countSteamConnectionsDetour);
        static SafetyHookMid ipConnectionsHook = safetyhook::create_mid(reinterpret_cast<void*>(moduleBaseAddress + 0x36B70F), countIpConnectionsDetour);
        static SafetyHookMid inputResetHook = safetyhook::create_mid(reinterpret_cast<void*>(moduleBaseAddress + 0x1FD91C), finishInputResetDetour);
        static SafetyHookMid localControllerHook = safetyhook::create_mid(reinterpret_cast<void*>(moduleBaseAddress + 0x1514FE), boundLocalControllerDetour);
        static SafetyHookMid menuAppearanceHook = safetyhook::create_mid(reinterpret_cast<void*>(moduleBaseAddress + 0x30DDA6), initializeMenuAppearanceDetour);
        static SafetyHookMid joinAppearanceHook = safetyhook::create_mid(reinterpret_cast<void*>(moduleBaseAddress + 0x309191), selectJoinAppearanceDetour);
        static SafetyHookMid randomClassHook = safetyhook::create_mid(reinterpret_cast<void*>(moduleBaseAddress + 0x3088B5), selectRandomClassDetour);
        static SafetyHookMid hudPositionHook = safetyhook::create_mid(reinterpret_cast<void*>(moduleBaseAddress + 0x30E614), positionExtraHudDetour);
        static SafetyHookMid roomGraveHook = safetyhook::create_mid(reinterpret_cast<void*>(moduleBaseAddress + 0x35AA40), addRoomGraveDetour);
        static SafetyHookMid graveAccessHook = safetyhook::create_mid(reinterpret_cast<void*>(moduleBaseAddress + 0x1C9015), boundGraveAccessDetour);
        static SafetyHookMid finishGravesHook = safetyhook::create_mid(reinterpret_cast<void*>(moduleBaseAddress + 0x1C911D), finishGravesDetour);
        // The zero-player branch enters the cleanup loop directly.
        static SafetyHookMid emptyGravesHook = safetyhook::create_mid(reinterpret_cast<void*>(moduleBaseAddress + 0x1C9129), finishGravesDetour);

        if (!steamConnectionsHook || !ipConnectionsHook || !inputResetHook || !localControllerHook ||
            !menuAppearanceHook || !joinAppearanceHook || !randomClassHook || !hudPositionHook || !roomGraveHook ||
            !graveAccessHook || !finishGravesHook || !emptyGravesHook || !InitGameplay() || !InitStorage()) {
            multiplayerLogger.LogError("SafetyHook installation failed.");
            return false;
        }

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
