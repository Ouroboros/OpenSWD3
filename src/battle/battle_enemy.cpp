#include "battle/battle_enemy.h"

#include <cstring>

#include "audio/audio.h"
#include "core/random.h"
#include "draw/draw.h"
#include "res/act_anim.h"
#include "res/tsw.h"
#include "crt/crt.h"
#include "data/globals.h"
#include "swd3/w32.h"

namespace swd3 {

using std::int16_t;
using std::int32_t;
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

// swd32.exe 0x004755E0: as BattleHero::StepGauge with the speed +0x5A of the data; invisibility costs a tenth
int BattleEnemy::StepGauge(int quick) {
    const uint16_t marks = at<uint16_t>(0x26D0);
    if ((marks & 0x40) != 0 || (marks & 0x4000) != 0) {
        return 0;
    }
    const uint16_t gauge = at<uint16_t>(0x2A12);
    if (static_cast<int>(gauge) >= G.gaugeFull) {
        at<int32_t>(0x2AB0) = 1;
        at<int32_t>(0x2AEC) = 0;
        if (at<int32_t>(0x2B20) == 1) {
            at<int32_t>(0x2B20) = 0;
            at<int32_t>(0x2B08) = 0;
        }
        return 1;
    }
    int step = mem::u16(at<uint8_t*>(0xC), 0x5A) >> 2;
    if ((at<uint8_t>(0x26C0) & 0x40) != 0) {
        step >>= 1;
    }
    if (quick == 1) {
        step += step / 4 + 1;
    }
    const uint32_t status = at<uint32_t>(0x26C0);
    int up = 0;
    int down = 0;
    if ((status & 0x20000000) != 0) {
        up = 30 * step / 100;
    }
    if ((status & 0x8000000) != 0) {
        down = 30 * step / 100;
    }
    if ((status & 0x80000000) != 0) {
        down += 10 * step / 100;
    }
    at<int32_t>(0x2AB0) = 0;
    at<uint16_t>(0x2A12) = static_cast<uint16_t>(gauge - down + up + step);
    return 0;
}

// swd32.exe 0x00476080: below a third of its hit points the enemy heals itself (+0x2A9C: 1 with the item +0x7C
// on a roll of 0..4, 2 with +0x80 on 5..9) when it may (+0x8E, not mark 0x100)
int BattleEnemy::ChooseHealing() {
    const auto roll = static_cast<uint16_t>(R250Below(10));
    int choice = roll;
    if (roll > 4) {
        if (roll <= 9) {
            choice = 1;
        }
    } else {
        choice = 0;
    }
    const uint8_t* data = at<uint8_t*>(0xC);
    int32_t hitPoints = mem::s16(data, 0x64);
    if (at<int32_t>(0x26B4) != 0) {
        hitPoints = at<int32_t>(0x26B4);
    }
    if (hitPoints >= mem::s32(data, 0x4C) / 3) {
        return 0;
    }
    const bool allowed = (at<uint8_t>(0x26D1) & 1) == 0 && data[0x8E] != 0;
    if (choice == 0) {
        if (mem::u16(data, 0x7C) != 0 && allowed) {
            at<uint8_t>(0x2A9C) = 1;
            return 1;
        }
    } else if (choice == 1 && mem::u16(data, 0x80) != 0 && allowed) {
        at<uint8_t>(0x2A9C) = 2;
        return 1;
    }
    return 0;
}

// swd32.exe 0x00476330: whether a hero of `level` steals from the enemy (chance +0x91 of the data): more than 10
// levels above 80%, 6..10 above 50%, exactly 5 above chance / 10, otherwise never. A first roll of 0..11 is
// discarded.
bool BattleEnemy::RollSteal(uint8_t level) {
    if ((at<uint8_t>(0x26D1) & 8) != 0) {
        return false;
    }
    R250Below(0xC);
    const uint8_t chance = at<uint8_t*>(0xC)[0x91];
    if (chance == 0) {
        return false;
    }
    const int difference = level - mem::u16(at<uint8_t*>(0xC), 0x54);
    if (difference > 0xA) {
        return static_cast<uint16_t>(R250Below(10)) < 8;
    }
    if (difference > 5) {
        return static_cast<uint16_t>(R250Below(10)) < 5;
    }
    if (difference < 5) {
        return false;
    }
    return static_cast<uint16_t>(R250Below(10)) < at<uint8_t*>(0xC)[0x91];
}

namespace {

w32::HANDLE toHandle(std::int32_t value) {
    return static_cast<w32::HANDLE>(reinterpret_cast<void*>(static_cast<std::uintptr_t>(static_cast<std::uint32_t>(value))));
}

}  // namespace

// swd32.exe 0x00476A80: reads the script of skill `id` from mon.dat (kept open) into `skill`: word opcodes with
// operands until 5; the leading 0 word (required) is itself run as opcode 0, unknown opcodes take no operand
int LoadMonsterSkill(uint8_t* skill, uint16_t id) {
    w32::HANDLE file;
    if (G.monsterFileOpen != 0) {
        file = toHandle(G.monsterFile);
    } else {
        w32::lstrcpyA(G.pathBuffer, G.gameDir);
        w32::lstrcatA(G.pathBuffer, "mon.dat");
        file = w32::CreateFileA(G.pathBuffer, w32::GENERIC_READ, 1, nullptr, w32::OPEN_ALWAYS,
                                w32::FILE_ATTRIBUTE_NORMAL, nullptr);
        G.monsterFile = static_cast<std::int32_t>(reinterpret_cast<std::uintptr_t>(file));
        if (file == w32::INVALID_HANDLE_VALUE) {
            return 0;
        }
        G.monsterFileOpen = 1;
    }
    uint32_t offset = 0;
    w32::DWORD read = 0;
    w32::SetFilePointer(file, 0x204, nullptr, w32::FILE_BEGIN);
    w32::ReadFile(toHandle(G.monsterFile), &offset, 4, &read, nullptr);
    w32::SetFilePointer(toHandle(G.monsterFile), static_cast<w32::LONG>(offset + 4u * id + 0x200), nullptr,
                        w32::FILE_BEGIN);
    w32::ReadFile(toHandle(G.monsterFile), &offset, 4, &read, nullptr);
    w32::SetFilePointer(toHandle(G.monsterFile), static_cast<w32::LONG>(offset + 0x200), nullptr, w32::FILE_BEGIN);
    auto* script = static_cast<uint8_t*>(crt::malloc(0x400));
    std::memset(script, 0, 0x400);
    w32::ReadFile(toHandle(G.monsterFile), script, 0x400, &read, nullptr);
    if (mem::u16(script, 0) != 0) {
        crt::free(script);
        return 0;
    }
    const uint8_t* code = script;
    for (;;) {
        const uint16_t opcode = mem::u16(code, 0);
        code += 2;
        if (opcode == 5) {
            break;
        }
        switch (opcode) {
            case 0:
                mem::put32(skill, 0xC, mem::u32(code, 0));
                mem::put32(skill, 0x10, mem::u32(code, 4));
                code += 8;
                break;
            case 2:
                skill[0x24] = code[0];
                code += 2;
                break;
            case 3:
                mem::ref<uint32_t>(skill, 4) |= 1;
                break;
            case 4:
                mem::put16(skill, 0x16, mem::u16(code, 0));
                code += 2;
                break;
            case 6:
                mem::put16(skill, 0x18, mem::u16(code, 0));
                code += 2;
                mem::ref<uint32_t>(skill, 4) |= 0x80;
                break;
            case 7:
                mem::put16(skill, 0x18, mem::u16(code, 0));
                code += 2;
                break;
            case 8:
                mem::put16(skill, 0x14, mem::u16(code, 0));
                code += 2;
                break;
            case 9:
                mem::ref<uint32_t>(skill, 4) |= 2;
                break;
            case 0xA:
                mem::put16(skill, 0x1E, mem::u16(code, 0));
                code += 2;
                skill[0x1F] |= 0x80;
                break;
            case 0xB:
                mem::ref<uint32_t>(skill, 4) |= 4;
                break;
            case 0xC:
                mem::put32(skill, 8, mem::u32(code, 0));
                code += 4;
                break;
            case 0xD:
                mem::put16(skill, 0x1C, mem::u16(code, 0));
                code += 2;
                break;
            case 0xE:
                mem::ref<uint32_t>(skill, 4) |= 8;
                mem::put16(skill, 0x1A, mem::u16(code, 0));
                code += 2;
                break;
            case 0xF:
                mem::ref<uint32_t>(skill, 4) |= 0x10;
                break;
            case 0x10:
                mem::ref<uint32_t>(skill, 4) |= 0x20;
                break;
            case 0x11:
                mem::ref<uint32_t>(skill, 4) |= 0x40;
                break;
            case 0x12:
                mem::ref<uint32_t>(skill, 4) |= 0x100;
                break;
            case 0x13:
                mem::ref<uint32_t>(skill, 4) |= 0x200;
                break;
            case 0x14:
                mem::ref<uint32_t>(skill, 4) |= 0x400;
                break;
            case 0x15:
                mem::ref<uint32_t>(skill, 4) |= 0x800;
                break;
            case 0x16:
                mem::ref<uint32_t>(skill, 4) |= 0x1000;
                code += 4;
                mem::put16(skill, 0x20, mem::u16(code, -4));
                skill[0x24] = code[-2];
                break;
            case 0x17:
                mem::put16(skill, 0x22, mem::u16(code, 0));
                code += 2;
                break;
            case 0x18:
                mem::ref<uint32_t>(skill, 4) |= 0x2000;
                break;
            case 0x19:
                mem::ref<uint32_t>(skill, 4) |= 0x4000;
                break;
            default:
                break;
        }
    }
    crt::free(script);
    return 1;
}

namespace {

w32::HANDLE openDataFile(std::int32_t& handle, std::int32_t& open, const char* name) {
    if (open != 0) {
        return toHandle(handle);
    }
    w32::lstrcpyA(G.pathBuffer, G.gameDir);
    w32::lstrcatA(G.pathBuffer, name);
    w32::HANDLE file = w32::CreateFileA(G.pathBuffer, w32::GENERIC_READ, 1, nullptr, w32::OPEN_ALWAYS,
                                        w32::FILE_ATTRIBUTE_NORMAL, nullptr);
    handle = static_cast<std::int32_t>(reinterpret_cast<std::uintptr_t>(file));
    if (file == w32::INVALID_HANDLE_VALUE) {
        return file;
    }
    open = 1;
    return file;
}

// length of the text up to "$$" (at most 0xFF bytes searched); 0xFF when there is none
int textLength(const uint8_t* text) {
    int length = 0;
    while (text[length] != '$' || text[length + 1] != '$') {
        if (++length >= 0xFF) {
            return length;
        }
    }
    return length;
}

}  // namespace

// swd32.exe 0x00476DB0: reads the data of monster `id` from mon.dat into `data` (0xA4 bytes, cleared first; a
// previous description at +0xA0 is freed and uncounted). The script must start with 0x3E8 (its name up to "$$"),
// then word opcodes until 5; 0x1C / 0x1D take a single byte and 0x1E a description kept at +0xA0.
int LoadMonster(uint8_t* data, uint16_t id) {
    if (mem::ptr<uint8_t>(data, 0xA0) != nullptr) {
        G.monsterTextBytes -= static_cast<std::int32_t>(crt::msize(mem::ptr<uint8_t>(data, 0xA0)));
        crt::free(mem::ptr<uint8_t>(data, 0xA0));
        mem::putPtr(data, 0xA0, static_cast<uint8_t*>(nullptr));
    }
    std::memset(data, 0, 0xA4);
    w32::HANDLE file = openDataFile(G.monsterFile, G.monsterFileOpen, "mon.dat");
    if (file == w32::INVALID_HANDLE_VALUE) {
        return 0;
    }
    uint32_t base = 0;
    uint32_t offset = 0;
    w32::DWORD read = 0;
    w32::SetFilePointer(file, 0x204, nullptr, w32::FILE_BEGIN);
    w32::ReadFile(toHandle(G.monsterFile), &base, 4, &read, nullptr);
    w32::SetFilePointer(toHandle(G.monsterFile), static_cast<w32::LONG>(4 * id - 4), nullptr, w32::FILE_CURRENT);
    w32::ReadFile(toHandle(G.monsterFile), &offset, 4, &read, nullptr);
    w32::SetFilePointer(toHandle(G.monsterFile), static_cast<w32::LONG>(offset + 0x200), nullptr, w32::FILE_BEGIN);
    auto* script = static_cast<uint8_t*>(crt::malloc(0x400));
    std::memset(script, 0, 0x400);
    w32::ReadFile(toHandle(G.monsterFile), script, 0x400, &read, nullptr);
    if (mem::u16(script, 0) != 0x3E8) {
        crt::free(script);
        return 0;
    }
    const uint8_t* code = script;
    static const struct {
        uint16_t opcode;
        uint16_t field;
    } kWords[] = {{6, 0x40},   {7, 0x24},   {8, 0x26},   {9, 0x2C},   {0xA, 0x32},  {0xB, 0x46},
                  {0xC, 0x42}, {0xD, 0x44}, {0xE, 0x50}, {0xF, 0x28}, {0x10, 0x2A}, {0x11, 0x2E},
                  {0x12, 0x30}, {0x13, 0x34}, {0x14, 0x36}, {0x15, 0x38}, {0x16, 0x48}, {0x1A, 0x3A},
                  {0x1B, 0x3C}, {0x64, 0x3E}};
    for (;;) {
        const uint16_t opcode = mem::u16(code, 0);
        code += 2;
        if (opcode == 5) {
            break;
        }
        bool word = false;
        for (const auto& entry : kWords) {
            if (entry.opcode == opcode) {
                mem::put16(data, entry.field, mem::u16(code, 0));
                code += 2;
                word = true;
                break;
            }
        }
        if (word) {
            continue;
        }
        switch (opcode) {
            case 1:
                std::memcpy(data + 0x50, code, 0x4D);
                code += 0x4D;
                break;
            case 0x19:
                mem::put32(data, 0x20, mem::u32(code, 2));
                code += 6;
                break;
            case 0x1C:
                data[0x9B] = code[0];
                code += 1;
                break;
            case 0x1D:
                data[0x9C] = code[0];
                code += 1;
                break;
            case 0x1E: {
                const int length = textLength(code);
                if (length >= 0xFF) {
                    code += length;
                    break;
                }
                auto* text = static_cast<uint8_t*>(crt::malloc(static_cast<std::uint32_t>(length + 1)));
                mem::putPtr(data, 0xA0, text);
                std::memset(text, 0, static_cast<std::size_t>(length + 1));
                std::memcpy(mem::ptr<uint8_t>(data, 0xA0), code, static_cast<std::size_t>(length));
                code += length + 2;
                mem::ptr<uint8_t>(data, 0xA0)[length] = 0;
                G.monsterTextBytes += length + 1;
                break;
            }
            case 0x3E8: {
                const int length = textLength(code);
                if (length < 0xFF) {
                    std::memcpy(data, code, static_cast<std::size_t>(length));
                }
                code += length + 2;
                break;
            }
            case 0x7D0:
                mem::put32(data, 0x92, mem::u32(code, 0));
                mem::put32(data, 0x96, mem::u32(code, 4));
                data[0x9A] = code[8];
                data[0x9B] = code[9];
                data[0x9C] = code[0xA];
                mem::put16(data, 0x54, mem::u16(code, 0xB));
                mem::put16(data, 0x52, mem::u16(code, 0xD));
                code += 0xF;
                break;
            default:
                break;
        }
    }
    crt::free(script);
    return 1;
}

// swd32.exe 0x00477290: reads entry (`group`, `index`) of level.dat; opcode 0 carries 13 words whose dword at
// byte 22 goes to *value, opcode 1 skips a word
int LoadLevelEntry(int group, int index, uint32_t* value) {
    w32::HANDLE file = openDataFile(G.monsterFile2, G.monsterFile2Open, "level.dat");
    if (file == w32::INVALID_HANDLE_VALUE) {
        return 0;
    }
    uint32_t offset = 0;
    w32::DWORD read = 0;
    w32::SetFilePointer(file, static_cast<w32::LONG>(4 * (index + 0x64 * group) + 0x70), nullptr, w32::FILE_BEGIN);
    w32::ReadFile(toHandle(G.monsterFile2), &offset, 4, &read, nullptr);
    w32::SetFilePointer(toHandle(G.monsterFile2), static_cast<w32::LONG>(offset + 0x200), nullptr, w32::FILE_BEGIN);
    auto* script = static_cast<uint8_t*>(crt::malloc(0x400));
    std::memset(script, 0, 0x400);
    w32::ReadFile(toHandle(G.monsterFile2), script, 0x400, &read, nullptr);
    if (mem::u16(script, 0) != 0) {
        crt::free(script);
        return 0;
    }
    const uint8_t* code = script;
    for (;;) {
        const uint16_t opcode = mem::u16(code, 0);
        code += 2;
        if (opcode == 5) {
            break;
        }
        if (opcode == 0) {
            *value = mem::u32(code, 0x16);
            code += 0x1A;
        } else if (opcode == 1) {
            code += 2;
        }
    }
    crt::free(script);
    return 1;
}

// swd32.exe 0x004763D0: recoil of an enemy (action 0x24 of +0x2A0C, state +0x468) while +0x2668 counts down above
// 6, pushed 25 pixels back per frame; sound 0x10F on frame 1 and 0x2F at the start. Returns 1 when done.
int BattleEnemy::DrawRecoil() {
    if (at<int32_t>(0x2668) <= 6) {
        std::memset(raw(0x468), 0, sizeof(ActState));
        return 1;
    }
    if (at<uint16_t>(0x4B4) == 1) {
        PlaySound(0x10F, G.soundVolume);
    }
    auto* state = reinterpret_cast<ActState*>(raw(0x468));
    at<int32_t>(0x468) = at<uint16_t>(0x2A0C);
    at<int32_t>(0x2AAC) = 1;
    at<int32_t>(0x470) = 0x24;
    at<int32_t>(0x4F8) = 0;
    if (at<int32_t>(0x2AD0) == 1) {
        at<int32_t>(0x4F8) = 1;
    }
    if (ActRun(state) == 0) {
        return 1;
    }
    Picture* picture = TswGetPicture(at<uint16_t>(0x4B2), at<uint16_t>(0x4B4));
    const uint32_t flags = at<uint32_t>(0x480);
    const uint16_t originX = at<uint16_t>(0x478);
    at<Picture*>(0x254C) = picture;
    at<uint32_t>(0x26A0) = flags;
    at<uint16_t>(0x29B4) = originX;
    if (at<int32_t>(0x2668) == 0xF) {
        at<uint16_t>(0x4C0) = 0x2F;
        PlaySound(0x2F, G.soundVolume);
        SetSoundPan(at<uint16_t>(0x4C0), at<int32_t>(0x2B08) == 1 ? 0x10 : -0x10);
        at<uint16_t>(0x4C0) = 0;
    }
    const uint32_t toggled = at<uint32_t>(0x26A0);
    at<uint32_t>(0x26A0) = (toggled & 1) != 0 ? toggled & ~1u : toggled | 1;
    if (at<int32_t>(0x2B08) == 1) {
        const uint32_t mirrored = at<uint32_t>(0x26A0);
        at<uint32_t>(0x26A0) = (mirrored & 1) != 0 ? mirrored & ~1u : mirrored | 1;
        if (at<int32_t>(0x478) != 0) {
            at<uint16_t>(0x29B4) = static_cast<uint16_t>(at<Picture*>(0x254C)->width - at<uint16_t>(0x478));
        }
    }
    uint32_t x = 0;
    uint32_t y = 0;
    GetPosition(reinterpret_cast<uint16_t*>(&x), reinterpret_cast<uint16_t*>(&y));
    x = at<int32_t>(0x2B08) == 1 ? x + 0x19 : x - 0x19;
    SetPosition(static_cast<int16_t>(x), static_cast<int16_t>(y));
    Picture* current = at<Picture*>(0x254C);
    G.drawSource = reinterpret_cast<uint8_t*>(current->pixels);
    DrawPicture(at<int16_t>(0xD66) - at<int16_t>(0x29B4), at<int16_t>(0xD68) - at<int32_t>(0x47C), current->width,
                current->height, static_cast<int>(at<uint32_t>(0x26A0)),
                static_cast<int>(reinterpret_cast<std::uintptr_t>(current->reserved4)));
    --at<int32_t>(0x2668);
    return 0;
}

}  // namespace swd3
