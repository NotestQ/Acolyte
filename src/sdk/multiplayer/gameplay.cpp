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

// This offset no longer fits in one byte with the larger healing array
void bonfireShrinkEndDetour(SafetyHookContext& ctx) {
    ctx.esi = ctx.edx + Multiplayer::playerSlots * sizeof(Bonfire::HealingPlayerState);
}

void bonfireAllocationEndDetour(SafetyHookContext& ctx) {
    ctx.eax = ctx.edi + Multiplayer::playerSlots * sizeof(Bonfire::HealingPlayerState);
}

void bonfireSeatDetour(SafetyHookContext& ctx) {
    // There are four seat positions, healing still uses the player's own slot
    ctx.eax %= 4 * sizeof(float);
}

void sendBonfirePlayersDetour(SafetyHookContext& ctx) {
    Bonfire* bonfire = reinterpret_cast<Bonfire*>(ctx.esi);
    // Bit 0 is the fire, the next 31 bits are players. Skip flags2 since the bonfire uses it for effects
    uint flags[4]{};
    flags[0] = bonfire->currSprite == &bonfire->sprLit;
    for (uint i = 0; i < bonfire->healingPlayers.size(); ++i) {
        auto& playerHealing = bonfire->healingPlayers[i];
        if (!playerHealing.healing || !playerHealing.player) continue;

        uint playerBit = i + 1;
        uint flagIndex = playerBit / 32;
        uint bitIndex = playerBit % 32;
        flags[flagIndex] |= 1u << bitIndex;
    }
    bonfire->entityState.flags3 = flags[1];
    bonfire->entityState.flags4 = flags[2];
    bonfire->entityState.flags5 = flags[3];
    ctx.eax = flags[0];
    ctx.eip = moduleBaseAddress + 0x101D53;
}

void readBonfireHealingDetour(SafetyHookContext& ctx) {
    EntityState* state = *reinterpret_cast<EntityState**>(ctx.ebp + 8);
    uint flags[] = { state->flags, state->flags3, state->flags4, state->flags5 };
    // EAX selects the flags field, ECX is the mask for this player
    bool playerHealing = (flags[ctx.eax] & ctx.ecx) != 0;
    ctx.eip = moduleBaseAddress + (playerHealing ? 0x101EF0 : 0x101FA8);
}

void receiveBonfirePlayersDetour(SafetyHookContext& ctx) {
    ctx.eip = moduleBaseAddress + (ctx.edx < Multiplayer::playerSlots * sizeof(Bonfire::HealingPlayerState) ? 0x101E03 : 0x1020ED);
}

void distributeCookedFoodDetour(SafetyHookContext& ctx) {
    ctx.eip = moduleBaseAddress + (ctx.eax < Multiplayer::playerSlots * sizeof(Bonfire::HealingPlayerState) ? 0x100ED0 : 0x100F30);
}

static void ResetGrave(Grave* grave) {
    grave->player.reset();
    grave->touchingPlayer = false;
    grave->showPortraitSpriteTimer = 0;
    grave->haveRevivedSomeone = false;
    grave->netHaveTakenCost = false;
    grave->netReviveSourcePlayerIndex = -1;
    grave->fullRevive = false;
    grave->reviving = false;
    grave->reviveFrames = 0;
    grave->afterReviveFrames = 0;
    grave->netSinking = false;
}

