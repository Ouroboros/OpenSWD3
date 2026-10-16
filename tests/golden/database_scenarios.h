#pragma once

// mon.dat and level.dat scenarios shared by the legacy golden generator and the Database test.

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

// Whether the skill script `id` of mon.dat ends (opcode 5) within the 0x400 bytes the readers load: other ids are not
// skills and send both readers past their buffer.
inline bool SkillEndsInBuffer(const std::string& monPath, u16 id)
{
    std::ifstream file(monPath, std::ios::binary);

    if (!file)
    {
        return false;
    }

    const u32 base = ReadU32(file, 0x204);
    const u32 offset = ReadU32(file, base + 4u * id + 0x200);
    std::vector<u8> script(0x400, 0);
    file.seekg(offset + 0x200);
    file.read(reinterpret_cast<char*>(script.data()), static_cast<std::streamsize>(script.size()));
    std::size_t at = 0;

    auto word = [&](std::size_t position)
    {
        return static_cast<u16>(script[position] | (script[position + 1] << 8));
    };

    if (word(0) != 0)
    {
        return true;
    }

    while (at + 1 < script.size())
    {
        const u16 opcode = word(at);
        at += 2;

        if (opcode == 5)
        {
            return true;
        }

        switch (opcode)
        {
            case 0:
                at += 8;
                break;

            case 0xC:
            case 0x16:
                at += 4;
                break;

            case 2:
            case 4:
            case 6:
            case 7:
            case 8:
            case 0xA:
            case 0xD:
            case 0xE:
            case 0x17:
                at += 2;
                break;

            default:
                break;
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

    for (u16 id = 1; id <= 2000; ++id)
    {
        const std::vector<u8> object = backend.Object(id);
        report("object/" + std::to_string(id), Hash(object.data(), object.size()));
    }

    for (u16 id = 0; id <= 2000; ++id)
    {
        if (!SkillEndsInBuffer(monPath, id))
        {
            continue;
        }

        const std::vector<u8> skill = backend.Skill(id);
        report("skill/" + std::to_string(id), Hash(skill.data(), skill.size()));
    }

    // The game reads groups 1..4 (hero + 1); group 5 lies past the table (an empty script read past the buffer).
    for (int group = 0; group < 5; ++group)
    {
        std::vector<u8> levels;

        for (int index = 0; index < 100; ++index)
        {
            const std::vector<u8> level = backend.Level(group, index);
            levels.insert(levels.end(), level.begin(), level.end());
        }

        report("level/" + std::to_string(group), Hash(levels.data(), levels.size()));
    }
}

}  // namespace golden
