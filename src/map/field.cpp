#include "map/field.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <span>
#include <string>

#include "config/config.h"
#include "core/byte_reader.h"
#include "core/debug_log.h"
#include "core/format.h"
#include "core/words.h"
#include "game/game.h"
#include "game/game_state.h"
#include "map/blocking_map.h"
#include "map/map_cache.h"
#include "pic/picture.h"
#include "pic/rle.h"
#include "platform/display.h"

namespace swd3
{

namespace
{

constexpr std::string_view kLmf = "HUGE.lmf";
constexpr u32 kHeaderRead = 0x2000;
constexpr u32 kSectionRead = 0x10000;
constexpr u32 kWorkBytes = 0x20000;

// HUGE.lmf: its first dword is the offset of the directory at its end, one kDirectoryEntryBytes entry per map with the
// start of the map and its number. Offsets inside a map are from its start.
constexpr std::size_t kDirectoryEntryBytes = 0x10;
constexpr std::size_t kEntryStartAt = 0;
constexpr std::size_t kEntryMapAt = 8;

// The map header.
constexpr std::size_t kVersionAt = 0;
constexpr std::size_t kDataAt = 4;
constexpr std::size_t kRolesAt = 0x14;
constexpr std::size_t kOverlaysAt = 0x18;
constexpr std::size_t kExtrasAt = 0x1C;
constexpr std::size_t kUnitSizeAt = 0x20;
constexpr std::size_t kWidthAt = 0x84;
constexpr std::size_t kHeightAt = 0x86;
constexpr std::size_t kBitsAt = 0x88;
constexpr std::size_t kTilesLowAt = 0x8A;  // the tile count (MapHeader::unk0C); a high word is refused
constexpr std::size_t kLayersAt = 0x8C;
constexpr std::size_t kTilesHighAt = 0x8E;
constexpr std::size_t kNameAt = 0x96;  // followed by the tile words, each in a dword, and the packed blocks
constexpr int kUnknownVersion = 0;     // Field::Version1 and Version2 maps
constexpr int kFirstVersion = 1;       // its pixels are in the map, the blocks follow a skipped part
constexpr int kSecondVersion = 2;

// A unit (a picture of the map): its size in tiles, its format, the unpacked and packed sizes, then the data.
constexpr std::size_t kUnitWidthAt = 2;
constexpr std::size_t kUnitHeightAt = 4;
constexpr std::size_t kUnitFormatAt = 6;
constexpr std::size_t kUnitUnpackedAt = 8;
constexpr std::size_t kUnitPackedAt = 0xC;
constexpr std::size_t kUnitHeaderBytes = 0x10;
constexpr u16 kRleUnit = 0x10;  // a 16-bit RLE picture; other units are 8-bit tiles

// A region: its id, event, flag, a dword, then its name.
constexpr std::size_t kRegionEventAt = 2;
constexpr std::size_t kRegionFlagAt = 6;
constexpr std::size_t kRegionUnk10At = 0xA;
constexpr std::size_t kRegionNameAt = 0xE;

// A role of the role section: its picture (1-based), its tile x, the tile row of its top, its tile y, the placement
// word, then its name.
constexpr std::size_t kRoleXAt = 2;
constexpr std::size_t kRoleTopAt = 4;
constexpr std::size_t kRoleYAt = 8;
constexpr std::size_t kRolePlacementAt = 0xA;
constexpr std::size_t kRoleNameAt = 0xC;

// An extra role: its act, its action, its tile x and y, the placement word, then its name.
constexpr std::size_t kExtraActionAt = 2;
constexpr std::size_t kExtraXAt = 4;
constexpr std::size_t kExtraYAt = 6;
constexpr std::size_t kExtraPlacementAt = 8;
constexpr std::size_t kExtraNameAt = 0xA;

// The placement word of a role: the layer (Role::LayerMask), bits 8..11 and bits 12..15.
constexpr int kPlacementOffsetYShift = 8;
constexpr u16 kPlacementOffsetYMask = 0xF;
constexpr int kPlacementOffsetXShift = 0xC;

// An overlay: its size in tiles, its rectangle in tiles, its kind and parallax, the unpacked and packed sizes, then
// the data.
constexpr std::size_t kOverlayWidthAt = 0;
constexpr std::size_t kOverlayHeightAt = 2;
constexpr std::size_t kOverlayLeftAt = 6;
constexpr std::size_t kOverlayTopAt = 8;
constexpr std::size_t kOverlayRightAt = 0xA;
constexpr std::size_t kOverlayBottomAt = 0xC;
constexpr std::size_t kOverlayKindAt = 0xE;
constexpr std::size_t kOverlayParallaxXAt = 0xF;
constexpr std::size_t kOverlayParallaxYAt = 0x10;
constexpr std::size_t kOverlayUnpackedAt = 0x12;
constexpr std::size_t kOverlayPackedAt = 0x16;
constexpr std::size_t kOverlayDataAt = 0x1A;

constexpr std::size_t kCountBytes = sizeof(u32);  // a section starts with its count, then a dword per record
constexpr std::size_t kTileEntryBytes = sizeof(u32);

// The loading progress after each part (percent).
constexpr int kProgressHeader = 15;
constexpr int kProgressPixels = 60;
constexpr int kProgressRegions = 65;
constexpr int kProgressUnits = 70;
constexpr int kProgressRoles = 75;
constexpr int kProgressOverlays = 80;
constexpr int kProgressDone = 85;

constexpr i32 kTwoTiles = 2;  // a role this wide or high has its second cell set directly

u16 Word(std::span<const u8> bytes, std::size_t at)
{
    return ByteReader::Word16At(bytes, at);
}

u32 Dword(std::span<const u8> bytes, std::size_t at)
{
    return MakeWords(Word(bytes, at + sizeof(u16)), Word(bytes, at));
}

std::size_t StringLength(std::span<const u8> bytes, std::size_t at)
{
    std::size_t length = 0;

    while (at + length < bytes.size() && bytes[at + length] != 0)
    {
        ++length;
    }

    return length;
}

// Reads `bytes` at the current position (fewer at the end of the file; the rest stays zero).
std::vector<u8> ReadBlock(File& file, u32 bytes)
{
    std::vector<u8> block(bytes, 0);
    u32 read = bytes;
    file.Read(block.data(), read);
    return block;
}

// A camera's place on an axis held inside the map as the original holds it: not before the start, and at the end where
// the view would pass it (before the start on a map smaller than the view). The rebuild's view wider (higher) than
// 640x480 (`wide`) shows a map smaller than it in its middle instead.
i32 HoldPlace(i32 place, i32 extent, i32 view, bool wide)
{
    if (wide && extent < view)
    {
        return (extent - view) / 2;
    }

    place = std::max(place, 0);
    return place + view >= extent ? extent - view : place;
}

}  // namespace

Field::Field(GameState& state, MapCache& cache, const Config& config, const RleCodec& codec)
    : state(state),
      cache(cache),
      config(config),
      codec(codec)
{
}

// swd32.exe 0x00411620: the row lists of the three layers (the map's height plus 40 rows, all empty)
void Field::ResetRows()
{
    for (std::vector<Role*>& layer : this->rows)
    {
        layer.assign(
            static_cast<std::size_t>(std::max(this->state.map.height, 0)) + Field::RowMargin + Field::RowMargin,
            nullptr
        );
    }
}

// swd32.exe 0x00411490: `role` into the row of its y in `layer`, before the first role of the row it is not above (by
// y, then by id)
void Field::InsertRole(Role& role, int layer)
{
    const auto y = static_cast<u32>(role.y);
    const auto row = static_cast<std::size_t>(static_cast<i32>(y) / MapHeader::TileSize + Field::RowMargin);
    std::vector<Role*>& lists = this->rows[static_cast<std::size_t>(layer)];

    if (row >= lists.size())
    {
        return;
    }

    Role*& head = lists[row];
    Role* current = head;

    if (current == nullptr)
    {
        head = &role;
        role.next = nullptr;
        return;
    }

    Role* following = current->next;
    const auto currentY = static_cast<u32>(current->y);

    if (following == nullptr)
    {
        if (currentY <= y && current->id >= role.id)
        {
            role.next = nullptr;
            current->next = &role;
            return;
        }

        role.next = current;
        head = &role;
        return;
    }

    if (currentY <= y && role.id >= current->id)
    {
        role.next = current;
        head = &role;
        return;
    }

    for (;;)
    {
        if (static_cast<u32>(current->y) <= y && current->id >= role.id && following->id <= role.id)
        {
            break;
        }

        current = following;
        following = following->next;

        if (following == nullptr)
        {
            current->next = &role;
            role.next = nullptr;
            return;
        }
    }

    role.next = current->next;
    current->next = &role;
}

// swd32.exe 0x00411530: unlinks the role with `id` from the first row at or below `row` of `layer` holding it; with
// `keep` 1 it is returned, otherwise put back by its y (and nullptr returned)
Role* Field::RemoveRole(u16 id, int layer, int row, int keep)
{
    std::vector<Role*>& lists = this->rows[static_cast<std::size_t>(layer)];
    int at = row;

    for (; at < this->state.map.height; ++at)
    {
        const auto index = static_cast<std::size_t>(at + Field::RowMargin);

        if (index >= lists.size())
        {
            break;
        }

        Role*& head = lists[index];
        Role* first = head;

        if (first == nullptr)
        {
            continue;
        }

        Role* removed = nullptr;

        if (first->id == id)
        {
            removed = first;
            head = first->next;
        }
        else
        {
            Role* previous = first;

            for (Role* current = first->next; current != nullptr; previous = current, current = current->next)
            {
                if (current->id == id)
                {
                    removed = current;
                    previous->next = current->next;
                    break;
                }
            }
        }

        if (removed == nullptr)
        {
            continue;
        }

        removed->next = nullptr;

        if (keep == 1)
        {
            return removed;
        }

        this->InsertRole(*removed, static_cast<int>(removed->flags & Role::LayerMask));
        return nullptr;
    }

    // 0x00411530 LABEL_10: the rows searched to the map's height without the role (not when `row` starts beyond it).
    if (at == this->state.map.height)
    {
        DebugLog(u8"更新角色串列 找不到角色(%d)/[%d] ", static_cast<i32>(id), layer);
    }

    return nullptr;
}

// swd32.exe 0x0040AD10: unit `unit` of the map from HUGE.lmf unpacked into `picture` (16-bit RLE pictures converted to
// the display, 8-bit tiles to 16 bits); false past the last unit
bool Field::LoadUnitPicture(u32 unit, Picture& picture)
{
    picture.data.clear();

    if (unit > static_cast<u32>(this->unitCount) || unit >= Field::Units)
    {
        return false;
    }

    const u32 bytes = this->unitSizes[unit] + kUnitHeaderBytes;
    this->cache.lmf.SeekBegin(static_cast<i32>(this->unitBase + this->unitOffsets[unit]));
    const std::vector<u8> block = ReadBlock(this->cache.lmf, bytes);
    picture.width = static_cast<u16>(MapHeader::TileSize * Word(block, kUnitWidthAt));
    picture.height = static_cast<u16>(MapHeader::TileSize * Word(block, kUnitHeightAt));
    const u16 format = Word(block, kUnitFormatAt);
    const u32 unpacked = Dword(block, kUnitUnpackedAt);
    const u32 packed = Dword(block, kUnitPackedAt);
    picture.data.assign(unpacked, 0);
    MapCache::Unpack(
        std::span<const u8>(block)
            .subspan(kUnitHeaderBytes, std::min<std::size_t>(packed, block.size() - kUnitHeaderBytes)),
        picture.data
    );

    if (format == kRleUnit)
    {
        RleHeader header;
        this->codec.ConvertToDisplay(picture.data, header);
        picture.width = header.width;
        picture.height = header.height;
    }
    else
    {
        this->codec.ConvertTileTo16(picture.data);
    }

    return true;
}

// The pictures of the map units for the picture archive (id 0xFFFF).
bool Field::LoadMapPicture(int unit, Picture& picture)
{
    return this->LoadUnitPicture(static_cast<u32>(unit), picture);
}

// swd32.exe 0x0040D0C0: the camera centred on the controlled role (304 pixels left of it, 240 above: half the view
// less a tile across), held inside the map
void Field::CenterCamera()
{
    const Role& player = this->state.Player();
    const std::array<i32, 2> place = this->CameraCentredOn(player.x, player.y);
    this->cameraLeft = place[0];
    this->cameraTop = place[1];
    this->cameraRight = this->cameraLeft + this->viewWidth;
    this->cameraBottom = this->cameraTop + this->viewHeight;
    this->original =
        this->HoldScreen({player.x - (Display::Width / 2 - MapHeader::TileSize), player.y - Display::Height / 2});
    this->originalPan = this->original;
}

std::array<i32, 2> Field::CameraCentredOn(i32 x, i32 y, int scale) const
{
    return this
        ->HoldCamera(x - (this->viewWidth / 2 - MapHeader::TileSize) * scale, y - this->viewHeight / 2 * scale, scale);
}

// The camera at (left, top) (in 1/scale pixels) held inside the map (HoldPlace; the rebuild's wider view, see
// deviations.md).
std::array<i32, 2> Field::HoldCamera(i32 left, i32 top, int scale) const
{
    const i32 width = MapHeader::TileSize * this->state.map.width;
    const i32 height = MapHeader::TileSize * this->state.map.height;
    return {
        HoldPlace(left, width * scale, this->viewWidth * scale, this->viewWidth > Display::Width),
        HoldPlace(top, height * scale, this->viewHeight * scale, this->viewHeight > Display::Height),
    };
}

std::array<bool, 2> Field::MapInsideView() const
{
    const bool wide = this->viewWidth > Display::Width;
    const bool high = this->viewHeight > Display::Height;
    return {
        wide && MapHeader::TileSize * this->state.map.width < this->viewWidth,
        high && MapHeader::TileSize * this->state.map.height < this->viewHeight,
    };
}

bool Field::WiderView() const
{
    return this->viewWidth != Display::Width || this->viewHeight != Display::Height;
}

MapPoint Field::HoldScreen(MapPoint place) const
{
    return {
        HoldPlace(place.x, MapHeader::TileSize * this->state.map.width, Display::Width, false),
        HoldPlace(place.y, MapHeader::TileSize * this->state.map.height, Display::Height, false),
    };
}

void Field::MoveOriginal(MapPoint by)
{
    this->original = this->HoldScreen({this->original.x + by.x, this->original.y + by.y});
    this->originalPan = this->HoldScreen({this->originalPan.x + by.x, this->originalPan.y + by.y});
}

void Field::PanOriginal(MapPoint to, MapPoint step)
{
    this->originalPan = this->HoldScreen(to);
    this->originalPanStep = {std::abs(step.x), std::abs(step.y)};
}

// The original's pan steps until it is where it goes (a step that does not divide the distance stops there).
void Field::StepOriginalPan()
{
    auto approach = [](i32 place, i32 target, i32 step)
    {
        step = std::max(step, 1);
        return place + std::clamp(target - place, -step, step);
    };

    this->original.x = approach(this->original.x, this->originalPan.x, this->originalPanStep.x);
    this->original.y = approach(this->original.y, this->originalPan.y, this->originalPanStep.y);
}

bool Field::OriginalPanning() const
{
    return this->original != this->originalPan;
}

i32 Field::ScreenLeft() const
{
    return this->cameraLeft + this->screenX;
}

i32 Field::ScreenTop() const
{
    return this->cameraTop + this->screenY;
}

// swd32.exe 0x00425BE0: map `map` of HUGE.lmf (from the CD for the small installation): its header, the cached unit of
// its pixels, the tiles and blocks, the regions, the unit table, the roles of its two role sections and the overlays.
// False on a missing file, map or section (a missing map also ends the game).
bool Field::LoadMap(u32 map, u32& systemFlags)
{
    MapHeader& header = this->state.map;
    header.blocks.clear();
    header.tiles.clear();
    this->regions = MapRegion{};
    this->unitOffsets.fill(0);
    this->unitSizes.fill(0);
    this->unitCount = 0;
    File& lmf = this->cache.lmf;
    lmf.Close();
    // The original reads the minimal installation's HUGE.lmf from the CD (<cd>swd3\HUGE.lmf); the rebuild reads it
    // from the installation whatever the type (see deviations.md).
    const std::u8string path = PathFormat("%s%s", {this->config.installDirectory, kLmf});

    if (!lmf.Open(path, File::Disposition::OpenExisting, File::Access::Read))
    {
        ErrorLog(u8"Load(%d)  ,LMF can't open:%s", map, path);
        return false;
    }

    const u32 size = lmf.Size();
    u32 directory = 0;
    lmf.ReadDword(directory);
    const u32 directoryBytes = size - directory;
    const i32 entries = static_cast<i32>(directoryBytes - 1) / kDirectoryEntryBytes;
    lmf.SeekBegin(static_cast<i32>(directory));
    const std::vector<u8> table = ReadBlock(lmf, directoryBytes);
    i32 entry = 0;

    while (entry < entries && Dword(table, kDirectoryEntryBytes * static_cast<std::size_t>(entry) + kEntryMapAt) != map)
    {
        ++entry;
    }

    if (entry >= entries)
    {
        systemFlags |= Game::QuitRequested;
        ErrorLog(u8"Load(%d)  ,MapCode fail:%d", map, map);
        return false;
    }

    this->unitBase = Dword(table, kDirectoryEntryBytes * static_cast<std::size_t>(entry) + kEntryStartAt);
    lmf.SeekBegin(static_cast<i32>(this->unitBase));
    std::vector<u8> block = ReadBlock(lmf, kHeaderRead);
    block.resize(kWorkBytes, 0);
    const u32 dataOffset = Dword(block, kDataAt);
    const u32 rolesOffset = Dword(block, kRolesAt);
    const u32 overlaysOffset = Dword(block, kOverlaysAt);
    const u32 extrasOffset = Dword(block, kExtrasAt);
    const u32 unitSize = Dword(block, kUnitSizeAt);
    int version = kUnknownVersion;

    if (Dword(block, kVersionAt) == Field::Version1)
    {
        // 0x00425E5F (nullsub_1 with "LoadMap" and the text 0x004A11CC)
        DebugLog(u8"LoadMap 地圖[%d]是舊格式", map);
        version = kFirstVersion;
    }

    if (Dword(block, kVersionAt) == Field::Version2)
    {
        version = kSecondVersion;
    }
    else if (version == kUnknownVersion)
    {
        return false;
    }

    if (this->cache.progress)
    {
        this->cache.progress(kProgressHeader);
    }

    if (lmf.SeekBegin(static_cast<i32>(dataOffset + this->unitBase)) == 0)
    {
        ErrorLog(u8"Load(%d)  ,Seek2Map fail:%d", map, map);
        return false;
    }

    if (Word(block, kTilesHighAt) != 0)
    {
        systemFlags |= Game::QuitRequested;
        ErrorLog(u8"Load(%d)  ,Tile TOO much:%d", map, MakeWords(Word(block, kTilesHighAt), Word(block, kTilesLowAt)));
        return false;
    }

    header.width = Word(block, kWidthAt);
    header.height = Word(block, kHeightAt);
    header.bitsPerPixel = Word(block, kBitsAt);
    header.unk0C = Word(block, kTilesLowAt);
    header.layers = Word(block, kLayersAt);
    header.unk14 = 0;
    this->ResetRows();
    const std::size_t nameLength = StringLength(block, kNameAt);
    header.name = {};
    std::copy_n(
        block.begin() + kNameAt,
        std::min<std::size_t>(nameLength, header.name.size() - 1),
        header.name.begin()
    );
    const bool unmapped = this->cache.unitFile.UnmapView(header.pixels);
    header.pixels = nullptr;
    this->cache.view = nullptr;

    if (!unmapped)
    {
        ErrorLog(u8"Load(%d) ,CloseView fail ,%s", map, this->cache.unitFile.ErrorText());
    }

    if (!this->cache.unitFile.CloseMapping())
    {
        ErrorLog(u8"Load(%d)  ,CloseMapping fail ,%s", map, this->cache.unitFile.ErrorText());
    }

    if (!this->cache.unitFile.Close())
    {
        ErrorLog(u8"Load(%d)  ,Close fail ,%s", map, this->cache.unitFile.ErrorText());
    }

    header.pixels =
        this->cache.GetCachedMap(map, this->unitBase, version != kFirstVersion ? unitSize : 0, header.bitsPerPixel);

    if (this->cache.progress)
    {
        this->cache.progress(kProgressPixels);
    }

    lmf.SeekBegin(static_cast<i32>(kNameAt + nameLength + 1 + this->unitBase));
    const u32 tileBytes = kTileEntryBytes * static_cast<u32>(header.width) * static_cast<u32>(header.layers) *
                              static_cast<u32>(header.height) +
                          kCountBytes;
    const std::vector<u8> tileWords = ReadBlock(lmf, tileBytes);
    const u32 tileCount = static_cast<u32>((tileBytes - kCountBytes) / kTileEntryBytes);
    header.tiles.assign(tileCount, 0);

    for (u32 tile = 0; tile < tileCount; ++tile)
    {
        header.tiles[tile] = Word(tileWords, kTileEntryBytes * static_cast<std::size_t>(tile));
    }

    u32 next = Dword(tileWords, kTileEntryBytes * static_cast<std::size_t>(tileCount));
    u32 packedBlocks = next;

    if (version == kFirstVersion)
    {
        lmf.SeekCurrent(static_cast<i32>(next));
        lmf.ReadDword(packedBlocks);
    }

    const std::vector<u8> packed = ReadBlock(lmf, static_cast<u32>(packedBlocks + kCountBytes));
    const u32 regionCount = Dword(packed, packedBlocks);
    header.blocks.assign(static_cast<std::size_t>(header.width) * static_cast<std::size_t>(header.height), 0);
    MapCache::Unpack(
        std::span<const u8>(packed).first(packedBlocks),
        std::span<u8>(reinterpret_cast<u8*>(header.blocks.data()), header.blocks.size() * sizeof(u32))
    );
    block = ReadBlock(lmf, kSectionRead);
    std::size_t at = kCountBytes * static_cast<std::size_t>(regionCount);

    for (u32 region = 0; region < regionCount; ++region)
    {
        auto node = std::make_unique<MapRegion>();
        node->id = Word(block, at);
        node->event = static_cast<i32>(Dword(block, at + kRegionEventAt));
        node->flag = static_cast<i32>(Dword(block, at + kRegionFlagAt));
        node->unk10 = static_cast<i32>(Dword(block, at + kRegionUnk10At));
        at += kRegionNameAt;
        const std::size_t length = StringLength(block, at);
        std::copy_n(
            block.begin() + static_cast<std::ptrdiff_t>(at),
            std::min<std::size_t>(length, node->name.size() - 1),
            node->name.begin()
        );
        at += length + 1;
        node->next = std::move(this->regions.next);
        this->regions.next = std::move(node);
    }

    if (this->cache.progress)
    {
        this->cache.progress(kProgressRegions);
    }

    this->unitCount = static_cast<i32>(Dword(block, at));

    for (i32 unit = 0; unit < this->unitCount && unit < Field::Units; ++unit)
    {
        this->unitOffsets[static_cast<std::size_t>(unit)] =
            Dword(block, at + kCountBytes + kCountBytes * static_cast<std::size_t>(unit));
    }

    for (i32 unit = 0; unit < this->unitCount && unit < Field::Units; ++unit)
    {
        lmf.SeekBegin(
            static_cast<i32>(this->unitOffsets[static_cast<std::size_t>(unit)] + this->unitBase + kUnitPackedAt)
        );
        lmf.ReadDword(this->unitSizes[static_cast<std::size_t>(unit)]);
    }

    if (this->cache.progress)
    {
        this->cache.progress(kProgressUnits);
    }

    if (lmf.SeekBegin(static_cast<i32>(rolesOffset + this->unitBase)) == 0)
    {
        return false;
    }

    block = ReadBlock(lmf, kSectionRead);
    this->sectionRoles = static_cast<i32>(Dword(block, 0));
    at = kCountBytes * (static_cast<std::size_t>(this->sectionRoles) + 1);

    for (i32 index = 0; index < this->sectionRoles; ++index)
    {
        if (this->state.roleCount < 0 || this->state.roleCount >= GameState::Roles)
        {
            break;
        }

        Role& role = this->state.roles[static_cast<std::size_t>(this->state.roleCount)];
        role.act = ActState{};
        ResetPlayback(role.act);
        const u16 placement = Word(block, at + kRolePlacementAt);
        const u32 flags = (placement & Role::LayerMask) | Role::Visible | Role::StepsAct;
        role.id = Role::NoId;
        role.flags = flags;
        role.drawOffsetX = static_cast<u16>(placement >> kPlacementOffsetXShift);
        role.drawOffsetY = static_cast<u16>((placement >> kPlacementOffsetYShift) & kPlacementOffsetYMask);
        role.x = MapHeader::TileSize * static_cast<i16>(Word(block, at + kRoleXAt));
        role.y = MapHeader::TileSize * static_cast<i16>(Word(block, at + kRoleYAt));
        role.act.id = 0;
        role.act.picture = static_cast<u16>(Word(block, at) - 1);
        role.act.x = 0;
        role.cell = nullptr;
        role.act.y = MapHeader::TileSize * (Word(block, at + kRoleYAt) - Word(block, at + kRoleTopAt));

        if (static_cast<u32>(role.y) >= static_cast<u32>(MapHeader::TileSize * (header.height + Field::RowMargin)) ||
            role.y <= -MapHeader::TileSize * Field::RowMargin)
        {
            --this->state.roleCount;
        }
        else
        {
            this->InsertRole(role, static_cast<int>(flags & Role::LayerMask));
        }

        at += kRoleNameAt + StringLength(block, at + kRoleNameAt) + 1;
        ++this->state.roleCount;
    }

    if (this->cache.progress)
    {
        this->cache.progress(kProgressRoles);
    }

    if (lmf.SeekBegin(static_cast<i32>(overlaysOffset + this->unitBase)) == 0)
    {
        return false;
    }

    block = ReadBlock(lmf, kHeaderRead);
    const u32 overlayCount = Dword(block, 0);
    std::vector<u32> offsets;

    for (u32 overlay = 0; overlay < overlayCount; ++overlay)
    {
        offsets.push_back(Dword(block, kCountBytes + kCountBytes * static_cast<std::size_t>(overlay)));
    }

    for (const u32 offset : offsets)
    {
        if (lmf.SeekBegin(static_cast<i32>(this->unitBase + offset)) == 0)
        {
            return false;
        }

        block = ReadBlock(lmf, kHeaderRead);
        auto node = std::make_unique<MapOverlay>();
        node->width = static_cast<u16>(MapHeader::TileSize * Word(block, kOverlayWidthAt));
        node->height = static_cast<u16>(MapHeader::TileSize * Word(block, kOverlayHeightAt));
        node->left = static_cast<u16>(MapHeader::TileSize * Word(block, kOverlayLeftAt));
        node->top = static_cast<u16>(MapHeader::TileSize * Word(block, kOverlayTopAt));
        node->right = static_cast<u16>(MapHeader::TileSize * Word(block, kOverlayRightAt));
        node->bottom = static_cast<u16>(MapHeader::TileSize * Word(block, kOverlayBottomAt));
        node->kind = block[kOverlayKindAt];
        node->parallaxX = block[kOverlayParallaxXAt];
        node->parallaxY = block[kOverlayParallaxYAt];
        const u32 unpacked = Dword(block, kOverlayUnpackedAt);
        const u32 packedBytes = Dword(block, kOverlayPackedAt);
        node->picture.assign(unpacked, 0);

        if (lmf.SeekBegin(static_cast<i32>(offset + this->unitBase + kOverlayDataAt)) == 0)
        {
            return false;
        }

        const std::vector<u8> picture = ReadBlock(lmf, packedBytes);
        MapCache::Unpack(picture, node->picture);
        RleHeader rle;
        this->codec.ConvertToDisplay(node->picture, rle);
        node->width = rle.width;
        node->height = rle.height;
        node->next = std::move(this->overlays);
        this->overlays = std::move(node);
    }

    if (this->cache.progress)
    {
        this->cache.progress(kProgressOverlays);
    }

    lmf.SeekBegin(static_cast<i32>(extrasOffset + this->unitBase));
    block = ReadBlock(lmf, kSectionRead);
    const u32 extraCount = Dword(block, 0);
    at = kCountBytes * (static_cast<std::size_t>(extraCount) + 1);

    for (u32 index = 0; index < extraCount; ++index)
    {
        if (this->state.roleCount < 0 || this->state.roleCount >= GameState::Roles)
        {
            break;
        }

        Role& role = this->state.roles[static_cast<std::size_t>(this->state.roleCount)];
        const u16 placement = Word(block, at + kExtraPlacementAt);
        role.act.id = Word(block, at);
        role.act.action = Word(block, at + kExtraActionAt);
        role.x = Word(block, at + kExtraXAt);
        role.y = Word(block, at + kExtraYAt);
        role.flags = placement & Role::LayerMask;
        role.drawOffsetX = static_cast<u16>(placement >> kPlacementOffsetXShift);
        role.drawOffsetY = static_cast<u16>((placement >> kPlacementOffsetYShift) & kPlacementOffsetYMask);
        at += kExtraNameAt + StringLength(block, at + kExtraNameAt) + 1;
        role.id = Role::NoId;
        role.x *= MapHeader::TileSize;
        role.y *= MapHeader::TileSize;
        role.flags |= Role::Visible | Role::StepsAct;
        role.act.wait = 0;

        if (static_cast<u32>(role.y) < static_cast<u32>(MapHeader::TileSize * header.height))
        {
            this->InsertRole(role, static_cast<int>(role.flags & Role::LayerMask));
        }
        else
        {
            --this->state.roleCount;
            role.x = 0;
            role.y = 0;
        }

        ++this->state.roleCount;
    }

    if (this->cache.progress)
    {
        this->cache.progress(kProgressDone);
    }

    return true;
}

namespace
{

// The cells a role covers from its own cell: its width and height in tiles (the act's 'HW'); the original walks them
// through a pointer into the grid.
template <typename Change>
void ChangeRoleCells(std::vector<u32>& blocks, i32 mapWidth, const Role& role, Change change)
{
    if (role.cell == nullptr || blocks.empty())
    {
        return;
    }

    const auto start = static_cast<std::ptrdiff_t>(role.cell - blocks.data());
    const i32 width = role.act.width;
    const i32 height = role.act.height;

    auto apply = [&](std::ptrdiff_t at)
    {
        if (at >= 0 && static_cast<std::size_t>(at) < blocks.size())
        {
            change(blocks[static_cast<std::size_t>(at)]);
        }
    };

    apply(start);

    if (height == 1)
    {
        if (width == kTwoTiles)
        {
            apply(start + 1);
            return;
        }

        if (width == 1)
        {
            return;
        }
    }
    else if (height == kTwoTiles && width == 1)
    {
        apply(start + mapWidth);
        return;
    }

    std::ptrdiff_t at = start;

    for (i32 row = 0; row < height; ++row)
    {
        for (i32 column = 0; column < width; ++column)
        {
            apply(at++);
        }

        at += mapWidth - width;
    }
}

}  // namespace

// swd32.exe 0x0040AEC0: the cells under a role marked taken (bit 28; bit 29 too for flag 0x4000, bit 23 for flag 0x10,
// bit 8 for the leader); the first cell is marked once more by the walk
void Field::MarkRoleCells(const Role& role, i32 leader)
{
    u32 bits = (role.flags & Role::Solid) != 0 ? MapHeader::RoleCell | MapHeader::SolidRole : MapHeader::RoleCell;

    if ((role.flags & Role::Unk10) != 0)
    {
        bits |= MapHeader::Unk800000;
    }

    if (static_cast<i32>(role.id) == leader)
    {
        bits |= MapHeader::LeaderCell;
    }

    ChangeRoleCells(
        this->state.map.blocks,
        this->state.map.width,
        role,
        [bits](u32& cell)
        {
            cell |= bits;
        }
    );
}

// swd32.exe 0x0040AE20: the marks of a role taken off its cells (the leader's bit 8 too)
void Field::ClearRoleCells(const Role& role, i32 leader)
{
    const u32 mask =
        ~(MapHeader::RoleCell | MapHeader::SolidRole | MapHeader::Unk800000 |
          (static_cast<i32>(role.id) == leader ? MapHeader::LeaderCell : 0u));
    ChangeRoleCells(
        this->state.map.blocks,
        this->state.map.width,
        role,
        [mask](u32& cell)
        {
            cell &= mask;
        }
    );
}

// swd32.exe 0x0040DC30: the region with `id` after the list head (nullptr when none)
MapRegion* Field::FindRegion(int id)
{
    for (MapRegion* region = this->regions.next.get(); region != nullptr; region = region->next.get())
    {
        if (region->id == id)
        {
            return region;
        }
    }

    return nullptr;
}

// swd32.exe 0x0040BB50: the cells around a footprint of width x height cells at `cell` holding bits of `mask`, by
// direction: 1 up left, 2 up, 4 up right, 8 right, 0x10 down right, 0x20 down, 0x40 down left, 0x80 left. Footprints of
// at most 2 cells take an unrolled path that treats sizes below 1 like 1. The original keeps the four edges in one
// 32-entry stack array whose parts overlap for footprints above 6 cells and run past it from 7 cells high; here they
// are separate. A cell outside the grid has every bit set.
u32 Field::ProbeCells(std::ptrdiff_t cell, int width, int height, u32 mask) const
{
    const std::vector<u32>& blocks = this->state.map.blocks;
    const int mapWidth = this->state.map.width;

    auto read = [&blocks, mask](std::ptrdiff_t at) -> u32
    {
        return (at >= 0 && static_cast<std::size_t>(at) < blocks.size() ? blocks[static_cast<std::size_t>(at)]
                                                                        : BlockingMap::Outside) &
               mask;
    };

    u32 upLeft = 0;
    u32 up = 0;
    u32 upRight = 0;
    u32 right = 0;
    u32 downRight = 0;
    u32 down = 0;
    u32 downLeft = 0;
    u32 left = 0;
    const std::ptrdiff_t topBase = cell - (mapWidth + 1);
    const std::ptrdiff_t bottomBase = cell + static_cast<std::ptrdiff_t>(height) * mapWidth - 1;
    std::vector<u32> top(
        static_cast<std::size_t>(std::max(width + FootprintEdge::Corners, FootprintEdge::SmallCells)),
        0
    );
    std::vector<u32> bottom(top.size(), 0);

    if (width > FootprintEdge::SmallFootprint)
    {
        for (int index = 0; index < width + FootprintEdge::Corners; ++index)
        {
            top[static_cast<std::size_t>(index)] |= read(topBase + index);
            bottom[static_cast<std::size_t>(index)] |= read(bottomBase + index);
        }

        for (std::size_t index = 0; index < static_cast<std::size_t>(width); ++index)
        {
            up |= top[index + FootprintEdge::First];
            upLeft |= top[index];
            upRight |= top[index + FootprintEdge::AfterOne];
            down |= bottom[index + FootprintEdge::First];
            downLeft |= bottom[index];
            downRight |= bottom[index + FootprintEdge::AfterOne];
        }
    }
    else
    {
        for (int index = 0; index < static_cast<int>(FootprintEdge::AfterTwo); ++index)
        {
            top[static_cast<std::size_t>(index)] |= read(topBase + index);
            bottom[static_cast<std::size_t>(index)] |= read(bottomBase + index);
        }

        if (width == FootprintEdge::SmallFootprint)
        {
            top[FootprintEdge::AfterTwo] |= read(topBase + static_cast<std::ptrdiff_t>(FootprintEdge::AfterTwo));
            bottom[FootprintEdge::AfterTwo] |= read(bottomBase + static_cast<std::ptrdiff_t>(FootprintEdge::AfterTwo));
        }

        upLeft = top[FootprintEdge::Before];
        downLeft = bottom[FootprintEdge::Before];
        up = top[FootprintEdge::First];
        down = bottom[FootprintEdge::First];
        downRight = bottom[FootprintEdge::AfterOne];
        upRight = top[FootprintEdge::AfterOne];

        if (width == FootprintEdge::SmallFootprint)
        {
            upLeft = top[FootprintEdge::First] | top[FootprintEdge::Before];
            up = upRight | top[FootprintEdge::First];
            upRight |= top[FootprintEdge::AfterTwo];
            downLeft = bottom[FootprintEdge::First] | bottom[FootprintEdge::Before];
            down = bottom[FootprintEdge::AfterOne] | bottom[FootprintEdge::First];
            downRight = bottom[FootprintEdge::AfterTwo] | bottom[FootprintEdge::AfterOne];
        }
    }

    const std::ptrdiff_t leftBase = topBase;
    const std::ptrdiff_t rightBase = cell + width - mapWidth;
    std::vector<u32> leftSide(
        static_cast<std::size_t>(std::max(height + FootprintEdge::Corners, FootprintEdge::SmallCells)),
        0
    );
    std::vector<u32> rightSide(leftSide.size(), 0);

    if (height > FootprintEdge::SmallFootprint)
    {
        for (int index = 0; index < height + FootprintEdge::Corners; ++index)
        {
            leftSide[static_cast<std::size_t>(index)] |= read(leftBase + static_cast<std::ptrdiff_t>(index) * mapWidth);
            rightSide[static_cast<std::size_t>(index)] |=
                read(rightBase + static_cast<std::ptrdiff_t>(index) * mapWidth);
        }

        for (std::size_t index = 0; index < static_cast<std::size_t>(height); ++index)
        {
            left |= leftSide[index + FootprintEdge::First];
            upLeft |= leftSide[index];
            downLeft |= leftSide[index + FootprintEdge::AfterOne];
            right |= rightSide[index + FootprintEdge::First];
            upRight |= rightSide[index];
            downRight |= rightSide[index + FootprintEdge::AfterOne];
        }
    }
    else
    {
        for (int index = 0; index < static_cast<int>(FootprintEdge::AfterTwo); ++index)
        {
            leftSide[static_cast<std::size_t>(index)] |= read(leftBase + static_cast<std::ptrdiff_t>(index) * mapWidth);
            rightSide[static_cast<std::size_t>(index)] |=
                read(rightBase + static_cast<std::ptrdiff_t>(index) * mapWidth);
        }

        if (height == FootprintEdge::SmallFootprint)
        {
            leftSide[FootprintEdge::AfterTwo] |=
                read(leftBase + static_cast<std::ptrdiff_t>(FootprintEdge::AfterTwo) * mapWidth);
            rightSide[FootprintEdge::AfterTwo] |=
                read(rightBase + static_cast<std::ptrdiff_t>(FootprintEdge::AfterTwo) * mapWidth);
        }

        upLeft |= leftSide[FootprintEdge::Before];
        left = leftSide[FootprintEdge::First];
        downLeft |= leftSide[FootprintEdge::AfterOne];
        upRight |= rightSide[FootprintEdge::Before];
        right = rightSide[FootprintEdge::First];
        const u32 lowRight = rightSide[FootprintEdge::AfterOne];
        downRight |= lowRight;

        if (height == FootprintEdge::SmallFootprint)
        {
            upLeft |= leftSide[FootprintEdge::First];
            left = leftSide[FootprintEdge::AfterOne] | leftSide[FootprintEdge::First];
            downLeft |= leftSide[FootprintEdge::AfterTwo];
            upRight |= rightSide[FootprintEdge::First];
            right = lowRight | rightSide[FootprintEdge::First];
            downRight |= rightSide[FootprintEdge::AfterTwo];
        }
    }

    u32 result = 0;
    result |= up != 0 ? BlockingMap::ProbeUp : 0u;
    result |= upLeft != 0 ? BlockingMap::ProbeUpLeft : 0u;
    result |= upRight != 0 ? BlockingMap::ProbeUpRight : 0u;
    result |= down != 0 ? BlockingMap::ProbeDown : 0u;
    result |= downLeft != 0 ? BlockingMap::ProbeDownLeft : 0u;
    result |= downRight != 0 ? BlockingMap::ProbeDownRight : 0u;
    result |= left != 0 ? BlockingMap::ProbeLeft : 0u;
    result |= right != 0 ? BlockingMap::ProbeRight : 0u;
    return result;
}

}  // namespace swd3
