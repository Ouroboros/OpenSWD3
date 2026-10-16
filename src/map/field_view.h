#pragma once

#include <memory>

#include "core/types.h"

namespace swd3
{

class Game;
struct Picture;
struct Role;
struct ScriptAnimation;
struct ScriptObject;

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

    i32 tileX = 0;  // 0x004CC2E4 the screen x of the first visible tile column
    i32 tileY = 0;  // 0x004CC2E8

private:
    struct TileSet;

    void DrawTileGrid(const TileSet& tiles);
    void DrawTileGridScrolled(const TileSet& tiles);
    u32 Block(int index) const;
    u16 Tile(int index) const;
    void SetTileSource(const TileSet& tiles, int index);
    void DrawScreenTint(int red, int green, int blue);

    Game& game;
};

}  // namespace swd3
