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

namespace {

std::uint8_t* makeItem(std::uint16_t id, std::uint16_t count, std::uint32_t kind) {
    auto* item = static_cast<std::uint8_t*>(crt::malloc(0x60));
    std::memset(item, 0, 0x60);
    mem::put16(item, 4, id);
    mem::put16(item, 8, count);
    mem::put32(item, 0x2C, kind);
    item[0x46] = 1;
    std::strcpy(reinterpret_cast<char*>(item + 0xC), "item");
    return item;
}

}  // namespace

TEST_CASE("battle.hero items are counted, reserved and consumed") {
    HeroFixture f;
    std::uint8_t head[0x60] = {};
    std::uint8_t* first = makeItem(5, 1, 0x10);
    std::uint8_t* second = makeItem(6, 3, 0x10);
    mem::put16(second, 0x48, 0x402);
    mem::putPtr(head, 0, first);
    mem::putPtr(first, 0, second);
    f.hero.at<std::uint8_t*>(0x2ECC) = head;
    std::uint16_t count = 0;
    f.hero.CountItems(0, &count);
    CHECK_EQ(count, 2);
    char name[32] = {};
    std::int16_t flags = 0;
    CHECK_EQ(f.hero.GetUsableItem(0, 2, name, &flags, &count), 1);
    CHECK_EQ(count, 3);
    CHECK_EQ(static_cast<std::uint16_t>(flags), 0x8000);

    // the second item has two uses on its single count: the first consume only spends a use
    f.hero.at<std::uint8_t*>(0x2ED0) = second;
    mem::put16(second, 6, 1);
    CHECK_EQ(f.hero.ConsumeReservedItem(0, 0), 6);
    CHECK_EQ(mem::u16(second, 0x48), 0x401);
    CHECK_EQ(mem::u16(second, 8), 3);
    CHECK(f.hero.at<std::uint8_t*>(0x2ED0) == second);

    // the first item is used up, freed and unlinked
    f.hero.at<std::uint8_t*>(0x2ED0) = first;
    mem::put16(first, 6, 1);
    CHECK_EQ(f.hero.ConsumeReservedItem(0, 0), 5);
    CHECK(mem::ptr<std::uint8_t>(head, 0) == second);
    CHECK(f.hero.at<std::uint8_t*>(0x2ED0) == nullptr);
    crt::free(second);
}
