#include "multiplayer.h"
#include <Acolyte/resources.h>
#include <Acolyte/utils.h>
#include <Vagante/entities.h>
#include <Vagante/stage.h>
#include <Vagante/game.h>
#include <safetyhook.hpp>
#include <map>
#include <random>

static uintptr_t moduleBaseAddress = (uintptr_t)GetModuleHandleA(NULL);

using LoadClassSpritesFn = void(__thiscall*)(Player* playerThis, PlayerClass character, int palette);
using LoadSpriteTextureFn = void(__thiscall*)(Sprite* spriteThis);
using CleanUpFn = void(__thiscall*)(GameEngine* engineThis);
static LoadSpriteTextureFn LoadSpriteTexture = nullptr;

struct TintedTexture {
    sf::Texture texture;
    ulong64 sourceVersion;
};

struct PlayerTextureCache {
    std::map<std::pair<sf::Texture*, uint>, TintedTexture> textures;
    std::map<sf::Texture*, sf::Texture*> originals;
    std::map<Player*, std::weak_ptr<Entity>> players;
};

static std::map<std::vector<std::shared_ptr<Player>>*, PlayerTextureCache> textureCaches;

static sf::Color PlayerColor(unsigned seed) {
    std::mt19937 random(seed);
    return {uchar(64 + random() % 192), uchar(64 + random() % 192), uchar(64 + random() % 192), 255};
}

static uchar BlendOverlay(uchar base, uchar tint) {
    float color = base < 128 ? 2.0f * base * tint / 255 :
        255 - 2.0f * (255 - base) * (255 - tint) / 255;
    return static_cast<uchar>(color + 0.5f);
}

static void TintSprite(Sprite& sprite, unsigned seed, bool tint, PlayerTextureCache& cache) {
    if (sprite.m_textureName.empty()) return;
    if (!tint) {
        auto original = cache.originals.find(sprite.texture);
        if (original != cache.originals.end())
            sprite.texture = original->second;
        return;
    }
    // lazyLoadTexture sets the sprite's texture coordinates and frame counts too
    LoadSpriteTexture(&sprite);
    if (!sprite.texture) return;
    auto original = cache.originals.find(sprite.texture);
    sf::Texture* source = original == cache.originals.end() ? sprite.texture : original->second;
    auto key = std::make_pair(source, seed);
    auto found = cache.textures.find(key);
    // Reloading a texture can change m_cacheId without changing its pointer
    if (found == cache.textures.end() || found->second.sourceVersion != source->m_cacheId) {
        sprite.texture = source;
        sf::Image image{};
        sdk::Resources::CopyToImage(source, &image);
        if (image.m_pixels.empty()) return;
        auto color = PlayerColor(seed);
        for (size_t i = 0; i + 3 < image.m_pixels.size(); i += 4) {
            if (!image.m_pixels[i + 3]) continue;
            image.m_pixels[i] = BlendOverlay(image.m_pixels[i], color.r);
            image.m_pixels[i + 1] = BlendOverlay(image.m_pixels[i + 1], color.g);
            image.m_pixels[i + 2] = BlendOverlay(image.m_pixels[i + 2], color.b);
        }

        bool created = found == cache.textures.end();
        if (created) {
            found = cache.textures.try_emplace(key).first;
            sdk::Resources::TextureConstructor(&found->second.texture);
        }
        sf::Rect<int> rect{};
        if (!sdk::Resources::LoadTextureFromImage(&found->second.texture, &image, &rect)) {
            if (created) {
                sdk::Resources::TextureDestructor(&found->second.texture);
                cache.textures.erase(found);
            }
            return;
        }
        found->second.sourceVersion = source->m_cacheId;
        cache.originals[&found->second.texture] = source;
    }
    sprite.texture = &found->second.texture;
}

LoadClassSpritesFn pOriginalLoadClassSprites = nullptr;
void __fastcall loadPlayerSpritesDetour(Player* playerThis, void*, PlayerClass character, int palette) {
    pOriginalLoadClassSprites(playerThis, character, palette < 0 ? palette : palette % 4);
}

