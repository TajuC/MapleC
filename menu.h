#ifndef UNICODE
#define UNICODE
#endif

#ifndef _UNICODE
#define _UNICODE
#endif

#pragma once
#include <d3d9.h>
#include <string>
#include "Core/globals.h"

namespace Menu {
    void Core();
    bool SetupDX() noexcept;
    void DestroyDX() noexcept;
    bool setup_wnd_class(const char* class_name) noexcept;
    void destroy_wnd_class() noexcept;
    bool setup_hwnd(const char* name) noexcept;
    void destroy_hwnd() noexcept;
    void SetupMenu(LPDIRECT3DDEVICE9 pDevice) noexcept;
    void Destroy() noexcept;
    void Render() noexcept;
    void RenderPacketGUI() noexcept;

    extern bool show_overlay;
    extern bool setup;
    extern bool is_ready;
    extern bool show_packet_gui;

    // Correctly use WNDCLASSEX to match with the Unicode setting
    extern WNDCLASSEX wnd_class;
    extern HWND hwnd;
    extern WNDPROC org_wndproc;
    extern IDirect3D9* d3d9;
    extern IDirect3DDevice9* device;
}

LRESULT CALLBACK WNDProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
