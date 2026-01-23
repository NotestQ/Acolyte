#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include "sfml.h"

typedef enum StatusEffectType {
    STATUS_HEALING = 0,
    STATUS_MAGIC_POWER = 1,
    STATUS_POISON = 2,
    STATUS_GRAVITY = 3,
    STATUS_SPEED = 4,
    STATUS_STUNNED = 5,
    STATUS_BURNING = 6,
    STATUS_DEFENSE = 7,
    STATUS_STRENGTH = 8,
    STATUS_FROZEN = 9,
    STATUS_FAIRY_FIRE = 10,
    STATUS_BERSERK = 11,
    STATUS_DEAD = 12,
    STATUS_CHARM = 13,
    STATUS_EXHAUSTED = 14,
    STATUS_FIRE_SHIELD = 15,
    STATUS_CURSED = 16,
    STATUS_BLUE_BURNING = 17,
    STATUS_SLOWED = 18,
    STATUS_DARK_METAMORPH = 19,
    STATUS_UNBOUNDED_SIGHT = 20,
    STATUS_BREATH_UNDERWATER = 21,
    STATUS_CANDY = 22,
    STATUS_FAIRY_FIRE_CURSED = 23,
    STATUS_FREEZE_IMMUNE = 24,
    STATUS_ENCHANTED_FIRE = 25,
    STATUS_ENCHANTED_ICE = 26,
    STATUS_ENCHANTED_ELEC = 27,
    STATUS_DIVINE_WEAPON = 28,
    STATUS_SECOND_CHANCE = 29,
    STATUS_STEALTH = 30,
    STATUS_ENCHANTED_WIND = 31,
    STATUS_GOD_FIST = 32,
} StatusEffectType;

typedef enum SkillType {
    DASH = 0,
    FIRE_SHIELD = 1,
    TELEPORT = 2,
    ICE_BOLT = 3,
    FIREBALL = 4,
    LIGHTNING = 5,
    CHAIN_LIGHTNING = 6,
    SPIRITS = 7,
    MAGIC_MISSILE = 8,
    SHOCKWAVE = 9,
    ELEC_LANCE = 10,
    FROST_NOVA = 11,
    FLAME_PILLAR = 12,
    DARK_METAMORPH = 13,
    CHARM = 14,
    SUMMON_MONSTER = 15,
    NUM_SKILL_TYPES = 16,
    PORTAL = 17,
    HEAL = 18,
    BLUE_FIRE_BOLT = 19,
    HOLY_INVULNERABILITY = 20,
    HOLY_WEAPON = 21,
    THROW_BONE = 22,
    ICE_SPIRE = 23,
    FLARE_NOVA = 24,
    TELEKINESIS = 25,
    PSYCHIC_PUSH = 26,
    CREATE_POTION = 27,
    THROW_POTION = 28,
    DOG_WHISTLE = 29
} SkillType;
typedef enum BackgroundType {
    BG_NONE = 0,
    BG_STRONG = 1,
    BG_EXPERIENCED = 2,
    BG_BOOKWORM = 3,
    BG_SURVIVOR = 4,
    BG_ILLITERATE = 5,
    BG_MERCHANT = 6,
    BG_WANTED = 7,
    BG_FARSIGHTED = 8,
    BG_ASCETIC = 9,
    BG_SNAKEEATER = 10,
    BG_NIMBLE = 11,
    BG_WISPCURSE = 12,
    BG_SYNCHRONIZED = 13,
    BG_DEATH = 14,
    BG_MAGE_HEIRLOOM = 15,
    BG_ROGUE_CROSSBOW = 16,
    BG_KNIGHT_ARMORED = 17,
    BG_WILDLING_WEAPON = 18,
    BG_POTION_ADDICT = 19,
    BG_STOIC = 20,
    BG_GRAPPLING_HOOK = 21,
    BG_BOOMERANG = 22,
    BG_FLAIL = 23,
    BG_MIMIC = 24,
    BG_GOD_BLOOD = 25,
    BG_GOD_SMITH = 26,
    BG_GOD_SPELL = 27,
    BG_GOD_SHACKLED = 28,
    BG_GOD_LIGHT = 29,
    BG_GOD_CHAOS = 30,
    BG_FAITHLESS = 31,
    NUM_BACKGROUNDS = 32
} BackgroundType;
typedef enum PlayerClass {
    CLASS_KNIGHT = 0,
    CLASS_ROGUE = 1,
    CLASS_MAGE = 2,
    CLASS_DEPRIVED = 3,
    CLASS_BEASTMASTER = 4,
    CLASS_SKELETON = 5,
    CLASS_RANDOM = 6,
    NUM_PLAYER_CLASSES = 7
} PlayerClass;
struct PlayerMenu {};
struct PlayerCustomizeMenu {};

struct Player {
    char padding[0x344];
    int playerClass;
    char __padding[0x7670];
};

struct AttackScript {
    std::string attackScriptName;
    char padding[0xc]; //TODO: This
    std::string previewWeilderSprite;
    sf::Vector2<int> previewWeilderSpriteDimensions;
    std::string previewAimArmSprite;
    sf::Vector2<int> previewAimArmSpriteDimensions;
    std::string previewWeilderOverlaySprite;
    sf::Vector2<int> previewWeilderOverlaySpriteDimensions;
    sf::Vector2<float> weilderSpriteAnchor;
    sf::Vector2<float> aimArmAnchor;
    bool useAfterImages;
    bool useKeyframeAimOrigin;
    int defaultProjectileType;
    uchar bowDrawFrames;
    bool manualAttackStep;
    bool manualAttackStop;
    bool requireProjectileReloadAnimation;
    bool maintainProjectileVelocity;
    bool renderProjectileAfterAimArm;
    bool useSeparateAmmo;
    bool isOverheadSwing;

};

struct WeaponProperties {
    std::string weaponName;
    std::string weaponSpriteSheet;
    ushort weaponSpriteFrame;
    sf::Vector2<int> weaponSpriteDimensions;
    sf::Vector2<float> weaponSpriteAnchor;
    uchar perFrameDuration;
    uchar wandTypeFlags;
    uchar rodTypeFlags;
    int projectileDamageMin;
    int projectileDamageMax;
    float projectileKnockback;
    float projectileVelocity;
    int defaultProjectileType;
    ushort typeFlags;
    bool isHeavy;
    char padding[0xc]; //TODO: This
};

struct Item {
    char padding[0x560];
};

struct Weapon {
    char padding[0x318];
    AttackScript* attackScript;
    WeaponProperties* weaponProperties;
    char _padding[0x60];
};

struct Sprite {
    char padding[0x194];
};