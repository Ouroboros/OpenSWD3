#include "battle/battle_unit.h"

#include <cstring>

#include "core/random.h"
#include "crt/crt.h"
#include "data/globals.h"
#include "data/image.h"
#include "res/act_anim.h"
#include "res/tsw.h"
#include "audio/audio.h"
#include "battle/battle_draw.h"
#include "battle/battle_message.h"
#include "swd3/w32.h"
#include "draw/draw.h"

namespace swd3 {

using std::int16_t;
using std::int32_t;
using std::uint16_t;
using std::uint32_t;
using std::uint8_t;

// swd32.exe 0x00478220: frees the buffer at +0xA0 of the 0xA4 byte block of a BattleEnemy
int FreeUnitExtra(uint8_t* extra) {
    void* buffer = mem::ptr<void>(extra, 0xA0);
    if (buffer == nullptr) {
        return 0;
    }
    crt::free(buffer);
    mem::put32(extra, 0xA0, 0);
    return 0;
}

// swd32.exe 0x00478250
BattleUnit::BattleUnit() {
    at<uint32_t>(0x2A56) = 0xFFFFFFFFu;
    at<uint32_t>(0x2A5A) = 0xFFFFFFFFu;
    at<uint32_t>(0x2A5E) = 0xFFFFFFFFu;
    at<uint32_t>(0x2A62) = 0xFFFFFFFFu;
    at<uint16_t>(0x29A2) = 0xFFFF;
    at<int32_t>(0x2668) = 0xF;
    at<int32_t>(0x266C) = 1;
    at<uint16_t>(0x2958) = 0;
    at<uint16_t>(0x2A0A) = 4;
    at<uint16_t>(0x2A68) = 2;
    at<uint16_t>(0x2A6A) = 0x18;
    at<uint16_t>(0x2954) = 0;
    at<uint16_t>(0x2956) = 0;
    at<uint16_t>(0x2A66) = 0;
    at<uint16_t>(0x2A0E) = 0;
    at<uint16_t>(0x2A6C) = 0;
    at<uint8_t>(0x2A94) = 0;
    at<uint32_t>(0x26BC) = 0x062B062Bu;
    at<int32_t>(0x2584) = 0;
    at<int32_t>(0x2630) = 0;
    at<int32_t>(0x2634) = 0;
    at<int32_t>(0x2638) = 0;
    at<int32_t>(0x263C) = 0;
    std::memset(raw(0x10), 0, 0xA4);
}

// swd32.exe 0x00478300
BattleUnit::~BattleUnit() {
    void* buffer = at<void*>(0xB0);
    if (buffer != nullptr) {
        crt::free(buffer);
        at<void*>(0xB0) = nullptr;
    }
}

// swd32.exe 0x00478330
int BattleUnit::SetTarget(int value) {
    at<int32_t>(0x2AE4) = value;
    return value;
}

// swd32.exe 0x00478340: gauge (+0x2A12) scaled to 62 pixels of G.gaugeFull
int BattleUnit::GaugeLength() {
    return static_cast<int32_t>(
        crt::ftol(static_cast<double>(at<uint16_t>(0x2A12)) / static_cast<double>(G.gaugeFull) * 62.0));
}

// swd32.exe 0x00478370
int16_t BattleUnit::ResetGauge() {
    at<uint16_t>(0x2A12) = static_cast<uint16_t>(G.gaugeFull);
    return static_cast<int16_t>(G.gaugeFull);
}

// swd32.exe 0x00478380
int BattleUnit::RandomizeGauge() {
    const int value = 0x96 / static_cast<int>(R250Below(9) + 1) + 0x12C;
    at<uint16_t>(0x2A12) = static_cast<uint16_t>(value);
    return value;
}

// swd32.exe 0x004783B0
int16_t BattleUnit::GetDrawPosition(uint16_t* x, uint16_t* y) {
    if (at<uint16_t>(0x26D8) != 0) {
        *x = at<uint16_t>(0xD86);
        *y = at<uint16_t>(0xD88);
        return static_cast<int16_t>(*y);
    }
    *x = at<uint16_t>(0xD66);
    *y = at<uint16_t>(0xD68);
    return static_cast<int16_t>(reinterpret_cast<std::uintptr_t>(x));
}

// swd32.exe 0x00478400
uint16_t* BattleUnit::GetOffset(uint16_t* x, uint16_t* y) {
    *x = at<uint16_t>(0x316);
    *y = at<uint16_t>(0x318);
    if (at<uint8_t>(0x2A87) & 2) {
        *x = at<uint16_t>(0xB66);
        *y = at<uint16_t>(0xB68);
    }
    if (at<int32_t>(0x2B08) == 1) {
        const uint16_t offset = at<uint16_t>(0x316);
        if (offset != 0) {
            *x = static_cast<uint16_t>(mem::u16(at<uint8_t*>(0x2548), 0xC) - offset);
        }
    }
    return x;
}

// swd32.exe 0x00478470
int16_t BattleUnit::GetOrigin(uint16_t* x, uint16_t* y) {
    *x = static_cast<uint16_t>(at<uint16_t>(0xD66) - at<uint16_t>(0x29B2));
    const auto value = static_cast<uint16_t>(at<uint16_t>(0xD68) - at<uint16_t>(0x2B4));
    *y = value;
    return static_cast<int16_t>(value);
}

// swd32.exe 0x004784A0: rectangle of the shadow picture ('HW' action 0x24, 0x33 while defeated) under the unit
int16_t BattleUnit::GetShadowRect(int32_t* rect) {
    ActState state;
    std::memset(&state, 0, sizeof(state));
    const int32_t defeat = at<int32_t>(0x2AB8);
    int32_t value = defeat;
    if (defeat == 1 && (value = at<int32_t>(0x2AA0)) == 0) {
        return static_cast<int16_t>(value);
    }
    const uint16_t id = at<uint16_t>(0x2A0C);
    value = (value & 0xFFFF0000) | id;
    if (id == 0) {
        return static_cast<int16_t>(value);
    }
    state.action = 0x24;
    state.id = id;
    if (at<int32_t>(0x2A8) != 0) {
        state.action = at<int32_t>(0x2A8);
    }
    if (defeat == 1) {
        state.action = 0x33;
    }
    ActRun(&state);
    Picture* picture = TswGetPicture(state.fr, state.ap);
    at<Picture*>(0x254C) = picture;
    auto* image = reinterpret_cast<uint8_t*>(picture);
    if (at<int32_t>(0x2B08) == 1) {
        state.x = mem::u16(image, 0xC) - state.x;
    }
    rect[0] = at<int16_t>(0xD66) - state.x;
    rect[1] = at<int16_t>(0xD68) - state.y;
    rect[2] = mem::u16(at<uint8_t*>(0x254C), 0xC);
    rect[3] = mem::u16(at<uint8_t*>(0x254C), 0xE);
    return static_cast<int16_t>(reinterpret_cast<std::uintptr_t>(rect));
}

// swd32.exe 0x004785A0
int16_t BattleUnit::Move(int16_t dx, int16_t dy) {
    at<int16_t>(0xD66) = static_cast<int16_t>(at<int16_t>(0xD66) + dx);
    at<int16_t>(0xD68) = static_cast<int16_t>(at<int16_t>(0xD68) + dy);
    return dx;
}

// swd32.exe 0x004785C0
int16_t BattleUnit::SetPosition(int16_t x, int16_t y) {
    at<int16_t>(0xD66) = x;
    at<int16_t>(0xD68) = y;
    std::memcpy(raw(0xD70), raw(0xD50), 0x20);
    return x;
}

// swd32.exe 0x00478600
int16_t BattleUnit::GetPosition(uint16_t* x, uint16_t* y) {
    *x = at<uint16_t>(0xD66);
    const uint16_t value = at<uint16_t>(0xD68);
    *y = value;
    return static_cast<int16_t>(value);
}

// swd32.exe 0x00478620: runs a copy of the idle action at +0x2A0 in +0xCB8 and fetches its picture
int BattleUnit::StepIdleAction() {
    std::memcpy(raw(0xCB8), raw(0x2A0), sizeof(ActState));
    const int result = ActRun(reinterpret_cast<ActState*>(raw(0xCB8)));
    if (result == 0) {
        return 0;
    }
    Picture* picture = TswGetPicture(at<uint16_t>(0xD02), at<uint16_t>(0xD04));
    at<Picture*>(0x254C) = picture;
    return static_cast<int>(reinterpret_cast<std::uintptr_t>(picture));
}

// swd32.exe 0x00478670
int BattleUnit::SelectEntry(int second) {
    if (second != 0) {
        return at<int32_t>(0);
    }
    return at<int32_t>(4);
}

// swd32.exe 0x00478690
int BattleUnit::GetValueAAC() { return at<int32_t>(0x2AAC); }

// swd32.exe 0x004786A0
int BattleUnit::GetValueAB4() { return at<int32_t>(0x2AB4); }

// swd32.exe 0x004786B0
uint16_t BattleUnit::GetWord2A6C() { return at<uint16_t>(0x2A6C); }

// swd32.exe 0x004786C0
uint16_t BattleUnit::GetWord2A70() { return at<uint16_t>(0x2A70); }

// swd32.exe 0x004786D0
uint16_t BattleUnit::GetWord2A74() { return at<uint16_t>(0x2A74); }

// swd32.exe 0x004786E0
uint16_t BattleUnit::GetWord29A2() { return at<uint16_t>(0x29A2); }

// swd32.exe 0x004786F0
int BattleUnit::ClearIdleAction() {
    std::memset(raw(0x2A0), 0, sizeof(ActState));
    return 0;
}

// swd32.exe 0x00478710: commands 2, 3, 6, 13, 14 and 15 go to +0x2A70 (6 and 14/15 also set flags), any other to
// +0x2A6C
int BattleUnit::SetCommand(int command) {
    if (command == 2 || command == 0xD || command == 3) {
        // stored below
    } else if (command == 0xE || command == 0xF) {
        at<uint8_t>(0x2A87) |= 0x40;
    } else if (command == 6) {
        at<uint8_t>(0x2A87) |= 8;
    } else {
        at<uint16_t>(0x2A6C) = static_cast<uint16_t>(command);
        return 1;
    }
    at<uint16_t>(0x2A70) = static_cast<uint16_t>(command);
    at<uint16_t>(0x2A6C) = 0;
    return 1;
}

// swd32.exe 0x00478770
void BattleUnit::ClearFlag26B8() { at<uint32_t>(0x26B8) &= ~0x80000000u; }

// swd32.exe 0x00478780
void BattleUnit::RaiseFlag26B8() {
    if (at<uint32_t>(0x26C0) & 0x2000000) {
        return;
    }
    const int32_t value = at<int32_t>(0x26B8);
    if (value < 0) {
        at<uint16_t>(0x2A78) = 0;
        at<uint16_t>(0x542) = 0;
    }
    at<uint32_t>(0x26B8) = static_cast<uint32_t>(value) | 0x80000000u;
}

// swd32.exe 0x004787C0
int BattleUnit::Flag26B8() { return static_cast<int>(at<uint32_t>(0x26B8) >> 31); }

// swd32.exe 0x004787D0: stores at the current count (+0x2A7C) without advancing it
int BattleUnit::PushTarget(uint16_t target) {
    const uint16_t count = at<uint16_t>(0x2A7C);
    at<uint16_t>(0x29C4 + 2 * count) = target;
    return count;
}

// swd32.exe 0x004787F0
int BattleUnit::StartDefeat(int kind) {
    at<int32_t>(0x2AB8) = kind;
    at<int32_t>(0x2ABC) = 1;
    if (at<uint8_t>(0x2A94) == 0) {
        at<uint8_t>(0x2A94) = 6;
    }
    if (at<int32_t>(0x2AA0) == 1) {
        uint8_t* owner = at<uint8_t*>(4);
        if (owner != nullptr) {
            mem::put16(owner, 4, 0);
        }
    }
    return 1;
}

// swd32.exe 0x00478830
int BattleUnit::ToggleAF8() {
    const int value = at<int32_t>(0x2AF8) == 0;
    at<int32_t>(0x2AF8) = value;
    return value;
}

// swd32.exe 0x00478850: clears the per-turn state; units with +0x2AA0 == 1 get a random gauge of 50..189
int BattleUnit::ResetTurn() {
    at<uint16_t>(0x2A12) = 0;
    at<int32_t>(0x2AAC) = 0;
    const uint8_t mode = at<uint8_t>(0xD9C);
    at<int32_t>(0x2AB0) = 0;
    at<int32_t>(0x2AB4) = 0;
    at<int32_t>(0x2AA8) = 0;
    at<int32_t>(0x2AE0) = 0;
    at<int32_t>(0x2B14) = 0;
    at<uint16_t>(0x2A8C) = 0;
    at<uint16_t>(0x2A86) = 0;
    at<int32_t>(0x2AC8) = 0;
    at<int32_t>(0x2AD4) = 0;
    at<int32_t>(0x2ACC) = 0;
    at<int32_t>(0x2B0C) = 0;
    at<int32_t>(0x2AF4) = 0;
    at<uint16_t>(0x2A6C) = 0;
    at<uint16_t>(0x2A70) = 0;
    at<uint16_t>(0x2A72) = 0;
    at<uint16_t>(0x2A7A) = 0;
    at<uint16_t>(0x29A4) = 0;
    at<int32_t>(0x2668) = 0xF;
    at<int32_t>(0x266C) = 1;
    at<uint16_t>(0x2954) = 0;
    at<uint16_t>(0x2A68) = 2;
    at<uint16_t>(0x2A6A) = 8;
    at<int32_t>(0x2674) = 0;
    at<uint16_t>(0x2A0E) = 0;
    at<int32_t>(0x2690) = 0;
    at<int32_t>(0x2670) = 0;
    at<uint16_t>(0x2A8A) = 0;
    at<uint16_t>(0x29A6) = 0;
    at<uint16_t>(0x29A8) = 0;
    at<uint16_t>(0x29AA) = 0;
    at<uint16_t>(0x316) = 0;
    at<uint16_t>(0x318) = 0;
    at<int32_t>(0x26CC) = 0;
    if (mode & 8) {
        at<int32_t>(0x2AF0) = 0;
    }
    if (at<uint8_t>(0xD94) & 4) {
        at<int32_t>(0x2AF0) = 0;
        uint16_t flags = static_cast<uint16_t>(at<uint16_t>(0x26D0) & 0xFFF7);
        at<int32_t>(0x266C) = -0x20;
        flags |= 0x200;
        at<uint16_t>(0x26D0) = flags;
    }
    if (at<int32_t>(0x2B00) == 1) {
        at<int32_t>(0x2AF0) = 1;
    }
    if (at<int32_t>(0x2B04) == 1) {
        std::memcpy(raw(0xD50), raw(0xD70), 0x20);
    }
    std::memset(raw(0x338), 0, 0x98);
    std::memset(raw(0x3D0), 0, 0x98);
    std::memset(raw(0x468), 0, 0x98);
    std::memset(raw(0x500), 0, 0x98);
    std::memset(raw(0x630), 0, 0x4C0);
    at<int32_t>(0x2630) = 0;
    at<int32_t>(0x2634) = 0;
    at<int32_t>(0x2638) = 0;
    at<int32_t>(0x263C) = 0;
    std::memset(raw(0x630), 0, 0x4C0);
    std::memset(raw(0xD90), 0, 0x28);
    at<uint32_t>(0x2A56) = 0xFFFFFFFFu;
    at<uint32_t>(0x2A5A) = 0xFFFFFFFFu;
    at<uint32_t>(0x2A5E) = 0xFFFFFFFFu;
    at<uint32_t>(0x2A62) = 0xFFFFFFFFu;
    int result = at<int32_t>(0x2AA0);
    if (result == 1) {
        result = static_cast<int>(R250Below(0x8C)) + 0x32;
        at<uint16_t>(0x2A12) = static_cast<uint16_t>(result);
    }
    return result;
}

// swd32.exe 0x00478A70
int16_t BattleUnit::Retarget(int16_t target) {
    at<int32_t>(0x2AB4) = 1;
    at<int16_t>(0x29A2) = target;
    at<uint16_t>(0x2A12) = 0;
    return target;
}

// swd32.exe 0x00478AA0
void BattleUnit::IncreaseCounter2A76() { ++at<uint16_t>(0x2A76); }

// swd32.exe 0x00478AB0
uint16_t BattleUnit::Counter2A76() { return at<uint16_t>(0x2A76); }

// swd32.exe 0x00478AC0
void BattleUnit::IncreaseHits() {
    ++at<uint16_t>(0x2A74);
    at<int32_t>(0x2AE0) = 1;
}

// swd32.exe 0x00478AE0
int16_t BattleUnit::DecreaseHits() {
    if (at<uint16_t>(0x2A74) != 0) {
        --at<uint16_t>(0x2A74);
    }
    int16_t result = at<int16_t>(0x2A76);
    if (result != 0) {
        at<int16_t>(0x2A76) = --result;
    }
    at<int32_t>(0x2AE0) = 0;
    return result;
}

// swd32.exe 0x00478B20
void BattleUnit::ClearWord29A2() { at<uint16_t>(0x29A2) = 0xFFFF; }

// swd32.exe 0x00478B30
void BattleUnit::SetAA8() { at<int32_t>(0x2AA8) = 1; }

// swd32.exe 0x00478B40
int BattleUnit::GetAA8() { return at<int32_t>(0x2AA8); }

// swd32.exe 0x00478B50
int BattleUnit::GetAE0() { return at<int32_t>(0x2AE0); }

// swd32.exe 0x0047C660
int BattleUnit::SetAC0(int value) {
    at<int32_t>(0x2AC0) = value;
    return value;
}

// swd32.exe 0x0047C670
bool BattleUnit::IsAE4Clear() { return at<int32_t>(0x2AE4) == 0; }

// swd32.exe 0x0047C680
uint16_t BattleUnit::GetWordDA6() { return at<uint16_t>(0xDA6); }

// swd32.exe 0x0047C690
void BattleUnit::MarkAC8() {
    if (!(at<uint8_t>(0x2A87) & 2)) {
        at<int32_t>(0x2AC8) = 1;
    }
}

namespace {

// Mirroring of the action picture shared by the fade routines: toggles bit 0 of the draw flags and measures the
// x offset from the right edge.
void mirrorFlags(BattleUnit& unit, std::uint32_t actFlags, bool forced) {
    Picture* picture = unit.at<Picture*>(0x254C);
    if (forced) {
        if (unit.at<int32_t>(0x2B08) == 1) {
            unit.at<uint32_t>(0x2694) = (actFlags & 1) ? actFlags & ~1u : actFlags | 1;
            unit.at<uint16_t>(0x29B2) = static_cast<uint16_t>(picture->width - unit.at<uint16_t>(0x478));
        }
        unit.at<uint32_t>(0x2694) |= 4;
    }
    if (unit.at<int32_t>(0x2B08) == 1 && !forced) {
        uint32_t flags = actFlags;
        if (flags & 1) {
            flags &= ~1u;
        } else {
            flags = unit.at<uint32_t>(0x2694) | 1;
        }
        unit.at<uint32_t>(0x2694) = flags;
        unit.at<uint16_t>(0x29B2) = static_cast<uint16_t>(picture->width - unit.at<uint16_t>(0x478));
    }
}

}  // namespace

// swd32.exe 0x0047C6B0: fades the unit out over the counter at +0x2668 (sound 0x2F on the first frame)
int BattleUnit::DrawFadeOut() {
    if (at<int32_t>(0x26C0) < 0 || at<int32_t>(0x2668) <= 0) {
        return 1;
    }
    at<int32_t>(0x468) = at<uint16_t>(0x2A0C);
    const int32_t paused = at<int32_t>(0x2AD0);
    at<int32_t>(0x2AAC) = 1;
    at<int32_t>(0x470) = 0x24;
    at<int32_t>(0x4F8) = 0;
    if (paused == 1) {
        at<int32_t>(0x4F8) = 1;
    }
    if (!ActRun(reinterpret_cast<ActState*>(raw(0x468)))) {
        return 1;
    }
    at<Picture*>(0x254C) = TswGetPicture(at<uint16_t>(0x4B2), at<uint16_t>(0x4B4));
    if (at<uint8_t>(0x480) & 0x14) {
        at<int32_t>(0x2668) = 0;
        return 0;
    }
    if (at<int32_t>(0x2668) == 0xF) {
        at<uint16_t>(0x4C0) = 0x2F;
        PlaySound(0x2F, G.soundVolume);
        SetSoundPan(at<uint16_t>(0x4C0), at<int32_t>(0x2B08) == 1 ? 0x10 : -0x10);
        at<uint16_t>(0x4C0) = 0;
    }
    Picture* picture = at<Picture*>(0x254C);
    G.drawSource = reinterpret_cast<std::uint8_t*>(picture->pixels);
    const int32_t blend = at<int32_t>(0x2B00);
    const uint32_t actFlags = at<uint32_t>(0x480);
    at<uint32_t>(0x2694) = actFlags;
    mirrorFlags(*this, actFlags, blend == 1);
    const int32_t level = at<int32_t>(0x2668);
    const int x = at<int16_t>(0xD66) - at<int16_t>(0x29B2);
    const int y = at<int16_t>(0xD68) - at<int32_t>(0x47C);
    const int arg = static_cast<int>(reinterpret_cast<std::uintptr_t>(picture->reserved4));
    if (blend == 1) {
        G.drawOption71C = level - 0xF;
        G.drawOption30C = level - 0xF;
        G.drawOption304 = level - 0xF;
        DrawPicture(x, y, picture->width, picture->height, static_cast<int>(at<uint32_t>(0x2694)), arg);
        if (at<int32_t>(0x2AD0) == 0) {
            --at<int32_t>(0x2668);
        }
        return 0;
    }
    G.drawAlpha = level;
    DrawPicture(x, y, picture->width, picture->height, static_cast<int>(at<uint32_t>(0x2694) | 0x14), arg);
    G.drawAlpha = 0;
    if (at<int32_t>(0x2AD0) == 0) {
        at<int32_t>(0x2668) -= at<uint16_t>(0x2A0A);
    }
    return 0;
}

// swd32.exe 0x0047C950: fades the unit in at the draw position of `anchor` (sound 0x30 on the first frame)
int BattleUnit::DrawFadeIn(BattleUnit* anchor) {
    if (at<int32_t>(0x26C0) < 0) {
        return 1;
    }
    if (at<int32_t>(0x266C) >= 0xF) {
        std::memset(raw(0x468), 0, sizeof(ActState));
        at<int32_t>(0x2AAC) = 0;
        return 1;
    }
    const uint16_t id = at<uint16_t>(0x2A0C);
    const bool alternate = at<uint16_t>(0x2A6C) == 1;
    at<int32_t>(0x2AAC) = 1;
    at<int32_t>(0x470) = 0x24;
    at<int32_t>(0x468) = id;
    if (alternate) {
        at<int32_t>(0x470) = 0x28;
    }
    const uint16_t pose = at<uint16_t>(0x2A8C);
    if (pose != 0) {
        at<int32_t>(0x470) = 0x30;
        if (pose == 2) {
            at<int32_t>(0x470) = 0x31;
        }
    }
    const int result = ActRun(reinterpret_cast<ActState*>(raw(0x468)));
    if (result == 0) {
        return 0;
    }
    if (at<uint8_t>(0x480) & 0x14) {
        at<int32_t>(0x266C) = 0x10;
        return 0;
    }
    at<Picture*>(0x254C) = TswGetPicture(at<uint16_t>(0x4B2), 0);
    if (at<int32_t>(0x266C) == 1) {
        at<uint16_t>(0x4C0) = 0x30;
        PlaySound(0x30, G.soundVolume);
        SetSoundPan(at<uint16_t>(0x4C0), at<int32_t>(0x2B08) == 1 ? -0x10 : 0x10);
        at<uint16_t>(0x4C0) = 0;
    }
    Picture* picture = at<Picture*>(0x254C);
    G.drawSource = reinterpret_cast<std::uint8_t*>(picture->pixels);
    const uint32_t actFlags = at<uint32_t>(0x480);
    at<uint16_t>(0x29B2) = at<uint16_t>(0x478);
    const int32_t blend = at<int32_t>(0x2B00);
    at<uint32_t>(0x2694) = actFlags;
    at<int32_t>(0x4F8) = 1;
    mirrorFlags(*this, actFlags, blend == 1);
    uint16_t anchorX;
    uint16_t anchorY;
    anchor->GetDrawPosition(&anchorX, &anchorY);
    const int x = static_cast<int16_t>(anchorX) - at<int16_t>(0x29B2);
    const int y = static_cast<int16_t>(anchorY) - at<int32_t>(0x47C);
    const int32_t level = at<int32_t>(0x266C);
    if (at<int32_t>(0x2B00) == 1) {
        G.drawOption71C = level - 0x10;
        G.drawOption30C = level - 0x10;
        G.drawOption304 = level - 0x10;
        DrawPicture(x, y, picture->width, picture->height, static_cast<int>(at<uint32_t>(0x2694)), 0);
        if (at<int32_t>(0x2AD0) == 0) {
            ++at<int32_t>(0x266C);
        }
        return 0;
    }
    G.drawAlpha = level;
    DrawPicture(x, y, picture->width, picture->height, static_cast<int>(at<uint32_t>(0x2694) | 0x14),
                static_cast<int>(reinterpret_cast<std::uintptr_t>(picture->reserved4)));
    G.drawAlpha = 0;
    if (at<int32_t>(0x2AD0) == 0) {
        at<int32_t>(0x266C) += at<uint16_t>(0x2A0A);
    }
    return 0;
}

// swd32.exe 0x0047CC40
int BattleUnit::SetAD0(int value) {
    at<int32_t>(0x2AD0) = value;
    return value;
}

// swd32.exe 0x0047CC50
int BattleUnit::SetAD4(int value) {
    at<int32_t>(0x2AD4) = value;
    return value;
}

// swd32.exe 0x0047CC60: red/green tint pulse 2..24..0 over the picture at +0x2548
int BattleUnit::DrawFlash() {
    const uint16_t phase = at<uint16_t>(0x2A68);
    if (phase == 0x18) {
        const uint16_t fall = at<uint16_t>(0x2A6A);
        if (fall != 0) {
            if (at<int32_t>(0x2AD0) == 0) {
                at<uint16_t>(0x2A6A) = static_cast<uint16_t>(fall - 1);
            }
            G.drawOption71C = at<uint16_t>(0x2A6A);
            G.drawOption30C = at<uint16_t>(0x2A6A);
        } else {
            at<uint16_t>(0x2A68) = 2;
            at<uint16_t>(0x2A6A) = 0x18;
        }
    } else {
        if (at<int32_t>(0x2AD0) == 0) {
            at<uint16_t>(0x2A68) = static_cast<uint16_t>(phase + 1);
        }
        G.drawOption71C = at<uint16_t>(0x2A68);
        G.drawOption30C = at<uint16_t>(0x2A68);
    }
    Picture* picture = at<Picture*>(0x2548);
    G.drawSource = reinterpret_cast<std::uint8_t*>(picture->pixels);
    uint32_t flags = at<uint32_t>(0x2694) & 0x80000003u;
    at<uint32_t>(0x26A4) = flags;
    flags |= 0x10;
    DrawPicture(at<int16_t>(0xD66) - at<int16_t>(0x29B2), at<int16_t>(0xD68) - at<int32_t>(0x2B4), picture->width,
                picture->height, static_cast<int>(flags), 0);
    return 1;
}

// swd32.exe 0x0047CD60: evasion roll; streak counters at 0x0053D028 / 0x0053D029 force or forbid a success
int BattleUnit::RollEvade(uint16_t chance) {
    auto success = [this]() {
        G.evadeStreak = 0;
        G.evadeJustSucceeded = 1;
        at<uint16_t>(0x26D0) |= 1;
        return 1;
    };
    if (G.evadeDisabled == 1) {
        return 0;
    }
    if (at<int32_t>(0x26C0) < 0) {
        if (static_cast<uint16_t>(R250Below(10) + 1) <= 9) {
            return success();
        }
        return 0;
    }
    const uint8_t skill = at<uint8_t>(0x2A93);
    if (skill == 0) {
        return 0;
    }
    if (chance != 0) {
        if (static_cast<uint16_t>(R250Below(10)) < chance) {
            return success();
        }
        return 0;
    }
    if (G.evadeJustSucceeded == 1) {
        G.evadeJustSucceeded = 0;
        return 0;
    }
    const uint8_t streak = G.evadeStreak;
    bool hit;
    if (streak >= 0x18) {
        hit = true;
    } else if (streak >= 0x14) {
        hit = static_cast<uint16_t>(R250Below(3)) == 2;
    } else if (skill <= 6) {
        if (static_cast<uint16_t>(R250Below(10)) < 8) {
            hit = false;
        } else {
            hit = static_cast<uint16_t>(R250Below(10)) < at<uint8_t>(0x2A93);
        }
    } else {
        hit = static_cast<uint16_t>(R250Below(10)) < at<uint8_t>(0x2A93);
    }
    if (hit) {
        return success();
    }
    ++G.evadeStreak;
    return 0;
}

// swd32.exe 0x0047CE70
int BattleUnit::Mirrored() { return at<uint8_t>(0x2694) & 1; }

// swd32.exe 0x0047CE80
bool BattleUnit::IsDefeated() { return at<int32_t>(0x2AB8) == 1 || (at<uint8_t>(0x26D1) & 0x20) != 0; }

// swd32.exe 0x0047CEA0
int BattleUnit::StatusBit13() { return (at<uint16_t>(0x26D0) >> 13) & 1; }

// swd32.exe 0x0047CEB0
bool BattleUnit::IsAB8() { return at<int32_t>(0x2AB8) == 1; }

// swd32.exe 0x0047CEC0: +1 (capped at 0x22) or -1 on the target count
int BattleUnit::StepTargetCount(int increase) {
    if (increase == 1) {
        const bool below = ++at<uint16_t>(0x2A7C) < 0x23;
        at<int32_t>(0x2AD8) = 1;
        if (!below) {
            at<uint16_t>(0x2A7C) = 0x22;
        }
    } else {
        --at<uint16_t>(0x2A7C);
        at<int32_t>(0x2AD8) = increase;
    }
    return increase;
}

// swd32.exe 0x0047CF00
int BattleUnit::PushTargetAlt(uint16_t target) {
    const uint16_t count = at<uint16_t>(0x2A7C);
    at<uint16_t>(0x26DE + 2 * count) = target;
    return count;
}

// swd32.exe 0x0047D2E0: value +/- tenths for -10..127, value * (tenths + 10) / 10 (8-bit wrap) below
int ScaleByTenths(std::int8_t tenths, int16_t* value) {
    if (tenths >= -10) {
        *value = static_cast<int16_t>(*value + tenths * *value / 10);
        return 0;
    }
    *value = static_cast<int16_t>(*value * static_cast<std::int8_t>(tenths + 10) / 10);
    return 1;
}

// swd32.exe 0x0047D7D0
int BattleUnit::ResetAfterGuard() {
    at<int32_t>(0x2AEC) = 1;
    at<uint16_t>(0x2A12) = 0;
    at<int32_t>(0x2670) = 0;
    return 0;
}

// swd32.exe 0x0047D7F0
int BattleUnit::GetGuard() {
    if (at<uint8_t>(0x26D0) & 0x42) {
        return 0;
    }
    return at<int32_t>(0x2AEC);
}

// swd32.exe 0x0047D810
int BattleUnit::SetStatus(int bits) {
    if (static_cast<uint16_t>(bits) == 0x200) {
        at<int32_t>(0x266C) = -0x20;
    }
    at<uint16_t>(0x26D0) |= static_cast<uint16_t>(bits);
    return bits;
}

// swd32.exe 0x0047D830
int BattleUnit::ClearStatus(int bits) {
    at<uint16_t>(0x26D0) &= static_cast<uint16_t>(~bits);
    if (bits & 0x1000) {
        at<int32_t>(0x2AF0) = 0;
    }
    return bits;
}

// swd32.exe 0x0047D860
int16_t BattleUnit::SetWord2A0A(int16_t value) {
    at<int16_t>(0x2A0A) = value;
    return value;
}

// swd32.exe 0x0047D870
int BattleUnit::SetAF0(int value) {
    at<int32_t>(0x2AF0) = value;
    return value;
}

// swd32.exe 0x0047D880
int BattleUnit::ActFlag1() { return (at<uint8_t>(0xD94) >> 1) & 1; }

// swd32.exe 0x0047D8A0
int BattleUnit::ActFlag11() { return (at<uint32_t>(0xD94) >> 11) & 1; }

// swd32.exe 0x0047D8B0
int16_t BattleUnit::ActSpeed() {
    const int16_t speed = at<int16_t>(0xDAE);
    if (speed >= 0) {
        return 0;
    }
    return static_cast<int16_t>(speed & 0x7FFF);
}

// swd32.exe 0x0047D8D0
int BattleUnit::ActFlag0() { return at<uint32_t>(0xD94) & 1; }

// swd32.exe 0x0047D8E0
int BattleUnit::ActFlag13() { return (at<uint32_t>(0xD94) >> 13) & 1; }

// swd32.exe 0x0047D8F0
int BattleUnit::ActFlag14() { return (at<uint32_t>(0xD94) >> 14) & 1; }

// swd32.exe 0x0047D900
int BattleUnit::SetAction2A0E(int16_t action) {
    at<int16_t>(0x2A0E) = action;
    std::memset(raw(0x2A0), 0, sizeof(ActState));
    return 0;
}

// swd32.exe 0x0047D930
bool BattleUnit::IsGuarding() { return at<int32_t>(0x2AF0) == 1 || (at<uint8_t>(0x26D1) & 0x10) != 0; }

// swd32.exe 0x0047D950: heroes (+0x2AA0 == 1) update the current/maximum pairs of their record at +4 (-1 restores
// the current value from the maximum); enemies store the first value in their block at +0xC
void BattleUnit::SetStats(int16_t first, int16_t second, int16_t third) {
    if (at<int32_t>(0x2AA0) == 1) {
        uint8_t* record = at<uint8_t*>(4);
        if (record == nullptr) {
            return;
        }
        if (first > 0) {
            mem::put16(record, 0xA, static_cast<uint16_t>(first));
            mem::put16(record, 4, static_cast<uint16_t>(first));
        }
        if (first == -1) {
            mem::put16(record, 4, mem::u16(record, 0xA));
        }
        if (second > 0) {
            mem::put16(record, 0xC, static_cast<uint16_t>(second));
            mem::put16(record, 6, static_cast<uint16_t>(second));
        }
        if (second == -1) {
            mem::put16(record, 6, mem::u16(record, 0xC));
        }
        if (third > 0) {
            mem::put16(record, 0xE, static_cast<uint16_t>(third));
            mem::put16(record, 8, static_cast<uint16_t>(third));
        }
        if (third == -1) {
            mem::put16(record, 8, mem::u16(record, 0xE));
        }
        return;
    }
    uint8_t* block = at<uint8_t*>(0xC);
    if (block != nullptr) {
        mem::put32(block, 0x4C, static_cast<uint32_t>(static_cast<int32_t>(first)));
        mem::put16(block, 0x64, static_cast<uint16_t>(first));
    }
}

// swd32.exe 0x0047DA10: positive values only; heroes write their record at +0 (not checked for null)
int16_t BattleUnit::SetLimits(int16_t first, int16_t second, int16_t third) {
    if (at<int32_t>(0x2AA0) == 1) {
        uint8_t* record = at<uint8_t*>(0);
        if (first > 0) {
            mem::put16(record, 0x26, static_cast<uint16_t>(first));
        }
        if (second > 0) {
            mem::put16(record, 0x28, static_cast<uint16_t>(second));
        }
        if (third > 0) {
            mem::put16(record, 0x16, static_cast<uint16_t>(third));
        }
        return third;
    }
    uint8_t* block = at<uint8_t*>(0xC);
    if (block == nullptr) {
        return 0;
    }
    if (first > 0) {
        mem::put16(block, 0x56, static_cast<uint16_t>(first));
    }
    if (second > 0) {
        mem::put16(block, 0x58, static_cast<uint16_t>(second));
    }
    if (third > 0) {
        mem::put16(block, 0x5A, static_cast<uint16_t>(third));
    }
    return third;
}

// swd32.exe 0x0047DA90
void BattleUnit::SetEnemyWord54(int16_t value) {
    if (at<int32_t>(0x2AA0) != 1) {
        uint8_t* block = at<uint8_t*>(0xC);
        if (block != nullptr) {
            mem::put16(block, 0x54, static_cast<uint16_t>(value));
        }
    }
}

// swd32.exe 0x0047DAB0
int BattleUnit::SetB00(int value) {
    at<int32_t>(0x2B00) = value;
    return value;
}

// swd32.exe 0x0047DAC0
int BattleUnit::SetB04(int value) {
    at<int32_t>(0x2B04) = value;
    return value;
}

// Status effects are a list headed at +0x2584 (node: next, ActState at +4, bits +0x9C, timer +0xA0, turns +0xA2,
// message +0xA4, icon +0xA6). Some walks start at the head field itself, treating the unit bytes at +0x2584 as
// a node.

// swd32.exe 0x0047E4C0
int BattleUnit::HasEffectMessage(int16_t message) {
    if (message == 0) {
        return 0;
    }
    for (uint8_t* node = at<uint8_t*>(0x2584); node != nullptr; node = mem::ptr<uint8_t>(node, 0)) {
        if (message == mem::s16(node, 0xA4)) {
            return 1;
        }
    }
    return 0;
}

// swd32.exe 0x0047E500: kinds 1, 2, 6, 7 map to effect bits 0x1000000, 0x800000, 0x400000, 0x200000 (others to 0)
int BattleUnit::HasEffectKind(int16_t kind) {
    uint8_t* node = at<uint8_t*>(0x2584);
    if (node == nullptr) {
        return 0;
    }
    uint32_t bits = 0;
    switch (kind) {
    case 1:
        bits = 0x1000000;
        break;
    case 2:
        bits = 0x800000;
        break;
    case 6:
        bits = 0x400000;
        break;
    case 7:
        bits = 0x200000;
        break;
    default:
        break;
    }
    for (; node != nullptr; node = mem::ptr<uint8_t>(node, 0)) {
        if (bits == mem::u32(node, 0x9C)) {
            return 1;
        }
    }
    return 0;
}

// swd32.exe 0x0047E580
int BattleUnit::HasEffectBits(uint16_t bits) {
    for (uint8_t* node = at<uint8_t*>(0x2584); node != nullptr; node = mem::ptr<uint8_t>(node, 0)) {
        if (mem::u32(node, 0x9C) & bits) {
            return 1;
        }
    }
    return 0;
}

// swd32.exe 0x0047E650: animates the first effect with an action id (the head field itself when +0x2588 > 0)
// above the unit, then the stone (+0x26C0 bit 2) or freeze (bit 12) overlay at (x, y)
int BattleUnit::DrawEffects(int x, int y) {
    int result = at<int32_t>(0x2584);
    if (result == 0) {
        return 0;
    }
    uint8_t* node = raw(0x2584);
    bool found = true;
    if (at<int32_t>(0x2588) <= 0) {
        do {
            node = mem::ptr<uint8_t>(node, 0);
            if (node == nullptr) {
                found = false;
                break;
            }
        } while (mem::s32(node, 4) <= 0);
    }
    if (found) {
        mem::put32(node, 0x94, 0);
        if (at<int32_t>(0x2AD0) == 1) {
            mem::put32(node, 0x94, 1);
        }
        result = ActRun(reinterpret_cast<ActState*>(node + 4));
        if (result == 0) {
            return 0;
        }
        Picture* picture = TswGetPicture(mem::u16(node, 0x4E), mem::u16(node, 0x50));
        at<Picture*>(0x254C) = picture;
        G.drawSource = reinterpret_cast<std::uint8_t*>(picture->pixels);
        if (at<int32_t>(0x26B8) >= 0) {
            Picture* current = at<Picture*>(0x254C);
            DrawPicture(at<int16_t>(0xD66) - mem::s32(node, 0x14),
                        at<int16_t>(0xD68) - (current->height >> 1) - at<int32_t>(0x2B4), current->width,
                        current->height, mem::s32(node, 0x1C), 0);
        }
    }
    result = at<int32_t>(0x26C0);
    Picture* body = at<Picture*>(0x2548);
    if (result & 4) {
        G.drawSource = reinterpret_cast<std::uint8_t*>(body->pixels);
        const uint32_t flags = (at<uint32_t>(0x2694) & 0x80000000u) | (at<uint32_t>(0x2694) & 3) | 0x28;
        at<uint32_t>(0x26A4) = flags;
        return DrawPicture(x, y, body->width, body->height, static_cast<int>(flags), 0);
    }
    if (result & 0x1000) {
        at<uint32_t>(0x26A4) = at<uint32_t>(0x2694) & 0x80000003u;
        G.drawOption71C = -12;
        G.drawOption30C = 0;
        G.drawOption304 = -12;
        G.drawSource = reinterpret_cast<std::uint8_t*>(body->pixels);
        return DrawPicture(x, y, body->width, body->height, static_cast<int>(at<uint32_t>(0x26A4) | 0x10), 0);
    }
    return result;
}

// swd32.exe 0x0047E7F0: icons (picture 0x241D, frame icon - 1) of the effects in a row, starting with the head
// field taken as a node
int BattleUnit::DrawEffectIcons(int x, int y) {
    int count = 0;
    int result = at<int32_t>(0x2584);
    if (result == 0) {
        return 0;
    }
    for (uint8_t* node = raw(0x2584);;) {
        const uint16_t icon = mem::u16(node, 0xA6);
        if (icon != 0 && mem::s32(node, 0x9C) >= 0) {
            Picture* picture = TswGetPicture(0x241D, icon - 1u);
            at<Picture*>(0x254C) = picture;
            G.drawSource = reinterpret_cast<std::uint8_t*>(picture->pixels);
            Picture* current = at<Picture*>(0x254C);
            DrawPicture(x + count * current->width, y, current->width, current->height, 0, 0);
            ++count;
        }
        result = mem::s32(node, 0);
        if (result == 0) {
            break;
        }
        node = mem::ptr<uint8_t>(node, 0);
    }
    return result;
}

// swd32.exe 0x0047F150: applies hp damage and mp / sp changes; returns 1 when the unit falls. Heroes with the
// gold guard (+0x2B18) pay hp damage from G.gold while it is not zero.
int BattleUnit::Damage(int hp, int16_t mp, int16_t sp) {
    if (hp > 0 && ((at<uint8_t>(0x26D1) & 0x80) || (at<uint32_t>(0x26C0) & 0x2000000))) {
        return 0;
    }
    if (at<int32_t>(0x2AB8) == 1 || EffectFlag3() == 1) {
        return 0;
    }
    if (at<int32_t>(0x2AA0) != 1) {
        uint8_t* block = at<uint8_t*>(0xC);
        const int32_t shield = at<int32_t>(0x26B4);
        if (shield != 0) {
            at<int32_t>(0x26B4) = hp < shield ? shield - hp : 0;
            const int32_t limit = mem::s32(block, 0x4C);
            if (at<int32_t>(0x26B4) > limit) {
                at<int32_t>(0x26B4) = limit;
            }
            if (at<int32_t>(0x26B4) > 0) {
                return 0;
            }
            mem::put16(block, 0x64, 0);
            at<int32_t>(0x26B4) = 0;
        } else {
            const int16_t life = mem::s16(block, 0x64);
            mem::put16(block, 0x64, hp < life ? static_cast<uint16_t>(life - hp) : 0);
            if (mem::s16(block, 0x64) < 0) {
                mem::put16(block, 0x64, mem::u16(block, 0x4C));
            }
            if (mem::s16(block, 0x64) > mem::s32(block, 0x4C)) {
                mem::put16(block, 0x64, mem::u16(block, 0x4C));
            }
            if (mem::s16(block, 0x64) > 0) {
                return 0;
            }
            mem::put16(block, 0x64, 0);
        }
        const uint8_t pose = at<uint8_t>(0x2A94);
        at<int32_t>(0x2ABC) = 1;
        at<uint16_t>(0x2958) = 0;
        if (pose == 0) {
            at<uint8_t>(0x2A94) = static_cast<uint8_t>(R250Below(10) + 1);
        }
        return 1;
    }
    uint8_t* record = at<uint8_t*>(4);
    if (at<int32_t>(0x2B18) == 1 && G.gold != 0) {
        G.gold -= hp;
        if (G.gold == 0) {
            G.gold = 0;
        }
    } else {
        mem::put16(record, 4, static_cast<uint16_t>(mem::u16(record, 4) - hp));
        if (mem::s16(record, 4) > mem::s16(record, 0xA)) {
            mem::put16(record, 4, mem::u16(record, 0xA));
        }
    }
    mem::put16(record, 6, static_cast<uint16_t>(mem::u16(record, 6) + mp));
    if (mem::s16(record, 6) > mem::s16(record, 0xC)) {
        mem::put16(record, 6, mem::u16(record, 0xC));
    }
    mem::put16(record, 8, static_cast<uint16_t>(mem::u16(record, 8) + sp));
    if (mem::s16(record, 8) > mem::s16(record, 0xE)) {
        mem::put16(record, 8, mem::u16(record, 0xE));
    }
    if (mem::s16(record, 6) <= 0) {
        mem::put16(record, 6, 0);
    }
    if (mem::s16(record, 8) <= 0) {
        mem::put16(record, 8, 0);
    }
    if (mem::s16(record, 4) > 0) {
        return 0;
    }
    mem::put16(record, 4, 0);
    at<int32_t>(0x2ABC) = 1;
    if (at<int32_t>(0x2B00) == 1) {
        at<uint8_t>(0x2A94) = 6;
    }
    return 1;
}

// swd32.exe 0x0047F340
int BattleUnit::EffectFlag2() { return (at<uint8_t>(0x26C8) >> 2) & 1; }

// swd32.exe 0x0047F360
int BattleUnit::EffectFlag3() { return (at<uint8_t>(0x26C8) >> 3) & 1; }

// swd32.exe 0x0047F380: a fixed pose (+0x2A95) overrides the requested one
uint8_t BattleUnit::SetDefeatPose(uint8_t pose) {
    at<uint8_t>(0x2A94) = pose;
    const uint8_t fixed = at<uint8_t>(0x2A95);
    if (fixed != 0) {
        at<uint8_t>(0x2A94) = fixed;
    }
    return fixed;
}

// swd32.exe 0x0047F3A0
uint8_t BattleUnit::SetDefeatPoseFixed(uint8_t pose) {
    at<uint8_t>(0x2A95) = pose;
    at<uint8_t>(0x2A94) = pose;
    return pose;
}

namespace {

void announce(const char* format, const char* name, int flags) {
    w32::wsprintfA(G.battleText, format, name);
    AddBattleMessage(0x118, 0xA, 0x28, G.battleText, flags);
}

void announceWithSound(const char* format, const char* name, int flags) {
    announce(format, name, flags);
    PlaySound(0x117, G.soundVolume);
}

}  // namespace

// swd32.exe 0x0047D640: splits `amount` into the four digits of the current target slot (a set bit 15 negates it).
// While the barrier status (0x2000000) is up against act 0x235E, a non-negated hit only counts the barrier down
// and ends it with "抵抗所有攻擊解除".
void BattleUnit::SetDamageDigits(int amount) {
    if ((at<uint8_t>(0x26D1) & 0x80) != 0) {
        return;
    }
    const uint16_t slot = at<uint16_t>(0x2A7C);
    if (at<uint16_t>(0x29C4 + 2 * slot) == 0x235E && (at<uint32_t>(0x26C0) & 0x2000000) != 0 &&
        (amount & 0x8000) == 0) {
        if (--at<uint16_t>(0x2A90) != 0) {
            return;
        }
        announceWithSound(G.resistAllEndText, at<const char*>(0x2560), kMessageBlackBar | kMessageCentered);
        at<uint32_t>(0x26C0) &= ~0x2000000u;
        return;
    }
    if ((amount & 0x8000) != 0) {
        amount = -amount;
    }
    at<int16_t>(0x2724 + 8 * slot) = 0;
    at<int16_t>(0x2724 + 8 * at<uint16_t>(0x2A7C)) = static_cast<int16_t>(amount / 1000);
    uint8_t* digits = raw(8 * at<uint16_t>(0x2A7C));
    amount -= 1000 * mem::s16(digits, 0x2724);
    mem::put16(digits, 0x2726, static_cast<uint16_t>(amount / 100));
    digits = raw(8 * at<uint16_t>(0x2A7C));
    amount -= 100 * mem::s16(digits, 0x2726);
    mem::put16(digits, 0x2728, static_cast<uint16_t>(amount / 10));
    digits = raw(8 * at<uint16_t>(0x2A7C));
    mem::put16(digits, 0x272A, static_cast<uint16_t>(amount - static_cast<uint16_t>(mem::u16(digits, 0x2728) * 10)));
}

// swd32.exe 0x0047E5C0: applies the pending damage at +0x2A84 (bit 15: healing); true when the unit fell
bool BattleUnit::ApplyPendingDamage() {
    if (at<uint16_t>(0x2A84) == 0) {
        return false;
    }
    PushTarget(0x235E);
    if ((at<uint16_t>(0x2A84) & 0x8000) != 0) {
        PushTarget(0x246F);
    }
    SetDamageDigits(at<int16_t>(0x2A84));
    StepTargetCount(1);
    if ((at<uint16_t>(0x2A84) & 0x8000) == 0) {
        RaiseFlag26B8();
    }
    const int result = Damage(at<int16_t>(0x2A84), 0, 0);
    at<uint16_t>(0x2A84) = 0;
    return result == 1;
}

// swd32.exe 0x0047E950: with `clearAll` 0, every effect whose turns (+0xA2) ran out loses its status bit and is
// announced, then freed. The list must not be empty, and the effect following a freed one is skipped. Otherwise
// all effects are freed and the status cleared. The return value is unused by callers.
void BattleUnit::ExpireEffects(int clearAll) {
    if (clearAll != 0) {
        uint8_t* node = at<uint8_t*>(0x2584);
        at<uint16_t>(0x26D0) &= 0xFEBD;
        at<uint32_t>(0x26C0) = 0;
        at<uint8_t*>(0x2584) = nullptr;
        while (node != nullptr) {
            uint8_t* next = mem::ptr<uint8_t>(node, 0);
            crt::free(node);
            node = next;
        }
        return;
    }
    const char* name = at<const char*>(0x2560);
    uint32_t& status = at<uint32_t>(0x26C0);
    uint16_t& marks = at<uint16_t>(0x26D0);
    auto** link = reinterpret_cast<uint8_t**>(raw(0x2584));
    for (;;) {
        uint8_t* node = *link;
        if (mem::u16(node, 0xA2) == 0) {
            bool clearMark40 = false;
            switch (mem::u32(node, 0x9C)) {
                case 1:
                    status &= ~1u;
                    clearMark40 = (status & 0xC) == 0;
                    at<int32_t>(0x2AD0) = 0;
                    break;
                case 2:
                    marks &= ~2;
                    status &= ~2u;
                    break;
                case 4:
                    status &= ~4u;
                    clearMark40 = (status & 9) == 0;
                    at<int32_t>(0x2AD0) = 0;
                    break;
                case 8:
                    status &= ~8u;
                    clearMark40 = (status & 5) == 0;
                    at<int32_t>(0x2AD0) = 0;
                    break;
                case 0x10:
                    marks &= ~0x100;
                    status &= ~0x10u;
                    break;
                case 0x20:
                    status &= ~0x20u;
                    break;
                case 0x40:
                    status &= ~0x40u;
                    break;
                case 0x80:
                    status &= ~0x80u;
                    break;
                case 0x100:
                    status &= ~0x100u;
                    break;
                case 0x200:
                    status &= ~0x200u;
                    if (at<int32_t>(0x2AA0) == 1) {
                        uint8_t* stats = at<uint8_t*>(0);
                        for (const std::intptr_t offset : {0x26, 0x28, 0x16, 0x14}) {
                            const uint16_t value = mem::u16(stats, offset);
                            mem::put16(stats, offset, static_cast<uint16_t>(value + 10 * value / 100));
                        }
                    }
                    break;
                case 0x1000:
                    status &= ~0x1000u;
                    break;
                case 0x80000:
                    announceWithSound(G.defenseDownEndText, name, kMessageBlackBar | kMessageCentered);
                    status &= ~0x100000u;
                    break;
                case 0x100000:
                    announceWithSound(G.speedDownEndText, name, kMessageBlackBar | kMessageCentered);
                    status &= ~0x8000000u;
                    break;
                case 0x200000:
                    announceWithSound(G.darkAttackUpEndText, name, kMessageBlackBar | kMessageCentered);
                    break;
                case 0x400000:
                    announceWithSound(G.lightAttackUpEndText, name, kMessageBlackBar | kMessageCentered);
                    break;
                case 0x800000:
                    announceWithSound(G.iceAttackUpEndText, name, kMessageFrame | kMessageCentered);
                    break;
                case 0x1000000:
                    announceWithSound(G.fireAttackUpEndText, name, kMessageFrame | kMessageCentered);
                    break;
                case 0x2000000:
                    announceWithSound(G.barrierEndText, name, kMessageBlackBar | kMessageCentered);
                    status &= ~0x4000000u;
                    break;
                case 0x8000000:
                    announceWithSound(G.attackDownEndText, name, kMessageBlackBar | kMessageCentered);
                    status &= ~0x1000000u;
                    at<uint32_t>(0x26CC) &= ~0x100u;
                    break;
                case 0x10000000:
                    at<uint8_t>(0x26D1) |= 4;
                    status &= 0x7FFFFFFF;
                    at<uint16_t>(0x2958) = 0;
                    if (at<int32_t>(0x2AA0) != 0) {
                        announceWithSound(G.invisibleEndText, name, kMessageBlackBar | kMessageCentered);
                    }
                    break;
                case 0x20000000:
                    announceWithSound(G.defenseUpEndText, name, kMessageBlackBar | kMessageCentered);
                    status &= ~0x40000000u;
                    break;
                case 0x40000000:
                    announceWithSound(G.speedUpEndText, name, kMessageBlackBar | kMessageCentered);
                    status &= ~0x20000000u;
                    break;
                case 0x80000000:
                    announceWithSound(G.attackUpEndText, name, kMessageBlackBar | kMessageCentered);
                    status &= ~0x10000000u;
                    break;
                default:
                    break;
            }
            if (clearMark40) {
                marks &= ~0x40;
            }
            const char* resistEnd = nullptr;
            switch (mem::u16(node, 0xA4)) {
                case 1: resistEnd = G.resistFireEndText; break;
                case 2: resistEnd = G.resistIceEndText; break;
                case 3: resistEnd = G.resistWindEndText; break;
                case 4: resistEnd = G.resistEarthEndText; break;
                case 5: resistEnd = G.resistPoisonEndText; break;
                case 6: resistEnd = G.resistLightEndText; break;
                case 7: resistEnd = G.resistDarkEndText; break;
                case 8: resistEnd = G.resistThunderEndText; break;
                case 9: resistEnd = G.resistPhysicalEndText; break;
                default: break;
            }
            if (resistEnd != nullptr) {
                announceWithSound(resistEnd, name, kMessageBlackBar | kMessageCentered);
            }
            uint8_t* next = mem::ptr<uint8_t>(*link, 0);
            if (next == nullptr) {
                crt::free(*link);
                *link = nullptr;
                return;
            }
            crt::free(*link);
            *link = next;
        }
        uint8_t* current = *link;
        if (mem::ptr<uint8_t>(current, 0) == nullptr) {
            return;
        }
        link = reinterpret_cast<uint8_t**>(current);
    }
}

namespace {

// +0x330 (the paused flag of the action state at +0x2A0) follows +0x2AD0
void syncPaused(BattleUnit& unit) {
    unit.at<int32_t>(0x330) = 0;
    if (unit.at<int32_t>(0x2AD0) == 1) {
        unit.at<int32_t>(0x330) = 1;
    }
}

}  // namespace

// swd32.exe 0x0047F3C0: draws the unit darkening by 2 per frame (unless paused) down to -32; then the action
// state at +0x468 is cleared and 1 returned. The argument is unused.
int BattleUnit::DrawDarken(int unused) {
    static_cast<void>(unused);
    syncPaused(*this);
    Picture* picture = TswGetPicture(at<uint16_t>(0x2EA), at<uint16_t>(0x2EC));
    at<Picture*>(0x254C) = picture;
    G.drawSource = reinterpret_cast<uint8_t*>(picture->pixels);
    G.drawOption71C = at<int16_t>(0x2958);
    G.drawOption30C = at<int16_t>(0x2958);
    G.drawOption304 = at<int16_t>(0x2958);
    DrawPicture(at<int16_t>(0xD66) - at<int16_t>(0x29B2), at<int16_t>(0xD68) - at<int32_t>(0x2B4),
                at<Picture*>(0x254C)->width, at<Picture*>(0x254C)->height,
                static_cast<int>(at<uint32_t>(0x2694) | 4), 0);
    if (at<uint16_t>(0x2EE) != 0 && at<uint16_t>(0x2F0) != 0) {
        G.drawOption71C = at<int16_t>(0x2958);
        G.drawOption30C = at<int16_t>(0x2958);
        G.drawOption304 = at<int16_t>(0x2958);
        Picture* overlay = TswGetPicture(at<uint16_t>(0x2EE), static_cast<uint16_t>(at<uint16_t>(0x2F0) - 1));
        at<Picture*>(0x2554) = overlay;
        G.drawSource = reinterpret_cast<uint8_t*>(overlay->pixels);
        DrawPicture(at<int16_t>(0xD66) + at<int16_t>(0x2FE) - at<int16_t>(0x29B2),
                    at<int16_t>(0xD68) + at<int16_t>(0x300) - at<int32_t>(0x2B4), at<Picture*>(0x2554)->width,
                    at<Picture*>(0x2554)->height, static_cast<int>(at<uint32_t>(0x2694) | 4), 0);
    }
    if (at<int32_t>(0x2AD0) == 0) {
        at<int16_t>(0x2958) -= 2;
    }
    if (at<int16_t>(0x2958) > -0x20) {
        return 0;
    }
    at<int16_t>(0x2958) = -0x20;
    std::memset(raw(0x468), 0, sizeof(ActState));
    return 1;
}

// swd32.exe 0x0047F580: draws the unit brightening by 1 per frame (unless paused) from the level at +0x266C;
// at 0 the mark 0x200 is cleared, the level set to 1 and 1 returned
int BattleUnit::DrawBrighten() {
    syncPaused(*this);
    Picture* picture = TswGetPicture(at<uint16_t>(0x2EA), at<uint16_t>(0x2EC));
    at<Picture*>(0x254C) = picture;
    G.drawSource = reinterpret_cast<uint8_t*>(picture->pixels);
    G.drawOption71C = at<int32_t>(0x266C);
    G.drawOption30C = G.drawOption71C;
    G.drawOption304 = G.drawOption71C;
    DrawPicture(at<int16_t>(0xD66) - at<int16_t>(0x29B2), at<int16_t>(0xD68) - at<int32_t>(0x2B4),
                at<Picture*>(0x254C)->width, at<Picture*>(0x254C)->height,
                static_cast<int>(at<uint32_t>(0x2694) | 4), 0);
    if (at<uint16_t>(0x2EE) != 0 && at<uint16_t>(0x2F0) != 0) {
        const int32_t level = at<int32_t>(0x266C);
        G.drawOption71C = level;
        G.drawOption30C = level;
        G.drawOption304 = level;
        Picture* overlay = TswGetPicture(at<uint16_t>(0x2EE), static_cast<uint16_t>(at<uint16_t>(0x2F0) - 1));
        at<Picture*>(0x2554) = overlay;
        G.drawSource = reinterpret_cast<uint8_t*>(overlay->pixels);
        DrawPicture(at<int16_t>(0xD66) + at<int16_t>(0x2FE) - at<int16_t>(0x29B2),
                    at<int16_t>(0xD68) + at<int16_t>(0x300) - at<int32_t>(0x2B4), at<Picture*>(0x2554)->width,
                    at<Picture*>(0x2554)->height, static_cast<int>(at<uint32_t>(0x2694) | 4), 0);
    }
    if (at<int32_t>(0x2AD0) == 0) {
        ++at<int32_t>(0x266C);
    }
    if (at<int32_t>(0x266C) < 0) {
        return 0;
    }
    at<uint16_t>(0x26D0) &= ~0x200;
    at<int32_t>(0x266C) = 1;
    return 1;
}

// swd32.exe 0x0047F710: 24 frames of the unit over its squashed shadow picture (+0x2548) with translucency
// fading out (`mode` 0) or in (`mode` 1). Returns 1 while running; at the end mode 0 sets status bit 31, and
// marks 0x480 are cleared.
int BattleUnit::DrawPhase(int mode) {
    syncPaused(*this);
    Picture* picture = TswGetPicture(at<uint16_t>(0x2EA), at<uint16_t>(0x2EC));
    at<Picture*>(0x254C) = picture;
    G.drawSource = reinterpret_cast<uint8_t*>(picture->pixels);
    if (++at<int16_t>(0x2958) <= 0x18) {
        G.drawScaleHeight = at<Picture*>(0x2548)->height / 3;
        G.drawShear = at<Picture*>(0x2548)->height >> 2;
        G.drawOption304 = -1;
        G.drawOption30C = -1;
        G.drawOption71C = -1;
        if (at<int32_t>(0x2B00) == 1) {
            G.drawOption304 = -1;
            G.drawOption30C = -1;
            G.drawOption71C = -1;
        }
        Picture* shadow = at<Picture*>(0x2548);
        const uint32_t shadowFlags = (at<uint32_t>(0x2694) & 0x8000000F) | 0xC;
        at<uint32_t>(0x26A4) = shadowFlags;
        DrawPicture(at<int16_t>(0xD66) - at<int16_t>(0x29B2) - 5, at<int16_t>(0xD68) - G.drawScaleHeight,
                    shadow->width, shadow->height, static_cast<int>(shadowFlags), 0);
        if (mode == 0) {
            G.drawAlpha = 0xF - at<int16_t>(0x2958) / 2;
        } else if (mode == 1) {
            G.drawAlpha = at<int16_t>(0x2958) / 2;
        }
        Picture* current = at<Picture*>(0x254C);
        const uint32_t flags = at<uint32_t>(0x2694) & 0x80000003;
        at<uint32_t>(0x26A4) = flags;
        DrawPicture(at<int16_t>(0xD66) - at<int16_t>(0x29B2), at<int16_t>(0xD68) - at<int32_t>(0x2B4),
                    current->width, current->height, static_cast<int>(flags | 0x14), 0);
        return 1;
    }
    at<int16_t>(0x2958) = 0x18;
    if (mode == 1) {
        at<uint16_t>(0x26D0) &= 0xFB7F;
        at<int16_t>(0x2958) = 0;
        return 0;
    }
    if (mode == 0) {
        at<uint32_t>(0x26C0) |= 0x80000000u;
    }
    at<uint16_t>(0x26D0) &= 0xFB7F;
    return 0;
}

// swd32.exe 0x0047F900
int BattleUnit::SetB08(int value) {
    at<int32_t>(0x2B08) = value;
    return value;
}

// swd32.exe 0x0047F910
std::int16_t BattleUnit::GetWord2A0C() { return at<int16_t>(0x2A0C); }

// swd32.exe 0x0047F920: held by mark 0x40 or by the value at +0x2AD0
int BattleUnit::IsHeld() {
    if ((at<uint8_t>(0x26D0) & 0x40) == 0) {
        return at<int32_t>(0x2AD0);
    }
    return 1;
}

// swd32.exe 0x0047C1F0: damage numbers from slot +0x2A7E up to the slot count +0x2A7C: after their delay
// (+0x26DE) the four digits (+0x2724) pop in one after another (phases +0x283C, offsets G.numberPopOffsets),
// then rise and fade out (+0x295A) and the next slot follows. A queued extra number (+0x2AE8, digits in
// G.dword_53AEDC..G.dword_53AEE4) is appended once the digits have landed. Returns 1 when all are done.
int BattleUnit::DrawDamageNumbers() {
    if (at<int32_t>(0x2AD8) == 0) {
        return 0;
    }
    const int x = at<int16_t>(0xD66);
    const int top = at<int16_t>(0xD68) - 0x1E;
    for (int slot = at<uint16_t>(0x2A7E); slot < at<uint16_t>(0x2A7C); ++slot) {
        int16_t* digits = &at<int16_t>(0x2724 + 8 * slot);
        int16_t* phases = &at<int16_t>(0x283C + 8 * slot);
        uint16_t& delay = at<uint16_t>(0x26DE + 2 * slot);
        int16_t& rise = at<int16_t>(0x295A + 2 * slot);
        const auto font = static_cast<int16_t>(at<uint16_t>(0x29C4 + 2 * slot));
        if (delay > 0) {
            --delay;
            continue;
        }
        if (phases[0] == 9 && phases[1] == 9 && phases[2] == 9 && phases[3] == 9) {
            if (at<int32_t>(0x2AE8) == 1) {
                at<int32_t>(0x2AE8) = 0;
                at<int16_t>(0x2724 + 8 * at<uint16_t>(0x2A7C)) = static_cast<int16_t>(G.dword_53AEDC);
                at<int16_t>(0x2726 + 8 * at<uint16_t>(0x2A7C)) = static_cast<int16_t>(G.dword_53AEE0);
                at<int16_t>(0x2728 + 8 * at<uint16_t>(0x2A7C)) = static_cast<int16_t>(G.dword_53AEE4);
                ++at<uint16_t>(0x2A7C);
            }
            if (rise >= 0x30) {
                phases[0] = 0;
                phases[1] = 0;
                phases[2] = 0;
                phases[3] = 0;
                rise = 0;
                G.digitMode = 0;
                ++at<uint16_t>(0x2A7E);
                std::memset(digits, 0, 8);
                continue;
            }
            if (at<int32_t>(0x2AD0) == 0) {
                rise = static_cast<int16_t>(rise + 2);
            }
            auto fade = [&rise]() {
                G.drawOption71C = -rise;
                G.drawOption30C = -rise;
                G.drawOption304 = -rise;
                G.drawScaleHeight = rise + 0x1A;
            };
            if (digits[0] != 0) {
                fade();
                DrawNumber(font, digits[0], x - 0xC, top - rise);
            }
            if (digits[0] != 0 || digits[1] != 0) {
                fade();
                DrawNumber(font, digits[1], x, top - rise);
            }
            fade();
            if (digits[0] != 0 || digits[1] != 0 || digits[2] != 0) {
                DrawNumber(font, digits[2], x + 0xC, top - rise);
            }
            DrawNumber(font, digits[3], x + 0x18, top - rise);
            G.digitMode = 0;
            continue;
        }
        G.digitMode = 0;
        if (digits[0] != 0) {
            DrawNumber(font, digits[0], x - 0xC, top - G.numberPopOffsets[phases[0]]);
        }
        if ((digits[1] != 0 || digits[0] != 0) && phases[0] > 0) {
            DrawNumber(font, digits[1], x, top - G.numberPopOffsets[phases[1]]);
        }
        if (phases[1] > 0) {
            if (digits[0] != 0 || digits[1] != 0 || digits[2] != 0) {
                DrawNumber(font, digits[2], x + 0xC, top - G.numberPopOffsets[phases[2]]);
            }
            DrawNumber(font, digits[3], x + 0x18, top - G.numberPopOffsets[phases[3]]);
        }
        if (at<int32_t>(0x2AD0) == 0) {
            for (int digit = 0; digit < 4; ++digit) {
                if (++phases[digit] > 9) {
                    phases[digit] = 9;
                }
                if (phases[digit] <= 0) {
                    break;
                }
            }
        }
    }
    if (at<uint16_t>(0x2A7E) < at<uint16_t>(0x2A7C)) {
        return 0;
    }
    G.digitMode = 0;
    at<uint16_t>(0x2A7C) = 0;
    at<uint16_t>(0x2A7E) = 0;
    at<int32_t>(0x2AD8) = 0;
    return 1;
}

// swd32.exe 0x0047BA80: one frame of the attack action (state +0x500, act of +0x2A0C): the unit steps forward by
// G.attackStepX per stage (+0x2A78, 1..5, advanced every second frame), a lunge copy (G.attackLunge read from
// +4, so stage 5 reads the bytes of G.attackStepX) is drawn for heroes, then shadow, body, highlight, effects,
// overlay and the weapon action (+0x3D0). Returns 1 when the action ended or after stage 5.
int BattleUnit::DrawAttack() {
    auto* attack = reinterpret_cast<ActState*>(raw(0x500));
    at<int32_t>(0x500) = at<uint16_t>(0x2A0C);
    if (at<int32_t>(0x2AA0) != 1 || at<int32_t>(0x2B00) != 0 || at<int32_t>(0x2B04) != 0) {
        at<int32_t>(0x508) = 0x24;
    } else {
        at<int32_t>(0x508) = 0x25;
        if ((GetGuard() == 1 || (at<uint8_t>(0x26D0) & 1) != 0) && (at<uint8_t>(0x26D0) & 0x40) == 0) {
            at<int32_t>(0x508) = 0x27;
        }
        if ((at<uint32_t>(0x26C0) & 0x1000) != 0) {
            at<int32_t>(0x508) = 0x25;
        }
    }
    at<int32_t>(0x590) = 0;
    if (at<int32_t>(0x2AD0) == 1) {
        at<int32_t>(0x590) = 1;
    }
    if (ActRun(attack) == 0) {
        return 1;
    }
    if (at<int32_t>(0x2AF4) == 1 && at<int32_t>(0x2AA0) == 0) {
        at<uint16_t>(0x54C) = at<uint16_t>(0x2EC);
    }
    if (at<int32_t>(0x2AF0) == 1) {
        return 1;
    }
    if ((at<uint8_t>(0x26C0) & 4) != 0) {
        std::memcpy(raw(0x500), raw(0x2A0), sizeof(ActState));
    }
    Picture* body = TswGetPicture(at<uint16_t>(0x54A), at<uint16_t>(0x54C));
    at<Picture*>(0x2548) = body;
    G.drawSource = reinterpret_cast<uint8_t*>(body->pixels);
    PlaySound(at<uint16_t>(0x558), G.soundVolume);
    if (at<uint16_t>(0x558) != 0) {
        SetSoundPan(at<uint16_t>(0x558), at<int32_t>(0x2B08) == 1 ? -0x10 : 0x10);
    }
    const int baseX = at<int16_t>(0xD66);
    const int originX = at<int32_t>(0x510);
    const uint16_t stage = at<uint16_t>(0x2A78);
    at<uint16_t>(0x558) = 0;
    int y = at<int16_t>(0xD68) - at<int32_t>(0x514);
    at<int32_t>(0x2670) = originX;
    const int step = G.attackStepX[stage];
    int x = baseX - originX + step;
    if (at<int32_t>(0x2B00) == 1) {
        if (at<int32_t>(0x2B08) != 0) {
            x = baseX - step - originX;
        } else {
            at<int32_t>(0x2670) = at<Picture*>(0x2548)->width - at<int32_t>(0x510);
            x = G.attackStepX[stage] - at<int32_t>(0x2670) + baseX;
        }
    }
    if (at<int32_t>(0x2B08) == 1 && at<int32_t>(0x2B00) == 0) {
        at<int32_t>(0x2670) = at<Picture*>(0x2548)->width - at<int32_t>(0x510);
        x = baseX - G.attackStepX[stage] - at<int32_t>(0x2670);
    }
    const uint32_t mirrored = at<uint32_t>(0x26C0) & 4;
    if (mirrored != 0) {
        x = baseX - at<int32_t>(0x2670);
    }
    if (at<int32_t>(0x2AA0) == 0 && mirrored == 0) {
        G.drawOption71C = 0xF;
        G.drawOption30C = 0;
        G.drawOption304 = 0;
        const uint32_t flags = at<uint32_t>(0x2694) & 0x80000003;
        at<uint32_t>(0x26A4) = flags;
        Picture* picture = at<Picture*>(0x2548);
        const int lungeNext = image::at<int32_t>(0x004A7DF4u + 4u * at<uint16_t>(0x2A78));
        if (at<int32_t>(0x2B10) == 0) {
            if (at<int32_t>(0x2B08) == 0) {
                DrawPicture(at<int16_t>(0xD66) - lungeNext - at<int32_t>(0x2670),
                            at<int16_t>(0xD68) - at<int32_t>(0x514), picture->width, picture->height,
                            static_cast<int>(flags | 0x10), 0);
                x = at<int16_t>(0xD66) - G.attackLunge[at<uint16_t>(0x2A78)] - at<int32_t>(0x2670);
            } else {
                DrawPicture(lungeNext - at<int32_t>(0x2670) + at<int16_t>(0xD66),
                            at<int16_t>(0xD68) - at<int32_t>(0x514), picture->width, picture->height,
                            static_cast<int>(flags | 0x10), 0);
                x = G.attackLunge[at<uint16_t>(0x2A78)] - at<int32_t>(0x2670) + at<int16_t>(0xD66);
            }
        } else {
            DrawPicture(at<int16_t>(0xD66) - at<int32_t>(0x2670),
                        at<int16_t>(0xD68) - lungeNext - at<int32_t>(0x514), picture->width, picture->height,
                        static_cast<int>(flags | 0x10), 0);
            x = at<int16_t>(0xD66) - at<int32_t>(0x2670);
            y = at<int16_t>(0xD68) - G.attackLunge[at<uint16_t>(0x2A78)] - at<int32_t>(0x514);
        }
    }
    G.drawScaleHeight = at<Picture*>(0x2548)->height / 3;
    G.drawOption304 = -6;
    G.drawShear = at<Picture*>(0x2548)->height >> 2;
    G.drawOption30C = -6;
    G.drawOption71C = -6;
    if (at<int32_t>(0x2B00) == 1) {
        G.drawOption304 = -1;
        G.drawOption30C = -1;
        G.drawOption71C = -1;
    }
    Picture* picture = at<Picture*>(0x2548);
    const uint32_t shadowFlags = (at<uint32_t>(0x2694) & 0x8000000F) | 0xC;
    at<uint32_t>(0x26A4) = shadowFlags;
    DrawPicture(x, at<int16_t>(0xD68) - G.drawScaleHeight, picture->width, picture->height,
                static_cast<int>(shadowFlags), 0);
    const uint32_t flags = at<uint32_t>(0x2694) & 0x80000003;
    at<uint32_t>(0x2694) = flags;
    picture = at<Picture*>(0x2548);
    DrawPicture(x, y, picture->width, picture->height, static_cast<int>(flags), 0);
    if ((at<uint8_t>(0x26C0) & 4) == 0 && at<int32_t>(0x508) != 0x27) {
        G.drawOption71C = 0x10;
        picture = at<Picture*>(0x2548);
        DrawPicture(x, y, picture->width, picture->height, static_cast<int>(at<uint32_t>(0x2694) | 0x10), 0);
    }
    DrawEffects(x, y);
    if ((at<uint8_t>(0x26C0) & 4) == 0 && at<uint16_t>(0x54E) != 0 && at<uint16_t>(0x550) != 0) {
        Picture* overlay = TswGetPicture(at<uint16_t>(0x54E), static_cast<uint16_t>(at<uint16_t>(0x550) - 1));
        at<Picture*>(0x2554) = overlay;
        G.drawSource = reinterpret_cast<uint8_t*>(overlay->pixels);
        overlay = at<Picture*>(0x2554);
        DrawPicture(at<int16_t>(0x55E) - at<int32_t>(0x2670) + at<int16_t>(0xD66),
                    at<int16_t>(0x560) - at<int32_t>(0x514) + at<int16_t>(0xD68), overlay->width, overlay->height,
                    static_cast<int>(at<uint32_t>(0x2694) | 4), 0);
    }
    if ((at<uint8_t>(0x55A) & 2) != 0) {
        at<int32_t>(0x3D8) = at<int32_t>(0x528);
        at<int32_t>(0x3D0) = at<int32_t>(0x524);
        if (ActRun(reinterpret_cast<ActState*>(raw(0x3D0))) == 0) {
            return 0;
        }
        PlaySound(at<uint16_t>(0x428), G.soundVolume);
        at<uint16_t>(0x428) = 0;
        Picture* weapon = TswGetPicture(at<uint16_t>(0x41A), at<uint16_t>(0x41C));
        at<Picture*>(0x254C) = weapon;
        G.drawSource = reinterpret_cast<uint8_t*>(weapon->pixels);
        uint32_t weaponFlags = at<uint32_t>(0x3E8);
        int weaponX = at<int32_t>(0x3E0);
        if (at<uint16_t>(0x576) != 0) {
            at<uint16_t>(0x29AC) = at<uint16_t>(0x576);
        }
        if (at<int32_t>(0x2B08) == 1) {
            weaponFlags = (weaponFlags & 1) != 0 ? weaponFlags & ~1u : weaponFlags | 1;
            weaponX = at<Picture*>(0x254C)->width - weaponX;
            at<uint16_t>(0x29AC) =
                static_cast<uint16_t>(at<Picture*>(0x2548)->width - at<uint16_t>(0x576));
        }
        weapon = at<Picture*>(0x254C);
        DrawPicture(at<int16_t>(0x29AC) - weaponX + x, at<int16_t>(0x578) - at<int32_t>(0x3E4) + y, weapon->width,
                    weapon->height, static_cast<int>(weaponFlags), 0);
    }
    if (at<int32_t>(0x2AD0) == 0) {
        if (++at<uint16_t>(0x2A66) == 2) {
            ++at<uint16_t>(0x2A78);
            at<uint16_t>(0x2A66) = 0;
        }
    }
    at<int32_t>(0x2AAC) = 1;
    if (at<uint16_t>(0x2A78) <= 5) {
        return 0;
    }
    G.word_53BF46 = 0;
    G.word_53BF48 = 0;
    at<uint16_t>(0x2A78) = 1;
    G.drawShear = 0;
    at<int32_t>(0x2AAC) = 0;
    at<int32_t>(0x2AC4) = 1;
    at<int32_t>(0x2B10) = 0;
    at<uint16_t>(0x55A) = 0;
    std::memset(raw(0x500), 0, sizeof(ActState));
    at<int32_t>(0x2A56) = -1;
    at<int32_t>(0x2A5A) = -1;
    at<int32_t>(0x2A5E) = -1;
    at<int32_t>(0x2A62) = -1;
    return 1;
}

// swd32.exe 0x0047CF20: while mark 1 is set, the four letters (frames 0..3 of digit font 0) pop in one after
// another (phases +0x2884, the later ones only advance while the first is past 0), then rise and fade out
// (+0x29A0, 4 per frame). Returns 1 when done.
int BattleUnit::DrawGuardText() {
    if ((at<uint8_t>(0x26D0) & 1) == 0) {
        return 0;
    }
    const int top = at<int16_t>(0xD68) - 0x14;
    const int left = at<int16_t>(0xD66) - 0x14;
    int16_t* phases = &at<int16_t>(0x2884);
    if (phases[0] == 9 && phases[1] == 9 && phases[2] == 9 && phases[3] == 9) {
        const int16_t rise = at<int16_t>(0x29A0);
        if (rise >= 0x18) {
            phases[0] = 0;
            phases[1] = 0;
            phases[2] = 0;
            phases[3] = 0;
            at<int16_t>(0x29A0) = 0;
            G.digitMode = 0;
            at<uint16_t>(0x26D0) &= 0xFFFE;
            at<uint16_t>(0x2A0E) = 0;
            at<int32_t>(0x26A8) = 0;
            at<int32_t>(0x26AC) = 0;
            return 1;
        }
        if (at<int32_t>(0x2AD0) == 0) {
            at<int16_t>(0x29A0) = static_cast<int16_t>(rise + 4);
        }
        uint32_t offset = 0;
        for (int letter = 0; letter < 4; ++letter) {
            const int16_t current = at<int16_t>(0x29A0);
            G.drawOption71C = -current;
            G.drawOption30C = -current;
            G.drawOption304 = -current;
            G.drawScaleHeight = current + 0x1A;
            offset += DrawDigit(0, letter, static_cast<uint16_t>(offset) + left, top - current);
        }
        return 0;
    }
    G.digitMode = 0;
    uint32_t offset = DrawDigit(0, 0, left, top - G.numberPopOffsets[phases[0]]);
    for (int letter = 1; letter < 4 && phases[0] > 0; ++letter) {
        offset += DrawDigit(0, letter, static_cast<uint16_t>(offset) + left, top - G.numberPopOffsets[phases[letter]]);
        if (at<int32_t>(0x2AD0) == 0) {
            ++phases[letter];
        }
        if (phases[letter] > 9) {
            phases[letter] = 9;
        }
    }
    if (++phases[0] > 9) {
        phases[0] = 9;
    }
    return 0;
}

namespace {

uint8_t* newEffect() {
    auto* node = static_cast<uint8_t*>(crt::malloc(0xAC));
    std::memset(node, 0, 0xAC);
    return node;
}

void setEffectIcon(uint8_t* node, int frame, int16_t icon) {
    mem::put32(node, 4, 0x2AF8);
    mem::put32(node, 0xC, static_cast<uint32_t>(frame));
    mem::put16(node, 0xA6, static_cast<uint16_t>(icon));
}

}  // namespace

// swd32.exe 0x0047DBD0: ailment `kind` (+0x18) of `skill` lands unless bit 13 (mark 0x20 instead) or the
// skill's +4 bit 7 is set, or a roll of 1..10 exceeds the resistance (hero stats +0x1C, enemy data +0x5E). The
// search for an existing effect starts with the effect fields of the unit itself (+0x2620). A refreshed or
// new effect gets the turns (+0x24) and value (+0xE); kind bit 15 adds nothing new.
int BattleUnit::AddAilment(const uint8_t* skill) {
    const uint16_t kind = mem::u16(skill, 0x18);
    auto** head = reinterpret_cast<uint8_t**>(raw(0x2584));
    if ((kind & 0x2000) != 0) {
        at<uint8_t>(0x26D0) |= 0x20;
        return 0;
    }
    if (kind == 0 || (skill[4] & 0x80) != 0) {
        return 0;
    }
    const int roll = static_cast<int>(R250Below(10)) + 1;
    if (at<int32_t>(0x2AA0) == 1) {
        if (roll > mem::u16(at<uint8_t*>(0), 0x1C)) {
            return 0;
        }
    } else if (roll > mem::u16(at<uint8_t*>(0xC), 0x5E)) {
        return 0;
    }
    bool found = false;
    for (uint8_t* node = raw(0x2584); node != nullptr; node = mem::ptr<uint8_t>(node, 0)) {
        if (mem::u32(node, 0x9C) == mem::u16(skill, 0x18)) {
            mem::put16(node, 0xA2, skill[0x24]);
            mem::put16(node, 0xA8, mem::u16(skill, 0xE));
            found = true;
            break;
        }
    }
    if ((skill[0x19] & 0x80) != 0 || found) {
        return 1;
    }
    uint8_t* node = newEffect();
    mem::putPtr(node, 0, *head);
    *head = node;
    mem::put16(node, 0xA2, skill[0x24]);
    mem::put32(node, 0x9C, mem::u16(skill, 0x18));
    mem::put16(node, 0xA8, mem::u16(skill, 0xE));
    uint32_t& status = at<uint32_t>(0x26C0);
    switch (mem::u16(skill, 0x18)) {
        case 1:
            setEffectIcon(node, 1, 2);
            status |= 1;
            at<uint8_t>(0x26D0) |= 0x40;
            break;
        case 2:
            setEffectIcon(node, 7, 8);
            status |= 2;
            at<uint16_t>(0x26D0) |= 2;
            break;
        case 4:
            status |= 4;
            mem::put16(node, 0xA6, 3);
            at<uint8_t>(0x26D0) |= 0x40;
            break;
        case 8:
            setEffectIcon(node, 0, 4);
            status |= 8;
            at<uint8_t>(0x26D0) |= 0x40;
            break;
        case 0x10:
            setEffectIcon(node, 5, 5);
            status |= 0x10;
            at<uint8_t>(0x26D1) |= 1;
            break;
        case 0x20:
            setEffectIcon(node, 4, 9);
            status |= 0x20;
            break;
        case 0x40:
            setEffectIcon(node, 3, 6);
            status |= 0x40;
            break;
        case 0x80:
            setEffectIcon(node, 2, 7);
            status |= 0x80;
            break;
        case 0x100:
            setEffectIcon(node, 8, 0xA);
            status |= 0x100;
            break;
        case 0x200:
            setEffectIcon(node, 6, 0xB);
            status |= 0x200;
            if (at<int32_t>(0x2AA0) == 1) {
                uint8_t* stats = at<uint8_t*>(0);
                for (const std::intptr_t offset : {0x26, 0x28, 0x16, 0x14}) {
                    const uint16_t value = mem::u16(stats, offset);
                    mem::put16(stats, offset, static_cast<uint16_t>(value - 10 * value / 100));
                }
            }
            break;
        case 0x1000:
            status |= 0x1000;
            mem::put16(node, 0xA6, 1);
            break;
        default:
            break;
    }
    return 1;
}

// swd32.exe 0x0047E070: buff of `skill` (status bits +8, element +0x1A when +4 bit 3): 0x4000000 is the hit
// barrier (turns in +0x2A90); otherwise an effect with the same bits or element is refreshed or a new one is
// appended (the search starts with the effect fields of the unit itself), then it is announced.
int BattleUnit::AddBuff(const uint8_t* skill) {
    const uint32_t bits = mem::u32(skill, 8);
    if (bits == 0 && (skill[4] & 8) == 0) {
        return 0;
    }
    if ((bits & 0x4000000) != 0) {
        at<uint32_t>(0x26C0) |= 0x2000000;
        at<uint16_t>(0x2A90) = skill[0x24];
        return 1;
    }
    uint8_t* node = raw(0x2584);
    bool refreshed = false;
    for (;;) {
        const uint32_t kind = mem::u32(node, 0x9C);
        if (kind != 0 && kind == bits) {
            refreshed = true;
            break;
        }
        const uint16_t element = mem::u16(node, 0xA4);
        if (element != 0 && element == mem::u32(skill, 4)) {
            refreshed = true;
            break;
        }
        if (mem::ptr<uint8_t>(node, 0) == nullptr) {
            break;
        }
        node = mem::ptr<uint8_t>(node, 0);
    }
    if (refreshed) {
        mem::put16(node, 0xA2, skill[0x24]);
    } else {
        uint8_t* added = newEffect();
        mem::putPtr(node, 0, added);
        node = added;
        mem::putPtr(node, 0, static_cast<uint8_t*>(nullptr));
        mem::put16(node, 0xA2, skill[0x24]);
        mem::put16(node, 0xA8, mem::u16(skill, 0xE));
        if (mem::u32(skill, 8) != 0) {
            mem::put32(node, 0x9C, mem::u32(skill, 8));
        }
        if ((skill[4] & 8) != 0) {
            mem::put16(node, 0xA4, mem::u16(skill, 0x1A));
        }
    }
    const char* name = at<const char*>(0x2560);
    const char* resistUp = nullptr;
    switch (mem::u16(skill, 0x1A)) {
        case 1: resistUp = G.fireResistUpText; break;
        case 2: resistUp = G.waterResistUpText; break;
        case 5: resistUp = G.poisonResistUpText; break;
        case 6: resistUp = G.lightResistUpText; break;
        case 7: resistUp = G.darkResistUpText; break;
        case 9: resistUp = G.physicalResistUpText; break;
        default: break;
    }
    if (resistUp != nullptr) {
        announceWithSound(resistUp, name, kMessageBlackBar | kMessageCentered);
    }
    uint32_t& status = at<uint32_t>(0x26C0);
    const char* text = nullptr;
    switch (mem::u32(skill, 8)) {
        case 0x80000:
            mem::put32(node, 0xC, 0);
            status |= 0x100000;
            text = G.defenseDownText;
            break;
        case 0x100000:
            mem::put32(node, 0xC, 0);
            status |= 0x8000000;
            text = G.speedDownText;
            break;
        case 0x2000000:
            mem::put32(node, 0xC, 0);
            status |= 0x4000000;
            if (at<int32_t>(0x2AA0) != 0) {
                text = G.barrierStartText;
            }
            break;
        case 0x8000000:
            mem::put32(node, 0xC, 0);
            status |= 0x1000000;
            text = G.attackDownText;
            break;
        case 0x10000000:
            mem::put32(node, 0xC, 0);
            at<uint8_t>(0x26D0) |= 0x80;
            if (at<int32_t>(0x2AA0) != 0) {
                text = G.invisibleStartText;
            }
            break;
        case 0x20000000:
            mem::put32(node, 0xC, 0);
            status |= 0x40000000;
            text = G.defenseUpText;
            break;
        case 0x40000000:
            mem::put32(node, 0xC, 0);
            status |= 0x20000000;
            text = G.speedUpText;
            break;
        case 0x80000000:
            mem::put32(node, 0xC, 0);
            status |= 0x10000000;
            text = G.attackUpText;
            break;
        default:
            break;
    }
    if (text != nullptr) {
        announceWithSound(text, name, kMessageBlackBar | kMessageCentered);
    }
    return 1;
}

// swd32.exe 0x004800F0: appends a 0x1DC byte action to the list at +0x236C and counts it in +0x2A80
uint8_t* BattleUnit::QueueAction(int kind, int target, int16_t x, int16_t y, int16_t width, int16_t height,
                                 int16_t delay, int16_t speed, int data) {
    auto** link = reinterpret_cast<uint8_t**>(raw(0x236C));
    while (*link != nullptr) {
        link = reinterpret_cast<uint8_t**>(*link);
    }
    auto* action = static_cast<uint8_t*>(crt::malloc(0x1DC));
    *link = action;
    std::memset(action, 0, 0x1DC);
    action = *link;
    mem::put32(action, 4, static_cast<uint32_t>(kind));
    mem::put32(action, 0xC, static_cast<uint32_t>(target));
    mem::put32(action, 0x9C, static_cast<uint32_t>(data));
    mem::put16(action, 0x1D0, static_cast<uint16_t>(x));
    mem::put16(action, 0x1D2, static_cast<uint16_t>(y));
    mem::put16(action, 0x1CC, static_cast<uint16_t>(width));
    mem::put16(action, 0x1CE, static_cast<uint16_t>(height));
    mem::put16(action, 0x1D4, static_cast<uint16_t>(speed));
    mem::put16(action, 0x1D6, static_cast<uint16_t>(delay));
    action[0x1DA] = 0;
    ++at<uint16_t>(0x2A80);
    return action;
}

// swd32.exe 0x004801A0: marks the last queued action (the list head fields of the unit when the list is empty)
// as final and, once, gives it the follow-up (first, second, third) with eight empty hit slots
uint8_t* BattleUnit::FinishQueuedAction(int first, int second, int third) {
    uint8_t* last = raw(0x236C);
    for (uint8_t* node = mem::ptr<uint8_t>(last, 0); node != nullptr; node = mem::ptr<uint8_t>(node, 0)) {
        last = node;
    }
    last[0x1DB] = 1;
    if (mem::u16(last, 0x1C6) != 0) {
        return last;
    }
    mem::put16(last, 0x1C6, 1);
    for (int slot = 0; slot < 8; ++slot) {
        mem::put16(last, 0x138 + 0x10 * slot, 0xFFFF);
    }
    mem::put32(last, 0x1B4, static_cast<uint32_t>(first));
    mem::put32(last, 0x1B8, static_cast<uint32_t>(second));
    mem::put32(last, 0x1BC, static_cast<uint32_t>(third));
    mem::put16(last, 0x1C4, 2);
    last[0x1C8] = 3;
    return last;
}

// swd32.exe 0x00480AD0
int BattleUnit::GetData0C() { return at<int32_t>(0xC); }

// swd32.exe 0x00480AE0: the cast action (act +0x2C4, action +0x2C8, state +0x3D0) started by flag 2 of the
// main action; flag 0x200 of it pauses the main action. Returns 0 while the cast state stops, else 1.
int BattleUnit::DrawCastAction() {
    const uint16_t mainFlags = at<uint16_t>(0x2FA);
    if ((mainFlags & 2) != 0) {
        if (at<int32_t>(0x2C4) != 0) {
            at<uint32_t>(0x267C) |= 0x4000;
        }
        if (at<uint16_t>(0xDA2) > 0) {
            at<uint32_t>(0x267C) |= 0x8000;
        }
        if ((mainFlags & 0x200) != 0) {
            at<int32_t>(0x330) = 1;
        }
        at<uint16_t>(0x2FA) = mainFlags & 0xFFFD;
        std::memset(raw(0x3D0), 0, sizeof(ActState));
    }
    if ((at<uint32_t>(0x267C) & 0x4000) == 0) {
        return 1;
    }
    at<int32_t>(0x3D8) = at<int32_t>(0x2C8);
    at<int32_t>(0x3D0) = at<int32_t>(0x2C4);
    const int running = ActRun(reinterpret_cast<ActState*>(raw(0x3D0)));
    if (running == 0) {
        return running;
    }
    PlaySound(at<uint16_t>(0x428), G.soundVolume);
    if (at<uint16_t>(0x428) != 0) {
        SetSoundPan(at<uint16_t>(0x428), at<int16_t>(0xD66) < 0x140 ? -0x10 : 0x10);
    }
    at<uint16_t>(0x428) = 0;
    Picture* picture = TswGetPicture(at<uint16_t>(0x41A), at<uint16_t>(0x41C));
    at<Picture*>(0x254C) = picture;
    G.drawSource = reinterpret_cast<uint8_t*>(picture->pixels);
    uint32_t flags = at<uint32_t>(0x3E8);
    int originX = at<int32_t>(0x3E0);
    if (at<int32_t>(0x2B08) == 1) {
        flags = (flags & 1) != 0 ? flags & ~1u : flags | 1;
        if (originX != 0) {
            originX = at<Picture*>(0x254C)->width - originX;
        }
    }
    picture = at<Picture*>(0x254C);
    const int x = at<int16_t>(0xD66) - at<int16_t>(0x29B2) + at<int16_t>(0x29AC) - originX;
    const int y = at<int16_t>(0xD68) - at<int32_t>(0x3E4) + at<int16_t>(0x318) - at<int32_t>(0x2B4);
    DrawPicture(x, y, picture->width, picture->height, static_cast<int>(flags), 0);
    if (at<int32_t>(0x45C) != 1) {
        return 1;
    }
    at<uint16_t>(0x2FA) &= 0xFDFF;
    at<uint32_t>(0x267C) &= ~0x4000u;
    at<uint16_t>(0x2954) = 0;
    at<uint16_t>(0x29AC) = 0;
    at<int32_t>(0x2ACC) = 0;
    at<int32_t>(0x330) = 0;
    at<int32_t>(0x2C4) = 0;
    std::memset(raw(0x3D0), 0, sizeof(ActState));
    return 1;
}

// swd32.exe 0x00480D40: the follow-up action of act +0xDA2 (state +0x500) while flag 0x8000 of +0x267C is set;
// its end clears the action flags. Returns 0 while the state stops, else 1.
int BattleUnit::DrawFollowUpAction() {
    if ((at<uint32_t>(0x267C) & 0x8000) == 0) {
        return 1;
    }
    at<int32_t>(0x500) = at<uint16_t>(0xDA2);
    at<int32_t>(0x508) = 0;
    const int running = ActRun(reinterpret_cast<ActState*>(raw(0x500)));
    if (running == 0) {
        return running;
    }
    PlaySound(at<uint16_t>(0x558), G.soundVolume);
    if (at<uint16_t>(0x558) != 0) {
        SetSoundPan(at<uint16_t>(0x558), at<int16_t>(0xD66) < 0x140 ? -0x10 : 0x10);
    }
    at<uint16_t>(0x558) = 0;
    Picture* picture = TswGetPicture(at<uint16_t>(0x54A), at<uint16_t>(0x54C));
    at<Picture*>(0x254C) = picture;
    G.drawSource = reinterpret_cast<uint8_t*>(picture->pixels);
    uint32_t flags = at<uint32_t>(0x518);
    int originX = at<int32_t>(0x510);
    if (at<int32_t>(0x2B08) == 1) {
        flags = (flags & 1) != 0 ? flags & ~1u : flags | 1;
        originX = at<Picture*>(0x254C)->width - originX;
    }
    picture = at<Picture*>(0x254C);
    DrawPicture(at<int16_t>(0xD66) - originX, at<int16_t>(0xD68) - at<int32_t>(0x514), picture->width,
                picture->height, static_cast<int>(flags), 0);
    if (at<int32_t>(0x58C) != 1) {
        return 1;
    }
    at<uint32_t>(0x267C) = 0;
    at<uint16_t>(0x2954) = 0;
    at<uint16_t>(0x2FA) = 0;
    at<uint16_t>(0x29AC) = 0;
    at<int32_t>(0x2ACC) = 0;
    at<int32_t>(0x330) = 0;
    at<int32_t>(0x2C4) = 0;
    std::memset(raw(0x500), 0, sizeof(ActState));
    return 1;
}

// swd32.exe 0x00480EE0: defence (hero stats +0x28, enemy data +0x58) with +30% (status 0x40000000), -30%
// (0x100000) and, for heroes, +70% (barrier 0x4000000). For an enemy without either effect the high half of the
// result is the high half of +0x2AA0 (the register it was loaded from).
uint32_t BattleUnit::Defense() {
    uint32_t up = 0;
    uint32_t barrier = 0;
    uint32_t down = 0;
    const uint32_t kind = at<uint32_t>(0x2AA0);
    const uint32_t status = at<uint32_t>(0x26C0);
    if (kind == 1) {
        const uint32_t base = mem::u16(at<uint8_t*>(0), 0x28);
        if ((status & 0x40000000) != 0) {
            up = 30 * base / 100;
        }
        if ((status & 0x100000) != 0) {
            down = 30 * base / 100;
        }
        if ((status & 0x4000000) != 0) {
            barrier = 70 * base / 100;
        }
        return base - down + barrier + up;
    }
    uint32_t high = kind & 0xFFFF0000u;
    const uint32_t base = mem::u16(at<uint8_t*>(0xC), 0x58);
    if ((status & 0x40000000) != 0) {
        up = 30 * base / 100;
        high = 0;
    }
    if ((status & 0x100000) != 0) {
        down = 30 * base / 100;
        high = 0;
    }
    return (high | base) - down + up;
}

// swd32.exe 0x00481FB0: 1 while mark 0x100 is clear
uint32_t BattleUnit::Mark100Clear() { return (static_cast<uint32_t>(~at<uint16_t>(0x26D0)) >> 8) & 1; }

// swd32.exe 0x00481FC0
int BattleUnit::Mark2() { return (at<uint8_t>(0x26D0) >> 1) & 1; }

// swd32.exe 0x00481FD0: action flag 0x10 resets the x of the caller (0x280 for enemies and mirrored heroes when
// flag 1 of +0xD9C is set), action flag 0x20 its y. The return value is unused by callers.
void BattleUnit::ResetActionPosition(uint16_t* x, uint16_t* y) {
    if ((at<uint8_t>(0xD94) & 0x10) != 0) {
        *x = 0;
        if ((at<uint8_t>(0xD9C) & 1) != 0) {
            if (at<int32_t>(0x2AA0) == 1 && at<int32_t>(0x2B08) == 1) {
                *x = 0x280;
            }
            if (at<int32_t>(0x2AA0) == 0) {
                *x = 0x280;
            }
        }
    }
    if ((at<uint8_t>(0xD94) & 0x20) != 0) {
        *y = 0;
    }
}

// swd32.exe 0x00482030: ends the main action (state +0x2A0) once it finished (+0x32C) or when forced
int BattleUnit::EndAction(int16_t next, int force) {
    if (at<int32_t>(0x32C) != 1 && force != 1) {
        return 0;
    }
    std::memset(raw(0x2A0), 0, sizeof(ActState));
    at<int32_t>(0x45C) = 1;
    at<int16_t>(0x26D2) = next;
    return 1;
}

// swd32.exe 0x00482080
int16_t BattleUnit::SetWords26D6(int16_t first, int16_t second) {
    at<int16_t>(0x26D6) = first;
    at<int16_t>(0x26D8) = second;
    return first;
}

// swd32.exe 0x004820A0: afterimages of picture `id` kept in `trail` (16 byte entries: flags, frame, fade, x, y;
// +0x80 tint, +0x90 layout, +0x94 count, +0x95 fade step, +0x96 delay): every third call the entries move down
// and the current frame is added; each entry fades and is drawn translucent (tinted when fading above 1), or
// darkened with flag 4. An entry is still drawn on the frame it fades out. The return value is unused.
void BattleUnit::DrawAfterimages(uint8_t* trail, int id, int16_t frame, uint32_t flags, int16_t x, int16_t y) {
    int spacing = 0x1E;
    if (mem::u16(trail, 0x90) == 1) {
        spacing = -0x1E;
    }
    if (trail[0x96] != 0) {
        --trail[0x96];
    } else {
        for (int entry = static_cast<int8_t>(trail[0x94]) - 1; entry > 0; --entry) {
            std::memcpy(trail + 0x10 * entry, trail + 0x10 * (entry - 1), 0x10);
        }
        mem::put16(trail, 6, 0xC);
        mem::put16(trail, 4, static_cast<uint16_t>(frame));
        if ((flags & 4) != 0) {
            mem::put16(trail, 6, 0x18);
        }
        mem::put32(trail, 0, flags);
        mem::put16(trail, 8, static_cast<uint16_t>(x));
        mem::put16(trail, 0xA, static_cast<uint16_t>(y));
        trail[0x96] = 2;
    }
    for (int index = 0; index < static_cast<int8_t>(trail[0x94]); ++index) {
        uint8_t* entry = trail + 0x10 * index;
        const uint16_t entryFrame = mem::u16(entry, 4);
        if (entryFrame == 0xFFFF) {
            continue;
        }
        int offset = 0xC - mem::s16(entry, 8);
        const uint16_t layout = mem::u16(trail, 0x90);
        if (layout == 1) {
            offset = mem::s16(entry, 0xA) - 0xC;
        }
        if (layout == 2) {
            offset = 0;
            spacing = 0;
        }
        Picture* picture = TswGetPicture(static_cast<uint32_t>(id), entryFrame);
        at<Picture*>(0x2550) = picture;
        G.drawSource = reinterpret_cast<uint8_t*>(picture->pixels);
        int16_t fade = mem::s16(entry, 6);
        if (fade > 0) {
            fade = static_cast<int16_t>(fade - static_cast<int8_t>(trail[0x95]));
            mem::put16(entry, 6, static_cast<uint16_t>(fade));
            if (fade <= 0) {
                mem::put16(entry, 4, 0xFFFF);
                mem::put16(entry, 6, 0);
            }
        }
        fade = mem::s16(entry, 6);
        const int drawX = mem::s16(entry, 8) + spacing * index + offset;
        const int drawY = mem::s16(entry, 0xA);
        if ((flags & 4) != 0) {
            G.drawOption71C = fade - 0x1F;
            G.drawOption304 = -0xC;
            G.drawOption30C = fade - 0x1F;
            DrawPicture(drawX, drawY, at<Picture*>(0x2550)->width, at<Picture*>(0x2550)->height,
                        static_cast<int>(flags), 0);
            continue;
        }
        G.drawAlpha = fade;
        DrawPicture(drawX, drawY, at<Picture*>(0x2550)->width, at<Picture*>(0x2550)->height,
                    static_cast<int>(flags | 0x14), 0);
        if (mem::s16(entry, 6) > 1) {
            G.drawOption71C = mem::s32(trail, 0x80);
            G.drawOption30C = mem::s32(trail, 0x84);
            G.drawOption304 = mem::s32(trail, 0x88);
            DrawPicture(drawX, drawY, at<Picture*>(0x2550)->width, at<Picture*>(0x2550)->height,
                        static_cast<int>(flags | 0x10), 0);
        }
    }
}

// swd32.exe 0x00482DA0: fills the gauge (*+8, full at 0x38) by amount / (speed +4 +0xA) * 47 of it; -1 fills it,
// 0 empties it. The carry at +0x26B0 is meant for amounts rounding to 0 but is compared before rounding, so it
// is always reset. Plays sound 0xBC when full.
void BattleUnit::AddGauge(int amount) {
    if (at<int32_t>(0x2B00) == 1 || at<int32_t>(0x2B04) == 1) {
        return;
    }
    if (amount == -1) {
        *at<int32_t*>(8) = 0x38;
    } else if (amount == 0) {
        *at<int32_t*>(8) = 0;
    }
    int32_t* gauge = at<int32_t*>(8);
    const int32_t level = *gauge;
    if (level >= 0x38 || amount <= 0) {
        return;
    }
    const int speed = mem::s16(at<uint8_t*>(4), 0xA);
    if (amount < speed) {
        const int32_t carry = at<int32_t>(0x26B0);
        if (carry > 0) {
            amount += carry;
        }
        const double share = static_cast<double>(amount) / static_cast<double>(speed) * 47.0;
        *gauge = level + static_cast<int32_t>(crt::ftol(share));
        if (share == 0.0) {
            at<int32_t>(0x26B0) += amount;
        } else {
            at<int32_t>(0x26B0) = 0;
        }
    } else {
        *gauge = 0x38;
    }
    int32_t* full = at<int32_t*>(8);
    if (*full >= 0x38) {
        *full = 0x38;
        PlaySound(0xBC, G.soundVolume);
    }
}

// swd32.exe 0x00482E80
bool BattleUnit::GaugeIsFull() { return *at<int32_t*>(8) == 0x38; }

// swd32.exe 0x00482E90: guard kind of the action flags (+0xD94): 0x100 gives 8 with status bit 5 of +0x26C8,
// else 1; 0x200 gives 2; otherwise flag 0x400 gives 4. The high half of the result is left from the flags.
uint32_t BattleUnit::GuardKind() {
    const uint32_t flags = at<uint32_t>(0xD94);
    if ((flags & 0x100) != 0) {
        return (at<uint8_t>(0x26C8) & 0x20) != 0 ? 8 : 1;
    }
    if ((flags & 0x200) != 0) {
        return (flags & 0xFFFF0000u) | 2;
    }
    const uint32_t shifted = flags >> 8;
    return (shifted & 0xFFFF0000u) | (shifted & 4);
}

// swd32.exe 0x00482EC0: whether the main action is on frame `frame` + 1, or finished for frame -1
bool BattleUnit::IsActionFrame(int16_t frame) {
    if (at<uint16_t>(0x2A0E) == 0) {
        return false;
    }
    if (frame == -1) {
        return at<int32_t>(0x32C) == 1;
    }
    return at<uint16_t>(0x2EC) == frame - 1;
}

// swd32.exe 0x00482F70: low 14 bits of +0x198 of the first of the two 0xA4 byte records (+0x1A0) with id `id`
uint32_t BattleUnit::RecordValue(int id) {
    for (int record = 0; record < 2; ++record) {
        if (at<int16_t>(0x1A0 + 0xA4 * record) == id) {
            return at<uint16_t>(0x198 + 0xA4 * record) & 0x3FFF;
        }
    }
    return 0;
}

}  // namespace swd3
