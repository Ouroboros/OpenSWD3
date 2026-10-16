#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "config/config.h"
#include "draw/painter.h"
#include "game/game_state.h"
#include "game_data.h"
#include "map/map_cache.h"
#include "platform/surface.h"
#include "talk/text_colors.h"
#include "test.h"

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

}  // namespace

TEST_CASE("game init: text colour tables match swd32.exe")
{
    const std::vector<u8> exe = swd3test::ReadGameFile("swd32.exe");

    if (exe.empty())
    {
        std::printf("  (swd32.exe not found, skipped)\n");
        return;
    }

    CHECK(std::memcmp(exe.data() + (0x0049E0C8 - 0x400000), swd3::TextColors::kPrimary.data(), 0x40) == 0);
    CHECK(std::memcmp(exe.data() + (0x0049E108 - 0x400000), swd3::TextColors::kSecondary.data(), 0x40) == 0);
    swd3::Surface surface(640, 480);
    swd3::Painter painter;
    painter.Init(surface);
    swd3::TextColors colors;
    colors.Convert(painter);
    CHECK(colors.primary[0] != 0);
    CHECK_EQ(colors.primary[0] >> 16, colors.primary[0] & 0xFFFF);
    CHECK_EQ(colors.primary[1], 0u);
    CHECK_EQ(colors.primary[8] & 0xFFFF, 0x001Fu);
}

TEST_CASE("game init: the session mark of env.dat empties the map cache")
{
    const std::string directory = TestDirectory();
    swd3::Config config;
    config.installDirectory = directory;
    swd3::Surface surface(640, 480);
    swd3::Painter painter;
    painter.Init(surface);
    swd3::MapCache cache(config, painter);
    WriteBytes(directory + "env.dat", std::vector<u8>(0x40, 0));
    WriteBytes(directory + "data/3.cm", std::vector<u8>(100, 7));
    WriteBytes(directory + "data/mcache.dat", std::vector<u8>(0x180, 9));
    CHECK_EQ(cache.Check(0), 100u);
    cache.MarkDirty();
    const std::vector<u8> marked = ReadBytes(directory + "env.dat");
    CHECK_EQ(marked.back(), 1);
    CHECK_EQ(cache.Check(0), 0u);
    const std::vector<u8> cleared = ReadBytes(directory + "env.dat");
    CHECK_EQ(cleared.back(), 0);
    const std::vector<u8> table = ReadBytes(directory + "data/mcache.dat");
    CHECK_EQ(table.size(), 0x180u);
    CHECK_EQ(table[0], 0xFF);
    CHECK_EQ(table[0x1C], 1);
    WriteBytes(directory + "data/3.cm", std::vector<u8>(5, 7));
    cache.MarkDirty();
    cache.MarkClean();
    CHECK_EQ(cache.Check(0), 5u);
    std::filesystem::remove_all(directory);
}

TEST_CASE("game init: clearing the world")
{
    swd3::GameState world;
    world.roles[3].unk38.assign(4, 1);
    world.roles[3].x = 9;
    world.map.unk18.assign(8, 1);
    world.ClearWorld();
    CHECK(world.roles[3].unk38.empty());
    CHECK_EQ(world.roles[3].x, 0);
    CHECK_EQ(world.roleCount, 0);
    CHECK_EQ(world.leader.id, 0xFFFF);
    CHECK_EQ(world.leader.act.id, -1);
    CHECK(world.map.unk18.empty());
    CHECK_EQ(world.moves[71].unk000[5], 0xFF);
}
