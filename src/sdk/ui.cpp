#include <Acolyte/ui.h>
#include <Acolyte/utils.h>
#include <Vagante/client.h>
#include <sdk_registry.h>
#include <safetyhook.hpp>
#include <new>
#include "multiplayer/multiplayer.h"

static uintptr_t moduleBaseAddress = (uintptr_t)GetModuleHandleA(NULL);

using AllocateFn = void*(__cdecl*)(size_t size);
using ButtonConstructorFn = GuiButton*(__thiscall*)(GuiButton* buttonThis, int x, int y, int w, int h, bool background, std::string text, std::string tooltip);
using CheckboxConstructorFn = GuiCheckbox*(__thiscall*)(GuiCheckbox* checkboxThis, int x, int y, bool checked, std::string text, std::string tooltip);
using LabelConstructorFn = GuiLabel*(__thiscall*)(GuiLabel* labelThis, std::string text, int x, int y);
using ElementConstructorFn = GuiElement*(__thiscall*)(GuiElement* elementThis, GuiElementType type, int x, int y, int w, int h);
using PollActionFn = bool(__thiscall*)(GuiButton* buttonThis);
using ClientUpdateFn = void(__thiscall*)(Client* clientThis);
using GetViewFn = sf::View*(__thiscall*)(sf::RenderTarget* targetThis);
using SetViewFn = void(__thiscall*)(sf::RenderTarget* targetThis, sf::View* view);

static AllocateFn Allocate = nullptr;
static ButtonConstructorFn ButtonConstructor = nullptr;
static CheckboxConstructorFn CheckboxConstructor = nullptr;
static LabelConstructorFn LabelConstructor = nullptr;
static ElementConstructorFn ElementConstructor = nullptr;
static GetViewFn GetView = nullptr;
static SetViewFn SetView = nullptr;

PollActionFn pOriginalPollAction = nullptr;
bool __fastcall pollActionDetour(GuiButton* buttonThis, void*) {
    bool pressed = pOriginalPollAction(buttonThis);
    if (pressed) sdk::UI::OnButtonPressed(buttonThis);
    return pressed;
}

ClientUpdateFn pOriginalClientStep = nullptr;
void __fastcall clientStepDetour(Client* clientThis, void*) {
    pOriginalClientStep(clientThis);
    sdk::UI::OnStep(clientThis, clientThis->input);
}

ClientUpdateFn pOriginalClientDestroy = nullptr;
void __fastcall clientDestroyDetour(Client* clientThis, void*) {
    sdk::UI::OnClientDestroyed(clientThis);
    pOriginalClientDestroy(clientThis);
}

void drawUIDetour(SafetyHookContext& ctx) {
    sdk::UI::OnDraw(reinterpret_cast<Client*>(ctx.esi), reinterpret_cast<sf::RenderTarget*>(ctx.edi));
}

namespace sdk::UI {
    sigslot::signal<GuiButton*> OnButtonPressed;
    sigslot::signal<Client*, Input*> OnStep;
    sigslot::signal<Client*, sf::RenderTarget*> OnDraw;
    sigslot::signal<Client*> OnClientDestroyed;

    GuiButton* CreateButton(int x, int y, int w, int h, std::string text, bool background) {
        auto button = static_cast<GuiButton*>(Allocate(sizeof(GuiButton)));
        ButtonConstructor(button, x, y, w, h, background, {}, {});
        SetText(button, text);
        return button;
    }

    GuiCheckbox* CreateCheckbox(int x, int y, std::string text, bool checked) {
        auto checkbox = static_cast<GuiCheckbox*>(Allocate(sizeof(GuiCheckbox)));
        CheckboxConstructor(checkbox, x, y, checked, {}, {});
        SetText(checkbox, text);
        return checkbox;
    }

    GuiLabel* CreateLabel(int x, int y, std::string text) {
        auto label = static_cast<GuiLabel*>(Allocate(sizeof(GuiLabel)));
        LabelConstructor(label, {}, x, y);
        SetText(label, text);
        return label;
    }

    GuiWindow* CreatePanel(int x, int y, int w, int h, bool background) {
        auto window = static_cast<GuiWindow*>(Allocate(sizeof(GuiWindow)));
        ElementConstructor(window, GuiElementType::WINDOW, x, y, w, h);
        window->vtable = reinterpret_cast<void*>(moduleBaseAddress + 0x71d158);
        new (&window->elements) std::vector<GuiElement*>();
        window->hasBg = background;
        return window;
    }

    void Destroy(GuiElement* element) {
        if (!element) return;
        using DestroyFn = void*(__thiscall*)(GuiElement*, uint);
        auto vtable = static_cast<void**>(element->vtable);
        reinterpret_cast<DestroyFn>(vtable[0])(element, 1);
    }

