#pragma once
#include <Vagante/sfml.h>
#include <Vagante/forward.h>
#include <Vagante/enums.h>
#include <Vagante/sprite.h>
#include <vector>
#include <string>
#include <memory>
#include <map>
#include <Vagante/entities.h>

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

struct Item: public Entity {
    char padding[0x368];
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