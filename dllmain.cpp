#include <windows.h>
#include <psapi.h>
#include "Core/globals.h"
#include "Core/utils.h"
#include "hooks/hooks.h"
#include "Logger.h"
#include "Console.h"
#include "menu.h"
#include <stdexcept>
#include <dbghelp.h>
#include <memory>
#include <atomic>

#pragma comment(lib, "dbghelp.lib")

HMODULE hlmodule = nullptr;
std::atomic<bool> loop{true};

// Custom deleter for HANDLE
struct HandleDeleter {
    void operator()(HANDLE h) const {
        if (h != INVALID_HANDLE_VALUE) CloseHandle(h);
    }
};

// RAII wrapper for HANDLE
using UniqueHandle = std::unique_ptr<void, HandleDeleter>;

LONG WINAPI CustomUnhandledExceptionFilter(EXCEPTION_POINTERS* ExceptionInfo)
{
    Logger::Log("Unhandled exception occurred!", Logger::LogLevel::Critical);
    Logger::Log("Exception code: " + Logger::GetHexStr(static_cast<UINT64>(ExceptionInfo->ExceptionRecord->ExceptionCode)), Logger::LogLevel::Critical);
    Logger::Log("Exception address: " + Logger::GetHexStr(reinterpret_cast<UINT64>(ExceptionInfo->ExceptionRecord->ExceptionAddress)), Logger::LogLevel::Critical);

    // Create minidump
    UniqueHandle hFile(CreateFileW(L"crash.dmp", GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL));
    if (hFile)
    {
        MINIDUMP_EXCEPTION_INFORMATION mei;
        mei.ThreadId = GetCurrentThreadId();
        mei.ExceptionPointers = ExceptionInfo;
        mei.ClientPointers = TRUE;

        if (MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile.get(), MiniDumpNormal, &mei, NULL, NULL))
        {
            Logger::Log("Minidump created: crash.dmp", Logger::LogLevel::Critical);
        }
        else
        {
            Logger::Log("Failed to write minidump: " + std::to_string(GetLastError()), Logger::LogLevel::Error);
        }
    }
    else
    {
        Logger::Log("Failed to create minidump file: " + std::to_string(GetLastError()), Logger::LogLevel::Error);
    }

    return EXCEPTION_CONTINUE_SEARCH;
}

void injection_thread() {
    Logger::Log("Injection thread started", Logger::LogLevel::Info);

    // Set up exception handler
    SetUnhandledExceptionFilter(CustomUnhandledExceptionFilter);

    try {
        Logger::Log("Verifying Window::base: " + Logger::GetHexStr(Window::base), Logger::LogLevel::Info);
        if (Window::base == 0) {
            throw std::runtime_error("Window::base is invalid (0)");
        }
        // Get the base address again and compare
        HMODULE hModule = GetModuleHandle(NULL);
        uintptr_t baseAddr = reinterpret_cast<uintptr_t>(hModule);
        if (baseAddr != Window::base) {
            Logger::Log("Warning: Window::base mismatch. Expected: " + Logger::GetHexStr(baseAddr) + 
                        ", Actual: " + Logger::GetHexStr(Window::base), Logger::LogLevel::Warning);
            Window::base = baseAddr; // Correct the base address
        } else {
            Logger::Log("Window::base verified correct", Logger::LogLevel::Info);
        }

        // Initialize console
        Logger::Log("Initializing console", Logger::LogLevel::Info);
        Console::Initialize();
        Logger::Log("Console initialized", Logger::LogLevel::Info);

        Logger::Log("About to call Menu::Core()", Logger::LogLevel::Info);
        Menu::Core();
        Logger::Log("Menu::Core() completed", Logger::LogLevel::Info);


        Logger::Log("Setting up hooks", Logger::LogLevel::Info);

        hooks::Setup();
        Logger::Log("Hooks setup completed", Logger::LogLevel::Info);

        Logger::Log("Entering main loop", Logger::LogLevel::Info);
        while (loop.load(std::memory_order_relaxed)) {
            Sleep(100);  // Reduced CPU usage
            if (GetAsyncKeyState(VK_END) & 1)
                loop.store(false, std::memory_order_relaxed);
        }
        Logger::Log("Exited main loop", Logger::LogLevel::Info);
    }
    catch (const std::exception& err) {
        Logger::Log("Error in injection thread: " + std::string(err.what()), Logger::LogLevel::Critical);
    }
    catch (...) {
        Logger::Log("Unknown error occurred in injection thread", Logger::LogLevel::Critical);
    }

    Logger::Log("Cleaning up", Logger::LogLevel::Info);
    hooks::DisableHooks();
    hooks::Destroy();
    Menu::Destroy();
    Logger::Log("Cleanup complete. Exiting thread", Logger::LogLevel::Info);
    Logger::Close();

    // Shutdown console
   // Console::Shutdown();

    FreeLibraryAndExitThread(hlmodule, 1);
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID reserved) {
    switch (reason) {
    case DLL_PROCESS_ATTACH: {
        Logger::Init();
        Logger::Log("DLL_PROCESS_ATTACH called", Logger::LogLevel::Info);
        hlmodule = module;
        DisableThreadLibraryCalls(module);
        
        // Get the base address multiple times and log each attempt
        for (int i = 0; i < 5; i++) {
            uintptr_t base = reinterpret_cast<uintptr_t>(GetModuleHandle(NULL));
            Logger::Log("Attempt " + std::to_string(i+1) + " - Window::base: " + Logger::GetHexStr(base), Logger::LogLevel::Info);
            if (base != 0) {
                Window::base = base;
                break;
            }
            Sleep(100); // Wait a bit before trying again
        }

        if (Window::base == 0) {
            Logger::Log("Failed to get valid Window::base after multiple attempts", Logger::LogLevel::Error);
            return FALSE; // Fail DLL loading if we can't get a valid base address
        }

        Logger::Log("Final Window::base set to: " + Logger::GetHexStr(Window::base), Logger::LogLevel::Info);     
        
        HANDLE hThread = CreateThread(nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(injection_thread), nullptr, 0, nullptr);
        if (hThread == NULL) {
            Logger::LogLastError("Failed to create injection thread");
            return FALSE;
        } else {
            CloseHandle(hThread);
        }
    } break;
    case DLL_PROCESS_DETACH:
        Logger::Log("DLL detaching", Logger::LogLevel::Info);
        loop.store(false, std::memory_order_relaxed);
        break;
    }
    return TRUE;
}
