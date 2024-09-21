#pragma once
#include <string>
#include <Windows.h>
#include <iostream>
#include <fstream>
#include <thread>
#include <atomic>

class Console {
public:
    static void Initialize();
    static void Shutdown();

private:
    static void ReadLogFile();
    static void SetConsoleColor(int color);

    static std::atomic<bool> s_Running;
    static std::thread s_LogThread;
};