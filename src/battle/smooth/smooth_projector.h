#pragma once

#include <array>
#include <memory>
#include <vector>

#include "battle/smooth/smooth_draw.h"
#include "core/types.h"

namespace swd3
{

class Painter;
class Projector;
struct ParticleEffect;
struct ProjectedPicture;

namespace crt
{
class Random;
}

}  // namespace swd3

// The rebuild's smooth battle (deviations.md): the particle explosions and projected pictures of the projector split
// into the part deciding the frame and the part drawing it.
namespace swd3::smooth
{

// What a frame of Projector::DrawExplosion draws, in its order: the rest of the picture (its rows as they are once the
// burst took its particles) and each live particle as a two by two block at (x, y).
struct ExplosionView
{
    struct Block
    {
        int x = 0;
        int y = 0;
        std::array<u16, 4> pixels = {};
    };

    bool body = false;
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
    bool mirrored = false;
    bool mixed = false;
    u16 fill = 0;
    u16 skip = 0;
    std::vector<u16> pixels;
    std::vector<Block> blocks;
};

class ExplosionFrame
{
public:
    explicit ExplosionFrame(Projector& projector);

    // Projector::DrawExplosion: its result.
    int Advance(Painter& painter, crt::Random& random, ParticleEffect& effect, ExplosionView& view);
    void Draw(u16* screen, Painter& painter, const ExplosionView& view) const;
    // The same decided into a scene (the screen and painter of the draw are the scene's).
    int Explosion(u16* screen, Painter& painter, crt::Random& random, ParticleEffect& effect, Scene& scene);
    // Projector::DrawScaled: it changes nothing of the frame but the painter's tint (set for the draws after it), so
    // the picture (its pixels kept) is drawn as it is.
    void Scaled(u16* screen, Painter& painter, const ProjectedPicture& picture, Scene& scene);

private:
    Projector& projector;
};

}  // namespace swd3::smooth
