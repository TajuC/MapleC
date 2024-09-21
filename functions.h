#pragma once
#include <string>
#include <vector>
#include <Windows.h>
#include "Core/globals.h"
#include <array>

namespace Functions {
    void UpdateHPMP();
    void UpdateCharacterName();
    uintptr_t DerefPointerChain(uintptr_t base, const std::vector<uintptr_t>& offsets);
}

extern int currentHP, currentMP;
extern std::string characterName;