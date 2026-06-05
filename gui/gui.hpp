#pragma once
#include <string>
#include <vector>

namespace ModFramework::GUI {

    class Menu {
    public:
        static bool m_bShowMenu;
        
        // Initialisiert ImGui und setzt Rendering-Hooks
        static bool Initialize();
        
        // Baut das ImGui Fenster auf
        static void Render();
        
        // Räumt ImGui und die Hooks auf
        static void Shutdown();

    private:
        
        // UI Layout rendering
        static void RenderModdingInterface();

        // UI State
        static int m_SelectedCategory;
        static int m_SelectedWeapon;
        static int m_SelectedSkin;
        
        static float m_WearCondition;
        static int m_PatternSeed;
        static bool m_StatTrak;
        static char m_CustomNameBuffer[64];
        
        // Dummy Data for the mockup
        static const std::vector<std::string> Categories;
        static const std::vector<std::vector<std::string>> Weapons;
        static const std::vector<std::string> DummySkins;
    };

}
