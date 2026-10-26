#pragma once
#include <Vagante/graphic.h>
#include <Vagante/forward.h>
#include <Vagante/net.h>
#include <tuple>
#include <map>
#include <stack>
#include <functional>

struct GuiButton: public GuiElement {
    bool centered;
    bool isATab;
    sf::Color fgColor;
    std::string name;
    bool hasBackground;
    bool toggleState;
    bool doAction;
    bool rightClick;
    Sprite buttonSprite;
    Sprite tabSprite;
    uint hoverTime;
};

struct GuiCheckbox: public GuiElement {
    bool useRightLabel;
    bool checked;
    std::string label;
    Sprite checkboxSprite;
};

struct GuiLabel: public GuiElement {
    bool alignRight;
    std::string label;
    sf::Color color;
};

struct GuiSelect: public GuiElement {
    int fontSize;
    float lineH;
    int maxSelected;
    int minSelected;
    uint hoverIndex;
    uint cursor;
    std::vector<std::string> entries;
    std::vector<bool> selected;
    Sprite selectHoverSprite;
    Sprite backgroundSprite;
    bool doAction;
};

struct GuiSlider: public GuiElement {
    bool doAction;
    float value;
    std::string label;
    Sprite sliderSprite;
};

struct GuiTextBox: public GuiElement {
    bool focus;
    bool useAboveLabel;
    Input* input;
    bool doAction;
    std::string bgLabel;
    std::string label;
    uint frame;
    bool bound;
    std::string* boundBuffer;
};

struct GuiWindow: public GuiElement {
    std::vector<GuiElement*> elements;
    bool hasBg;
};

struct GuiCursor {
    float x;
    float y;
    float alphaModifier;
    GuiElement* target;
    Sprite selectHoverSprite;
    Sprite selectGrabSprite;
};

struct PlayerCustomizeMenu {
    bool hidden;
    bool open;
    bool hasFocus;
    int gameMode;
    uint frame;
    float caravanDarkness;
    float caravanPosition;
    float caravanPositionY;
    bool stopped;
    std::vector<uint> currentPlayer;
    bool selectingClass;
    uint currentClass;
    std::vector<std::shared_ptr<Player>>* players;
    std::vector<std::shared_ptr<PlayerMenu>>* guis;
    Sprite caravanSprite;
    std::shared_ptr<Caravan> caravan;
    std::vector<PlayerClass> playerClasses;
    std::vector<uint> playerColors;
    std::vector<int> playerPalettes;
    bool networked;
};

struct PlayerMenu {
    bool networked;
    bool serverside;
    int networkedPlayerIndex;
    std::string netPlayerName;
    std::shared_ptr<Player> player;
    PlayerCustomizeMenu* playerCustomizeMenu;
    Config* config;
    Input* input;
    std::shared_ptr<Player> alivePlayer;
    uint playerIndex;
    sf::Vector2<float> chatBoxOffset;
    uint frame;
    uint randIndex;
    VFont* tinyFont;
    sf::Sprite* steamAvatarSprite;
    Sprite* causeOfDeathSprite;
    Sprite healthBgNetNoClassSprite;
    Sprite healthBgSprite;
    Sprite equipSlotsSprite;
    Sprite equipSlotsQuantitySprite;
    Sprite tabsSprite;
    Sprite tabsGreySprite;
    Sprite tabsHighlightSprite;
    Sprite baseInventorySprite;
    Sprite baseAffinitiesSprite;
    Sprite affinityLevelBg;
    Sprite affinityLevelBgConnector;
    Sprite affinityLevelIcon;
    Sprite affinityLevelSelected;
    Sprite baseSpellsSprite;
    Sprite spellControlsSprite;
    Sprite spellQuickConfigSprite;
    Sprite baseStatsSprite;
    Sprite baseCustomizeSprite;
    Sprite backgroundIconsSprite;
    Sprite classIconsSprite;
    Sprite classSelectArrowsSprite;
    Sprite classTextSprite;
    Sprite baseGameOverSprite;
    Sprite iconLockSprite;
    Sprite iconPlusSprite;
    Sprite miniIconsSprite;
    Sprite plusStatSprite;
    Sprite selectHoverSprite;
    Sprite selectGrabSprite;
    float subMenuHeight;
    sf::Vector2<float> menuPosition;
    sf::Vector2<float> barSpritePos;
    sf::Vector2<float> portraitPos;
    sf::Vector2<float> hpBarPos;
    sf::Vector2<float> hpTextPos;
    sf::Vector2<float> mpTextPos;
    sf::Vector2<float> statusEffectPos;
    sf::Vector2<float> equipmentPos;
    sf::Vector2<float> namePos;
    sf::Vector2<int> currAffinity;
    sf::Vector2<int> customizeCursor;
    int lastMovedCursorFrames;
    SubSkillMenu subSkillMenu;
    bool assigningSkillControl;
    int currSkill;
    int selectedSkillControl;
    int selectedQuickConfig;
    GuiCheckbox enableQuickSpellCast;
    GuiCheckbox enableQuickSpellSwap;
    GuiLabel lblConfigQuickSpells;
    sf::Vector2<int> statSelector;
    float healthBarSize;
    int toolTipShowDelay;
    float tabFade;
    std::vector<float> slotPulseAmount;
    std::vector<float> tabPulseAmount;
    bool forceHudUpdate;
    float prevExtendAmount;
    float extendAmount;
    bool extending;
    bool retracting;
    int backgroundSwitchedTimer;
    int classSwitchedTimer;
    int classSwitchDirection;
    int prevNumAffinityPoints;
    uint levelUpAnimationTimer;
    uint useAffinityPointTimer;
    uint switchAffinityTimer;
    GameOverMenuState gameOverMenuState;
    int gameOverAnimateTimer;
    bool hidePressStartButton;
    bool inClassSelectionArea;
    bool choosingClass;
    bool classChosen;
    bool clientExists;
    bool menuOpen;
    bool hidden;
    MenuState state;
    bool affinityMenuOpen;
    std::vector<std::tuple<Sprite, Sprite, Sprite>> availablePalettes;
    std::vector<std::tuple<Sprite, Sprite, Sprite>> petPalettes;
    std::vector<int> paletteFxSitTimer;
    std::vector<std::pair<BackgroundType, bool>> availableBackgrounds;
    std::vector<PlayerClass> availableClasses;
    BackgroundType playerBackground;
    int selectedBackgroundIndex;
    BackgroundType prevBg;
    int playerPalette;
    int debugCauseOfDeathType;
    bool debugResetCauseOfDeath;
    std::vector<AffinityType> randomClassAffinities;
    int playerClass;
    float pressStartOffset;
    uint cancelFrames;
    GuiTextContainer* classTextContainer;
    GuiTextContainer* backgroundTextContainer;
    bool updateTextContainer;
};

