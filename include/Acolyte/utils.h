#pragma once
#include <string>
#include <optional>
#include <Windows.h>

namespace Utils {
	void CreateHook(LPVOID pTarget, LPVOID pDetour, LPVOID* ppOriginal);
	int PatchMemory(void* address, const void* data, SIZE_T len);
	int PatchByteAt(uintptr_t addr, size_t offset, uint8_t value);
	std::optional<std::string> GetCallingModule();
}