#pragma once

struct ESoundBuffer {
    char padding[0x28];
    float sourceRandomPitchModifier;
    float sourceVolumeModifier;
    float sourceDefaultAttenuation;
    bool hasCustomDefaultAttenuation;
    std::string filename;
    bool playedThisFrame;
};

struct ESound {
    char padding[0x1c];
    ESoundBuffer* e_buffer;
    bool e_global;
    float e_volume;
    float e_attenuation;
    float e_specificVolumeFactor;
    float e_spatialVolumeFactor;
    sf::Vector2<float> e_gamePosition;
};