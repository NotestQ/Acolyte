#include <Acolyte/vaganteutils.h>
#include <Acolyte/utils.h>
#include <Acolyte/logger.h>
#include <Acolyte/player.h>
#include <Vagante/entities.h>
#include <sdk_registry.h>
#include <safetyhook.hpp>

static Logger playerLogger("Acolyte");
static const uintptr_t moduleBaseAddress = (uintptr_t)GetModuleHandleA(NULL);

void addPlayerEndDetour(SafetyHookContext& ctx) {
	std::shared_ptr<Player>& player = *reinterpret_cast<std::shared_ptr<Player>*>(ctx.ebp + -0x14);
	// If you press spacebar after the menu is down and you can't move your character we get junk data as addPlayer is still called
	// So... Don't do that? For now?
	//playerLogger.LogWarning("Junk data check: " + std::to_string(player->playerClass));
	sdk::Player::OnPlayerAdded(player);
}

namespace sdk::Player {

	void InitPlayer() {
		playerLogger.LogDebug("Initializing Player system...");
		
		static SafetyHookMid addPlayerEndHook = safetyhook::create_mid(reinterpret_cast<void*>(moduleBaseAddress + 0x30876A), addPlayerEndDetour); // Inbetween line 169 and line 170
	
		playerLogger.LogDebug("Player system initialized!");
	}

	SDK_REGISTER(InitPlayer);
}