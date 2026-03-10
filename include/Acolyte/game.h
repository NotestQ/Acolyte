#pragma once
#include <Vagante/net.h>
#include <Acolyte/exports.h>

namespace sdk::Game {
	using HandleInputFn = void(__fastcall*)(Client* clientThis, void* edx);
	using HandleDebugInputFn = void(__fastcall*)(Client* clientThis, void* edx);

	ACOLYTE_API extern HandleInputFn HandleInput;
	ACOLYTE_API extern HandleDebugInputFn HandleDebugInput;
}