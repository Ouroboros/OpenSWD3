#include <array>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "codec/lzo.h"
#include "core/byte_reader.h"
#include "core/types.h"
#include "game/game_state.h"
#include "game_data.h"
#include "res/tsw_archive.h"
#include "test.h"

using swd3::ByteReader;
using swd3::u16;
using swd3::u32;
using swd3::u8;

using swd3::LzoDecoder;
using swd3::LzoStatus;

namespace
{

// Hand-made LZO1X streams: a first byte above kFirstLiteralBias starts with that many literals less it, and
// kEndMarker followed by two zero bytes ends the stream.
constexpr u8 kFirstLiteralBias = 17;
constexpr u8 kEndMarker = 0x11;
constexpr std::string_view kHello = "hello";
constexpr std::array<u8, 9> kHelloStream =
    {kFirstLiteralBias + kHello.size(), 'h', 'e', 'l', 'l', 'o', kEndMarker, 0, 0};
constexpr std::string_view kRepeated = "abcabcabc";
// "abc", then an M2 match of the six bytes from three back.
constexpr std::array<u8, 9> kRepeatStream = {kFirstLiteralBias + 3, 'a', 'b', 'c', 0xA8, 0x00, kEndMarker, 0, 0};
constexpr std::array<u8, 6> kExtraStream = {kFirstLiteralBias + 1, 'x', kEndMarker, 0, 0, 0};  // a byte past the end
constexpr std::array<u8, 3> kTruncatedStream = {kFirstLiteralBias + 4, 'a', 'b'};              // two literals short
constexpr std::array<u8, 12> kLongStream =
    {kFirstLiteralBias + 8, '1', '2', '3', '4', '5', '6', '7', '8', kEndMarker, 0, 0};
constexpr std::size_t kSmallOutput = 4;  // too small for kLongStream
constexpr std::size_t kOutputBytes = 16;

// The first entry of all_sys.tsw and the RLE magic its first frame starts with.
constexpr u16 kRleMagic = 0xFFFF;

// Round trips: input sizes around the encoder's thresholds and a pseudo-random fill that repeats in places.
constexpr std::array<int, 2> kDictionaryBits = {14, 15};
constexpr std::array<std::size_t, 10> kSizes = {0, 1, 3, 13, 14, 200, 239, 300, 5000, 70000};
constexpr u32 kSeedMultiplier = 2654435761u;  // Knuth's multiplicative hash
constexpr u32 kLcgMultiplier = 1103515245u;
constexpr u32 kLcgIncrement = 12345u;
constexpr std::size_t kPattern = 7;  // of every kPattern bytes the first kRandomBytes are random
constexpr std::size_t kRandomBytes = 3;
constexpr std::size_t kRunLength = 11;  // the others count up every kRunLength bytes
constexpr int kRandomShift = 24;

// The packed blocks of a save file (Save/N.sav): the flags, a table, the data and the fame lists.
constexpr std::size_t kSaveBytes = 0x9700;
constexpr std::size_t kFlagsBlockAt = 0x962C;
constexpr std::size_t kBlockHeaderBytes = 2 * sizeof(u32);  // the packed size and the unpacked size in words
constexpr std::size_t kAfterTableBytes = 7 * sizeof(u32);
constexpr std::size_t kAfterDataBytes = 0x40 + sizeof(u32) + 0x40 + 0x180;  // the names, the play time and two blocks
constexpr int kSaveBits = 14;
constexpr int kFameBits = 15;
constexpr u8 kFameVersion = 1;
constexpr std::size_t kFameSizeAt = 2;  // after the version
constexpr std::size_t kFamePackedAt = 6;
constexpr std::size_t kFameHeaderBytes = 0xA;
constexpr int kBlocksPerSave = 4;

std::vector<u8> Decode(std::span<const u8> packed, std::size_t size)
{
    std::vector<u8> output(size);
    LzoDecoder decoder(packed, output);
    CHECK(decoder.Run() == LzoStatus::Ok);
    CHECK_EQ(decoder.Written(), size);
    return output;
}

}  // namespace

TEST_CASE("lzo: literal run followed by the end marker")
{
    std::vector<u8> output(kOutputBytes, 0);
    LzoDecoder decoder(kHelloStream, output);
    CHECK(decoder.Run() == LzoStatus::Ok);
    CHECK_EQ(decoder.Written(), kHello.size());
    CHECK(std::memcmp(output.data(), kHello.data(), kHello.size()) == 0);
}

TEST_CASE("lzo: overlapping match repeats the bytes just written")
{
    std::vector<u8> output(kOutputBytes, 0);
    LzoDecoder decoder(kRepeatStream, output);
    CHECK(decoder.Run() == LzoStatus::Ok);
    CHECK_EQ(decoder.Written(), kRepeated.size());
    CHECK(std::memcmp(output.data(), kRepeated.data(), kRepeated.size()) == 0);
}

