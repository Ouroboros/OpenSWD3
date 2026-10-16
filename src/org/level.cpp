#include "org/level.h"

#include <array>
#include <cstring>
#include <vector>

#include "core/assertion.h"
#include "org/database.h"
#include "party/party.h"
#include "text/big5.h"

namespace swd3
{

namespace
{

constexpr std::string_view kOrgFile = "C:\\Project\\swd102aDVD\\org.cpp";
// "升級魔法編號為0!!" and "魔法編號為0!!" (swd32.exe 0x004A7D04, 0x004A7D38).
constexpr std::u8string_view kLevelSkillZero = u8"升級魔法編號為0!!";
constexpr std::u8string_view kSkillZero = u8"魔法編號為0!!";
constexpr u32 kLevelSkillLine = 0x330;
constexpr u32 kSkillLine = 0x50A;
constexpr i16 kNoSkill = static_cast<i16>(0x8000);
constexpr int kStatWords = 13;

u16 WordAt(const std::vector<u8>& script, std::size_t at)
{
    return at + 1 < script.size() ? static_cast<u16>(script[at] | (script[at + 1] << 8)) : u16{0};
}

// The skill list of a party role gets `id` unless the list (its "無" head included) has it: a new node at its end with
// the object of mon.dat. True when one was added.
bool AppendSkill(ItemNode& head, i16 id, Database& database, LearnedSkill& last)
{
    ItemNode* node = &head;

    if (static_cast<i16>(node->id) == id)
    {
        return false;
    }

    while (node->next != nullptr)
    {
        node = node->next.get();

        if (static_cast<i16>(node->id) == id)
        {
            return false;
        }
    }

    if (id == kNoSkill)
    {
        return false;
    }

    node->next = std::make_unique<ItemNode>();
    ItemNode& added = *node->next;
    added.id = static_cast<u16>(id);
    database.LoadObject(added.record, added.id);
    last.name = std::string(
        added.record.data.name.data(),
        strnlen(added.record.data.name.data(), added.record.data.name.size())
    );
    return true;
}

}  // namespace

// swd32.exe 0x00477400: level `level` of role `role1` (1..4) from level.dat: opcode 0 brings the 13 words of the
// maxima, attributes and experience and fills hit points, spirit and stamina; opcode 1 teaches a skill (an id of 0
// raises the maintenance assertion). The opcodes are read as the sign-extended low byte of each word.
bool LevelUp(
    RoleRecord& role,
    int role1,
    int level,
    int& learned,
    Party& party,
    Database& database,
    LearnedSkill& last,
    ErrorReporter& reporter
)
{
    std::vector<u8> script;

    if (!database.ReadLevelScript(role1, level, script))
    {
        return false;
    }

    if (WordAt(script, 0) != 0)
    {
        return false;
    }

    std::size_t at = 0;

    while (at + 1 < script.size())
    {
        const auto opcode = static_cast<i8>(script[at]);
        at += 2;

        if (opcode == 5)
        {
            break;
        }

        if (opcode == 0)
        {
            std::array<u16, kStatWords> words = {};

            for (std::size_t i = 0; i < words.size(); ++i)
            {
                words[i] = WordAt(script, at + 2 * i);
            }

            role.maxHitPoints = static_cast<i16>(words[0]);
            role.maxSpirit = static_cast<i16>(words[1]);
            role.maxStamina = static_cast<i16>(words[2]);
            role.attack = static_cast<i16>(words[3]);
            role.defense = static_cast<i16>(words[4]);
            role.unk14 = static_cast<i16>(words[5]);
            role.agility = static_cast<i16>(words[6]);
            role.unk18 = static_cast<i16>(words[7]);
            role.unk1A = static_cast<i16>(words[8]);
            role.ailmentChance = static_cast<i16>(words[9]);
            role.unk1E = static_cast<i16>(words[10]);
            role.experience = static_cast<i32>(words[11] | (static_cast<u32>(words[12]) << 16));
            at += 2 * kStatWords;
            role.level = static_cast<u8>(level);
            role.hitPoints = role.maxHitPoints;
            role.spirit = role.maxSpirit;
            role.stamina = role.maxStamina;
        }
        else if (opcode == 1)
        {
            const auto id = static_cast<i16>(WordAt(script, at));

            if (id == 0)
            {
                MaintenanceAssert(reporter, kLevelSkillZero, 0, kOrgFile, kLevelSkillLine);
            }

            if (AppendSkill(*party.skills[static_cast<std::size_t>(role1 - 1)], id, database, last))
            {
                learned = 1;
            }

            at += 2;
        }
    }

    return true;
}

// swd32.exe 0x00477BD0: role `role` (0..3) learns skill `id`; 0 when the list has it already, 1 otherwise (also for the
// "no skill" id 0x8000, which adds nothing). An id of 0 raises the maintenance assertion.
int LearnSkill(int role, i16 id, Party& party, Database& database, LearnedSkill& last, ErrorReporter& reporter)
{
    if (id == 0)
    {
        MaintenanceAssert(reporter, kSkillZero, 0, kOrgFile, kSkillLine);
    }

    ItemNode& head = *party.skills[static_cast<std::size_t>(role)];
    ItemNode* node = &head;

    if (static_cast<i16>(node->id) == id)
    {
        return 0;
    }

    while (node->next != nullptr)
    {
        node = node->next.get();

        if (static_cast<i16>(node->id) == id)
        {
            return 0;
        }
    }

    AppendSkill(head, id, database, last);
    return 1;
}

}  // namespace swd3
