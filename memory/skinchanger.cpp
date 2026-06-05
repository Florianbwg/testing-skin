#include "skinchanger.hpp"
#include "memory.hpp"
#include "pattern.hpp"
#include "../core/hooks.hpp"
#include "../offsets/cs2_dumper/offsets.hpp"
#include "../offsets/cs2_dumper/client_dll.hpp"
#include <iostream>

namespace ModFramework::Skinchanger {

    // Konfiguration (von der GUI)
    struct SkinConfig {
        int targetWeaponID = 0;
        int paintKit = 0;
        float wear = 0.001f;
        int seed = 1;
        int statTrak = -1;
    };

    const char* GetKnifeModel(int defIdx) {
        switch (defIdx) {
            case 500: return "models/weapons/v_knife_bayonet.vmdl";
            case 503: return "models/weapons/v_knife_css.vmdl";
            case 505: return "models/weapons/v_knife_flip.vmdl";
            case 506: return "models/weapons/v_knife_gut.vmdl";
            case 507: return "models/weapons/v_knife_karam.vmdl";
            case 508: return "models/weapons/v_knife_m9_bay.vmdl";
            case 509: return "models/weapons/v_knife_tactical.vmdl";
            case 512: return "models/weapons/v_knife_falchion_advanced.vmdl";
            case 514: return "models/weapons/v_knife_survival_bowie.vmdl";
            case 515: return "models/weapons/v_knife_butterfly.vmdl";
            case 516: return "models/weapons/v_knife_push.vmdl";
            case 519: return "models/weapons/v_knife_ursus.vmdl";
            case 520: return "models/weapons/v_knife_gypsy_jackknife.vmdl";
            case 522: return "models/weapons/v_knife_stiletto.vmdl";
            case 523: return "models/weapons/v_knife_widowmaker.vmdl";
            default: return "models/weapons/v_knife_default_t.vmdl";
        }
    }

    int g_DesiredKnifeID = 507; // 507 = Karambit, 0 = Default
    SkinConfig g_CurrentConfig;

    void SetDesiredSkin(int itemDefIndex, int paintKit, float wear, int seed, int statTrak) {
        g_CurrentConfig.targetWeaponID = itemDefIndex;
        g_CurrentConfig.paintKit = paintKit;
        g_CurrentConfig.wear = wear;
        g_CurrentConfig.seed = seed;
        g_CurrentConfig.statTrak = statTrak;
    }

    void SetDesiredKnife(int itemDefIndex) {
        g_DesiredKnifeID = itemDefIndex;
    }

