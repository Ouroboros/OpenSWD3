#pragma once

#include <array>
#include <functional>
#include <memory>
#include <source_location>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "core/types.h"
#include "draw/painter.h"
#include "font/text_font.h"
#include "ui/ui_draw.h"

namespace swd3
{

struct BattleServices;
struct Picture;
struct ScriptObject;
class Surface;
class TswArchive;

}  // namespace swd3

// The rebuild's smooth battle (deviations.md): what the parts deciding a battle frame hand to the parts drawing it.
namespace swd3::smooth
{

// A picture a view draws: its archive id and frame, taken again when drawn (a cached picture goes when the cache
// trims for another).
struct PictureRef
{
    u32 id = 0;
    u32 frame = 0;
    bool shown = false;
    bool kept = false;                     // TswArchive::LoadPictureKept
    std::shared_ptr<const Picture> owned;  // a copy of a picture outside the archive (one its owner holds)

    const Picture* Get(TswArchive& archive) const;
};

// The reference of a picture the frame holds (one in the archive's cache; none for no picture).
PictureRef RefOf(TswArchive& archive, const Picture* picture);

// A picture drawn at (x, y) with `mode`, the painter's options set first as the frame set them (each only when
// marked: the others stay as they are).
struct PictureStep
{
    PictureRef picture;
    int x = 0;
    int y = 0;
    u32 mode = 0;
    bool tinted = false;
    int tintRed = 0;
    int tintGreen = 0;
    int tintBlue = 0;
    bool palette = false;  // drawn with the picture's palette (PaletteArgument)
    bool blended = false;
    int alpha = 0;
};

using PictureSteps = std::vector<PictureStep>;

// The steps drawn in their order (a step that moves by the next frame says so to the smooth battle's record).
void DrawSteps(BattleServices& services, const PictureSteps& steps);

// What a battle frame draws, in its order: each part deciding the frame adds the draws of what it decided (a view
// drawn by the part drawing it); the original's order draws them as they come.
class Scene
{
    friend class BattlePlayer;

public:
    using Draw = std::function<void(BattleServices& services)>;
    using Where = std::source_location;

    // Each draw is added with the place of the code adding it: the smooth battle takes the draws of a frame for those
    // of the frame before by it (BattlePlayer).
    void Add(Draw draw, Where where = Where::current());
    void Add(PictureSteps steps, Where where = Where::current());
    void Add(const PictureStep& step, Where where = Where::current());
    void Add(Scene&& scene);
    void DrawAll(BattleServices& services) const;
    // The draws handed on: to the battle frame's scene (BattleServices::frame), or drawn at once without one.
    void Show(BattleServices& services);
    bool Empty() const;

private:
    struct Step
    {
        Draw draw;
        const char* file = nullptr;
        u32 line = 0;
    };

    std::vector<Step> steps;
};

// What a part deciding a frame draws with in place of the painter: each call adds to the scene, in its order, what the
// painter would do then (a picture as its cached reference, the options of the draw, the wave, the clip). The
// options are the painter's own, taken by each draw.
class ScenePainter
{
public:
    ScenePainter(BattleServices& services, Scene& scene, Scene::Where where = Scene::Where::current());

    ScenePainter(const ScenePainter&) = delete;
    ScenePainter& operator=(const ScenePainter&) = delete;

    void SetSource(const void* data);
    u32 MakeColor(u32 red, u32 green, u32 blue) const;
    // The same for a picture that may be outside the archive (its owner's): a draw of it keeps a copy.
    void SetSource(const Picture& picture);
    // The same for raw pixels of `width` x `height` (a surface): a draw keeps a copy of them as they are then.
    void SetSource(const u16* pixels, int width, int height);
    int ScreenPitch() const;
    DrawOptions& Options();
    void SetWave(int mode);
    void SetClipClamped(int left, int top, int right, int bottom);
    // 0, as the painter's draw returns (every blitter returns 0).
    int DrawPicture(int x, int y, int width, int height, u32 mode, const DrawArgument& argument = {});
    void DrawOutline(int x, int y, int width, int height, u32 mode, const DrawArgument& argument = {});
    void SetZoom(int anchorX, int anchorY, int zoomX, int zoomY);
    void DrawZoomed(int x, int y, int width, int height, bool mirrored = false);
    void DrawZoomedMirrored(int x, int y, int width, int height);
    // The screen copied into a ripple of the shape `ripple` has now, then refracted through it.
    void Refract(const ScriptObject& ripple);
    // The painter's colour shifts of the first pixels of the screen (`pixels` starts at the screen).
    void TintPixels(std::span<u16> pixels, int red, int green, int blue);
    void ShiftRed(std::span<u16> pixels, int amount);
    void ShiftGreenPairs(std::span<u16> pixels, int amount);
    void ShiftBluePairs(std::span<u16> pixels, int amount);
    // CopySurface of `from` to the frame's screen (`to`): `from` as it is now.
    void CopyScreen(const Surface& from, Surface& to);

private:
    BattleServices& services;
    Scene& scene;
    Scene::Where where;

    PictureRef Source();
    void Draw(int x, int y, int width, int height, u32 mode, const DrawArgument& argument, bool outline);

    const Picture* owner = nullptr;  // SetSource(const Picture&)
    std::shared_ptr<const Picture> ownedCopy;
    const u16* ownerPixels = nullptr;  // SetSource(const u16*, int, int)
    std::size_t ownerPixelCount = 0;
    int ownerWidth = 0;
    int ownerHeight = 0;

    std::size_t ScreenPixels(std::span<u16> pixels) const;
};

// What a part deciding a frame draws text with in place of a font: the font's settings are its own (set in the
// original's order); each text drawn adds to the scene the text with the settings it is drawn with, drawn to the
// screen of the scene's draw (the target given is the frame's screen).
class SceneFont
{
public:
    SceneFont(TextFont& font, Scene& scene, Scene::Where where = Scene::Where::current());

    void SetAdvance(int advance);
    void SetBackground(Pixel color);
    void SetShadow(Pixel color);
    void DrawText(Pixel* target, int x, int y, std::string_view text, Pixel color, int style);
    void DrawText(Pixel* target, int x, int y, std::u8string_view text, Pixel color, int style);

private:
    TextFont& font;
    Scene& scene;
    Scene::Where where;
};

// The first `count` pixels of the screen (from its start, without its pitch) set to `color`.
void FillScreen(u16 color, std::size_t count, Scene& scene, Scene::Where where = Scene::Where::current());

// FillBox (ui_draw.h) added to `scene`: it holds nothing of the frame, so it is drawn as it is.
void Box(
    Painter& painter,
    int x,
    int y,
    int width,
    int height,
    int red,
    int green,
    int blue,
    FillMode mode,
    Scene& scene,
    Scene::Where where = Scene::Where::current()
);

// DrawWindowFrame (ui_draw.h) added to `scene`: it holds nothing of the frame, so it is drawn as it is.
void WindowFrame(
    Painter& painter,
    TswArchive& archive,
    int id,
    int left,
    int top,
    int right,
    int bottom,
    int alpha,
    int border,
    Scene& scene,
    Scene::Where where = Scene::Where::current()
);

}  // namespace swd3::smooth
