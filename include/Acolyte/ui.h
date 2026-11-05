#pragma once
#include <Vagante/ui.h>
#include <Acolyte/exports.h>
#include <sigslot/signal.hpp>

namespace sdk::UI {
    // Create elements from OnResourceInitialized or OnStep
    ACOLYTE_API GuiButton* CreateButton(int x, int y, int w, int h, std::string text, bool background = true);
    ACOLYTE_API GuiCheckbox* CreateCheckbox(int x, int y, std::string text, bool checked = false);
    ACOLYTE_API GuiLabel* CreateLabel(int x, int y, std::string text);
    ACOLYTE_API GuiWindow* CreatePanel(int x, int y, int w, int h, bool background = true);

    // Only use this for an unattached element, the window or menu deletes the ones you added to it
    ACOLYTE_API void Destroy(GuiElement* element);
    // Elements keep their absolute coordinates when added to a window
    ACOLYTE_API void AddElement(std::vector<GuiElement*>& elements, GuiElement* element);
    ACOLYTE_API void AddElement(GuiWindow* window, GuiElement* element);
    ACOLYTE_API void SetText(GuiElement* element, std::string text);
    ACOLYTE_API void SetTooltip(GuiElement* element, std::string text);

    ACOLYTE_API void Action(GuiElement* element, bool down);
    // Mouse coordinates are in the current view, the menu handles focus and hit testing
    ACOLYTE_API void MouseAction(GuiElement* element, int x, int y, bool down, bool leftButton);
    ACOLYTE_API void Step(GuiElement* element, int mouseX, int mouseY);
    ACOLYTE_API void AfterStep(GuiElement* element, bool leftHeld, bool rightHeld);
    ACOLYTE_API void Draw(GuiElement* element, sf::RenderTarget* target);
    ACOLYTE_API void Draw(GuiElement* element, sf::RenderTarget* target, sf::View& view);
    ACOLYTE_API bool PollAction(GuiButton* button);

    // Fires when PollAction reports a press, including on the game's own buttons
    // Don't use this to destroy a button in NetMenu::guiElements, NetMenu::step can still write its alphaModifier this frame
    // If you need to destroy it just leave it, NetMenu::~NetMenu deletes it with the other buttons
    ACOLYTE_API extern sigslot::signal<GuiButton*> OnButtonPressed;
    ACOLYTE_API extern sigslot::signal<Client*, Input*> OnStep;
    ACOLYTE_API extern sigslot::signal<Client*, sf::RenderTarget*> OnDraw;
    ACOLYTE_API extern sigslot::signal<Client*> OnClientDestroyed;
}