struct NetMenu {
    bool customWorld;
    bool selectingCustomWorld;
    NetMenuReturnValue postCustomWorldSelectAction;
    int animateInTimer;
    int mouseStillTimer;
    GuiElement* currHovered;
    int frame;
    float drawFrame;
    int lastRefreshed;
    sf::RenderWindow* window;
    Input* input;
    UGCBrowser* ugcBrowser;
    sf::View view;
    sf::Vector2<float> mousePos;
    std::vector<bool> directionHeld;
    std::vector<std::pair<int, int>> framesHoldingDirection;
    int cursorIndex;
    sf::Vector2<int> selectPos;
    bool open;
    bool tryUseSteam;
    bool tryConnectLobby;
    float retrieveLobbiesSpinnerAlpha;
    CSteamID targetLobbySteamId;
    SteamCallResult m_SteamCallResultLobbyMatchList;
    GuiTextBox addressTextBox;
    std::string targetIpAddress;
    std::string gameName;
    bool setDefaultGameName;
    sf::IpAddress targetIp;
    std::vector<GuiElement*> guiElements;
    GuiCheckbox* privateGameCheckbox;
    GuiCheckbox* customGameCheckbox;
    GuiTextBox* gameNameField;
    GuiButton* hostGameButton;
    GuiButton* hostSteamGameButton;
    GuiButton* connectToIpButton;
    GuiButton* backButton;
    GuiButton* refreshButton;
    GuiButton* cancelButton;
    GuiFriendBrowser* friendBrowser;
    std::vector<GuiServerDetails*> serverList;
    GuiCursor guiCursor;
    bool loading;
    std::string loadingSubtext;
    float loadingMusicFade;
    Sprite spinnerSprite;
    std::string overlayDialogString;
    int overlayDialogFrames;
};

struct Unlockable {
    bool unlocked;
    bool hasCondition;
    UnlockableType type;
    std::string name;
    std::string conditions;
    std::function<bool()> conditionFunction;
    int progressionLevel;
    BackgroundType background;
};

struct ProgressionGui {
    std::map<std::string, ProgressionGuiItem*> guiItems;
    std::string active;
    std::string hot;
    std::string focused;
    bool mouseActive;
    int mouseStillTimer;
    sf::Vector2<float> mousePosition;
    sf::Vector2<int> selection;
    int cardsPage;
    std::stack<GuiItemAnimation> currAnimation;
    bool prevMenuOpen;
    std::vector<std::pair<std::string, Unlockable>> unlocksView;
    sf::Text pageLabel;
    sf::Text sortLabel;
    int sortOrder;
    int pageDirection;
    int pageAnimTimer;
    int gameOverTimer;
    int gameOverState;
    float gameOverStateProgress;
    int prevProgressionPoints;
};

struct GameGui {
    struct GoldPiece {
        float x;
        float y;
        int frame;
    };

    uint frame;
    Sprite goldFontSprite;
    Sprite goldBannerSprite;
    Sprite goldCoinSprite;
    Sprite gamepadSprite;
    Sprite keyboardSprite;
    std::vector<GameGui::GoldPiece> goldPieces;
    int shakePurse;
    int spawnedGoldTimer;
    int justPickedUpGoldTimer;
    int prevGoldReserve;
    float displayExperience;
    float experienceEffectCounter;
    float experienceShineEffectCounter;
    float timerPosition;
    int hideTimerDelay;
    sf::Vector2<float> menuPosition;
    bool showTimer;
};
