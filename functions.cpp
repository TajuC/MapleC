#include "functions.h"
#include "Logger.h"
#include "hooks/hooks.h"
#include "SafeMemoryAccess.h"
#include <array>

int currentHP = 0, currentMP = 0;
std::string characterName = "Unknown";

namespace Functions {

    uintptr_t DerefPointerChain(uintptr_t base, const std::vector<uintptr_t>& offsets) {
        Logger::Log("DerefPointerChain called with base: " + Logger::GetHexStr(base));

        auto result = SafeMemoryAccess::DerefPointerChain<uintptr_t, uintptr_t>(base, offsets);
        if (result) {
            Logger::Log("Successfully dereferenced pointer chain. Final address: " + Logger::GetHexStr(*result), Logger::LogLevel::Info);
            return *result;
        } else {
            Logger::Log("Failed to dereference pointer chain", Logger::LogLevel::Error);
            return 0;
        }
    }

    void UpdateHPMP() {
        Logger::Log("Updating HP/MP");
        Logger::Log("HP_MPAddress: " + Logger::GetHexStr(HP_MPAddress));
        Logger::Log("Window::base: " + Logger::GetHexStr(Window::base));

        std::vector<uintptr_t> hpOffsets(HPoffsets, HPoffsets + HPoffsetsSize);
        std::vector<uintptr_t> mpOffsets(MPoffsets, MPoffsets + MPoffsetsSize);

        uintptr_t hpAddress = DerefPointerChain(Window::base + HP_MPAddress, hpOffsets);
        uintptr_t mpAddress = DerefPointerChain(Window::base + HP_MPAddress, mpOffsets);

        if (hpAddress) {
            auto hp = SafeMemoryAccess::ReadMemory<int>(hpAddress);
            if (hp) {
                currentHP = *hp;
                Logger::Log("Updated HP: " + std::to_string(currentHP));
            } else {
                Logger::Log("Failed to read HP value", Logger::LogLevel::Warning);
            }
        } else {
            Logger::Log("Failed to get HP address", Logger::LogLevel::Warning);
        }

        if (mpAddress) {
            auto mp = SafeMemoryAccess::ReadMemory<int>(mpAddress);
            if (mp) {
                currentMP = *mp;
                Logger::Log("Updated MP: " + std::to_string(currentMP));
            } else {
                Logger::Log("Failed to read MP value", Logger::LogLevel::Warning);
            }
        } else {
            Logger::Log("Failed to get MP address", Logger::LogLevel::Warning);
        }

        Logger::Log("HP/MP update completed");
    }

    void UpdateCharacterName() {
        Logger::Log("Updating character name");
        Logger::Log("characterNameBase: " + Logger::GetHexStr(characterNameBase));
        Logger::Log("Window::base: " + Logger::GetHexStr(Window::base));

        std::vector<ptrdiff_t> nameOffsets(characterNameOffsets, characterNameOffsets + characterNameOffsetsSize);
        auto charNameAddress = SafeMemoryAccess::DerefPointerChain<uintptr_t, ptrdiff_t>(Window::base + characterNameBase, nameOffsets);

        if (charNameAddress) {
            auto name = SafeMemoryAccess::ReadMemory<std::array<char, 23>>(*charNameAddress);
            if (name) {
                characterName = std::string(name->data());
                Logger::Log("Updated character name: " + characterName);
            } else {
                Logger::Log("Failed to read character name from memory", Logger::LogLevel::Warning);
            }
        } else {
            Logger::Log("Failed to get character name address", Logger::LogLevel::Warning);
        }
        Logger::Log("Character name update completed");
    }
}