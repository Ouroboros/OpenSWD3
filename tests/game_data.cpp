#include "game_data.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iterator>

#include "golden/draw_scenarios.h"
#include "platform/file_system.h"
#include "text/big5.h"

namespace swd3test
{

std::string GameDirectory()
{
    const std::string configured = golden::Environment("SWD3_GAME_DIR");
#if defined(_WIN32)
    const std::string directory = !configured.empty() ? configured : "E:/Game/swd3";
#else
    const std::string directory = !configured.empty() ? configured : "/mnt/e/Game/swd3";
#endif
    return directory + "/";
}

std::string GameFile(const std::string& name)
{
    return swd3::ResolvePath(GameDirectory() + name);
}

std::vector<u8> ReadGameFile(const std::string& name)
{
    std::ifstream stream(GameFile(name), std::ios::binary);

    if (!stream)
    {
        return {};
    }

    return {std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
}

bool SameText(const std::vector<u8>& exe, swd3::u32 address, std::string_view text)
{
    const std::size_t at = address - kImageBase;

    if (address < kImageBase || at + text.size() >= exe.size())
    {
        return false;
    }

    return std::equal(
               text.begin(),
               text.end(),
               exe.begin() + static_cast<std::ptrdiff_t>(at),
               [](char left, u8 right)
               {
                   return static_cast<u8>(left) == right;
               }
           ) &&
           exe[at + text.size()] == 0;
}

bool SameText(const std::vector<u8>& exe, swd3::u32 address, std::u8string_view text)
{
    return SameText(exe, address, swd3::Big5(text));
}

bool SameBytes(const std::vector<u8>& exe, swd3::u32 address, std::span<const std::byte> bytes)
{
    const std::size_t at = address - kImageBase;

    if (address < kImageBase || at + bytes.size() > exe.size())
    {
        return false;
    }

    return std::memcmp(exe.data() + at, bytes.data(), bytes.size()) == 0;
}

swd3::u32 ExeDword(const std::vector<u8>& exe, swd3::u32 address)
{
    swd3::u32 value = 0;
    const std::size_t at = address - kImageBase;

    if (address >= kImageBase && at + sizeof(value) <= exe.size())
    {
        std::memcpy(&value, exe.data() + at, sizeof(value));
    }

    return value;
}
}  // namespace swd3test
