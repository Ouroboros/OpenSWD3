#include <cstdio>
#include <cstring>
#include <fstream>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "codec/lzo.h"
#include "core/types.h"
#include "draw/painter.h"
#include "game_data.h"
#include "golden/draw_scenarios.h"
#include "golden_check.h"
#include "platform/surface.h"
#include "test.h"

using swd3::u16;
using swd3::u32;
using swd3::u64;
using swd3::u8;

namespace
{

class PainterBackend : public golden::Backend
{
public:
    PainterBackend()
    {
        this->Reset(0);
        this->painter.Init(*this->surface);
    }

    void Reset(u32 seed) override
    {
        this->surface = std::make_unique<swd3::Surface>(golden::kScreenWidth, golden::kScreenHeight);

        for (int y = 0; y < golden::kScreenHeight; ++y)
        {
            for (int x = 0; x < golden::kScreenWidth; ++x)
            {
                this->surface->Row(y)[x] = golden::Background(seed, x, y);
            }
        }

        this->painter.SetTarget(this->surface->Pixels());
        this->painter.SetClipClamped(0, 0, golden::kScreenWidth, golden::kScreenHeight);
    }

    void SetClip(int left, int top, int right, int bottom) override
    {
        this->painter.SetClipClamped(left, top, right, bottom);
    }

    int Draw(const golden::DrawCall& call) override
    {
        this->Prepare(call);
        swd3::DrawArgument argument;
        argument.alphaMap = call.alphaMap;
        argument.colorPair = call.colorPair;
        return this->painter.DrawPicture(call.x, call.y, call.width, call.height, call.mode, argument);
    }

    void Outline(const golden::DrawCall& call) override
    {
        this->Prepare(call);
        swd3::DrawArgument argument;
        argument.colorPair = call.colorPair;
        this->painter.DrawOutline(call.x, call.y, call.width, call.height, call.mode, argument);
    }

    void Zoom(
        const void* source,
        int x,
        int y,
        int width,
        int height,
        bool mirrored,
        int anchorX,
        int anchorY,
        int zoomX,
        int zoomY
    ) override
    {
        this->painter.SetSource(source);
        this->painter.SetZoom(anchorX, anchorY, zoomX, zoomY);

        if (mirrored)
        {
            this->painter.DrawZoomedMirrored(x, y, width, height);
        }
        else
        {
            this->painter.DrawZoomed(x, y, width, height);
        }
    }

    void Tile(int kind, const void* source, const u16* palette, int x, int y) override
    {
        this->painter.SetSource(source);
        this->painter.SetPalette(palette);

        switch (kind)
        {
            case 0:
                this->painter.DrawTile16(x, y);
                break;

            case 1:
                this->painter.DrawTile16Keyed(x, y);
                break;

            case 2:
                this->painter.DrawTile8(x, y);
                break;

            default:
                this->painter.DrawTile8Keyed(x, y);
                break;
        }

        this->painter.SetPalette(nullptr);
    }

    void Pixels(
        golden::PixelOperation operation,
        u16* pixels,
        std::size_t count,
        const u16* other,
        int a,
        int b,
        int c
    ) override
    {
        const std::span<u16> span(pixels, count);

        switch (operation)
        {
            case golden::PixelOperation::Tint:
                this->painter.TintPixels(span, a, b, c);
                break;

            case golden::PixelOperation::ShiftRed:
                this->painter.ShiftRed(span, a);
                break;

            case golden::PixelOperation::ShiftGreenPairs:
                this->painter.ShiftGreenPairs(span, a);
                break;

            case golden::PixelOperation::ShiftBluePairs:
                this->painter.ShiftBluePairs(span, a);
                break;

            case golden::PixelOperation::Mix:
                this->painter.MixPixels(std::span<const u16>(other, count), span);
                break;

            case golden::PixelOperation::Gray:
                this->painter.GrayPixels(span);
                break;

            case golden::PixelOperation::BlendPairs:
            {
                std::vector<u32> pairs(count / 2);
                std::memcpy(pairs.data(), pixels, pairs.size() * 4);
                this->painter.BlendPairsWithColor(pairs, static_cast<u32>(a));
                std::memcpy(pixels, pairs.data(), pairs.size() * 4);
                break;
            }
        }
    }

    u32 MakeColor(u32 red, u32 green, u32 blue) override
    {
        return this->painter.MakeColor(red, green, blue);
    }

    const u16* Screen() const override
    {
        return this->surface->Pixels();
    }

private:
    void Prepare(const golden::DrawCall& call)
    {
        this->painter.SetSource(call.source);
        this->painter.SetPalette(call.palette);
        this->painter.SetWave(call.wave);
        swd3::DrawOptions& options = this->painter.Options();
        options.alpha = call.alpha;
        options.tintRed = call.tintRed;
        options.tintGreen = call.tintGreen;
        options.tintBlue = call.tintBlue;
        options.dropLines = call.dropLines;
        options.scaleHeight = call.scaleHeight;
        options.shear = call.shear;
    }

    std::unique_ptr<swd3::Surface> surface;
    swd3::Painter painter;
};

}  // namespace

TEST_CASE("painter: matches the golden hashes of the legacy drawing kernel")
{
    const std::map<std::string, std::string> golden =
        swd3test::LoadGolden(SWD3_SOURCE_DIR "/tests/golden/draw_golden.txt");

    auto decompress = [](const u8* input, std::size_t size, std::vector<u8>& out)
    {
        swd3::LzoDecoder decoder({input, size}, out);
        return decoder.Run() == swd3::LzoStatus::Ok && decoder.Written() == out.size();
    };

    const std::vector<golden::Frame> frames = golden::LoadGoldenFrames(
        [](const std::string& name)
        {
            return swd3test::GameFile(name);
        },

        decompress
    );

    if (frames.empty() || golden.empty())
    {
        std::printf("  (game data or golden file not found, skipped)\n");
        return;
    }

    PainterBackend backend;
    swd3test::GoldenCheck check(golden);
    golden::Run(backend, frames, std::ref(check));
    std::printf("  compared %d scenarios\n", check.compared);
    CHECK_EQ(check.mismatches, 0);
    CHECK_EQ(check.compared, static_cast<int>(golden.size()));
}
