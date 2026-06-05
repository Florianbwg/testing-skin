#pragma once

namespace ModFramework {
    class Framework {
    public:
        // Initialisiert Memory, liest Offsets und setzt GUI Hooks
        static bool Initialize();
        
        // Hält die DLL am Leben, z. B. auf Hotkeys zum Entladen warten
        static void Run();
        
        // Räumt auf (entfernt Hooks etc.)
        static void Shutdown();

    private:
        static bool m_bIsRunning;
    };
}
