#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>

#include "config/config.h"
#include "draw/painter.h"
#include "game/databases.h"
#include "game/game_state.h"
#include "game_data.h"
#include "map/field.h"
#include "map/map_cache.h"
#include "pic/rle.h"
#include "platform/surface.h"
#include "role/role.h"
#include "test.h"

using swd3::u16;
using swd3::u32;
using swd3::u8;

namespace
{

// A directory with HUGE.lmf of the game linked in, for the cache files the loading writes.
std::string MapDirectory()
{
    const std::string directory = SWD3_SOURCE_DIR "/tmp/field_test/";
    std::filesystem::remove_all(directory);
    std::filesystem::create_directories(directory + "data");
    std::error_code error;
    std::filesystem::create_symlink(swd3test::GameFile("HUGE.lmf"), directory + "HUGE.lmf", error);
    return directory;
}

struct Fixture
{
    Fixture()
        : surface(640, 480), cache(this->config, this->painter), codec(this->painter),
          field(this->state, this->cache, this->config, this->codec)
    {
        this->painter.Init(this->surface);
    }

    swd3::Config config;
    swd3::Surface surface;
    swd3::Painter painter;
    swd3::GameState state;
    swd3::MapCache cache;
    swd3::RleCodec codec;
    swd3::Field field;
};

}  // namespace

TEST_CASE("field: the first map of a new game from HUGE.lmf")
{
    std::vector<u8> maps = swd3test::ReadGameFile("maps.dat");

    if (maps.size() <= 0x200 || !std::filesystem::exists(swd3test::GameFile("HUGE.lmf")))
    {
        std::printf("  (maps.dat or HUGE.lmf not found, skipped)\n");
        return;
    }

    maps.erase(maps.begin(), maps.begin() + 0x200);
    const u32 start = swd3::MapsTable(maps, 4);
    const u16 map = swd3::MapsWord(maps, start);
    u16 unit = 0xFFFF;

    for (u32 at = swd3::MapsTable(maps, 3); swd3::MapsWord(maps, at) != 0xFFFF; at += 0xE)
    {
        if (swd3::MapsWord(maps, at) == map)
        {
            unit = swd3::MapsWord(maps, at + 2);
            break;
        }
    }

    CHECK(unit != 0xFFFF);
    Fixture all;
    all.config.installType = 2;
    all.config.installDirectory = MapDirectory();
    u32 flags = 0;
    CHECK(all.field.LoadMap(unit, flags));
    CHECK_EQ(flags, 0u);
    const swd3::MapHeader& header = all.state.map;
    CHECK(header.width > 0);
    CHECK(header.height > 0);
    CHECK(header.bitsPerPixel == 8 || header.bitsPerPixel == 16);
    CHECK_EQ(header.blocks.size(), static_cast<std::size_t>(header.width) * header.height);
    CHECK_EQ(header.tiles.size(), static_cast<std::size_t>(header.width) * header.height * header.layers);
    CHECK(header.pixels != nullptr);
    CHECK(std::filesystem::exists(all.config.installDirectory + "data/mcache.dat"));
    CHECK_EQ(all.field.rows[0].size(), static_cast<std::size_t>(header.height) + 2 * swd3::Field::kRowMargin);

    const int width = header.width;
    CHECK(all.field.LoadMap(unit, flags));
    CHECK_EQ(all.state.map.width, width);
    CHECK(all.state.map.pixels != nullptr);

    flags = 0;
    CHECK(!all.field.LoadMap(0xFFFE, flags));
    CHECK_EQ(flags, 4u);
    std::filesystem::remove_all(all.config.installDirectory);
}

TEST_CASE("field: the drawing order of the roles in a row")
{
    Fixture all;
    all.state.map.height = 4;
    all.field.ResetRows();
    swd3::Role first;
    swd3::Role second;
    swd3::Role third;
    first.id = 5;
    second.id = 3;
    third.id = 7;

    for (swd3::Role* role : {&first, &second, &third})
    {
        role->y = 0x20;
        all.field.InsertRole(*role, 0);
    }

    swd3::Role* head = all.field.rows[0][2 + swd3::Field::kRowMargin];
    CHECK(head == &third);
    CHECK(head->next == &first);
    CHECK(first.next == &second);
    CHECK(second.next == nullptr);

    CHECK(all.field.RemoveRole(5, 0, 0, 1) == &first);
    CHECK(third.next == &second);
    CHECK(all.field.RemoveRole(9, 0, 0, 1) == nullptr);
    CHECK(all.field.RemoveRole(3, 0, 0, 0) == nullptr);
    CHECK(third.next == &second);
}
