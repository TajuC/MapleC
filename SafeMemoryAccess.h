#pragma once
#include <optional>
#include <vector>
#include <Windows.h>
#include "Logger.h"

class SafeMemoryAccess {
public:
    template<typename T, typename OffsetType>
    static std::optional<T> DerefPointerChain(uintptr_t baseAddress, const std::vector<OffsetType>& offsets) {
        uintptr_t currentAddress = baseAddress;
        
        for (size_t i = 0; i < offsets.size(); ++i) {
            if (!IsValidMemory(reinterpret_cast<void*>(currentAddress))) {
                Logger::Log("Invalid memory at step " + std::to_string(i) + ": " + Logger::GetHexStr(currentAddress), Logger::LogLevel::Warning);
                return std::nullopt;
            }

            if (i == offsets.size() - 1) {
                return ReadMemory<T>(currentAddress + static_cast<uintptr_t>(offsets[i]));
            }


            auto nextAddress = ReadMemory<uintptr_t>(currentAddress + static_cast<uintptr_t>(offsets[i]));
            if (!nextAddress) {

                Logger::Log("Failed to read address at step " + std::to_string(i) + ": " + Logger::GetHexStr(currentAddress), Logger::LogLevel::Warning);
                return std::nullopt;
            }

            currentAddress = *nextAddress;
            Logger::Log("Step " + std::to_string(i) + " address: " + Logger::GetHexStr(currentAddress), Logger::LogLevel::Info);
        }

        return std::nullopt;
    }

    template<typename T>
    static std::optional<T> ReadMemory(uintptr_t address) {
        if (!IsValidMemory(reinterpret_cast<void*>(address))) {
            Logger::Log("Invalid memory address for read: " + Logger::GetHexStr(address), Logger::LogLevel::Warning);
            return std::nullopt;
        }

        T value;
        if (!ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<LPCVOID>(address), &value, sizeof(T), nullptr)) {
            Logger::Log("Failed to read memory at " + Logger::GetHexStr(address), Logger::LogLevel::Error);
            return std::nullopt;
        }

        return value;
    }

    template<typename T>
    static bool WriteMemory(uintptr_t address, const T& value) {
        if (!IsValidMemory(reinterpret_cast<void*>(address))) {
            Logger::Log("Invalid memory address for write: " + Logger::GetHexStr(address), Logger::LogLevel::Warning);
            return false;
        }

        if (!WriteProcessMemory(GetCurrentProcess(), reinterpret_cast<LPVOID>(address), &value, sizeof(T), nullptr)) {
            Logger::Log("Failed to write memory at " + Logger::GetHexStr(address), Logger::LogLevel::Error);
            return false;
        }

        return true;
    }

private:
    static bool IsValidMemory(void* ptr) {
        if (ptr == nullptr) return false;

        MEMORY_BASIC_INFORMATION mbi;
        if (VirtualQuery(ptr, &mbi, sizeof(mbi)) == 0) {
            return false;
        }

        return (mbi.State == MEM_COMMIT) && ((mbi.Protect & PAGE_GUARD) == 0);
    }
};