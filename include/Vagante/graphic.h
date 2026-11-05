#pragma once
#include <Vagante/enums.h>
#include <Vagante/sfml.h>
#include <Vagante/forward.h>

struct Sprite: public sf::Transformable {
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

struct FancyText {
    sf::String s;
    sf::Color color;
    sf::Style style;
    float height;
    float overflow;
};

struct GuiElement {
    struct VTable {
        void* (__thiscall* destroy)(GuiElement* elementThis, uint flags);
        void (__thiscall* action)(GuiElement* elementThis, bool down);
        void (__thiscall* mouseAction)(GuiElement* elementThis, int x, int y, bool down, bool leftButton);
        void (__thiscall* step)(GuiElement* elementThis, int mouseX, int mouseY);
        void (__thiscall* afterStep)(GuiElement* elementThis, bool leftHeld, bool rightHeld);
        void (__thiscall* draw)(GuiElement* elementThis, sf::RenderTarget* target);
        void (__thiscall* setOffset)(GuiElement* elementThis, float x, float y);
    };

    VTable* vtable;
    bool hover;
    GuiElementType type;
    sf::Vector2<float> cursorOffset;
    std::string tooltip;
    bool hidden;
    bool pressed;
    bool disabled;
    int x;
    int y;
    int w;
    int h;
    sf::Vector2<float> offset;
    float alphaModifier;
};

struct GuiTextContainer : public GuiElement {
    std::vector<FancyText> buffer;
    bool reverseYAnchor;
    bool showBackground;
    float alpha;
    float padding;
    Sprite nineSliceSprite;
    float scale;
};
