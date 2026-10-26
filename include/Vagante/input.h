#pragma once
#include <Vagante/graphic.h>
#include <map>

struct ActionBind {
    bool isKey;
    sf::Keyboard::Key key;
    uint joyButton;
    sf::Joystick::Axis joyAxis;
    bool joyAxisPositive;
    bool joyUsingAxis;
    bool bound;
};

struct InputProfile {
    std::string profileName;
    std::string filename;
    bool keyboard;
    std::vector<ActionBind> bindings;
    std::vector<ActionBind> secondaryBindings;
    bool isDefault;
};

struct ControllerIdentifier {
    bool keyboard;
    bool steamController;
    int joystickId;
    ulong64 steamControllerHandle;
    std::string name;
};

struct ControlScheme {
    int slot;
    std::vector<ActionBind> keyBindings;
    std::vector<ActionBind> joystickBindings;
    std::vector<ActionBind> alternateJoystickBindings;
    std::vector<bool> binded;
    std::vector<bool> actions;
    std::vector<bool> prevActions;
    bool enabled;
    bool binding;
    int bindTimer;
    int currentlyBinding;
    bool bindingAlternate;
    bool justBinded;
    sf::Keyboard::Key lastBindedKeyEvent;
    ActionBind lastBindedAction;
    sf::Keyboard::Key currKey;
    uint currButton;
    sf::Joystick::Axis currAxis;
    bool currAxisPositive;
    bool bindingAxis;
    std::vector<sf::Joystick::Axis> validAxes;
    int controllerChooseTimer;
    bool usingJoystick;
    bool usingSteamController;
    bool tryToUseJoystick;
    uint joystickId;
    ulong64 steamControllerHandle;
    ControlsType controlsType;
    bool specialL2R2;
    std::string joystickName;
    uint joystickVendorId;
    uint joystickProductId;
};

struct Input {
    ulong64 steamControllerActionHandles[15];
    ulong64 steamControllerActionSetHandle;
    std::vector<bool> keys;
    std::vector<bool> prevKeys;
    std::vector<ControllerIdentifier> controllersDetected;
    std::vector<std::string> debugStates;
    std::vector<std::string> actionNames;
    std::vector<InputProfile> inputProfiles;
    std::map<sf::Keyboard::Key, int> keysBoundCount;
    std::vector<ControlScheme> controllers;
    std::vector<float> controllerSwitchedTimers;
    std::vector<float> keyboardSwitchedTimers;
    bool joystickDetected;
    bool enteringText;
    bool enteredTextPressedReturn;
    bool skipInputThisFrame;
    bool someConnectEvent;
    sf::Window* window;
    std::string* textBuffer;
    bool allowNewLines;
    Sprite genericSprite;
    Sprite steamSprite;
    Sprite xbox360Sprite;
    Sprite keyboardSprite;
    Sprite xboxOneSprite;
    Sprite ps3Sprite;
    Sprite ps4Sprite;
    int textCursorIndexStart;
    int textCursorIndexEnd;
    bool steamControllerInitialized;
    bool controllerSpritesInitialized;
    std::string copyBuffer;
};
