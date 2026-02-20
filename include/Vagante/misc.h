#pragma once

struct paddedVtable {
    void* vtable;
    int _padding;
};

struct vtable {
    void* vtable;
};

struct Tween {
    TweenType type;
    float time;
    float b;
    float c;
    float d;
    float endWaitTime;
    float pauseTime;
};