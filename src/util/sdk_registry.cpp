#pragma once
#include <vector>
#include <mutex>
static std::vector<void(*)()> sdk_funs;
std::mutex g_sdk_mutex;

bool sdk_register(void(*fn)()) {
	const std::lock_guard<std::mutex> lock(g_sdk_mutex);
	sdk_funs.push_back(fn);
	return true;
}

void InitAllSDKComponents() {
	for (auto func : sdk_funs) {
		func();
	}
}

