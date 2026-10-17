#include "pic/projector.h"

#include <cmath>

#include "crt/msvc.h"
#include "draw/painter.h"
#include "gfx/graphics.h"
#include "platform/display.h"
#include "platform/surface.h"

namespace swd3
{

namespace
{

constexpr int kBufferWidth = 0x500;
constexpr int kBufferHeight = 0x300;
constexpr int kScreenWidth = Display::Width;
constexpr int kScreenHeight = Display::Height;
constexpr double kDegree = 0.017444444444444446;
constexpr u16 kMirrored = 1;
constexpr u16 kMixed = 0x16;
constexpr u16 kWithinRange = 0x40;
constexpr u16 kWithinBurst = 0x80;
constexpr int kFactorRange = 0x1C;
constexpr int kQuarterTurn = 90;  // the direction tables are in degrees
constexpr int kHalfTurn = 2 * kQuarterTurn;
constexpr int kThreeQuarterTurn = 3 * kQuarterTurn;
constexpr int kFullTurn = 4 * kQuarterTurn;
constexpr i32 kSlopeUnit = 1000;       // a direction steps kSlopeUnit across
constexpr i32 kSteepSlope = 100000;    // the slope of a straight up or down direction
constexpr int kScaleBits = 10;         // Picture scales are in 1/1024
constexpr std::size_t kLowerLeft = 2;  // a particle is two by two pixels
constexpr std::size_t kLowerRight = 3;

void FillRows(std::vector<i32>& table, int width, int height)
{
    table.assign(static_cast<std::size_t>(height > 0 ? height : 0), 0);

    for (int row = 0; row < height; ++row)
    {
        table[static_cast<std::size_t>(row)] = row * width;
    }
}

void Mix(Painter& painter, u16 source, u16& target)
{
    const std::array<u16, 1> added = {source};
    painter.MixPixels(added, std::span<u16>(&target, 1));
}

}  // namespace

// swd32.exe 0x00433C40: the work buffer 0x500 x 0x300 and the screen 640 x 480; dx is -1000 for 0..89 and 271..359
// and 1000 for 91..269, dy is -1000 * tan(angle * 0.017444) mirrored around 90 and 270, with 0, 90, 180 and 270
// fixed. (The host tan differs from the x87 fptan by at most an ulp; no value lies within 1e-4 of an integer.)
Projector::Projector()
{
    this->SetBufferRows(kBufferWidth, kBufferHeight);
    this->SetScreenRows(kScreenWidth, kScreenHeight);
    this->SetClip(0, 0, kScreenWidth, kScreenHeight);

    for (int angle = 0; angle < kQuarterTurn; ++angle)
    {
        this->dirX[static_cast<std::size_t>(angle)] = -kSlopeUnit;
        const double slope = std::tan(static_cast<double>(angle) * kDegree) * -static_cast<double>(kSlopeUnit);
        this->dirY[static_cast<std::size_t>(angle)] = static_cast<i32>(crt::Ftol(slope));
    }

    for (int angle = kQuarterTurn + 1; angle < kHalfTurn; ++angle)
    {
        this->dirX[static_cast<std::size_t>(angle)] = kSlopeUnit;
        this->dirY[static_cast<std::size_t>(angle)] = this->dirY[static_cast<std::size_t>(kHalfTurn - 1 - angle)];
    }

    for (int angle = 0; angle < kQuarterTurn; ++angle)
    {
        this->dirX[static_cast<std::size_t>(kHalfTurn + angle)] = kSlopeUnit;
        this->dirY[static_cast<std::size_t>(kHalfTurn + angle)] = -this->dirY[static_cast<std::size_t>(angle)];
    }

    for (int angle = kThreeQuarterTurn + 1; angle < Projector::Directions; ++angle)
    {
        this->dirX[static_cast<std::size_t>(angle)] = -kSlopeUnit;
        this->dirY[static_cast<std::size_t>(angle)] =
            -this->dirY[static_cast<std::size_t>(Projector::Directions - 1 - angle)];
    }

    this->dirY[kHalfTurn] = 0;
    this->dirX[kQuarterTurn] = 0;
    this->dirX[kThreeQuarterTurn] = 0;
    this->dirX[kHalfTurn] = kSteepSlope;
    this->dirY[kQuarterTurn] = -kSteepSlope;
    this->dirY[kThreeQuarterTurn] = kSteepSlope;
}

// swd32.exe 0x00433D70
void Projector::Release()
{
    this->FreeParticles();
    this->screenRows.clear();
    this->bufferRows.clear();
}

// swd32.exe 0x00433DC0: the screen tables from the back buffer (pitch / 2 pixels per row)
int Projector::Init(const Graphics& graphics)
{
    i32 pitch = 0;
    i32 height = 0;
    graphics.BackSize(pitch, height);
    pitch /= static_cast<i32>(sizeof(Pixel));
    this->SetScreenRows(pitch, height);
    this->SetClip(0, 0, pitch, height);
    return this->SetBufferRows(kBufferWidth, kBufferHeight);
}

// swd32.exe 0x00433E20
int Projector::SetBufferRows(int width, int height)
{
    FillRows(this->bufferRows, width, height);
    this->bufferWidth = width;
    this->bufferHeight = height;
    return height > 0 ? height : 0;
}

// swd32.exe 0x00433E90
int Projector::SetScreenRows(int width, int height)
{
    FillRows(this->screenRows, width, height);
    this->screenWidth = width;
    this->screenHeight = height;
    return height > 0 ? height : 0;
}

// swd32.exe 0x00433F00
void Projector::FreeParticles()
{
    this->unkB68.clear();
}

// swd32.exe 0x00433F30
int Projector::SetScreen(int width, int height)
{
    this->screenWidth = width;
    this->screenHeight = height;
    this->SetScreenRows(width, height);
    return this->SetClip(0, 0, width, height);
}

// swd32.exe 0x004342E0: a rectangle past the right or bottom edge is moved back, not shrunk
int Projector::SetClip(int x, int y, int width, int height)
{
    if (x < 0)
    {
        width = x + width;
        x = 0;
    }

    if (y < 0)
    {
        height = y + height;
        y = 0;
    }

    if (x + width >= this->screenWidth)
    {
        x = this->screenWidth - width;
    }

    if (y + height >= this->screenHeight)
    {
        y = this->screenHeight - height;
    }

    this->clipTop = y;
    this->clipLeft = x;
    this->clipRight = width + x;
    this->clipBottom = height + y;
    return height + y;
}

// swd32.exe 0x00434350: one Bresenham step towards (toX, toY); true once it is reached
bool LineStep(LineWalker& walker)
{
    const int toX = walker.toX;
    int dx = toX - walker.fromX;
    int stepX = 1;
    int dy = walker.toY - walker.fromY;
    int stepY = 1;

    if (dx < 0)
    {
        stepX = -1;
        dx = walker.fromX - walker.toX;
    }

    if (dy < 0)
    {
        stepY = -1;
        dy = walker.fromY - walker.toY;
    }

    if (dx == 0)
    {
        walker.y += stepY;
    }
    else if (dy == 0)
    {
        walker.x += stepX;
    }
    else if (dx == dy)
    {
        walker.x += stepX;
        walker.y += stepY;
    }
    else if (dx > dy)
    {
        walker.errorY += dy;

        if (walker.errorY > dx >> 1)
        {
            walker.errorY -= dx;
            walker.y += stepY;
        }

        walker.x += stepX;
    }
    else
    {
        walker.errorX += dx;

        if (walker.errorX > dy >> 1)
        {
            walker.errorX -= dy;
            walker.x += stepX;
        }

        walker.y += stepY;
    }

    return walker.x == toX && walker.y == walker.toY;
}

// swd32.exe 0x00434420: one step along the direction of `walker.angle`
void Projector::Step(DirectionWalker& walker) const
{
    int stepX = 1;
    int stepY = 1;
    int dx = this->dirX[static_cast<std::size_t>(walker.angle)];
    int dy = this->dirY[static_cast<std::size_t>(walker.angle)];

    if (dx < 0)
    {
        stepX = -1;
        dx = -dx;
    }

    if (dy < 0)
    {
        stepY = -1;
        dy = -dy;
    }

    if (dx == 0)
    {
        walker.y += stepY;
    }
    else if (dy == 0)
    {
        walker.x += stepX;
    }
    else if (dx == dy)
    {
        walker.x += stepX;
        walker.y += stepY;
    }
    else if (dx > dy)
    {
        walker.errorY += dy;

        if (walker.errorY > dx >> 1)
        {
            walker.errorY -= dx;
            walker.y += stepY;
        }

        walker.x += stepX;
    }
    else
    {
        walker.errorX += dx;

        if (walker.errorX > dy >> 1)
        {
            walker.errorX -= dy;
            walker.x += stepX;
        }

        walker.y += stepY;
    }
}

// swd32.exe 0x004344E0: the picture scaled by scaleX / scaleY (1024 = 1) upwards from its bottom row at (x, y),
// skipping both colour keys; mixed with flags 0x16, mirrored with flag 1. The direction walkers are stepped but their
// positions are unused. The mirrored bottom row reads one pixel past the picture, which counts as transparent here.
void Projector::DrawScaled(Surface& surface, Painter& painter, const ProjectedPicture& picture)
{
    const i64 lastRow = static_cast<i64>(picture.width) * (picture.height - 1);
    DrawOptions& options = painter.Options();
    options.tintRed = picture.tintRed;
    options.tintGreen = picture.tintGreen;
    options.tintBlue = picture.tintBlue;
    const u16 fill = painter.Rle().fill16;
    const u16 skip = painter.Rle().skip16;
    const int width = picture.width;
    const int left = picture.x;
    const int columns = (width << kScaleBits) / picture.scaleX;
    const int rows = (picture.height << kScaleBits) / picture.scaleY;
    DirectionWalker outer;
    DirectionWalker inner;
    outer.angle = picture.angle;
    inner.angle = (picture.angle + kQuarterTurn) % kFullTurn;
    outer.y = picture.y;
    outer.x = left;
    int sourceRow = 0;
    int rowAccumulator = 0;
    int y = picture.y + rows;

    for (int row = rows; row > 0; --row, --y)
    {
        this->Step(outer);
        inner.y = outer.y;
        inner.x = outer.x;
        inner.errorX = 0;
        inner.errorY = 0;
        int x = left;
        int column = 0;
        int columnAccumulator = 0;

        for (int count = columns; count > 0; --count, ++x)
        {
            this->Step(inner);

            if (x >= 0 && x < this->screenWidth && y >= 0 && y < this->screenHeight)
            {
                const int index = (picture.flags & kMirrored) != 0 ? width - column - sourceRow : column - sourceRow;
                const i64 at = lastRow + index;

                if (at >= 0 && at < static_cast<i64>(picture.pixels.size()))
                {
                    const u16 pixel = picture.pixels[static_cast<std::size_t>(at)];

                    if (pixel != fill && pixel != skip)
                    {
                        u16& target = surface.Pixels()[this->screenRows[static_cast<std::size_t>(y)] + x];

                        if ((picture.flags & kMixed) != 0)
                        {
                            Mix(painter, pixel, target);
                        }
                        else
                        {
                            target = pixel;
                        }
                    }
                }
            }

            columnAccumulator += picture.scaleX;
            column = columnAccumulator >> kScaleBits;
        }

        rowAccumulator += picture.scaleY;
        sourceRow = width * (rowAccumulator >> kScaleBits);
    }
}

// swd32.exe 0x00434DD0: tries `count` random pixels (within particleRange with flag 0x40, within one burst with 0x80)
// and turns each opaque 2 x 2 block into a particle flying to a random point of the target rectangle; the block is
// cleared to the fill colour. With flag 0x80 alone the start position is left from the previous particle (zero for
// the first). Every particle gets an empty successor, which becomes the next free one. The original writes through a
// missing or freed free particle and outside the picture for blocks at its last row or column; here a new particle is
// appended and pixels outside the picture count as the fill colour (and are not cleared).
int Projector::ParticlePut(ParticleEffect& effect, int count, const Painter& painter, crt::Random& random)
{
    if (this->particleRange == 0)
    {
        this->particleRange = effect.burstStep;
    }

    const u16 fill = painter.Rle().fill16;
    const u16 skip = painter.Rle().skip16;
    const auto size = static_cast<i64>(effect.pixels.size());

    auto pixelAt = [&](i64 index)
    {
        return index >= 0 && index < size ? effect.pixels[static_cast<std::size_t>(index)] : fill;
    };

    auto clear = [&](i64 index)
    {
        if (index >= 0 && index < size)
        {
            effect.pixels[static_cast<std::size_t>(index)] = fill;
        }
    };

    std::list<Particle>::iterator particle;

    if (effect.list.empty())
    {
        effect.list.emplace_back();
        particle = std::prev(effect.list.end());
    }
    else if (effect.tail == effect.list.end())
    {
        effect.list.emplace_back();
        particle = std::prev(effect.list.end());
    }
    else
    {
        particle = effect.tail;
    }

    int startX = count;
    int startY = 0;

    for (int attempt = 0; attempt < count; ++attempt)
    {
        i64 index = 0;

        if ((effect.flags & kWithinRange) != 0)
        {
            const int factor = random.Next() % kFactorRange;
            index = random.Next() * factor % this->particleRange;
        }
        else if ((effect.flags & kWithinBurst) != 0)
        {
            const int factor = random.Next() % kFactorRange;
            const int product = factor * random.Next();
            index = product % (effect.pixelCount / effect.bursts);
        }
        else
        {
            const int factor = random.Next() % kFactorRange;
            index = random.Next() * factor % effect.pixelCount;
        }

        const u16 pixel = pixelAt(index);

        if (pixel == fill || pixel == skip)
        {
            continue;
        }

        ++effect.particles;
        const u32 width = effect.width;

        if ((effect.flags & kMirrored) != 0)
        {
            startX = static_cast<int>(width - static_cast<u32>(index) % width) + effect.x;
            particle->walker.fromX = startX;
        }
        else if ((effect.flags & kWithinBurst) == 0)
        {
            startX = static_cast<int>(static_cast<u32>(index) % width) + effect.x;
            particle->walker.fromX = startX;
        }

        if ((effect.flags & kMirrored) != 0 || (effect.flags & kWithinBurst) == 0)
        {
            startY = static_cast<int>(static_cast<u32>(index) / width) + effect.y;
            particle->walker.fromY = startY;
            particle->walker.x = startX;
            particle->walker.y = startY;
        }

        const int targetX = random.Next() % effect.targetWidth + effect.targetLeft;
        particle->walker.toX = targetX;
        const int targetY = random.Next() % effect.targetHeight + effect.targetTop;
        particle->walker.toY = targetY;
        const auto dx = static_cast<u32>(targetX - startX);
        const auto dy = static_cast<u32>(targetY - startY);
        u32 distance = dx * dx + dy * dy;

        if (distance == 0)
        {
            distance = 1;
        }

        const auto roll = static_cast<u32>(random.Next());
        const auto span = static_cast<u32>(crt::Ftol(std::sqrt(static_cast<double>(distance))));
        particle->life = static_cast<i32>(roll % span + effect.lifeBase);
        particle->speed = random.Next() % effect.speed + 1;
        const i64 below = index + effect.width;
        particle->pixels = {pixelAt(index), pixelAt(index + 1), pixelAt(below), pixelAt(below + 1)};
        clear(index);
        clear(index + 1);
        clear(below);
        clear(below + 1);
        particle = effect.list.emplace(std::next(particle));
        effect.tail = particle;
    }

    --effect.bursts;
    this->particleRange += effect.burstStep;

    if (effect.bursts == 0)
    {
        this->particleRange = 0;
    }

    return count;
}

// swd32.exe 0x00434790: one frame of the explosion. The first call seeds rand with the time and counts the opaque
// pixels; while bursts are left particles are emitted and the rest of the picture (but its last row) is drawn; then
// every particle moves (faster at or below slowLimit live particles), is drawn as a 2 x 2 block and is removed once
// its life ends or it reached the target rectangle. Returns 1 when everything is gone. The mirrored picture is
// clipped at x + width - column but drawn at x + column, and a mixed particle's top right pixel is written again
// unmixed. Removing the last particle makes the one before it the next free particle (the next burst overwrites it).
int Projector::DrawExplosion(Surface& surface, Painter& painter, crt::Random& random, ParticleEffect& effect)
{
    const u16 fill = painter.Rle().fill16;
    const u16 skip = painter.Rle().skip16;
    const int x = effect.x;
    int y = effect.y;

    if (effect.started == 0)
    {
        random.Seed(static_cast<u32>(crt::Time()));
        DrawOptions& options = painter.Options();
        options.tintRed = effect.tintRed;
        options.tintGreen = effect.tintGreen;
        options.tintBlue = effect.tintBlue;
        const int pixelCount = effect.height * effect.width;
        effect.pixelCount = pixelCount;
        effect.particles = 0;
        effect.started = 1;
        effect.opaque = 0;

        for (int i = 0; i < pixelCount; ++i)
        {
            const u16 pixel = effect.pixels[static_cast<std::size_t>(i)];

            if (pixel != fill && pixel != skip)
            {
                ++effect.opaque;
            }
        }

        effect.slowLimit = effect.opaque / Projector::SlowShare;
        effect.burstStep = pixelCount / effect.bursts;
    }

    if (effect.bursts > 0 && effect.particles < effect.opaque / effect.divisor)
    {
        this->ParticlePut(effect, effect.pixelCount / effect.bursts, painter, random);
    }

    if (effect.particles <= 0 && effect.bursts == 0)
    {
        return 1;
    }

    u16* pixels = surface.Pixels();

    if (effect.bursts >= 1)
    {
        const bool mirrored = (effect.flags & kMirrored) != 0;
        std::size_t row = 0;

        for (int line = 0; line < effect.height - 1; ++line, ++y)
        {
            for (int column = 0; column < effect.width; ++column)
            {
                const int sourceColumn = mirrored ? effect.width - column : column;
                const int clipX = mirrored ? sourceColumn + x : column + x;

                if (clipX < 0 || clipX >= this->screenWidth || y < 0 || y >= this->screenHeight)
                {
                    continue;
                }

                const u16 pixel = effect.pixels[row + static_cast<std::size_t>(sourceColumn)];

                if (pixel == fill || pixel == skip)
                {
                    continue;
                }

                u16& target = pixels[this->screenRows[static_cast<std::size_t>(y)] + column + x];

                if ((effect.flags & kMixed) != 0)
                {
                    Mix(painter, pixel, target);
                }
                else
                {
                    target = pixel;
                }
            }

            row += effect.width;
        }
    }

    for (auto particle = effect.list.begin(); particle != effect.list.end();)
    {
        if (effect.particles <= effect.slowLimit)
        {
            const int speed = effect.speed;
            particle->speed = random.Next() % speed + speed;
        }

        for (int step = 0; step < particle->speed; ++step)
        {
            LineStep(particle->walker);
        }

        particle->life -= particle->speed;
        const int px = particle->walker.x;
        const int py = particle->walker.y;
        bool remove = particle->life <= 0;

        if (!remove && px >= effect.targetLeft && py >= effect.targetTop &&
            px < effect.targetWidth + effect.targetLeft && py < effect.targetHeight + effect.targetTop)
        {
            remove = true;
        }

        if (!remove)
        {
            const bool mixed = (effect.flags & kMixed) != 0;

            if (px + 1 >= 0 && px + 1 < this->screenWidth && py >= 0 && py < this->screenHeight)
            {
                u16* line = pixels + this->screenRows[static_cast<std::size_t>(py)] + px;

                if (mixed)
                {
                    if (particle->pixels[0] != fill)
                    {
                        Mix(painter, particle->pixels[0], line[0]);
                    }

                    if (particle->pixels[1] != fill)
                    {
                        Mix(painter, particle->pixels[1], line[1]);
                    }

                    line[1] = particle->pixels[1];
                }
                else
                {
                    if (particle->pixels[0] != fill)
                    {
                        line[0] = particle->pixels[0];
                    }

                    if (particle->pixels[1] != fill)
                    {
                        line[1] = particle->pixels[1];
                    }
                }
            }

            if (px + 1 >= 0 && px + 1 < this->screenWidth && py + 1 >= 0 && py + 1 < this->screenHeight)
            {
                u16* line = pixels + this->screenRows[static_cast<std::size_t>(py + 1)] + px;

                if (mixed)
                {
                    if (particle->pixels[kLowerLeft] != fill)
                    {
                        Mix(painter, particle->pixels[kLowerLeft], line[0]);
                    }

                    if (particle->pixels[kLowerRight] != fill)
                    {
                        Mix(painter, particle->pixels[kLowerRight], line[1]);
                    }
                }
                else
                {
                    if (particle->pixels[kLowerLeft] != fill)
                    {
                        line[0] = particle->pixels[kLowerLeft];
                    }

                    if (particle->pixels[kLowerRight] != fill)
                    {
                        line[1] = particle->pixels[kLowerRight];
                    }
                }
            }

            ++particle;
            continue;
        }

        const bool first = particle == effect.list.begin();
        const bool last = std::next(particle) == effect.list.end();

        if (first && last)
        {
            effect.list.clear();
            effect.tail = effect.list.end();
            effect.particles = 0;
            return 0;
        }

        if (!first && last)
        {
            effect.tail = std::prev(particle);
        }
        else if (effect.tail == particle)
        {
            effect.tail = effect.list.end();
        }

        particle = effect.list.erase(particle);
        --effect.particles;
        --effect.opaque;
    }

    return 0;
}

}  // namespace swd3
