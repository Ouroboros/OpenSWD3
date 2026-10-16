#include "pic/projector.h"

#include <cmath>
#include <cstring>

#include "crt/crt.h"
#include "data/globals.h"
#include "draw/pixel_ops.h"
#include "gfx/graphics.h"
#include "swd3/w32.h"

namespace swd3 {

namespace {

int32_t* rowTable(int32_t*& table, int width, int height) {
    if (table != nullptr) {
        crt::free(table);
        table = nullptr;
    }
    table = static_cast<int32_t*>(crt::malloc(static_cast<std::size_t>(4 * height)));
    return table;
}

}  // namespace

// swd32.exe 0x00433C40: buffer rows 0x500 x 0x300, screen 640 x 480; dx is -1000 for 0..89 and 271..359 and
// 1000 for 91..269, dy is -1000 * tan(angle * 0.0174444) mirrored around 90 and 270, with 0, 90 and 180 fixed.
// (The host tan differs from x87 fptan by at most an ulp; no value lies within 1e-4 of an integer.)
Projector::Projector() {
    m_bufferRows = nullptr;
    m_screenRows = nullptr;
    SetBufferRows(0x500, 0x300);
    SetScreenRows(0x280, 0x1E0);
    SetClip(0, 0, 0x280, 0x1E0);
    for (int angle = 0; angle < 0x5A; ++angle) {
        m_dirX[angle] = -1000;
        const double slope = std::tan(static_cast<double>(angle) * 0.017444444444444446) * -1000.0;
        m_dirY[angle] = static_cast<int32_t>(crt::ftol(slope));
    }
    for (int step = 0; step < 0x59; ++step) {
        m_dirX[0x5B + step] = 1000;
        m_dirY[0x5B + step] = m_dirY[0x58 - step];
    }
    for (int angle = 0; angle < 0x5A; ++angle) {
        m_dirX[0xB4 + angle] = 1000;
        m_dirY[0xB4 + angle] = -m_dirY[angle];
    }
    for (int step = 0; step < 0x59; ++step) {
        m_dirX[0x10F + step] = -1000;
        m_dirY[0x10F + step] = -m_dirY[0x58 - step];
    }
    m_dirY[0xB4] = 0;
    m_dirX[0x5A] = 0;
    m_dirX[0x10E] = 0;
    m_dirX[0xB4] = 100000;
    m_dirY[0x5A] = -100000;
    m_dirY[0x10E] = 100000;
}

// swd32.exe 0x00433D70
void Projector::Release() {
    FreeParticles();
    if (m_screenRows != nullptr) {
        crt::free(m_screenRows);
        m_screenRows = nullptr;
    }
    if (m_bufferRows != nullptr) {
        crt::free(m_bufferRows);
        m_bufferRows = nullptr;
    }
}

// swd32.exe 0x00433DC0: screen tables from the back buffer (pitch / 2 pixels per row)
int Projector::Init(Graphics* graphics) {
    LONG pitch = 0;
    LONG height = 0;
    graphics->GetBackSize(&pitch, &height);
    pitch /= 2;
    SetScreenRows(pitch, height);
    SetClip(0, 0, pitch, height);
    return SetBufferRows(0x500, 0x300);
}

// swd32.exe 0x00433E20
int Projector::SetBufferRows(int width, int height) {
    int32_t* table = rowTable(m_bufferRows, width, height);
    if (table == nullptr) {
        return 0;
    }
    m_bufferWidth = width;
    m_bufferHeight = height;
    int row = 0;
    for (int offset = 0; row < height; offset += width) {
        table[row++] = offset;
    }
    return height > 0 ? row : 0;
}

// swd32.exe 0x00433E90
int Projector::SetScreenRows(int width, int height) {
    int32_t* table = rowTable(m_screenRows, width, height);
    if (table == nullptr) {
        return 0;
    }
    m_screenWidth = width;
    m_screenHeight = height;
    int row = 0;
    for (int offset = 0; row < height; offset += width) {
        table[row++] = offset;
    }
    return height > 0 ? row : 0;
}

// swd32.exe 0x00433F00
void Projector::FreeParticles() {
    if (m_particles != nullptr) {
        crt::free(m_particles);
        m_particles = nullptr;
    }
}

// swd32.exe 0x00433F30
int Projector::SetScreen(int width, int height) {
    m_screenWidth = width;
    m_screenHeight = height;
    SetScreenRows(width, height);
    return SetClip(0, 0, width, height);
}

// swd32.exe 0x004342E0: a rectangle past the right / bottom edge is moved back, not shrunk
int Projector::SetClip(int x, int y, int width, int height) {
    if (x < 0) {
        width = x + width;
        x = 0;
    }
    if (y < 0) {
        height = y + height;
        y = 0;
    }
    if (x + width >= m_screenWidth) {
        x = m_screenWidth - width;
    }
    if (y + height >= m_screenHeight) {
        y = m_screenHeight - height;
    }
    m_clipTop = y;
    m_clipLeft = x;
    m_clipRight = width + x;
    m_clipBottom = height + y;
    return height + y;
}

// swd32.exe 0x00434350: one Bresenham step towards (toX, toY); true once reached
bool LineStep(LineWalker* walker) {
    const int toX = walker->toX;
    int dx = toX - walker->fromX;
    int stepX = 1;
    int dy = walker->toY - walker->fromY;
    int stepY = 1;
    if (dx < 0) {
        stepX = -1;
        dx = walker->fromX - walker->toX;
    }
    if (dy < 0) {
        stepY = -1;
        dy = walker->fromY - walker->toY;
    }
    if (dx == 0) {
        walker->y += stepY;
    } else if (dy == 0) {
        walker->x += stepX;
    } else if (dx >= dy) {
        if (dx == dy) {
            walker->x += stepX;
            walker->y += stepY;
        } else if (dx > dy) {
            walker->errorY += dy;
            if (walker->errorY > dx >> 1) {
                walker->errorY -= dx;
                walker->y += stepY;
            }
            walker->x += stepX;
        }
    } else {
        walker->errorX += dx;
        if (walker->errorX > dy >> 1) {
            walker->errorX -= dy;
            walker->x += stepX;
        }
        walker->y += stepY;
    }
    return walker->x == toX && walker->y == walker->toY;
}

// swd32.exe 0x00434420: one step along the direction of `walker->angle`
DirectionWalker* Projector::Step(DirectionWalker* walker) {
    int stepX = 1;
    int stepY = 1;
    int dx = m_dirX[walker->angle];
    int dy = m_dirY[walker->angle];
    if (dx < 0) {
        stepX = -1;
        dx = -dx;
    }
    if (dy < 0) {
        stepY = -1;
        dy = -dy;
    }
    if (dx == 0) {
        walker->y += stepY;
        return walker;
    }
    if (dy == 0) {
        walker->x += stepX;
        return walker;
    }
    if (dx == dy) {
        walker->x += stepX;
        walker->y += stepY;
        return walker;
    }
    if (dx > dy) {
        walker->errorY += dy;
        if (walker->errorY > dx >> 1) {
            walker->errorY -= dx;
            walker->y += stepY;
        }
        walker->x += stepX;
        return walker;
    }
    walker->errorX += dx;
    if (walker->errorX > dy >> 1) {
        walker->errorX -= dy;
        walker->x += stepX;
    }
    walker->y += stepY;
    return walker;
}

// swd32.exe 0x004344E0: draws `picture` scaled by scaleX / scaleY (1024 = 1) upwards from its bottom row at
// (x, y) into `surface` through the screen rows, skipping both colour keys; mixed with flags 0x16, mirrored with
// flag 1 (which reads one pixel to the right). The direction walkers are stepped but their positions unused.
void Projector::DrawScaled(std::uint16_t* surface, const ProjectedPicture* picture) {
    const std::uint16_t* lastRow = picture->pixels + static_cast<std::size_t>(picture->width) * (picture->height - 1);
    G.drawOption71C = picture->tint[0];
    G.drawOption30C = picture->tint[1];
    G.drawOption304 = picture->tint[2];
    const int width = picture->width;
    const int left = picture->x;
    const int scaleX = picture->scaleX;
    const int scaleY = picture->scaleY;
    const int columns = (width << 10) / scaleX;
    const int rows = (picture->height << 10) / scaleY;
    DirectionWalker outer{};
    DirectionWalker inner{};
    outer.angle = picture->angle;
    inner.angle = (picture->angle + 0x5A) % 0x168;
    outer.y = picture->y;
    outer.x = left;
    if (rows <= 0) {
        return;
    }
    int sourceRow = 0;
    int rowAccumulator = 0;
    int y = picture->y + rows;
    for (int row = rows; row != 0; --row, --y) {
        Step(&outer);
        inner.y = outer.y;
        inner.x = outer.x;
        inner.errorX = 0;
        inner.errorY = 0;
        if (columns > 0) {
            int x = left;
            int column = 0;
            int columnAccumulator = 0;
            for (int count = columns; count != 0; --count, ++x) {
                Step(&inner);
                const std::uint16_t flags = picture->flags;
                if (x >= 0 && x < m_screenWidth && y >= 0 && y < m_screenHeight) {
                    const int index = (flags & 1) != 0 ? width - column - sourceRow : column - sourceRow;
                    const std::uint16_t* source = lastRow + index;
                    const std::uint16_t pixel = *source;
                    if (pixel != static_cast<std::uint32_t>(G.rleFillColor16) &&
                        pixel != static_cast<std::uint32_t>(G.rleSkipColor16)) {
                        std::uint16_t* target = surface + x + m_screenRows[y];
                        if ((flags & 0x16) != 0) {
                            MixPixels(source, target, 1);
                        } else {
                            *target = pixel;
                        }
                    }
                }
                columnAccumulator += scaleX;
                column = columnAccumulator >> 10;
            }
        }
        rowAccumulator += scaleY;
        sourceRow = width * (rowAccumulator >> 10);
    }
}

namespace {

std::uint16_t fillKey() { return static_cast<std::uint16_t>(G.rleFillColor16); }

bool isKey(std::uint16_t pixel) {
    return pixel == static_cast<std::uint16_t>(G.rleFillColor16) || pixel == static_cast<std::uint16_t>(G.rleSkipColor16);
}

}  // namespace

// swd32.exe 0x00434DD0: tries `count` random pixels of the picture (within G.particleRange with flag 0x40, within
// one burst with 0x80) and turns each opaque 2 x 2 block into a particle flying to a random point of the target
// rectangle; the block is cleared. With flag 0x80 alone the start position is left from the previous particle
// (zero for the first). Every new particle gets an empty successor, and the tail is set to that successor's
// next (null), so a later call writes through a null particle.
int ParticlePut(ParticleEffect* effect, int count) {
    if (G.particleRange == 0) {
        G.particleRange = effect->burstStep;
    }
    const int top = effect->y;
    std::uint16_t* pixels = effect->pixels;
    const int left = effect->x;
    Particle* particle;
    if (effect->head == nullptr) {
        particle = static_cast<Particle*>(crt::malloc(sizeof(Particle)));
        if (particle == nullptr) {
            w32::MessageBoxA(nullptr, G.aParticlePut, reinterpret_cast<const char*>(&G.byte_4A60B0), 0);
        }
        std::memset(particle, 0, sizeof(Particle));
        effect->head = particle;
    } else {
        particle = effect->tail;
    }
    int result = count;
    int startX = count;
    int startY = 0;
    for (int attempt = 0; attempt < count; ++attempt) {
        const std::uint16_t flags = effect->flags;
        int index;
        if ((flags & 0x40) != 0) {
            const int factor = crt::rand() % 0x1C;
            index = crt::rand() * factor % G.particleRange;
        } else if ((flags & 0x80) != 0) {
            const int factor = crt::rand() % 0x1C;
            const int product = factor * crt::rand();
            index = product % (effect->pixelCount / effect->bursts);
        } else {
            const int factor = crt::rand() % 0x1C;
            index = crt::rand() * factor % effect->pixelCount;
        }
        std::uint16_t* source = pixels + index;
        if (isKey(*source)) {
            continue;
        }
        ++effect->particles;
        const std::uint32_t width = effect->width;
        if ((effect->flags & 1) != 0) {
            startX = static_cast<int>(width - static_cast<std::uint32_t>(index) % width) + left;
            particle->walker.fromX = startX;
        } else if ((effect->flags & 0x80) == 0) {
            startX = static_cast<int>(static_cast<std::uint32_t>(index) % width) + left;
            particle->walker.fromX = startX;
        }
        if ((effect->flags & 1) != 0 || (effect->flags & 0x80) == 0) {
            startY = static_cast<int>(static_cast<std::uint32_t>(index) / width) + top;
            particle->walker.fromY = startY;
            particle->walker.x = startX;
            particle->walker.y = startY;
        }
        const int targetX = crt::rand() % effect->targetWidth + effect->targetLeft;
        particle->walker.toX = targetX;
        const int targetY = crt::rand() % effect->targetHeight + effect->targetTop;
        particle->walker.toY = targetY;
        const auto dx = static_cast<std::uint32_t>(targetX - startX);
        const auto dy = static_cast<std::uint32_t>(targetY - startY);
        std::uint32_t distance = dx * dx + dy * dy;
        if (distance == 0) {
            distance = 1;
        }
        const auto roll = static_cast<std::uint32_t>(crt::rand());
        const auto span = static_cast<std::uint32_t>(crt::ftol(std::sqrt(static_cast<double>(distance))));
        particle->life = static_cast<int32_t>(roll % span + effect->lifeBase);
        particle->speed = crt::rand() % effect->speed + 1;
        particle->pixels[0] = source[0];
        particle->pixels[1] = source[1];
        particle->pixels[2] = source[effect->width];
        particle->pixels[3] = source[effect->width + 1];
        source[0] = fillKey();
        source[1] = fillKey();
        source[effect->width] = fillKey();
        source[effect->width + 1] = fillKey();
        auto* next = static_cast<Particle*>(crt::malloc(sizeof(Particle)));
        particle->next = next;
        if (next == nullptr) {
            return w32::MessageBoxA(nullptr, G.aParticlePut_0, reinterpret_cast<const char*>(&G.byte_4A60B0), 0);
        }
        std::memset(particle->next, 0, sizeof(Particle));
        particle->next->previous = particle;
        particle->next->next = nullptr;
        particle = particle->next;
        effect->tail = particle->next;
    }
    --effect->bursts;
    G.particleRange += effect->burstStep;
    if (effect->bursts == 0) {
        G.particleRange = 0;
    }
    return result;
}

// swd32.exe 0x00434790: one frame of the explosion: the first call seeds rand with the time and counts the opaque
// pixels; while bursts are left particles are emitted and the rest of the picture (but its last row) is drawn;
// then every particle moves (faster below slowLimit live particles), is drawn as a 2 x 2 block and is removed
// once its life ends or it reached the target rectangle. Returns 1 when everything is gone. The mirrored picture
// is clipped at x + width - column but drawn at x + column, and a mixed particle's top right pixel is written
// again unmixed.
int Projector::DrawExplosion(std::uint16_t* surface, ParticleEffect* effect) {
    std::uint16_t* row = effect->pixels;
    const int x = effect->x;
    int y = effect->y;
    if (effect->started == 0) {
        crt::srand(static_cast<std::uint32_t>(crt::time(nullptr)));
        G.drawOption71C = effect->tint[0];
        G.drawOption30C = effect->tint[1];
        G.drawOption304 = effect->tint[2];
        const int pixelCount = effect->height * effect->width;
        effect->pixelCount = pixelCount;
        effect->particles = 0;
        effect->started = 1;
        effect->opaque = 0;
        for (int i = 0; i < pixelCount; ++i) {
            if (!isKey(row[i])) {
                ++effect->opaque;
            }
        }
        effect->slowLimit = effect->opaque / 20;
        effect->burstStep = pixelCount / effect->bursts;
    }
    if (effect->bursts > 0 && effect->particles < effect->opaque / effect->divisor) {
        ParticlePut(effect, effect->pixelCount / effect->bursts);
    }
    if (effect->particles <= 0 && effect->bursts == 0) {
        return 1;
    }
    if (effect->bursts >= 1) {
        const bool mirrored = (effect->flags & 1) != 0;
        for (int line = 0; line < effect->height - 1; ++line, ++y) {
            for (int column = 0; column < effect->width; ++column) {
                const int sourceColumn = mirrored ? effect->width - column : column;
                const int clipX = mirrored ? sourceColumn + x : column + x;
                if (clipX < 0 || clipX >= m_screenWidth || y < 0 || y >= m_screenHeight) {
                    continue;
                }
                const std::uint16_t* source = row + sourceColumn;
                if (isKey(*source)) {
                    continue;
                }
                std::uint16_t* target = surface + m_screenRows[y] + column + x;
                if ((effect->flags & 0x16) != 0) {
                    MixPixels(source, target, 1);
                } else {
                    *target = *source;
                }
            }
            row += effect->width;
        }
    }
    Particle* particle = effect->head;
    while (particle != nullptr) {
        if (effect->particles <= effect->slowLimit) {
            const int speed = effect->speed;
            particle->speed = crt::rand() % speed + speed;
        }
        for (int step = 0; step < particle->speed; ++step) {
            LineStep(&particle->walker);
        }
        particle->life -= particle->speed;
        const int px = particle->walker.x;
        const int py = particle->walker.y;
        bool remove = particle->life <= 0;
        if (!remove && px >= effect->targetLeft && py >= effect->targetTop &&
            px < effect->targetWidth + effect->targetLeft && py < effect->targetHeight + effect->targetTop) {
            remove = true;
        }
        if (!remove) {
            const bool mixed = (effect->flags & 0x16) != 0;
            if (px + 1 >= 0 && px + 1 < m_screenWidth && py >= 0 && py < m_screenHeight) {
                std::uint16_t* line = surface + m_screenRows[py] + px;
                if (mixed) {
                    if (particle->pixels[0] != fillKey()) {
                        MixPixels(&particle->pixels[0], line, 1);
                    }
                    if (particle->pixels[1] != fillKey()) {
                        MixPixels(&particle->pixels[1], line + 1, 1);
                    }
                    line[1] = particle->pixels[1];
                } else {
                    if (particle->pixels[0] != fillKey()) {
                        line[0] = particle->pixels[0];
                    }
                    if (particle->pixels[1] != fillKey()) {
                        line[1] = particle->pixels[1];
                    }
                }
            }
            if (px + 1 >= 0 && px + 1 < m_screenWidth && py + 1 >= 0 && py + 1 < m_screenHeight) {
                std::uint16_t* line = surface + m_screenRows[py + 1] + px;
                if (mixed) {
                    if (particle->pixels[2] != fillKey()) {
                        MixPixels(&particle->pixels[2], line, 1);
                    }
                    if (particle->pixels[3] != fillKey()) {
                        MixPixels(&particle->pixels[3], line + 1, 1);
                    }
                } else {
                    if (particle->pixels[2] != fillKey()) {
                        line[0] = particle->pixels[2];
                    }
                    if (particle->pixels[3] != fillKey()) {
                        line[1] = particle->pixels[3];
                    }
                }
            }
            particle = particle->next;
            continue;
        }
        Particle* previous = particle->previous;
        Particle* next = particle->next;
        if (previous == nullptr) {
            if (next == nullptr) {
                effect->head = nullptr;
                effect->tail = nullptr;
                effect->particles = 0;
                crt::free(particle);
                return 0;
            }
            next->previous = nullptr;
            effect->head = particle->next;
        } else if (next == nullptr) {
            previous->next = nullptr;
            effect->tail = particle->previous;
        } else {
            previous->next = next;
            particle->next->previous = particle->previous;
        }
        Particle* removed = particle;
        particle = particle->next;
        crt::free(removed);
        --effect->particles;
        --effect->opaque;
    }
    return 0;
}

}  // namespace swd3
