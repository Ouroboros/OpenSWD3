#pragma once

#include <cstdint>

namespace swd3 {

class Graphics;

// Direction walker of Projector::Step (7 dwords).
struct DirectionWalker {
    std::int32_t angle;     // +0x00 0..359
    std::int32_t unknown4;
    std::int32_t unknown8;
    std::int32_t x;         // +0x0C
    std::int32_t y;         // +0x10
    std::int32_t errorX;    // +0x14
    std::int32_t errorY;    // +0x18
};

// Bresenham line walker of LineStep (8 dwords).
struct LineWalker {
    std::int32_t fromX;
    std::int32_t fromY;
    std::int32_t toX;
    std::int32_t toY;
    std::int32_t x;
    std::int32_t y;
    std::int32_t errorX;
    std::int32_t errorY;
};

// Scaled picture blit parameters of Projector::DrawScaled.
struct ProjectedPicture {
    std::uint16_t* pixels;   // +0x00 raw rows
    std::uint16_t width;     // +0x04
    std::uint16_t height;    // +0x06
    std::int32_t unknown8;
    std::int32_t unknownC;
    std::int32_t x;          // +0x10
    std::int32_t y;          // +0x14 bottom row
    std::int32_t unknown18;
    std::int32_t unknown1C;
    std::int32_t scaleX;     // +0x20 1024 = 1
    std::int32_t scaleY;     // +0x24
    std::uint16_t flags;     // +0x28 1: mirrored, 0x16: mixed
    std::uint16_t unknown2A;
    std::int32_t tint[3];    // +0x2C draw options 0x71C / 0x30C / 0x304
    std::int32_t angle;      // +0x38
};

// Particle of the explosion effect (0x38 bytes): a 2 x 2 pixel block flying along a line.
struct Particle {
    std::uint16_t pixels[4];   // +0x00 top left, top right, bottom left, bottom right
    std::int32_t speed;        // +0x08 line steps per frame
    std::int32_t life;         // +0x0C
    LineWalker walker;         // +0x10
    Particle* previous;        // +0x30
    Particle* next;            // +0x34
};

static_assert(sizeof(Particle) == 0x38);

// Picture breaking up into particles that fly into a target rectangle (0x58 bytes).
struct ParticleEffect {
    std::uint16_t* pixels;     // +0x00 raw rows, consumed as particles leave
    std::uint16_t width;       // +0x04
    std::uint16_t height;      // +0x06
    std::int32_t x;            // +0x08
    std::int32_t y;            // +0x0C
    std::int32_t targetLeft;   // +0x10
    std::int32_t targetWidth;  // +0x14
    std::int32_t targetTop;    // +0x18
    std::int32_t targetHeight; // +0x1C
    std::uint16_t lifeBase;    // +0x20
    std::uint16_t speed;       // +0x22
    std::uint16_t bursts;      // +0x24 particle bursts left
    std::uint16_t divisor;     // +0x26
    std::uint16_t flags;       // +0x28 1: mirrored, 0x16: mixed, 0x40 / 0x80: pixel choice
    std::uint16_t unknown2A;
    std::int32_t tint[3];      // +0x2C draw options 0x71C / 0x30C / 0x304
    std::int32_t started;      // +0x38
    std::int32_t pixelCount;   // +0x3C
    std::int32_t particles;    // +0x40 live particles
    std::int32_t slowLimit;    // +0x44 opaque pixels / 20
    std::int32_t opaque;       // +0x48 opaque pixels not yet gone
    std::int32_t burstStep;    // +0x4C pixel count / bursts
    Particle* head;            // +0x50
    Particle* tail;            // +0x54 next free particle
};

static_assert(sizeof(ParticleEffect) == 0x58);

// Projection helper (global at 0x0053B0B8, 0xB6C bytes): direction steps for 360 degrees (dx -1000.., dy
// -1000 * tan), row offset tables of a work buffer and of the screen, a clip rectangle and a particle buffer.
class Projector {
public:
    Projector();
    void Release();
    int Init(Graphics* graphics);
    int SetBufferRows(int width, int height);
    int SetScreenRows(int width, int height);
    void FreeParticles();
    int SetScreen(int width, int height);
    int SetClip(int x, int y, int width, int height);
    DirectionWalker* Step(DirectionWalker* walker);
    void DrawScaled(std::uint16_t* surface, const ProjectedPicture* picture);
    int DrawExplosion(std::uint16_t* surface, ParticleEffect* effect);

    std::int32_t m_dirX[360];        // +0x000
    std::int32_t m_dirY[360];        // +0x5A0
    std::int32_t* m_bufferRows;      // +0xB40
    std::int32_t* m_screenRows;      // +0xB44
    std::int32_t m_bufferWidth;      // +0xB48
    std::int32_t m_bufferHeight;     // +0xB4C
    std::int32_t m_screenWidth;      // +0xB50
    std::int32_t m_screenHeight;     // +0xB54
    std::int32_t m_clipLeft;         // +0xB58
    std::int32_t m_clipTop;          // +0xB5C
    std::int32_t m_clipRight;        // +0xB60
    std::int32_t m_clipBottom;       // +0xB64
    void* m_particles;               // +0xB68
};

static_assert(sizeof(Projector) == 0xB6C);

bool LineStep(LineWalker* walker);
int ParticlePut(ParticleEffect* effect, int count);

}  // namespace swd3
