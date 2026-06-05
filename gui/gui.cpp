#include "gui.hpp"
#include "../memory/skinchanger.hpp"
#include <iostream>
#include <windows.h>

#include "../imgui/imgui.h"

namespace ModFramework::GUI {

    bool Menu::m_bShowMenu = true;
    
    // UI State initialization
    int Menu::m_SelectedCategory = 0;
    int Menu::m_SelectedWeapon = 0;
    int Menu::m_SelectedSkin = 0;
    float Menu::m_WearCondition = 0.0f;
    int Menu::m_PatternSeed = 1;
    bool Menu::m_StatTrak = false;
    char Menu::m_CustomNameBuffer[64] = "";

    // Dummy Data
    const std::vector<std::string> Menu::Categories = { "Pistols", "Rifles", "Knives", "Gloves" };
    const std::vector<std::vector<std::string>> Menu::Weapons = {
        { "Glock-18", "USP-S", "Desert Eagle", "P250" },
        { "AK-47", "M4A4", "M4A1-S", "AWP" },
        { "Karambit", "Butterfly Knife", "M9 Bayonet" },
        { "Sport Gloves", "Driver Gloves", "Hand Wraps" }
    };
    const std::vector<std::string> Menu::DummySkins = {
        "Default", "Asiimov", "Redline", "Fade", "Doppler", "Hyper Beast", "Vulcan", "Crimson Web"
    };

    bool Menu::Initialize() {
        std::cout << "[INFO] Initialisiere ImGui UI...\n";
        return true;
    }

    void Menu::Render() {
        // Toggle Menü mit der Einfügen (INSERT) Taste
        if (GetAsyncKeyState(VK_INSERT) & 1) {
            m_bShowMenu = !m_bShowMenu;
        }

        if (!m_bShowMenu) return;

        // Hauptfenster
        ImGui::Begin("Asset Modding Interface", &m_bShowMenu);
        RenderModdingInterface();
        ImGui::End();
    }

    void Menu::RenderModdingInterface() {
        // Wir teilen das Fenster in 3 Spalten auf
        ImGui::Columns(3, "ModdingColumns", true);

        // -----------------------------
        // Spalte 1: Kategorien & Waffen
        // -----------------------------
        ImGui::Text("Kategorien");
        ImGui::Separator();
        for (int i = 0; i < Categories.size(); ++i) {
            if (ImGui::Selectable(Categories[i].c_str(), m_SelectedCategory == i)) {
                m_SelectedCategory = i;
                m_SelectedWeapon = 0; // Reset Waffe wenn Kategorie wechselt
            }
        }
        
        ImGui::Text(""); // Spacer
        ImGui::Text("Waffen");
        ImGui::Separator();
        const auto& currentWeapons = Weapons[m_SelectedCategory];
        for (int i = 0; i < currentWeapons.size(); ++i) {
            if (ImGui::Selectable(currentWeapons[i].c_str(), m_SelectedWeapon == i)) {
                m_SelectedWeapon = i;
            }
        }

        ImGui::NextColumn();

        // -----------------------------
        // Spalte 2: Skin Auswahl (Texturen)
        // -----------------------------
        ImGui::Text("Verfügbare Skins");
        ImGui::Separator();
        
        // In einer echten UI würde man hier Texturen (ImGui::Image) rendern.
        // Für das Grundgerüst listen wir die Namen auf.
        for (int i = 0; i < DummySkins.size(); ++i) {
            // Ein leicht hervorgehobenes Selectable simulieren
            bool isSelected = (m_SelectedSkin == i);
            if (isSelected) {
                ImGui::PushStyleColor(ImGuiCol_Button, 0xFF4444FF); // Blaues Highlighting
            }
            
            std::string label = "Skin: " + DummySkins[i];
            if (ImGui::Selectable(label.c_str(), isSelected)) {
                m_SelectedSkin = i;
            }
            
            if (isSelected) {
                ImGui::PopStyleColor(1);
            }
        }

        ImGui::NextColumn();

        // -----------------------------
        // Spalte 3: Parameter & Apply
        // -----------------------------
        ImGui::Text("Skin Parameter");
        ImGui::Separator();

        ImGui::PushItemWidth(-1); // Volle Breite
        
        ImGui::Text("Zustand (Wear): %.2f", m_WearCondition);
        ImGui::SliderFloat("##Wear", &m_WearCondition, 0.0f, 1.0f, "%.3f");
        
        ImGui::Text("Muster (Seed): %d", m_PatternSeed);
        ImGui::SliderInt("##Seed", &m_PatternSeed, 1, 1000);
        
        ImGui::Text("");
        ImGui::Checkbox("StatTrak aktivieren", &m_StatTrak);
        
        ImGui::Text("Namensschild:");
        ImGui::InputText("##Name", m_CustomNameBuffer, sizeof(m_CustomNameBuffer));
        
        ImGui::PopItemWidth();

        ImGui::Text(""); ImGui::Text(""); // Spacer
        
        // Großer Apply Button
        ImGui::PushStyleColor(ImGuiCol_Button, 0xFF00AA00); // Grün
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, 0xFF00CC00);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, 0xFF008800);
        
        if (ImGui::Button("Skin Anwenden", ImVec2(-1, 40))) {
            // Mapping für Messer (Dummy Logik für den PoC)
            int itemDef = 0;
            if (Categories[m_SelectedCategory] == "Knives") {
                if (Weapons[m_SelectedCategory][m_SelectedWeapon] == "Karambit") itemDef = 507;
                else if (Weapons[m_SelectedCategory][m_SelectedWeapon] == "Butterfly Knife") itemDef = 515;
                else if (Weapons[m_SelectedCategory][m_SelectedWeapon] == "M9 Bayonet") itemDef = 508;
                
                Skinchanger::SetDesiredKnife(itemDef);
            }
            
            // Mapping für Skins
            int paintKit = 0;
            if (DummySkins[m_SelectedSkin] == "Doppler") paintKit = 415; // Ruby
            else if (DummySkins[m_SelectedSkin] == "Fade") paintKit = 38;
            else if (DummySkins[m_SelectedSkin] == "Asimov") paintKit = 279;
            else if (DummySkins[m_SelectedSkin] == "Redline") paintKit = 320;
            
            int statTrakValue = m_StatTrak ? 1337 : -1;
            
            Skinchanger::SetDesiredSkin(itemDef, paintKit, m_WearCondition, m_PatternSeed, statTrakValue);
            
            std::cout << "[APPLY] Wende Skin an: ItemDef=" << itemDef << " PaintKit=" << paintKit << "\n";
            // TODO: Fallback für normale Waffen (braucht exakte ItemDef IDs wie AK47=7)
            // HIER würde der Code zum Beschreiben des Speichers stehen:
            // z.B. Memory::Write(pEconItemView + Offsets::m_nFallbackPaintKit, skinID);
            // Engine->ForceFullUpdate();
        }
        
        ImGui::PopStyleColor(3);

        ImGui::Columns(1); // Spalten-Layout beenden
    }

    void Menu::Shutdown() {
        std::cout << "[INFO] Räume UI auf...\n";
    }

}
