#include <cstdio>
#include <cstring>

#include "battle/battle_enemy.h"
#include "battle/fame.h"
#include "core/mem.h"
#include "crt/crt.h"
#include "data/globals.h"
#include "test.h"

using namespace swd3;

namespace {

#if defined(_WIN32)
const char* kGameDir = "E:\\Game\\swd3\\";
#else
const char* kGameDir = "/mnt/e/Game/swd3/";
#endif

}  // namespace

TEST_CASE("battle.monster data and skills load from mon.dat and level.dat") {
    char savedDir[260];
    std::memcpy(savedDir, G.gameDir, sizeof(savedDir));
    std::strcpy(G.gameDir, kGameDir);
    G.monsterFileOpen = 0;
    G.monsterFile2Open = 0;
    G.monsterTextBytes = 0;
    std::uint8_t data[0xA4] = {};
    const int loaded = LoadMonster(data, 1);
    if (loaded == 0) {
        std::printf("  (mon.dat not readable at %s, skipped)\n", kGameDir);
    } else {
        CHECK(data[0] != 0);
        CHECK(std::strlen(reinterpret_cast<const char*>(data)) < 0x20);
        // monster 1: name, then opcode 1 copies 0x4D bytes to +0x50 (0, 0x17, ...)
        CHECK_EQ(mem::u16(data, 0x52), 0x17);
        CHECK_EQ(G.monsterFileOpen, 1);
        if (mem::ptr<std::uint8_t>(data, 0xA0) != nullptr) {
            CHECK(G.monsterTextBytes > 0);
            CHECK_EQ(LoadMonster(data, 1), 1);
            CHECK_EQ(G.monsterTextBytes, static_cast<std::int32_t>(crt::msize(mem::ptr<std::uint8_t>(data, 0xA0))));
            crt::free(mem::ptr<std::uint8_t>(data, 0xA0));
        }
        std::uint32_t experience = 0;
        CHECK_EQ(LoadLevelEntry(0, 1, &experience), 0);
        CHECK_EQ(LoadLevelEntry(1, 3, &experience), 1);
        CHECK_EQ(experience, 0x28u);
        CHECK_EQ(G.monsterFile2Open, 1);
        CloseRecordFiles();
        CHECK_EQ(G.monsterFileOpen, 0);
        CHECK_EQ(G.monsterFile, -1);
    }
    std::memcpy(G.gameDir, savedDir, sizeof(savedDir));
}
