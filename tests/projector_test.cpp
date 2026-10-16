#include <cstring>
#include <memory>
#include <new>
#include <vector>

#include "crt/crt.h"
#include "data/globals.h"
#include "pic/pic_ops.h"
#include "pic/projector.h"
#include "test.h"

using namespace swd3;

TEST_CASE("pic.projector direction tables, rows and clip") {
    std::unique_ptr<std::uint8_t[]> storage{new std::uint8_t[sizeof(Projector)]()};
    auto* projector = new (storage.get()) Projector();
    CHECK_EQ(projector->m_dirX[0], -1000);
    CHECK_EQ(projector->m_dirY[0], 0);
    CHECK_EQ(projector->m_dirY[45], -999);
    CHECK_EQ(projector->m_dirY[54], -1374);
    CHECK_EQ(projector->m_dirX[0x5A], 0);
    CHECK_EQ(projector->m_dirY[0x5A], -100000);
    CHECK_EQ(projector->m_dirY[0x5B], projector->m_dirY[0x58]);
    CHECK_EQ(projector->m_dirX[0xB4], 100000);
    CHECK_EQ(projector->m_dirY[0xB5], -projector->m_dirY[1]);
    CHECK_EQ(projector->m_dirX[0x167], -1000);
    CHECK_EQ(projector->m_screenRows[3], 3 * 0x280);
    CHECK_EQ(projector->m_bufferRows[2], 2 * 0x500);
    CHECK_EQ(projector->SetClip(-10, 470, 100, 20), 480);
    CHECK_EQ(projector->m_clipLeft, 0);
    CHECK_EQ(projector->m_clipRight, 90);
    CHECK_EQ(projector->m_clipTop, 460);

    DirectionWalker walker{};
    walker.angle = 0;
    projector->Step(&walker);
    CHECK_EQ(walker.x, -1);
    CHECK_EQ(walker.y, 0);
    walker = DirectionWalker{};
    walker.angle = 45;
    projector->Step(&walker);
    projector->Step(&walker);
    CHECK_EQ(walker.x, -2);
    CHECK_EQ(walker.y, -2);

    LineWalker line{0, 0, 3, 1, 0, 0, 0, 0};
    int steps = 1;
    while (!LineStep(&line)) {
        ++steps;
    }
    CHECK_EQ(steps, 3);
    CHECK_EQ(line.x, 3);
    projector->Release();
    CHECK(projector->m_bufferRows == nullptr);
}

TEST_CASE("pic.shift picture rows and pixels") {
    // 3 x 2 raw picture: header, then rows of 2 header words + 3 pixels + 1 word
    std::vector<std::uint16_t> data = {0xFFFF, 3, 2, 0, 0, 0, 1, 2, 3, 9, 0, 0, 4, 5, 6, 9};
    ShiftPicture(data.data(), kShiftLeft, 1);
    CHECK_EQ(data[6], 2);
    CHECK_EQ(data[8], 1);
    CHECK_EQ(data[12], 5);
    ShiftPicture(data.data(), kShiftRight, 1);
    CHECK_EQ(data[6], 1);
    CHECK_EQ(data[14], 6);
    ShiftPicture(data.data(), kShiftUp, 1);
    CHECK_EQ(data[6], 4);
    CHECK_EQ(data[12], 1);
    ShiftPicture(data.data(), kShiftDown, 1);
    CHECK_EQ(data[6], 1);
    CHECK_EQ(data[12], 4);
}

TEST_CASE("pic.hit test literal runs with inclusive bounds") {
    // 4 x 2 RLE picture: row 0 = skip 1, 2 literal pixels; row 1 = 3 literal pixels
    std::vector<std::uint16_t> data = {0xFFFF, 4, 2, 0, 10, 0xC001, 2, 7, 7, 10, 3, 7, 7, 7};
    Picture picture{};
    picture.pixels = data.data();
    picture.width = 4;
    picture.height = 2;
    CHECK_EQ(PictureHitTest(&picture, 10, 20, 10, 20), 0);
    CHECK_EQ(PictureHitTest(&picture, 12, 20, 10, 20), 1);
    CHECK_EQ(PictureHitTest(&picture, 13, 20, 10, 20), 1);
    CHECK_EQ(PictureHitTest(&picture, 9, 20, 10, 20), 0);
    CHECK_EQ(PictureHitTest(&picture, 10, 22, 10, 20), 0);
}

TEST_CASE("pic.explosion emits particles once and finishes") {
    std::unique_ptr<std::uint8_t[]> storage{new std::uint8_t[sizeof(Projector)]()};
    auto* projector = new (storage.get()) Projector();
    const std::int32_t savedFill = G.rleFillColor16;
    const std::int32_t savedSkip = G.rleSkipColor16;
    G.rleFillColor16 = 0xF81F;
    G.rleSkipColor16 = 0xF81F;
    G.particleRange = 0;
    // blocks of the last row reach one row (and a pixel) past the picture, as in the original
    std::vector<std::uint16_t> pixels(8 * 6 + 8 + 2, 0x1234);
    std::vector<std::uint16_t> screen(640 * 480, 0);
    ParticleEffect effect{};
    effect.pixels = pixels.data();
    effect.width = 8;
    effect.height = 6;
    effect.x = 100;
    effect.y = 100;
    effect.targetLeft = 300;
    effect.targetWidth = 10;
    effect.targetTop = 300;
    effect.targetHeight = 10;
    effect.lifeBase = 400;
    effect.speed = 4;
    effect.bursts = 1;
    effect.divisor = 1;
    CHECK_EQ(projector->DrawExplosion(screen.data(), &effect), 0);
    CHECK_EQ(effect.started, 1);
    CHECK_EQ(effect.pixelCount, 48);
    CHECK_EQ(effect.bursts, 0);
    CHECK(effect.particles > 0);
    int frames = 1;
    while (projector->DrawExplosion(screen.data(), &effect) == 0 && frames < 1000) {
        ++frames;
    }
    CHECK(frames < 1000);
    // the empty successor also counts down, so the effect can end with one particle still listed
    CHECK(effect.particles <= 0);
    while (effect.head != nullptr) {
        Particle* next = effect.head->next;
        crt::free(effect.head);
        effect.head = next;
    }
    int drawn = 0;
    for (std::uint16_t pixel : screen) {
        drawn += pixel == 0x1234;
    }
    CHECK(drawn > 0);
    G.rleFillColor16 = savedFill;
    G.rleSkipColor16 = savedSkip;
    G.particleRange = 0;
    projector->Release();
}
