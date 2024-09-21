#include "Logger.h"
#include <iostream>
#include <ShlObj.h>
std::ofstream Logger::logFile;
std::mutex Logger::logMutex;
std::unordered_map<std::string, std::chrono::steady_clock::time_point> Logger::lastLogTimes;

void Logger::Init() {
    char desktopPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_DESKTOP, NULL, 0, desktopPath))) {
        std::string logFilePath = std::string(desktopPath) + "\\MapleCLogs.txt";
        logFile.open(logFilePath, std::ios::app);
        if (!logFile.is_open()) {
            std::cerr << "Failed to open log file: " << logFilePath << std::endl;
        } else {
            Log("Log file opened successfully: " + logFilePath, LogLevel::Info);
        }
    } else {
        std::cerr << "Failed to get desktop path" << std::endl;
    }
}

void Logger::Log(const std::string& message, LogLevel level) {
    std::lock_guard<std::mutex> lock(logMutex);

    if (!ShouldLog(message)) {
        return;
    }

    std::string levelStr;
    switch (level) {
    case LogLevel::Debug:    levelStr = "[DEBUG]"; break;
    case LogLevel::Info:     levelStr = "[INFO]"; break;
    case LogLevel::Warning:  levelStr = "[WARNING]"; break;
    case LogLevel::Error:    levelStr = "[ERROR]"; break;
    case LogLevel::Critical: levelStr = "[CRITICAL]"; break;
    }

    std::string timeStr = GetTimeStamp();
    std::string fullMessage = timeStr + " " + levelStr + " " + message;

    // Log to file
    if (logFile.is_open()) {
        logFile << fullMessage << std::endl;
        logFile.flush();
    }

    // Log to console
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    WORD color;
    switch (level) {
    case LogLevel::Debug:    color = 8; break; // Gray
    case LogLevel::Info:     color = 7; break; // White
    case LogLevel::Warning:  color = 14; break; // Yellow
    case LogLevel::Error:    color = 12; break; // Red
    case LogLevel::Critical: color = 79; break; // White on Red
    default:                 color = 7; break; // White
    }
    SetConsoleTextAttribute(hConsole, color);
    std::cout << fullMessage << std::endl;
    SetConsoleTextAttribute(hConsole, 7); // Reset to default color
}

void Logger::LogLastError(const std::string& context) {
    DWORD error = GetLastError();
    LPSTR errorMessage = nullptr;
    size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&errorMessage, 0, NULL);
    std::string errorStr(errorMessage, size);
    LocalFree(errorMessage);

    Log(context + ": " + errorStr, LogLevel::Error);
}

void Logger::Close() {
    if (logFile.is_open()) {
        logFile.close();
    }
}

std::string Logger::GetTimeStamp() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm timeInfo;
    localtime_s(&timeInfo, &in_time_t);
    std::stringstream ss;
    ss << std::put_time(&timeInfo, "[%Y-%m-%d %H:%M:%S]");
    return ss.str();
}

std::string Logger::GetHexStr(HRESULT hr) {
    std::stringstream ss;
    ss << std::hex << std::showbase << hr;
    return ss.str();
}

std::string Logger::GetHexStr(UINT64 value) {
    std::stringstream ss;
    ss << std::hex << std::uppercase << value;
    return ss.str();
}

bool Logger::ShouldLog(const std::string& message) {
    auto now = std::chrono::steady_clock::now();
    auto& lastTime = lastLogTimes[message];
    if (now - lastTime < std::chrono::milliseconds(100)) {
        return false;
    }
    lastTime = now;
    return true;
}