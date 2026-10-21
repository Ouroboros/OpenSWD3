#pragma once

#include <array>
#include <string>
#include <unordered_map>
#include <vector>

#include "core/types.h"

namespace swd3
{

// The rebuild's hint of hidden items: whether the script of an event gives an item (command 0x80) somewhere between
// its start and the next script of its file, a script it opens (0xA1) read with it. The Talk files are read apart
// from the running script, once, and the answer is kept per event.
class ItemEvents
{
public:
    bool GivesItem(const std::u8string& directory, u32 event);

private:
    static constexpr std::size_t Files = 4;  // Talk1.dat..Talk4.dat
    static constexpr int OpenDepth = 3;      // scripts opened from scripts that are read

    bool Scan(const std::u8string& directory, u32 event, int depth);
    const std::vector<u8>& Script(const std::u8string& directory, std::size_t file);

    std::array<std::vector<u8>, Files> files;
    std::array<bool, Files> loaded = {};
    std::unordered_map<u32, bool> events;
};

}  // namespace swd3
