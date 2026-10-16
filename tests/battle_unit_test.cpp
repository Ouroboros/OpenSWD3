#include <cstring>
#include <memory>
#include <new>

#include "battle/battle_unit.h"
#include "core/random.h"
#include "data/globals.h"
#include "test.h"

using namespace swd3;

namespace {

struct UnitFixture {
    std::unique_ptr<std::uint8_t[]> storage{new std::uint8_t[sizeof(BattleUnit)]()};
    BattleUnit& unit = *new (storage.get()) BattleUnit();
    ~UnitFixture() { unit.~BattleUnit(); }
};

}  // namespace

TEST_CASE("battle.unit construction defaults and counters") {
    UnitFixture f;
    CHECK_EQ(f.unit.at<std::uint32_t>(0x2A56), 0xFFFFFFFFu);
    CHECK_EQ(f.unit.GetWord29A2(), 0xFFFF);
    CHECK_EQ(f.unit.at<std::int32_t>(0x2668), 0xF);
    CHECK_EQ(f.unit.at<std::uint32_t>(0x26BC), 0x062B062Bu);
    for (int i = 0; i < 40; ++i) {
        f.unit.StepTargetCount(1);
    }
    CHECK_EQ(f.unit.at<std::uint16_t>(0x2A7C), 0x22);
    CHECK_EQ(f.unit.PushTarget(7), 0x22);
    CHECK_EQ(f.unit.at<std::uint16_t>(0x29C4 + 2 * 0x22), 7);
    CHECK_EQ(f.unit.SetStatus(0x200), 0x200);
    CHECK_EQ(f.unit.at<std::int32_t>(0x266C), -0x20);
    f.unit.ClearStatus(0x1200);
    CHECK_EQ(f.unit.at<std::uint16_t>(0x26D0), 0);
    f.unit.IncreaseHits();
    CHECK_EQ(f.unit.GetAE0(), 1);
    CHECK_EQ(f.unit.DecreaseHits(), 0);
    CHECK_EQ(f.unit.GetWord2A74(), 0);
}

TEST_CASE("battle.scale by tenths and evasion streaks") {
    std::int16_t value = 200;
    CHECK_EQ(ScaleByTenths(3, &value), 0);
    CHECK_EQ(value, 260);
    value = 200;
    CHECK_EQ(ScaleByTenths(-15, &value), 1);
    CHECK_EQ(value, -100);

    UnitFixture f;
    G.evadeDisabled = 1;
    CHECK_EQ(f.unit.RollEvade(0), 0);
    G.evadeDisabled = 0;
    CHECK_EQ(f.unit.RollEvade(0), 0);  // no evasion skill
    f.unit.at<std::uint8_t>(0x2A93) = 5;
    G.evadeStreak = 0x18;
    G.evadeJustSucceeded = 0;
    CHECK_EQ(f.unit.RollEvade(0), 1);
    CHECK_EQ(G.evadeStreak, 0);
    CHECK_EQ(G.evadeJustSucceeded, 1);
    CHECK_EQ(f.unit.at<std::uint16_t>(0x26D0) & 1, 1);
    CHECK_EQ(f.unit.RollEvade(0), 0);
    CHECK_EQ(G.evadeJustSucceeded, 0);
}
