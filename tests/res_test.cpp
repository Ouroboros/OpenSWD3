#include <cstring>

#include "crt/crt.h"
#include "data/globals.h"
#include "res/act.h"
#include "res/act_anim.h"
#include "res/tsw.h"
#include "test.h"

using namespace swd3;

namespace {

#if defined(_WIN32)
const char* kGameDir = "E:\\Game\\swd3\\";
#else
const char* kGameDir = "/mnt/e/Game/swd3/";
#endif

bool prepareGameDir() {
    std::strcpy(G.gameDir, kGameDir);
    std::strcpy(G.tswPath, kGameDir);
    std::strcat(G.tswPath, "all_sys.tsw");
    HANDLE probe = TswOpenFile(G.tswPath);
    if (!probe) {
        std::printf("  (game data not found at %s, skipped)\n", kGameDir);
        return false;
    }
    w32::CloseHandle(probe);
    G.rleMagic = 0xFFFF;
    G.rleFillColor8 = 1;
    G.rleSkipColor8 = 3;
    return true;
}

}  // namespace

TEST_CASE("res.tsw loads, caches and evicts pictures from all_sys.tsw") {
    if (!prepareGameDir()) {
        return;
    }
    Picture* picture = TswGetPicture(9001, 0);
    CHECK(picture != nullptr);
    if (!picture) {
        return;
    }
    CHECK_EQ(picture->pixels[0], 0xFFFF);
    CHECK_EQ(picture->pixels[3] & 0x7FFF, 0x10);
    CHECK(picture->width > 0);
    CHECK(picture->height > 0);
    CHECK_EQ(picture->pixels[1], picture->width);
    CHECK(G.tswCacheBytes == static_cast<std::int32_t>(picture->size));
    CHECK_EQ(G.tswBucketCounts[9001 % 10], 1);
    CHECK(TswFindPicture(9001, 0) == picture);
    CHECK(TswGetPicture(9001, 0) == picture);
    CHECK_EQ(G.tswBucketCounts[9001 % 10], 1);
    const int limit = G.tswCacheLimit;
    G.tswCacheLimit = 1;
    TswCacheEvict();
    CHECK_EQ(G.tswBucketCounts[9001 % 10], 0);
    CHECK_EQ(G.tswCacheBytes, 0);
    G.tswCacheLimit = limit;
    CHECK(TswGetPicture(9002, 0) != nullptr);
    CHECK_EQ(G.tswBucketCounts[9002 % 10], 1);
    TswCloseAll();
    CHECK_EQ(G.tswCacheBytes, 0);
    CHECK_EQ(G.tswOpened, 0);
    CHECK(G.tswFiles[3] == INVALID_HANDLE_VALUE);
}

TEST_CASE("res.act loads and caches entries from all_sys.act") {
    if (!prepareGameDir()) {
        return;
    }
    std::uint8_t* data = ActGet(9001, 0);
    CHECK(data != nullptr);
    CHECK(G.actCacheBytes > 0);
    CHECK(ActGet(9001, 0) == data);
    CHECK_EQ(G.actBucketCounts[9001 % 10], 1);
    std::uint8_t* direct = ActLoadDirect(9001, 0);
    CHECK(direct != nullptr);
    CHECK(direct != data);
    CHECK(std::memcmp(direct, data, static_cast<std::size_t>(G.actCacheBytes)) == 0);
    crt::free(direct);
    ActCacheClear();
    CHECK_EQ(G.actCacheBytes, 0);
    CHECK_EQ(ActCloseAll(), 0);
}

TEST_CASE("res.act script runs frame by frame from all_sys.act") {
    if (!prepareGameDir()) {
        return;
    }
    G.actOption = 1;
    ActState state{};
    state.id = 9001;
    CHECK_EQ(ActRun(&state), 1);
    CHECK(state.script == ActGet(9001, 0));
    CHECK_EQ(G.actStateReset, 0);
    const std::uint16_t afterFirst = state.pc;
    CHECK(afterFirst > 0);
    // the frame ends on a 'DE' word just before the new position
    std::uint16_t end;
    std::memcpy(&end, state.script + 2 * (afterFirst - 1), 2);
    CHECK_EQ(end, 0x4544);
    // a direction change restarts the script
    state.direction = 1;
    state.delay = 0;
    ActRun(&state);
    CHECK_EQ(state.lastDirection, 1);
    ActCacheClear();
    ActCloseAll();
}
