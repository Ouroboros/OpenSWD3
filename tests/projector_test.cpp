#include <cstring>
#include <memory>
#include <new>
#include <vector>

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
