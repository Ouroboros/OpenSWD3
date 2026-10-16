// Golden generator for mon.dat and level.dat: runs tests/golden/database_scenarios.h through the legacy
// transliteration and writes "name hash" lines to $GOLDEN_OUT (see README.md).

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "battle/battle_enemy.h"
#include "core/mem.h"
#include "crt/crt.h"
#include "data/globals.h"
#include "golden/database_scenarios.h"
#include "swd3/w32.h"
#include "test.h"

using namespace swd3;

namespace
{

class LegacyDatabase : public golden::DatabaseBackend
{
public:
    std::vector<golden::u8> Object(golden::u16 id) override
    {
        std::vector<std::uint8_t> data(0xA4, 0);
        const int result = LoadMonster(data.data(), id);
        std::vector<golden::u8> out;
        golden::Append32(out, static_cast<golden::u32>(result));
        out.insert(out.end(), data.begin(), data.begin() + 0xA0);
        const char* text = mem::ptr<const char>(data.data(), 0xA0);
        out.push_back(text != nullptr ? 1 : 0);

        if (text != nullptr)
        {
            out.insert(out.end(), text, text + std::strlen(text));
            G.monsterTextBytes -= static_cast<std::int32_t>(std::strlen(text) + 1);
            crt::free(const_cast<char*>(text));
        }

        return out;
    }

    std::vector<golden::u8> Skill(golden::u16 id) override
    {
        std::vector<std::uint8_t> skill(0x40, 0);
        const int result = LoadMonsterSkill(skill.data(), id);
        std::vector<golden::u8> out;
        golden::Append32(out, static_cast<golden::u32>(result));
        out.insert(out.end(), skill.begin() + 4, skill.begin() + 0x25);
        return out;
    }

    std::vector<golden::u8> Level(int group, int index) override
    {
        std::uint32_t experience = 0;
        const int result = LoadLevelEntry(group, index, &experience);
        std::vector<golden::u8> out;
        golden::Append32(out, static_cast<golden::u32>(result));
        golden::Append32(out, experience);
        return out;
    }
};

}  // namespace

TEST_CASE("golden: legacy database")
{
    const char* output = std::getenv("GOLDEN_OUT");
    const char* gameDir = std::getenv("SWD3_GAME_DIR");

    if (output == nullptr || gameDir == nullptr)
    {
        std::printf("  (GOLDEN_OUT / SWD3_GAME_DIR not set, skipped)\n");
        return;
    }

    std::snprintf(G.gameDir, sizeof(G.gameDir), "%s/", gameDir);
    const std::string directory = std::string(gameDir) + "/";
    std::FILE* file = std::fopen(output, "w");
    LegacyDatabase backend;
    golden::RunDatabase(
        backend,
        directory + "mon.dat",
        [file](const std::string& name, golden::u64 hash)
        {
            std::fprintf(file, "%s %016llx\n", name.c_str(), static_cast<unsigned long long>(hash));
            std::fflush(file);
        }
    );
    std::fclose(file);
}
