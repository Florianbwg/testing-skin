#pragma once
#include <windows.h>
#include <cstdint>
#include <string>

namespace ModFramework::Memory {
    
    // Holt die Basisadresse eines Moduls (z.B. "client.dll") im eigenen Prozess
    inline uintptr_t GetModuleBase(const std::string& moduleName) {
        HMODULE hModule = GetModuleHandleA(moduleName.c_str());
        return reinterpret_cast<uintptr_t>(hModule);
    }

    // Liest einen Wert aus dem Speicher
    template <typename T>
    inline T Read(uintptr_t address) {
        if (!address) return T{};
        return *reinterpret_cast<T*>(address);
    }

    // Schreibt einen Wert in den Speicher
    template <typename T>
    inline void Write(uintptr_t address, const T& value) {
        if (!address) return;
        *reinterpret_cast<T*>(address) = value;
    }

    // Holt ein Interface aus einer Engine-DLL
    template <typename T>
    inline T* GetInterface(const char* moduleName, const char* interfaceName) {
        HMODULE hModule = GetModuleHandleA(moduleName);
        if (!hModule) return nullptr;

        using CreateInterfaceFn = void* (*)(const char*, int*);
        CreateInterfaceFn createInterface = reinterpret_cast<CreateInterfaceFn>(GetProcAddress(hModule, "CreateInterface"));
        if (!createInterface) return nullptr;

        return reinterpret_cast<T*>(createInterface(interfaceName, nullptr));
    }
}
