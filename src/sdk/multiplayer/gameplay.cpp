#include <Acolyte/utils.h>
#include <Vagante/entities.h>
#include <safetyhook.hpp>
#include <cstdint>
#include "multiplayer.h"

static uintptr_t moduleBaseAddress = (uintptr_t)GetModuleHandleA(NULL);

static void ExtendHealingAllowance(Level* level) {
    auto& healed = level->amountHealedPerPlayer;
    if (healed.size() < Multiplayer::playerSlots)
        healed.resize(Multiplayer::playerSlots, 0);
}

void initializeHealingAllowanceDetour(SafetyHookContext& ctx) {
    ExtendHealingAllowance(reinterpret_cast<Level*>(ctx.edi));
}

void restoreHealingAllowanceDetour(SafetyHookContext& ctx) {
    ExtendHealingAllowance(*reinterpret_cast<Level**>(ctx.ebp - 0x40));
}

void scaleExtraPlayersDetour(SafetyHookContext& ctx) {
    if (ctx.ebx <= 4) return;
    // The game stops scaling at four players, continue the same increase for the rest
    Monster* monster = reinterpret_cast<Monster*>(ctx.esi);
    monster->healthScaleFactor = 1.0f + 0.25f * (ctx.ebx - 1);
    monster->damageScaleFactor = 1.0f + 0.10f * (ctx.ebx - 1);
}

// The larger healing array doesn't fit in the original one-byte displacement.
void bonfireShrinkEndDetour(SafetyHookContext& ctx) {
    ctx.esi = ctx.edx + Multiplayer::playerSlots * sizeof(Bonfire::HealingPlayerState);
}

void bonfireAllocationEndDetour(SafetyHookContext& ctx) {
    ctx.eax = ctx.edi + Multiplayer::playerSlots * sizeof(Bonfire::HealingPlayerState);
}

void receiveBonfirePlayersDetour(SafetyHookContext& ctx) {
    ctx.eip = moduleBaseAddress + (ctx.edx < Multiplayer::playerSlots * sizeof(Bonfire::HealingPlayerState) ? 0x101E03 : 0x1020ED);
}

void distributeCookedFoodDetour(SafetyHookContext& ctx) {
    ctx.eip = moduleBaseAddress + (ctx.eax < Multiplayer::playerSlots * sizeof(Bonfire::HealingPlayerState) ? 0x100ED0 : 0x100F30);
}

namespace Multiplayer {
    bool InitGameplay() {
        static SafetyHookMid healingAllowanceHook = safetyhook::create_mid(reinterpret_cast<void*>(moduleBaseAddress + 0x246FFA), initializeHealingAllowanceDetour);
        static SafetyHookMid restoredAllowanceHook = safetyhook::create_mid(reinterpret_cast<void*>(moduleBaseAddress + 0x1D1FCF), restoreHealingAllowanceDetour);
        static SafetyHookMid enemyScalingHook = safetyhook::create_mid(reinterpret_cast<void*>(moduleBaseAddress + 0x2B392E), scaleExtraPlayersDetour);
        static SafetyHookMid bonfireShrinkHook = safetyhook::create_mid(reinterpret_cast<void*>(moduleBaseAddress + 0xFF6BE), bonfireShrinkEndDetour);
        static SafetyHookMid bonfireAllocationHook = safetyhook::create_mid(reinterpret_cast<void*>(moduleBaseAddress + 0x103DCD), bonfireAllocationEndDetour);
        static SafetyHookMid bonfireReceiveHook = safetyhook::create_mid(reinterpret_cast<void*>(moduleBaseAddress + 0x1020E4), receiveBonfirePlayersDetour);
        static SafetyHookMid bonfireFoodHook = safetyhook::create_mid(reinterpret_cast<void*>(moduleBaseAddress + 0x100F2B), distributeCookedFoodDetour);
        return healingAllowanceHook && restoredAllowanceHook && enemyScalingHook &&
               bonfireShrinkHook && bonfireAllocationHook && bonfireReceiveHook && bonfireFoodHook;
    }

}
