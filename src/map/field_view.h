#pragma once

#include <memory>
#include <string_view>
#include <vector>

#include "core/types.h"
#include "crt/msvc.h"
#include "file/file.h"
#include "text/big5.h"

namespace swd3
{

class Game;
struct Picture;
struct Role;
struct ScriptAnimation;
struct ScriptObject;

// A chunk of a map movie (0x24 bytes, the list at 0x004CAD28): where it lies in the file, its size and rows, and the
// size, rows and word of the chunk after it (read from its last 12 bytes).
struct MovieChunk
{
    u32 offset = 0;                    // +0x00
    u32 size = 0;                      // +0x04
    u32 rows = 0;                      // +0x08
    u32 unk0C = 0;                     // +0x0C
    u32 nextSize = 0;                  // +0x10
    u32 nextRows = 0;                  // +0x14
    u32 nextUnk0C = 0;                 // +0x18
    i32 frame = 0;                     // +0x1C
    std::unique_ptr<MovieChunk> next;  // +0x20
};

// The movie played over the map (opened by the talk scripts from Video\): LZO-packed frames of rows drawn centred,
// 8-bit frames through palettes changing at given frames, faded in and out with black bars.
struct MapMovie
{
    File file;                       // 0x004A9380
    MovieChunk chunks;               // 0x004CAD28 the list head
    MovieChunk* current = nullptr;   // 0x004C8BDC
    std::vector<u8> frame;           // 0x004B8738 the unpacked frame
    std::vector<u8> buffer;          // 0x004B7924 the packed chunk (also a saved screen)
    std::vector<u16> palette;        // 0x004B89F8
    std::vector<u16> paletteFrames;  // 0x004AB37C the frames the palettes change at
    u16 paletteCount = 0;            // 0x004B7994
    u16 paletteIndex = 0;            // 0x004ACBBC
    i32 lastFrame = 0;               // 0x004B729C
    i32 active = 0;                  // 0x004CAE8C
    i32 height = 0;                  // 0x004C97E0
    i32 frameIndex = 0;              // 0x004B7AC8 below 1 the fade in, from 10000 the fade out
    i32 saved = 0;                   // 0x004B88B8 the screen is saved in the buffer
    u32 options = 0;                 // 0x004B7AA0 bit 0: fade out darkening, bit 1: no fade in, bit 4: paused
};

// The drawing of the map (the original's picPaint, 0x00412930..0x00416CC0): its tiles, the roles by row with their
// shadows, reflections, sounds and bubbles, the script lists (animations, sprites, role heads, lines), the overlays and
// the screen effects of the map (weather, leaves, heat, lightning, fireflies, spotlight, ripples).
class FieldView
{
public:
    explicit FieldView(Game& game);

    void DrawTiles16(int unused);
    void DrawTiles16Scrolled(int unused);
    void DrawTiles8(int unused);
    void DrawTiles8Scrolled(int unused);
    void DrawRoles();
    int DrawRole(Role& role);
    void UpdateRoleSound(Role& role);
    void DrawShadow(const Role& role, const Picture& picture, int x, int y, int kind, int tint);
    void DrawReflections();
    int DrawReflection(const Role& role);
    void DrawBubbles(int x, int y, u16 role);
    void FadeColors(int step);
    void DrawAnimations(std::unique_ptr<ScriptAnimation>& list);
    void DrawSprites();
    void DrawRoleHeads();
    void DrawLines();
    void DrawOverlays();
    void DrawWeather();
    void DrawLeaves();
    void DrawHeat();
    void DrawLightning();
    void DrawFireflies();
    void DrawSpotlight();
    void DrawRipples();
    void DrawTimer(int x, int y, int which);
    void FillCellMark(int x, int y, int width, int height, int color);
    void MarkCells(u32 mask, int color, int offsetX, int offsetY, int inset);
    void PanCamera();
    void ShakeCamera();
    int UpdateMovie();
    void DrawMovieFrame();
    int ReadMovieFrame();
    void Paint();
    void Render();
    void UpdateRoleCellFlags(Role& role);

    // swd32.exe 0x004302F0: a formatted line in the small font (a UTF-8 format; UTF-8 arguments are converted to Big5,
    // char strings are game text already)
    template <typename... Args>
    void DrawDebugText(int x, int y, u32 color, std::u8string_view format, const Args&... args)
    {
        this->DrawSmallText(x, y, color, crt::Sprintf(Big5(format), GameArgument(args)...));
    }

    void DrawDebug(int cameraLeft, int cameraTop);

    i32 tileX = 0;  // 0x004CC2E4 the screen x of the first visible tile column
    i32 tileY = 0;  // 0x004CC2E8
    MapMovie movie;

private:
    struct TileSet;

    void DrawSmallText(int x, int y, u32 color, const std::string& text);
    void DrawTileGrid(const TileSet& tiles);
    void DrawTileGridScrolled(const TileSet& tiles);
    u32 Block(int index) const;
    u16 Tile(int index) const;
    void SetTileSource(const TileSet& tiles, int index);
    void DrawScreenTint(int red, int green, int blue);

    Game& game;
};

}  // namespace swd3
