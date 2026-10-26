#pragma once
#include <Vagante/items.h>

struct GameSave {
    struct ItemAttribData {
        std::string typeString;
        float val;
        bool base;
        bool smithBlessing;
    };

    struct ItemData {
        ItemType type;
        int count;
        int inventoryX;
        int inventoryY;
        bool equipped;
        bool belongsToShopkeeper;
        uint randIndex;
        uint wandCharges;
        ItemAffix affix;
        std::vector<GameSave::ItemAttribData> attributes;
        std::vector<GameSave::ItemData> storedItems;
        bool cursed;
        bool godCursed;
        bool identified;
        bool isMaskedUsable;
        bool xbowDrawn;
        bool artifact;
    };

    struct InventoryData {
        std::vector<GameSave::ItemData> itemDat;
        int currMeleeWeapon;
        int currRangeWeapon;
        std::vector<std::pair<int, int>> meleeQuickslots;
        std::vector<std::pair<int, int>> rangeQuickslots;
    };

    struct AffinityData {
        AffinityType affinityType;
        int affinityStatsType;
        uint level;
    };

    struct SkillData {
        SkillType type;
        bool equipped;
        bool inQuickSlot;
        int level;
        int charges;
    };

    struct StatusEffectData {
        StatusEffectType type;
        bool lastsForever;
        int duration;
        int durationTimer;
        float strength;
    };

    struct Resistances {
        int resistFire;
        int resistCold;
        int resistElec;
        int resistPois;
    };

    struct PetData {
        std::vector<int> stats;
    };

    struct PlayerData {
        PlayerClass playerClass;
        BackgroundType bg;
        std::vector<GameSave::AffinityData> affinities;
        GameSave::InventoryData inventory;
        std::vector<GameSave::SkillData> skills;
        std::vector<SkillType> skillQuickSlots;
        std::vector<SkillType> skillControls;
        float hp;
        int experienceLevel;
        int ap;
        std::vector<int> statBonuses;
        GameSave::Resistances finalResistances;
        std::vector<GameSave::StatusEffectData> statusEffects;
        int lightRadius;
        int palette;
        int floorsSurvivedWithMortalAttribute;
        bool asceticVow;
        bool dead;
        bool haveRechargedAtBonfire;
        bool useControlledSkills;
        bool useDirectionSpellCasting;
        bool hasFreeQuaffed;
        GameSave::PetData petData;
    };

    struct LevelGenDat {
        std::vector<bool> haveSpawnedFairy;
        int thievesSpawned;
        std::set<EntityType> spawnedBosses;
        bool newGamePlus;
        bool hardMode;
        std::vector<bool> savedFairies;
    };

    struct ItemGenDat {
        std::map<ItemType, bool> identified;
        std::set<std::string> usedArtifactNames;
        std::set<ItemType> spawnedUniqueItemTypes;
        std::set<AttributeType> spawnedUniqueItemAttributeTypes;
    };

    GameSave::PlayerData m_playerData[4];
    GameSave::LevelGenDat m_levelGenDat;
    GameSave::ItemGenDat m_itemGenDat;
    uint m_prevAct;
    uint m_prevFloor;
    bool m_goToBranch;
    int m_gold;
    uint m_itemSeed;
    uint m_frameCount;
    uint m_levelSeed;
    bool m_shopKeepersAngered;
    int m_shopKeeperChaseCount;
    int m_intermissionKeepers;
    std::string m_loadedFileName;
    bool m_rendezvousWithThief;
    HeistItemAttributes m_heistItemData;
    std::vector<int> m_pieties;
    std::vector<std::vector<int>> m_sacrificedItemTypes;
    std::vector<int> m_amountHealedPerPlayer;
    int m_experience;
    bool m_bonfireLit;
    bool m_insideNewGamePlusTransition;
    bool m_newGamePlus;
    int m_totalGoldSpentAtShops;
};
