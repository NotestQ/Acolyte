#pragma once
#include <Acolyte/exports.h>
#include <Acolyte/logger.h>
#include <Windows.h>
#include <string>
#include <optional>
#include <vector>
#include <any>

namespace Utils {
	ACOLYTE_API void CreateHook(LPVOID pTarget, LPVOID pDetour, LPVOID* ppOriginal);
	ACOLYTE_API int PatchMemory(void* address, const void* data, SIZE_T len);
	ACOLYTE_API int PatchByteAt(uintptr_t addr, size_t offset, uint8_t value);
	ACOLYTE_API std::optional<std::string> GetCallingModule();
}