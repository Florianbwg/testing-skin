#include "pattern.hpp"
#include <Psapi.h>
#include <sstream>

namespace ModFramework::Memory {

    uintptr_t PatternScanner::FindPattern(const std::string& moduleName, const std::string& pattern) {
        HMODULE hModule = GetModuleHandleA(moduleName.c_str());
        if (!hModule) return 0;

        MODULEINFO moduleInfo;
        GetModuleInformation(GetCurrentProcess(), hModule, &moduleInfo, sizeof(moduleInfo));

        uint8_t* moduleBase = reinterpret_cast<uint8_t*>(moduleInfo.lpBaseOfDll);
        size_t moduleSize = moduleInfo.SizeOfImage;

        std::vector<int> patternBytes;
        std::istringstream iss(pattern);
        std::string hexByte;
        while (iss >> hexByte) {
            if (hexByte == "?" || hexByte == "??") {
                patternBytes.push_back(-1);
            } else {
                patternBytes.push_back(std::stoi(hexByte, nullptr, 16));
            }
        }

        for (size_t i = 0; i < moduleSize - patternBytes.size(); ++i) {
            bool found = true;
            for (size_t j = 0; j < patternBytes.size(); ++j) {
                if (patternBytes[j] != -1 && moduleBase[i + j] != patternBytes[j]) {
                    found = false;
                    break;
                }
            }
            if (found) {
                return reinterpret_cast<uintptr_t>(&moduleBase[i]);
            }
        }

        return 0;
    }

    uintptr_t PatternScanner::GetAbsoluteAddress(uintptr_t instruction, int offset, int size) {
        if (!instruction) return 0;
        int32_t relativeOffset = *reinterpret_cast<int32_t*>(instruction + offset);
        return instruction + size + relativeOffset;
    }

}
