#pragma once
#include <windows.h>
#include <d3d11.h>

namespace ModFramework::Hooks {
    bool Initialize();
    void Shutdown();

    // Hook Callbacks
    HRESULT WINAPI hkPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
    
    // WndProc Hook (Maus/Tastatur)
    extern WNDPROC oWndProc;
    LRESULT __stdcall hkWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    
    // FrameStageNotify Hook
    typedef void(__fastcall* FrameStageNotifyFn)(void*, int);
    extern FrameStageNotifyFn oFrameStageNotify;
    void __fastcall hkFrameStageNotify(void* rcx, int stage);

    // SetModel Hook
    typedef void(__fastcall* SetModelFn)(void*, const char*);
    extern SetModelFn oSetModel;
    void __fastcall hkSetModel(void* pThis, const char* pszModel);
}
