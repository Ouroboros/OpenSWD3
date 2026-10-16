#include <cstring>
#include <memory>
#include <new>

#include "battle/battle_message.h"
#include "battle/battle_unit.h"
#include "crt/crt.h"
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

TEST_CASE("battle.unit damage digits per target slot") {
    UnitFixture f;
    f.unit.at<std::uint16_t>(0x2A7C) = 2;
    f.unit.SetDamageDigits(1234);
    CHECK_EQ(f.unit.at<std::int16_t>(0x2724 + 16), 1);
    CHECK_EQ(f.unit.at<std::int16_t>(0x2726 + 16), 2);
    CHECK_EQ(f.unit.at<std::int16_t>(0x2728 + 16), 3);
    CHECK_EQ(f.unit.at<std::int16_t>(0x272A + 16), 4);
    f.unit.SetDamageDigits(static_cast<std::int16_t>(-56));
    CHECK_EQ(f.unit.at<std::int16_t>(0x2724 + 16), 0);
    CHECK_EQ(f.unit.at<std::int16_t>(0x2726 + 16), 0);
    CHECK_EQ(f.unit.at<std::int16_t>(0x2728 + 16), 5);
    CHECK_EQ(f.unit.at<std::int16_t>(0x272A + 16), 6);
}

TEST_CASE("battle.unit expired effects lose their status and skip the effect after a freed one") {
    UnitFixture f;
    G.battleMessages = nullptr;
    char name[] = "unit";
    f.unit.at<char*>(0x2560) = name;
    auto* first = static_cast<std::uint8_t*>(crt::malloc(0xB0));
    auto* second = static_cast<std::uint8_t*>(crt::malloc(0xB0));
    auto* third = static_cast<std::uint8_t*>(crt::malloc(0xB0));
    std::memset(first, 0, 0xB0);
    std::memset(second, 0, 0xB0);
    std::memset(third, 0, 0xB0);
    mem::putPtr(first, 0, second);
    mem::putPtr(second, 0, third);
    mem::put32(first, 0x9C, 0x20);
    mem::put32(second, 0x9C, 0x40);
    mem::put32(third, 0x9C, 0x80);
    f.unit.at<std::uint8_t*>(0x2584) = first;
    f.unit.at<std::uint32_t>(0x26C0) = 0xE0;
    f.unit.ExpireEffects(0);
    CHECK(f.unit.at<std::uint8_t*>(0x2584) == second);
    CHECK_EQ(f.unit.at<std::uint32_t>(0x26C0), 0x40u);
    CHECK(mem::ptr<std::uint8_t>(second, 0) == nullptr);
    CHECK(G.battleMessages == nullptr);
    f.unit.ExpireEffects(1);
    CHECK(f.unit.at<std::uint8_t*>(0x2584) == nullptr);
    CHECK_EQ(f.unit.at<std::uint32_t>(0x26C0), 0u);
}

namespace {

void freeMessages() {
    while (G.battleMessages != nullptr) {
        BattleMessage* next = G.battleMessages->next;
        crt::free(G.battleMessages);
        G.battleMessages = next;
    }
}

}  // namespace

TEST_CASE("battle.unit buffs are appended once, refreshed and announced") {
    UnitFixture f;
    G.battleMessages = nullptr;
    char name[] = "unit";
    f.unit.at<char*>(0x2560) = name;
    f.unit.at<std::int32_t>(0x2AA0) = 1;
    std::uint8_t skill[0x30] = {};
    mem::put32(skill, 8, 0x80000000u);
    skill[0x24] = 3;
    mem::put16(skill, 0xE, 0x55);
    CHECK_EQ(f.unit.AddBuff(skill), 1);
    std::uint8_t* node = f.unit.at<std::uint8_t*>(0x2584);
    CHECK(node != nullptr);
    CHECK_EQ(mem::u32(node, 0x9C), 0x80000000u);
    CHECK_EQ(mem::u16(node, 0xA2), 3);
    CHECK_EQ(mem::u16(node, 0xA8), 0x55);
    CHECK_EQ(f.unit.at<std::uint32_t>(0x26C0), 0x10000000u);
    CHECK(G.battleMessages != nullptr);
    skill[0x24] = 5;
    CHECK_EQ(f.unit.AddBuff(skill), 1);
    CHECK(f.unit.at<std::uint8_t*>(0x2584) == node);
    CHECK(mem::ptr<std::uint8_t>(node, 0) == nullptr);
    CHECK_EQ(mem::u16(node, 0xA2), 5);
    mem::put32(skill, 8, 0x4000000u);
    CHECK_EQ(f.unit.AddBuff(skill), 1);
    CHECK_EQ(f.unit.at<std::uint16_t>(0x2A90), 5);
    CHECK_EQ(f.unit.at<std::uint32_t>(0x26C0), 0x12000000u);
    f.unit.ExpireEffects(1);
    freeMessages();
}

TEST_CASE("battle.unit ailments roll against the resistance and are prepended") {
    UnitFixture f;
    std::uint8_t stats[0x40] = {};
    f.unit.at<std::uint8_t*>(0) = stats;
    f.unit.at<std::int32_t>(0x2AA0) = 1;
    std::uint8_t skill[0x30] = {};
    mem::put16(skill, 0x18, 0x20);
    skill[0x24] = 2;
    mem::put16(stats, 0x1C, 0);
    CHECK_EQ(f.unit.AddAilment(skill), 0);
    CHECK(f.unit.at<std::uint8_t*>(0x2584) == nullptr);
    mem::put16(stats, 0x1C, 10);
    CHECK_EQ(f.unit.AddAilment(skill), 1);
    std::uint8_t* node = f.unit.at<std::uint8_t*>(0x2584);
    CHECK(node != nullptr);
    CHECK_EQ(mem::u32(node, 0x9C), 0x20u);
    CHECK_EQ(mem::u32(node, 4), 0x2AF8u);
    CHECK_EQ(mem::u32(node, 0xC), 4u);
    CHECK_EQ(mem::u16(node, 0xA6), 9);
    CHECK_EQ(f.unit.at<std::uint32_t>(0x26C0), 0x20u);
    mem::put16(skill, 0x18, 0x2000);
    CHECK_EQ(f.unit.AddAilment(skill), 0);
    CHECK_EQ(f.unit.at<std::uint8_t>(0x26D0), 0x20);
    f.unit.ExpireEffects(1);
}
