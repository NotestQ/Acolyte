#include "multiplayer.h"
#include <Acolyte/utils.h>
#include <Vagante/entities.h>
#include <Vagante/server.h>
#include <safetyhook.hpp>

static uintptr_t moduleBaseAddress = (uintptr_t)GetModuleHandleA(NULL);

void emptySnapshotFillDetour(SafetyHookContext& ctx) {
    // Each snapshot allocates 1023 entity states, wait until we actually use it
    Snapshot* snapshot = *reinterpret_cast<Snapshot**>(ctx.esp + 4);
    snapshot->netEntities.clear();
    snapshot->netEntities.shrink_to_fit(); // clear() alone would keep the allocated memory.
}

void allocateSnapshotEntitiesDetour(SafetyHookContext& ctx) {
    // Allocate the entity states when Server::addEntityToSnapshot first needs them.
    Snapshot* snapshot = *reinterpret_cast<Snapshot**>(ctx.ebp + 8);
    if (snapshot->netEntities.empty())
        snapshot->netEntities.resize(1023);
}

// These vectors exceed 4 KiB, so use the game's aligned allocation path.
// It stores the original pointer needed when freeing the vector.
void controlSchemeAllocationDetour(SafetyHookContext& ctx) {
    ctx.eax = Multiplayer::playerSlots;
    ctx.eip = moduleBaseAddress + 0x2077B9;
}

void playerInformationAllocationDetour(SafetyHookContext& ctx) {
    ctx.esi = Multiplayer::playerSlots;
    *reinterpret_cast<unsigned*>(ctx.ebp - 0x18) = Multiplayer::playerSlots;
    ctx.eip = moduleBaseAddress + 0x1712AA;
}

// The larger end offsets don't fit in the original LEA instructions.
// Calculate them here and skip the copied three-byte instruction.
void serverConnectionEndDetour(SafetyHookContext& ctx) {
    ctx.eax = ctx.edx + Multiplayer::playerSlots * sizeof(ClientConnectionData*);
    ctx.eip += 3;
}

void connectionAllocationEndDetour(SafetyHookContext& ctx) {
    ctx.eax = ctx.esi + Multiplayer::playerSlots * sizeof(ClientConnectionData*);
    ctx.eip += 3;
}

void playerMenuEndDetour(SafetyHookContext& ctx) {
    ctx.esi = ctx.eax + Multiplayer::playerSlots * sizeof(std::shared_ptr<PlayerMenu>);
    ctx.eip += 3;
}

void playerMenuAllocationEndDetour(SafetyHookContext& ctx) {
    ctx.eax = ctx.esi + Multiplayer::playerSlots * sizeof(std::shared_ptr<PlayerMenu>);
    ctx.eip += 3;
}

void playerResetEndDetour(SafetyHookContext& ctx) {
    ctx.esi = ctx.edx + Multiplayer::playerSlots * sizeof(std::shared_ptr<Player>);
    ctx.eip += 3;
}

void runStatsEndDetour(SafetyHookContext& ctx) {
    ctx.esi += Multiplayer::playerSlots * sizeof(std::map<std::string, int>);
    ctx.eip += 3;
}

void runStatsAllocationEndDetour(SafetyHookContext& ctx) {
    ctx.eax = ctx.ebx + Multiplayer::playerSlots * sizeof(std::map<std::string, int>);
    ctx.eip += 3;
}

void menuClassEndDetour(SafetyHookContext& ctx) {
    ctx.eax = ctx.edx + Multiplayer::playerSlots * sizeof(int);
    ctx.eip += 3;
}

void menuPaletteEndDetour(SafetyHookContext& ctx) {
    ctx.ecx = ctx.edx + Multiplayer::playerSlots * sizeof(int);
    ctx.eip += 3;
}

void inputTimerEndDetour(SafetyHookContext& ctx) {
    ctx.eax = ctx.edi + Multiplayer::playerSlots * sizeof(float);
    ctx.eip += 3;
}

void bonfireHealingEndDetour(SafetyHookContext& ctx) {
    ctx.eax = ctx.edx + Multiplayer::playerSlots * sizeof(int);
    ctx.eip += 3;
}

void pendingConnectionLoopDetour(SafetyHookContext& ctx) {
    ctx.eip = moduleBaseAddress + (ctx.esi < Multiplayer::playerSlots * sizeof(ClientConnectionData*) ? 0x363BE0 : 0x363C34);
}

void sendPlayerListLoopDetour(SafetyHookContext& ctx) {
    ctx.eip = moduleBaseAddress + (ctx.ebx < Multiplayer::playerSlots * sizeof(ClientConnectionData*) ? 0x365550 : 0x365614);
}

void findSteamConnectionLoopDetour(SafetyHookContext& ctx) {
    ctx.eip = moduleBaseAddress + (ctx.eax < Multiplayer::playerSlots * sizeof(ClientConnectionData*) ? 0x36B051 : 0x36B076);
}

void affinityPlayersLoopDetour(SafetyHookContext& ctx) {
    ctx.eip = moduleBaseAddress + (ctx.edi < Multiplayer::playerSlots * sizeof(ClientConnectionData*) ? 0x36FDA1 : 0x36FE6C);
}

void serverMessagePlayersLoopDetour(SafetyHookContext& ctx) {
    ctx.eip = moduleBaseAddress + (ctx.edi < Multiplayer::playerSlots * sizeof(ClientConnectionData*) ? 0x370223 : 0x370355);
}

void restorePlayersLoopDetour(SafetyHookContext& ctx) {
    ctx.eip = moduleBaseAddress + (ctx.ebx < Multiplayer::playerSlots * sizeof(std::shared_ptr<Player>) ? 0x1D21B0 : 0x1D23C6);
}

