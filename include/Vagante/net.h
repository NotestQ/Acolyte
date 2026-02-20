#pragma once
#include <Vagante/enums.h>
#include <Vagante/forward.h>
#include <Vagante/sfml.h>

struct NetGameEvent {
    NetGameEventType eventType;
    int ignorePlayerIndex;
    int exclusivePlayerIndex;
    float x;
    float y;
    int weaponFrame;
    int numWeaponFrames;
    bool justStartedFrame;
    float organicDamageDealt;
    std::string text_s;
    int text_type;
    int text_netid;
    bool text_playerText;
    bool text_critical;
};

struct DamageEventMeta {
    int	damageAmount;
    DamageType type;
    bool critical;
    short _padding;
    std::shared_ptr<Entity> source;
    std::shared_ptr<Entity> instigator;
};

struct NetHurtEvent {
    int	damage;
    DamageType damageType;
    bool critical;
    int	sourceNetId;
    int	instigatorNetId;
};

struct NetReflectBlockEvent {
    int sourceUnitNetId;
    DamageEventMeta meta;
};

struct NetTeleportEvent {
    sf::Vector2<float> from;
    sf::Vector2<float> to;
    bool teleFrag;
};

struct NetChaosEvent {
    float castStrength;
    int luckModifier;
};

struct NetModSpaceTimeEvent {
    bool value;
    sf::Vector2<float> playerPosition;
};

struct InterpInformation {
    float x;
    float y;
    float dx;
    float dy;
    uint flags;
    uint flags2;
    uint flags3;
    uint flags4;
    uint flags5;
    uint flags6;
    int frames;
};

struct WeaponNetSwingMeta {
    int parentItemNetId;
    bool rodCanOvercharge;
    bool swordPogo;
    bool swordOverheadSwing;
    bool daggerThrown;
    bool cqcUppercut;
    bool cqcDown;
    bool isPummeling;
    bool facingLeft;
    bool projectileReloaded;
    bool swordSpin;
    bool godFist;
};

struct Rpc {};

struct LightEvent {};