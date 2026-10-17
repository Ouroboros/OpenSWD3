#include <cstdio>
#include <fstream>
#include <functional>
#include <map>
#include <memory>
#include <string>

#include "draw/painter.h"
#include "game_data.h"
#include "golden/ui_scenarios.h"
#include "golden_check.h"
#include "pic/rle.h"
#include "platform/surface.h"
#include "recording_reporter.h"
#include "res/tsw_archive.h"
#include "test.h"
#include "ui/ui_draw.h"

using golden::u16;
using golden::u32;
using golden::u64;

namespace
{

class UiBackend : public golden::UiBackend
{
public:
    UiBackend()
        : screen(640, 480),
          codec(this->painter),
          archive(this->codec, this->errors)
    {
        this->painter.Init(this->screen);
        this->archive.SetGameDirectory(swd3test::GameDirectory());
    }

    void Reset(u32 seed, int left, int top, int right, int bottom) override
    {
        for (int y = 0; y < 480; ++y)
        {
            for (int x = 0; x < 640; ++x)
            {
                this->screen.Row(y)[x] = golden::Background(seed, x, y);
            }
        }

        this->painter.SetTarget(this->screen.Pixels());
        this->painter.SetClipClamped(left, top, right, bottom);
    }

    void Fill(int x, int y, int width, int height, int red, int green, int blue, int mode) override
    {
        swd3::FillBox(this->painter, x, y, width, height, red, green, blue, static_cast<swd3::FillMode>(mode));
    }

    void Frame(int id, int left, int top, int right, int bottom, int alpha, int border) override
    {
        swd3::DrawWindowFrame(this->painter, this->archive, id, left, top, right, bottom, alpha, border);
    }

    const u16* Screen() const override
    {
        return this->screen.Pixels();
    }

private:
    swd3::Surface screen;
    swd3::Painter painter;
    swd3::RleCodec codec;
    swd3test::RecordingReporter errors;
    swd3::TswArchive archive;
};

}  // namespace

TEST_CASE("ui drawing: a blend one pixel wide covers one pair")
{
    swd3::Surface screen(640, 480);
    screen.Fill(0x7BEF);
    swd3::Painter painter;
    CHECK(painter.Init(screen));
    painter.SetTarget(screen.Pixels());
    painter.SetClipClamped(0, 0, 640, 480);
    swd3::FillBox(painter, 10, 10, 1, 3, 31, 31, 31, swd3::FillMode::Blend);
    CHECK(screen.Row(10)[10] != 0x7BEF);
    CHECK(screen.Row(10)[11] != 0x7BEF);
    CHECK_EQ(screen.Row(10)[12], 0x7BEF);
    CHECK_EQ(screen.Row(13)[10], 0x7BEF);
}

TEST_CASE("ui drawing: matches the golden results of the legacy boxes and frames")
{
    const std::map<std::string, std::string> golden =
        swd3test::LoadGolden(SWD3_SOURCE_DIR "/tests/golden/ui_golden.txt");

    if (golden.empty() || swd3test::ReadGameFile("all_sys.tsw").empty())
    {
        std::printf("  (game data or golden file not found, skipped)\n");
        return;
    }

    UiBackend backend;
    swd3test::GoldenCheck check(golden);
    golden::RunUi(backend, swd3test::GameFile("all_sys.tsw"), std::ref(check));
    std::printf("  compared %d scenarios\n", check.compared);
    CHECK_EQ(check.mismatches, 0);
    CHECK_EQ(check.compared, static_cast<int>(golden.size()));
}
