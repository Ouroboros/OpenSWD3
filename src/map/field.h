#pragma once

#include <array>
#include <memory>
#include <vector>

#include "core/point.h"
#include "core/types.h"
#include "platform/display.h"
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
// A camera rectangle in pixels (four dwords in the original).
struct CameraRect
{
    i32 left = 0;
    i32 top = 0;
    i32 right = 0;
    i32 bottom = 0;
};

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
    // The camera CenterCamera gives a controlled role at (x, y): its left and top (in 1/scale pixels, as x and y).
    std::array<i32, 2> CameraCentredOn(i32 x, i32 y, int scale = 1) const;
    std::array<i32, 2> HoldCamera(i32 left, i32 top, int scale = 1) const;
    // Whether the rebuild's wider (higher) view holds the camera in its middle across (down): the map is narrower
    // (lower) than the view, so the camera does not move on that axis (HoldCamera).
    std::array<bool, 2> MapInsideView() const;
    // Whether the rebuild's view is other than the 640x480 screen (the map fills the window).
    bool WiderView() const;
    // The original's camera at (left, top), held inside the map as it holds it (HoldCamera on a 640x480 view).
    Point HoldScreen(Point place) const;
    // The original's camera moved by `by` and held.
    void MoveOriginal(Point by);

    // The sides a camera `camera` showing `width` x `height` can follow a step of the controlled role at (x, y) to: the
    // role at the middle of the view or past it that way (its place on the view taken unsigned, as the original does)
    // and the camera not at the map's edge (part of RoleControl::ControlPlayer, 0x00402F80).
    struct ScrollSides
    {
        bool left = false;
        bool right = false;
        bool up = false;
        bool down = false;
    };

    ScrollSides ScrollSidesOf(const CameraRect& camera, i32 width, i32 height, i32 x, i32 y) const;
    // A step of the controlled role at (x, y) starting by (stepX, stepY) (-1, 0 or 1 on each axis): whether the
    // original's camera scrolls with it on each axis, as the original decides it for its 640x480 camera
    // (ScrollSidesOf).
    void DecideOriginalScroll(i32 x, i32 y, int stepX, int stepY);
    // The controlled role moved by `by`: the original's camera follows it on the axes it scrolls on.
    void FollowOriginal(Point by);
    // A pan of the original's camera to `to` (held) by `step` pixels a frame on each axis; a frame of it.
    void PanOriginal(Point to, Point step);
    void StepOriginalPan();
    bool OriginalPanning() const;
    // Where the game's 640x480 screen starts on the map (the camera when the map does not fill the window): the
    // interface places what it shows of the map from here.
    i32 ScreenLeft() const;
    i32 ScreenTop() const;
    void MarkRoleCells(const Role& role, i32 leader);
    void ClearRoleCells(const Role& role, i32 leader);
    MapRegion* FindRegion(int id);
    u32 ProbeCells(std::ptrdiff_t cell, int width, int height, u32 mask) const;
    void CameraAround(int x, int y, CameraRect& rect) const;

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
    // The rebuild (deviations.md): the canvas the map is drawn to, which the camera spans (the 640x480 screen unless
    // the map fills the window), and where the game's 640x480 screen lies on it (the interface is drawn there).
    i32 viewWidth = Display::Width;
    i32 viewHeight = Display::Height;
    i32 screenX = 0;
    i32 screenY = 0;
    // The rebuild (deviations.md): where the original's 640x480 camera would be (its left and top) for what the story
    // places on that screen (its animations, the spotlight, the pans to a tile): set and moved as the camera is, held
    // inside the map as the original holds its camera; a pan takes it to `originalPan` by `originalPanStep` a frame.
    Point original;
    Point originalPan;
    Point originalPanStep;
    std::array<bool, 2> originalScrolls = {};  // DecideOriginalScroll: across and down
    std::vector<u16> palette;                  // 0x004B7404 the colours of an 8-bit map

private:
    GameState& state;
    MapCache& cache;
    const Config& config;
    const RleCodec& codec;
};

}  // namespace swd3
