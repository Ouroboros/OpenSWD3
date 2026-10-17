#pragma once

// mon.dat and level.dat scenarios shared by the legacy golden generator and the Database test.

#include <array>
#include <climits>
#include <fstream>
#include <functional>
#include <string>
#include <vector>

#include "golden/draw_scenarios.h"

namespace golden
{

class DatabaseBackend
{
public:
    virtual ~DatabaseBackend() = default;
    // The result, the 0xA0 fixed bytes, a description mark (0: none) and the description.
    virtual std::vector<u8> Object(u16 id) = 0;
    // The result and the script fields from +0x04 to +0x24 in record order.
    virtual std::vector<u8> Skill(u16 id) = 0;
    // The result and the experience.
    virtual std::vector<u8> Level(int group, int index) = 0;
};

// The mon.dat layout: the record offsets past the header, the skill table pointer, and the skill scripts.
constexpr u32 kMonHeader = 0x200;
constexpr u32 kSkillTableAt = 0x204;
constexpr std::size_t kScriptBytes = 0x400;
constexpr u16 kEndOpcode = 5;

struct OperandBytes
{
    u16 opcode;
    std::size_t bytes;
};

// The operand bytes of the opcodes that have any.
constexpr std::array<OperandBytes, 12> kOperandBytes = {{
    {0, 8},
    {0xC, 4},
    {0x16, 4},
    {2, 2},
    {4, 2},
    {6, 2},
    {7, 2},
    {8, 2},
    {0xA, 2},
    {0xD, 2},
    {0xE, 2},
    {0x17, 2},
}};

constexpr u16 kLastId = 2000;  // the object and skill ids tried
constexpr int kLevelGroups = 5;
constexpr int kLevelsPerGroup = 100;

// Whether the skill script `id` of mon.dat ends (kEndOpcode) within the kScriptBytes bytes the readers load: other ids
// are not skills and send both readers past their buffer.
inline bool SkillEndsInBuffer(const std::string& monPath, u16 id)
{
    std::ifstream file(monPath, std::ios::binary);

    if (!file)
    {
        return false;
    }

    const u32 base = ReadU32(file, kSkillTableAt);
    const u32 offset = ReadU32(file, base + sizeof(u32) * id + kMonHeader);
    std::vector<u8> script(kScriptBytes, 0);
    file.seekg(offset + kMonHeader);
    file.read(reinterpret_cast<char*>(script.data()), static_cast<std::streamsize>(script.size()));
    std::size_t at = 0;

    auto word = [&](std::size_t position)
    {
        return static_cast<u16>(script[position] | (script[position + 1] << CHAR_BIT));
    };

    if (word(0) != 0)
    {
        return true;
    }

    while (at + 1 < script.size())
    {
        const u16 opcode = word(at);
        at += sizeof(u16);

        if (opcode == kEndOpcode)
        {
            return true;
        }

        for (const OperandBytes& entry : kOperandBytes)
        {
            if (entry.opcode == opcode)
            {
                at += entry.bytes;
                break;
            }
        }
    }

    return false;
}

inline void RunDatabase(
    DatabaseBackend& backend,
    const std::string& monPath,
    const std::function<void(const std::string&, u64)>& report
)
{
    std::vector<u8> objects;

    for (u16 id = 1; id <= kLastId; ++id)
    {
        const std::vector<u8> object = backend.Object(id);
        report("object/" + std::to_string(id), Hash(object.data(), object.size()));
    }

    for (u16 id = 0; id <= kLastId; ++id)
    {
        if (!SkillEndsInBuffer(monPath, id))
        {
            continue;
        }

        const std::vector<u8> skill = backend.Skill(id);
        report("skill/" + std::to_string(id), Hash(skill.data(), skill.size()));
    }

    // The game reads groups 1..4 (role + 1); group 5 lies past the table (an empty script read past the buffer).
    for (int group = 0; group < kLevelGroups; ++group)
    {
        std::vector<u8> levels;

        for (int index = 0; index < kLevelsPerGroup; ++index)
        {
            const std::vector<u8> level = backend.Level(group, index);
            levels.insert(levels.end(), level.begin(), level.end());
        }

        report("level/" + std::to_string(group), Hash(levels.data(), levels.size()));
    }
}

}  // namespace golden