    void Update() {
        static ULONGLONG s_lastPrintTime = 0;
        ULONGLONG currentTime = GetTickCount64();
        bool bDebug = (currentTime - s_lastPrintTime > 2000); // Alle 2 Sekunden printen
        if (bDebug) s_lastPrintTime = currentTime;

        uintptr_t client = Memory::GetModuleBase("client.dll");
        if (!client) { if (bDebug) std::cout << "[ERR] client.dll nicht gefunden\n"; return; }

        // Finde den lokalen Spieler
        uintptr_t localPlayerPawn = Memory::Read<uintptr_t>(client + cs2_dumper::offsets::client_dll::dwLocalPlayerPawn);
        if (!localPlayerPawn) { if (bDebug) std::cout << "[ERR] localPlayerPawn ist 0\n"; return; }

        // Finde WeaponServices
        uintptr_t weaponServices = Memory::Read<uintptr_t>(localPlayerPawn + cs2_dumper::schemas::client_dll::C_BasePlayerPawn::m_pWeaponServices);
        if (!weaponServices) { if (bDebug) std::cout << "[ERR] weaponServices ist 0\n"; return; }

        // Lese Active Weapon Handle
        uint32_t activeWeaponHandle = Memory::Read<uint32_t>(weaponServices + cs2_dumper::schemas::client_dll::CPlayer_WeaponServices::m_hActiveWeapon);
        if (!activeWeaponHandle || activeWeaponHandle == 0xFFFFFFFF) { if (bDebug) std::cout << "[ERR] activeWeaponHandle ungueltig\n"; return; }

        // EntityList
        uintptr_t entityList = Memory::Read<uintptr_t>(client + cs2_dumper::offsets::client_dll::dwEntityList);
        if (!entityList) { if (bDebug) std::cout << "[ERR] entityList ist 0\n"; return; }

        // Resolve Handle zu Entity
        int weaponIndex = activeWeaponHandle & 0x7FFF;
        uintptr_t listEntry = Memory::Read<uintptr_t>(entityList + 0x8 * (weaponIndex >> 9) + 0x10);
        if (!listEntry) { if (bDebug) std::cout << "[ERR] listEntry ist 0 fuer Index " << weaponIndex << "\n"; return; }

        uintptr_t activeWeapon = Memory::Read<uintptr_t>(listEntry + 112 * (weaponIndex & 0x1FF));
        
        bool isKnife = false;
        short weaponID = 0;

        if (!activeWeapon) { 
            if (bDebug) {
                std::cout << "[WARN] activeWeapon Pointer ist 0! Handle: 0x" << std::hex << activeWeaponHandle << " Index: " << std::dec << weaponIndex << "\n"; 
                std::cout << "[WARN] Nehme an, dass es sich um das Default-Messer handelt.\n";
            }
            // Da USP etc. funktionieren, nehmen wir an, dass Pointer 0 das Default Messer ist!
            isKnife = true;
            weaponID = 42; // Fallback CT Messer ID
        } else {
            // Lese die aktuelle Item Definition ID der Waffe
            weaponID = Memory::Read<short>(activeWeapon + cs2_dumper::schemas::client_dll::C_EconEntity::m_AttributeManager + 
                                           cs2_dumper::schemas::client_dll::C_AttributeContainer::m_Item + 
                                           cs2_dumper::schemas::client_dll::C_EconItemView::m_iItemDefinitionIndex);
            
            isKnife = (weaponID == 42 || weaponID == 59 || weaponID >= 500);
            
            if (bDebug) {
                std::cout << "[OK] Waffe aktiv. ID: " << weaponID << "\n";
            }
        }

        static uint32_t s_lastWeaponHandle = 0;
        static int s_forceUpdateFrames = 0;

        if (activeWeaponHandle != s_lastWeaponHandle) {
            s_lastWeaponHandle = activeWeaponHandle;
            s_forceUpdateFrames = 5; // 5 Frames lang ein Update erzwingen
        }

        if (isKnife && g_DesiredKnifeID != 0) {
            if (weaponID != g_DesiredKnifeID || s_forceUpdateFrames > 0) {
                if (s_forceUpdateFrames > 0) s_forceUpdateFrames--;

                if (activeWeapon) {
                    // Messer ersetzen (Definition Index)
                    Memory::Write<short>(activeWeapon + cs2_dumper::schemas::client_dll::C_EconEntity::m_AttributeManager + 
                                          cs2_dumper::schemas::client_dll::C_AttributeContainer::m_Item + 
                                          cs2_dumper::schemas::client_dll::C_EconItemView::m_iItemDefinitionIndex, g_DesiredKnifeID);
                    
                    Memory::Write<int>(activeWeapon + cs2_dumper::schemas::client_dll::C_EconEntity::m_OriginalOwnerXuidLow, 0);
                    Memory::Write<int>(activeWeapon + cs2_dumper::schemas::client_dll::C_EconEntity::m_OriginalOwnerXuidHigh, 0);
                    Memory::Write<int>(activeWeapon + cs2_dumper::schemas::client_dll::C_EconEntity::m_nFallbackPaintKit, 0);
                    Memory::Write<int>(activeWeapon + cs2_dumper::schemas::client_dll::C_EconEntity::m_nFallbackStatTrak, -1);
                    Memory::Write<int>(activeWeapon + cs2_dumper::schemas::client_dll::C_EconEntity::m_nFallbackSeed, 0);
                    Memory::Write<float>(activeWeapon + cs2_dumper::schemas::client_dll::C_EconEntity::m_flFallbackWear, 0.001f);
                    
                    Memory::Write<int>(activeWeapon + cs2_dumper::schemas::client_dll::C_EconEntity::m_AttributeManager + 
                                        cs2_dumper::schemas::client_dll::C_AttributeContainer::m_Item + 
                                        cs2_dumper::schemas::client_dll::C_EconItemView::m_iItemIDHigh, -1);

                    if (ModFramework::Hooks::oSetModel) {
                        void* pLocalViewModel = nullptr;
                        for (int chunk_idx = 0; chunk_idx < 64; ++chunk_idx) {
                            uintptr_t chunk = Memory::Read<uintptr_t>(entityList + 0x8 * chunk_idx + 0x10);
                            if (!chunk) continue;
                            for (int ent_idx = 0; ent_idx < 512; ++ent_idx) {
                                uintptr_t ent = Memory::Read<uintptr_t>(chunk + 0x70 * ent_idx);
                                if (!ent) continue;
                                uintptr_t identity = Memory::Read<uintptr_t>(ent + 0x10);
                                if (!identity) continue;
                                uintptr_t designer_name_ptr = Memory::Read<uintptr_t>(identity + 0x20);
                                if (designer_name_ptr && strcmp((const char*)designer_name_ptr, "predicted_viewmodel") == 0) {
                                    uint32_t owner = Memory::Read<uint32_t>(ent + cs2_dumper::schemas::client_dll::C_BaseEntity::m_hOwnerEntity);
                                    if (owner != 0xFFFFFFFF && owner != 0) {
                                        uintptr_t owner_chunk = Memory::Read<uintptr_t>(entityList + 0x8 * ((owner & 0x7FFF) >> 9) + 0x10);
                                        if (owner_chunk) {
                                            uintptr_t owner_ent = Memory::Read<uintptr_t>(owner_chunk + 0x70 * (owner & 0x1FF));
                                            if (owner_ent == localPlayerPawn) {
                                                pLocalViewModel = (void*)ent;
                                                break;
                                            }
                                        }
                                    }
                                }
                            }
                            if (pLocalViewModel) break;
                        }
                        
                        if (pLocalViewModel) {
                            ModFramework::Hooks::oSetModel(pLocalViewModel, GetKnifeModel(g_DesiredKnifeID));
                        }

                        // CS2 Knife Fix: Spoof m_nSubclassID and call UpdateSubclass
                        uint32_t currentSubclass = Memory::Read<uint32_t>(activeWeapon + 0x380); // C_BasePlayerWeapon::m_nSubclassID
                        if (currentSubclass != (uint32_t)g_DesiredKnifeID) {
                            Memory::Write<uint32_t>(activeWeapon + 0x380, g_DesiredKnifeID);

                            static uintptr_t updateSubclassAddr = 0;
                            if (!updateSubclassAddr) {
                                // Use the correct prologue signature, NOT the user's sig which points to the middle of the function!
                                updateSubclassAddr = Memory::PatternScanner::FindPattern("client.dll", "4C 8B DC 53 48 81 EC ? ? ? ? 48 8B 41");
                            }

                            if (updateSubclassAddr) {
                                typedef void(__fastcall* UpdateSubclassFn)(void*);
                                auto updateSubclass = reinterpret_cast<UpdateSubclassFn>(updateSubclassAddr);
                                updateSubclass(reinterpret_cast<void*>(activeWeapon));
                            }
                        }

                        // Force Mesh Update (SetMeshGroupMask)
                        static uintptr_t setMeshGroupMaskAddr = 0;
                        if (!setMeshGroupMaskAddr) {
                            setMeshGroupMaskAddr = Memory::PatternScanner::FindPattern("client.dll", "48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC ? 48 8D 99 ? ? ? ? 48 8B 71");
                        }
                        if (setMeshGroupMaskAddr) {
                            typedef void(__fastcall* SetMeshGroupMaskFn)(void*, uint64_t);
                            auto setMeshGroupMask = reinterpret_cast<SetMeshGroupMaskFn>(setMeshGroupMaskAddr);
                            
                            // Waffe aktualisieren
                            uintptr_t weaponSceneNode = Memory::Read<uintptr_t>(activeWeapon + cs2_dumper::schemas::client_dll::C_BaseEntity::m_pGameSceneNode);
                            if (weaponSceneNode) {
                                setMeshGroupMask((void*)weaponSceneNode, 2);
                            }
                            
                            // Viewmodel aktualisieren
                            if (pLocalViewModel) {
                                uintptr_t vmSceneNode = Memory::Read<uintptr_t>((uintptr_t)pLocalViewModel + cs2_dumper::schemas::client_dll::C_BaseEntity::m_pGameSceneNode);
                                if (vmSceneNode) {
                                    setMeshGroupMask((void*)vmSceneNode, 2);
                                }
                            }
                        }
                    }
                }
                                
                if (bDebug) {
                    std::cout << "[SKINCHANGER] Messer erfolgreich ueberschrieben! (Wechsle Waffe zum Aktualisieren)\n";
                }
            } // End of if (weaponID != g_DesiredKnifeID || s_forceUpdateFrames > 0)
        }
        else if (g_CurrentConfig.targetWeaponID != 0 && weaponID == g_CurrentConfig.targetWeaponID) {
            // Normalen Waffenskin anwenden
            Memory::Write<int>(activeWeapon + cs2_dumper::schemas::client_dll::C_EconEntity::m_nFallbackPaintKit, g_CurrentConfig.paintKit);
            Memory::Write<int>(activeWeapon + cs2_dumper::schemas::client_dll::C_EconEntity::m_nFallbackSeed, g_CurrentConfig.seed);
            Memory::Write<float>(activeWeapon + cs2_dumper::schemas::client_dll::C_EconEntity::m_flFallbackWear, g_CurrentConfig.wear);
            Memory::Write<int>(activeWeapon + cs2_dumper::schemas::client_dll::C_EconEntity::m_nFallbackStatTrak, g_CurrentConfig.statTrak);
            
            Memory::Write<int>(activeWeapon + cs2_dumper::schemas::client_dll::C_EconEntity::m_AttributeManager + 
                                cs2_dumper::schemas::client_dll::C_AttributeContainer::m_Item + 
                                cs2_dumper::schemas::client_dll::C_EconItemView::m_iItemIDHigh, -1);
        }
    }
}
