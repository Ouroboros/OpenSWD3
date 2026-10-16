#include <SDL3/SDL.h>

#include <cstdio>

#include "game_data.h"
#include "movie/movie.h"
#include "platform/surface.h"
#include "recording_reporter.h"
#include "test.h"

TEST_CASE("movie: Bink frames are decoded, copied and counted")
{
    const std::string path = swd3test::GameFile("Video/Parade.bik");

    if (swd3test::ReadGameFile("Video/Parade.bik").empty())
    {
        std::printf("  (Parade.bik not found, skipped)\n");
        return;
    }

    SDL_SetHint(SDL_HINT_AUDIO_DRIVER, "dummy");
    swd3::Movie movie;
    movie.Open(path, 0x9000);
    CHECK(movie.Width() > 0 && movie.Width() <= 640);
    CHECK(movie.Height() > 0 && movie.Height() <= 480);
    CHECK_EQ(movie.Position(), 1);
    swd3::Surface surface(640, 480);
    surface.Fill(0);
    swd3test::RecordingReporter reporter;
    const int x = (640 - movie.Width()) / 2;
    const int y = (480 - movie.Height()) / 2;
    const int frames = movie.Frame(surface, x, y, reporter);
    CHECK(frames > 1);
    CHECK(reporter.messages.empty());
    CHECK_EQ(movie.Position(), 2);
    int lit = 0;

    for (int row = 0; row < 480; ++row)
    {
        for (int column = 0; column < 640; ++column)
        {
            lit += surface.Row(row)[column] != 0 ? 1 : 0;
        }
    }

    CHECK(lit > 0);
    int last = frames;

    for (int frame = 2; frame <= frames && last > 0; ++frame)
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
    movie.Open(swd3test::GameFile("Video/missing.bik"), 100);
    CHECK(!movie.Error().empty());
    CHECK_EQ(movie.Width(), 0);
    CHECK(!movie.Waiting());
    swd3::Surface surface(640, 480);
    swd3test::RecordingReporter reporter;
    CHECK_EQ(movie.Frame(surface, 0, 0, reporter), 0);
    CHECK_EQ(reporter.messages.size(), 1u);
}
