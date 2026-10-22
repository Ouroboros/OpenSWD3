#pragma once

#include <array>
#include <functional>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

#include "core/types.h"
#include "talk/event_command.h"

namespace swd3
{

// The rebuild's reading of event scripts apart from the running one: the commands of an event's script between its
// start and the next script of its file, a script it opens (0xA1) read with it. The Talk files are read once and the
// answers are kept per event.
class EventScan
{
public:
    // The hint of hidden items: whether the script gives an item (command 0x80).
    bool GivesItem(const std::u8string& directory, u32 event);
    // The map overview: the maps the script changes to (command 0x1B), each once, in the order read.
    const std::vector<u16>& EnteredMaps(const std::u8string& directory, u32 event);

private:
    static constexpr std::size_t Files = 4;  // Talk1.dat..Talk4.dat
    static constexpr int OpenDepth = 3;      // scripts opened from scripts that are read

    // Each command read, with its bytes from its opcode on; reading stops once `found` returns true (true then).
    using Found = std::function<bool(EventCommand code, std::span<const u8> command)>;

    bool Scan(const std::u8string& directory, u32 event, int depth, const Found& found);
    const std::vector<u8>& Script(const std::u8string& directory, std::size_t file);

    std::array<std::vector<u8>, Files> files;
    std::array<bool, Files> loaded = {};
    std::unordered_map<u32, bool> items;
    std::unordered_map<u32, std::vector<u16>> maps;
};

}  // namespace swd3
