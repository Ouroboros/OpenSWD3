#include "battle/battle_enemy.h"

#include <cstring>

#include "core/random.h"
#include "crt/crt.h"

namespace swd3 {

using std::int16_t;
using std::uint16_t;
using std::uint32_t;
using std::uint8_t;

// swd32.exe 0x00475560
BattleEnemy::BattleEnemy() {
    auto* data = static_cast<uint8_t*>(crt::malloc(0xA4));
    at<uint8_t*>(0xC) = data;
    std::memset(data, 0, 0xA4);
}

// swd32.exe 0x00475590
BattleEnemy::~BattleEnemy() { FreeData(); }

// swd32.exe 0x00476A60
int BattleEnemy::FreeData() {
    uint8_t* data = at<uint8_t*>(0xC);
    if (data == nullptr) {
        return 0;
    }
    crt::free(data);
    at<uint8_t*>(0xC) = nullptr;
    return 0;
}

// swd32.exe 0x00475870
uint16_t* BattleEnemy::GetDropInfo(uint16_t* first, uint16_t* second) {
    *first = mem::u16(at<uint8_t*>(0xC), 0x62);
    *second = mem::u16(at<uint8_t*>(0xC), 0x8A);
    return second;
}

// swd32.exe 0x00476140
int BattleEnemy::StatusFlag1000Or10000000() {
    if ((at<uint32_t>(0xD98) & 0x10000000) != 0) {
        return 1;
    }
    return (at<uint32_t>(0xD94) >> 12) & 1;
}

// swd32.exe 0x004762F0: the item +0x82 of the data when a roll of 0..19 is below +0x84
uint16_t BattleEnemy::RollDrop() {
    if (mem::u16(at<uint8_t*>(0xC), 0x82) == 0) {
        return 0;
    }
    const auto roll = static_cast<uint16_t>(R250Below(0x14));
    const uint8_t* data = at<uint8_t*>(0xC);
    if (roll >= mem::u16(data, 0x84)) {
        return 0;
    }
    return mem::u16(data, 0x82);
}

// swd32.exe 0x004768D0: flag 0x800 (without 0x20) up to level 0x15
bool BattleEnemy::IsCatchable() {
    const uint8_t* data = at<uint8_t*>(0xC);
    const uint32_t flags = mem::u32(data, 0x20);
    if ((flags & 0x20) != 0) {
        return false;
    }
    if ((flags & 0x800) != 0) {
        return mem::u16(data, 0x52) <= 0x15;
    }
    return false;
}

// swd32.exe 0x00476900
int16_t BattleEnemy::GetCatchInfo(uint16_t* act, uint16_t* frame) {
    *act = at<uint16_t>(0xDB0);
    const uint16_t value = at<uint8_t>(0xDB4);
    *frame = value;
    return static_cast<int16_t>(value);
}

// swd32.exe 0x00476920: the low bytes of nine words become the element resistances (+0x92 of the data)
const uint8_t* BattleEnemy::SetResistances(const uint8_t* tenths) {
    for (int element = 0; element < 9; ++element) {
        at<uint8_t*>(0xC)[0x92 + element] = tenths[2 * element];
    }
    return tenths;
}

// swd32.exe 0x004769A0: non-zero values replace the words +0x66..+0x70 of the data
int16_t BattleEnemy::SetStats66(int16_t first, int16_t second, int16_t third, int16_t fourth, int16_t fifth,
                                int16_t sixth) {
    const int16_t values[6] = {first, second, third, fourth, fifth, sixth};
    for (int i = 0; i < 6; ++i) {
        if (values[i] != 0) {
            mem::put16(at<uint8_t*>(0xC), 0x66 + 2 * i, static_cast<uint16_t>(values[i]));
        }
    }
    return sixth;
}

// swd32.exe 0x00476A10: non-zero values replace +0x72, +0x74, +0x76 and again +0x74 (fourth) of the data
int16_t BattleEnemy::SetStats72(int16_t first, int16_t second, int16_t third, int16_t fourth) {
    if (first != 0) {
        mem::put16(at<uint8_t*>(0xC), 0x72, static_cast<uint16_t>(first));
    }
    if (second != 0) {
        mem::put16(at<uint8_t*>(0xC), 0x74, static_cast<uint16_t>(second));
    }
    if (third != 0) {
        mem::put16(at<uint8_t*>(0xC), 0x76, static_cast<uint16_t>(third));
    }
    if (fourth != 0) {
        mem::put16(at<uint8_t*>(0xC), 0x74, static_cast<uint16_t>(fourth));
    }
    return fourth;
}

}  // namespace swd3
