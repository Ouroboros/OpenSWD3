#include "map/field.h"

#include <algorithm>
#include <cstring>
#include <span>
#include <string>

#include "config/config.h"
#include "core/debug_log.h"
#include "core/format.h"
#include "game/game_state.h"
#include "map/map_cache.h"
#include "pic/picture.h"
#include "pic/rle.h"

namespace swd3
{

namespace
{

constexpr std::string_view kLmf = "HUGE.lmf";
constexpr u32 kHeaderRead = 0x2000;
constexpr u32 kSectionRead = 0x10000;
constexpr u32 kWorkBytes = 0x20000;

u16 Word(std::span<const u8> bytes, std::size_t at)
{
    return at + 2 <= bytes.size() ? static_cast<u16>(bytes[at] | bytes[at + 1] << 8) : u16{0};
}

u32 Dword(std::span<const u8> bytes, std::size_t at)
{
    return static_cast<u32>(Word(bytes, at) | static_cast<u32>(Word(bytes, at + 2)) << 16);
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
        layer.assign(static_cast<std::size_t>(std::max(this->state.map.height, 0)) + 2 * Field::RowMargin, nullptr);
    }
}

// swd32.exe 0x00411490: `role` into the row of its y in `layer`, before the first role of the row it is not above (by
// y, then by id)
void Field::InsertRole(Role& role, int layer)
{
    const auto y = static_cast<u32>(role.y);
    const auto row = static_cast<std::size_t>(static_cast<i32>(y) / 0x10 + Field::RowMargin);
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

    for (int at = row; at < this->state.map.height; ++at)
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

        this->InsertRole(*removed, static_cast<int>(removed->flags & 3));
        return nullptr;
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

    const u32 bytes = this->unitSizes[unit] + 0x10;
    this->cache.lmf.SeekBegin(static_cast<i32>(this->unitBase + this->unitOffsets[unit]));
    const std::vector<u8> block = ReadBlock(this->cache.lmf, bytes);
    picture.width = static_cast<u16>(0x10 * Word(block, 2));
    picture.height = static_cast<u16>(0x10 * Word(block, 4));
    const u16 format = Word(block, 6);
    const u32 unpacked = Dword(block, 8);
    const u32 packed = Dword(block, 0xC);
    picture.data.assign(unpacked, 0);
    MapCache::Unpack(
        std::span<const u8>(block).subspan(0x10, std::min<std::size_t>(packed, block.size() - 0x10)),
        picture.data
    );

    if (format == 0x10)
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

// swd32.exe 0x0040D0C0: the camera centred on the controlled role (304 pixels left of it, 240 above), held inside the
// map
void Field::CenterCamera()
{
    const Role& player = this->state.Player();
    const i32 width = 0x10 * this->state.map.width;
    const i32 height = 0x10 * this->state.map.height;
    this->cameraLeft = std::max(player.x - 0x130, 0);
    this->cameraTop = std::max(player.y - 0xF0, 0);
    this->cameraRight = this->cameraLeft + 0x280;

    if (this->cameraRight >= width)
    {
        this->cameraRight = width;
        this->cameraLeft = 0x10 * (this->state.map.width - 0x28);
    }

    this->cameraBottom = this->cameraTop + 0x1E0;

    if (this->cameraBottom >= height)
    {
        this->cameraBottom = height;
        this->cameraTop = 0x10 * (this->state.map.height - 0x1E);
    }
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
    std::string path;

    if (this->config.installType == 1)
    {
        path = WinFormat("%sswd3\\%s", {this->config.cdRoot, kLmf});
    }

    if (this->config.installType == 2)
    {
        path = WinFormat("%s%s", {this->config.installDirectory, kLmf});
    }

    if (!lmf.Open(path, File::Disposition::OpenExisting, File::Access::Read))
    {
        ErrorLog(u8"Load(%d)  ,LMF can't open:%s", map, path);
        return false;
    }

    const u32 size = lmf.Size();
    u32 directory = 0;
    lmf.ReadDword(directory);
    const u32 directoryBytes = size - directory;
    const i32 entries = static_cast<i32>(directoryBytes - 1) / 0x10;
    lmf.SeekBegin(static_cast<i32>(directory));
    const std::vector<u8> table = ReadBlock(lmf, directoryBytes);
    i32 entry = 0;

    while (entry < entries && Dword(table, 0x10 * static_cast<std::size_t>(entry) + 8) != map)
    {
        ++entry;
    }

    if (entry >= entries)
    {
        systemFlags |= 4;
        ErrorLog(u8"Load(%d)  ,MapCode fail:%d", map, map);
        return false;
    }

    this->unitBase = Dword(table, 0x10 * static_cast<std::size_t>(entry));
    lmf.SeekBegin(static_cast<i32>(this->unitBase));
    std::vector<u8> block = ReadBlock(lmf, kHeaderRead);
    block.resize(kWorkBytes, 0);
    const u32 dataOffset = Dword(block, 4);
    const u32 rolesOffset = Dword(block, 0x14);
    const u32 overlaysOffset = Dword(block, 0x18);
    const u32 extrasOffset = Dword(block, 0x1C);
    const u32 unitSize = Dword(block, 0x20);
    int version = 0;

    if (Dword(block, 0) == Field::Version1)
    {
        version = 1;
    }

    if (Dword(block, 0) == Field::Version2)
    {
        version = 2;
    }
    else if (version == 0)
    {
        return false;
    }

    if (this->cache.progress)
    {
        this->cache.progress(0xF);
    }

    if (lmf.SeekBegin(static_cast<i32>(dataOffset + this->unitBase)) == 0)
    {
        ErrorLog(u8"Load(%d)  ,Seek2Map fail:%d", map, map);
        return false;
    }

    if (Word(block, 0x8E) != 0)
    {
        systemFlags |= 4;
        ErrorLog(u8"Load(%d)  ,Tile TOO much:%d", map, (Word(block, 0x8E) << 0x10) + Word(block, 0x8A));
        return false;
    }

    header.width = Word(block, 0x84);
    header.height = Word(block, 0x86);
    header.bitsPerPixel = Word(block, 0x88);
    header.unk0C = Word(block, 0x8A);
    header.layers = Word(block, 0x8C);
    header.unk14 = 0;
    this->ResetRows();
    const std::size_t nameLength = StringLength(block, 0x96);
    header.name = {};
    std::copy_n(block.begin() + 0x96, std::min<std::size_t>(nameLength, header.name.size() - 1), header.name.begin());
    this->cache.unitFile.UnmapView(header.pixels);
    header.pixels = nullptr;
    this->cache.view = nullptr;
    this->cache.unitFile.CloseMapping();
    this->cache.unitFile.Close();
    header.pixels = this->cache.GetCachedMap(map, this->unitBase, version != 1 ? unitSize : 0, header.bitsPerPixel);

    if (this->cache.progress)
    {
        this->cache.progress(0x3C);
    }

    lmf.SeekBegin(static_cast<i32>(0x96 + nameLength + 1 + this->unitBase));
    const u32 tileBytes =
        4 * static_cast<u32>(header.width) * static_cast<u32>(header.layers) * static_cast<u32>(header.height) + 4;
    const std::vector<u8> tileWords = ReadBlock(lmf, tileBytes);
    const u32 tileCount = (tileBytes - 4) >> 2;
    header.tiles.assign(tileCount, 0);

    for (u32 tile = 0; tile < tileCount; ++tile)
    {
        header.tiles[tile] = Word(tileWords, 4 * static_cast<std::size_t>(tile));
    }

    u32 next = Dword(tileWords, 4 * static_cast<std::size_t>(tileCount));
    u32 packedBlocks = next;

    if (version == 1)
    {
        lmf.SeekCurrent(static_cast<i32>(next));
        lmf.ReadDword(packedBlocks);
    }

    const std::vector<u8> packed = ReadBlock(lmf, packedBlocks + 4);
    const u32 regionCount = Dword(packed, packedBlocks);
    header.blocks.assign(static_cast<std::size_t>(header.width) * static_cast<std::size_t>(header.height), 0);
    MapCache::Unpack(
        std::span<const u8>(packed).first(packedBlocks),
        std::span<u8>(reinterpret_cast<u8*>(header.blocks.data()), header.blocks.size() * sizeof(u32))
    );
    block = ReadBlock(lmf, kSectionRead);
    std::size_t at = 4 * static_cast<std::size_t>(regionCount);

    for (u32 region = 0; region < regionCount; ++region)
    {
        auto node = std::make_unique<MapRegion>();
        node->id = Word(block, at);
        node->event = static_cast<i32>(Dword(block, at + 2));
        node->flag = static_cast<i32>(Dword(block, at + 6));
        node->unk10 = static_cast<i32>(Dword(block, at + 0xA));
        at += 0xE;
        const std::size_t length = StringLength(block, at);
        std::copy_n(
            block.begin() + static_cast<long>(at),
            std::min<std::size_t>(length, node->name.size() - 1),
            node->name.begin()
        );
        at += length + 1;
        node->next = std::move(this->regions.next);
        this->regions.next = std::move(node);
    }

    if (this->cache.progress)
    {
        this->cache.progress(0x41);
    }

    this->unitCount = static_cast<i32>(Dword(block, at));

    for (i32 unit = 0; unit < this->unitCount && unit < Field::Units; ++unit)
    {
        this->unitOffsets[static_cast<std::size_t>(unit)] = Dword(block, at + 4 + 4 * static_cast<std::size_t>(unit));
    }

    for (i32 unit = 0; unit < this->unitCount && unit < Field::Units; ++unit)
    {
        lmf.SeekBegin(static_cast<i32>(this->unitOffsets[static_cast<std::size_t>(unit)] + this->unitBase + 0xC));
        lmf.ReadDword(this->unitSizes[static_cast<std::size_t>(unit)]);
    }

    if (this->cache.progress)
    {
        this->cache.progress(0x46);
    }

    if (lmf.SeekBegin(static_cast<i32>(rolesOffset + this->unitBase)) == 0)
    {
        return false;
    }

    block = ReadBlock(lmf, kSectionRead);
    this->sectionRoles = static_cast<i32>(Dword(block, 0));
    at = 4 * (static_cast<std::size_t>(this->sectionRoles) + 1);

    for (i32 index = 0; index < this->sectionRoles; ++index)
    {
        if (this->state.roleCount < 0 || this->state.roleCount >= GameState::Roles)
        {
            break;
        }

        Role& role = this->state.roles[static_cast<std::size_t>(this->state.roleCount)];
        role.act = ActState{};
        ResetPlayback(role.act);
        const u16 placement = Word(block, at + 0xA);
        const u32 flags = (placement & 3) | 0x9000;
        role.id = 0xFFFF;
        role.flags = flags;
        role.unk28 = static_cast<u16>(placement >> 0xC);
        role.unk2A = static_cast<u16>((placement >> 8) & 0xF);
        role.x = 0x10 * static_cast<i16>(Word(block, at + 2));
        role.y = 0x10 * static_cast<i16>(Word(block, at + 8));
        role.act.id = 0;
        role.act.picture = static_cast<u16>(Word(block, at) - 1);
        role.act.x = 0;
        role.cell = nullptr;
        role.act.y = 0x10 * (Word(block, at + 8) - Word(block, at + 4));

        if (static_cast<u32>(role.y) >= static_cast<u32>(0x10 * (header.height + 0x14)) || role.y <= -0x140)
        {
            --this->state.roleCount;
        }
        else
        {
            this->InsertRole(role, static_cast<int>(flags & 3));
        }

        at += 0xC + StringLength(block, at + 0xC) + 1;
        ++this->state.roleCount;
    }

    if (this->cache.progress)
    {
        this->cache.progress(0x4B);
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
        offsets.push_back(Dword(block, 4 + 4 * static_cast<std::size_t>(overlay)));
    }

    for (const u32 offset : offsets)
    {
        if (lmf.SeekBegin(static_cast<i32>(this->unitBase + offset)) == 0)
        {
            return false;
        }

        block = ReadBlock(lmf, kHeaderRead);
        auto node = std::make_unique<MapOverlay>();
        node->width = static_cast<u16>(0x10 * Word(block, 0));
        node->height = static_cast<u16>(0x10 * Word(block, 2));
        node->left = static_cast<u16>(0x10 * Word(block, 6));
        node->top = static_cast<u16>(0x10 * Word(block, 8));
        node->right = static_cast<u16>(0x10 * Word(block, 0xA));
        node->bottom = static_cast<u16>(0x10 * Word(block, 0xC));
        node->kind = block[0xE];
        node->parallaxX = block[0xF];
        node->parallaxY = block[0x10];
        const u32 unpacked = Dword(block, 0x12);
        const u32 packedBytes = Dword(block, 0x16);
        node->picture.assign(unpacked, 0);

        if (lmf.SeekBegin(static_cast<i32>(offset + this->unitBase + 0x1A)) == 0)
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
        this->cache.progress(0x50);
    }

    lmf.SeekBegin(static_cast<i32>(extrasOffset + this->unitBase));
    block = ReadBlock(lmf, kSectionRead);
    const u32 extraCount = Dword(block, 0);
    at = 4 * (static_cast<std::size_t>(extraCount) + 1);

    for (u32 index = 0; index < extraCount; ++index)
    {
        if (this->state.roleCount < 0 || this->state.roleCount >= GameState::Roles)
        {
            break;
        }

        Role& role = this->state.roles[static_cast<std::size_t>(this->state.roleCount)];
        const u16 placement = Word(block, at + 8);
        role.act.id = Word(block, at);
        role.act.action = Word(block, at + 2);
        role.x = Word(block, at + 4);
        role.y = Word(block, at + 6);
        role.flags = placement & 3;
        role.unk28 = static_cast<u16>(placement >> 0xC);
        role.unk2A = static_cast<u16>((placement >> 8) & 0xF);
        at += 0xA + StringLength(block, at + 0xA) + 1;
        role.id = 0xFFFF;
        role.x *= 0x10;
        role.y *= 0x10;
        role.flags |= Role::Visible | Role::StepsAct;
        role.act.wait = 0;

        if (static_cast<u32>(role.y) < static_cast<u32>(0x10 * header.height))
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
        this->cache.progress(0x55);
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
        if (width == 2)
        {
            apply(start + 1);
            return;
        }

        if (width == 1)
        {
            return;
        }
    }
    else if (height == 2 && width == 1)
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
void Field::MarkRoleCells(const Role& role, u16 leader)
{
    u32 bits = (role.flags & Role::Solid) != 0 ? MapHeader::RoleCell | MapHeader::SolidRole : MapHeader::RoleCell;

    if ((role.flags & Role::Unk10) != 0)
    {
        bits |= MapHeader::Unk800000;
    }

    if (role.id == leader)
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
void Field::ClearRoleCells(const Role& role, u16 leader)
{
    const u32 mask =
        ~(MapHeader::RoleCell | MapHeader::SolidRole | MapHeader::Unk800000 |
          (role.id == leader ? MapHeader::LeaderCell : 0u));
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
                                                                        : 0xFFFFFFFF) &
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
    std::vector<u32> top(static_cast<std::size_t>(std::max(width + 2, 4)), 0);
    std::vector<u32> bottom(top.size(), 0);

    if (width > 2)
    {
        for (int index = 0; index < width + 2; ++index)
        {
            top[static_cast<std::size_t>(index)] |= read(topBase + index);
            bottom[static_cast<std::size_t>(index)] |= read(bottomBase + index);
        }

        for (std::size_t index = 0; index < static_cast<std::size_t>(width); ++index)
        {
            up |= top[index + 1];
            upLeft |= top[index];
            upRight |= top[index + 2];
            down |= bottom[index + 1];
            downLeft |= bottom[index];
            downRight |= bottom[index + 2];
        }
    }
    else
    {
        for (int index = 0; index < 3; ++index)
        {
            top[static_cast<std::size_t>(index)] |= read(topBase + index);
            bottom[static_cast<std::size_t>(index)] |= read(bottomBase + index);
        }

        if (width == 2)
        {
            top[3] |= read(topBase + 3);
            bottom[3] |= read(bottomBase + 3);
        }

        upLeft = top[0];
        downLeft = bottom[0];
        up = top[1];
        down = bottom[1];
        downRight = bottom[2];
        upRight = top[2];

        if (width == 2)
        {
            upLeft = top[1] | top[0];
            up = upRight | top[1];
            upRight |= top[3];
            downLeft = bottom[1] | bottom[0];
            down = bottom[2] | bottom[1];
            downRight = bottom[3] | bottom[2];
        }
    }

    const std::ptrdiff_t leftBase = topBase;
    const std::ptrdiff_t rightBase = cell + width - mapWidth;
    std::vector<u32> leftSide(static_cast<std::size_t>(std::max(height + 2, 4)), 0);
    std::vector<u32> rightSide(leftSide.size(), 0);

    if (height > 2)
    {
        for (int index = 0; index < height + 2; ++index)
        {
            leftSide[static_cast<std::size_t>(index)] |= read(leftBase + static_cast<std::ptrdiff_t>(index) * mapWidth);
            rightSide[static_cast<std::size_t>(index)] |=
                read(rightBase + static_cast<std::ptrdiff_t>(index) * mapWidth);
        }

        for (std::size_t index = 0; index < static_cast<std::size_t>(height); ++index)
        {
            left |= leftSide[index + 1];
            upLeft |= leftSide[index];
            downLeft |= leftSide[index + 2];
            right |= rightSide[index + 1];
            upRight |= rightSide[index];
            downRight |= rightSide[index + 2];
        }
    }
    else
    {
        for (int index = 0; index < 3; ++index)
        {
            leftSide[static_cast<std::size_t>(index)] |= read(leftBase + static_cast<std::ptrdiff_t>(index) * mapWidth);
            rightSide[static_cast<std::size_t>(index)] |=
                read(rightBase + static_cast<std::ptrdiff_t>(index) * mapWidth);
        }

        if (height == 2)
        {
            leftSide[3] |= read(leftBase + 3 * static_cast<std::ptrdiff_t>(mapWidth));
            rightSide[3] |= read(rightBase + 3 * static_cast<std::ptrdiff_t>(mapWidth));
        }

        upLeft |= leftSide[0];
        left = leftSide[1];
        downLeft |= leftSide[2];
        upRight |= rightSide[0];
        right = rightSide[1];
        const u32 lowRight = rightSide[2];
        downRight |= lowRight;

        if (height == 2)
        {
            upLeft |= leftSide[1];
            left = leftSide[2] | leftSide[1];
            downLeft |= leftSide[3];
            upRight |= rightSide[1];
            right = lowRight | rightSide[1];
            downRight |= rightSide[3];
        }
    }

    u32 result = 0;
    result |= up != 0 ? 2u : 0u;
    result |= upLeft != 0 ? 1u : 0u;
    result |= upRight != 0 ? 4u : 0u;
    result |= down != 0 ? 0x20u : 0u;
    result |= downLeft != 0 ? 0x40u : 0u;
    result |= downRight != 0 ? 0x10u : 0u;
    result |= left != 0 ? 0x80u : 0u;
    result |= right != 0 ? 8u : 0u;
    return result;
}

}  // namespace swd3
