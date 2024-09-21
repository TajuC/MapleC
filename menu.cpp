#include "menu.h"
#include "Logger.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx9.h"
#include "hooks/hooks.h"
#include "functions.h"
#include "SafeMemoryAccess.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace Menu {
    bool show_overlay = true;
    bool setup = false;
    bool is_ready = false;
    bool show_packet_gui = false;
    HWND hwnd = nullptr;
    WNDPROC org_wndproc = nullptr;
    IDirect3DDevice9* device = nullptr;

    // Core function for initializing the menu
    void Menu::Core() {
        Logger::Log("Menu::Core() called", Logger::LogLevel::Info);

        try {
            Logger::Log("Initializing Menu variables", Logger::LogLevel::Info);
            show_overlay = true;
            setup = false;
            is_ready = false;
            show_packet_gui = false;

            Logger::Log("Initializing ImGui", Logger::LogLevel::Info);
            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO(); (void)io;

            // Find the MapleStory window by class name
            Logger::Log("Looking for MapleStory window (MapleStoryClass)", Logger::LogLevel::Info);
            hwnd = FindWindowW(L"MapleStoryClass", NULL);
            if (!hwnd) {
                Logger::Log("Failed to find MapleStory window", Logger::LogLevel::Error);
                throw std::runtime_error("Failed to find MapleStory window");
            }

            // Initialize ImGui Win32 implementation
            Logger::Log("Setting up ImGui Win32 implementation", Logger::LogLevel::Info);
            if (!ImGui_ImplWin32_Init(hwnd)) {
                Logger::Log("Failed to initialize ImGui Win32 implementation", Logger::LogLevel::Error);
                throw std::runtime_error("Failed to initialize ImGui Win32 implementation");
            }

            // Check if DirectX device is initialized
            Logger::Log("Checking if DirectX device is initialized", Logger::LogLevel::Info);
            if (device == nullptr) {
                Logger::Log("DirectX device is not initialized. Waiting for device...", Logger::LogLevel::Warning);
                return; // Exit early, wait for device creation via CreateDevice hook
            }

            Logger::Log("DirectX device address: " + Logger::GetHexStr(reinterpret_cast<UINT64>(device)), Logger::LogLevel::Info);

            // Initialize ImGui DirectX 9 implementation
            Logger::Log("Setting up ImGui DirectX 9 implementation", Logger::LogLevel::Info);
            if (!ImGui_ImplDX9_Init(device)) {
                Logger::Log("Failed to initialize ImGui DirectX 9 implementation", Logger::LogLevel::Error);
                throw std::runtime_error("Failed to initialize ImGui DirectX 9 implementation");
            }

            // Set ImGui style
            Logger::Log("Setting ImGui style", Logger::LogLevel::Info);
            ImGui::StyleColorsDark();

            setup = true;
            is_ready = true;
            Logger::Log("Menu core initialized successfully", Logger::LogLevel::Info);
        }
        catch (const std::exception& e) {
            Logger::Log("Exception in Menu::Core(): " + std::string(e.what()), Logger::LogLevel::Critical);
        }
        catch (...) {
            Logger::Log("Unknown exception in Menu::Core()", Logger::LogLevel::Critical);
        }
    }

    // Function to set up window using provided class name
    bool setup_hwnd(const wchar_t* name) noexcept {
        Logger::Log("Menu::setup_hwnd() called", Logger::LogLevel::Info);

        hwnd = CreateWindowExW(
            0,                       // Optional window styles
            name,                    // Class name, passed as parameter
            L"Menu Overlay",         // Window name (note the L prefix for wide string)
            WS_OVERLAPPEDWINDOW,     // Window style
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,  // Size and position
            NULL,                    // Parent window    
            NULL,                    // Menu
            GetModuleHandle(NULL),   // Instance handle
            NULL                     // Additional application data
        );

        if (!hwnd) {
            Logger::Log("Failed to create window. Error code: " + std::to_string(GetLastError()), Logger::LogLevel::Error);
            return false;
        }

        Logger::Log("Window created successfully", Logger::LogLevel::Info);
        return true;
    }

    // Function to destroy ImGui and DirectX contexts
    void Destroy() noexcept {
        Logger::Log("Menu::Destroy() called");
        if (setup) {
            ImGui_ImplDX9_Shutdown();
            ImGui_ImplWin32_Shutdown();
            ImGui::DestroyContext();
            setup = false;
        }
        device = nullptr;
    }

    // Render function to handle ImGui UI rendering
    void Render() noexcept {
        if (!setup) return;

        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_FirstUseEver);

        if (ImGui::Begin("MapleC Menu", &show_overlay)) {
            // HP/MP Display
            std::vector<uintptr_t> hpOffsets(HPoffsets, HPoffsets + HPoffsetsSize);
            std::vector<uintptr_t> mpOffsets(MPoffsets, MPoffsets + MPoffsetsSize);

            auto hpAddress = SafeMemoryAccess::DerefPointerChain<uintptr_t, uintptr_t>(Window::base + HP_MPAddress, hpOffsets);
            if (hpAddress) {
                auto hp = SafeMemoryAccess::ReadMemory<int>(*hpAddress);
                if (hp) {
                    ImGui::Text("HP: %d", *hp);
                }
                else {
                    ImGui::Text("Failed to read HP");
                }
            }
            else {
                ImGui::Text("Failed to dereference HP address");
            }

            auto mpAddress = SafeMemoryAccess::DerefPointerChain<uintptr_t, uintptr_t>(Window::base + HP_MPAddress, mpOffsets);
            if (mpAddress) {
                auto mp = SafeMemoryAccess::ReadMemory<int>(*mpAddress);
                if (mp) {
                    ImGui::Text("MP: %d", *mp);
                }
                else {
                    ImGui::Text("Failed to read MP");
                }
            }
            else {
                ImGui::Text("Failed to dereference MP address");
            }

            ImGui::Text("EXP: %.2f%%", hooks::currentEXP);
            ImGui::Text("Mesos: %llu", hooks::currentMesos);

            if (ImGui::Button("Deactivate")) {
                is_ready = false;
                Logger::Log("Deactivate button pressed, deactivating mod features");
                hooks::DisableHooks();
            }
        }

        ImGui::End();

        if (show_packet_gui) {
            RenderPacketGUI();
        }
    }

    // Function to render packet GUI for additional controls
    void RenderPacketGUI() noexcept {
        ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);
        ImGui::Begin("Packet GUI", &show_packet_gui);

        static char packetData[1024] = "";
        ImGui::InputTextMultiline("Packet Data", packetData, IM_ARRAYSIZE(packetData));

        if (ImGui::Button("Send Packet")) {
            Logger::Log("Sending packet: " + std::string(packetData), Logger::LogLevel::Info);
        }

        ImGui::End();
    }

    // Function to set up the DirectX device and ImGui context
    void SetupMenu(LPDIRECT3DDEVICE9 pDevice) noexcept {
        Logger::Log("Menu::SetupMenu() called");
        device = pDevice;
        Logger::Log("Device set in SetupMenu: " + Logger::GetHexStr(reinterpret_cast<UINT64>(device)), Logger::LogLevel::Info);
        ImGui::CreateContext();
        ImGui_ImplWin32_Init(hwnd);
        ImGui_ImplDX9_Init(device);
        ImGui::StyleColorsDark();
        Logger::Log("Menu setup completed");
    }

    // Function to create the DirectX device
    bool create_device(HWND window, IDirect3D9* d3d9, IDirect3DDevice9** device) noexcept {
        Logger::Log("Creating DirectX device");
        D3DPRESENT_PARAMETERS params = {};
        params.Windowed = TRUE;
        params.SwapEffect = D3DSWAPEFFECT_DISCARD;
        params.BackBufferFormat = D3DFMT_UNKNOWN;
        params.EnableAutoDepthStencil = TRUE;
        params.AutoDepthStencilFormat = D3DFMT_D16;
        params.PresentationInterval = D3DPRESENT_INTERVAL_ONE;

        HRESULT hr = d3d9->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, window, D3DCREATE_HARDWARE_VERTEXPROCESSING, &params, device);

        if (FAILED(hr)) {
            Logger::Log("Failed to create D3D device", Logger::LogLevel::Error);
            return false;
        }

        Logger::Log("DirectX setup completed");
        return true;
    }

    // Function to destroy DirectX device and related resources
    void DestroyDX() noexcept {
        Logger::Log("Destroying DirectX");
        if (device) {
            device->Release();
            device = nullptr;
        }
        Logger::Log("DirectX destroyed");
    }
}

// Custom WNDProc to handle ImGui events
LRESULT CALLBACK WNDProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (Menu::show_overlay && ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    return CallWindowProc(Menu::org_wndproc, hWnd, msg, wParam, lParam);
}
