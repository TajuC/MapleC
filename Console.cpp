#include "Console.h"
#include "Logger.h"
std::atomic<bool> Console::s_Running(false);
std::thread Console::s_LogThread;

void Console::Initialize() {
    AllocConsole();
    FILE* fDummy;
    freopen_s(&fDummy, "CONOUT$", "w", stdout);
    freopen_s(&fDummy, "CONOUT$", "w", stderr);
    freopen_s(&fDummy, "CONIN$", "r", stdin);
    std::cout.clear();
    std::clog.clear();
    std::cerr.clear();
    std::cin.clear();

    SetConsoleTitleW(L"MapleC Log Console");

    s_Running = true;
    s_LogThread = std::thread(ReadLogFile);
}

void Console::Shutdown() {
    s_Running = false;
    if (s_LogThread.joinable()) {
        s_LogThread.join();
    }
    FreeConsole();
}

void Console::ReadLogFile() {
    std::ifstream file("MapleCLogs.txt", std::ios::in);
    if (!file.is_open()) {
        std::cerr << "Failed to open MapleCLogs.txt" << std::endl;
        return;
    }

    std::string line;
    while (s_Running) {
        if (std::getline(file, line)) {
            if (line.find("[INFO]") != std::string::npos) {
                SetConsoleColor(7); // White
            }
            else if (line.find("[WARNING]") != std::string::npos) {
                SetConsoleColor(14); // Yellow
            }
            else if (line.find("[ERROR]") != std::string::npos || line.find("[CRITICAL]") != std::string::npos) {
                SetConsoleColor(12); // Red
            }
            else {
                SetConsoleColor(7); // Default to white
            }
            std::cout << line << std::endl;
        }
        else {
            file.clear();
            file.seekg(0, std::ios::end);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    file.close();
}

void Console::SetConsoleColor(int color) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
}