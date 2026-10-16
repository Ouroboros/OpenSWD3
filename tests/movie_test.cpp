#include <SDL3/SDL.h>

#include <cstdio>
#include <vector>

#include "data/globals.h"
#include "movie/movie.h"
#include "test.h"

using namespace swd3;

namespace {

#if defined(_WIN32)
const char* kParade = "E:\\Game\\swd3\\Video\\Parade.bik";
#else
const char* kParade = "/mnt/e/Game/swd3/Video/Parade.bik";
#endif

}  // namespace

TEST_CASE("movie.bink playback through the Movie object") {
    SDL_SetHint(SDL_HINT_AUDIO_DRIVER, "dummy");
    MovieInit(nullptr, 0, 0);
    CHECK_EQ(G.movieSurfaceType, static_cast<std::int32_t>(bink::BINKSURFACE565));
    Movie missing;
    CHECK_EQ(missing.Open("no-such-movie.bik", 0, 0x4000, 0), 0);
    CHECK(missing.m_bink == nullptr);
    CHECK(missing.m_error[0] != 0);
    CHECK_EQ(missing.Position(), -1);
    CHECK_EQ(missing.Width(), 0);

    Movie movie;
    movie.Open(kParade, 0, 0x9000, 0);
    if (movie.m_bink == nullptr) {
        std::printf("  (%s not found, skipped)\n", kParade);
        return;
    }
    const int width = movie.Width();
    const int height = movie.Height();
    CHECK(width > 0 && width <= 640);
    CHECK(height > 0 && height <= 480);
    const int frames = static_cast<int>(movie.m_bink->Frames);
    CHECK(frames > 1);
    CHECK_EQ(movie.Position(), 1);
    std::vector<std::uint16_t> screen(640 * 480, 0);
    CHECK_EQ(movie.Frame(screen.data(), 640 * 2, 480, 0, 0), frames);
    CHECK_EQ(movie.Position(), 2);
    auto lit = [&screen, width, height]() {
        int count = 0;
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                count += screen[y * 640 + x] != 0;
            }
        }
        return count;
    };
    int litFrames = lit() > 0 ? 1 : 0;
    for (int i = 2; i < frames; ++i) {
        movie.Frame(screen.data(), 640 * 2, 480, 0, 0);
        litFrames += lit() > 0 ? 1 : 0;
    }
    CHECK(litFrames > frames / 2);
    CHECK_EQ(movie.Position(), frames);
    CHECK_EQ(movie.Frame(screen.data(), 640 * 2, 480, 0, 0), -frames);
    CHECK_EQ(movie.Close(), 1);
    CHECK_EQ(movie.Close(), 0);
}
