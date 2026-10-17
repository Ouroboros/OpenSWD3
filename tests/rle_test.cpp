#include <array>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <functional>
#include <map>
#include <string>
#include <vector>

#include "codec/lzo.h"
#include "core/format.h"
#include "core/types.h"
#include "draw/painter.h"
#include "game_data.h"
#include "golden/rle_scenarios.h"
#include "golden_check.h"
#include "pic/rle.h"
#include "platform/display.h"
#include "platform/surface.h"
#include "test.h"

using swd3::u16;
using swd3::u32;
using swd3::u64;
using swd3::u8;

namespace
{

class CodecBackend : public golden::RleBackend
{
public:
    explicit CodecBackend(const swd3::RleCodec& codec)
        : codec(codec)
    {
    }

    std::vector<u8> Encode(const void* pixels, int width, int height, int format) override
    {
        return this->codec.Encode(pixels, width, height, format);
    }

    std::vector<u8> Decode(const std::vector<u8>& rle) override
    {
        const auto picture = this->codec.Decode(rle);

        if (!picture)
        {
            return {};
        }

        std::vector<u8> out = picture->pixels;
        golden::Append32(out, static_cast<u32>(picture->width));
        golden::Append32(out, static_cast<u32>(picture->height));
        golden::Append32(out, static_cast<u32>(picture->format));
        return out;
    }

    std::vector<u8> ConvertToDisplay(const std::vector<u8>& rle) override
    {
        std::vector<u8> copy = rle;
        swd3::RleHeader header;
        const bool result = this->codec.ConvertToDisplay(copy, header);
        golden::Append32(copy, result ? 1 : 0);
        golden::Append32(copy, header.width);
        golden::Append32(copy, header.height);
        golden::Append32(copy, header.format);
        return copy;
    }

    std::vector<u8>

    ConvertTo16(const std::vector<u8>& rle, const std::vector<u16>& palette) override
    {
        swd3::Picture picture;
        picture.data = rle;
        picture.palette = palette;
        this->codec.ConvertTo16(picture);
        std::vector<u8> out = picture.data;
        golden::Append32(out, picture.width);
        golden::Append32(out, picture.height);
        return out;
    }

    std::vector<u8> ConvertTileTo16(const std::vector<u8>& tile) override
    {
        std::vector<u8> copy = tile;
        this->codec.ConvertTileTo16(copy);
        return copy;
    }

private:
    const swd3::RleCodec& codec;
};

// A line of two fill pixels (the default fill colour 0x26B), two literals, a skip pixel (0x319F) and a literal, and
// its RLE: the header, the line header with its byte length, the runs and literal runs, the line and picture ends.
constexpr std::array<u16, 6> kLinePixels = {0x26B, 0x26B, 0x1111, 0x2222, 0x319F, 0x3333};
constexpr std::array<u16, 14> kEncodedLine =
    {0xFFFF, 6, 1, 0x10, 18, 0xC002, 0x0002, 0x1111, 0x2222, 0x8001, 0x0001, 0x3333, 0x0000, 0x0000};

}  // namespace

TEST_CASE("rle: encode and decode a 16-bit picture")
{
    swd3::Surface screen(swd3::Display::Width, swd3::Display::Height);
    swd3::Painter painter;
    painter.Init(screen);
    const swd3::RleCodec codec(painter);
    const std::vector<u8> rle =
        codec.Encode(kLinePixels.data(), static_cast<int>(kLinePixels.size()), 1, swd3::RleHeader::TrueColor);
    CHECK_EQ(rle.size(), sizeof(kEncodedLine));
    CHECK(std::memcmp(rle.data(), kEncodedLine.data(), sizeof(kEncodedLine)) == 0);
    const auto decoded = codec.Decode(rle);
    CHECK(decoded.has_value());
    CHECK(std::memcmp(decoded->pixels.data(), kLinePixels.data(), sizeof(kLinePixels)) == 0);
}

TEST_CASE("rle: matches the golden hashes of the legacy codec")
{
    const std::map<std::string, std::string> golden =
        swd3test::LoadGolden(SWD3_SOURCE_DIR "/tests/golden/rle_golden.txt");

    auto decompress = [](const u8* input, std::size_t size, std::vector<u8>& out)
    {
        swd3::LzoDecoder decoder({input, size}, out);
        return decoder.Run() == swd3::LzoStatus::Ok && decoder.Written() == out.size();
    };

    auto gameFile = [](const std::string& name)
    {
        return std::string(swd3::AsBytes(swd3test::GameFile(name)));
    };

    const std::vector<golden::Frame> frames = golden::LoadGoldenFrames(gameFile, decompress);
    const std::vector<golden::PaletteFrame> palettes = golden::LoadGoldenPaletteFrames(gameFile, decompress);

    if (frames.empty() || golden.empty())
    {
        std::printf("  (game data or golden file not found, skipped)\n");
        return;
    }

    swd3::Surface screen(swd3::Display::Width, swd3::Display::Height);
    swd3::Painter painter;
    painter.Init(screen);
    const swd3::RleCodec codec(painter);
    CodecBackend backend(codec);
    swd3test::GoldenCheck check(golden);
    golden::RunRle(backend, frames, palettes, std::ref(check));
    std::printf("  compared %d scenarios\n", check.compared);
    CHECK_EQ(check.mismatches, 0);
    CHECK_EQ(check.compared, static_cast<int>(golden.size()));
}
