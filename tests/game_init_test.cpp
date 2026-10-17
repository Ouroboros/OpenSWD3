#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "config/config.h"
#include "core/words.h"
#include "draw/painter.h"
#include "game/databases.h"
#include "game/game_state.h"
#include "game_data.h"
#include "map/map_cache.h"
#include "party/party.h"
#include "platform/display.h"
#include "platform/host_settings.h"
#include "platform/surface.h"
#include "talk/text_colors.h"
#include "test.h"
#include "text/big5.h"

using swd3::u32;
using swd3::u8;

namespace
{

std::string TestDirectory()
{
    const std::string directory = SWD3_SOURCE_DIR "/tmp/game_init_test/";
    std::filesystem::remove_all(directory);
    std::filesystem::create_directories(directory + "data");
    return directory;
}

void WriteBytes(const std::string& path, const std::vector<u8>& bytes)
{
    std::ofstream file(path, std::ios::binary);
    file.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
}

std::vector<u8> ReadBytes(const std::string& path)
{
    std::ifstream file(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

constexpr swd3::u32 kPrimaryColorsAt = 0x0049E0C8;
constexpr swd3::u32 kSecondaryColorsAt = 0x0049E108;
constexpr std::size_t kBlackColor = 1;  // text colours 1 and 8 are black and pure blue
constexpr std::size_t kBlueColor = 8;

constexpr std::string_view kCacheFile = "data/3.cm";
constexpr u32 kCachedBytes = 100;
constexpr u32 kSmallCache = 5;
constexpr u8 kFill = 7;
constexpr std::size_t kRole = 3;
constexpr std::size_t kLabelLength = 4;
constexpr swd3::i32 kRoleX = 9;
constexpr std::size_t kBlocks = 8;

}  // namespace

TEST_CASE("game init: text colour tables match swd32.exe")
{
    const std::vector<u8> exe = swd3test::ReadGameFile("swd32.exe");

    if (exe.empty())
    {
        std::printf("  (swd32.exe not found, skipped)\n");
        return;
    }

    CHECK(swd3test::SameTable(exe, kPrimaryColorsAt, swd3::TextColors::Primary));
    CHECK(swd3test::SameTable(exe, kSecondaryColorsAt, swd3::TextColors::Secondary));
    swd3::Surface surface(swd3::Display::Width, swd3::Display::Height);
    swd3::Painter painter;
    painter.Init(surface);
    swd3::TextColors colors;
    colors.Convert(painter);
    CHECK(colors.primary[0] != 0);
    CHECK_EQ(swd3::HighWord(colors.primary[0]), swd3::LowWord(colors.primary[0]));
    CHECK_EQ(colors.primary[kBlackColor], 0u);
    CHECK_EQ(swd3::LowWord(colors.primary[kBlueColor]), swd3::Surface::BlueMask);
}

TEST_CASE("game init: the session mark of [game] empties the map cache")
{
    using swd3::MapCache;
    const std::string directory = TestDirectory();
    swd3::Config config;
    config.installDirectory = directory;
    config.settingsFile = directory + "swd3r.default.toml";
    swd3::Surface surface(swd3::Display::Width, swd3::Display::Height);
    swd3::Painter painter;
    painter.Init(surface);
    MapCache cache(config, painter);

    auto sessionOpen = [&config, &directory]
    {
        return swd3::LoadHostSettings({}, config.settingsFile, directory).values.game.sessionOpen.value_or(false);
    };

    WriteBytes(directory + std::string(kCacheFile), std::vector<u8>(kCachedBytes, kFill));
    WriteBytes(directory + "data/mcache.dat", std::vector<u8>(MapCache::TableBytes, kFill));
    CHECK_EQ(cache.Check(MapCache::CheckSessionMark), kCachedBytes);
    cache.MarkDirty();
    CHECK(sessionOpen());
    CHECK_EQ(cache.Check(MapCache::CheckSessionMark), 0u);
    CHECK(!sessionOpen());
    const std::vector<u8> table = ReadBytes(directory + "data/mcache.dat");
    CHECK_EQ(table.size(), MapCache::TableBytes);
    CHECK_EQ(table[0], swd3::LowByte(swd3::LowWord(MapCache::Slot::Free)));
    CHECK_EQ(table[sizeof(MapCache::Slot) + offsetof(MapCache::Slot, unit)], 1);  // the second slot keeps unit 1
    WriteBytes(directory + std::string(kCacheFile), std::vector<u8>(kSmallCache, kFill));
    cache.MarkDirty();
    cache.MarkClean();
    CHECK_EQ(cache.Check(MapCache::CheckSessionMark), kSmallCache);
    std::filesystem::remove_all(directory);
}

TEST_CASE("game init: clearing the world")
{
    swd3::GameState world;
    swd3::Role& role = world.roles[kRole];
    role.label.emplace(kLabelLength, 'a');
    role.x = kRoleX;
    world.map.blocks.assign(kBlocks, 1);
    world.ClearWorld();
    CHECK(!role.label.has_value());
    CHECK_EQ(role.x, 0);
    CHECK_EQ(world.roleCount, 0);
    CHECK_EQ(world.leader.id, swd3::Role::NoId);
    CHECK_EQ(world.leader.act.id, -1);
    CHECK(world.map.blocks.empty());
    CHECK(
        std::all_of(
            world.moves.back().directions.begin(),
            world.moves.back().directions.end(),
            [](u8 direction)
            {
                return direction == swd3::MoveRecord::EndOfPath;
            }
        )
    );
    CHECK_EQ(world.moves.back().role, swd3::MoveRecord::NoRole);
}

TEST_CASE("game init: the starting roles of maps.dat")
{
    using swd3::GameDatabases;
    std::vector<u8> maps = swd3test::ReadGameFile("maps.dat");

    if (maps.size() <= GameDatabases::MapsHeader)
    {
        std::printf("  (maps.dat not found, skipped)\n");
        return;
    }

    maps.erase(maps.begin(), maps.begin() + GameDatabases::MapsHeader);
    const u32 table = swd3::MapsTable(maps, swd3::MapsDataTable::StartRoles);
    CHECK(table + swd3::Party::Roles * swd3::MapsRecord::StartRoleBytes <= maps.size());

    for (int index = 0; index < swd3::Party::Roles; ++index)
    {
        swd3::RoleRecord role;
        const std::size_t at = table + swd3::MapsRecord::StartRoleBytes * static_cast<std::size_t>(index);
        role.Load(std::span<const u8>(maps).subspan(at, swd3::MapsRecord::StartRoleBytes));
        CHECK(role.maxHitPoints > 0);
        CHECK(role.hitPoints <= role.maxHitPoints);
        CHECK(role.level >= 1);
    }
}

TEST_CASE("game init: the object descriptions of maps.dat")
{
    using swd3::GameDatabases;
    using swd3::u16;
    constexpr u16 kStaminaSkill = 0x31;
    constexpr u16 kTalismanPaper = 0x300;
    constexpr std::u8string_view kStaminaCost = u8"消費：300點體力";
    constexpr std::u8string_view kPaperStart = u8"練符法寶專用的符紙";
    constexpr std::u8string_view kNone = u8"無";
    constexpr u16 kEmpty = 0xFFDC;
    std::vector<u8> maps = swd3test::ReadGameFile("maps.dat");

    if (maps.size() <= GameDatabases::MapsHeader)
    {
        std::printf("  (maps.dat not found, skipped)\n");
        return;
    }

    maps.erase(maps.begin(), maps.begin() + GameDatabases::MapsHeader);
    CHECK(swd3::ObjectDescription(maps, kStaminaSkill) == swd3::Big5(kStaminaCost));
    CHECK(swd3::ObjectDescription(maps, kTalismanPaper).starts_with(swd3::Big5(kPaperStart)));
    CHECK(swd3::ObjectDescription(maps, kEmpty) == swd3::Big5(kNone));
}
