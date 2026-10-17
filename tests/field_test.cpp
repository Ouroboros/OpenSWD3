#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>

#include "config/config.h"
#include "draw/painter.h"
#include "game/databases.h"
#include "game/game.h"
#include "game/game_state.h"
#include "game_data.h"
#include "map/blocking_map.h"
#include "map/field.h"
#include "map/map_cache.h"
#include "pic/rle.h"
#include "platform/display.h"
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
        : surface(swd3::Display::Width, swd3::Display::Height),
          cache(this->config, this->painter),
          codec(this->painter),
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

constexpr u32 kNoSuchUnit = 0xFFFE;
constexpr int kRows = 4;
constexpr u16 kFirstId = 5;  // three roles in one row
constexpr u16 kSecondId = 3;
constexpr u16 kThirdId = 7;
constexpr u16 kAbsentId = 9;
constexpr swd3::i32 kRoleY = 0x20;
constexpr int kGridSide = 8;
constexpr int kCellX = 3;  // a one-cell footprint
constexpr int kCellY = 3;
constexpr int kLargeX = 2;  // a 3 x 3 footprint
constexpr int kLargeY = 2;
constexpr int kLargeSide = 3;

}  // namespace

TEST_CASE("field: the first map of a new game from HUGE.lmf")
{
    using swd3::MapsRecord;
    std::vector<u8> maps = swd3test::ReadGameFile("maps.dat");

    if (maps.size() <= swd3::GameDatabases::MapsHeader || !std::filesystem::exists(swd3test::GameFile("HUGE.lmf")))
    {
        std::printf("  (maps.dat or HUGE.lmf not found, skipped)\n");
        return;
    }

    maps.erase(maps.begin(), maps.begin() + swd3::GameDatabases::MapsHeader);
    const u32 start = swd3::MapsTable(maps, swd3::MapsDataTable::NewStory);
    const u16 map = swd3::MapsWord(maps, start);
    u16 unit = MapsRecord::TableEnd;

    for (u32 at = swd3::MapsTable(maps, swd3::MapsDataTable::MapSettings);
         swd3::MapsWord(maps, at) != MapsRecord::TableEnd;
         at += MapsRecord::SettingsBytes)
    {
        if (swd3::MapsWord(maps, at) == map)
        {
            unit = swd3::MapsWord(maps, at + MapsRecord::SettingsUnitAt);
            break;
        }
    }

    CHECK(unit != MapsRecord::TableEnd);
    Fixture all;
    all.config.installType = swd3::Config::FullInstall;
    all.config.installDirectory = MapDirectory();
    u32 flags = 0;
    CHECK(all.field.LoadMap(unit, flags));
    CHECK_EQ(flags, 0u);
    const swd3::MapHeader& header = all.state.map;
    CHECK(header.width > 0);
    CHECK(header.height > 0);
    CHECK(header.bitsPerPixel == swd3::MapHeader::IndexedBits || header.bitsPerPixel == swd3::MapHeader::TrueColorBits);
    CHECK_EQ(header.blocks.size(), static_cast<std::size_t>(header.width) * header.height);
    CHECK_EQ(header.tiles.size(), static_cast<std::size_t>(header.width) * header.height * header.layers);
    CHECK(header.pixels != nullptr);
    CHECK(std::filesystem::exists(all.config.installDirectory + "data/mcache.dat"));
    CHECK_EQ(all.field.rows[0].size(), static_cast<std::size_t>(header.height) + 2 * swd3::Field::RowMargin);

    const int width = header.width;
    CHECK(all.field.LoadMap(unit, flags));
    CHECK_EQ(all.state.map.width, width);
    CHECK(all.state.map.pixels != nullptr);

    flags = 0;
    CHECK(!all.field.LoadMap(kNoSuchUnit, flags));
    CHECK_EQ(flags, static_cast<u32>(swd3::Game::QuitRequested));
    std::filesystem::remove_all(all.config.installDirectory);
}

TEST_CASE("field: the drawing order of the roles in a row")
{
    Fixture all;
    all.state.map.height = kRows;
    all.field.ResetRows();
    swd3::Role first;
    swd3::Role second;
    swd3::Role third;
    first.id = kFirstId;
    second.id = kSecondId;
    third.id = kThirdId;

    for (swd3::Role* role : {&first, &second, &third})
    {
        role->y = kRoleY;
        all.field.InsertRole(*role, 0);
    }

    swd3::Role* head = all.field.rows[0][(kRoleY >> swd3::MapHeader::TileShift) + swd3::Field::RowMargin];
    CHECK(head == &third);
    CHECK(head->next == &first);
    CHECK(first.next == &second);
    CHECK(second.next == nullptr);

    CHECK(all.field.RemoveRole(kFirstId, 0, 0, 1) == &first);
    CHECK(third.next == &second);
    CHECK(all.field.RemoveRole(kAbsentId, 0, 0, 1) == nullptr);
    CHECK(all.field.RemoveRole(kSecondId, 0, 0, 0) == nullptr);
    CHECK(third.next == &second);
}

TEST_CASE("field: the cells around a footprint")
{
    using swd3::BlockingMap;
    using swd3::MapHeader;
    Fixture all;
    MapHeader& map = all.state.map;
    map.width = kGridSide;
    map.height = kGridSide;
    map.blocks.assign(kGridSide * kGridSide, 0);

    auto at = [](int x, int y)
    {
        return static_cast<std::size_t>(x + kGridSide * y);
    };

    constexpr u32 kMask = MapHeader::Wall | MapHeader::SolidRole | MapHeader::Unk800000;
    map.blocks[at(kCellX - 1, kCellY - 1)] = MapHeader::Wall;   // up left
    map.blocks[at(kCellX + 1, kCellY)] = MapHeader::SolidRole;  // right
    map.blocks[at(kCellX, kCellY + 1)] = MapHeader::Unk800000;  // down
    map.blocks[at(kCellX - 1, kCellY + 1)] = 1;                 // down left, a region outside the mask
    CHECK_EQ(
        all.field.ProbeCells(static_cast<std::ptrdiff_t>(at(kCellX, kCellY)), 1, 1, kMask),
        BlockingMap::ProbeUpLeft | BlockingMap::ProbeRight | BlockingMap::ProbeDown
    );
    CHECK_EQ(
        all.field.ProbeCells(static_cast<std::ptrdiff_t>(at(0, 0)), 1, 1, kMask),
        BlockingMap::ProbeUpLeft | BlockingMap::ProbeUp | BlockingMap::ProbeUpRight | BlockingMap::ProbeLeft
    );  // the left column wraps to the row end

    map.blocks.assign(kGridSide * kGridSide, 0);
    map.blocks[at(kLargeX + kLargeSide, kLargeY)] = MapHeader::SolidRole;  // right of the footprint, its top row
    map.blocks[at(kLargeX + 1, kLargeY - 1)] = MapHeader::SolidRole;       // above its middle
    CHECK_EQ(
        all.field.ProbeCells(static_cast<std::ptrdiff_t>(at(kLargeX, kLargeY)), kLargeSide, kLargeSide, kMask),
        BlockingMap::ProbeUpLeft | BlockingMap::ProbeUp | BlockingMap::ProbeUpRight | BlockingMap::ProbeRight
    );
}
