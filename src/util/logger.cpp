#include "Acolyte/logger.h"
#include "colors.hpp"
#include <string>
#include <memory>
// TODO: Cache the object with shared_ptr, make constructor internal or... Something, unsure
// TODO: Log levels
// TODO: Logging to file

std::string Logger::AssembleLog(std::string type) {
	std::string strTitle = (std::string)this->title;
	return !strTitle.empty() ? "[" + strTitle + " - " + type + "] " : "";
}

Logger::Logger(std::string &title) {
	this->title = title.c_str();
}

Logger::Logger(const char* title) {
	this->title = title;
}

Logger::Logger() {}

void Logger::LogError(std::string str, std::string error) {
	std::string output = AssembleLog("Error") + str + error;
	std::cerr << dye::red(output) << std::endl;
}

void Logger::LogFail(std::string procName) {;
	Logger::LogError(procName + " failed, error: ", std::to_string(GetLastError()));
}

void Logger::LogDebug(std::string str) {
	std::string output = AssembleLog("Debug") + str;
	std::cout << dye::grey(output) << std::endl;
}

void Logger::LogWarning(std::string str) {
	std::string output = AssembleLog("Warning") + str;
	std::cout << dye::yellow(output) << std::endl;
}

void Logger::Log(std::string str) {
	std::string output = AssembleLog("Log") + str;
	std::cerr << output << std::endl;
}