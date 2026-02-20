#pragma once
#include <Vagante/enums.h>
#include <Vagante/forward.h>
#include <Vagante/effects.h>
#include <Vagante/net.h>
#include <Vagante/ai.h>
#include <unordered_set>
#include <map>

struct Tile {
    signed char type;
    int goalType;
    short backgroundIndex;
    short fargroundIndex;
    short midgroundIndex;
    short foregroundIndex;
    short closegroundIndex;
    bool indestructible;
    bool solid;
    bool left;
    bool up;
    bool right;
    bool down;
    bool platform;
    bool explored;
    bool magicRevealed;
    int magicRevealDelay;
    short minLight;
    short light;
    int waterElectricEffect;
};

struct TileChange {
    int x;
    int y;
    bool solid;
    bool updateDisplay;
    bool isPlatformUpdate;
};

struct TileUnexploreMeta {
    sf::Vector2<int> location;
    uint distanceToPlayers;
};

struct WaterChange {
    uint index;
    uchar mass;
};

struct HeistItemAttributes {
    int type;
    uint randIndex;
};

struct Cutscene {
    sf::Vector2<float> cameraCenter;
    Level* level;
    sf::View lastKnownView;
    sf::Color fadeColor;
    Scene scene;
    uint fadingFrames;
    uint fadingFramesTotal;
    bool fadingOut;
};

struct Room {
    char padding[0xec];
};

struct Level {
    uint seed;
    bool intermission;
    IntermissionType intermissionType;
    bool intro;
    bool alternateMusic;
    bool isBeingTested;
    bool customWorldLevel;
    bool customWorldHasTrees;
    std::string customWorldLevelName;
    std::string customWorldMusic;
    std::string customWorldBossMusic;
    std::vector<TileChange> tileChanges;
    std::vector<WaterChange> waterChanges;
    bool addWaterChanges;
    uint sendWaterIndex;
    std::vector<NetGameEvent> netGameEvents;
    bool particlesEnabled;
    sf::Color bgColor;
    sf::Color fogColor;
    int widthOfSingleRoom;
    int heightOfSingleRoom;
    int roomW;
    int roomH;
    int w;
    int h;
    uint floorIndex;
    uint levelAct;
    int difficultyPlayers;
    bool newGamePlus;
    bool isBranch;
    bool netInBossFight;
    int bossType;
    bool bossKilled;
    short _padding;
    std::map<int, int> monstersKilled;
    bool fairyLevel;
    short __padding;
    /*std::vector<bool>*/char fairiesSaved[16];
    int bossRoomX;
    int bossRoomY;
    int shopRoomX;
    int shopRoomY;
    SkyType skyType;
    bool openSky;
    bool openLeftSide;
    bool openRightSide;
    std::vector<sf::Vector2<int>> entranceLocations;
    sf::Vector2<int> exitLocation;
    sf::Vector2<int> branchLocation;
    sf::Vector2<int> bossChestLocation;
    bool isTutorial;
    float shakeAmount;
    float shakeTimer;
    int levelFeelTimer;
    Room*** rooms;
    std::vector<std::shared_ptr<Entity>> debugTargets;
    bool rendezvousWithThief;
    HeistItemAttributes heistItem;
    std::shared_ptr<Doodad> entranceDoodad;
    bool cumulativeCombatTextDamage;
    uint frame;
    uchar* fluid;
    uchar* prevFluid;
    bool doFluidThreadUpdate;
    bool cleanUpFluidThread;
    int debugUpdateCount;
    int fluidWidth;
    int fluidHeight;
    std::vector<sf::Vertex> waterQuads;
    std::vector<int>* pieties;
    std::vector<std::vector<int>>* sacrificedItemTypes;
    std::vector<std::shared_ptr<Player>>* players;
    int* playerExperience;
    int* playerGold;
    std::vector<int> amountHealedPerPlayer;
    bool bonfireLit;
    float whiteFog;
    float towerWaterLevel;
    PathManager pathManager;
    RetargetManager retargetManager;
    sf::Vector2<float> viewCenter;
    bool acidWater;
    bool clientSide;
    bool serverSide;
    float gravity;
    ParticlePool particlePool;
    sf::RenderTexture* minimapTexture;
    Sprite minimapIconsSprite;
    Sprite bgTilesSprite;
    Sprite farGTilesSprite;
    Sprite fgTilesSprite;
    Sprite mgTilesSprite;
    Sprite cgTilesSprite;
    sf::RenderStates tileStatesBg;
    sf::RenderStates tileStatesFarg;
    sf::RenderStates tileStatesFg;
    sf::RenderStates tileStatesMg;
    sf::RenderStates tileStatesCg;
    std::vector<sf::Sprite*> parallaxBgSprites;
    std::vector<Sprite> trees;
    std::vector<std::pair<int, int>> minimapTilesToUpdate;
    std::set<std::pair<int, int>> purpleTiles;
    float** serverSideWaterMasses;
    Tile** tiles;
    sf::VertexArray fog;
    int numFogPoints;
    int** cornerLightVals;
    bool** cornerLightValsAlreadySet;
    std::vector<LightEvent> lightEvents;
    std::queue<sf::Vector2<int>> magicRevealTiles;
    std::unordered_set<sf::Vector2<int>> alreadyRevealedTiles;
    bool magicallyRevealing;
    LevelFeel levelFeel;
    std::shared_ptr<HoverText> levelFeelHoverText;
    std::vector<std::shared_ptr<Entity>>* entities;
    ItemGenerator* itemGenerator;
    std::vector<std::vector<sf::VertexArray>> bgVertices;
    std::vector<std::vector<sf::VertexArray>> farGVertices;
    std::vector<std::vector<sf::VertexArray>> mgVertices;
    std::vector<std::vector<sf::VertexArray>> fgVertices;
    std::vector<std::vector<sf::VertexArray>> cgVertices;
    int startx;
    int starty;
    float truex;
    float truey;
    float radius;
    int oldCastX;
    int oldCastY;
    float oldCastRadius;
    ushort currEntityId;
    short ___padding;
    std::set<std::shared_ptr<Entity>> activeBosses;
    std::map<int, std::shared_ptr<Entity>>* netEntities;
    std::vector<sf::Vector2<float>> arenaGateCheckQ;
    std::vector<std::vector<std::shared_ptr<Entity>>> antiFactions;
    Cutscene cutscene;
    bool triggerCredits;
    bool triggerGoodEnding;
    bool endOfCampaign;
    float tileCountToUnexplore;
    bool runeOrbTakenRuins;
    bool runeOrbTakenTower;
    std::vector<TileUnexploreMeta> tileMetaList;
    std::vector<Rpc> reliableClientToServerRpcs;
    bool hardMode;
    float towerMaxWaterLevel;
    sf::Vector2<int> editorExcludeStart;
    sf::Vector2<int> editorExcludeEnd;
};

struct HalloweenEventLocations {
    sf::Vector2<float> position;
    HalloweenEventLocationType type;
    bool facingLeft;
    uint shakeFrames;
};