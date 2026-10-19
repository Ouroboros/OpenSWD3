#pragma once

#include <array>
#include <cstddef>
#include <memory>
#include <span>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "core/glide.h"
#include "core/types.h"
#include "crt/msvc.h"
#include "file/file.h"
#include "platform/surface.h"
#include "text/big5.h"

namespace swd3
{

class Game;
class Surface;
struct Picture;
struct Role;
struct ScriptAnimation;
struct ScriptObject;

// A chunk of a map movie (0x24 bytes, the list at 0x004CAD28): where it lies in the file, its size and rows, and the
// size, rows and word of the chunk after it (read from its last 12 bytes).
struct MovieChunk
{
    static constexpr u32 TrailerBytes = 0xC;  // the next chunk's word, rows and size end each chunk

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

// The header of a map movie (0x24 bytes): its first chunk (FirstChunkBytes) and that chunk's trailer.
struct MovieHeader
{
    static constexpr std::size_t Bytes = 0x24;
    static constexpr u32 FirstChunkBytes = 0x18;

    u32 unk00 = 0;         // +0x00
    u32 lastFrame = 0;     // +0x04
    u16 bitsPerPixel = 0;  // +0x08 8: the frames index the palettes
    u16 unk0A = 0;         // +0x0A
    u16 unk0C = 0;         // +0x0C
    u16 active = 0;        // +0x0E
    u16 height = 0;        // +0x10 the rows of a frame
    u16 unk12 = 0;         // +0x12
    u16 paletteCount = 0;  // +0x14
    u16 unk16 = 0;         // +0x16
    u32 nextUnk0C = 0;     // +0x18 the trailer of the first chunk
    u32 nextRows = 0;      // +0x1C
    u32 nextSize = 0;      // +0x20

    static MovieHeader Read(std::span<const u8, Bytes> bytes);
};

// The movie played over the map (opened by the talk scripts from Video\): LZO-packed frames of rows drawn centred,
// 8-bit frames through palettes changing at given frames, faded in and out with black bars.
struct MapMovie
{
    static constexpr u32 FileHeaderBytes = 0xC;  // the chunk offsets count from after it
    static constexpr u32 PaletteColors = 0x100;
    static constexpr std::size_t RedAt = 0;  // a palette entry: red, green and blue bytes
    static constexpr std::size_t GreenAt = 1;
    static constexpr std::size_t BlueAt = 2;
    static constexpr u32 PaletteBytes = 3 * PaletteColors;    // RGB triplets, stored before PaletteTailBytes at the end
    static constexpr u32 PaletteTailBytes = 0x400;            // the frames the palettes change at (u16 each)
    static constexpr u16 IndexedBits = 8;                     // MovieHeader::bitsPerPixel of a paletted movie
    static constexpr std::size_t FrameBufferBytes = 0x9C400;  // the unpacked frame and the packed chunk buffers

    // An RGB triplet of a palette as an RGB555 pixel (the top five bits of each channel).
    static u16 Rgb555(u8 red, u8 green, u8 blue);

    // The bits of `options`.
    enum Option : u32
    {
        DarkenOut = 1,  // the fade out darkens the last frame instead of closing black bars
        NoFadeIn = 2,   // the movie starts without the bars opening
        Paused = 0x10,
    };

    static constexpr i32 FadeInStart = -13;     // `frameIndex` when the bars start opening
    static constexpr int BarStep = 4;           // lines the bars move per frame
    static constexpr i32 FadeOutStart = 10000;  // `frameIndex` once the last frame was shown
    static constexpr i32 DarkenFrames = 0xF;    // the length of the darkening fade out
    static constexpr i32 CloseFrames = 0x1E;    // the length of the closing fade out

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
    // The rebuild's continuous motion (deviations.md): the controlled role and the camera moved a millisecond at a time
    // up to `now` (SystemUptime milliseconds), the controlled role settled on each tile it reaches; their places in
    // 1/scale pixels.
    void AdvanceMotion(u32 now);
    void SettlePlayer();
    std::array<i32, 2> CameraFine(int scale) const;
    std::array<i32, 2> PlayerFine(int scale) const;
    void UpdateRoleCellFlags(Role& role);

    // swd32.exe 0x004302F0: a formatted line in the small font (a UTF-8 format; UTF-8 arguments are converted to Big5,
    // char strings are game text already)
    template <typename... Args>
    void DrawDebugText(int x, int y, u32 color, std::u8string_view format, const Args&... args)
    {
        this->DrawSmallText(x, y, color, crt::Sprintf(Big5(format), GameArgument(args)...));
    }

    void DrawDebug(int cameraLeft, int cameraTop);

    // The rebuild's smooth motion (deviations.md): the map drawn between two game frames with the camera and the roles
    // shown where they glide to at `alpha` (0..1) of the way to the next game frame, the interface of the last frame
    // laid over it. `out` is the logical screen times a whole render scale N: the camera and the roles are placed to
    // 1/N of a pixel, every pixel of the game shown as an N x N block. Nothing the game frames read is changed.
    void PaintBetween(double alpha, Surface& out);

    // A run of pixels of the logical screen (a later step of the last game frame drew over them).
    struct Span
    {
        u32 start = 0;
        u32 length = 0;
    };

    // The pixels of `spans` of a logical layer into a screen `scale` times as large, each as a scale x scale block.
    static void CopySpans(
        const std::vector<Span>& spans,
        const std::vector<Pixel>& from,
        int scale,
        std::span<Pixel> to
    );
    static void ScaleLayer(const std::vector<Pixel>& from, int scale, std::span<Pixel> to);
    // The N x N logical screens of a render scale N woven into one: screen b * N + a gives the pixels of column a
    // and row b of every block.
    static void Weave(std::span<const Surface* const> parts, int scale, std::span<Pixel> to);

