#include <SDL3/SDL.h>

#include <cstdio>
#include <fstream>
#include <map>
#include <sstream>
#include <string>

#include "audio/audio_manager.h"
#include "battle/battle_hud.h"
#include "draw/painter.h"
#include "game_data.h"
#include "golden/hud_scenarios.h"
#include "pic/rle.h"
#include "platform/audio_device.h"
#include "platform/surface.h"
#include "random/random.h"
#include "recording_reporter.h"
#include "res/act_archive.h"
#include "res/tsw_archive.h"
#include "test.h"

using golden::i32;
using golden::u16;
using golden::u32;
using golden::u64;

namespace
{

std::map<std::string, std::string> LoadGolden(const std::string& path)
{
    std::map<std::string, std::string> golden;
    std::ifstream file(path);
    std::string line;

    while (std::getline(file, line))
    {
        std::istringstream fields(line);
        std::string name;
        std::string hash;

        if (fields >> name >> hash)
        {
            golden[name] = hash;
        }
    }

    return golden;
}

class HudBackend : public golden::HudBackend
{
public:
    HudBackend()
        : screen(640, 480),
          codec(this->painter),
          archive(this->codec, this->errors),
          acts(this->errors),
          audio(this->device),
          hud(this->painter, this->archive, this->acts, this->random, this->audio)
    {
        this->painter.Init(this->screen);
        this->archive.SetGameDirectory(swd3test::GameDirectory());
        this->acts.SetGameDirectory(swd3test::GameDirectory());
    }

    void Reset(u32 seed) override
    {
        for (int y = 0; y < 480; ++y)
        {
            for (int x = 0; x < 640; ++x)
            {
                this->screen.Row(y)[x] = golden::Background(seed, x, y);
            }
        }

        this->painter.SetTarget(this->screen.Pixels());
        this->painter.SetClipClamped(0, 0, 640, 480);
    }

    void SeedRandom(u32 seed) override
    {
        this->random.Seed(seed);
    }

    int PictureFrame0(int id, int x, int y) override
    {
        return this->hud.DrawPictureFrame0(id, x, y) != 0;
    }

    int Portrait(int index, int x, int y, int dim, bool highlight) override
    {
        this->hud.portraitHighlight[static_cast<std::size_t>(index)] = highlight ? 1 : 0;
        return this->hud.DrawPortrait(static_cast<u16>(index), x, y, dim) != 0;
    }

    int Arrow(int x, int y, int slot) override
    {
        return this->hud.DrawArrow(x, y, slot) != 0;
    }

    int Bar(int id, int frame, int x, int y, int width) override
    {
        return this->hud.DrawBar(id, frame, x, y, width) != 0;
    }

    int PictureFrame(int id, int frame, int x, int y) override
    {
        return this->hud.DrawPictureFrame(id, frame, x, y) != 0;
    }

    int Gauge(int id, int x, int y, int fill) override
    {
        return this->hud.DrawGauge(id, x, y, fill) != 0;
    }

    int GaugeAlt(int id, int x, int y, int fill) override
    {
        return this->hud.DrawGaugeAlt(id, x, y, fill);
    }

    int GaugeWide(int id, int x, int y, int fill) override
    {
        return this->hud.DrawGaugeWide(id, x, y, static_cast<u16>(fill)) != 0;
    }

    int SmallNumber(int font, int value, int x, int y) override
    {
        return this->hud.DrawSmallNumber(font, value, x, y) != 0;
    }

    int Number(int font, int value, int x, int y, int digitMode) override
    {
        this->hud.digitMode = static_cast<u16>(digitMode);
        return this->hud.DrawNumber(static_cast<swd3::i16>(font), value, x, y);
    }

    int SlotAnimation(int id, int slot, int x, int y) override
    {
        return this->hud.DrawSlotAnimation(id, slot, x, y) != 0;
    }

    int Fill(int x, int y, int width, int height, u32 color) override
    {
        return this->hud.FillColor(x, y, width, height, color) != 0;
    }

    int Effect(int id, int action, int x, int y, int mirror) override
    {
        return this->hud.DrawEffectAt(id, action, x, y, mirror);
    }

    int ResetEffect() override
    {
        return this->hud.ResetEffectAt();
    }

    int Cursor(int id, int x, int y) override
    {
        return this->hud.DrawCursorAnimation(id, x, y) != 0;
    }

    int Digit(int font, int digit, int x, int y, int digitMode) override
    {
        this->hud.digitMode = static_cast<u16>(digitMode);
        return this->hud.DrawDigit(font, digit, x, y);
    }

    int ScrollBar(int id, int x, int y, int rows, int offset, int style, int count, int first) override
    {
        this->hud.scrollCount = count;
        this->hud.scrollFirst = first;
        return this->hud.DrawScrollBar(static_cast<u16>(id), x, y, rows, offset, style) != 0;
    }

    int CoinToss() override
    {
        return this->hud.DrawCoinToss();
    }

    int TimingBar(int x, int y) override
    {
        return this->hud.DrawTimingBar(x, y);
    }

    int TallGauge(int x, int y, int level) override
    {
        return this->hud.DrawTallGauge(x, y, level) != 0;
    }

    int AnimationLoad(int x, int y, int id, int divisor) override
    {
        return this->hud.LoadAnimation(this->animation, 0, x, y, static_cast<u16>(id), static_cast<u16>(divisor));
    }

    int AnimationDraw() override
    {
        return this->hud.DrawAnimation(this->animation);
    }

    int AnimationPlayShifted(int amount) override
    {
        return this->hud.PlayAnimationShifted(this->animation, 0, amount);
    }

    int AnimationFree() override
    {
        return this->hud.FreeAnimation(this->animation);
    }

    std::vector<i32> State() override
    {
        return {
            this->hud.numberX,
            this->hud.numberValue,
            this->hud.scrollStep,
            this->hud.scrollThumbTop,
            this->hud.scrollThumbBottom,
            this->hud.coinFrames,
            static_cast<i32>(this->hud.coinState),
            static_cast<i32>(this->hud.coinDelay),
            this->hud.timingCounter,
            this->hud.timingHit,
        };
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
    swd3::ActArchive acts;
    swd3::Random random;
    swd3::AudioDevice device;
    swd3::AudioManager audio;
    swd3::BattleHud hud;
    swd3::BattleAnimation animation;
};

}  // namespace

TEST_CASE("battle hud: matches the golden results of the legacy helpers")
{
    const std::map<std::string, std::string> golden = LoadGolden(SWD3_SOURCE_DIR "/tests/golden/hud_golden.txt");

    if (golden.empty() || swd3test::ReadGameFile("all_sys.act").empty())
    {
        std::printf("  (game data or golden file not found, skipped)\n");
        return;
    }

    SDL_SetHint(SDL_HINT_AUDIO_DRIVER, "dummy");
    HudBackend backend;
    int compared = 0;
    int mismatches = 0;
    golden::RunHud(
        backend,
        [&](const std::string& name, u64 hash)
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