    void AddElement(std::vector<GuiElement*>& elements, GuiElement* element) {
        elements.push_back(element);
    }

    void AddElement(GuiWindow* window, GuiElement* element) {
        AddElement(window->elements, element);
    }

    void SetText(GuiElement* element, std::string text) {
        std::string* destination;
        switch (element->type) {
        case GuiElementType::BUTTON:
            destination = &static_cast<GuiButton*>(element)->name;
            break;
        case GuiElementType::CHECKBOX:
            destination = &static_cast<GuiCheckbox*>(element)->label;
            break;
        case GuiElementType::LABEL:
            destination = &static_cast<GuiLabel*>(element)->label;
            break;
        case GuiElementType::SLIDER:
            destination = &static_cast<GuiSlider*>(element)->label;
            break;
        case GuiElementType::TEXTBOX:
            destination = &static_cast<GuiTextBox*>(element)->label;
            break;
        default:
            return;
        }
        *destination = text;
    }

    void SetTooltip(GuiElement* element, std::string text) {
        element->tooltip = text;
    }

    void Action(GuiElement* element, bool down) {
        if (element->hidden || element->disabled) return;
        using ActionFn = void(__thiscall*)(GuiElement*, bool);
        reinterpret_cast<ActionFn>(static_cast<void**>(element->vtable)[1])(element, down);
    }

    void MouseAction(GuiElement* element, int x, int y, bool down, bool leftButton) {
        if (element->hidden || element->disabled) return;
        using MouseActionFn = void(__thiscall*)(GuiElement*, int, int, bool, bool);
        reinterpret_cast<MouseActionFn>(static_cast<void**>(element->vtable)[2])(element, x, y, down, leftButton);
    }

    void Step(GuiElement* element, int mouseX, int mouseY) {
        if (element->hidden) return;
        using StepFn = void(__thiscall*)(GuiElement*, int, int);
        reinterpret_cast<StepFn>(static_cast<void**>(element->vtable)[3])(element, mouseX, mouseY);
    }

    void AfterStep(GuiElement* element, bool leftHeld, bool rightHeld) {
        if (element->hidden) return;
        using AfterStepFn = void(__thiscall*)(GuiElement*, bool, bool);
        reinterpret_cast<AfterStepFn>(static_cast<void**>(element->vtable)[4])(element, leftHeld, rightHeld);
    }

    void Draw(GuiElement* element, sf::RenderTarget* target) {
        if (element->hidden) return;
        using DrawFn = void(__thiscall*)(GuiElement*, sf::RenderTarget*);
        reinterpret_cast<DrawFn>(static_cast<void**>(element->vtable)[5])(element, target);
    }

    void Draw(GuiElement* element, sf::RenderTarget* target, sf::View& view) {
        sf::View previous = *GetView(target);
        SetView(target, &view);
        Draw(element, target);
        SetView(target, &previous);
    }

    bool PollAction(GuiButton* button) {
        return pollActionDetour(button, nullptr);
    }

    void InitUI() {
        Allocate = reinterpret_cast<AllocateFn>(moduleBaseAddress + 0x4ccfa2);
        ButtonConstructor = reinterpret_cast<ButtonConstructorFn>(moduleBaseAddress + 0x1e4b70);
        CheckboxConstructor = reinterpret_cast<CheckboxConstructorFn>(moduleBaseAddress + 0x1e58c0);
        LabelConstructor = reinterpret_cast<LabelConstructorFn>(moduleBaseAddress + 0x1e65f0);
        ElementConstructor = reinterpret_cast<ElementConstructorFn>(moduleBaseAddress + 0x1e64f0);
        GetView = reinterpret_cast<GetViewFn>(moduleBaseAddress + 0x414120);
        SetView = reinterpret_cast<SetViewFn>(moduleBaseAddress + 0x414530);
        Utils::CreateHook(reinterpret_cast<void*>(moduleBaseAddress + 0x1e57f0),
            &pollActionDetour, reinterpret_cast<void**>(&pOriginalPollAction));
        Utils::CreateHook(reinterpret_cast<void*>(moduleBaseAddress + 0x15e500),
            &clientStepDetour, reinterpret_cast<void**>(&pOriginalClientStep));
        Utils::CreateHook(reinterpret_cast<void*>(moduleBaseAddress + 0x14ae70),
            &clientDestroyDetour, reinterpret_cast<void**>(&pOriginalClientDestroy));
        static SafetyHookMid drawHook = safetyhook::create_mid(reinterpret_cast<void*>(moduleBaseAddress + 0x163222), drawUIDetour);
        if (!pOriginalPollAction || !pOriginalClientStep || !pOriginalClientDestroy || !drawHook)
            Multiplayer::FailInitialization();
    }
    SDK_REGISTER(InitUI);
}
