#include <SDL3/SDL.h>

#include <cstdio>

#include "game_data.h"
#include "movie/movie.h"
#include "platform/display.h"
#include "platform/surface.h"
#include "recording_reporter.h"
#include "test.h"

using swd3::Display;

namespace
{

constexpr int kVolume = 0x9000;  // past Movie::MaxVolume
constexpr int kLowVolume = 100;
constexpr int kSecondFrame = 2;  // Position counts the frames from 1

}  // namespace

TEST_CASE("movie: Bink frames are decoded, copied and counted")
{
    const std::u8string path = swd3test::GameFile("Video/Parade.bik");

    if (swd3test::ReadGameFile("Video/Parade.bik").empty())
    {
        std::printf("  (Parade.bik not found, skipped)\n");
        return;
    }

    SDL_SetHint(SDL_HINT_AUDIO_DRIVER, "dummy");
    swd3::Movie movie;
    movie.Open(path, kVolume);
    CHECK(movie.Width() > 0 && movie.Width() <= Display::Width);
    CHECK(movie.Height() > 0 && movie.Height() <= Display::Height);
    CHECK_EQ(movie.Position(), 1);
    swd3::Surface surface(swd3::Display::Width, swd3::Display::Height);
    surface.Fill(0);
    swd3test::RecordingReporter reporter;
    const int x = (Display::Width - movie.Width()) / 2;
    const int y = (Display::Height - movie.Height()) / 2;
    const int frames = movie.Frame(surface, x, y, reporter);
    CHECK(frames > 1);
    CHECK(reporter.messages.empty());
    CHECK_EQ(movie.Position(), kSecondFrame);
    int lit = 0;

    for (int row = 0; row < Display::Height; ++row)
    {
        for (int column = 0; column < Display::Width; ++column)
        {
            lit += surface.Row(row)[column] != 0 ? 1 : 0;
        }
    }

    CHECK(lit > 0);
    int last = frames;

    for (int frame = kSecondFrame; frame <= frames && last > 0; ++frame)
    {
        last = movie.Frame(surface, x, y, reporter);
    }

    CHECK_EQ(last, -frames);
    CHECK_EQ(movie.Position(), frames);
    CHECK(movie.Close());
    CHECK(!movie.Close());
    CHECK_EQ(movie.Position(), -1);
    CHECK_EQ(movie.Width(), 0);
}

TEST_CASE("movie: a missing file keeps the error and stops at the first frame")
{
    swd3::Movie movie;
    movie.Open(swd3test::GameFile("Video/missing.bik"), kLowVolume);
    CHECK(!movie.Error().empty());
    CHECK_EQ(movie.Width(), 0);
    CHECK(!movie.Waiting());
    swd3::Surface surface(swd3::Display::Width, swd3::Display::Height);
    swd3test::RecordingReporter reporter;
    CHECK_EQ(movie.Frame(surface, 0, 0, reporter), 0);
    CHECK_EQ(reporter.messages.size(), 1u);
}
