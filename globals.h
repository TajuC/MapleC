#pragma once
#include <d3d9.h>
#include <string>
#include <vector>
#include <optional>

// Define the Packet struct
struct Packet {
    std::string type;
    int size;
    std::string status;
    std::string data;
};

namespace Menu {
    extern bool show_overlay;
    extern bool setup;
    extern bool is_ready;
    extern bool show_packet_gui;

    bool setup_wnd_class(const char* class_name) noexcept;
    void destroy_wnd_class() noexcept;
    bool setup_hwnd(const char* name) noexcept;
    void destroy_hwnd() noexcept;
    bool SetupDX() noexcept;
    void DestroyDX() noexcept;
    void Core();
    void SetupMenu(LPDIRECT3DDEVICE9 device) noexcept;
    void Destroy() noexcept;
    void Render() noexcept;
    void RenderPacketGUI() noexcept;

    extern HWND hwnd;
    extern WNDPROC org_wndproc;
    extern LPDIRECT3DDEVICE9 device;
    extern LPDIRECT3D9 d3d9;

    // Correctly use WNDCLASSEX to match with the Unicode setting
    extern WNDCLASSEX wnd_class;
}

namespace Window {
    extern uintptr_t base;
}

// Define your offsets and addresses here
extern const DWORD_PTR HPoffsets[];
extern const DWORD_PTR MPoffsets[];
extern const size_t HPoffsetsSize;
extern const size_t MPoffsetsSize;
extern const size_t characterNameOffsetsSize;
extern const DWORD_PTR HP_MPAddress;
extern const DWORD_PTR characterNameBase;
extern const DWORD_PTR characterNameOffsets[];

// Declare the vector of Packets
extern std::vector<Packet> capturedPackets;

// Function declarations
LRESULT CALLBACK WNDProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