void reuseGraveDetour(SafetyHookContext& ctx) {
    Grave* grave = reinterpret_cast<Grave*>(ctx.ecx);
    Level* level = grave->level;
    if (grave->clientSide || !level || !level->players || !level->entities)
        return;
    if (grave->reviving) return;

    // The grave takes 120 frames to sink after a revival
    if (grave->haveRevivedSomeone && grave->afterReviveFrames < 120) return;
    if (!grave->haveRevivedSomeone && !grave->player.expired()) return;

    for (auto& player : *level->players) {
        if (!player || player->disconnected || !player->isSkeleton || player->doRevive)
            continue;

        bool hasGrave = false;
        for (auto& entity : *level->entities) {
            if (entity->type != TYPE_GRAVE) continue;
            auto* otherGrave = static_cast<Grave*>(entity.get());
            if (otherGrave->player.lock() == player) {
                hasGrave = true;
                break;
            }
        }
        if (hasGrave) continue;

        ResetGrave(grave);
        grave->player = player;
        grave->skeletonPlayerClass = static_cast<PlayerClass>(player->playerClass);
        grave->skeletonPalette = player->palette;
        grave->visible = true;
        grave->palpable = true;
        break;
    }
}

void receiveGravePlayerDetour(SafetyHookContext& ctx) {
    Grave* grave = reinterpret_cast<Grave*>(ctx.ecx);
    EntityState* state = *reinterpret_cast<EntityState**>(ctx.esp + 4);
    bool visible = (state->flags & 1) != 0;
    bool reviving = (state->flags & 2) != 0;
    bool revived = (state->flags & 4) != 0;
    if (!visible || revived) return;

    uint playerIndex = (state->flags >> 8) & 0xFF;
    auto player = grave->player.lock();
    // Clear the old player and haveRevivedSomeone first or setEntityState won't assign the next skeleton
    if (grave->haveRevivedSomeone || grave->netSinking || (grave->reviving && !reviving) ||
        (player && player->playerIndex != playerIndex))
        ResetGrave(grave);
}

namespace Multiplayer {
    void InitGameplay() {
        static SafetyHookMid healingAllowanceHook = Utils::CreateMidHook(reinterpret_cast<void*>(moduleBaseAddress + 0x246FFA), initializeHealingAllowanceDetour);
        static SafetyHookMid restoredAllowanceHook = Utils::CreateMidHook(reinterpret_cast<void*>(moduleBaseAddress + 0x1D1FCF), restoreHealingAllowanceDetour);
        static SafetyHookMid enemyScalingHook = Utils::CreateMidHook(reinterpret_cast<void*>(moduleBaseAddress + 0x2B392E), scaleExtraPlayersDetour);

        static SafetyHookMid bonfireShrinkHook = Utils::CreateMidHook(reinterpret_cast<void*>(moduleBaseAddress + 0xFF6BE), bonfireShrinkEndDetour);
        static SafetyHookMid bonfireAllocationHook = Utils::CreateMidHook(reinterpret_cast<void*>(moduleBaseAddress + 0x103DCD), bonfireAllocationEndDetour);
        static SafetyHookMid bonfireSeatHook = Utils::CreateMidHook(reinterpret_cast<void*>(moduleBaseAddress + 0xFFE7E), bonfireSeatDetour);
        static SafetyHookMid bonfireSendHook = Utils::CreateMidHook(reinterpret_cast<void*>(moduleBaseAddress + 0x101CC1), sendBonfirePlayersDetour);
        static SafetyHookMid bonfireHealingHook = Utils::CreateMidHook(reinterpret_cast<void*>(moduleBaseAddress + 0x101EE6), readBonfireHealingDetour);
        static SafetyHookMid bonfireReceiveHook = Utils::CreateMidHook(reinterpret_cast<void*>(moduleBaseAddress + 0x1020E4), receiveBonfirePlayersDetour);
        static SafetyHookMid bonfireFoodHook = Utils::CreateMidHook(reinterpret_cast<void*>(moduleBaseAddress + 0x100F2B), distributeCookedFoodDetour);

        static SafetyHookMid graveReuseHook = Utils::CreateMidHook(reinterpret_cast<void*>(moduleBaseAddress + 0x757A0), reuseGraveDetour);
        static SafetyHookMid gravePlayerHook = Utils::CreateMidHook(reinterpret_cast<void*>(moduleBaseAddress + 0x76030), receiveGravePlayerDetour);
    }
}
