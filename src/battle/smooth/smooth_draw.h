#pragma once

#include <array>
#include <functional>
#include <vector>

#include "core/types.h"
#include "draw/painter.h"

namespace swd3
{

struct BattleServices;
struct Picture;
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

    const Picture* Get(TswArchive& archive) const;
};

// The reference of a picture the frame holds (one in the archive's cache; none for no picture).
PictureRef RefOf(TswArchive& archive, const Picture* picture);

// A picture drawn at (x, y) with `mode`, the painter's options set first as the frame set them (each only when
// marked: the others stay as they are); `motion`: the rebuild's smooth battle reckoning.
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
    std::array<int, 2> motion = {};
};

using PictureSteps = std::vector<PictureStep>;

// The steps drawn in their order (a step that moves by the next frame says so to the smooth battle's record).
void DrawSteps(BattleServices& services, const PictureSteps& steps);

// What a battle frame draws, in its order: each part deciding the frame adds the draws of what it decided (a view
// drawn by the part drawing it); the original's order draws them as they come.
class Scene
{
public:
    using Draw = std::function<void(BattleServices& services)>;

    void Add(Draw draw);
    void Add(PictureSteps steps);
    void Add(const PictureStep& step);
    void DrawAll(BattleServices& services) const;

private:
    std::vector<Draw> draws;
};

// What a part deciding a frame draws with in place of the painter: each call adds to the scene, in its order, what the
// painter would do then (a picture as its cached reference, the options of the draw, the wave, the clip). The
// options left set without a draw are set again when it ends, as the painter keeps them for the next draw.
class ScenePainter
{
public:
    ScenePainter(BattleServices& services, Scene& scene);
    ~ScenePainter();
    ScenePainter(const ScenePainter&) = delete;
    ScenePainter& operator=(const ScenePainter&) = delete;

    void SetSource(const void* data);
    DrawOptions& Options();
    void SetWave(int mode);
    void SetClipClamped(int left, int top, int right, int bottom);
    void DrawPicture(int x, int y, int width, int height, u32 mode, const DrawArgument& argument = {});
    void SetZoom(int anchorX, int anchorY, int zoomX, int zoomY);
    void DrawZoomed(int x, int y, int width, int height, bool mirrored = false);
    void DrawZoomedMirrored(int x, int y, int width, int height);

private:
    BattleServices& services;
    Scene& scene;

    PictureRef Source() const;
    DrawOptions options;
};

}  // namespace swd3::smooth
