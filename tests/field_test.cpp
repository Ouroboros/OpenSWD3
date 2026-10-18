#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>

#include "config/config.h"
#include "core/format.h"
#include "draw/painter.h"
#include "game/databases.h"
#include "game/game.h"
#include "game/game_state.h"
#include "game_data.h"
#include "map/blocking_map.h"
#include "map/field.h"
#include "map/field_view.h"
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
    all.config.installDirectory = swd3::Utf8Bytes(MapDirectory());
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
    CHECK(std::filesystem::exists(std::filesystem::path(all.config.installDirectory + u8"data/mcache.dat")));
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

TEST_CASE("field: the speed nibbles of the map settings match swd32.exe")
{
    constexpr swd3::u32 kSpeedNibblesAt = 0x0049FBA8;
    const std::vector<swd3::u8> exe = swd3test::ReadGameFile("swd32.exe");

    if (exe.empty())
    {
        return;
    }

    CHECK(swd3test::SameTable(exe, kSpeedNibblesAt, swd3::Game::SpeedNibbles));
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

TEST_CASE("field view: the parts of a render scale are woven into its blocks")
{
    using swd3::Display;
    using swd3::FieldView;
    constexpr int kWidth = Display::Width;
    constexpr int kHeight = Display::Height;
    int wrong = 0;

    for (int scale = 1; scale <= swd3::Game::MaxRenderScale; ++scale)
    {
        const auto blocks = static_cast<std::size_t>(scale);
        std::vector<swd3::Surface> parts(blocks * blocks);

        for (std::size_t part = 0; part < parts.size(); ++part)
        {
            parts[part].Resize(kWidth, kHeight);

            for (int y = 0; y < kHeight; ++y)
            {
                for (int x = 0; x < kWidth; ++x)
                {
                    parts[part].Row(y)[x] = static_cast<swd3::u16>(part * 0x1000 + static_cast<std::size_t>(y + x * 7));
                }
            }
        }

        std::vector<const swd3::Surface*> pointers;

        for (const swd3::Surface& part : parts)
        {
            pointers.push_back(&part);
        }

        std::vector<swd3::u16> woven(blocks * blocks * Display::Pixels);
        FieldView::Weave(pointers, scale, woven);
        const std::size_t pitch = blocks * kWidth;

        for (std::size_t y = 0; y < blocks * kHeight; ++y)
        {
            for (std::size_t x = 0; x < pitch; ++x)
            {
                const swd3::Surface& part = parts[(y % blocks) * blocks + x % blocks];
                wrong += woven[y * pitch + x] == part.Row(static_cast<int>(y / blocks))[x / blocks] ? 0 : 1;
            }
        }
    }

    CHECK_EQ(wrong, 0);
}

TEST_CASE("field view: spans of a logical layer scaled into blocks")
{
    using swd3::Display;
    using swd3::FieldView;
    constexpr int kScale = 3;
    constexpr std::size_t kBlocks = kScale;
    constexpr std::size_t kPitch = kBlocks * Display::Width;
    constexpr swd3::u32 kStart = Display::Width - 5;  // a run over the end of the first row
    constexpr swd3::u32 kLength = 12;
    constexpr swd3::u16 kUntouched = 0xFFFF;
    std::vector<swd3::u16> layer(Display::Pixels);

    for (std::size_t index = 0; index < layer.size(); ++index)
    {
        layer[index] = static_cast<swd3::u16>(index % 0x7FFF);
    }

    std::vector<swd3::u16> screen(kBlocks * kBlocks * Display::Pixels, kUntouched);
    FieldView::CopySpans({FieldView::Span{kStart, kLength}}, layer, kScale, screen);
    int wrong = 0;

    for (std::size_t y = 0; y < kBlocks * Display::Height; ++y)
    {
        for (std::size_t x = 0; x < kPitch; ++x)
        {
            const std::size_t index = (y / kBlocks) * Display::Width + x / kBlocks;
            const bool inside = index >= kStart && index < kStart + kLength;
            wrong += screen[y * kPitch + x] == (inside ? layer[index] : kUntouched) ? 0 : 1;
        }
    }

    CHECK_EQ(wrong, 0);
}
