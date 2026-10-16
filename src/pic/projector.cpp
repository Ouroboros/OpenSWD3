#include "pic/projector.h"

#include <cmath>
#include <cstring>

#include "crt/crt.h"
#include "data/globals.h"
#include "draw/pixel_ops.h"
#include "gfx/graphics.h"

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

}  // namespace swd3
