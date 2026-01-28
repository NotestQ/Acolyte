#pragma once
#include <Vagante/sfml.h>
#include <Vagante/sprite.h>
#include <Vagante/stage.h>

struct Particle {
    char padding[0x78];
};

struct ParticlePool {
    uint poolSize;
    Particle* pool;
    Level* level;
    Particle* firstAvailable;
    Sprite sprite;
};

struct RiftParticle {
    sf::Vector2<float> position;
    sf::Vector2<float> scale;
    float rotationDegrees;
    sf::Color color;
};

struct WaveEcho {
    sf::Vector2<float> midPosition;
    sf::Color color;
    float alphaFade;
};

struct SpecialEffect {
    char padding[0x240];
};