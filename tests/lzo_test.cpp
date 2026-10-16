#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "codec/lzo.h"
#include "game_data.h"
#include "test.h"

using swd3::LzoDecoder;
using swd3::LzoStatus;

namespace
{

std::uint32_t ReadU32(const std::vector<std::uint8_t>& data, std::size_t offset)
{
    std::uint32_t value = 0;
    std::memcpy(&value, data.data() + offset, 4);
    return value;
}

}  // namespace

TEST_CASE("lzo: literal run followed by the end marker")
{
    const std::vector<std::uint8_t> input = {17 + 5, 'h', 'e', 'l', 'l', 'o', 0x11, 0x00, 0x00};
    std::vector<std::uint8_t> output(16, 0);
    LzoDecoder decoder(input, output);
    CHECK(decoder.Run() == LzoStatus::Ok);
    CHECK_EQ(decoder.Written(), 5u);
    CHECK(std::memcmp(output.data(), "hello", 5) == 0);
}

TEST_CASE("lzo: overlapping match repeats the bytes just written")
{
    const std::vector<std::uint8_t> input = {17 + 3, 'a', 'b', 'c', 0xA8, 0x00, 0x11, 0x00, 0x00};
    std::vector<std::uint8_t> output(16, 0);
    LzoDecoder decoder(input, output);
    CHECK(decoder.Run() == LzoStatus::Ok);
    CHECK_EQ(decoder.Written(), 9u);
    CHECK(std::memcmp(output.data(), "abcabcabc", 9) == 0);
}

TEST_CASE("lzo: trailing input and truncated input")
{
    const std::vector<std::uint8_t> extra = {17 + 1, 'x', 0x11, 0x00, 0x00, 0x00};
    std::vector<std::uint8_t> output(4, 0);
    CHECK(LzoDecoder(extra, output).Run() == LzoStatus::InputNotConsumed);
    const std::vector<std::uint8_t> truncated = {17 + 4, 'a', 'b'};
    CHECK(LzoDecoder(truncated, output).Run() == LzoStatus::InputOverrun);
    const std::vector<std::uint8_t> tooLong = {17 + 8, '1', '2', '3', '4', '5', '6', '7', '8', 0x11, 0, 0};
    CHECK(LzoDecoder(tooLong, output).Run() == LzoStatus::OutputOverrun);
}

TEST_CASE("lzo: first frame of all_sys.tsw entry 1")
{
    const std::vector<std::uint8_t> file = swd3test::ReadGameFile("all_sys.tsw");

    if (file.empty())
    {
        std::printf("  (game data not found, skipped)\n");
        return;
    }

    const std::uint32_t entry = ReadU32(file, 0x1C + 0x18);
    std::uint16_t format = 0;
    std::memcpy(&format, file.data() + entry + 8, 2);
    const std::size_t frames = entry + 12 + (format == 8 ? 0x200 : 0);
    const std::uint32_t offset = ReadU32(file, frames);
    const std::uint32_t packed = ReadU32(file, frames + 4);
    const std::uint32_t size = ReadU32(file, frames + 8);
    std::vector<std::uint8_t> output(size, 0);
    LzoDecoder decoder({file.data() + entry + offset, packed}, output);
    CHECK(decoder.Run() == LzoStatus::Ok);
    CHECK_EQ(decoder.Written(), size);
    std::uint16_t magic = 0;
    std::memcpy(&magic, output.data(), 2);
    CHECK_EQ(magic, 0xFFFFu);
}
