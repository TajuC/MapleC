#include "globals.h"

// Define offsets and addresses
const DWORD_PTR HP_MPAddress = 0x0723BC40;
const DWORD_PTR HPoffsets[] = { 0x1E0, 0x268, 0x0, 0x240, 0x0, 0x2C0, 0x40 };
const DWORD_PTR MPoffsets[] = { 0x1E0, 0x268, 0x0, 0x2D0, 0x40 };
const DWORD_PTR characterNameBase = 0x072298D0;
const DWORD_PTR characterNameOffsets[] = { 0x20, 0xC };

// Define size constants
const size_t HPoffsetsSize = sizeof(HPoffsets) / sizeof(HPoffsets[0]);
const size_t MPoffsetsSize = sizeof(MPoffsets) / sizeof(MPoffsets[0]);
const size_t characterNameOffsetsSize = sizeof(characterNameOffsets) / sizeof(characterNameOffsets[0]);
std::vector<Packet> capturedPackets;
namespace Window {
    uintptr_t base = 0;
}