#include "battle/battle_hero.h"

#include <cstring>

#include "core/random.h"
#include "crt/crt.h"
#include "data/globals.h"

namespace swd3 {

using std::int16_t;
using std::int32_t;
using std::uint16_t;
using std::uint32_t;
using std::uint8_t;

// swd32.exe 0x0046E490: the hero stats block (+0) is a zeroed 0x38 byte copy
BattleHero::BattleHero() {
    at<uint16_t>(0x2F26) = 0;
    at<uint16_t>(0x2F18) = 0;
    auto* stats = static_cast<uint8_t*>(crt::malloc(0x38));
    at<uint8_t*>(0) = stats;
    std::memset(stats, 0, 0x38);
}

// swd32.exe 0x0046E090
int BattleHero::Flag2B00() { return at<int32_t>(0x2B00); }

// swd32.exe 0x0046E0A0
int BattleHero::Flag2B04() { return at<int32_t>(0x2B04); }

// swd32.exe 0x0046E520: fills the action gauge (+0x2A12) by a quarter of the speed (an eighth while slowed by
// status 0x40, +1/4 + 1 when `quick`), +30% with status 0x20000000, -30% with 0x8000000 and, while invisible
// (bit 31), minus (speed * +0x26DC / 100) / 2 + 4. Once full the turn starts: returns 1.
int BattleHero::StepGauge(int quick) {
    const uint16_t marks = at<uint16_t>(0x26D0);
    if ((marks & 0x40) != 0 || (marks & 0x4000) != 0) {
        return 0;
    }
    if (at<int32_t>(0x2AB8) == 1 || at<int32_t>(0x2AB0) == 1) {
        return 0;
    }
    const uint16_t gauge = at<uint16_t>(0x2A12);
    if (static_cast<int32_t>(gauge) >= G.gaugeFull) {
        at<int32_t>(0x2AB0) = 1;
        at<int32_t>(0x2AEC) = 0;
        if (at<int32_t>(0x2B20) == 1) {
            at<int32_t>(0x2B20) = 0;
            if (at<int32_t>(0x2B04) == 0) {
                at<int32_t>(0x2B08) = 0;
            }
        }
        at<int32_t>(0x2C4) = 0;
        at<int32_t>(0x2C8) = 0;
        at<int32_t>(0x45C) = 1;
        return 1;
    }
    int step = mem::u16(at<uint8_t*>(0), 0x16) >> 2;
    const uint32_t status = at<uint32_t>(0x26C0);
    if ((status & 0x40) != 0) {
        step >>= 1;
    }
    if (quick == 1) {
        step += step / 4 + 1;
    }
    int up = 0;
    int down = 0;
    if ((status & 0x20000000) != 0) {
        up = 30 * step / 100;
    }
    if ((status & 0x8000000) != 0) {
        down = 30 * step / 100;
    }
    if ((status & 0x80000000) != 0) {
        down += at<uint16_t>(0x26DC) * step / 100 / 2 + 4;
    }
    at<int32_t>(0x2AB0) = 0;
    at<uint16_t>(0x2A12) = static_cast<uint16_t>(gauge - down + up + step);
    return 0;
}

// swd32.exe 0x0046E6A0
int BattleHero::ResetHeroState() {
    at<uint16_t>(0x2F1E) = 0;
    at<uint16_t>(0x2F20) = 0;
    at<uint16_t>(0x2F22) = 0;
    at<int32_t>(0x2F0C) = 0;
    at<uint16_t>(0x2F10) = 0;
    at<uint16_t>(0x2F12) = 0;
    at<uint16_t>(0x2F1C) = 0;
    at<uint16_t>(0x2F1A) = 0;
    at<uint16_t>(0x2F24) = 0;
    at<uint16_t>(0x2F14) = 0;
    at<uint16_t>(0x2F18) = 0;
    at<uint16_t>(0x2F16) = 0;
    std::memset(raw(0x2BC8), 0, 0x2F8);
    std::memset(raw(0xAF0), 0, 0x130);
    std::memset(raw(0x2B24), 0, 0xA4);
    return 0;
}

// swd32.exe 0x0046E850
int BattleHero::SetBoth2EC8(int value) {
    at<int32_t>(0x2EC8) = value;
    at<int32_t>(0x2ECC) = value;
    return value;
}

// swd32.exe 0x0046E870
int BattleHero::SetBoth2EC0(int value) {
    at<int32_t>(0x2EC0) = value;
    at<int32_t>(0x2EC4) = value;
    return value;
}

namespace {

// progress entries (0x14 bytes: next, id, value, percent, done) of the two records (+0x19C max, +0x1A8 id)
uint8_t* findProgress(uint8_t*& entry, uint16_t id) {
    if (id == mem::u16(entry, 4)) {
        return entry;
    }
    for (uint8_t* next = mem::ptr<uint8_t>(entry, 0); next != nullptr; next = mem::ptr<uint8_t>(entry, 0)) {
        entry = next;
        if (id == mem::u16(entry, 4)) {
            return entry;
        }
    }
    return nullptr;
}

void raiseProgress(uint8_t* entry, uint16_t amount, uint16_t max) {
    if (mem::u16(entry, 0xA) != 0) {
        return;
    }
    mem::put16(entry, 6, static_cast<uint16_t>(mem::u16(entry, 6) + amount));
    if (mem::u16(entry, 6) >= max) {
        mem::put16(entry, 6, max);
    }
    const double percent = static_cast<double>(mem::u16(entry, 6)) / static_cast<double>(max) * 100.0;
    mem::put16(entry, 8, static_cast<uint16_t>(crt::ftol(percent)));
}

uint8_t* appendProgress(uint8_t* entry, uint16_t id, uint16_t amount, uint16_t max, uint8_t* list) {
    auto* added = static_cast<uint8_t*>(crt::malloc(0x14));
    mem::putPtr(entry, 0, added);
    std::memset(added, 0, 0x14);
    mem::put16(added, 4, id);
    mem::put16(added, 6, amount);
    const double percent = static_cast<double>(amount) / static_cast<double>(max) * 100.0;
    mem::put16(added, 8, static_cast<uint16_t>(crt::ftol(percent)));
    ++mem::ref<uint16_t>(list, 4);
    return added;
}

}  // namespace

// swd32.exe 0x0046F5B0: as BattleUnit::AddRecordProgress, but every record searches from the list itself
int BattleHero::AddRecordProgress(uint8_t* list, uint16_t amount) {
    int any = 0;
    for (int record = 0; record < 2; ++record) {
        const uint8_t* fields = raw(0x19C + 0xA4 * record);
        const uint16_t id = mem::u16(fields, 0xC);
        if (id == 0) {
            continue;
        }
        uint8_t* entry = list;
        any = 1;
        if (uint8_t* found = findProgress(entry, id)) {
            raiseProgress(found, amount, mem::u16(fields, 0));
        } else {
            appendProgress(entry, id, amount, mem::u16(fields, 0), list);
        }
    }
    return any;
}

// swd32.exe 0x0046F6E0: records of type 0x33 gain 12 points for items with +0x10 in 1..9 (the original also
// checks the address of the item's slot byte at +0x1E9 of the record against null)
int BattleHero::AddFixedRecordProgress(uint8_t* list, const uint8_t* item) {
    int any = 0;
    for (int record = 0; record < 2; ++record) {
        const uint8_t* fields = raw(0x19C + 0xA4 * record);
        const uint16_t id = mem::u16(fields, 0xC);
        if (id == 0 || mem::u16(fields, 4) != 0x33) {
            continue;
        }
        const uint16_t slot = mem::u16(item, 0x10);
        if (slot < 1 || slot > 9) {
            continue;
        }
        uint8_t* entry = list;
        any = 1;
        if (uint8_t* found = findProgress(entry, id)) {
            raiseProgress(found, 0xC, mem::u16(fields, 0));
        } else {
            appendProgress(entry, id, 0xC, mem::u16(fields, 0), list);
        }
    }
    return any;
}

// swd32.exe 0x0046F850: completes the first record whose entry reached its maximum; returns its id
uint32_t BattleHero::CompleteRecord(uint8_t* list) {
    uint32_t result = 0;
    bool done = false;
    for (int record = 0; record < 2; ++record) {
        const uint8_t* fields = raw(0x19C + 0xA4 * record);
        const uint16_t id = mem::u16(fields, 0xC);
        if (id == 0) {
            continue;
        }
        if (done) {
            return result;
        }
        for (uint8_t* entry = list; entry != nullptr; entry = mem::ptr<uint8_t>(entry, 0)) {
            if (id == mem::u16(entry, 4) && mem::u16(entry, 0xA) == 0 && mem::u16(entry, 6) >= mem::u16(fields, 0)) {
                done = true;
                mem::put16(entry, 6, mem::u16(fields, 0));
                mem::put16(entry, 0xA, 1);
                result = mem::u16(fields, 0xC);
                break;
            }
        }
    }
    return result;
}

// swd32.exe 0x0046FF00: whether the hero strikes first (+0x2F30): always with status bit 0 of +0x26C8, after
// G.dword_53BCE4 + 8 counted turns (low byte of G.dword_53CEB4), or past 8 turns on a roll above 5 of 10;
// never twice in a row for the same hero (+0xD64 kept in G.dword_53CEBC)
int BattleHero::TryFirstStrike() {
    if (at<int32_t>(0x2B00) == 1 || at<int32_t>(0x2B04) == 1 || at<int32_t>(0x26CC) != 0 ||
        static_cast<uint32_t>(G.dword_53CEBC) == at<uint16_t>(0xD64)) {
        return 0;
    }
    if ((at<uint8_t>(0x26C8) & 1) != 0) {
        at<uint8_t>(0x2F30) = 1;
        return 1;
    }
    auto& counter = *reinterpret_cast<uint8_t*>(&G.dword_53CEB4);
    if (counter >= static_cast<uint32_t>(G.dword_53BCE4 + 8)) {
        at<uint8_t>(0x2F30) = 1;
        counter = 0;
        G.dword_53CEBC = at<uint16_t>(0xD64);
        return 1;
    }
    if (counter < 8 || static_cast<uint16_t>(R250Below(10)) <= 5) {
        return 0;
    }
    at<uint8_t>(0x2F30) = 1;
    counter = 0;
    G.dword_53CEBC = at<uint16_t>(0xD64);
    return 1;
}

// swd32.exe 0x0046FFE0
int BattleHero::Restore2EC0() {
    const int32_t value = at<int32_t>(0x2EC4);
    at<int32_t>(0x2EC0) = value;
    return value;
}

// swd32.exe 0x00470890: takes the pending cost (+0x2F12) from the MP (+8 of the hero data), not below 0
void BattleHero::ApplyPendingMpCost() {
    const uint16_t cost = at<uint16_t>(0x2F12);
    if (cost == 0) {
        return;
    }
    uint8_t* data = at<uint8_t*>(4);
    mem::put16(data, 8, static_cast<uint16_t>(mem::u16(data, 8) - cost));
    if (mem::s16(at<uint8_t*>(4), 8) < 0) {
        mem::put16(at<uint8_t*>(4), 8, 0);
    }
    at<uint16_t>(0x2F12) = 0;
}

// swd32.exe 0x004708C0
int BattleHero::MarkGuarded() {
    const uint16_t action = at<uint16_t>(0x2F18);
    at<uint8_t>(0x2A87) |= 2;
    at<int32_t>(0x2B0C) = 1;
    if (action != 0) {
        at<uint16_t>(0x2A6C) = action;
    }
    std::memset(raw(0xAF0), 0, 0x130);
    return 0;
}

// swd32.exe 0x00470900
int BattleHero::Restore2EC8() {
    const int32_t value = at<int32_t>(0x2ECC);
    at<int32_t>(0x2EC8) = value;
    return value;
}

// swd32.exe 0x004717D0
int BattleHero::ActionFlag13() { return (at<uint16_t>(0x2A86) >> 13) & 1; }

// swd32.exe 0x004717E0
int BattleHero::ActionFlag12() { return (at<uint16_t>(0x2A86) >> 12) & 1; }

// swd32.exe 0x00472710: the return value of GetData0C is dropped
int GetEnemyDataAlways(BattleUnit* unit) {
    unit->GetData0C();
    return 1;
}

// swd32.exe 0x004728D0
uint32_t BattleHero::Mark800Clear() { return (static_cast<uint32_t>(~at<uint16_t>(0x26D0)) >> 11) & 1; }

// swd32.exe 0x00474B60: roll of 0..99 against 70% of `percent` + 10
bool RollChance(int percent) {
    const auto roll = static_cast<uint16_t>(R250Below(100));
    return static_cast<uint16_t>(70 * percent / 100 + 10) >= roll;
}

// swd32.exe 0x004750C0: clears the chosen action; a reserved item (+0x2ED0) gives its count (+6 of +0x2EC8) back.
// The return value is unused by callers.
int BattleHero::ClearActionState() {
    at<uint16_t>(0x2A86) = 0;
    std::memset(raw(0xD90), 0, 0x28);
    at<int32_t>(0x2630) = 0;
    at<int32_t>(0x2634) = 0;
    at<int32_t>(0x2638) = 0;
    at<int32_t>(0x263C) = 0;
    const int32_t reserved = at<int32_t>(0x2ED0);
    at<uint16_t>(0x2F10) = 0;
    at<uint16_t>(0x2F12) = 0;
    at<uint16_t>(0x2A8A) = 0;
    at<uint16_t>(0x2F16) = 0;
    at<uint16_t>(0x2F18) = 0;
    at<uint16_t>(0x29A6) = 0;
    at<uint16_t>(0x29A8) = 0;
    at<uint16_t>(0x29AA) = 0;
    at<uint16_t>(0x29A4) = 0;
    at<uint16_t>(0x2F14) = 0;
    if (reserved == 0) {
        return 0;
    }
    uint8_t* item = at<uint8_t*>(0x2EC8);
    const uint16_t count = mem::u16(item, 6);
    if (count != 0) {
        mem::put16(item, 6, static_cast<uint16_t>(count - 1));
    }
    at<int32_t>(0x2ED0) = 0;
    return 0;
}

}  // namespace swd3
