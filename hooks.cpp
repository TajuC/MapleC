#include "hooks.h"
#include <stdexcept>
#include <d3d9.h>
#include "../minhook/MinHook.h"
#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_win32.h"
#include "../imgui/imgui_impl_dx9.h"
#include "../Logger.h"
#include "../Core/globals.h"
#include "../menu.h"
#include "../SafeMemoryAccess.h"
#include <string>

namespace hooks
{
    float currentEXP = 0.0f;
    uint64_t currentMesos = 0;
    EndSceneFn EndSceneOrg = nullptr;
    ResetFn ResetOrg = nullptr;
    ExpCalcFn ExpCalcOrg = nullptr;
    MesosUpdateFn MesosUpdateOrg = nullptr;
    CreateDeviceFn oCreateDevice = nullptr;

    constexpr void* VF(void* ptr, size_t index) noexcept
    {
        return (*static_cast<void***>(ptr))[index];
    }
}

HRESULT WINAPI hooks::hkCreateDevice(IDirect3D9* pD3D, UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags, D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DDevice9** ppReturnedDeviceInterface) {
    Logger::Log("hkCreateDevice called", Logger::LogLevel::Info);

    HRESULT hr = oCreateDevice(pD3D, Adapter, DeviceType, hFocusWindow, BehaviorFlags, pPresentationParameters, ppReturnedDeviceInterface);
    if (SUCCEEDED(hr) && ppReturnedDeviceInterface && *ppReturnedDeviceInterface) {
        Menu::device = *ppReturnedDeviceInterface;  // Capture the DirectX device
        Logger::Log("DirectX device captured successfully: " + Logger::GetHexStr(reinterpret_cast<UINT64>(Menu::device)), Logger::LogLevel::Info);
        hooks::SetupHooks();  // Setup additional hooks after device creation
    }
    else {
        Logger::Log("Failed to capture DirectX device. HRESULT: " + Logger::GetHexStr(hr), Logger::LogLevel::Error);
    }

    return hr;
}

void hooks::Setup()
{
    Logger::Log("Initializing MinHook...", Logger::LogLevel::Info);
    if (MH_Initialize() != MH_OK)
    {
        Logger::Log("Failed to initialize MinHook!", Logger::LogLevel::Error);
        throw std::runtime_error("Failed to init minhook!");
    }
    Logger::Log("MinHook initialized successfully.", Logger::LogLevel::Info);

    // Log the current Window::base
    Logger::Log("Current Window::base: " + Logger::GetHexStr(Window::base), Logger::LogLevel::Info);

    // Create a Direct3D9 object
    HMODULE d3d9Module = GetModuleHandleA("d3d9.dll");
    if (!d3d9Module)
    {
        DWORD error = GetLastError();
        Logger::Log("Failed to get d3d9.dll module handle. Error: " + std::to_string(error) + " (" + std::system_category().message(error) + ")", Logger::LogLevel::Error);
        throw std::runtime_error("Failed to get d3d9.dll module handle");
    }

    Logger::Log("d3d9.dll module handle obtained: " + Logger::GetHexStr(reinterpret_cast<uintptr_t>(d3d9Module)), Logger::LogLevel::Info);

    LPDIRECT3D9 pD3D = Direct3DCreate9(D3D_SDK_VERSION);
    if (!pD3D)
    {
        Logger::Log("Failed to create Direct3D9 object", Logger::LogLevel::Error);
        throw std::runtime_error("Failed to create Direct3D9 object");
    }

    Logger::Log("Direct3D9 object created successfully", Logger::LogLevel::Info);

    // Set up dummy parameters to create a temporary device
    D3DPRESENT_PARAMETERS d3dpp = {};
    d3dpp.Windowed = TRUE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;

    HWND hwndDummy = CreateWindowA("STATIC", "Dummy Window", WS_OVERLAPPEDWINDOW, 0, 0, 100, 100, NULL, NULL, NULL, NULL);
    if (!hwndDummy)
    {
        Logger::Log("Failed to create dummy window", Logger::LogLevel::Error);
        pD3D->Release();
        throw std::runtime_error("Failed to create dummy window");
    }

    LPDIRECT3DDEVICE9 pDevice = nullptr;
    if (FAILED(pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwndDummy, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &pDevice)))
    {
        Logger::Log("Failed to create Direct3D9 device", Logger::LogLevel::Error);
        DestroyWindow(hwndDummy);
        pD3D->Release();
        throw std::runtime_error("Failed to create Direct3D9 device");
    }

    Logger::Log("Direct3D9 device created successfully", Logger::LogLevel::Info);

    // Get the virtual function table (vtable) for the Direct3D9 device
    void* createDeviceAddr = (*reinterpret_cast<void***>(pDevice))[16];  // CreateDevice is at index 16 in vtable
    if (createDeviceAddr)
    {
        Logger::Log("CreateDevice vtable address obtained: " + Logger::GetHexStr(reinterpret_cast<uintptr_t>(createDeviceAddr)), Logger::LogLevel::Info);
        if (MH_CreateHook(createDeviceAddr, &hkCreateDevice, reinterpret_cast<LPVOID*>(&oCreateDevice)) == MH_OK)
        {
            Logger::Log("CreateDevice hooked successfully", Logger::LogLevel::Info);
        }
        else
        {
            Logger::Log("Failed to hook CreateDevice", Logger::LogLevel::Error);
        }
    }
    else
    {
        Logger::Log("Failed to get CreateDevice vtable address", Logger::LogLevel::Error);
    }

    // Cleanup
    pDevice->Release();
    DestroyWindow(hwndDummy);
    pD3D->Release();

    // Enable the hook
    Logger::Log("Enabling CreateDevice hook...", Logger::LogLevel::Info);
    MH_STATUS enableStatus = MH_EnableHook(MH_ALL_HOOKS);
    if (enableStatus != MH_OK)
    {
        Logger::Log("Failed to enable CreateDevice hook!", Logger::LogLevel::Error);
        throw std::runtime_error("Failed to enable CreateDevice hook");
    }
    Logger::Log("CreateDevice hook enabled successfully!", Logger::LogLevel::Info);
}

