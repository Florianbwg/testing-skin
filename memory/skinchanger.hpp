#pragma once

namespace ModFramework::Skinchanger {
    extern int g_DesiredKnifeID;

    // Startet die Skinchanger Logik (sollte im MainThread Loop aufgerufen werden)
    void Update();

    // Konfiguriert die gewünschten Einstellungen aus dem Menü
    void SetDesiredSkin(int itemDefIndex, int paintKit, float wear, int seed, int statTrak);
    
    // Konfiguriert das gewünschte Messer
    void SetDesiredKnife(int itemDefIndex);
}
