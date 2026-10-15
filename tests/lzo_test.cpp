#include <cstdio>
#include <cstring>
#include <vector>

#include "codec/lzo.h"
#include "test.h"

using namespace swd3;

namespace {

#if defined(_WIN32)
const char* kSysTsw = "E:\\Game\\swd3\\all_sys.tsw";
#else
const char* kSysTsw = "/mnt/e/Game/swd3/all_sys.tsw";
#endif

std::vector<std::uint8_t> readFile(const char* path) {
    std::vector<std::uint8_t> data;
    if (FILE* f = std::fopen(path, "rb")) {
        std::fseek(f, 0, SEEK_END);
        data.resize(static_cast<std::size_t>(std::ftell(f)));
        std::fseek(f, 0, SEEK_SET);
        if (std::fread(data.data(), 1, data.size(), f) != data.size()) {
            data.clear();
        }
        std::fclose(f);
    }
    return data;
}

std::uint32_t u32(const std::vector<std::uint8_t>& d, std::size_t at) {
    std::uint32_t v;
    std::memcpy(&v, d.data() + at, 4);
    return v;
}

}  // namespace

TEST_CASE("lzo.decompress first frame of all_sys.tsw entry 1") {
    std::vector<std::uint8_t> file = readFile(kSysTsw);
    if (file.empty()) {
        std::printf("  (game data not found, skipped)\n");
        return;
    }
    const std::uint32_t base = u32(file, 0x1C + 0x18);
    std::uint16_t format;
    std::memcpy(&format, file.data() + base + 8, 2);
    std::size_t frames = base + 12 + (format == 8 ? 0x200 : 0);
    const std::uint32_t offset = u32(file, frames);
    const std::uint32_t packed = u32(file, frames + 4);
    const std::uint32_t size = u32(file, frames + 8);
    std::printf("  base=0x%X frame offset=0x%X packed=%u size=%u\n", base, offset, packed, size);
    std::vector<std::uint8_t> out(size + 0x10000, 0xEE);
    std::uint32_t outLength = 0;
    int rc = Lzo1xDecompress(file.data() + base + offset, packed, out.data(), &outLength);
    CHECK_EQ(rc, 0);
    CHECK_EQ(outLength, size);
    CHECK_EQ(out[size], 0xEE);
    std::uint16_t magic;
    std::memcpy(&magic, out.data(), 2);
    CHECK_EQ(magic, 0xFFFF);
}