void hooks::SetupHooks()
{
    if (!Menu::device)
    {
        Logger::Log("Menu::device is null!", Logger::LogLevel::Error);
        throw std::runtime_error("Menu::device is null");
    }

    Logger::Log("Creating hook for EndScene...", Logger::LogLevel::Info);
    void* endSceneAddr = VF(Menu::device, 42);
    Logger::Log("EndScene address: " + Logger::GetHexStr(reinterpret_cast<UINT64>(endSceneAddr)), Logger::LogLevel::Info);

    MH_STATUS status = MH_CreateHook(endSceneAddr, &hooks::EndScene, reinterpret_cast<void**>(&EndSceneOrg));
    if (status != MH_OK)
    {
        Logger::Log("Failed to create hook for EndScene! MH_STATUS: " + std::to_string(static_cast<int>(status)), Logger::LogLevel::Error);
        throw std::runtime_error("Unable to hook EndScene");
    }
    Logger::Log("Hooked EndScene successfully.", Logger::LogLevel::Info);

    Logger::Log("Creating hook for Reset...", Logger::LogLevel::Info);
    void* resetAddr = VF(Menu::device, 16);
    Logger::Log("Reset address: " + Logger::GetHexStr(reinterpret_cast<UINT64>(resetAddr)), Logger::LogLevel::Info);

    if (resetAddr == nullptr)
    {
        Logger::Log("Failed to get Reset address", Logger::LogLevel::Error);
        throw std::runtime_error("Failed to get Reset address");
    }

    // Check if hook for Reset is already created
    if (MH_EnableHook(resetAddr) == MH_ERROR_ALREADY_CREATED)
    {
        Logger::Log("Reset hook already created. Skipping hook creation.", Logger::LogLevel::Warning);
    }
    else
    {
        status = MH_CreateHook(resetAddr, &hooks::Reset, reinterpret_cast<void**>(&ResetOrg));
        if (status != MH_OK)
        {
            Logger::Log("Failed to create hook for Reset! MH_STATUS: " + std::to_string(static_cast<int>(status)), Logger::LogLevel::Error);
            throw std::runtime_error("Unable to hook Reset");
        }
        Logger::Log("Hooked Reset successfully.", Logger::LogLevel::Info);
    }

    Logger::Log("Creating hook for ExpCalc...", Logger::LogLevel::Info);
    constexpr uintptr_t expCalcAddress = 0x144AB8950;  // This should be the absolute address
    void* expCalcTarget = reinterpret_cast<void*>(expCalcAddress);
    
    Logger::Log("ExpCalc target address: " + Logger::GetHexStr(reinterpret_cast<UINT64>(expCalcTarget)), Logger::LogLevel::Info);

    DWORD oldProtect;
    if (!VirtualProtect(expCalcTarget, 5, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        Logger::Log("Failed to change memory protection for ExpCalc. Error: " + std::to_string(GetLastError()), Logger::LogLevel::Error);
        throw std::runtime_error("Failed to change memory protection for ExpCalc");
    }

    status = MH_CreateHook(expCalcTarget, &hooks::ExpCalc, reinterpret_cast<void**>(&ExpCalcOrg));
    if (status != MH_OK)
    {
        Logger::Log("Failed to create hook for ExpCalc! MH_STATUS: " + std::to_string(static_cast<int>(status)), Logger::LogLevel::Error);
        throw std::runtime_error("Unable to hook ExpCalc");
    }
    Logger::Log("Hooked ExpCalc successfully.", Logger::LogLevel::Info);

    DWORD temp;
    if (!VirtualProtect(expCalcTarget, 5, oldProtect, &temp)) {
        Logger::Log("Failed to restore memory protection for ExpCalc. Error: " + std::to_string(GetLastError()), Logger::LogLevel::Error);
        throw std::runtime_error("Failed to restore memory protection for ExpCalc");
    }

    Logger::Log("Creating hook for MesosUpdate...", Logger::LogLevel::Info);
    constexpr uintptr_t mesosUpdateAddress = 0x144B9A941;  // This is the address from your script
    void* mesosUpdateTarget = reinterpret_cast<void*>(Window::base + mesosUpdateAddress - 0x140000000);
    
    Logger::Log("MesosUpdate target address: " + Logger::GetHexStr(reinterpret_cast<UINT64>(mesosUpdateTarget)), Logger::LogLevel::Info);

    if (!VirtualProtect(mesosUpdateTarget, 5, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        Logger::Log("Failed to change memory protection for MesosUpdate. Error: " + std::to_string(GetLastError()), Logger::LogLevel::Error);
        throw std::runtime_error("Failed to change memory protection for MesosUpdate");
    }

    status = MH_CreateHook(mesosUpdateTarget, &hooks::MesosUpdate, reinterpret_cast<void**>(&MesosUpdateOrg));
    if (status != MH_OK)
    {
        Logger::Log("Failed to create hook for MesosUpdate! MH_STATUS: " + std::to_string(static_cast<int>(status)), Logger::LogLevel::Error);
        throw std::runtime_error("Unable to hook MesosUpdate");
    }
    Logger::Log("Hooked MesosUpdate successfully.", Logger::LogLevel::Info);

    if (!VirtualProtect(mesosUpdateTarget, 5, oldProtect, &temp)) {
        Logger::Log("Failed to restore memory protection for MesosUpdate. Error: " + std::to_string(GetLastError()), Logger::LogLevel::Error);
        throw std::runtime_error("Failed to restore memory protection for MesosUpdate");
    }

    Logger::Log("Enabling all hooks...", Logger::LogLevel::Info);
    status = MH_EnableHook(MH_ALL_HOOKS);
    if (status != MH_OK)
    {
        Logger::Log("Failed to enable hooks! MH_STATUS: " + std::to_string(static_cast<int>(status)), Logger::LogLevel::Error);
        throw std::runtime_error("Failed to enable hooks");
    }
    Logger::Log("All hooks enabled successfully.", Logger::LogLevel::Info);
}

void hooks::Destroy() noexcept {
    Logger::Log("Disabling all hooks...", Logger::LogLevel::Info);

    MH_DisableHook(MH_ALL_HOOKS);
    Logger::Log("Removing all hooks...", Logger::LogLevel::Info);

    MH_RemoveHook(MH_ALL_HOOKS);
    Logger::Log("Uninitializing MinHook...", Logger::LogLevel::Info);

    MH_Uninitialize();
    Logger::Log("Hooks destroyed successfully.", Logger::LogLevel::Info);
}

void hooks::EnableHooks()
{
    Logger::Log("Enabling hooks...", Logger::LogLevel::Info);
    if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK)
    {
        Logger::Log("Failed to enable hooks!", Logger::LogLevel::Error);
        throw std::runtime_error("Failed to enable hooks");
    }
    Logger::Log("Hooks enabled successfully.", Logger::LogLevel::Info);
}

void hooks::DisableHooks()
{
    Logger::Log("Disabling hooks...", Logger::LogLevel::Info);
    if (MH_DisableHook(MH_ALL_HOOKS) != MH_OK)
    {
        Logger::Log("Failed to disable hooks!", Logger::LogLevel::Error);
        // Don't throw here, just log the error
    }
    else
    {
        Logger::Log("Hooks disabled successfully.", Logger::LogLevel::Info);
    }
}

HRESULT __stdcall hooks::Reset(IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* params) noexcept
{
    Logger::Log("Reset hook called", Logger::LogLevel::Info);
    try
    {
        Logger::Log("Invalidating ImGui device objects", Logger::LogLevel::Info);
        ImGui_ImplDX9_InvalidateDeviceObjects();
        
        Logger::Log("Calling original Reset function", Logger::LogLevel::Info);
        HRESULT result = ResetOrg(device, params);
        
        if (SUCCEEDED(result))
        {
            Logger::Log("Device reset successful. Recreating ImGui objects.", Logger::LogLevel::Info);
            ImGui_ImplDX9_CreateDeviceObjects();
        }
        else
        {
            Logger::Log("Device reset failed. HRESULT: " + Logger::GetHexStr(result), Logger::LogLevel::Error);
        }

        return result;
    }
    catch (const std::exception& e)
    {
        Logger::Log("Exception in Reset hook: " + std::string(e.what()), Logger::LogLevel::Error);
        return ResetOrg(device, params);
    }
    catch (...)
    {
        Logger::Log("Unknown exception in Reset hook", Logger::LogLevel::Error);
        return ResetOrg(device, params);
    }
}

void __fastcall hooks::ExpCalc(__int64 qword_146F9A3E8, __int64* v4, __int64* v5, unsigned __int8 a2) noexcept
{
    try
    {
        // Call the original function
        ExpCalcOrg(qword_146F9A3E8, v4, v5, a2);

        // Calculate the new EXP percentage
        auto v4Value = SafeMemoryAccess::ReadMemory<__int64>(reinterpret_cast<uintptr_t>(v4));
        auto v5Value = SafeMemoryAccess::ReadMemory<__int64>(reinterpret_cast<uintptr_t>(v5));

        if (v4Value && v5Value && *v5Value != 0) {
            float newEXP = static_cast<float>(*v4Value) * 100.0f / static_cast<float>(*v5Value);
            if (newEXP != currentEXP) {
                currentEXP = newEXP;
                Logger::Log("EXP updated: " + std::to_string(currentEXP) + "%", Logger::LogLevel::Info);
            }
        }
    }
    catch (const std::exception& e)
    {
        Logger::Log("Exception in ExpCalc hook: " + std::string(e.what()), Logger::LogLevel::Error);
    }
    catch (...)
    {
        Logger::Log("Unknown exception in ExpCalc hook", Logger::LogLevel::Error);
    }
}

void __fastcall hooks::MesosUpdate(uint64_t* mesosPtr, uint64_t value) noexcept
{
    try
    {
        MesosUpdateOrg(mesosPtr, value);

        auto newMesos = SafeMemoryAccess::ReadMemory<uint64_t>(reinterpret_cast<uintptr_t>(mesosPtr));
        if (newMesos) {
            currentMesos = *newMesos;
            Logger::Log("Mesos updated: " + std::to_string(currentMesos), Logger::LogLevel::Info);
        }
    }
    catch (const std::exception& e)
    {
        Logger::Log("Exception in MesosUpdate hook: " + std::string(e.what()), Logger::LogLevel::Error);
    }
    catch (...)
    {
        Logger::Log("Unknown exception in MesosUpdate hook", Logger::LogLevel::Error);
    }
}

long __stdcall hooks::EndScene(IDirect3DDevice9* device) noexcept
{
    static bool init = false;
    if (!init)
    {
        Menu::SetupMenu(device);
        init = true;
    }

    ImGui_ImplDX9_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    Menu::Render();

    ImGui::EndFrame();
    ImGui::Render();
    ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());

    return EndSceneOrg(device);
}