#pragma once
#include <fstream>
#include <string>
#include <windows.h>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <mutex>
#include <unordered_map>

class Logger {
private:
    static std::ofstream logFile;
    static std::mutex logMutex;
    static std::unordered_map<std::string, std::chrono::steady_clock::time_point> lastLogTimes;

    static std::string GetTimeStamp();
    static bool ShouldLog(const std::string& message);

public:
    enum class LogLevel { Debug, Info, Warning, Error, Critical };

    static void Init();
    static void Log(const std::string& message, LogLevel level = LogLevel::Info);
    static void LogLastError(const std::string& context);
    static void Close();

    static std::string GetHexStr(HRESULT hr);
    static std::string GetHexStr(UINT64 value);
};