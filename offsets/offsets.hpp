#pragma once
#include <string>
#include <cstdint>
#include <unordered_map>

namespace ModFramework::Offsets {

    class Manager {
    public:
        // Lädt die Dumper-JSON-Dateien
        static bool Load(const std::string& offsetsPath, const std::string& clientPath);
        
        // Holt einen Offset via Schlüsselnamen
        static uintptr_t GetOffset(const std::string& name);
        static uintptr_t GetClientOffset(const std::string& name);

    private:
        static std::unordered_map<std::string, uintptr_t> m_Offsets;
        static std::unordered_map<std::string, uintptr_t> m_ClientOffsets;
        
        static bool ParseJson(const std::string& filepath, std::unordered_map<std::string, uintptr_t>& mapOut);
    };

}