TEST_CASE("lzo: trailing input and truncated input")
{
    std::vector<u8> output(kSmallOutput, 0);
    CHECK(LzoDecoder(kExtraStream, output).Run() == LzoStatus::InputNotConsumed);
    CHECK(LzoDecoder(kTruncatedStream, output).Run() == LzoStatus::InputOverrun);
    CHECK(LzoDecoder(kLongStream, output).Run() == LzoStatus::OutputOverrun);
}

TEST_CASE("lzo: first frame of all_sys.tsw entry 1")
{
    using swd3::TswArchive;
    const std::vector<u8> file = swd3test::ReadGameFile("all_sys.tsw");

    if (file.empty())
    {
        std::printf("  (game data not found, skipped)\n");
        return;
    }

    const u32 entry =
        ByteReader::Word32At(file, TswArchive::DirectoryStart + offsetof(swd3::TswDirectoryEntry, offset));
    const u16 format = ByteReader::Word16At(file, entry + TswArchive::FormatAt);
    const std::size_t frames = entry + TswArchive::EntryHeaderBytes +
                               (format == TswArchive::PaletteFormat ? TswArchive::PaletteColors * sizeof(u16) : 0);
    const u32 offset = ByteReader::Word32At(file, frames + offsetof(swd3::TswFrameEntry, offset));
    const u32 packed = ByteReader::Word32At(file, frames + offsetof(swd3::TswFrameEntry, packedSize));
    const u32 size = ByteReader::Word32At(file, frames + offsetof(swd3::TswFrameEntry, size));
    std::vector<u8> output(size, 0);
    LzoDecoder decoder({file.data() + entry + offset, packed}, output);
    CHECK(decoder.Run() == LzoStatus::Ok);
    CHECK_EQ(decoder.Written(), size);
    CHECK_EQ(ByteReader::Word16At(output, 0), kRleMagic);
}

TEST_CASE("lzo encoder: round trips of both dictionary sizes")
{
    for (const int bits : kDictionaryBits)
    {
        swd3::LzoEncoder encoder(bits);

        for (const std::size_t size : kSizes)
        {
            std::vector<u8> input(size);
            u32 state = static_cast<u32>(size) * kSeedMultiplier + 1;

            for (std::size_t i = 0; i < size; ++i)
            {
                state = state * kLcgMultiplier + kLcgIncrement;
                input[i] = static_cast<u8>(i % kPattern < kRandomBytes ? (state >> kRandomShift) : (i / kRunLength));
            }

            const std::vector<u8> packed = encoder.Compress(input);
            CHECK(Decode(packed, size) == input);
        }
    }
}

TEST_CASE("lzo encoder: recompressing the blocks of original saves gives the same bytes")
{
    int blocks = 0;

    for (const std::string name : {"Save/0.sav", "Save/1.sav"})
    {
        const std::vector<u8> save = swd3test::ReadGameFile(name);

        if (save.size() < kSaveBytes)
        {
            continue;
        }

        auto same = [](int bits, const std::vector<u8>& unpacked, std::span<const u8> block)
        {
            return swd3::LzoEncoder(bits).Compress(unpacked) == std::vector<u8>(block.begin(), block.end());
        };

        std::size_t at = kFlagsBlockAt;
        const u32 flagsPacked = ByteReader::Word32At(save, at);
        at += kBlockHeaderBytes;
        const std::span<const u8> flagsBlock(save.data() + at, flagsPacked);
        CHECK(same(kSaveBits, Decode(flagsBlock, swd3::GameState::FlagBytes), flagsBlock));
        at += flagsPacked;
        const u32 tablePacked = ByteReader::Word32At(save, at);
        const u32 tableSize = ByteReader::Word32At(save, at + sizeof(u32));
        at += kBlockHeaderBytes;
        const std::span<const u8> tableBlock(save.data() + at, tablePacked);
        CHECK(same(kSaveBits, Decode(tableBlock, tableSize), tableBlock));
        at += tablePacked + kAfterTableBytes;
        const u32 dataPacked = ByteReader::Word32At(save, at);
        const u32 dataSize = ByteReader::Word32At(save, at + sizeof(u32));
        at += kBlockHeaderBytes;
        const std::span<const u8> dataBlock(save.data() + at, dataPacked);
        CHECK(same(kSaveBits, Decode(dataBlock, dataSize), dataBlock));
        at += dataPacked + kAfterDataBytes;
        const u32 fameLength = ByteReader::Word32At(save, at);
        at += sizeof(u32);
        CHECK_EQ(save[at], kFameVersion);
        const u32 fameSize = ByteReader::Word32At(save, at + kFameSizeAt);
        const u32 famePacked = ByteReader::Word32At(save, at + kFamePackedAt);
        CHECK_EQ(fameLength, famePacked + kFameHeaderBytes);
        const std::span<const u8> fameBlock(save.data() + at + kFameHeaderBytes, famePacked);
        CHECK(same(kFameBits, Decode(fameBlock, fameSize), fameBlock));
        blocks += kBlocksPerSave;
    }

    std::printf("  blocks: %d\n", blocks);
}
