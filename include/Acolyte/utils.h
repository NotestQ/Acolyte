#pragma once
#include <Acolyte/exports.h>
#include <Acolyte/logger.h>
#include <safetyhook.hpp>
#include <Windows.h>
#include <string>
#include <optional>
#include <vector>
#include <any>

namespace Utils {
	ACOLYTE_API void CreateHook(LPVOID pTarget, LPVOID pDetour, LPVOID* ppOriginal);
	// Stops the game if the hook can't be installed
	ACOLYTE_API SafetyHookMid CreateMidHook(void* target, safetyhook::MidHookFn detour);
	ACOLYTE_API int PatchMemory(void* address, const void* data, SIZE_T len);
	ACOLYTE_API int PatchByteAt(uintptr_t addr, size_t offset, uint8_t value);
	ACOLYTE_API std::optional<std::string> GetCallingModule();
	// Creates a unique empty file, delete it when finished
	ACOLYTE_API std::optional<std::string> CreateTemporaryFile(std::string directory);
}
