#pragma once
#include <Vagante/ui.h>
#include <Acolyte/exports.h>
#include <sigslot/signal.hpp>

namespace sdk::UI {
    // Create after resources are loaded, on the game thread
    ACOLYTE_API GuiButton* CreateButton(int x, int y, int w, int h, std::string text, bool background = true);
    ACOLYTE_API GuiCheckbox* CreateCheckbox(int x, int y, std::string text, bool checked = false);
    ACOLYTE_API GuiLabel* CreateLabel(int x, int y, std::string text);
    ACOLYTE_API GuiWindow* CreatePanel(int x, int y, int w, int h, bool background = true);
    // Use this for elements you still own, windows and menus delete their added elements
    ACOLYTE_API void Destroy(GuiElement* element);
    // Added elements use absolute coordinates, not positions relative to the window
    ACOLYTE_API void AddElement(std::vector<GuiElement*>& elements, GuiElement* element);
    ACOLYTE_API void AddElement(GuiWindow* window, GuiElement* element);
    ACOLYTE_API void SetText(GuiElement* element, std::string text);
    ACOLYTE_API void SetTooltip(GuiElement* element, std::string text);

    // Coordinates are in the current view, the calling menu handles focus and hit testing
    ACOLYTE_API void Action(GuiElement* element, bool down);
    ACOLYTE_API void MouseAction(GuiElement* element, int x, int y, bool down, bool leftButton);
    ACOLYTE_API void Step(GuiElement* element, int mouseX, int mouseY);
    ACOLYTE_API void AfterStep(GuiElement* element, bool leftHeld, bool rightHeld);
    ACOLYTE_API void Draw(GuiElement* element, sf::RenderTarget* target);
    ACOLYTE_API void Draw(GuiElement* element, sf::RenderTarget* target, sf::View& view);
    ACOLYTE_API bool PollAction(GuiButton* button);

    // Called when PollAction consumes a button press, including the game's own buttons
    // Don't delete the button here, the menu still uses it after this callback
    ACOLYTE_API extern sigslot::signal<GuiButton*> OnButtonPressed;
    ACOLYTE_API extern sigslot::signal<Client*, Input*> OnStep;
    ACOLYTE_API extern sigslot::signal<Client*, sf::RenderTarget*> OnDraw;
    ACOLYTE_API extern sigslot::signal<Client*> OnClientDestroyed;
}
