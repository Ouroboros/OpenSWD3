#include <cstring>

#include "data/globals.h"
#include "test.h"

using namespace swd3;

TEST_CASE("image.initial data matches swd32.exe") {
    CHECK_EQ(G.tswCacheLimit, 0x600000);
    CHECK_EQ(G.actCacheLimit, 0x100000);
    CHECK(std::strcmp(G.aAllCharTsw, "all_char.tsw") == 0);
    CHECK(image::va(&G.tswCacheLimit) == 0x004A6020u);
    CHECK(image::va(&G.file_4CF6E0) == 0x004CF6E0u);
    CHECK_EQ(image::at<std::int32_t>(0x004A833C), 1);
}

TEST_CASE("image.bss region is zero") {
    for (std::uint32_t va = image::kRawEnd; va < image::kEnd; va += 0x1000) {
        CHECK_EQ(image::at<std::uint8_t>(va), 0);
    }
}
