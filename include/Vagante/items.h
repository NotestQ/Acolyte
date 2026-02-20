#pragma once
#include <Vagante/sfml.h>
#include <Vagante/forward.h>
#include <Vagante/enums.h>
#include <Vagante/graphic.h>
#include <Vagante/entities.h>
#include <Vagante/misc.h>
#include <vector>
#include <string>
#include <memory>
#include <map>

#pragma warning(push)
#pragma warning(disable: 26495) // always initialize a member variable

namespace AttackScriptDatabase {
    struct EffectCue {
        EffectCueType type;
        bool beginAtFrameStart;
        bool onlyExecuteOnWeaponStopSwing;
        bool onlyExecuteOnWeaponReload;
        uchar frameDuration;
        sf::Vector2<float> position;
        sf::Vector2<float> aabbExtent;
        float radius;
        std::string sfxFileName;
        float shakeAmount;
        uchar shakeTime;
        uchar particleType;
        uchar particleCount;
        uint particleBits;
    };

    struct AttackShape {
        std::vector<sf::Vector2<float>> vertices;
        bool enabled;
        int damageMin;
        int damageMax;
        float knockback;
        ushort stunDuration;
        AttackHitSound defaultHitSound;
    };

    struct AttackScriptKeyframe {
        ushort frames;
        int weilderSpriteFrame;
        sf::Vector2<float> origin;
        float originRotationDegrees;
        sf::Vector2<float> aimOrigin;
        sf::Vector2<float> weaponGraphicalScale;
        sf::Vector2<float> projectileMidOrigin;
        float weaponDurationFactor;
        bool enabled;
        bool isChargeFrame;
        bool releaseProjectiles;
        std::vector<AttackShape> shapes;
        std::vector<EffectCue> effectCues;
    };

    struct AttackScript {
        std::string attackScriptName;
        std::vector<AttackScriptKeyframe> keyframes;
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
        std::vector<AttackShape> shapes;
    };
}

struct ItemAffix {
    std::string prefix;
    std::string suffix;
    std::string noun;
};

struct ItemAttribute {
    ItemAffix affix;
    std::string descriptor;
    ItemAffix cursedAffix;
    std::string cursedDescriptor;
    AttributeType type;
    float val;
    bool isNegative;
    bool weaponSpecific;
    bool base;
    bool hasCursedVersion;
    bool smithBlessing;
};

struct ItemProperties {
    std::string unidName;
    std::string unidDescription;
    uint spriteId;
};

struct ItemRarity {
    int minItemLevel;
    int maxItemLevel;
    float rarityPart;
    ItemType type;
};

struct ItemPoolRarity {
    ItemPoolType type;
    float rarity;
};

struct ItemPool {
    std::vector<ItemRarity>	rarities;
    std::vector<ItemPoolRarity>	subItemPools;
};

struct ItemGenerator {
    uint seed;
    std::vector<ItemProperties> potentialPotionProperties;
    std::map<ItemType, uint> potionProperties;
    std::vector<ItemProperties> potentialScrollProperties;
    std::map<ItemType, uint> scrollProperties;
    std::map<ItemType, bool> identified;
    std::map<ItemPoolType, ItemPool> itemPools;
    std::set<ItemType> uniqueItemTypes;
    std::set<ItemType> spawnedUniqueItemTypes;
    std::set<AttributeType> spawnedUniqueItemAttributeTypes;
    std::vector<std::string> magicPrefixes;
    std::vector<std::string> cursedMagicPrefixes;
    std::vector<std::string> artifactPrefixes;
    std::vector<std::string> artifactPrefixes2;
    std::vector<std::string> artifactNouns;
    std::set<std::string> usedArtifactNames;
    std::vector<SkillType> lastSpellbooksSacrificed;
    bool increasedCurseChance;
};

struct Item: public Entity {
    bool isBossChestReward;
    bool hasBeenPickedUpBefore;
    bool showedAnimationWhenPickedUp;
    bool showedEffectsWhenPickedUp;
    bool isCarried;
    bool usable;
    bool appliedToItem;
    bool equipable;
    bool godGift;
    bool xbowDrawn;
    uint equipSlot;
    bool unique;
    bool artifact;
    bool droppable;
    ItemType itemType;
    std::vector<ItemAttribute> attributes;
    bool resolvingBackInBounds;
    int collidingObstacles;
    int prevCollidingObstacles;
    std::weak_ptr<Entity> owner;
    bool serverDead;
    bool inventoryHover;
    uchar networkAttributesVersion;
    uchar clientAttributesVersion;
    ulong64 requestedAttributesTime;
    int deathTimer;
    bool sacrificed;
    bool stackable;
    int itemLevel;
    float cost;
    int droppedTimer;
    bool isMaskedUsable;
    bool droppedByPlayer;
    bool insideShrineOrChest;
    char _padding_;
    std::weak_ptr<Doodad> pedestal;
    std::weak_ptr<Projectile> attachedProjectile;
    uint count;
    bool baseAttributesSet;
    int godGiftTimer;
    sf::Vector3<float> tintColor;
    uint randIndex;
    uint frame;
    ItemGenerator* itemGenerator;
    VFont* tinyFont;
    Sprite* sprite;
    Sprite* droppedSprite;
    Sprite* droppedSpriteMask;
    Sprite* inventorySpriteMask;
    int spriteIndex;
    Sprite iconEquipSprite;
    ItemAffix affix;
    std::string name;
    std::string description;
    std::string useEffectText;
    bool invisible;
    float drawScale;
    float inInventoryScale;
    Tween drawScaleGrowTween;
    Tween drawScaleShrinkTween;
    bool fakeIdentified;
    bool identified;
    bool deadIdentified;
    int identifyEffectTimer;
    int curseEffectTimer;
    int uncurseEffectTimer;
    int equipEffectTimer;
    int godHitEffectTimer;
    int hoverEffectTimer;
    bool cursed;
    bool permanentCursed;
    bool touchingPlayer;
    bool belongsToShopkeeper;
    uchar wandCharges;
    uchar maxWandCharges;
    int outlineEffectTimer;
    int shopSelectTimer;
    std::weak_ptr<Player> holder;
    Skill* skill;
    std::weak_ptr<Entity> holderEntity;
    GuiTextContainer* textContainer;
    std::vector<std::weak_ptr<Item>> storedItems;
    bool isStoredItem;
};

