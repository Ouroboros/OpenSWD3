#include <cstdio>
#include <cstring>
#include <span>
#include <string>
#include <vector>

#include "codec/lzo.h"
#include "core/types.h"
#include "game_data.h"
#include "test.h"

using swd3::u16;
using swd3::u32;
using swd3::u8;

using swd3::LzoDecoder;
using swd3::LzoStatus;

namespace
{

u32 ReadU32(const std::vector<u8>& data, std::size_t offset)
{
    u32 value = 0;
    std::memcpy(&value, data.data() + offset, 4);
    return value;
}

}  // namespace

TEST_CASE("lzo: literal run followed by the end marker")
{
    const std::vector<u8> input = {17 + 5, 'h', 'e', 'l', 'l', 'o', 0x11, 0x00, 0x00};
    std::vector<u8> output(16, 0);
    LzoDecoder decoder(input, output);
    CHECK(decoder.Run() == LzoStatus::Ok);
    CHECK_EQ(decoder.Written(), 5u);
    CHECK(std::memcmp(output.data(), "hello", 5) == 0);
}

TEST_CASE("lzo: overlapping match repeats the bytes just written")
{
    const std::vector<u8> input = {17 + 3, 'a', 'b', 'c', 0xA8, 0x00, 0x11, 0x00, 0x00};
    std::vector<u8> output(16, 0);
    LzoDecoder decoder(input, output);
    CHECK(decoder.Run() == LzoStatus::Ok);
    CHECK_EQ(decoder.Written(), 9u);
    CHECK(std::memcmp(output.data(), "abcabcabc", 9) == 0);
}

TEST_CASE("lzo: trailing input and truncated input")
{
    const std::vector<u8> extra = {17 + 1, 'x', 0x11, 0x00, 0x00, 0x00};
    std::vector<u8> output(4, 0);
    CHECK(LzoDecoder(extra, output).Run() == LzoStatus::InputNotConsumed);
    const std::vector<u8> truncated = {17 + 4, 'a', 'b'};
    CHECK(LzoDecoder(truncated, output).Run() == LzoStatus::InputOverrun);
    const std::vector<u8> tooLong = {17 + 8, '1', '2', '3', '4', '5', '6', '7', '8', 0x11, 0, 0};
    CHECK(LzoDecoder(tooLong, output).Run() == LzoStatus::OutputOverrun);
}

TEST_CASE("lzo: first frame of all_sys.tsw entry 1")
{
    const std::vector<u8> file = swd3test::ReadGameFile("all_sys.tsw");

    if (file.empty())
    {
        std::printf("  (game data not found, skipped)\n");
        return;
    }

    const u32 entry = ReadU32(file, 0x1C + 0x18);
    u16 format = 0;
    std::memcpy(&format, file.data() + entry + 8, 2);
    const std::size_t frames = entry + 12 + (format == 8 ? 0x200 : 0);
    const u32 offset = ReadU32(file, frames);
    const u32 packed = ReadU32(file, frames + 4);
    const u32 size = ReadU32(file, frames + 8);
    std::vector<u8> output(size, 0);
    LzoDecoder decoder({file.data() + entry + offset, packed}, output);
    CHECK(decoder.Run() == LzoStatus::Ok);
    CHECK_EQ(decoder.Written(), size);
    u16 magic = 0;
    std::memcpy(&magic, output.data(), 2);
    CHECK_EQ(magic, 0xFFFFu);
}

namespace
{

swd3::u32 Dword(const std::vector<swd3::u8>& data, std::size_t offset)
{
    swd3::u32 value = 0;
    std::memcpy(&value, data.data() + offset, 4);
    return value;
}

std::vector<swd3::u8> Decode(std::span<const swd3::u8> packed, std::size_t size)
{
    std::vector<swd3::u8> output(size);
    swd3::LzoDecoder decoder(packed, output);
    CHECK(decoder.Run() == swd3::LzoStatus::Ok);
    CHECK_EQ(decoder.Written(), size);
    return output;
}

}  // namespace

TEST_CASE("lzo encoder: round trips of both dictionary sizes")
{
    for (const int bits : {14, 15})
    {
        swd3::LzoEncoder encoder(bits);

        for (const std::size_t size : {0u, 1u, 3u, 13u, 14u, 200u, 239u, 300u, 5000u, 70000u})
        {
            std::vector<swd3::u8> input(size);
            swd3::u32 state = static_cast<swd3::u32>(size) * 2654435761u + 1;

            for (std::size_t i = 0; i < size; ++i)
            {
                state = state * 1103515245u + 12345u;
                input[i] = static_cast<swd3::u8>(i % 7 < 3 ? (state >> 24) : (i / 11));
            }

            const std::vector<swd3::u8> packed = encoder.Compress(input);
            CHECK(Decode(packed, size) == input);
        }
    }
}

TEST_CASE("lzo encoder: recompressing the blocks of original saves gives the same bytes")
{
    int blocks = 0;

    for (const std::string name : {"Save/0.sav", "Save/1.sav"})
    {
        const std::vector<swd3::u8> save = swd3test::ReadGameFile(name);

        if (save.size() < 0x9700)
        {
            continue;
        }

        std::size_t at = 0x962C;
        const swd3::u32 flagsPacked = Dword(save, at);
        at += 8;
        const std::span<const swd3::u8> flagsBlock(save.data() + at, flagsPacked);
        const std::vector<swd3::u8> flags = Decode(flagsBlock, 0x400);
        CHECK(swd3::LzoEncoder(14).Compress(flags) == std::vector<swd3::u8>(flagsBlock.begin(), flagsBlock.end()));
        at += flagsPacked;
        const swd3::u32 tablePacked = Dword(save, at);
        const swd3::u32 tableSize = Dword(save, at + 4);
        at += 8;
        const std::span<const swd3::u8> tableBlock(save.data() + at, tablePacked);
        const std::vector<swd3::u8> table = Decode(tableBlock, tableSize);
        CHECK(swd3::LzoEncoder(14).Compress(table) == std::vector<swd3::u8>(tableBlock.begin(), tableBlock.end()));
        at += tablePacked + 7 * 4;
        const swd3::u32 dataPacked = Dword(save, at);
        const swd3::u32 dataSize = Dword(save, at + 4);
        at += 8;
        const std::span<const swd3::u8> dataBlock(save.data() + at, dataPacked);
        const std::vector<swd3::u8> data = Decode(dataBlock, dataSize);
        CHECK(swd3::LzoEncoder(14).Compress(data) == std::vector<swd3::u8>(dataBlock.begin(), dataBlock.end()));
        at += dataPacked + 0x40 + 4 + 0x40 + 0x180;
        const swd3::u32 fameLength = Dword(save, at);
        at += 4;
        CHECK_EQ(save[at], 1);
        const swd3::u32 fameSize = Dword(save, at + 2);
        const swd3::u32 famePacked = Dword(save, at + 6);
        CHECK_EQ(fameLength, famePacked + 0xA);
        const std::span<const swd3::u8> fameBlock(save.data() + at + 0xA, famePacked);
        const std::vector<swd3::u8> fame = Decode(fameBlock, fameSize);
        CHECK(swd3::LzoEncoder(15).Compress(fame) == std::vector<swd3::u8>(fameBlock.begin(), fameBlock.end()));
        blocks += 4;
    }

    std::printf("  blocks: %d\n", blocks);
}
