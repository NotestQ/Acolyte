#pragma once
#include <Vagante/sfml.h>
#include <Vagante/forward.h>
#include <string>

struct Sprite {
    char padding[0xa8];
    bool flipped;
    bool isEmpty;
    sf::Vertex vertices[4];
    sf::Texture* texture;
    sf::Rect<int> textureRect;
    int w;
    int h;
    int frame;
    int numFrames;
    int timer;
    int speed;
    int resetDelay;
    sf::Vector2<float> offset;
    bool reverse;
    bool frozen;
    bool loop;
    bool done;
    int rows;
    int cols;
    bool tileProcessed;
    std::string m_textureName;
    std::string m_paletteGroupName;
    std::string m_paletteName;
    std::weak_ptr<Entity> m_ownerEntity;
};