    i32 tileX = 0;  // 0x004CC2E4 the screen x of the first visible tile column
    i32 tileY = 0;  // 0x004CC2E8
    MapMovie movie;
    bool replaying = false;           // PaintBetween is drawing: the state of the game frames is left alone
    std::array<i32, 2> shakeShift{};  // the quake's offset of the camera in the frame shown

private:
    struct TileSet;

    void PaintWorld();
    void ClipToSpotlight();
    void CapturePlaces();
    void AimNextFrame();
    void RefreshAim(double time);

    // Where role `index` steps next as the game frames move it (a move record or a follower's), and in how many game
    // frames: `known` false when nothing moves it.
    struct NextPlace
    {
        i32 x = 0;
        i32 y = 0;
        u32 frames = 1;
        bool known = false;
    };

    NextPlace PredictRole(std::size_t index) const;
    void CaptureLayer(std::vector<Pixel>& layer) const;

    // An element of a screen effect (a weather particle, a leaf, a bolt, a firefly, a bubble, a sprite) followed by
    // smooth motion: its glide, whether its place is on the screen rather than the map, the game frame that last
    // drew it, and where the painting under way shows it (in 1/scale pixels).
    struct EffectGlide
    {
        Glide glide;
        bool screen = false;
        u32 seen = 0;
        i32 fineX = 0;
        i32 fineY = 0;
    };

    // Where an effect element is drawn: a game frame follows its place (a move further than `snap` jumps); a
    // painting between game frames gets the place it glides to, for the block column and row being painted.
    std::array<i32, 2> FollowEffect(u64 key, i32 x, i32 y, bool screen, u32 pace, i32 snap);
    // Its place for the block column and row of a render scale painted now.
    std::array<i32, 2> EffectPlace(const EffectGlide& effect, int column, int row) const;

    bool placesValid = false;
    u32 placesFrame = 0;  // Game::gameFrames of the last map frame
    int placesMap = 0;
    i32 cameraWidth = 0;
    i32 cameraHeight = 0;
    std::vector<Glide> glides;  // the camera, then the roles
    bool keyed = false;         // the keys move the controlled role: it and the camera head for the next frame
    bool placesContinuous = false;
    std::vector<u8> rolesAimed;  // per role: heading for where the next frame puts it

    // The step of the controlled role under way (continuous motion): where it and the camera started, the walking
    // speed (pixels a game frame), the directions across and down and whether the camera follows, the milliseconds
    // gone, and the places the motion left (anything else moving them ends it).
    struct PlayerMotion
    {
        bool active = false;
        i32 startX = 0;
        i32 startY = 0;
        i32 cameraX = 0;
        i32 cameraY = 0;
        i32 speed = 0;
        i32 across = 0;
        i32 down = 0;
        i32 scrollAcross = 0;
        i32 scrollDown = 0;
        i32 elapsed = 0;
        i32 leftX = 0;
        i32 leftY = 0;
        i32 leftCameraX = 0;
        i32 leftCameraY = 0;
    };

    void MotionTick();
    void BeginPlayerMotion();
    bool TryStep();
    void PinGlides();
    void EndPlayerMotion(bool atFrame);
    i32 MotionPixels(int scale) const;
    PlayerMotion playerMotion;
    u32 motionClock = 0;
    u32 refusedFrame = 0;  // the game frame a step from the keys was last refused in (tried again next frame)
    bool refused = false;
    bool motionClockSet = false;
    std::unordered_map<u64, EffectGlide> effectGlides;
    // The painting PaintBetween does: its render scale, its camera (in 1/scale pixels), the block column and row.
    int paintScale = 1;
    i32 paintCameraX = 0;
    i32 paintCameraY = 0;
    int paintColumn = 0;
    int paintRow = 0;
    // The places PaintBetween last painted at (the camera's, then every role's), its game frame and its target.
    std::vector<i32> paintedKey;
    std::vector<i32> placesKey;
    u32 paintedFrame = 0;
    const Pixel* paintedTarget = nullptr;
    bool paintedValid = false;

    void MarkChanges(const std::vector<Pixel>& before, std::vector<Span>& spans) const;

    std::vector<Pixel> worldLayer;      // the last game frame after the map, before the interface
    std::vector<Pixel> talkLayer;       // after the lines and the talk windows
    std::vector<Pixel> headsLayer;      // after the role heads
    std::vector<Pixel> interfaceLayer;  // after the gold and the pointer, before the fade
    std::vector<Pixel> fadedLayer;      // after the fade
    std::vector<Pixel> finalLayer;      // as shown (the notices, the timer and the debug display too)
    std::vector<Span> talkSpans;        // where talkLayer differs from worldLayer
    std::vector<Span> interfaceSpans;   // where interfaceLayer differs from headsLayer
    std::vector<Span> finalSpans;       // where finalLayer differs from fadedLayer
    std::vector<Surface> parts;         // the logical screens of a render scale above 1 (PaintBetween)
    bool fadeTinted = false;

    // The levels the fade tinted the last game frame by.
    struct FadeLevels
    {
        int red = 0;
        int green = 0;
        int blue = 0;
    };

    FadeLevels fadeLevels;

    void DrawSmallText(int x, int y, u32 color, const std::string& text);
    void DrawTileGrid(const TileSet& tiles);
    void DrawTileGridScrolled(const TileSet& tiles);
    u32 Block(int index) const;
    u16 Tile(int index) const;
    void SetTileSource(const TileSet& tiles, int index, bool wideRead = false);
    void DrawScreenTint(int red, int green, int blue);

    Game& game;
};

}  // namespace swd3
