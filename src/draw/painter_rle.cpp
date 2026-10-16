#include <algorithm>
#include <cstring>

#include "core/words.h"
#include "draw/painter.h"
#include "pic/rle.h"
#include "platform/display.h"

namespace swd3
{

namespace
{

// RLE data is a byte stream; its 16-bit words are not necessarily aligned.
u16 Word(const u8* bytes, std::ptrdiff_t index = 0)
{
    u16 value = 0;
    std::memcpy(&value, bytes + 2 * index, 2);
    return value;
}

u32 ReadPair(const u16* pixels)
{
    u32 value = 0;
    std::memcpy(&value, pixels, 4);
    return value;
}

void WritePair(u16* pixels, u32 value)
{
    std::memcpy(pixels, &value, 4);
}

// Whether a pixel pointer is not on a 4-byte boundary (the pair loops of the original align on it).
bool OddAligned(const u16* pixel)
{
    return (reinterpret_cast<std::uintptr_t>(pixel) & 2) != 0;
}

constexpr u16 kAlphaMapMarker = 0xFFF0;
constexpr int kAlphaMapShift = 2;     // an alpha map byte to a blend level
constexpr int kUnkSkippedLevel = 12;  // BlitAlphaMap leaves an odd first pixel at this level unblended
constexpr int kPairLoopMinimum = 4;   // longer runs are written in pixel pairs
constexpr int kPixelsPerPair = 2;
constexpr int kFadeShift = 4;  // BlitFadeDown: the level falls every sixteenth of the height
constexpr int kFadeSteps = 1 << kFadeShift;
constexpr int kGrayShift = 2;       // the grey is the channel sum over four
constexpr int kSmearLeftSteps = 4;  // the smear steps copy from the left, below, above and the right
constexpr int kSmearDownSteps = 10;
constexpr int kSmearUpSteps = 13;
constexpr int kSmearSteps = 16;
constexpr u32 kFractionMask = (1u << Painter::FractionBits) - 1;

// The levels of Blend and AlphaBlend: each bit takes a share of the source instead of the destination.
constexpr int kHalfLevel = 8;
constexpr int kQuarterLevel = 4;
constexpr int kEighthLevel = 2;
constexpr int kSixteenthLevel = 1;
constexpr int kHalfShift = 1;
constexpr int kQuarterShift = 2;
constexpr int kEighthShift = 3;
constexpr int kSixteenthShift = 4;
// The smear effect steps a fixed 640-pixel row up and down, whatever the pitch.
constexpr std::ptrdiff_t kSmearRow = Display::Width;

}  // namespace

u16 Painter::Blend(u16 dest, u16 source, int level) const
{
    if (level <= 0)
    {
        return dest;
    }

    if (level >= Painter::MaxAlpha)
    {
        return source;
    }

    const u32 half = ((level & kHalfLevel) != 0 ? source : dest) >> kHalfShift;
    const u32 quarter = ((level & kQuarterLevel) != 0 ? source : dest) >> kQuarterShift;
    const u32 eighth = ((level & kEighthLevel) != 0 ? source : dest) >> kEighthShift;
    const u32 sixteenth = ((level & kSixteenthLevel) != 0 ? source : dest) >> kSixteenthShift;
    return static_cast<u16>(
        (half & this->halfMaskPairs[kHalfShift - 1]) + (quarter & this->halfMaskPairs[kQuarterShift - 1]) +
        (eighth & this->halfMaskPairs[kEighthShift - 1]) + (sixteenth & this->halfMaskPairs[kSixteenthShift - 1])
    );
}

// The alpha map holds premultiplied sources: the destination terms 1/2, 1/4, 1/8 and 1/16 are added for the bits
// (8, 4, 2, 1) that are clear in the level.
u16 Painter::AlphaBlend(u16 dest, u16 source, int level) const
{
    if (level == 0)
    {
        return dest;
    }

    if (level == Painter::MaxAlpha)
    {
        return source;
    }

    u32 value = source;

    if ((level & kHalfLevel) == 0)
    {
        value += (static_cast<u32>(dest) >> kHalfShift) & this->halfMaskPairs[kHalfShift - 1];
    }

    if ((level & kQuarterLevel) == 0)
    {
        value += (static_cast<u32>(dest) >> kQuarterShift) & this->halfMaskPairs[kQuarterShift - 1];
    }

    if ((level & kEighthLevel) == 0)
    {
        value += (static_cast<u32>(dest) >> kEighthShift) & this->halfMaskPairs[kEighthShift - 1];
    }

    if ((level & kSixteenthLevel) == 0)
    {
        value += (static_cast<u32>(dest) >> kSixteenthShift) & this->halfMaskPairs[kSixteenthShift - 1];
    }

    return static_cast<u16>(value);
}

// Two pixels at once: carries of the low pixel run into the high one, as in the original's 32-bit arithmetic.
u32 Painter::AlphaBlendPair(u32 dest, u32 source, int level) const
{
    if (level == Painter::MaxAlpha)
    {
        return source;
    }

    u32 value = 0;

    if ((level & kHalfLevel) == 0)
    {
        value += (dest >> kHalfShift) & this->halfMaskPairs[kHalfShift - 1];
    }

    if ((level & kQuarterLevel) == 0)
    {
        value += (dest >> kQuarterShift) & this->halfMaskPairs[kQuarterShift - 1];
    }

    value += source;

    if ((level & kEighthLevel) == 0)
    {
        value += (dest >> kEighthShift) & this->halfMaskPairs[kEighthShift - 1];
    }

    if ((level & kSixteenthLevel) == 0)
    {
        value += (dest >> kSixteenthShift) & this->halfMaskPairs[kSixteenthShift - 1];
    }

    return value;
}

// Tint steps: one step is the channel mask shifted right by four; a negative step saturates to 0.
Painter::Tint Painter::MakeTint(int red, int green, int blue) const
{
    Tint tint;
    tint.red = static_cast<u32>(static_cast<i32>(this->red.halves[ColorChannel::TintHalf]) * red);
    tint.green = static_cast<u32>(static_cast<i32>(this->green.halves[ColorChannel::TintHalf]) * green);
    tint.blue = static_cast<u32>(static_cast<i32>(this->blue.halves[ColorChannel::TintHalf]) * blue);
    tint.redSaturated = red >= 0 ? this->red.mask : 0;
    tint.greenSaturated = green >= 0 ? this->green.mask : 0;
    tint.blueSaturated = blue >= 0 ? this->blue.mask : 0;
    return tint;
}

Painter::Tint Painter::CurrentTint() const
{
    return this->MakeTint(this->options.tintRed, this->options.tintGreen, this->options.tintBlue);
}

bool Painter::Tinted() const
{
    return this->options.tintRed != 0 || this->options.tintGreen != 0 || this->options.tintBlue != 0;
}

u16 Painter::TintPixel(const Tint& tint, u16 dest) const
{
    u32 red = (dest & this->red.mask) + tint.red;

    if ((this->red.inverseMask & red) != 0)
    {
        red = tint.redSaturated;
    }

    u32 green = (dest & this->green.mask) + tint.green;

    if ((this->green.inverseMask & green) != 0)
    {
        green = tint.greenSaturated;
    }

    u32 blue = (dest & this->blue.mask) + tint.blue;

    if ((this->blue.inverseMask & blue) != 0)
    {
        blue = tint.blueSaturated;
    }

    return static_cast<u16>(red | green | blue);
}

// Per channel: the source plus its tint step (saturated), then plus the destination (saturated to the mask).
u16 Painter::AddTinted(const Tint& tint, u16 source, u16 dest) const
{
    auto channel = [source, dest](const ColorChannel& color, u32 step, u32 saturated)
    {
        u32 value = (source & color.mask) + step;

        if ((color.inverseMask & value) != 0)
        {
            value = saturated;
        }

        value += dest & color.mask;

        if ((color.inverseMask & value) != 0)
        {
            value = color.mask;
        }

        return value;
    };

    return static_cast<u16>(
        channel(this->red, tint.red, tint.redSaturated) | channel(this->green, tint.green, tint.greenSaturated) |
        channel(this->blue, tint.blue, tint.blueSaturated)
    );
}

u16 Painter::Add(u16 source, u16 dest) const
{
    auto channel = [source, dest](const ColorChannel& color)
    {
        u32 value = (source & color.mask) + (dest & color.mask);

        if ((color.inverseMask & value) != 0)
        {
            value = color.mask;
        }

        return value;
    };

    return static_cast<u16>(channel(this->red) | channel(this->green) | channel(this->blue));
}

// dest - (source + tint) per channel, clamped at 0.
u16 Painter::SubtractTintedSource(const Tint& tint, u16 source, u16 dest) const
{
    auto channel = [source, dest](const ColorChannel& color, u32 step, u32 saturated)
    {
        u32 subtracted = (source & color.mask) + step;

        if ((color.inverseMask & subtracted) != 0)
        {
            subtracted = saturated;
        }

        u32 value = (dest & color.mask) - subtracted;

        if ((value & 0x80000000u) != 0)
        {
            value = 0;
        }

        return value & color.mask;
    };

    return static_cast<u16>(
        channel(this->red, tint.red, tint.redSaturated) | channel(this->green, tint.green, tint.greenSaturated) |
        channel(this->blue, tint.blue, tint.blueSaturated)
    );
}

// (dest + tint) - source per channel, clamped at 0 (the mirrored subtractive effect adds the tint to the other side).
u16 Painter::SubtractFromTintedDest(const Tint& tint, u16 source, u16 dest) const
{
    auto channel = [source, dest](const ColorChannel& color, u32 step, u32 saturated)
    {
        u32 value = (dest & color.mask) + step;

        if ((color.inverseMask & value) != 0)
        {
            value = saturated;
        }

        value -= source & color.mask;

        if ((value & 0x80000000u) != 0)
        {
            value = 0;
        }

        return value & color.mask;
    };

    return static_cast<u16>(
        channel(this->red, tint.red, tint.redSaturated) | channel(this->green, tint.green, tint.greenSaturated) |
        channel(this->blue, tint.blue, tint.blueSaturated)
    );
}

// (r + g + b) / 4 in every channel.
u16 Painter::Gray(u16 dest) const
{
    const u32 red = (dest & this->red.mask) >> this->red.shift;
    const u32 green = (dest & this->green.mask) >> this->green.shift;
    const u32 blue = (dest & this->blue.mask) >> this->blue.shift;
    const u32 average = (red + green + blue) >> kGrayShift;
    return static_cast<u16>(
        (average << this->red.shift) + (average << this->green.shift) + (average << this->blue.shift)
    );
}

// The smear counter cycles 1..16 and copies the left neighbour, the pixel one row below, one row above or the right
// neighbour; the 17th step resets the counter without writing.
void Painter::Smear(u16* at)
{
    const int step = this->smearCounter + 1;

    if (step <= kSmearLeftSteps)
    {
        this->smearCounter = step;
        *at = at[-1];
    }
    else if (step <= kSmearDownSteps)
    {
        this->smearCounter = step;
        *at = at[kSmearRow];
    }
    else if (step <= kSmearUpSteps)
    {
        this->smearCounter = step;
        *at = at[-kSmearRow];
    }
    else if (step <= kSmearSteps)
    {
        this->smearCounter = step;
        *at = at[1];
    }
    else
    {
        this->smearCounter = 0;
    }
}

// The wave rows start every `stride` bytes of the offset table (0x84, one row of 33 offsets; 0x528 in some effects,
// which leaves the table from mode 3 on: offsets beyond the table count as 0 here).
void Painter::WaveBegin(int stride)
{
    if (this->waveMode != 0)
    {
        this->waveStart = (this->waveMode - 1) * stride;
        this->wavePosition = this->waveStart + this->wavePhase;
        this->waveEnd = this->waveStart + Painter::WaveRowBytes;
    }
}

u16* Painter::WaveLine(u16* dest)
{
    if (this->waveMode == 0)
    {
        return dest;
    }

    int position = this->wavePosition + Painter::WaveEntryBytes;

    if (static_cast<u32>(position) >= static_cast<u32>(this->waveEnd))
    {
        position = this->waveStart;
    }

    this->wavePosition = position;
    const std::size_t index = static_cast<u32>(position) / Painter::WaveEntryBytes;
    return dest + (index < Painter::WaveTable.size() ? Painter::WaveTable[index] : 0);
}

void Painter::WaveAdvance()
{
    if (this->waveMode != 0)
    {
        this->wavePhase += Painter::WaveEntryBytes;

        if (static_cast<u32>(this->wavePhase) >= static_cast<u32>(Painter::WaveRowBytes))
        {
            this->wavePhase = 0;
        }
    }
}

// One output line of vertical scaling: the number of further source lines to move on.
u32 Painter::ScaleStep(u32& fraction) const
{
    const u32 sum = fraction + this->placement.scaleFraction;
    fraction = sum & kFractionMask;
    return sum >> Painter::FractionBits;
}

// Skipping the clipped top lines of a scaled picture: each round passes one line plus the scaled extra lines, and the
// round counter goes up twice per round, so only about half of the requested rounds are made. nullptr when the end
// of the picture is reached.
const u8* Painter::ScaledSkipTop(const u8* lines, u32& fraction) const
{
    int counter = 0;

    while (counter != this->placement.skipTop)
    {
        ++counter;
        lines += Word(lines) & RleHeader::LengthMask;
        u32 step = this->ScaleStep(fraction);

        if (!this->placement.scaleUp)
        {
            while (step-- != 0)
            {
                const u16 header = Word(lines);

                if (header == 0)
                {
                    return nullptr;
                }

                lines += header & RleHeader::LengthMask;
            }
        }
        else if (step != 0)
        {
            const u16 header = Word(lines);

            if (header == 0)
            {
                return nullptr;
            }

            lines += header & RleHeader::LengthMask;
        }

        if (counter == this->placement.skipTop)
        {
            break;
        }

        ++counter;
    }

    return lines;
}

// The source line of the next output line of a scaled picture; nullptr ends the picture.
const u8* Painter::ScaledNextLine(const u8* line, u32& fraction) const
{
    u32 step = this->ScaleStep(fraction);
    const u8* next = line + (Word(line) & RleHeader::LengthMask);

    if (!this->placement.scaleUp)
    {
        while (step-- != 0)
        {
            const u16 header = Word(next);

            if (header == 0)
            {
                return nullptr;
            }

            next += header & RleHeader::LengthMask;
        }

        return next;
    }

    if (step == 0)
    {
        return line;
    }

    return Word(next) == 0 ? nullptr : next;
}

// The runs of one RLE line clipped to the columns [start, start + width) of the picture: op(kind, pixels, count,
// offset) receives the visible part, its literal pixels and its distance from the first drawn column. A zero count
// ends the line, as does reaching the right edge.
template <typename RunOp>
void Painter::ForEachRun(const u8* codes, int start, int width, RunOp&& op) const
{
    const int end = start + width;
    int x = 0;

    while (true)
    {
        const u16 code = Word(codes);
        const int count = code & RleHeader::CountMask;
        codes += sizeof(u16);

        if (count == 0)
        {
            return;
        }

        RunKind kind = RunKind::Literal;

        if ((code & RleHeader::FillRun) != 0)
        {
            kind = RunKind::Fill;
        }
        else if ((code & RleHeader::SkipRun) != 0)
        {
            kind = RunKind::Skip;
        }

        const u8* pixels = codes;

        if (kind == RunKind::Literal)
        {
            codes += sizeof(u16) * count;
        }

        const int first = x > start ? x : start;
        const int last = x + count < end ? x + count : end;

        if (last > first)
        {
            op(kind, pixels + sizeof(u16) * (first - x), last - first, first - start);
        }

        x += count;

        if (x >= end)
        {
            return;
        }
    }
}

// The lines of the current picture: skips the clipped top lines, then calls op(codes, dest, line) for each visible
// line with the destination of its first drawn column (one past the last for mirrored walks) after the wave offset.
// lineHook runs for every skipped top line and before every line read (also the closing one).
template <typename LineOp, typename SkipHook>
void Painter::ForEachLine(const LineWalk& walk, LineOp&& op, SkipHook&& lineHook)
{
    this->WaveBegin(walk.waveStride);
    const u8* lines = this->source;

    if ((Word(lines, RleHeader::FormatWord) & RleHeader::TrueColor) != 0)
    {
        lines += RleHeader::HeaderBytes;
        u32 fraction = 0;

        if (walk.scaled)
        {
            lines = this->ScaledSkipTop(lines, fraction);
        }
        else
        {
            for (int skipped = 0; skipped != this->placement.skipTop; ++skipped)
            {
                lines += Word(lines) & RleHeader::LengthMask;
                lineHook();
            }
        }

        u16* dest = this->placement.dest + (walk.mirrored ? this->placement.width : 0);
        const int pitch = walk.signedPitch ? this->placement.pitch : this->screenPitch;
        int line = 0;

        while (lines != nullptr)
        {
            const u16 header = Word(lines);
            u16* lineDest = dest;
            u16* drawn = this->WaveLine(dest);
            lineHook();

            if (header == 0)
            {
                break;
            }

            ++line;

            if (line > this->placement.rows)
            {
                break;
            }

            bool dropped = false;

            if (walk.dropLines && this->options.dropLines)
            {
                if (++this->placement.ditherPhase >= Painter::DitherPhases)
                {
                    this->placement.ditherPhase = 0;
                    dropped = true;
                }
            }

            if (!dropped)
            {
                op(lines + sizeof(u16), drawn, line);
            }

            dest = lineDest + pitch;
            lines = walk.scaled ? this->ScaledNextLine(lines, fraction) : lines + (header & RleHeader::LengthMask);
        }
    }

    if (walk.advanceWave)
    {
        this->WaveAdvance();
    }
}

// Every visible run of every line: op(kind, pixels, count, at) with `at` the destination of the run's first pixel
// (one past it, to the right, for mirrored walks).
template <typename RunOp>
void Painter::WalkRuns(const LineWalk& walk, RunOp&& op)
{
    const int width = this->placement.width;
    const int start = walk.mirrored ? this->mirrorStart : this->placement.skipLeft;
    this->ForEachLine(
        walk,
        [this, &walk, &op, start, width](const u8* codes, u16* lineDest, int)
        {
            this->ForEachRun(
                codes,
                start,
                width,
                [&walk, &op, lineDest](RunKind kind, const u8* pixels, int count, int offset)
                {
                    op(kind, pixels, count, walk.mirrored ? lineDest - offset : lineDest + offset);
                }
            );
        },

        []
        {
        }
    );
}

// swd32.exe 0x00418350: literal pixels copied; both other run kinds are transparent
int Painter::BlitCopy(int, int, int, int, const DrawArgument&)
{
    const LineWalk walk{.signedPitch = true};
    this->WalkRuns(
        walk,
        [](RunKind kind, const u8* pixels, int count, u16* at)
        {
            if (kind == RunKind::Literal)
            {
                std::memcpy(at, pixels, sizeof(u16) * static_cast<std::size_t>(count));
            }
        }
    );
    return 0;
}

// swd32.exe 0x004185C0: mirrored copy; the wave phase is not advanced
int Painter::BlitCopyMirrored(int, int, int width, int, const DrawArgument&)
{
    this->mirrorStart = width - (this->placement.skipLeft + this->placement.width);
    const LineWalk walk{.mirrored = true, .signedPitch = true, .advanceWave = false};
    this->WalkRuns(
        walk,
        [](RunKind kind, const u8* pixels, int count, u16* at)
        {
            if (kind == RunKind::Literal)
            {
                for (int i = 0; i < count; ++i)
                {
                    at[-1 - i] = Word(pixels, i);
                }
            }
        }
    );
    return 0;
}

// swd32.exe 0x00418840: saturating add of the literal pixels, with the tint options when set; DrawOptions::dropLines
// skips every third line. Lines advance by the screen pitch (a flip is ignored).
int Painter::BlitAdditive(int, int, int, int, const DrawArgument&)
{
    const LineWalk walk{.dropLines = true};
    const bool tinted = this->Tinted();
    const Tint tint = this->CurrentTint();
    this->WalkRuns(
        walk,
        [this, tinted, &tint](RunKind kind, const u8* pixels, int count, u16* at)
        {
            if (kind != RunKind::Literal)
            {
                return;
            }

            for (int i = 0; i < count; ++i)
            {
                const u16 source = Word(pixels, i);
                at[i] = tinted ? this->AddTinted(tint, source, at[i]) : this->Add(source, at[i]);
            }
        }
    );
    return 0;
}

// swd32.exe 0x00418EB0: mirrored 0x00418840. The original's untinted loop jumps into the tinted one for a run clipped
// on both sides with uninitialised tint steps; with zero steps that loop gives the plain additive result used here.
int Painter::BlitAdditiveMirrored(int, int, int width, int, const DrawArgument&)
{
    this->mirrorStart = width - (this->placement.skipLeft + this->placement.width);
    const LineWalk walk{.mirrored = true, .dropLines = true};
    const bool tinted = this->Tinted();
    const Tint tint = this->CurrentTint();
    this->WalkRuns(
        walk,
        [this, tinted, &tint](RunKind kind, const u8* pixels, int count, u16* at)
        {
            if (kind != RunKind::Literal)
            {
                return;
            }

            for (int i = 0; i < count; ++i)
            {
                u16& dest = at[-1 - i];
                const u16 source = Word(pixels, i);
                dest = tinted ? this->AddTinted(tint, source, dest) : this->Add(source, dest);
            }
        }
    );
    return 0;
}

// swd32.exe 0x00419570: literal pixels blended with the per-pixel level of the alpha map (rows of `width` bytes,
// levels as multiples of 4); 0x4000 runs are transparent, 0x8000 runs are filled with 0xFFF0. The original turns the
// level byte into a jump-table offset: values from 0x40 on (and non-multiples of 4) jump through unrelated code, here
// the level is clamped to 15 and rounded down. The alignment pixel before the pair loop of runs longer than 4 is not
// drawn at level 12 (a no-op entry of that jump table).
int Painter::BlitAlphaMap(int, int, int width, int, const DrawArgument& argument)
{
    const LineWalk walk{};
    const int start = this->placement.skipLeft;
    const int columns = this->placement.width;
    const int skipTop = this->placement.skipTop;
    this->ForEachLine(
        walk,
        [this, &argument, width, start, columns, skipTop](const u8* codes, u16* lineDest, int line)
        {
            const u8* levels = argument.alphaMap + static_cast<std::ptrdiff_t>(skipTop + line - 1) * width;
            this->ForEachRun(
                codes,
                start,
                columns,
                [this, levels, lineDest, start](RunKind kind, const u8* pixels, int count, int offset)
                {
                    u16* at = lineDest + offset;
                    const u8* level = levels + start + offset;

                    if (kind == RunKind::Skip)
                    {
                        std::fill_n(at, count, kAlphaMapMarker);
                        return;
                    }

                    if (kind != RunKind::Literal)
                    {
                        return;
                    }

                    int i = 0;

                    if (count > kPairLoopMinimum && OddAligned(at))
                    {
                        const int first = std::min(level[0] >> kAlphaMapShift, Painter::MaxAlpha);

                        if (first != kUnkSkippedLevel)
                        {
                            at[0] = this->AlphaBlend(at[0], Word(pixels, 0), first);
                        }

                        i = 1;
                    }

                    for (; i < count; ++i)
                    {
                        const int value = std::min(level[i] >> kAlphaMapShift, Painter::MaxAlpha);
                        at[i] = this->AlphaBlend(at[i], Word(pixels, i), value);
                    }
                }
            );
        },

        []
        {
        }
    );
    return 0;
}

// swd32.exe 0x0041A3B0: mirrored 0x00419570. Literal runs longer than 4 pixels write pairs: each step blends the
// pixel and its right neighbour with the swapped source pair (the neighbour gets blended again). 0x8000 runs map the
// pixels through a lookup table one pixel to the right of the run; the game never sets that table (the original
// reads through a null pointer), so those runs leave the destination unchanged here.
int Painter::BlitAlphaMapMirrored(int, int, int width, int, const DrawArgument& argument)
{
    this->mirrorStart = width - (this->placement.skipLeft + this->placement.width);
    const LineWalk walk{.mirrored = true};
    const int start = this->mirrorStart;
    const int columns = this->placement.width;
    const int skipTop = this->placement.skipTop;
    this->ForEachLine(
        walk,
        [this, &argument, width, start, columns, skipTop](const u8* codes, u16* lineDest, int line)
        {
            const u8* levels = argument.alphaMap + static_cast<std::ptrdiff_t>(skipTop + line - 1) * width;
            this->ForEachRun(
                codes,
                start,
                columns,
                [this, levels, lineDest, start](RunKind kind, const u8* pixels, int count, int offset)
                {
                    if (kind != RunKind::Literal)
                    {
                        return;
                    }

                    u16* at = lineDest - offset;
                    const u8* level = levels + start + offset;

                    auto clamp = [](u8 value)
                    {
                        return std::min(value >> kAlphaMapShift, Painter::MaxAlpha);
                    };

                    if (count <= kPairLoopMinimum)
                    {
                        for (int i = 0; i < count; ++i)
                        {
                            u16& dest = at[-1 - i];
                            dest = this->AlphaBlend(dest, Word(pixels, i), clamp(level[i]));
                        }

                        return;
                    }

                    int i = 0;

                    if (OddAligned(at))
                    {
                        at[-1] = this->AlphaBlend(at[-1], Word(pixels, 0), clamp(level[0]));
                        i = 1;
                    }

                    for (; i < count; ++i)
                    {
                        const int value = clamp(level[i]);

                        if (value == 0)
                        {
                            continue;
                        }

                        u16* pair = at - 1 - i;
                        const u32 sourcePair = MakeWords(Word(pixels, i), Word(pixels, i + 1));
                        WritePair(pair, this->AlphaBlendPair(ReadPair(pair), sourcePair, value));
                    }
                }
            );
        },

        []
        {
        }
    );
    return 0;
}

// swd32.exe 0x0041B280: the destination under the literal pixels tinted (the source pixels are not used); nothing
// is drawn without a tint. The wave rows of this effect are 0x528 bytes apart.
int Painter::BlitTint(int, int, int, int, const DrawArgument&)
{
    if (!this->Tinted())
    {
        return 0;
    }

    const Tint tint = this->CurrentTint();
    const LineWalk walk{.waveStride = Painter::WideWaveStride};
    this->WalkRuns(
        walk,
        [this, &tint](RunKind kind, const u8*, int count, u16* at)
        {
            if (kind == RunKind::Literal)
            {
                for (int i = 0; i < count; ++i)
                {
                    at[i] = this->TintPixel(tint, at[i]);
                }
            }
        }
    );
    return 0;
}

// swd32.exe 0x0041B620: mirrored 0x0041B280 (wave rows 0x84 apart)
int Painter::BlitTintMirrored(int, int, int width, int, const DrawArgument&)
{
    if (!this->Tinted())
    {
        return 0;
    }

    this->mirrorStart = width - (this->placement.skipLeft + this->placement.width);
    const Tint tint = this->CurrentTint();
    const LineWalk walk{.mirrored = true};
    this->WalkRuns(
        walk,
        [this, &tint](RunKind kind, const u8*, int count, u16* at)
        {
            if (kind == RunKind::Literal)
            {
                for (int i = 0; i < count; ++i)
                {
                    at[-1 - i] = this->TintPixel(tint, at[-1 - i]);
                }
            }
        }
    );
    return 0;
}

// swd32.exe 0x0041D340: literal pixels blended at DrawOptions::alpha
int Painter::BlitTranslucent(int, int, int, int, const DrawArgument&)
{
    const int level = this->options.alpha;
    const LineWalk walk{.signedPitch = true};
    this->WalkRuns(
        walk,
        [this, level](RunKind kind, const u8* pixels, int count, u16* at)
        {
            if (kind == RunKind::Literal)
            {
                for (int i = 0; i < count; ++i)
                {
                    at[i] = this->Blend(at[i], Word(pixels, i), level);
                }
            }
        }
    );
    return 0;
}

// swd32.exe 0x0041E5C0: mirrored 0x0041D340
int Painter::BlitTranslucentMirrored(int, int, int width, int, const DrawArgument&)
{
    this->mirrorStart = width - (this->placement.skipLeft + this->placement.width);
    const int level = this->options.alpha;
    const LineWalk walk{.mirrored = true, .signedPitch = true};
    this->WalkRuns(
        walk,
        [this, level](RunKind kind, const u8* pixels, int count, u16* at)
        {
            if (kind == RunKind::Literal)
            {
                for (int i = 0; i < count; ++i)
                {
                    at[-1 - i] = this->Blend(at[-1 - i], Word(pixels, i), level);
                }
            }
        }
    );
    return 0;
}

// swd32.exe 0x0041CCF0: copy, then the first and the last pixel of every literal run set to the edge colour (the
// green mask)
int Painter::BlitEdgeMark(int, int, int, int, const DrawArgument&)
{
    const auto mark = static_cast<u16>(this->greenMaskPair);
    const LineWalk walk{};
    this->WalkRuns(
        walk,
        [mark](RunKind kind, const u8* pixels, int count, u16* at)
        {
            if (kind == RunKind::Literal)
            {
                std::memcpy(at, pixels, sizeof(u16) * static_cast<std::size_t>(count));
                at[0] = mark;
                at[count - 1] = mark;
            }
        }
    );
    return 0;
}

// swd32.exe 0x0041D010: mirrored 0x0041CCF0; the end mark lands one pixel beyond the run and the wave phase is not
// advanced
int Painter::BlitEdgeMarkMirrored(int, int, int width, int, const DrawArgument&)
{
    this->mirrorStart = width - (this->placement.skipLeft + this->placement.width);
    const auto mark = static_cast<u16>(this->greenMaskPair);
    const LineWalk walk{.mirrored = true, .advanceWave = false};
    this->WalkRuns(
        walk,
        [mark](RunKind kind, const u8* pixels, int count, u16* at)
        {
            if (kind == RunKind::Literal)
            {
                for (int i = 0; i < count; ++i)
                {
                    at[-1 - i] = Word(pixels, i);
                }

                at[-1] = mark;
                at[-count - 1] = mark;
            }
        }
    );
    return 0;
}

// swd32.exe 0x0041B9F0: literal pixels blended at a level that starts at 15 and drops by one every
// (height + 16) >> 4 lines, the clipped top lines included. The original indexes a jump table with the level; it
// never gets below 0 within a picture height (a negative level is taken as 0 here).
int Painter::BlitFadeDown(int, int, int, int height, const DrawArgument&)
{
    const int period = (height + kFadeSteps) >> kFadeShift;
    int countdown = period;
    int level = Painter::MaxAlpha;
    const int start = this->placement.skipLeft;
    const int columns = this->placement.width;
    const LineWalk walk{};
    this->ForEachLine(
        walk,
        [this, &level, start, columns](const u8* codes, u16* lineDest, int)
        {
            this->ForEachRun(
                codes,
                start,
                columns,
                [this, level, lineDest](RunKind kind, const u8* pixels, int count, int offset)
                {
                    if (kind == RunKind::Literal)
                    {
                        u16* at = lineDest + offset;

                        for (int i = 0; i < count; ++i)
                        {
                            at[i] = this->Blend(at[i], Word(pixels, i), level);
                        }
                    }
                }
            );
        },

        [&countdown, &level, period]
        {
            if (--countdown == 0)
            {
                countdown = period;
                --level;
            }
        }
    );
    return 0;
}

// swd32.exe 0x004208D0: vertically scaled (DrawOptions::scaleHeight) tinted add of the literal pixels; both other run
// kinds are transparent. The original never initializes its scale remainder (stack garbage below 0x400 for the first
// line); it starts at 0 here.
int Painter::BlitScaledGlow(int, int, int, int, const DrawArgument&)
{
    const Tint tint = this->CurrentTint();
    const LineWalk walk{.scaled = true};
    this->WalkRuns(
        walk,
        [this, &tint](RunKind kind, const u8* pixels, int count, u16* at)
        {
            if (kind == RunKind::Literal)
            {
                for (int i = 0; i < count; ++i)
                {
                    at[i] = this->AddTinted(tint, Word(pixels, i), at[i]);
                }
            }
        }
    );
    return 0;
}

// swd32.exe 0x00420D70: mirrored 0x004208D0 (same uninitialised remainder)
int Painter::BlitScaledGlowMirrored(int, int, int width, int, const DrawArgument&)
{
    this->mirrorStart = width - (this->placement.skipLeft + this->placement.width);
    const Tint tint = this->CurrentTint();
    const LineWalk walk{.mirrored = true, .scaled = true};
    this->WalkRuns(
        walk,
        [this, &tint](RunKind kind, const u8* pixels, int count, u16* at)
        {
            if (kind == RunKind::Literal)
            {
                for (int i = 0; i < count; ++i)
                {
                    at[-1 - i] = this->AddTinted(tint, Word(pixels, i), at[-1 - i]);
                }
            }
        }
    );
    return 0;
}

// swd32.exe 0x00421230: literal pixels filled with the colour pair: runs up to 4 pixels and the alignment and tail
// pixels take the low half, the pair loop writes whole pairs (every second pixel gets the high half)
int Painter::BlitSilhouette(int, int, int, int, const DrawArgument& argument)
{
    const u32 color = argument.colorPair;
    const auto low = static_cast<u16>(color);
    const LineWalk walk{.signedPitch = true};
    this->WalkRuns(
        walk,
        [color, low](RunKind kind, const u8*, int count, u16* at)
        {
            if (kind != RunKind::Literal)
            {
                return;
            }

            if (count <= kPairLoopMinimum)
            {
                std::fill_n(at, count, low);
                return;
            }

            if (OddAligned(at))
            {
                *at++ = low;
                --count;
            }

            for (; count >= kPixelsPerPair; count -= kPixelsPerPair, at += kPixelsPerPair)
            {
                WritePair(at, color);
            }

            if (count == 1)
            {
                *at = low;
            }
        }
    );
    return 0;
}

// swd32.exe 0x00421540: mirrored 0x00421230: the pairs are written swapped, so the high half lands on the left pixel;
// the wave phase is not advanced
int Painter::BlitSilhouetteMirrored(int, int, int width, int, const DrawArgument& argument)
{
    this->mirrorStart = width - (this->placement.skipLeft + this->placement.width);
    const u32 color = argument.colorPair;
    const auto low = static_cast<u16>(color);
    const u32 swapped = MakeWords(LowWord(color), HighWord(color));
    const LineWalk walk{.mirrored = true, .signedPitch = true, .advanceWave = false};
    this->WalkRuns(
        walk,
        [low, swapped](RunKind kind, const u8*, int count, u16* at)
        {
            if (kind != RunKind::Literal)
            {
                return;
            }

            if (count <= kPairLoopMinimum)
            {
                std::fill_n(at - count, count, low);
                return;
            }

            if (OddAligned(at))
            {
                *--at = low;

                if (--count == 0)
                {
                    return;
                }
            }

            for (; count >= kPixelsPerPair; count -= kPixelsPerPair)
            {
                at -= kPixelsPerPair;
                WritePair(at, swapped);
            }

            if (count == 1)
            {
                *--at = low;
            }
        }
    );
    return 0;
}

// swd32.exe 0x00421850: the destination under the literal pixels turned grey; like the tint effect it draws nothing
// without a tint option. Wave rows 0x528 bytes apart.
int Painter::BlitGray(int, int, int, int, const DrawArgument&)
{
    if (!this->Tinted())
    {
        return 0;
    }

    const LineWalk walk{.waveStride = Painter::WideWaveStride};
    this->WalkRuns(
        walk,
        [this](RunKind kind, const u8*, int count, u16* at)
        {
            if (kind == RunKind::Literal)
            {
                for (int i = 0; i < count; ++i)
                {
                    at[i] = this->Gray(at[i]);
                }
            }
        }
    );
    return 0;
}

// swd32.exe 0x00421BE0: mirrored 0x00421850 (wave rows 0x84 apart)
int Painter::BlitGrayMirrored(int, int, int width, int, const DrawArgument&)
{
    if (!this->Tinted())
    {
        return 0;
    }

    this->mirrorStart = width - (this->placement.skipLeft + this->placement.width);
    const LineWalk walk{.mirrored = true};
    this->WalkRuns(
        walk,
        [this](RunKind kind, const u8*, int count, u16* at)
        {
            if (kind == RunKind::Literal)
            {
                for (int i = 0; i < count; ++i)
                {
                    at[-1 - i] = this->Gray(at[-1 - i]);
                }
            }
        }
    );
    return 0;
}

// swd32.exe 0x00422030: dest - (source + tint) under the literal pixels; DrawOptions::dropLines skips every third line
int Painter::BlitSubtractive(int, int, int, int, const DrawArgument&)
{
    const Tint tint = this->CurrentTint();
    const LineWalk walk{.dropLines = true};
    this->WalkRuns(
        walk,
        [this, &tint](RunKind kind, const u8* pixels, int count, u16* at)
        {
            if (kind == RunKind::Literal)
            {
                for (int i = 0; i < count; ++i)
                {
                    at[i] = this->SubtractTintedSource(tint, Word(pixels, i), at[i]);
                }
            }
        }
    );
    return 0;
}

// swd32.exe 0x004223A0: mirrored 0x00422030, computing (dest + tint) - source
int Painter::BlitSubtractiveMirrored(int, int, int width, int, const DrawArgument&)
{
    this->mirrorStart = width - (this->placement.skipLeft + this->placement.width);
    const Tint tint = this->CurrentTint();
    const LineWalk walk{.mirrored = true, .dropLines = true};
    this->WalkRuns(
        walk,
        [this, &tint](RunKind kind, const u8* pixels, int count, u16* at)
        {
            if (kind == RunKind::Literal)
            {
                for (int i = 0; i < count; ++i)
                {
                    at[-1 - i] = this->SubtractFromTintedDest(tint, Word(pixels, i), at[-1 - i]);
                }
            }
        }
    );
    return 0;
}

// swd32.exe 0x00422730: the destination under the literal pixels smeared (see Smear)
int Painter::BlitSmear(int, int, int, int, const DrawArgument&)
{
    const LineWalk walk{.signedPitch = true};
    this->WalkRuns(
        walk,
        [this](RunKind kind, const u8*, int count, u16* at)
        {
            if (kind == RunKind::Literal)
            {
                for (int i = 0; i < count; ++i)
                {
                    this->Smear(at + i);
                }
            }
        }
    );
    return 0;
}

// swd32.exe 0x004229C0: mirrored 0x00422730; it smears before stepping back, so every pixel lands one to the right
// of its mirrored position; the wave phase is not advanced
int Painter::BlitSmearMirrored(int, int, int width, int, const DrawArgument&)
{
    this->mirrorStart = width - (this->placement.skipLeft + this->placement.width);
    const LineWalk walk{.mirrored = true, .signedPitch = true, .advanceWave = false};
    this->WalkRuns(
        walk,
        [this](RunKind kind, const u8*, int count, u16* at)
        {
            if (kind == RunKind::Literal)
            {
                for (int i = 0; i < count; ++i)
                {
                    this->Smear(at - i);
                }
            }
        }
    );
    return 0;
}

// The sheared, vertically scaled walk of the shadow effect. The top skip passes one line more than the clipped top
// lines and stops at the end of the picture. Every output line moves by the accumulated shear, is clipped on its own
// and is drawn one line lower than its index (row y + line). The wave offset is consumed but never applied; a line
// clipped away entirely still advances the source. x is replaced by the negated left clip when there is one.
template <typename LiteralOp>
void Painter::WalkShear(bool mirrored, int x, int y, int width, LiteralOp&& literal)
{
    if (this->placement.skipLeft != 0)
    {
        x = -this->placement.skipLeft;
    }

    u32 scaleFraction = 0;
    u32 shearFraction = 0;

    auto advanceShear = [this, &shearFraction]
    {
        const u32 sum = shearFraction + static_cast<u32>(this->placement.shearStep);
        shearFraction = sum & kFractionMask;
        this->options.shear += this->placement.shearDirection * static_cast<int>(sum >> Painter::FractionBits);
    };

    this->WaveBegin(Painter::WaveRowBytes);
    const u8* lines = this->source;
    u16* dest = this->placement.dest;
    bool finished = (Word(lines, RleHeader::FormatWord) & RleHeader::TrueColor) == 0;

    if (!finished)
    {
        lines += RleHeader::HeaderBytes;

        for (int counter = 0;; ++counter)
        {
            const u16 header = Word(lines);

            if (header == 0)
            {
                finished = true;
                break;
            }

            lines += header & RleHeader::LengthMask;
            u32 step = this->ScaleStep(scaleFraction);

            if (!this->placement.scaleUp)
            {
                while (!finished && step-- != 0)
                {
                    const u16 next = Word(lines);
                    finished = next == 0;
                    lines += next & RleHeader::LengthMask;
                }
            }
            else if (step != 0)
            {
                const u16 next = Word(lines);
                finished = next == 0;
                lines += next & RleHeader::LengthMask;
            }

            if (finished)
            {
                break;
            }

            advanceShear();

            if (counter == this->placement.skipTop)
            {
                break;
            }
        }
    }

    for (int line = 0; !finished && lines != nullptr;)
    {
        const u16 header = Word(lines);
        dest = this->WaveLine(dest);

        if (header == 0)
        {
            break;
        }

        ++line;

        if (line > this->placement.rows)
        {
            break;
        }

        advanceShear();
        int columns = width;
        int skipLeft = 0;
        int position = x + this->options.shear;
        bool visible = true;

        if (this->clipX > position)
        {
            columns = position + columns - this->clipX;
            visible = columns > 0;
            skipLeft = this->clipX - position;
            position = this->clipX;
        }

        if (visible && position + columns >= this->clipX + this->clipWidth)
        {
            columns = this->clipX + this->clipWidth - position;
            visible = columns > 0;
        }

        if (visible)
        {
            int row = y + line;
            int start = skipLeft;

            if (mirrored)
            {
                position += columns;
                start = width - skipLeft - columns;
            }

            if (this->placement.pitch < 0)
            {
                row = y + this->placement.rows - line;
            }

            dest = this->Pixel(position, row);
            u16* lineDest = dest;
            this->ForEachRun(
                lines + sizeof(u16),
                start,
                columns,
                [&literal, lineDest, mirrored](RunKind kind, const u8*, int count, int offset)
                {
                    if (kind == RunKind::Literal)
                    {
                        literal(mirrored ? lineDest - offset : lineDest + offset, count);
                    }
                }
            );
        }

        dest += this->placement.pitch;
        lines = this->ScaledNextLine(lines, scaleFraction);
    }

    if (!mirrored)
    {
        this->WaveAdvance();
    }
}

// swd32.exe 0x0041F8D0: the character shadow: the destination under the literal pixels tinted (also with zero tint
// options), sheared by DrawOptions::shear and scaled to DrawOptions::scaleHeight
int Painter::BlitShadow(int x, int y, int width, int, const DrawArgument&)
{
    const Tint tint = this->CurrentTint();
    this->WalkShear(
        false,
        x,
        y,
        width,
        [this, &tint](u16* at, int count)
        {
            for (int i = 0; i < count; ++i)
            {
                at[i] = this->TintPixel(tint, at[i]);
            }
        }
    );
    return 0;
}

// swd32.exe 0x0041FEA0: mirrored 0x0041F8D0 (wave phase not advanced)
int Painter::BlitShadowMirrored(int x, int y, int width, int, const DrawArgument&)
{
    const Tint tint = this->CurrentTint();
    this->WalkShear(
        true,
        x,
        y,
        width,
        [this, &tint](u16* at, int count)
        {
            for (int i = 0; i < count; ++i)
            {
                at[-1 - i] = this->TintPixel(tint, at[-1 - i]);
            }
        }
    );
    return 0;
}

}  // namespace swd3
