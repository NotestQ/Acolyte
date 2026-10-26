#pragma once
#include <Vagante/save.h>
#include <Vagante/ui.h>
#include <unordered_map>

struct CollisionBucket {
    std::vector<std::shared_ptr<Entity>> entityList;
};

struct GameEngine {
    struct AltLevel {
        Level* level;
        std::vector<std::shared_ptr<Entity>> entities;
    };

    PlayerCustomizeMenu playerCustomizeMenu;
    ItemGenerator itemGenerator;
    LevelGenerator levelGenerator;
    RoomDatabase* customRoomDatabase;
    std::vector<GameEngine::AltLevel*> altLevels;
    std::vector<std::shared_ptr<PlayerMenu>> playerMenus;
    std::vector<std::shared_ptr<Grave>> graves;
    std::vector<std::shared_ptr<Entity>> entities;
    std::unordered_map<int, CollisionBucket> collisionBuckets;
    bvhTree collisionTree;
    int numPlayers;
    std::vector<int> pieties;
    std::vector<std::vector<int>> sacrificedItemTypes;
    uint floorIndex;
    uint frameCount;
    bool intro;
    bool introLeft;
    bool intermission;
    bool goToTutorial;
    bool goToBranch;
    bool goToArena;
    bool goToFinalFight;
    bool doNewGame;
    bool newGameUseParams;
    int intermissionTimer;
    IntermissionType intermissionType;
    bool tutorial;
    std::weak_ptr<Fairy> intermissionFairy;
    std::vector<bool> fairiesSaved;
    int intermissionKeepers;
    int shopKeeperChaseCount;
    ushort shopKeeperChaseTimer;
    bool shopKeepersAngered;
    Tween fadeInTween;
    Tween fadeOutTween;
    bool fadeIn;
    bool fadeOut;
    bool gameOver;
    bool gameComplete;
    int gameOverTimer;
    int prevProgressionPoints;
    int playerLevel;
    int experience;
    int gold;
    int goldReserve;
    int score;
    int bossesKilled;
    std::map<EntityType, int> monstersKilled;
    std::vector<Superlative> superlatives;
    int totalGoldSpentAtShops;
    uint prevAct;
    uint prevFloor;
    uint itemSeed;
    uint levelSeed;
    bool saveAndQuit;
    std::string saveGameFilename;
    bool requestGameSaveData;
    bool loadGame;
    GameSave saveToLoad;
    bool insideNewGamePlusTransition;
    bool networked;
    bool customGameClientSide;
    bool clientSide;
    bool receivedNetworkDataForNewLevel;
    bool gameStarted;
    bool netClassChosen;
    std::string customWorldExitDestination;
    Level* currLevel;
    std::vector<std::shared_ptr<Player>> players;
    std::weak_ptr<LoadSavePoint> requestingLoadPoint;
    std::weak_ptr<Lever> hardModeLever;
};
