#include <cstdio>
#include <cstring>
#include <fstream>
#include <map>
#include <string>
#include <vector>

#include "codec/lzo.h"
#include "draw/painter.h"
#include "game_data.h"
#include "golden/rle_scenarios.h"
#include "pic/rle.h"
#include "platform/surface.h"
#include "test.h"

namespace
{

class CodecBackend : public golden::RleBackend
{
public:
    explicit CodecBackend(const swd3::RleCodec& codec) : codec(codec)
    {
    }

    std::vector<std::uint8_t> Encode(const void* pixels, int width, int height, int format) override
    {
        return this->codec.Encode(pixels, width, height, format);
    }

    std::vector<std::uint8_t> Decode(const std::vector<std::uint8_t>& rle) override
    {
        const auto picture = this->codec.Decode(rle);

        if (!picture)
        {
            return {};
        }

        std::vector<std::uint8_t> out = picture->pixels;
        golden::Append32(out, static_cast<std::uint32_t>(picture->width));
        golden::Append32(out, static_cast<std::uint32_t>(picture->height));
        golden::Append32(out, static_cast<std::uint32_t>(picture->format));
        return out;
    }

    std::vector<std::uint8_t> ConvertToDisplay(const std::vector<std::uint8_t>& rle) override
    {
        std::vector<std::uint8_t> copy = rle;
        swd3::RleHeader header;
        const bool result = this->codec.ConvertToDisplay(copy, header);
        golden::Append32(copy, result ? 1 : 0);
        golden::Append32(copy, header.width);
        golden::Append32(copy, header.height);
        golden::Append32(copy, header.format);
        return copy;
    }

    std::vector<std::uint8_t>

    ConvertTo16(const std::vector<std::uint8_t>& rle, const std::vector<std::uint16_t>& palette) override
    {
        swd3::Picture picture;
        picture.data = rle;
        picture.palette = palette;
        this->codec.ConvertTo16(picture);
        std::vector<std::uint8_t> out = picture.data;
        golden::Append32(out, picture.width);
        golden::Append32(out, picture.height);
        return out;
    }

    std::vector<std::uint8_t> ConvertTileTo16(const std::vector<std::uint8_t>& tile) override
    {
        std::vector<std::uint8_t> copy = tile;
        this->codec.ConvertTileTo16(copy);
        return copy;
    }

private:
    const swd3::RleCodec& codec;
};

std::map<std::string, std::string> LoadGolden(const std::string& path)
{
    std::map<std::string, std::string> entries;
    std::ifstream file(path);
    std::string line;

    while (std::getline(file, line))
    {
        const std::size_t space = line.rfind(' ');

        if (space != std::string::npos)
        {
            entries[line.substr(0, space)] = line.substr(space + 1);
        }
    }

    return entries;
}

}  // namespace

TEST_CASE("rle: encode and decode a 16-bit picture")
{
    swd3::Surface screen(640, 480);
    swd3::Painter painter;
    painter.Init(screen);
    const swd3::RleCodec codec(painter);
    const std::uint16_t pixels[] = {0x26B, 0x26B, 0x1111, 0x2222, 0x319F, 0x3333};
    const std::vector<std::uint8_t> rle = codec.Encode(pixels, 6, 1, 0x10);
    const std::uint16_t expected[] =
        {0xFFFF, 6, 1, 0x10, 18, 0xC002, 0x0002, 0x1111, 0x2222, 0x8001, 0x0001, 0x3333, 0x0000, 0x0000};
    CHECK_EQ(rle.size(), sizeof(expected));
    CHECK(std::memcmp(rle.data(), expected, sizeof(expected)) == 0);
    const auto decoded = codec.Decode(rle);
    CHECK(decoded.has_value());
    CHECK(std::memcmp(decoded->pixels.data(), pixels, sizeof(pixels)) == 0);
}

TEST_CASE("rle: matches the golden hashes of the legacy codec")
{
    const std::map<std::string, std::string> golden = LoadGolden(SWD3_SOURCE_DIR "/tests/golden/rle_golden.txt");

    auto decompress = [](const std::uint8_t* input, std::size_t size, std::vector<std::uint8_t>& out)
    {
        swd3::LzoDecoder decoder({input, size}, out);
        return decoder.Run() == swd3::LzoStatus::Ok && decoder.Written() == out.size();
    };

    auto gameFile = [](const std::string& name)
    {
        return swd3test::GameFile(name);
    };

    const std::vector<golden::Frame> frames = golden::LoadGoldenFrames(gameFile, decompress);
    const std::vector<golden::PaletteFrame> palettes = golden::LoadGoldenPaletteFrames(gameFile, decompress);

    if (frames.empty() || golden.empty())
    {
        std::printf("  (game data or golden file not found, skipped)\n");
        return;
    }

    swd3::Surface screen(640, 480);
    swd3::Painter painter;
    painter.Init(screen);
    const swd3::RleCodec codec(painter);
    CodecBackend backend(codec);
    int compared = 0;
    int mismatches = 0;
    golden::RunRle(
        backend,
        frames,
        palettes,
        [&](const std::string& name, std::uint64_t hash)
        {
            char text[17];
            std::snprintf(text, sizeof(text), "%016llx", static_cast<unsigned long long>(hash));
            const auto found = golden.find(name);
            ++compared;

            if (found == golden.end() || found->second != text)
            {
                if (++mismatches <= 20)
                {
                    std::printf("  mismatch: %s\n", name.c_str());
                }
            }
        }
    );
    std::printf("  compared %d scenarios\n", compared);
    CHECK_EQ(mismatches, 0);
    CHECK_EQ(compared, static_cast<int>(golden.size()));
}
