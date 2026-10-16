#include "battle/battle_hero.h"

#include <cstring>

#include "core/random.h"
#include "crt/crt.h"
#include "audio/audio.h"
#include "data/globals.h"
#include "draw/draw.h"
#include "pic/projector.h"
#include "pic/rle.h"
#include "res/act_anim.h"
#include "res/tsw.h"
#include "swd3/w32.h"

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

// swd32.exe 0x0046E4D0
BattleHero::~BattleHero() { Release(); }

// swd32.exe 0x00475180: frees the buffer at +0x2BC4 and the stats block; the return value is unused by callers
int BattleHero::Release() {
    if (at<uint8_t*>(0x2BC4) != nullptr) {
        crt::free(at<uint8_t*>(0x2BC4));
        at<uint8_t*>(0x2BC4) = nullptr;
    }
    if (at<uint8_t*>(0) != nullptr) {
        crt::free(at<uint8_t*>(0));
        at<uint8_t*>(0) = nullptr;
    }
    return 0;
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

// swd32.exe 0x004710D0: prepares the explosion of `target` (effect at +0xE14): the decoded current picture of the
// target flies apart into a point in front of the hero; the projector covers the screen
int BattleHero::StartExplosion(BattleUnit* target) {
    at<Picture*>(0x255C) = reinterpret_cast<Picture*>(static_cast<std::uintptr_t>(target->StepIdleAction()));
    uint32_t originX = 0;
    uint16_t originY = 0;
    target->GetOrigin(reinterpret_cast<uint16_t*>(&originX), &originY);
    auto* effect = reinterpret_cast<ParticleEffect*>(raw(0xE14));
    std::memset(effect, 0, sizeof(ParticleEffect));
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t format = 0;
    Picture* picture = at<Picture*>(0x255C);
    effect->pixels = static_cast<uint16_t*>(
        RleDecode(static_cast<const uint16_t*>(picture->pixels), &width, &height, &format));
    picture = at<Picture*>(0x255C);
    const int32_t lift = at<int32_t>(0x2B4);
    effect->width = picture->width;
    effect->height = picture->height;
    effect->x = static_cast<int16_t>(originX) - 1;
    effect->y = static_cast<int16_t>(originY);
    effect->speed = 0x1E;
    effect->lifeBase = 0x14;
    effect->targetLeft = at<int16_t>(0x29AC) - at<int16_t>(0x29B2) + at<int16_t>(0xD66) - 0x32;
    effect->targetTop = at<int16_t>(0xD68) - lift + 0x28;
    effect->targetWidth = 1;
    effect->targetHeight = 1;
    const uint16_t pictureHeight = picture->height;
    effect->bursts = pictureHeight >= 100 ? static_cast<uint16_t>(pictureHeight >> 1)
                                          : static_cast<uint16_t>(pictureHeight - 10);
    effect->divisor = 0x28;
    effect->tint[0] = 5;
    effect->tint[1] = 5;
    effect->tint[2] = 5;
    effect->flags = 0x56;
    if (target->Mirrored() == 1) {
        effect->flags |= 1;
    }
    at<uint8_t>(0x2A87) |= 8;
    const int screenHeight = w32::GetSystemMetrics(1);
    const int screenWidth = w32::GetSystemMetrics(0);
    G.projector.SetScreen(screenWidth, screenHeight);
    at<int32_t>(0x2680) = 0;
    std::memset(raw(0xDF4), 0, 0x20);
    std::memset(raw(0x500), 0, sizeof(ActState));
    std::memset(raw(0x2BC8), 0, 0x2F8);
    return 0;
}

// swd32.exe 0x004714B0: chance against `level` from the hero level (+0x2C of the stats): above it 35%, 1..7
// below it 70%, 8..12 below 90%, 13 or more always; equal levels and level 0 never
bool BattleHero::RollLevelChance(uint16_t level) {
    if (level == 0) {
        return false;
    }
    const uint8_t own = at<uint8_t*>(0)[0x2C];
    if (own < level) {
        return static_cast<int>(R250Below(100)) <= 0x23;
    }
    const int difference = own - level;
    if (difference < 8) {
        if (difference >= 1) {
            return static_cast<int>(R250Below(100)) <= 0x46;
        }
        return difference >= 0xD;
    }
    if (difference < 0xD) {
        return static_cast<int>(R250Below(100)) <= 0x5A;
    }
    return true;
}

// swd32.exe 0x00471540: recoil (action 0x2A of +0x2A0C, state +0x468) drawn while the level +0x2668 counts down
// from 15 to 6 (`mode` 1, which also steps the hero 16 pixels back with sound 0x2F) or 2; held heroes only count.
// Returns 1 when done.
int BattleHero::DrawRecoil(int mode) {
    if (at<int32_t>(0x2AB8) == 1) {
        at<int32_t>(0x2AAC) = 0;
        return 1;
    }
    at<int16_t>(0x2958) = 6;
    if (mode == 0) {
        at<int16_t>(0x2958) = 2;
    }
    if (IsHeld() == 1) {
        const int32_t level = at<int32_t>(0x2668);
        if (level <= at<int16_t>(0x2958)) {
            at<int32_t>(0x2668) = 0xF;
            return 1;
        }
        at<int32_t>(0x2668) = level - 1;
        return 0;
    }
    if (at<int32_t>(0x2668) <= at<int16_t>(0x2958)) {
        std::memset(raw(0x468), 0, sizeof(ActState));
        at<int32_t>(0x2668) = 0xF;
        if (mode == 1) {
            at<int32_t>(0x2AAC) = 1;
        }
        return 1;
    }
    auto* state = reinterpret_cast<ActState*>(raw(0x468));
    at<int32_t>(0x2AAC) = 1;
    at<int32_t>(0x468) = at<uint16_t>(0x2A0C);
    at<int32_t>(0x470) = 0x2A;
    at<int32_t>(0x4F8) = 0;
    if (at<int32_t>(0x2AD0) == 1) {
        at<int32_t>(0x4F8) = 1;
    }
    if (ActRun(state) == 0) {
        return 1;
    }
    Picture* picture = TswGetPicture(at<uint16_t>(0x4B2), at<uint16_t>(0x4B4));
    at<uint16_t>(0x29B4) = at<uint16_t>(0x478);
    at<Picture*>(0x254C) = picture;
    const uint32_t flags = at<uint32_t>(0x480);
    at<uint32_t>(0x26A0) = (flags & 1) != 0 ? flags & ~1u : flags | 1;
    if (at<int32_t>(0x2B08) == 1) {
        const uint32_t mirrored = at<uint32_t>(0x26A0);
        at<uint32_t>(0x26A0) = (mirrored & 1) != 0 ? mirrored & ~1u : mirrored | 1;
        at<uint16_t>(0x29B4) = static_cast<uint16_t>(picture->width - at<uint16_t>(0x478));
    }
    if (at<int32_t>(0x2668) == 0xF && mode == 1) {
        at<uint16_t>(0x4C0) = 0x2F;
        PlaySound(0x2F, G.soundVolume);
        SetSoundPan(at<uint16_t>(0x4C0), at<int32_t>(0x2B08) == 1 ? -0x10 : 0x10);
        at<uint16_t>(0x4C0) = 0;
    }
    uint32_t x = static_cast<uint32_t>(mode);
    uint16_t y = 0;
    GetPosition(reinterpret_cast<uint16_t*>(&x), &y);
    if (mode == 1) {
        x = at<int32_t>(0x2B08) == 1 ? x - 0x10 : x + 0x10;
    }
    SetPosition(static_cast<int16_t>(x), static_cast<int16_t>(y));
    Picture* current = at<Picture*>(0x254C);
    G.drawSource = reinterpret_cast<uint8_t*>(current->pixels);
    DrawPicture(at<int16_t>(0xD66) - at<int16_t>(0x29B4), at<int16_t>(0xD68) - at<int32_t>(0x47C), current->width,
                current->height, static_cast<int>(at<uint32_t>(0x26A0)),
                static_cast<int>(reinterpret_cast<std::uintptr_t>(current->reserved4)));
    --at<int32_t>(0x2668);
    return 0;
}

// swd32.exe 0x004717F0: leap (act 0x186B, state +0x500) along a line to `target` (its offset point, or its draw
// position when the offset has a zero coordinate), 8 more steps every frame. Returns 1 on arrival.
int BattleHero::DrawLeap(BattleUnit* target) {
    auto* state = reinterpret_cast<ActState*>(raw(0x500));
    uint32_t targetX = 0;
    uint32_t targetY = 0;
    uint32_t originX = 0;
    uint32_t originY = 0;
    at<int32_t>(0x500) = 0x186B;
    at<int32_t>(0x508) = 0;
    at<int32_t>(0x590) = 0;
    if (at<int32_t>(0x2AD0) == 1) {
        at<int32_t>(0x590) = 1;
    }
    if (ActRun(state) == 0) {
        return 0;
    }
    Picture* picture = TswGetPicture(at<uint16_t>(0x54A), at<uint16_t>(0x54C));
    at<Picture*>(0x254C) = picture;
    G.drawSource = reinterpret_cast<uint8_t*>(picture->pixels);
    uint32_t flags = at<uint32_t>(0x518);
    at<uint16_t>(0x29B4) = at<uint16_t>(0x510);
    at<uint32_t>(0x26A0) = flags;
    if (at<int32_t>(0x2B08) == 1) {
        flags = (flags & 1) != 0 ? flags & ~1u : flags | 1;
        at<uint32_t>(0x26A0) = flags;
        at<uint16_t>(0x29B4) = static_cast<uint16_t>(at<Picture*>(0x254C)->width - at<uint16_t>(0x510));
    }
    target->GetOffset(reinterpret_cast<uint16_t*>(&targetX), reinterpret_cast<uint16_t*>(&targetY));
    uint16_t endX;
    uint16_t endY;
    if (static_cast<uint16_t>(targetX) == 0 || static_cast<uint16_t>(targetY) == 0) {
        target->GetDrawPosition(reinterpret_cast<uint16_t*>(&targetX), reinterpret_cast<uint16_t*>(&targetY));
        endX = static_cast<uint16_t>(targetX - at<uint16_t>(0x29B4));
        endY = static_cast<uint16_t>(targetY - static_cast<uint16_t>(at<int32_t>(0x514)));
    } else {
        target->GetOrigin(reinterpret_cast<uint16_t*>(&originX), reinterpret_cast<uint16_t*>(&originY));
        endX = static_cast<uint16_t>(originX + static_cast<uint16_t>(targetX - at<uint16_t>(0x29B4)));
        endY = static_cast<uint16_t>(originY +
                                     static_cast<uint16_t>(targetY - static_cast<uint16_t>(at<int32_t>(0x514))));
    }
    auto* walker = reinterpret_cast<LineWalker*>(raw(0xDF4));
    std::memset(walker, 0, 0x20);
    walker->fromX = at<int16_t>(0x29AC) - at<int16_t>(0x29B2) + at<int16_t>(0xD66) - at<int16_t>(0x29B4);
    walker->fromY = at<int16_t>(0xD68) + at<int16_t>(0x318) - at<int32_t>(0x2B4) - at<int32_t>(0x514);
    walker->toX = static_cast<int16_t>(endX);
    walker->toY = static_cast<int16_t>(endY);
    int steps = 0;
    bool arrived = false;
    for (; steps < 8 * at<int32_t>(0x2680); ++steps) {
        LineStep(walker);
        if (walker->fromX + walker->x == walker->toX) {
            arrived = true;
            break;
        }
    }
    ++at<int32_t>(0x2680);
    PlaySound(at<uint16_t>(0x558), G.soundVolume);
    const uint32_t drawFlags = at<uint32_t>(0x26A0);
    Picture* current = at<Picture*>(0x254C);
    at<uint16_t>(0x558) = 0;
    if (arrived) {
        DrawPicture(static_cast<int16_t>(endX), static_cast<int16_t>(endY), current->width, current->height,
                    static_cast<int>(drawFlags), 0);
        at<int32_t>(0x2680) = 0;
        std::memset(walker, 0, 0x20);
        std::memset(state, 0, sizeof(ActState));
        return 1;
    }
    DrawPicture(walker->fromX + walker->x, walker->fromY + walker->y, current->width, current->height,
                static_cast<int>(drawFlags), 0);
    return 0;
}

// swd32.exe 0x00471AD0: leap back (act 0x186B, entry 1) from `target` to the hero's place, 8 more steps every
// frame. Returns 1 on arrival.
int BattleHero::DrawLeapBack(BattleUnit* target) {
    auto* state = reinterpret_cast<ActState*>(raw(0x500));
    uint32_t targetX = 0;
    uint32_t targetY = 0;
    uint32_t originX = 0;
    uint32_t originY = 0;
    at<int32_t>(0x500) = 0x186B;
    at<int32_t>(0x508) = 1;
    at<int32_t>(0x590) = 0;
    if (at<int32_t>(0x2AD0) == 1) {
        at<int32_t>(0x590) = 1;
    }
    if (ActRun(state) == 0) {
        return 0;
    }
    Picture* picture = TswGetPicture(at<uint16_t>(0x54A), at<uint16_t>(0x54C));
    at<Picture*>(0x254C) = picture;
    G.drawSource = reinterpret_cast<uint8_t*>(picture->pixels);
    target->GetOffset(reinterpret_cast<uint16_t*>(&targetX), reinterpret_cast<uint16_t*>(&targetY));
    uint16_t startX;
    uint16_t startY;
    if (static_cast<uint16_t>(targetX) != 0 && static_cast<uint16_t>(targetY) != 0) {
        target->GetOrigin(reinterpret_cast<uint16_t*>(&originX), reinterpret_cast<uint16_t*>(&originY));
        startX = static_cast<uint16_t>(originX + static_cast<uint16_t>(targetX - static_cast<uint16_t>(at<int32_t>(0x510))));
        startY = static_cast<uint16_t>(originY + static_cast<uint16_t>(targetY - static_cast<uint16_t>(at<int32_t>(0x514))));
    } else {
        target->GetDrawPosition(reinterpret_cast<uint16_t*>(&targetX), reinterpret_cast<uint16_t*>(&targetY));
        startX = static_cast<uint16_t>(targetX - static_cast<uint16_t>(at<int32_t>(0x510)));
        startY = static_cast<uint16_t>(targetY - static_cast<uint16_t>(at<int32_t>(0x514)));
    }
    auto* walker = reinterpret_cast<LineWalker*>(raw(0xDF4));
    std::memset(walker, 0, 0x20);
    walker->fromX = static_cast<int16_t>(startX);
    walker->fromY = static_cast<int16_t>(startY);
    walker->toX = at<int16_t>(0x316) + at<int16_t>(0xD66) - at<int16_t>(0x29B2) - at<int32_t>(0x510);
    walker->toY = at<int16_t>(0x318) + at<int16_t>(0xD68) - at<int32_t>(0x2B4) - at<int32_t>(0x514);
    bool arrived = false;
    for (int steps = 0; steps < 8 * at<int32_t>(0x2680); ++steps) {
        LineStep(walker);
        if (walker->x + walker->fromX == walker->toX) {
            arrived = true;
            break;
        }
    }
    ++at<int32_t>(0x2680);
    PlaySound(at<uint16_t>(0x558), G.soundVolume);
    const uint32_t flags = at<uint32_t>(0x518);
    Picture* current = at<Picture*>(0x254C);
    at<uint16_t>(0x558) = 0;
    if (arrived) {
        DrawPicture(walker->toX, walker->toY, current->width, current->height, static_cast<int>(flags), 0);
        at<int32_t>(0x2680) = 0;
        std::memset(walker, 0, 0x20);
        std::memset(state, 0, sizeof(ActState));
        return 1;
    }
    DrawPicture(walker->fromX + walker->x, walker->fromY + walker->y, current->width, current->height,
                static_cast<int>(flags), 0);
    return 0;
}

// swd32.exe 0x00471D60: the action +0x2F14 (entry 0x24) drawn at (x, y) brightening by 2 per frame up to 62, then
// darkening back (counting +0x2F26) to 0, which plays sound 0x6A and returns 1
int BattleHero::DrawFlicker(int x, int y) {
    auto* state = reinterpret_cast<ActState*>(raw(0x500));
    at<int32_t>(0x500) = at<uint16_t>(0x2F14);
    at<int32_t>(0x508) = 0x24;
    at<int32_t>(0x590) = 0;
    if (at<int32_t>(0x2AD0) == 1) {
        at<int32_t>(0x590) = 1;
    }
    if (ActRun(state) == 0) {
        return 0;
    }
    at<Picture*>(0x254C) = TswGetPicture(at<uint16_t>(0x54A), at<uint16_t>(0x54C));
    PlaySound(at<uint16_t>(0x558), G.soundVolume);
    uint32_t flags = at<uint32_t>(0x518);
    const int32_t originX = at<int32_t>(0x510);
    at<uint16_t>(0x4C0) = 0;
    at<uint32_t>(0x2688) = flags;
    at<int32_t>(0x268C) = originX;
    if (at<int32_t>(0x2B08) == 0) {
        flags = (flags & 1) != 0 ? flags & ~1u : flags | 1;
        at<uint32_t>(0x2688) = flags;
        at<int32_t>(0x268C) = at<Picture*>(0x254C)->width - originX;
    }
    auto shade = [this]() {
        const int level = at<int16_t>(0x2958) / 2 - 0x1F;
        G.drawOption71C = level;
        G.drawOption30C = level;
        G.drawOption304 = level;
    };
    if (at<uint16_t>(0x2A66) == 0) {
        shade();
        at<int16_t>(0x2958) = static_cast<int16_t>(at<int16_t>(0x2958) + 2);
        if (at<int16_t>(0x2958) > 0x3E) {
            at<uint16_t>(0x2A66) = 1;
        }
    }
    if (at<uint16_t>(0x2A66) == 1) {
        shade();
        at<int16_t>(0x2958) = static_cast<int16_t>(at<int16_t>(0x2958) - 2);
        ++at<uint16_t>(0x2F26);
        if (at<int16_t>(0x2958) <= 0) {
            at<uint16_t>(0x2A66) = 2;
        }
    }
    Picture* picture = at<Picture*>(0x254C);
    G.drawSource = reinterpret_cast<uint8_t*>(picture->pixels);
    DrawPicture(x - at<int32_t>(0x268C), y - at<int32_t>(0x514), picture->width, picture->height,
                static_cast<int>(at<uint32_t>(0x2688) | 4), 0);
    if (at<uint16_t>(0x2A66) != 2) {
        return 0;
    }
    PlaySound(0x6A, G.soundVolume);
    at<int16_t>(0x2958) = 0;
    at<int32_t>(0x2688) = 0;
    at<int32_t>(0x268C) = 0;
    at<uint16_t>(0x2A66) = 0;
    at<uint16_t>(0x2F26) = 0;
    at<uint16_t>(0x2A78) = 0;
    std::memset(state, 0, sizeof(ActState));
    std::memset(raw(0x2BC8), 0, 0x2F8);
    return 1;
}

// swd32.exe 0x00471FC0: projectile `slot` (state +0x2BC8 + 0x98 * slot, act `id` entry `action`) flying from
// (x, y) back to the hero, `speed` times its frame count (+0x2EF8) line steps per frame. Returns 1 on arrival.
int BattleHero::DrawProjectile(int id, int action, int slot, int16_t x, int16_t y, int speed) {
    uint8_t* bytes = raw(0x2BC8 + 0x98 * slot);
    auto* state = reinterpret_cast<ActState*>(bytes);
    int32_t& frames = at<int32_t>(0x2EF8 + 4 * slot);
    state->id = id;
    state->action = action;
    mem::put32(bytes, 0x90, 0);
    if (at<int32_t>(0x2AD0) == 1) {
        mem::put32(bytes, 0x90, 1);
    }
    if (ActRun(state) == 0) {
        return 0;
    }
    Picture* picture = TswGetPicture(state->fr, state->ap);
    at<Picture*>(0x254C) = picture;
    G.drawSource = reinterpret_cast<uint8_t*>(picture->pixels);
    const auto startY = static_cast<int16_t>(y - static_cast<uint16_t>(state->y));
    const auto startX = static_cast<int16_t>(x - static_cast<uint16_t>(state->x));
    auto* walker = reinterpret_cast<LineWalker*>(raw(0xDF4));
    std::memset(walker, 0, 0x20);
    walker->fromX = startX;
    walker->fromY = startY;
    walker->toX = at<int16_t>(0xD66) - at<int16_t>(0x29B2) + at<int16_t>(0x316) - state->x;
    walker->toY = at<int16_t>(0xD68) + at<int16_t>(0x318) - at<int32_t>(0x2B4) - state->y;
    for (int steps = 0; steps < frames * speed; ++steps) {
        LineStep(walker);
    }
    ++frames;
    PlaySound(mem::u16(bytes, 0x58), G.soundVolume);
    mem::put16(bytes, 0x58, 0);
    const int endX = walker->toX - at<int32_t>(0x2674);
    const int currentX = walker->x + walker->fromX;
    Picture* current = at<Picture*>(0x254C);
    if (currentX >= endX) {
        DrawPicture(endX, walker->toY, current->width, current->height, static_cast<int>(state->flags), 0);
        frames = 0;
        std::memset(walker, 0, 0x20);
        return 1;
    }
    DrawPicture(currentX, walker->fromY + walker->y, current->width, current->height, static_cast<int>(state->flags),
                0);
    return 0;
}

// swd32.exe 0x004721F0: the hero's stand action (0x2B of +0x2A0C, state +0x338) drawn with shadow at the draw
// position of `target`, mirrored once for mirrored heroes (bit 15 of the stored flags marks it). Returns 1 when
// the action finished.
int BattleHero::DrawStandAt(BattleUnit* target) {
    auto* state = reinterpret_cast<ActState*>(raw(0x338));
    at<int32_t>(0x340) = 0x2B;
    at<int32_t>(0x3C8) = 0;
    at<int32_t>(0x338) = at<uint16_t>(0x2A0C);
    if (at<int32_t>(0x2AD0) == 1) {
        at<int32_t>(0x3C8) = 1;
    }
    const int running = ActRun(state);
    if (running == 0) {
        return running;
    }
    Picture* picture = TswGetPicture(at<uint16_t>(0x382), at<uint16_t>(0x384));
    at<Picture*>(0x254C) = picture;
    G.drawSource = reinterpret_cast<uint8_t*>(picture->pixels);
    if (at<int32_t>(0x3C4) != 0) {
        std::memset(state, 0, sizeof(ActState));
        return 1;
    }
    if (at<int32_t>(0x2B08) == 1 && at<int32_t>(0x354) == 0) {
        uint32_t flags = at<uint32_t>(0x350);
        flags = (flags & 1) != 0 ? flags & ~1u : flags | 1;
        at<uint32_t>(0x350) = flags;
        at<uint32_t>(0x354) = flags;
        at<int32_t>(0x348) = at<Picture*>(0x254C)->width - at<int32_t>(0x348);
        at<uint32_t>(0x354) = flags | 0x8000;
    }
    uint16_t targetX = 0;
    uint16_t targetY = 0;
    target->GetDrawPosition(&targetX, &targetY);
    const auto x = static_cast<int16_t>(targetX - static_cast<uint16_t>(at<int32_t>(0x348)));
    const auto y = static_cast<int16_t>(targetY - static_cast<uint16_t>(at<int32_t>(0x34C)));
    Picture* current = at<Picture*>(0x254C);
    G.drawScaleHeight = current->height / 3;
    G.drawOption304 = -6;
    G.drawShear = current->height >> 2;
    G.drawOption30C = -6;
    G.drawOption71C = -6;
    if (at<int32_t>(0x2B00) == 1) {
        G.drawOption304 = -1;
        G.drawOption30C = -1;
        G.drawOption71C = -1;
    }
    PlaySound(at<uint16_t>(0x390), G.soundVolume);
    SetSoundPan(at<uint16_t>(0x390), x <= 0x140 ? -0x10 : 0x10);
    const uint32_t shadowFlags = (at<uint32_t>(0x350) & 0x8000000F) | 0xC;
    at<uint32_t>(0x26A4) = shadowFlags;
    at<uint16_t>(0x390) = 0;
    current = at<Picture*>(0x254C);
    DrawPicture(x - 5, y - G.drawScaleHeight + at<int32_t>(0x34C), current->width, current->height,
                static_cast<int>(shadowFlags), 0);
    current = at<Picture*>(0x254C);
    DrawPicture(x, y, current->width, current->height, static_cast<int>(at<uint32_t>(0x350)), 0);
    return 0;
}

namespace {

void standShadow(BattleHero& hero, int x, uint32_t flags) {
    Picture* picture = hero.at<Picture*>(0x254C);
    const uint32_t shadowFlags = (flags & 0x8000000F) | 0xC;
    hero.at<uint32_t>(0x26A4) = shadowFlags;
    DrawPicture(x, hero.at<int16_t>(0xD68) - G.drawScaleHeight, picture->width, picture->height,
                static_cast<int>(shadowFlags), 0);
}

void standOptions(BattleHero& hero, bool ghost) {
    Picture* picture = hero.at<Picture*>(0x254C);
    G.drawScaleHeight = picture->height / 3;
    G.drawShear = picture->height >> 2;
    G.drawOption304 = -6;
    G.drawOption30C = -6;
    G.drawOption71C = -6;
    if (ghost) {
        G.drawOption304 = -1;
        G.drawOption30C = -1;
        G.drawOption71C = -1;
    }
}

}  // namespace

// swd32.exe 0x004724D0: casting pose (0x28 of +0x2A0C, state +0x338) with shadow at the unmirrored origin +0x348
// (mirroring only updates +0x29B4 / +0x26A0). On a frame with flags 9 the cast act +0xDA4 is returned with bit 15
// set; 2 when the pose ended, else 0. `x` and `y` are unused.
uint16_t BattleHero::DrawCastPose(int x, int y) {
    static_cast<void>(x);
    static_cast<void>(y);
    auto* state = reinterpret_cast<ActState*>(raw(0x338));
    at<int32_t>(0x338) = at<uint16_t>(0x2A0C);
    at<int32_t>(0x2AAC) = 1;
    at<int32_t>(0x340) = 0x28;
    at<int32_t>(0x3C8) = 0;
    if (at<int32_t>(0x2AD0) == 1) {
        at<int32_t>(0x3C8) = 1;
    }
    if (ActRun(state) == 0) {
        return 0;
    }
    Picture* picture = TswGetPicture(at<uint16_t>(0x382), at<uint16_t>(0x384));
    at<Picture*>(0x254C) = picture;
    G.drawSource = reinterpret_cast<uint8_t*>(picture->pixels);
    uint32_t flags = at<uint32_t>(0x350);
    at<uint16_t>(0x29B4) = at<uint16_t>(0x348);
    const uint16_t overlay = at<uint16_t>(0x3AE);
    at<uint32_t>(0x26A0) = flags;
    at<uint16_t>(0x29AC) = overlay;
    if (at<int32_t>(0x2B08) == 1) {
        flags = (flags & 1) != 0 ? flags & ~1u : flags | 1;
        at<uint32_t>(0x26A0) = flags;
        Picture* current = at<Picture*>(0x254C);
        at<uint16_t>(0x29B4) = static_cast<uint16_t>(current->width - at<uint16_t>(0x348));
        if (overlay != 0) {
            at<uint16_t>(0x29AC) = static_cast<uint16_t>(current->width - overlay);
        }
    }
    Picture* current = at<Picture*>(0x254C);
    G.drawScaleHeight = current->height / 3;
    G.drawOption304 = -6;
    G.drawOption30C = -6;
    G.drawOption71C = -6;
    G.drawShear = current->height >> 2;
    PlaySound(at<uint16_t>(0x390), G.soundVolume);
    SetSoundPan(at<uint16_t>(0x390), -0x10);
    at<uint16_t>(0x390) = 0;
    const int drawX = at<int16_t>(0xD66) - at<int32_t>(0x348);
    standShadow(*this, drawX, at<uint32_t>(0x350));
    current = at<Picture*>(0x254C);
    DrawPicture(drawX, at<int16_t>(0xD68) - at<int32_t>(0x34C), current->width, current->height,
                static_cast<int>(at<uint32_t>(0x350)), 0);
    if ((at<uint8_t>(0x392) & 9) != 0) {
        const uint16_t cast = at<uint16_t>(0xDA4);
        at<uint16_t>(0x392) = 0;
        return static_cast<uint16_t>(cast | 0x8000);
    }
    if (at<int32_t>(0x3C4) != 1) {
        return 0;
    }
    at<int32_t>(0x2AAC) = 0;
    std::memset(state, 0, sizeof(ActState));
    return 2;
}

// swd32.exe 0x00472730: whether `target` can be caught: enemies with flag 0x800 (not 0x20) up to level 0x15,
// by the level difference to the hero: 10+ below always, 7..11 above when at a quarter of its hit points, 5..9
// below at a third or 80%, 1..6 above at a third or 20 - 5 * difference %, 0..4 below at a third or
// 20 + 10 * difference %
bool BattleHero::CanCatch(BattleUnit* target) {
    auto* data = reinterpret_cast<uint8_t*>(static_cast<std::uintptr_t>(target->GetData0C()));
    int32_t current = 0;
    int32_t maximum = 0;
    target->GetHitPoints(&current, &maximum);
    const uint32_t flags = mem::u32(data, 0x20);
    if ((flags & 0x20) != 0 || (flags & 0x800) == 0 || mem::u16(data, 0x52) > 0x15) {
        return false;
    }
    const int own = at<uint8_t*>(0)[0x2C];
    const int level = mem::u16(data, 0x54);
    const int above = level - own;
    if (above >= 0xC) {
        return false;
    }
    const int below = own - level;
    if (below >= 0xA) {
        return true;
    }
    if (above >= 7 && above <= 0xB) {
        return current <= maximum / 4;
    }
    if (below >= 5 && below < 0xA) {
        return current <= maximum / 3 || static_cast<int>(R250Below(100)) <= 0x50;
    }
    if (above < 7 && above >= 1) {
        return current <= maximum / 3 || static_cast<int>(R250Below(100)) <= 0x14 - 5 * above;
    }
    if (below >= 5 || below < 0) {
        return false;
    }
    return current <= maximum / 3 || static_cast<int>(R250Below(100)) <= 0xA * below + 0x14;
}

// swd32.exe 0x00472CE0: casting pose 0x2D with shadow; from the first frame with flags 9 the spell act `act`
// (state +0x630) is drawn at the draw position of `target`. Returns 1 when the spell ended.
int BattleHero::DrawCastAt(BattleUnit* target, int act) {
    auto* pose = reinterpret_cast<ActState*>(raw(0x338));
    at<int32_t>(0x2AAC) = 1;
    at<int32_t>(0x338) = at<uint16_t>(0x2A0C);
    at<int32_t>(0x340) = 0x2D;
    if (ActRun(pose) == 0) {
        return 0;
    }
    Picture* picture = TswGetPicture(at<uint16_t>(0x382), at<uint16_t>(0x384));
    at<Picture*>(0x254C) = picture;
    G.drawSource = reinterpret_cast<uint8_t*>(picture->pixels);
    const uint16_t overlay = at<uint16_t>(0x3AE);
    uint32_t flags = at<uint32_t>(0x350);
    uint16_t originX = at<uint16_t>(0x348);
    at<uint16_t>(0x29AC) = overlay;
    if (at<int32_t>(0x2B08) == 1) {
        flags = (flags & 1) != 0 ? flags & ~1u : flags | 1;
        Picture* current = at<Picture*>(0x254C);
        originX = static_cast<uint16_t>(current->width - at<uint16_t>(0x348));
        if (overlay != 0) {
            at<uint16_t>(0x29AC) = static_cast<uint16_t>(current->width - overlay);
        }
    }
    standOptions(*this, at<int32_t>(0x2B00) == 1);
    PlaySound(at<uint16_t>(0x390), G.soundVolume);
    const int drawX = at<int16_t>(0xD66) - static_cast<int16_t>(originX);
    SetSoundPan(at<uint16_t>(0x390), drawX >= 0x140 ? 0x10 : -0x10);
    at<uint16_t>(0x390) = 0;
    standShadow(*this, drawX, flags);
    Picture* current = at<Picture*>(0x254C);
    DrawPicture(drawX, at<int16_t>(0xD68) - at<int32_t>(0x34C), current->width, current->height,
                static_cast<int>(flags), 0);
    if ((at<uint8_t>(0x392) & 9) == 0) {
        return 0;
    }
    auto* spell = reinterpret_cast<ActState*>(raw(0x630));
    at<int32_t>(0x630) = act;
    at<int32_t>(0x638) = 0;
    if (ActRun(spell) == 0) {
        return 0;
    }
    Picture* effect = TswGetPicture(at<uint16_t>(0x67A), at<uint16_t>(0x67C));
    at<Picture*>(0x254C) = effect;
    G.drawSource = reinterpret_cast<uint8_t*>(effect->pixels);
    const uint16_t effectOverlay = at<uint16_t>(0x6A6);
    uint32_t effectFlags = at<uint32_t>(0x648);
    uint16_t effectX = at<uint16_t>(0x640);
    at<uint16_t>(0x29AC) = effectOverlay;
    if (at<int32_t>(0x2B08) == 1) {
        effectFlags = (effectFlags & 1) != 0 ? effectFlags & ~1u : effectFlags | 1;
        Picture* shown = at<Picture*>(0x254C);
        effectX = static_cast<uint16_t>(shown->width - at<uint16_t>(0x640));
        if (effectOverlay != 0) {
            at<uint16_t>(0x29AC) = static_cast<uint16_t>(shown->width - effectOverlay);
        }
    }
    uint16_t targetX = 0;
    uint16_t targetY = 0;
    target->GetDrawPosition(&targetX, &targetY);
    Picture* shown = at<Picture*>(0x254C);
    DrawPicture(static_cast<int16_t>(targetX - effectX), static_cast<int16_t>(targetY - at<uint16_t>(0x644)),
                shown->width, shown->height, static_cast<int>(effectFlags), 0);
    if (at<int32_t>(0x6BC) != 1) {
        return 0;
    }
    std::memset(spell, 0, sizeof(ActState));
    std::memset(pose, 0, sizeof(ActState));
    return 1;
}

}  // namespace swd3
