#include <Acolyte/game.h>
#include <Acolyte/utils.h>
#include <Acolyte/logger.h>
#include <sdk_registry.h>
#include <safetyhook.hpp>

static Logger gameLogger("Acolyte");
static const uintptr_t moduleBaseAddress = (uintptr_t)GetModuleHandleA(NULL);

sdk::Game::HandleInputFn pOriginalHandleInput = nullptr;
void __fastcall handleInputDetour(Client* clientThis, void*) {
	//sdk::Game::HandleDebugInput(clientThis, NULL);
	pOriginalHandleInput(clientThis, NULL);
}

namespace sdk::Game {
	HandleInputFn HandleInput = nullptr;
	HandleDebugInputFn HandleDebugInput = nullptr;

	void InitGame() {
		HandleInput = reinterpret_cast<HandleInputFn>(moduleBaseAddress + 0x150EA0);
		HandleDebugInput = reinterpret_cast<HandleDebugInputFn>(moduleBaseAddress + 0x151A50);

		Utils::CreateHook(reinterpret_cast<void**>(HandleInput), &handleInputDetour, reinterpret_cast<void**>(&pOriginalHandleInput));
	}

	SDK_REGISTER(InitGame);
}