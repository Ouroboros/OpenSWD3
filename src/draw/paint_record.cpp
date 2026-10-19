#include "draw/paint_record.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include "font/text_font.h"
#include "pic/rle.h"
#include "platform/display.h"

namespace swd3
{

namespace
{

constexpr std::size_t kCopyAlign = 8;    // the copies start aligned for the 16- and 32-bit reads of the painter
constexpr std::size_t kCopyPadding = 8;  // and end in zero words, the end of an RLE line list

PaintRecord* activeRecord = nullptr;

u16 Word(const u8* bytes, std::ptrdiff_t index = 0)
{
    u16 value = 0;
    std::memcpy(&value, bytes + index * static_cast<std::ptrdiff_t>(sizeof(u16)), sizeof(value));
    return value;
}

// The bytes of a true colour RLE picture: the header, the lines up to the empty one (at most its height) and that.
std::size_t RleBytes(const u8* source)
{
    const std::size_t lines = Word(source, RleHeader::HeightWord);
    std::size_t at = RleHeader::HeaderBytes;

    for (std::size_t line = 0; line <= lines; ++line)
    {
        const std::size_t length = Word(source + at) & RleHeader::LengthMask;

        if (length == 0)
        {
            break;
        }

        at += length;
    }

    return at + sizeof(u16);
}

}  // namespace

void PaintRecord::Start(u16* pixels, std::size_t count)
{
    this->target = pixels;
    this->targetPixels = count;
    this->base.assign(pixels, pixels + count);
    this->commands.clear();
    this->copies.clear();
    this->motion = {0, 0};
    this->clipped = false;
    this->complete = true;
    this->recording = true;
    activeRecord = this;
}

void PaintRecord::Stop()
{
    this->recording = false;

    if (activeRecord == this)
    {
        activeRecord = nullptr;
    }
}

bool PaintRecord::Complete() const
{
    return this->complete && !this->base.empty();
}

std::size_t PaintRecord::Draws() const
{
    return this->commands.size();
}

std::size_t PaintRecord::Moving() const
{
    return static_cast<std::size_t>(std::ranges::count_if(
        this->commands,
        [](const Command& command)
        {
            return command.motion[0] != 0 || command.motion[1] != 0;
        }
    ));
}

PaintRecord* PaintRecord::Active()
{
    return activeRecord;
}

PaintRecord::Motion::Motion(std::array<int, 2> motion, bool clipped)
    : record(activeRecord)
{
    if (this->record != nullptr)
    {
        this->kept = this->record->motion;
        this->keptClipped = this->record->clipped;
        this->record->motion = motion;
        this->record->clipped = clipped;
    }
}

PaintRecord::Motion::~Motion()
{
    if (this->record != nullptr)
    {
        this->record->motion = this->kept;
        this->record->clipped = this->keptClipped;
    }
}

void PaintRecord::Push(Command command)
{
    command.motion = this->motion;
    command.clipped = this->clipped;
    this->commands.push_back(command);
}

PaintRecord::Pause::Pause()
    : paused(activeRecord)
{
    activeRecord = nullptr;
}

PaintRecord::Pause::~Pause()
{
    activeRecord = this->paused;
}

bool PaintRecord::OnTarget(const u16* pixels, std::size_t count) const
{
    return pixels >= this->target && pixels + count <= this->target + this->targetPixels;
}

std::size_t PaintRecord::Keep(const void* data, std::size_t size)
{
    const std::size_t at = (this->copies.size() + kCopyAlign - 1) / kCopyAlign * kCopyAlign;
    this->copies.resize(at + size + kCopyPadding);
    std::memcpy(this->copies.data() + at, data, size);
    std::fill_n(this->copies.begin() + static_cast<std::ptrdiff_t>(at + size), kCopyPadding, u8{0});
    return at;
}

bool PaintRecord::KeepSource(Command& command, const Painter& painter, bool rle)
{
    const u8* source = command.state.source;
    std::size_t size = 0;

    if (rle)
    {
        if ((Word(source, RleHeader::FormatWord) & RleHeader::TrueColor) == 0)
        {
            return false;
        }

        size = RleBytes(source);
    }
    else
    {
        if (painter.SaveDrawState().palette != nullptr || command.width <= 0 || command.height <= 0)
        {
            return false;
        }

        size = (command.mode & Painter::EffectMask) == Painter::RawGradient
                   ? sizeof(u32)
                   : static_cast<std::size_t>(command.width) * static_cast<std::size_t>(command.height) * sizeof(u16);
    }

    command.dataAt = this->Keep(source, size);
    command.dataSize = size;
    return true;
}

void PaintRecord::Picture(
    const Painter& painter,
    int x,
    int y,
    int width,
    int height,
    u32 mode,
    const DrawArgument& argument
)
{
    Command command{Kind::Picture};
    command.state = painter.SaveDrawState();

    if (!this->recording || command.state.target != this->target || command.state.source == nullptr)
    {
        return;
    }

    const bool rle = Word(command.state.source) == painter.Rle().magic && command.state.palette == nullptr;

    if (rle && (mode & Painter::EffectMask) == Painter::AlphaMap)
    {
        this->complete = false;
        return;
    }

    command.x = x;
    command.y = y;
    command.width = width;
    command.height = height;
    command.mode = mode;
    command.colorPair = argument.colorPair;

    if (!this->KeepSource(command, painter, rle))
    {
        this->complete = false;
        return;
    }

    this->Push(command);
}

void PaintRecord::Zoomed(const Painter& painter, int x, int y, int width, int height, bool mirrored)
{
    Command command{mirrored ? Kind::ZoomedMirrored : Kind::Zoomed};
    command.state = painter.SaveDrawState();

    if (!this->recording || command.state.target != this->target || command.state.source == nullptr)
    {
        return;
    }

    command.x = x;
    command.y = y;
    command.width = width;
    command.height = height;

    if (!this->KeepSource(command, painter, true))
    {
        this->complete = false;
        return;
    }

    this->Push(command);
}

void PaintRecord::Pixels(const u16* pixels, std::size_t count, PixelOp op, int red, int green, int blue, u32 colorPair)
{
    if (!this->recording || !this->OnTarget(pixels, count))
    {
        return;
    }

    Command command{Kind::Pixels, op};
    command.pixelsAt = static_cast<std::size_t>(pixels - this->target);
    command.pixelCount = count;
    command.red = red;
    command.green = green;
    command.blue = blue;
    command.colorPair = colorPair;
    this->Push(command);
}

void PaintRecord::Mix(std::span<const u16> other, std::span<const u16> pixels)
{
    if (!this->recording || !this->OnTarget(pixels.data(), pixels.size()))
    {
        return;
    }

    Command command{Kind::Mix};
    command.pixelsAt = static_cast<std::size_t>(pixels.data() - this->target);
    command.pixelCount = pixels.size();
    command.dataSize = other.size_bytes();
    command.dataAt = this->Keep(other.data(), command.dataSize);
    this->Push(command);
}

void PaintRecord::Text(TextFont& font, const u16* to, int x, int y, std::string_view text, u16 color, int style)
{
    if (!this->recording || to != this->target)
    {
        return;
    }

    Command command{Kind::Text, PixelOp::Tint, x, y};
    command.red = color;
    command.green = style;
    command.font = &font;
    command.fontState = font.SaveDrawState();
    command.dataSize = text.size();
    command.dataAt = this->Keep(text.data(), text.size());
    this->Push(command);
}

void PaintRecord::Fill(TextFont& font, const u16* to, int x, int y, int width, int height, u16 color)
{
    if (!this->recording || to != this->target)
    {
        return;
    }

    Command command{Kind::Fill, PixelOp::Tint, x, y, width, height};
    command.red = color;
    command.font = &font;
    command.fontState = font.SaveDrawState();
    this->Push(command);
}

void PaintRecord::Call(const Painter& painter, Redraw redraw, std::span<const int> values)
{
    Command command{Kind::Call};
    command.state = painter.SaveDrawState();

    if (!this->recording || command.state.target != this->target)
    {
        return;
    }

    if (values.size() > CallValues)
    {
        this->complete = false;
        return;
    }

    command.redraw = redraw;
    command.valueCount = values.size();
    std::copy(values.begin(), values.end(), command.values.begin());
    this->Push(command);
}

void PaintRecord::Replay(Painter& painter, std::span<u16> out, int pitch, double share) const
{
    std::copy_n(this->base.begin(), std::min(this->base.size(), out.size()), out.begin());
    const Painter::DrawState kept = painter.SaveDrawState();

    for (const Command& command : this->commands)
    {
        const std::array<int, 2> shift = {
            static_cast<int>(std::lround(command.motion[0] * share)),
            static_cast<int>(std::lround(command.motion[1] * share)),
        };

        const int x = command.x + shift[0];
        const int y = command.y + shift[1];
        const u8* data = this->copies.data() + command.dataAt;

        switch (command.kind)
        {
            case Kind::Picture:
            case Kind::Zoomed:
            case Kind::ZoomedMirrored:
            {
                Painter::DrawState state = command.state;
                state.target = out.data();
                state.source = data;

                if (command.clipped)
                {
                    const int left = std::max(state.clipX + shift[0], 0);
                    const int top = std::max(state.clipY + shift[1], 0);
                    const int right = std::min(state.clipX + state.clipWidth + shift[0], Display::Width);
                    const int bottom = std::min(state.clipY + state.clipHeight + shift[1], Display::Height);
                    state.clipX = left;
                    state.clipY = top;
                    state.clipWidth = std::max(right - left, 0);
                    state.clipHeight = std::max(bottom - top, 0);

                    if (state.clipWidth == 0 || state.clipHeight == 0)
                    {
                        break;
                    }
                }

                painter.RestoreDrawState(state);

                if (command.kind == Kind::Picture)
                {
                    painter
                        .DrawPicture(x, y, command.width, command.height, command.mode, {nullptr, command.colorPair});
                }
                else if (command.kind == Kind::Zoomed)
                {
                    painter.DrawZoomed(x, y, command.width, command.height);
                }
                else
                {
                    painter.DrawZoomedMirrored(x, y, command.width, command.height);
                }

                break;
            }

            case Kind::Pixels:
            case Kind::Mix:
            {
                const std::ptrdiff_t at = static_cast<std::ptrdiff_t>(command.pixelsAt) + shift[1] * pitch + shift[0];

                if (at < 0 || static_cast<std::size_t>(at) + command.pixelCount > out.size())
                {
                    break;
                }

                const std::span<u16> pixels(out.data() + at, command.pixelCount);

                if (command.kind == Kind::Mix)
                {
                    painter.MixPixels({reinterpret_cast<const u16*>(data), command.dataSize / sizeof(u16)}, pixels);
                    break;
                }

                switch (command.op)
                {
                    case PixelOp::Tint:
                        painter.TintPixels(pixels, command.red, command.green, command.blue);
                        break;

                    case PixelOp::ShiftRed:
                        painter.ShiftRed(pixels, command.red);
                        break;

                    case PixelOp::ShiftGreen:
                        painter.ShiftGreenPairs(pixels, command.red);
                        break;

                    case PixelOp::ShiftBlue:
                        painter.ShiftBluePairs(pixels, command.red);
                        break;

                    case PixelOp::Gray:
                        painter.GrayPixels(pixels);
                        break;

                    case PixelOp::BlendPairs:
                        painter.BlendPairsWithColor(
                            {reinterpret_cast<u32*>(pixels.data()), pixels.size() / 2},
                            command.colorPair
                        );
                        break;
                }

                break;
            }

            case Kind::Text:
            case Kind::Fill:
            {
                TextFont& font = *command.font;
                const TextFont::DrawState fontKept = font.SaveDrawState();
                font.RestoreDrawState(command.fontState);

                if (command.kind == Kind::Text)
                {
                    font.DrawText(
                        out.data(),
                        x,
                        y,
                        std::string_view(reinterpret_cast<const char*>(data), command.dataSize),
                        static_cast<u16>(command.red),
                        command.green
                    );
                }
                else
                {
                    font.FillRect(out.data(), x, y, command.width, command.height, static_cast<u16>(command.red));
                }

                font.RestoreDrawState(fontKept);
                break;
            }

            case Kind::Call:
            {
                Painter::DrawState state = command.state;
                state.target = out.data();
                painter.RestoreDrawState(state);
                std::array<int, CallValues> values = command.values;
                values[0] += shift[0];
                values[1] += shift[1];
                command.redraw(painter, std::span<const int>(values.data(), command.valueCount));
                break;
            }
        }
    }

    painter.RestoreDrawState(kept);
}

bool PaintRecord::Matches(Painter& painter, std::span<const u16> drawn, std::vector<u16>& scratch, int pitch) const
{
    if (!this->Complete() || drawn.size() != this->base.size())
    {
        return false;
    }

    scratch.resize(drawn.size());
    this->Replay(painter, scratch, pitch, 0.0);
    return std::equal(drawn.begin(), drawn.end(), scratch.begin());
}

}  // namespace swd3
