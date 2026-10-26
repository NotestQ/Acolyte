#pragma once
#include <Vagante/enums.h>
#include <Vagante/forward.h>
#include <Vagante/sfml.h>
#include <map>

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

// Steam IDs are byte-aligned in the game's network structures.
#pragma pack(push, 1)
struct CSteamID {
    ulong64 value;
};
#pragma pack(pop)

struct SteamCallback {
    void* vtable;
    uchar flags;
    int callback;
};

struct SteamCallResult: public SteamCallback {
    ulong64 handle;
    void* object;
    void* function;
};

struct LobbyEnter_t {
    ulong64 m_ulSteamIDLobby;
    uint m_rgfChatPermissions;
    bool m_bLocked;
    uint m_EChatRoomEnterResponse;
};

struct Snapshot {
    std::vector<EntityState> netEntities;
    int parseEntitiesNum;
    int numEntities;
    bool valid;
    ulong64 serverTime;
    int messageNum;
    int deltaNum;
    int cmdNum;
    int floorIndex;
};

struct Netchan {
    bool usesSteamNetworking;
    CSteamID remoteAddress;
    sf::UdpSocket* udpSocket;
    sf::TcpSocket* tcpSocket;
    bool tcpSocketSetByServer;
    sf::IpAddress remoteIp;
    ushort remoteTcpPort;
    ushort remoteUdpPort;
    std::map<int, ulong64> sequenceToTime;
    ulong64 lastReceivedPacket;
    uint incomingSequence;
    uint outgoingSequence;
    uint fragmentSequence;
    ushort fragmentLength;
    uchar fragmentBuffer[16384];
    bool unsentFragments;
    ushort unsentFragmentStart;
    int unsentLength;
    uchar unsentBuffer[16384];
    bool isServerSide;
    ulong64 currentPing;
    ulong64 targetPing;
    ulong64 smoothedRoundTripTime;
    std::vector<int> graphBytesIn;
    std::vector<int> graphBytesInUncompressed;
    std::vector<int> graphBytesOut;
    std::vector<int> graphBytesOutUncompressed;
    std::vector<ulong64> graphPing;
    int graphPosition;
};
