#pragma once
#include <d3d9.h>
#include "../Core/globals.h"

namespace hooks
{
    void Setup();
    void SetupHooks();
    void Destroy() noexcept;
    void DisableHooks();
	void EnableHooks();

    using CreateDeviceFn = HRESULT(WINAPI*)(IDirect3D9*, UINT, D3DDEVTYPE, HWND, DWORD, D3DPRESENT_PARAMETERS*, IDirect3DDevice9**);
    extern CreateDeviceFn oCreateDevice;
    HRESULT WINAPI hkCreateDevice(IDirect3D9* pD3D, UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags, D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DDevice9** ppReturnedDeviceInterface);

    using EndSceneFn = long(__stdcall*)(IDirect3DDevice9*);
    extern EndSceneFn EndSceneOrg;
    long __stdcall EndScene(IDirect3DDevice9* device) noexcept;

    using ResetFn = HRESULT(__stdcall*)(IDirect3DDevice9*, D3DPRESENT_PARAMETERS*);
    extern ResetFn ResetOrg;
    HRESULT __stdcall Reset(IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* params) noexcept;

    using ExpCalcFn = void(__fastcall*)(__int64, __int64*, __int64*, unsigned __int8);
    extern ExpCalcFn ExpCalcOrg;
    void __fastcall ExpCalc(__int64 qword_146F9A3E8, __int64* v4, __int64* v5, unsigned __int8 a2) noexcept;

    using MesosUpdateFn = void(__fastcall*)(uint64_t*, uint64_t);
    extern MesosUpdateFn MesosUpdateOrg;
    void __fastcall MesosUpdate(uint64_t* mesosPtr, uint64_t value) noexcept;

    extern float currentEXP;
    extern uint64_t currentMesos;

    constexpr void* VF(void* ptr, size_t index) noexcept;
}