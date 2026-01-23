#pragma once
#include "exports.h"
#include <string>
#include <windows.h>
#include <unordered_map>

class ACOLYTE_API Logger {
private:
	std::string AssembleLog(std::string type);
	const char* title = "";
public:
	Logger(std::string &title);
	Logger(const char* title);
	Logger();
	void LogError(std::string str, std::string error = "");
	void LogFail(std::string procName);
	void LogDebug(std::string str);
	void LogWarning(std::string str);
	void Log(std::string str);
};