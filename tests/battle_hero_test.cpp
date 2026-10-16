#include <cstring>
#include <memory>
#include <new>

#include "battle/battle_hero.h"
#include "crt/crt.h"
#include "data/globals.h"
#include "test.h"

using namespace swd3;

namespace {

struct HeroFixture {
    std::unique_ptr<std::uint8_t[]> storage{new std::uint8_t[sizeof(BattleHero)]()};
    BattleHero& hero = *new (storage.get()) BattleHero();
    std::uint8_t data[0x40] = {};
    HeroFixture() { hero.at<std::uint8_t*>(4) = data; }
    ~HeroFixture() {
        crt::free(hero.at<std::uint8_t*>(0));
        hero.at<std::uint8_t*>(0) = nullptr;
    }
};

}  // namespace

TEST_CASE("battle.hero gauge steps with speed modifiers and starts the turn when full") {
    HeroFixture f;
    const std::int32_t savedFull = G.gaugeFull;
    G.gaugeFull = 100;
    mem::put16(f.hero.at<std::uint8_t*>(0), 0x16, 40);
    f.hero.at<std::uint32_t>(0x26C0) = 0x20000000;
    CHECK_EQ(f.hero.StepGauge(1), 0);
    CHECK_EQ(f.hero.at<std::uint16_t>(0x2A12), 16);
    f.hero.at<std::uint32_t>(0x26C0) = 0x40;
    CHECK_EQ(f.hero.StepGauge(0), 0);
    CHECK_EQ(f.hero.at<std::uint16_t>(0x2A12), 21);
    f.hero.at<std::uint16_t>(0x2A12) = 100;
    f.hero.at<std::int32_t>(0x2B20) = 1;
    f.hero.at<std::int32_t>(0x2B08) = 1;
    CHECK_EQ(f.hero.StepGauge(0), 1);
    CHECK_EQ(f.hero.at<std::int32_t>(0x2AB0), 1);
    CHECK_EQ(f.hero.at<std::int32_t>(0x2B08), 0);
    CHECK_EQ(f.hero.at<std::int32_t>(0x45C), 1);
    CHECK_EQ(f.hero.StepGauge(0), 0);
    G.gaugeFull = savedFull;
}

TEST_CASE("battle.hero mp cost and first strike bookkeeping") {
    HeroFixture f;
    mem::put16(f.data, 8, 5);
    f.hero.at<std::uint16_t>(0x2F12) = 7;
    f.hero.ApplyPendingMpCost();
    CHECK_EQ(mem::s16(f.data, 8), 0);
    CHECK_EQ(f.hero.at<std::uint16_t>(0x2F12), 0);

    G.dword_53CEBC = 0;
    G.dword_53CEB4 = 0;
    G.dword_53BCE4 = 2;
    f.hero.at<std::uint16_t>(0xD64) = 3;
    CHECK_EQ(f.hero.TryFirstStrike(), 0);
    G.dword_53CEB4 = 10;
    CHECK_EQ(f.hero.TryFirstStrike(), 1);
    CHECK_EQ(f.hero.at<std::uint8_t>(0x2F30), 1);
    CHECK_EQ(G.dword_53CEB4 & 0xFF, 0);
    CHECK_EQ(G.dword_53CEBC, 3);
    CHECK_EQ(f.hero.TryFirstStrike(), 0);
    G.dword_53CEBC = 0;
    G.dword_53BCE4 = 0;
}
