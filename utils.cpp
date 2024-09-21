#define _CRT_SECURE_NO_WARNINGS
#include <Windows.h>
#include <string>
#include "utils.h"
#include "../Logger.h"
bool utils::Attach_Console()
{
    if (!AllocConsole()) {
        Logger::Log("Failed to allocate console", Logger::LogLevel::Error);
        return false;
    }


    FILE* fDummy;
    freopen_s(&fDummy, "CONIN$", "r", stdin);
    freopen_s(&fDummy, "CONOUT$", "w", stdout);
    freopen_s(&fDummy, "CONOUT$", "w", stderr);

    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD consoleMode;

    GetConsoleMode(hConsole, &consoleMode);
    consoleMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    if (!SetConsoleMode(hConsole, consoleMode)) {
        Logger::Log("Failed to set console mode", Logger::LogLevel::Error);
        return false;
    }



    return true;
}

void utils::Detach_Console()
{
    HWND consoleWindow = GetConsoleWindow();
    FreeConsole();
    if (consoleWindow != NULL) {
        PostMessage(consoleWindow, WM_CLOSE, 0, 0);
    }
}