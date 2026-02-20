#pragma once
#include <Vagante/enums.h>
#include <Vagante/forward.h>
#include <Vagante/sfml.h>
#include <queue>
#include <set>

struct PathNode {
    int row;
    int col;
    MoveMethod moveMethod;
    float f;
    float g;
    float h;
    int parent[3];
    uint pathDepth;
    uint pathFindCallNumberClosed;
};

struct PathRequest {
    std::shared_ptr<Monster> caller;
    PathNode goalNode;
    uint max_search_depth;
    bool reverseDirection;
    bool optimize;
    std::vector<PathNode>* pathOutputAddress;
    bool pathRequestStarted;
};

struct PathManager {
    PathNode*** tileCache;
    uint tileCacheSize[3];
    Level* m_level;
    std::queue<PathRequest> pathRequests;
    std::vector<std::pair<std::shared_ptr<Monster>, uint>> requesterFrameCooldowns;
    uint pathFindCallCount;
    uint tilesProcessedThisFrame;
    std::set<PathNode> openSet;
    std::priority_queue<PathNode/*, PathManager::PathNodeComparator*/> openQueue;
};

struct RetargetRequest {
    std::shared_ptr<Monster> caller;
    bool ignoreLineOfSight;
    float maxEuclideanDistanceBetweenMidpoints;
    std::vector<std::weak_ptr<Unit>> ignoreUnits;
};

struct RetargetManager {
    uint frame;
    std::queue<RetargetRequest> retargetRequests;
    Level* m_level;
    std::vector<std::pair<std::shared_ptr<Monster>, uint>> requesterFrameCooldowns;
};