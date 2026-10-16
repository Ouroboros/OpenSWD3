#pragma once

#include <array>
#include <list>
#include <span>
#include <vector>

#include "core/types.h"

namespace swd3
{

class Graphics;
class Painter;
class Surface;

namespace crt
{
class Random;
}

// A walker stepping along one of 360 directions (7 dwords in the original).
struct DirectionWalker
{
    i32 angle = 0;  // 0..359
    i32 unk04 = 0;
    i32 unk08 = 0;
    i32 x = 0;
    i32 y = 0;
    i32 errorX = 0;
    i32 errorY = 0;
};

// A Bresenham walker from one point to another (8 dwords in the original).
struct LineWalker
{
    i32 fromX = 0;
    i32 fromY = 0;
    i32 toX = 0;
    i32 toY = 0;
    i32 x = 0;
    i32 y = 0;
    i32 errorX = 0;
    i32 errorY = 0;
};

// The parameters of a scaled picture (part of a battle unit in the original).
struct ProjectedPicture
{
    std::span<const u16> pixels;  // +0x00 raw rows
    u16 width = 0;                // +0x04
    u16 height = 0;               // +0x06
    i32 unk08 = 0;
    i32 unk0C = 0;
    i32 x = 0;  // +0x10
    i32 y = 0;  // +0x14 the bottom row
    i32 unk18 = 0;
    i32 unk1C = 0;
    i32 scaleX = 0;  // +0x20 1024 = 1
    i32 scaleY = 0;  // +0x24
    u16 flags = 0;   // +0x28 1: mirrored, 0x16: mixed
    u16 unk2A = 0;
    i32 tintRed = 0;  // +0x2C the draw tint while drawing
    i32 tintGreen = 0;
    i32 tintBlue = 0;
    i32 angle = 0;  // +0x38
};

// A 2 x 2 pixel block flying along a line (0x38 bytes in the original).
struct Particle
{
    std::array<u16, 4> pixels = {};  // top left, top right, bottom left, bottom right
    i32 speed = 0;                   // line steps per frame
    i32 life = 0;
    LineWalker walker;
};

// A picture breaking up into particles that fly into a target rectangle (0x58 bytes in the original).
struct ParticleEffect
{
    std::span<u16> pixels;  // +0x00 raw rows, cleared as the particles leave
    u16 width = 0;          // +0x04
    u16 height = 0;         // +0x06
    i32 x = 0;              // +0x08
    i32 y = 0;              // +0x0C
    i32 targetLeft = 0;     // +0x10
    i32 targetWidth = 0;    // +0x14
    i32 targetTop = 0;      // +0x18
    i32 targetHeight = 0;   // +0x1C
    u16 lifeBase = 0;       // +0x20
    u16 speed = 0;          // +0x22
    u16 bursts = 0;         // +0x24 particle bursts left
    u16 divisor = 0;        // +0x26
    u16 flags = 0;          // +0x28 1: mirrored, 0x16: mixed, 0x40 / 0x80: where the pixels are chosen
    u16 unk2A = 0;
    i32 tintRed = 0;  // +0x2C the draw tint while drawing
    i32 tintGreen = 0;
    i32 tintBlue = 0;
    i32 started = 0;     // +0x38
    i32 pixelCount = 0;  // +0x3C
    i32 particles = 0;   // +0x40 live particles
    i32 slowLimit = 0;   // +0x44 opaque pixels / 20
    i32 opaque = 0;      // +0x48 opaque pixels not yet gone
    i32 burstStep = 0;   // +0x4C pixel count / bursts
    // +0x50 the particles, the head first; +0x54 the next free one (the empty successor the last burst appended).
    std::list<Particle> list;
    std::list<Particle>::iterator tail = list.end();
};

// The projection helper (the object at 0x0053B0B8): steps for 360 directions, row offsets of a work buffer and of the
// screen, and a clip rectangle; it draws scaled pictures and particle explosions.
class Projector
{
public:
    static constexpr int kDirections = 360;

    Projector();

    void Release();
    int Init(const Graphics& graphics);
    int SetBufferRows(int width, int height);
    int SetScreenRows(int width, int height);
    void FreeParticles();
    int SetScreen(int width, int height);
    int SetClip(int x, int y, int width, int height);
    void Step(DirectionWalker& walker) const;
    void DrawScaled(Surface& surface, Painter& painter, const ProjectedPicture& picture);
    int DrawExplosion(Surface& surface, Painter& painter, crt::Random& random, ParticleEffect& effect);
    int ParticlePut(ParticleEffect& effect, int count, const Painter& painter, crt::Random& random);

    std::array<i32, kDirections> dirX = {};  // +0x000
    std::array<i32, kDirections> dirY = {};  // +0x5A0
    std::vector<i32> bufferRows;             // +0xB40
    std::vector<i32> screenRows;             // +0xB44
    i32 bufferWidth = 0;
    i32 bufferHeight = 0;
    i32 screenWidth = 0;
    i32 screenHeight = 0;
    i32 clipLeft = 0;
    i32 clipTop = 0;
    i32 clipRight = 0;
    i32 clipBottom = 0;
    std::vector<u8> unkB68;  // freed by FreeParticles, never allocated
    i32 particleRange = 0;   // 0x004FB568 the pixel range of ParticlePut with flag 0x40
};

bool LineStep(LineWalker& walker);

}  // namespace swd3
