#include "framework.hpp"
#include <windows.h>
#include <iostream>
#include "../offsets/offsets.hpp"
#include "../gui/gui.hpp"
#include "hooks.hpp"
#include "../memory/skinchanger.hpp"

namespace ModFramework {

    bool Framework::m_bIsRunning = false;

    bool Framework::Initialize() {
        // Optional: Konsole für Debugging allokieren
        AllocConsole();
        FILE* fp;
        freopen_s(&fp, "CONOUT$", "w", stdout);
        
        std::cout << "[INFO] ModFramework Initialisierung gestartet...\n";

        // 1. Offsets laden
        if (!Offsets::Manager::Load("offsets.json", "client.dll.json")) {
            std::cerr << "[ERROR] Fehler beim Laden der Offsets.\n";
            // In der Praxis könnten wir hier abbrechen, für ein Skript laufen wir weiter
        }

        // 2. DirectX Hook setzen
        if (!Hooks::Initialize()) {
            std::cerr << "[ERROR] Fehler bei der DX11 Hook Initialisierung.\n";
            return false;
        }

        // 3. GUI initialisieren
        if (!GUI::Menu::Initialize()) {
            std::cerr << "[ERROR] Fehler bei der GUI Initialisierung.\n";
            return false;
        }

        m_bIsRunning = true;
        return true;
    }

    void Framework::Run() {
        // Warte auf die 'END'-Taste, um das Framework zu beenden
        while (m_bIsRunning) {
            if (GetAsyncKeyState(VK_END) & 1) {
                m_bIsRunning = false;
            }
            Sleep(10);
        }
    }

    void Framework::Shutdown() {
        std::cout << "[INFO] ModFramework wird beendet...\n";

        // GUI und Hooks aufräumen
        Hooks::Shutdown();
        GUI::Menu::Shutdown();

        // Konsole schließen
        HWND consoleWnd = GetConsoleWindow();
        FreeConsole();
        if (consoleWnd) {
            PostMessage(consoleWnd, WM_CLOSE, 0, 0);
        }
    }
}
