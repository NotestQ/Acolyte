#pragma once
#include <Vagante/sfml.h>
#include <vector>

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

#pragma warning(pop)