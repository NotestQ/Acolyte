#pragma once
#include <Vagante/sfml.h>
#include <Vagante/forward.h>
#include <vector>
#include <set>

#pragma warning(push)
#pragma warning(disable: 26495) // always initialize a member variable

struct ConvexBody {
	float x;
	float y;
	float width;
	float height;
	float dx;
	float dy;
	float offsetX;
	float offsetY;
	std::vector<std::pair<float, float>> sides;
};

struct bvhAABB {
	sf::Vector2<float> lowerBound;
	sf::Vector2<float> upperBound;
};

struct bvhTreeNode {
    bvhAABB aabb;
    std::shared_ptr<Entity> userData;
    union {
        int parent;
        int next;
    };
    int child1;
    int child2;
    int height;
};

struct bvhTree {
    struct CollisionPair {
        int nextFree;
        int aId;
        int bId;
        std::shared_ptr<Entity> a;
        std::shared_ptr<Entity> b;
    };

    int m_root;
    std::vector<bvhTreeNode> m_nodes;
    int m_nodeCount;
    int m_nodeCapacity;
    int m_freeList;
    int m_path;
    int m_insertionCount;
    std::set<int> m_moveBuffer;
    std::set<int> m_deleteBuffer;
    std::vector<std::pair<int, int>> m_newPairBuffer;
    int m_freePairList;
    std::vector<bvhTree::CollisionPair> m_pairs;
};

#pragma warning(pop)
