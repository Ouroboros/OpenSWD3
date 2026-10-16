#pragma once

#include <array>
#include <memory>

#include "core/types.h"

namespace swd3
{

class GameState;
class Painter;
class TextFont;
struct TextColors;

// A line shown in the top right corner for a number of frames (0x88 bytes in the original).
struct Notice
{
    i32 frames = 0;                    // +0x00
    std::array<char, 0x80> text = {};  // +0x04
    std::unique_ptr<Notice> next;      // +0x84
};

// The corner notices (the list headed at 0x004B8970; the head is a node of its own and never shown).
class Notices
{
public:
    static constexpr int HiddenFlag = 0xE;

    void Draw(TextFont& font, Painter& painter, const GameState& state, const TextColors& colors);

    Notice head;
};

}  // namespace swd3
