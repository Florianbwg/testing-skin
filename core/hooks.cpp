#include "hooks.hpp"
#include "../gui/gui.hpp"
#include "../memory/skinchanger.hpp"
#include "../memory/pattern.hpp"
#include "../memory/memory.hpp"
#include <iostream>
#include "../../deps/minhook/include/MinHook.h"

#include "../imgui/imgui.h"
#include "../imgui/backends/imgui_impl_win32.h"
#include "../imgui/backends/imgui_impl_dx11.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace ModFramework::Hooks {

    typedef HRESULT(WINAPI* Present_t)(IDXGISwapChain*, UINT, UINT);
    Present_t oPresent = nullptr;

    typedef void(__fastcall* FrameStageNotifyFn)(void*, int);
    FrameStageNotifyFn oFrameStageNotify = nullptr;

    SetModelFn oSetModel = nullptr;

    typedef LRESULT(WINAPI* WndProc_t)(HWND, UINT, WPARAM, LPARAM);
    WNDPROC oWndProc = nullptr;

    HWND g_hWnd = nullptr;
    ID3D11Device* g_pd3dDevice = nullptr;
    ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
    ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;
    bool g_InitImGui = false;

    // Helper to get dummy swapchain VTable
    bool GetD3D11SwapchainDeviceContext(void** pSwapchainTable, size_t size) {
        WNDCLASSEXA wc{ sizeof(WNDCLASSEX), CS_CLASSDC, DefWindowProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, "DummyClass", NULL };
        RegisterClassExA(&wc);
        HWND hWnd = CreateWindowA("DummyClass", "DummyWindow", WS_OVERLAPPEDWINDOW, 100, 100, 100, 100, NULL, NULL, wc.hInstance, NULL);

        DXGI_SWAP_CHAIN_DESC sd{};
        sd.BufferCount = 1;
        sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.OutputWindow = hWnd;
        sd.SampleDesc.Count = 1;
        sd.Windowed = TRUE;
        sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
        sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
        sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

        D3D_FEATURE_LEVEL featureLevel;
        const D3D_FEATURE_LEVEL featureLevelArray[1] = { D3D_FEATURE_LEVEL_11_0 };
        
        IDXGISwapChain* pDummySwapChain = nullptr;
        ID3D11Device* pDummyDevice = nullptr;
        ID3D11DeviceContext* pDummyContext = nullptr;

        if (FAILED(D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, featureLevelArray, 1, D3D11_SDK_VERSION, &sd, &pDummySwapChain, &pDummyDevice, &featureLevel, &pDummyContext))) {
            DestroyWindow(hWnd);
            UnregisterClassA(wc.lpszClassName, wc.hInstance);
            return false;
        }

        memcpy(pSwapchainTable, *(void***)pDummySwapChain, size);

        pDummySwapChain->Release();
        pDummyDevice->Release();
        pDummyContext->Release();
        DestroyWindow(hWnd);
        UnregisterClassA(wc.lpszClassName, wc.hInstance);
        return true;
    }

    LRESULT WINAPI hkWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        if (g_InitImGui) {
            if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
                return true;
            
            // Blockiere Spiel-Input, wenn Menü offen ist
            if (GUI::Menu::m_bShowMenu && (uMsg >= WM_MOUSEFIRST && uMsg <= WM_MOUSELAST)) {
                return true;
            }
        }
        return CallWindowProc((WNDPROC)oWndProc, hWnd, uMsg, wParam, lParam);
    }

    HRESULT __stdcall hkPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags) {
        if (!g_InitImGui) {
            if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&g_pd3dDevice))) {
                g_pd3dDevice->GetImmediateContext(&g_pd3dDeviceContext);
                DXGI_SWAP_CHAIN_DESC sd;
                pSwapChain->GetDesc(&sd);
                g_hWnd = sd.OutputWindow;

                ID3D11Texture2D* pBackBuffer;
                pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
                g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_mainRenderTargetView);
                pBackBuffer->Release();

                oWndProc = (WNDPROC)SetWindowLongPtr(g_hWnd, GWLP_WNDPROC, (LONG_PTR)hkWndProc);

                ImGui::CreateContext();
                ImGuiIO& io = ImGui::GetIO();
                io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
                
                ImGui_ImplWin32_Init(g_hWnd);
                ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

                g_InitImGui = true;
            }
        }

        // --- Dynamische Hooks und Updates ---
        static bool s_bSetModelHooked = false;
        if (!s_bSetModelHooked) {
            // Direktes Pattern für C_BaseModelEntity::SetModel (CS2 Update)
            uintptr_t setModelFnAddr = Memory::PatternScanner::FindPattern("client.dll", "48 89 5C 24 ? 48 89 74 24 ? 55 57 41 54 41 56 41 57 48 8D AC 24 ? ? ? ? 48 81 EC ? ? ? ? 48 8B 05");
            
            // Fallback auf altes Pattern
            if (!setModelFnAddr) {
                uintptr_t callAddr = Memory::PatternScanner::FindPattern("client.dll", "E8 ? ? ? ? F3 0F 10 4C 3B ?");
                if (callAddr) {
                    setModelFnAddr = Memory::PatternScanner::GetAbsoluteAddress(callAddr, 1, 5);
                }
            }

            if (setModelFnAddr) {
                std::cout << "[HOOKS] SetModel Pattern gefunden in hkPresent! Addr: 0x" << std::hex << setModelFnAddr << std::dec << "\n";
                // Wir speichern die Addresse, um sie direkt aufzurufen!
                oSetModel = reinterpret_cast<SetModelFn>(setModelFnAddr);
                s_bSetModelHooked = true;
            } else {
                std::cout << "[HOOKS] KRITISCHER FEHLER: SetModel Pattern nicht gefunden!\n";
                // Damit die Meldung nicht spamt, setzen wir es auf true (er versucht es nicht nochmal)
                s_bSetModelHooked = true; 
            }
        }

        // Skinchanger Update in hkFrameStageNotify ausführen, wo es hingehört
        // if (s_bSetModelHooked && oSetModel) {
        //     Skinchanger::Update();
        // }

        if (g_InitImGui) {
            ImGui_ImplDX11_NewFrame();
            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();

            if (GUI::Menu::m_bShowMenu)
                GUI::Menu::Render();

            ImGui::Render();
            g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        }

        return oPresent(pSwapChain, SyncInterval, Flags);
    }

    void __fastcall hkFrameStageNotify(void* rcx, int stage) {
        if (stage == 6) { // FRAME_NET_UPDATE_POSTDATAUPDATE_START
            if (s_bSetModelHooked && oSetModel) {
                Skinchanger::Update();
            }
        }
        oFrameStageNotify(rcx, stage);
    }

    void __fastcall hkSetModel(void* pThis, const char* pszModel) {
        // Obsolete
    }

    bool Initialize() {
        std::cout << "[INFO] Initialisiere DX11 Hook...\n";
        void* d3d11SwapchainVtable[18];
        if (GetD3D11SwapchainDeviceContext(d3d11SwapchainVtable, sizeof(d3d11SwapchainVtable))) {
            if (MH_Initialize() != MH_OK) {
                std::cerr << "[-] MinHook Init failed.\n";
                return false;
            }

            MH_CreateHook(d3d11SwapchainVtable[8], &hkPresent, reinterpret_cast<void**>(&oPresent));
            
            // 4. Source2Client Interface für FrameStageNotify holen
            void* pClient = Memory::GetInterface<void>("client.dll", "Source2Client002");
            if (pClient) {
                std::cout << "[HOOKS] Source2Client002 gefunden bei: " << pClient << "\n";
                void** pClientVTable = *reinterpret_cast<void***>(pClient);
                // FrameStageNotify ist in aktuellen CS2 Versionen auf Index 36!
                void* pFrameStageNotify = pClientVTable[36];
                
                if (MH_CreateHook(pFrameStageNotify, &hkFrameStageNotify, reinterpret_cast<LPVOID*>(&oFrameStageNotify)) != MH_OK) {
                    std::cout << "[HOOKS] Fehler beim Erstellen des FrameStageNotify Hooks\n";
                }
                std::cout << "[HOOKS] FrameStageNotify Hook bereit auf Index 36\n";
            }

            // 5. Hooks aktivieren (ohne SetModel, da SetModel in hkFrameStageNotify geladen wird)
            if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK) {
                std::cout << "[HOOKS] Fehler beim Aktivieren der Hooks\n";
                return false;
            }
            std::cout << "[+] Hooks aktiv!\n";
            return true;
        }
        return false;
    }

    void Shutdown() {
        MH_DisableHook(MH_ALL_HOOKS);
        MH_Uninitialize();
        if (oWndProc) {
            SetWindowLongPtr(g_hWnd, GWLP_WNDPROC, (LONG_PTR)oWndProc);
        }
        if (g_InitImGui) {
            ImGui_ImplDX11_Shutdown();
            ImGui_ImplWin32_Shutdown();
            ImGui::DestroyContext();
        }
        if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
        if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
        if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
    }
}
