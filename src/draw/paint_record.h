#pragma once

#include <array>
#include <cstddef>
#include <span>
#include <string_view>
#include <vector>

#include "core/types.h"
#include "draw/painter.h"
#include "font/text_font.h"

namespace swd3
{

// The rebuild's smooth battle (deviations.md): the draws of a game frame to one target, each with a copy of what it
// reads and how far it moves by the next game frame (as the code drawing it reckons it under a Motion), so that the
// frame can be drawn again between game frames with every draw that far along its way. A draw that cannot be kept (an
// alpha map picture, a palette picture) leaves the record incomplete; what is written to the target past the painter,
// the fonts and FillBox is found by Matches.
class PaintRecord
{
public:
    // The pixel operations of the painter on part of the target.
    enum class PixelOp : u8
    {
        Tint,
        ShiftRed,
        ShiftGreen,
        ShiftBlue,
        Gray,
        BlendPairs,
    };

    // A draw of the caller's own drawn again: redraw(painter, values) with the painter's state as it was, values[0] and
    // values[1] being its x and y (moved with its motion).
    static constexpr std::size_t CallValues = 8;
    using Redraw = void (*)(Painter& painter, std::span<const int> values);

    // The draws made while it lives move by `motion` by the next game frame (no record recording: nothing); the other
    // draws stay where they are.
    class Motion
    {
    public:
        // `clipped`: the clip rectangle of the draws is theirs and moves with them.
        explicit Motion(std::array<int, 2> motion, bool clipped = false);
        ~Motion();
        Motion(const Motion&) = delete;
        Motion& operator=(const Motion&) = delete;

    private:
        PaintRecord* record;
        std::array<int, 2> kept{};
        bool keptClipped = false;
    };

    // Recording starts on the `pixels` of `target`, which are kept as the base the draws are drawn again over.
    void Start(u16* target, std::size_t pixels);
    void Stop();
    bool Complete() const;
    std::size_t Draws() const;
    // The draws that move by the next game frame.
    std::size_t Moving() const;

    // The record taking the draws, if one is recording.
    static PaintRecord* Active();

    // No draw recorded while it lives (the draws a recorded draw makes itself).
    class Pause
    {
    public:
        Pause();
        ~Pause();
        Pause(const Pause&) = delete;
        Pause& operator=(const Pause&) = delete;

    private:
        PaintRecord* paused;
    };

    // The hooks of the painter and the fonts, called before the draw.
    void Picture(const Painter& painter, int x, int y, int width, int height, u32 mode, const DrawArgument& argument);
    void Zoomed(const Painter& painter, int x, int y, int width, int height, bool mirrored);
    void Pixels(const u16* pixels, std::size_t count, PixelOp op, int red, int green, int blue, u32 colorPair = 0);
    void Mix(std::span<const u16> other, std::span<const u16> pixels);
    void Text(TextFont& font, const u16* target, int x, int y, std::string_view text, u16 color, int style);
    void Fill(TextFont& font, const u16* target, int x, int y, int width, int height, u16 color);
    void Call(const Painter& painter, Redraw redraw, std::span<const int> values);

    // The draws drawn again over the base into `out` (the pixels of the target, its pitch), each `share` (0..1) of its
    // motion along; the painter's and the fonts' state is left as it was.
    void Replay(Painter& painter, std::span<u16> out, int pitch, double share) const;
    // The draws drawn again over what `out` holds (another base), each `share` of its motion along.
    void DrawOver(Painter& painter, std::span<u16> out, int pitch, double share) const;
    // Whether drawing again unmoved gives `drawn` (every write to the target was kept); `scratch` takes the replay.
    bool Matches(Painter& painter, std::span<const u16> drawn, std::vector<u16>& scratch, int pitch) const;

private:
    enum class Kind : u8
    {
        Picture,
        Zoomed,
        ZoomedMirrored,
        Pixels,
        Mix,
        Text,
        Fill,
        Call,
    };

    struct Command
    {
        Kind kind = Kind::Picture;
        PixelOp op = PixelOp::Tint;
        int x = 0;
        int y = 0;
        int width = 0;
        int height = 0;
        u32 mode = 0;
        u32 colorPair = 0;
        int red = 0;
        int green = 0;
        int blue = 0;
        Painter::DrawState state;
        std::size_t dataAt = 0;  // the copy of the source, the text or the mixed pixels
        std::size_t dataSize = 0;
        std::size_t pixelsAt = 0;  // the pixels of the target operated on
        std::size_t pixelCount = 0;
        TextFont* font = nullptr;
        TextFont::DrawState fontState;
        Redraw redraw = nullptr;
        std::array<int, CallValues> values{};
        std::size_t valueCount = 0;
        std::array<int, 2> motion{};
        bool clipped = false;
    };

    bool OnTarget(const u16* pixels, std::size_t count) const;
    std::size_t Keep(const void* data, std::size_t size);
    void Push(Command command);
    bool KeepSource(Command& command, const Painter& painter, bool rle);

    u16* target = nullptr;
    std::size_t targetPixels = 0;
    std::vector<u16> base;
    std::vector<Command> commands;
    std::vector<u8> copies;
    std::array<int, 2> motion{};
    bool clipped = false;
    bool complete = true;
    bool recording = false;
};

}  // namespace swd3
