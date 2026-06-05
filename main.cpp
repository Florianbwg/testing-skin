#include <windows.h>
#include <thread>
#include "core/framework.hpp"

// Haupt-Thread für das Framework
DWORD WINAPI MainThread(LPVOID lpParam) {
    HMODULE hModule = static_cast<HMODULE>(lpParam);

    // Initialisiere das Framework
    if (ModFramework::Framework::Initialize()) {
        // Starte die Hauptschleife (z. B. auf Hotkeys warten)
        ModFramework::Framework::Run();
    }

    // Wenn die Schleife beendet wird, räume auf
    ModFramework::Framework::Shutdown();

    // Entlade die DLL (Freigabe des Speichers)
    FreeLibraryAndExitThread(hModule, 0);
    return 0;
}

// Einsprungspunkt der DLL (Wird beim Injezieren von Windows aufgerufen)
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
            // DEBUG: Eine extrem simple Message Box, um zu garantieren, dass unser Code überhaupt vom Spiel ausgeführt wird.
            MessageBoxA(NULL, "Die DLL wurde erfolgreich von CS2 geladen und DllMain wurde ausgefuehrt!", "ModFramework Debug", MB_OK | MB_ICONINFORMATION);
            
            // Verhindere Thread-Attach-Benachrichtigungen für bessere Performance
            DisableThreadLibraryCalls(hModule);
            
            // Erstelle einen neuen Thread, um den Loader-Lock nicht zu blockieren
            if (HANDLE hThread = CreateThread(nullptr, 0, MainThread, hModule, 0, nullptr)) {
                CloseHandle(hThread);
            }
            break;
            
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}
