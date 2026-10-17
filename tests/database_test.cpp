#include <cstdio>
#include <cstring>
#include <fstream>
#include <functional>
#include <map>
#include <string>
#include <vector>

#include "core/format.h"
#include "core/words.h"
#include "game_data.h"
#include "golden/database_scenarios.h"
#include "golden_check.h"
#include "org/database.h"
#include "test.h"

using golden::u16;
using golden::u32;
using golden::u64;
using golden::u8;

namespace
{

class DatabaseBackend : public golden::DatabaseBackend
{
public:
    explicit DatabaseBackend(swd3::Database& database)
        : database(database)
    {
    }

    std::vector<u8> Object(u16 id) override
    {
        swd3::ObjectRecord record;
        const bool result = this->database.LoadObject(record, id);
        std::vector<u8> out;
        golden::Append32(out, result ? 1 : 0);
        const auto* bytes = reinterpret_cast<const u8*>(&record.data);
        out.insert(out.end(), bytes, bytes + sizeof(record.data));
        out.push_back(record.description.has_value() ? 1 : 0);

        if (record.description.has_value())
        {
            out.insert(out.end(), record.description->begin(), record.description->end());
            this->database.descriptionBytes -= static_cast<swd3::i32>(record.description->size() + 1);
        }

        return out;
    }

    std::vector<u8> Skill(u16 id) override
    {
        swd3::SkillScript skill;
        const bool result = this->database.LoadSkill(skill, id);
        std::vector<u8> out;
        golden::Append32(out, result ? 1 : 0);
        golden::Append32(out, skill.flags);
        golden::Append32(out, skill.statusBits);
        golden::Append32(out, skill.unk0C);
        golden::Append32(out, skill.unk10);

        for (const u16 word :
             {skill.unk14,
              skill.unk16,
              skill.ailment,
              skill.element,
              skill.unk1C,
              skill.unk1E,
              skill.unk20,
              skill.unk22})
        {
            out.push_back(swd3::LowByte(word));
            out.push_back(swd3::HighByte(word));
        }

        out.push_back(skill.turns);
        return out;
    }

    std::vector<u8> Level(int group, int index) override
    {
        u32 experience = 0;
        const bool result = this->database.LoadLevelEntry(group, index, experience);
        std::vector<u8> out;
        golden::Append32(out, result ? 1 : 0);
        golden::Append32(out, experience);
        return out;
    }

private:
    swd3::Database& database;
};

}  // namespace

TEST_CASE("database: mon.dat objects and skills, level.dat entries match the legacy readers")
{
    const std::map<std::string, std::string> golden =
        swd3test::LoadGolden(SWD3_SOURCE_DIR "/tests/golden/database_golden.txt");

    if (golden.empty() || swd3test::ReadGameFile("mon.dat").empty())
    {
        std::printf("  (game data or golden file not found, skipped)\n");
        return;
    }

    const std::u8string directory = swd3test::GameDirectory() + u8"/";
    swd3::Database database(directory);
    DatabaseBackend backend(database);
    swd3test::GoldenCheck check(golden);
    golden::RunDatabase(backend, std::string(swd3::AsBytes(directory + u8"mon.dat")), std::ref(check));
    std::printf("  compared %d scenarios\n", check.compared);
    CHECK_EQ(check.mismatches, 0);
    CHECK_EQ(check.compared, static_cast<int>(golden.size()));
    CHECK_EQ(database.descriptionBytes, 0);
    swd3::ObjectRecord sword;
    CHECK(database.LoadObject(sword, 1));
    CHECK(sword.data.name[0] != '\0');
    database.CloseFiles();
}
