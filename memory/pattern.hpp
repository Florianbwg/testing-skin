#pragma once
#include <windows.h>
#include <string>
#include <vector>

namespace ModFramework::Memory {

    class PatternScanner {
    public:
        // Sucht ein Pattern in einem bestimmten Modul
        // Pattern Format: "E8 ? ? ? ? F3 0F 10 4C 3B ?"
        static uintptr_t FindPattern(const std::string& moduleName, const std::string& pattern);
        
        // Liest eine relative Adresse aus (z.B. bei CALL E8 oder JMP E9) und berechnet die absolute Adresse
        static uintptr_t GetAbsoluteAddress(uintptr_t instruction, int offset, int size);
    };

}
