#pragma once
#include <Vagante/entities.h>
#include <Acolyte/exports.h>
#include <sigslot/signal.hpp>

namespace sdk::Player {
	using UpdateStatsFn = void(__fastcall*)(::Player* playerThis, void* edx, bool param_1, bool clientOverride);

	ACOLYTE_API extern UpdateStatsFn UpdateStats;

	ACOLYTE_API sigslot::signal<std::shared_ptr<::Player>&> OnPlayerAdded;
	ACOLYTE_API sigslot::signal<::Player&> OnLevelSet;
}