struct alignas(8) Weapon: public Entity {
    bool cursed;
    bool godCursed;
    bool isCritical;
    int hitEffectFrames;
    bool swordSlideAttacking;
    bool cursedAccuracy;
    bool extraExplosive;
    bool enableRodLeechCast;
    bool enableRodCast;
    bool oneDamage;
    float wielderIas;
    int speed;
    bool aimingUp;
    bool aimingDown;
    bool backSwing;
    float wandBreakChance;
    int bonusMagicDamage;
    float bonusDamageMultiplier;
    ItemType ammoType;
    bool canActivateWand;
    int bonusMinDamage;
    int bonusMaxDamage;
    int wandTypeFlags;
    float stunBonus;
    uint wandCharges;
    float elementalDmgAccumulationFire;
    float elementalDmgAccumulationIce;
    float elementalDmgAccumulationElec;
    std::shared_ptr<Unit> parent;
    Sprite* sprite;
    Sprite* aimArmSprite;
    std::shared_ptr<Projectile> currProjectile;
    std::shared_ptr<SpecialEffect> lightningEffect;
    bool facingLeft;
    bool swinging;
    float playerKnockback;
    int wielderDamage;
    bool maxDamageBonus;
    bool minDamageBonus;
    int backstabModifier;
    uchar frame;
    int currSwingFrame;
    int totalSwingFrames;
    int frameTimer;
    bool chargingAttack;
    int chargeAttackFrame;
    int chargeAttackTimer;
    int chargeEffectTimer;
    bool shadow;
    int swordPogoCancelFrames;
    bool doMiss;
    bool doDaggerThrow;
    bool isDaggerThrown;
    int daggerThrowTimer;
    bool daggerMastery;
    bool daggerResetHit;
    bool slowsEnemies;
    bool fairyFire;
    bool cursedFairyFire;
    bool netInterpPlayedSFX;
    short _padding_;
    std::map<std::shared_ptr<Entity>, float> hitDegrees;
    std::map<std::shared_ptr<Entity>, uint> hitCounts;
    bool haveTargetAngle;
    float targetAngle;
    float aimAngleAccel;
    float aimAngle;
    float aimAngleStart;
    bool prevAimingUp;
    uint aimFrames;
    bool hasDoneDamage;
    bool hasDoneNewDamage;
    float totalRealDamageDealt;
    float totalOrganicDamageDealt;
    int maxAttackCharge;
    bool m_clearPotentialTargets;
    std::vector<ItemAttribute> itemAttributes;
    bool haveCastedRod;
    std::vector<Sprite> afterImages;
    uint stepFrame;
    bool discardProjectileOnCancel;
    AttackScriptDatabase::AttackScript* attackScript;
    AttackScriptDatabase::WeaponProperties* weaponProperties;
    std::vector<AttackScriptDatabase::AttackShape> attackShapeCacheCombined;
    std::vector<AttackScriptDatabase::AttackShape> attackShapeCacheCurrent;
    std::vector<AttackScriptDatabase::AttackShape> attackShapeCachePrevious;
    sf::Vector2<float> attackScriptOrigin;
    float weilderSpriteOffsetX;
    bool enableEffectCues;
    bool isSwordPogo;
    bool isSwordSpin;
    bool netSwordPogoHitGround;
    bool projectileReloaded;
    bool startedSwingLoaded;
    bool projectilesFired;
    int minDamage;
    int maxDamage;
    float totalDamageFactor;
    float attackShapeSizeModifier;
    float wielderRotationDegrees;
    bool pummeling;
    bool newPummelingSwing;
    bool stopSwingTriggered;
    bool thrownDaggerRetrieved;
    bool daggerThrowFastReturn;
    float meleeAttackTurnStrength;
    bool godFistAttackNextSwing;
    bool godFistAttack;
    bool aimArmSpriteGlow;
};

#pragma warning(pop)