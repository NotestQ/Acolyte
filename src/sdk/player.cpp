#include <Acolyte/resources.h>
#include <Acolyte/utils.h>
#include <Acolyte/logger.h>
#include <Acolyte/player.h>
#include <Vagante/entities.h>
#include <sdk_registry.h>
#include <safetyhook.hpp>

#include <Acolyte/item.h>

static Logger playerLogger("Acolyte");
static const uintptr_t moduleBaseAddress = (uintptr_t)GetModuleHandleA(NULL);

void addPlayerDetour(SafetyHookContext& ctx) {
	std::shared_ptr<Player>& player = *reinterpret_cast<std::shared_ptr<Player>*>(ctx.ebp + -0x14);

	sdk::Player::OnPlayerAdded(player);
}

void setLevelDetour(SafetyHookContext& ctx) {
	Player& player = *reinterpret_cast<Player*>(ctx.ebx);

	sdk::Player::OnLevelSet(player);
}

sdk::Player::UpdateStatsFn pOriginalUpdateStats = nullptr;
void __fastcall updateStatsDetour(::Player* playerThis, void*, bool param_1, bool clientOverride) {
	pOriginalUpdateStats(playerThis, NULL, param_1, clientOverride);
}

namespace sdk::Player {
	UpdateStatsFn UpdateStats = nullptr;

	AddNewSkillFn AddNewSkill = nullptr;
	SetCurrSkillFn SetCurrSkill = nullptr;
	SkillConstructorFn SkillConstructor = nullptr;

	sigslot::signal<std::shared_ptr<::Player>&> OnPlayerAdded;
	sigslot::signal<::Player&> OnLevelSet;

	void InitPlayer() {
		playerLogger.LogDebug("Initializing Player system...");
		
		UpdateStats = reinterpret_cast<UpdateStatsFn>(moduleBaseAddress + 0x2CB380);

		AddNewSkill = reinterpret_cast<AddNewSkillFn>(moduleBaseAddress + 0x2FD0D0);
		SetCurrSkill = reinterpret_cast<SetCurrSkillFn>(moduleBaseAddress + 0x3018A0);
		SkillConstructor = reinterpret_cast<SkillConstructorFn>(moduleBaseAddress + 0x38B150);

		static SafetyHookMid addPlayerHook = safetyhook::create_mid(reinterpret_cast<void*>(moduleBaseAddress + 0x30873B), addPlayerDetour); // After line 149
		static SafetyHookMid setLevelHookRet1 = safetyhook::create_mid(reinterpret_cast<void*>(moduleBaseAddress + 0x2C7EA8), setLevelDetour); // After line 1276
		static SafetyHookMid setLevelHookRet2 = safetyhook::create_mid(reinterpret_cast<void*>(moduleBaseAddress + 0x2C7ED0), setLevelDetour); // After line 1279
		
		Utils::CreateHook(reinterpret_cast<void**>(UpdateStats), &updateStatsDetour, reinterpret_cast<void**>(&pOriginalUpdateStats));
		
		playerLogger.LogDebug("Player system initialized!");
	}

	SDK_REGISTER(InitPlayer);
}