#pragma once

#include <array>
#include <map>
#include <memory>
#include <tuple>
#include <vector>

#include "battle/smooth/smooth_draw.h"
#include "core/types.h"
#include "draw/draw_placer.h"

namespace swd3
{

class Surface;
struct BattleServices;

}  // namespace swd3

// The rebuild's smooth battle (deviations.md): the stage the scenes of the battle frames are shown on. A frame is
// decided into its scene every game frame; the scene is drawn then and again at every refresh until the next, each
// picture and text moving from where the frame before drew it to where this frame draws it.
namespace swd3::smooth
{

class Stage : public DrawPlacer
{
public:
    explicit Stage(BattleServices& services);

    // The decided frame's scene drawn to the frame's screen where it draws (the frames between show it): the draws
    // of the same code (the same draw of the same step), as many as in the frame before, are each taken for the
    // nearest of those before, which the frames between move them from (others stay where they are).
    void Draw(Scene&& scene);
    // The frame drawn again `alpha` (0 to 1) of the way to its own places into `out`, N times the screen across and
    // down at the render scale N (the places in 1/N pixels, as the map's: N x N screens woven); the painter and the
    // HUD are left as they were.
    void PaintBetween(double alpha, Surface& out);
    // No frame before (a battle starts).
    void Clear();

    void Place(int& x, int& y, const void* site, const void* source) override;
    void Hold(bool held, int x, int y, const void* who) override;

private:
    // The code of a draw: the step (where it was added to the scene), where the draw is called from, what it shows
    // (DrawPlacer::Hold; none: anything) and its count among the draws of the step called from there for it.
    using Key = std::tuple<const char*, u32, const void*, const void*, u32>;

    // A draw of a frame: where it is drawn, the place it shows when held (DrawPlacer::Hold) and its picture.
    struct Drawn
    {
        std::array<int, 2> at = {};
        std::array<int, 2> stand = {};
        bool held = false;
        bool still = false;  // Scene::Still: it does not move
        const void* source = nullptr;
    };

    using Places = std::map<Key, std::vector<Drawn>>;

    // The scene drawn into `target` with the painter and the HUD left as they were.
    void DrawInto(Surface& target);
    void Replay();
    void Match();

    BattleServices& services;
    Scene scene;
    Places before;                             // the places of the frame before
    Places places;                             // the places of this frame
    std::vector<std::pair<Key, Drawn>> drawn;  // the draws of this frame in their order
    std::vector<std::array<int, 2>> moves;     // each draw's way back to its place before
    std::vector<Surface> parts;                // the screens of a render scale above 1
    bool measuring = false;
    int scale = 1;
    int column = 0;  // the screen drawn of the render scale's blocks
    int row = 0;
    double share = 1.0;
    std::size_t next = 0;
    const char* stepFile = nullptr;
    u32 stepLine = 0;
    bool stepStill = false;
    // The draws of the step so far, by where they are called and what they show.
    std::vector<std::pair<std::pair<const void*, const void*>, u32>> stepDraws;
    bool holding = false;  // Hold
    std::array<int, 2> held = {};
    const void* holdingWho = nullptr;
};

}  // namespace swd3::smooth
