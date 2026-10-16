#pragma once

#include <array>
#include <memory>
#include <vector>

#include "core/types.h"
#include "res/tsw_archive.h"

namespace swd3
{

class Config;
class GameState;
class MapCache;
class RleCodec;
struct Picture;
struct Role;

// A named region of the map (0x34 bytes; the list at 0x004ACAC0 starts with a node of its own).
struct MapRegion
{
    static constexpr i32 EventMask = 0x7FFF;      // in `event`
    static constexpr u32 ClickFlagMask = 0xFFFF;  // in `flag`: the story flag that lets it be clicked
    static constexpr u32 BlockFlagShift = 16;     // in `flag`: above it, the story flag that blocks it

    std::unique_ptr<MapRegion> next;   // +0x00
    i32 id = 0;                        // +0x04
    i32 event = 0;                     // +0x08 bit 15 aside: the event run when the region is clicked
    i32 flag = 0;                      // +0x0C ClickFlagMask and BlockFlagShift
    i32 unk10 = 0;                     // +0x10
    std::array<char, 0x20> name = {};  // +0x14
};

// A picture laid over the map (0x20 bytes, the list at 0x004AB2F4): its RLE picture and placement.
struct MapOverlay
{
    static constexpr int Orders = 0x1F;        // the drawing orders 0..30
    static constexpr u16 FixedToMap = 8;       // a parallax that moves with the map
    static constexpr int ParallaxUnit = 0x10;  // other parallaxes are in sixteenths of the camera's distance

    std::vector<u8> picture;           // +0x00 a 16-bit RLE picture
    u16 kind = 0;                      // +0x04 the drawing order (0..30)
    u16 width = 0;                     // +0x06
    u16 height = 0;                    // +0x08
    u16 left = 0;                      // +0x0A map pixels of the area it shows in
    u16 top = 0;                       // +0x0C
    u16 right = 0;                     // +0x0E
    u16 bottom = 0;                    // +0x10
    u16 parallaxX = 0;                 // +0x16 FixedToMap or sixteenths (ParallaxUnit)
    u16 parallaxY = 0;                 // +0x18
    std::unique_ptr<MapOverlay> next;  // +0x1C
};

// The map the party is on: HUGE.lmf's map (its blocks, tiles, units and roles), the drawing order of the roles by row
// for the three layers, the regions, the overlays and the camera.
class Field : public MapPictureSource
{
public:
    // Map numbers the code tests, named by their place names in maps.dat (table 0x14).
    enum Map : int
    {
        TwoRiversPlain = 6,             // 兩河平原
        WesternRegions = 8,             // 西域
        DemonRefiningPot = 0x16,        // 煉妖壺
        MonasteryOutside = 0x32,        // 修道院外
        VeniceTavern = 0x51,            // 威尼斯酒屋
        MediterraneanEastCoast = 0xC8,  // 地中海東岸
    };

    static constexpr int Layers = 3;
    static constexpr int RowMargin = 0x14;
    static constexpr int Units = 0x300;
    static constexpr u32 Version1 = 0x7046534D;  // "MSFp"
    static constexpr u32 Version2 = 0x3246534D;  // "MSF2"

    Field(GameState& state, MapCache& cache, const Config& config, const RleCodec& codec);

    bool LoadMap(u32 map, u32& systemFlags);
    void ResetRows();
    void InsertRole(Role& role, int layer);
    Role* RemoveRole(u16 id, int layer, int row, int keep);
    bool LoadUnitPicture(u32 unit, Picture& picture);
    bool LoadMapPicture(int unit, Picture& picture) override;
    void CenterCamera();
    void MarkRoleCells(const Role& role, u16 leader);
    void ClearRoleCells(const Role& role, u16 leader);
    MapRegion* FindRegion(int id);
    u32 ProbeCells(std::ptrdiff_t cell, int width, int height, u32 mask) const;
    void CameraAround(int x, int y, std::array<i32, 4>& rect) const;

    MapRegion regions;                            // 0x004ACAC0
    std::unique_ptr<MapOverlay> overlays;         // 0x004AB2F4
    std::array<std::vector<Role*>, Layers> rows;  // 0x004A9A04 the roles by row, RowMargin rows above the map
    i32 unitCount = 0;                            // 0x004CDEA4
    std::array<u32, Units> unitOffsets = {};      // 0x004CDEA8
    u32 unitBase = 0;                             // 0x004CEAA8 the map in HUGE.lmf
    std::array<u32, Units> unitSizes = {};        // 0x004CEAAC
    i32 sectionRoles = 0;                         // 0x004CDEA0
    i32 cameraLeft = 0;                           // 0x004AB980
    i32 cameraTop = 0;                            // 0x004AB984
    i32 cameraRight = 0;                          // 0x004AB988
    i32 cameraBottom = 0;                         // 0x004AB98C
    std::vector<u16> palette;                     // 0x004B7404 the colours of an 8-bit map

private:
    GameState& state;
    MapCache& cache;
    const Config& config;
    const RleCodec& codec;
};

}  // namespace swd3