static void TintPlayerSprites(Player* player, unsigned seed, bool tint, PlayerTextureCache& cache) {
    Sprite* sprites[] = {
        &player->standSprite, &player->climbSprite, &player->ladderSlideSprite,
        &player->wallSlideSprite, &player->wallJumpSprite, &player->duckSprite,
        &player->sitSprite, &player->defendSprite, &player->lookUpSprite,
        &player->enteringSprite, &player->exitingSprite, &player->walkSprite,
        &player->walkSlowSprite, &player->ledgeHangAnimSprite, &player->pushSprite,
        &player->rollSprite, &player->eagleRiseSprite, &player->swingSprite,
        &player->twoHandSwingSprite, &player->twoHandSwingArmsSprite, &player->stabSprite,
        &player->aimBowSprite, &player->aimArmSprite, &player->aimWalkSprite,
        &player->throwChargeSprite, &player->throwLaunchSprite, &player->throwArmSprite,
        &player->jumpSprite, &player->fallSprite, &player->jumpPogoSprite,
        &player->jumpPogoSpinSprite, &player->hangSprite, &player->dashSprite,
        &player->hurtSprite, &player->deadSprite, &player->swimSprite,
        &player->diveSprite, &player->castingSprite, &player->castReadySprite,
        &player->castReleasedSprite, &player->swimTransitionSprite,
        &player->swimTransitionReverseSprite, &player->skullWraithSprite, &player->wraithBodySprite,
        &player->holdDeepSwimSprite, &player->holdDuckSprite, &player->holdEnteringSprite,
        &player->holdExitingSprite, &player->holdJumpSprite, &player->holdFallSprite,
        &player->holdLookUpSprite, &player->holdSwimSprite, &player->holdStandSprite,
        &player->holdSwimTransitionSprite, &player->holdSwimTransitionReverseSprite,
        &player->holdThrowLaunchSprite, &player->holdWalkSprite, &player->holdWalkSlowSprite,
        &player->portraitSprite
    };
    for (Sprite* sprite : sprites)
        TintSprite(*sprite, seed, tint, cache);
}

void tintPlayerDetour(SafetyHookContext& ctx) {
    auto player = reinterpret_cast<Player*>(ctx.ecx);
    if (!player->level || !player->level->players) return;
    unsigned character = player->isSkeleton ? CLASS_SKELETON : player->playerClass;
    int palette = player->isSkeleton ? player->playerIndex % 4 : player->palette;
    bool tint = false;
    // Only tint if an earlier player already uses this class and palette
    for (auto& other : *player->level->players) {
        if (!other || other->playerIndex >= player->playerIndex) continue;
        unsigned otherClass = other->isSkeleton ? CLASS_SKELETON : other->playerClass;
        int otherPalette = other->isSkeleton ? other->playerIndex % 4 : other->palette;
        if (otherClass == character && otherPalette == palette) {
            tint = true;
            break;
        }
    }
    // Use the same seed on every client so they agree on the player's color
    unsigned seed = 0x736B656C + player->playerIndex + character * 256 + palette * 65536;
    auto& cache = textureCaches[player->level->players];
    cache.players[player] = player->weak_from_this();
    TintPlayerSprites(player, seed, tint, cache);
}

CleanUpFn pOriginalCleanUp = nullptr;
void __fastcall cleanUpTexturesDetour(GameEngine* engineThis, void*) {
    // If this is the server just call the original cleanup, it has no tinted textures to free
    if (engineThis->networked && !engineThis->clientSide) {
        pOriginalCleanUp(engineThis);
        return;
    }

    auto found = textureCaches.find(&engineThis->players);
    if (found != textureCaches.end()) {
        auto& cache = found->second;
        // Put the original textures back before deleting the tinted ones, a mod might still be holding the player
        for (auto& player : cache.players) {
            auto owner = player.second.lock();
            if (owner)
                TintPlayerSprites(player.first, 0, false, cache);
        }
        for (auto& texture : cache.textures)
            sdk::Resources::TextureDestructor(&texture.second.texture);
        textureCaches.erase(found);
    }
    pOriginalCleanUp(engineThis);
}

namespace Multiplayer {
    bool InitPalettes() {
        LoadSpriteTexture = reinterpret_cast<LoadSpriteTextureFn>(moduleBaseAddress + 0x3B9F00);
        Utils::CreateHook(reinterpret_cast<void*>(moduleBaseAddress + 0x2C8070),
            &loadPlayerSpritesDetour, reinterpret_cast<void**>(&pOriginalLoadClassSprites));
        Utils::CreateHook(reinterpret_cast<void*>(moduleBaseAddress + 0x1C1050),
            &cleanUpTexturesDetour, reinterpret_cast<void**>(&pOriginalCleanUp));
        // Network updates and other mods can replace the sprites, tint them afterwards
        static SafetyHookMid drawHook = Utils::CreateMidHook(reinterpret_cast<void*>(moduleBaseAddress + 0x2F21B0), tintPlayerDetour);
        return pOriginalLoadClassSprites && pOriginalCleanUp;
    }
}