void restoreInventoriesLoopDetour(SafetyHookContext& ctx) {
    ctx.eip = moduleBaseAddress + (ctx.ebx < Multiplayer::playerSlots * sizeof(std::shared_ptr<Player>) ? 0x1D23F7 : 0x1D24DE);
}

namespace Multiplayer {
    bool InitStorage() {
        static SafetyHookMid snapshotConstructorHook = safetyhook::create_mid(reinterpret_cast<void*>(moduleBaseAddress + 0x3626F4), emptySnapshotFillDetour);
        static SafetyHookMid snapshotResetHook = safetyhook::create_mid(reinterpret_cast<void*>(moduleBaseAddress + 0x36BFFD), emptySnapshotFillDetour);
        static SafetyHookMid snapshotEntitiesHook = safetyhook::create_mid(reinterpret_cast<void*>(moduleBaseAddress + 0x36595C), allocateSnapshotEntitiesDetour);
        static SafetyHookMid controlSchemeAllocationHook = safetyhook::create_mid(reinterpret_cast<void*>(moduleBaseAddress + 0x20779A), controlSchemeAllocationDetour);
        static SafetyHookMid playerInformationAllocationHook = safetyhook::create_mid(reinterpret_cast<void*>(moduleBaseAddress + 0x17128D), playerInformationAllocationDetour);
        static SafetyHookMid serverConnectionEndHook = safetyhook::create_mid(reinterpret_cast<void*>(moduleBaseAddress + 0x363BA2), serverConnectionEndDetour);
        static SafetyHookMid connectionAllocationEndHook = safetyhook::create_mid(reinterpret_cast<void*>(moduleBaseAddress + 0x372287), connectionAllocationEndDetour);
        static SafetyHookMid playerMenuEndHook = safetyhook::create_mid(reinterpret_cast<void*>(moduleBaseAddress + 0x1C0CCE), playerMenuEndDetour);
        static SafetyHookMid playerMenuAllocationEndHook = safetyhook::create_mid(reinterpret_cast<void*>(moduleBaseAddress + 0x1CC2DE), playerMenuAllocationEndDetour);
        static SafetyHookMid playerResetEndHook = safetyhook::create_mid(reinterpret_cast<void*>(moduleBaseAddress + 0x1C137B), playerResetEndDetour);
        static SafetyHookMid runStatsEndHook = safetyhook::create_mid(reinterpret_cast<void*>(moduleBaseAddress + 0x1C161E), runStatsEndDetour);
        static SafetyHookMid runStatsAllocationEndHook = safetyhook::create_mid(reinterpret_cast<void*>(moduleBaseAddress + 0x3BEAA7), runStatsAllocationEndDetour);
        static SafetyHookMid menuClassEndHook = safetyhook::create_mid(reinterpret_cast<void*>(moduleBaseAddress + 0x307D30), menuClassEndDetour);
        static SafetyHookMid menuPaletteEndHook = safetyhook::create_mid(reinterpret_cast<void*>(moduleBaseAddress + 0x307D80), menuPaletteEndDetour);
        static SafetyHookMid inputTimerEndHook = safetyhook::create_mid(reinterpret_cast<void*>(moduleBaseAddress + 0x1FD4AF), inputTimerEndDetour);
        static SafetyHookMid inputTimerResetEndHook = safetyhook::create_mid(reinterpret_cast<void*>(moduleBaseAddress + 0x1FD50F), inputTimerEndDetour);
        static SafetyHookMid bonfireHealingEndHook = safetyhook::create_mid(reinterpret_cast<void*>(moduleBaseAddress + 0xFF747), bonfireHealingEndDetour);
        static SafetyHookMid pendingConnectionLoopHook = safetyhook::create_mid(reinterpret_cast<void*>(moduleBaseAddress + 0x363C2F), pendingConnectionLoopDetour);
        static SafetyHookMid sendPlayerListLoopHook = safetyhook::create_mid(reinterpret_cast<void*>(moduleBaseAddress + 0x36560B), sendPlayerListLoopDetour);
        static SafetyHookMid findSteamConnectionLoopHook = safetyhook::create_mid(reinterpret_cast<void*>(moduleBaseAddress + 0x36B071), findSteamConnectionLoopDetour);
        static SafetyHookMid affinityPlayersLoopHook = safetyhook::create_mid(reinterpret_cast<void*>(moduleBaseAddress + 0x36FE63), affinityPlayersLoopDetour);
        static SafetyHookMid serverMessagePlayersLoopHook = safetyhook::create_mid(reinterpret_cast<void*>(moduleBaseAddress + 0x37034C), serverMessagePlayersLoopDetour);
        static SafetyHookMid restorePlayersLoopHook = safetyhook::create_mid(reinterpret_cast<void*>(moduleBaseAddress + 0x1D23BD), restorePlayersLoopDetour);
        static SafetyHookMid restoreInventoriesLoopHook = safetyhook::create_mid(reinterpret_cast<void*>(moduleBaseAddress + 0x1D24D5), restoreInventoriesLoopDetour);

        return snapshotConstructorHook && snapshotResetHook && snapshotEntitiesHook &&
               controlSchemeAllocationHook && playerInformationAllocationHook && serverConnectionEndHook &&
               connectionAllocationEndHook && playerMenuEndHook && playerMenuAllocationEndHook &&
               playerResetEndHook && runStatsEndHook && runStatsAllocationEndHook &&
               menuClassEndHook && menuPaletteEndHook && inputTimerEndHook &&
               inputTimerResetEndHook && bonfireHealingEndHook && pendingConnectionLoopHook &&
               sendPlayerListLoopHook && findSteamConnectionLoopHook && affinityPlayersLoopHook &&
               serverMessagePlayersLoopHook && restorePlayersLoopHook && restoreInventoriesLoopHook;
    }

}
