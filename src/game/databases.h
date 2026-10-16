#pragma once

#include <string>
#include <vector>

#include "core/types.h"
#include "file/file.h"

namespace swd3
{

class ErrorReporter;

// The data files opened at startup: maps.dat (the roles, items and starting state, read from 0x200), path.dat (mapped
// whole) and talk1.dat (the story).
class GameDatabases
{
public:
    static constexpr u32 kMapsHeader = 0x200;
    static constexpr u32 kScratchBytes = 0x8000;

    bool Open(const std::string& installDirectory, ErrorReporter& errors);
    void LoadMaps();

    File maps;                     // 0x004B74A0
    File paths;                    // 0x004B9F18
    const u8* pathData = nullptr;  // 0x004ACB94 the mapped path.dat
    File story;                    // 0x004A98C8
    std::vector<u8> mapsData;      // 0x004C9A10 maps.dat from offset 0x200
    std::vector<u8> scratch;       // 0x004B8860 0x8000 bytes
};

}  // namespace swd3
