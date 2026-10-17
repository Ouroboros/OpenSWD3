#pragma once

#include <array>
#include <cstdint>
#include <span>

#include "core/types.h"

namespace swd3
{

class Surface;

// Extra data of the effects that take one (the original passes a pointer as the sixth argument).
struct DrawArgument
{
    const u8* alphaMap = nullptr;  // kAlphaMap: one level byte (multiple of 4) per picture pixel
    u32 colorPair = 0;             // kSilhouette: the colour in both 16-bit halves
};

struct Picture;

// The palette of a picture as the draw argument of an effect (the original passes the palette pointer).
DrawArgument PaletteArgument(const Picture& picture);

// Options read by the next DrawPicture and reset after it (the original's draw option globals).
struct DrawOptions
{
    bool dropLines = false;  // every third line is skipped (kAdditive, kSubtractive)
    int tintRed = 0;         // tint steps per channel, in 1/16 of the channel range
    int tintGreen = 0;
    int tintBlue = 0;
    int unusedFlag = 0;   // reset like the others, read by no effect
    int scaleHeight = 0;  // output height of the vertically scaled effects (0: unscaled)
    int shear = 0;        // horizontal shear over the picture height (kShadow)
    int alpha = 0;        // kTranslucent level 1..15
};

// One colour channel of the display format.
struct ColorChannel
{
    static constexpr int Levels = 32;  // a 5-bit channel
    static constexpr std::size_t Shifts = 5;
    static constexpr std::size_t TintHalf = 4;  // halves[TintHalf] is one tint step

    u32 mask = 0;
    u32 inverseMask = 0;  // 0xFFFFFFFF - mask
    int shift = 0;
    // halves[k]: the mask shifted right by k with the bits of the channel kept; halves[4] is one tint step.
    std::array<u32, Shifts + 1> halves{};
    // steps[k]: (k + 1) << shift.
    std::array<u32, Shifts> steps{};
    // Saturating sum lookup: 32 zero entries, the ramp k * halves[4] for k = 0..31, 32 entries of the mask.
    std::array<u32, 3 * Levels> ramp{};

    u32 Ramp(int index) const;
};

// Colours of the RLE format in the display format (the codes 0x8000 / 0xC000 stand for runs of them).
struct RleColors
{
    u32 magic = 0xFFFF;
    u16 fill16 = 0x26B;   // 0xC000 runs
    u16 skip16 = 0x319F;  // 0x8000 runs
    u8 fill8 = 1;
    u8 reserved8 = 2;
    u8 skip8 = 3;
};

// The drawing kernel (0x00416D30-0x004239D0): pixel format detection and colour tables, clipping, and the blitters
// of RLE pictures (see pic/rle.h), raw pictures and 16x16 map tiles on a 16-bit RGB surface.
class Painter
{
public:
    // Draw modes of Painter::DrawPicture. Bit 0 mirrors horizontally, bit 1 flips vertically (only honoured by the
    // effects that advance lines by the signed pitch), bits 2..15 select the effect and bit 31 marks RLE source data
    // (set automatically for data starting with the RLE magic when no palette is selected).
    enum DrawMode : u32
    {
        Mirror = 0x1,
        Flip = 0x2,
        RleSource = 0x80000000u,
        EffectMask = 0xFFFC,

        // RLE effects.
        Copy = 0x00,         // literal pixels copied
        Additive = 0x04,     // saturating add, optionally tinted
        AlphaMap = 0x08,     // per-pixel level from a byte map (DrawArgument::alphaMap)
        Shadow = 0x0C,       // sheared and scaled tint of the destination
        TintUnder = 0x10,    // tint of the destination under the picture
        Translucent = 0x14,  // blend at DrawOptions::alpha
        EdgeMark = 0x18,     // copy, first and last pixel of each run marked
        FadeDown = 0x1C,     // blend level falling from the top
        ScaledGlow = 0x20,   // vertically scaled tinted add
        Silhouette = 0x24,   // literal pixels filled with DrawArgument::color
        GrayUnder = 0x28,    // destination under the picture turned grey
        Subtractive = 0x2C,  // saturating subtract, tinted
        SmearUnder = 0x30,   // destination smeared under the picture

        // Raw effects (16-bit pixels or palette indices).
        RawCopy = 0x00,
        RawKeyed = 0x04,        // fill colour transparent
        RawGradient = 0x08,     // destination blended towards the first source pixel
        RawTranslucent = 0x14,  // blend at DrawOptions::alpha, fill colour transparent
    };

    static constexpr u32 RawBlitters = 0x80;  // the blitters of raw pictures follow the RLE ones
    static constexpr int TileSide = 16;       // DrawTile16 and DrawTile8
    static constexpr int MaxAlpha = 0xF;      // DrawOptions::alpha of a fully opaque blend
    static constexpr int FractionBits = 10;   // the scale and shear steps are fixed point
    static constexpr int DitherPhases = 3;
    static constexpr u8 KeyIndex = 1;             // DrawTile8Keyed: the transparent palette index
    static constexpr int WaveRowBytes = 0x84;     // a row of the wave offsets (33 dwords)
    static constexpr int WideWaveStride = 0x528;  // the stride of the tint and grey walks
    static constexpr int WaveEntryBytes = sizeof(i32);

    // How the display stores colours; pictures are stored as RGB555 and converted on load.
    enum class Conversion
    {
        None,    // RGB555 display
        To5551,  // R5 G5 B5 A1
        To565,   // RGB565 (the converter of the reverse direction is set as well)
        To6551,  // R6 G5 B5 with the low red bit left over
    };

    Painter();

    // Pixel format and screen geometry of the screen surface; without a surface the RGB555 defaults are used.
    bool Init(Surface& screen);
    bool InitPixelFormat(const Surface* screen);
    void ColorMasks(u32& red, u32& green, u32& blue) const;
    u32 FillColorPair() const;
    void ConvertPixels(std::span<u16> pixels) const;
    void ConvertPixelsBack(std::span<u16> pixels) const;
    u32 MakeColor(u32 red, u32 green, u32 blue) const;
    const RleColors& Rle() const;
    const ColorChannel& Red() const;
    const ColorChannel& Green() const;
    const ColorChannel& Blue() const;

    // Target surface and clipping.
    u16* LockSurface(Surface& surface);
    void UnlockSurface(Surface& surface);
    void SetTarget(u16* pixels);
    u16* Target() const;
    bool SetClip(int left, int top, int right, int bottom);
    void SetClipClamped(int left, int top, int right, int bottom);
    // The clip rectangle as left, top, width and height.
    std::array<int, 4> ClipRect() const;
    // Pixels per line of the screen.
    int ScreenPitch() const;
    // The channel bits kept by (pair >> (index + 1)) in both halves of a pixel pair.
    u32 HalfMaskPair(int index) const;

    // Source and persistent effect state.
    void SetSource(const void* data);
    const void* Source() const;
    void SetPalette(const u16* palette);
    DrawOptions& Options();
    void SetWave(int mode);
    int WavePhase() const;
    void SetWavePhase(int phase);
    void SetZoom(int anchorX, int anchorY, int zoomX, int zoomY);

    // Drawing.
    int DrawPicture(int x, int y, int width, int height, u32 mode, const DrawArgument& argument = {});
    void DrawOutline(int x, int y, int width, int height, u32 mode, const DrawArgument& argument = {});
    void DrawTile16(int x, int y);
    void DrawTile16Keyed(int x, int y);
    void DrawTile8(int x, int y);
    void DrawTile8Keyed(int x, int y);
    void DrawZoomed(int x, int y, int width, int height);
    void DrawZoomedMirrored(int x, int y, int width, int height);

    // Pixel operations on arrays of display pixels.
    void BlendPairsWithColor(std::span<u32> pairs, u32 colorPair) const;
    void TintPixels(std::span<u16> pixels, int red, int green, int blue) const;
    void ShiftRed(std::span<u16> pixels, int delta) const;
    void ShiftGreenPairs(std::span<u16> pixels, int delta) const;
    void ShiftBluePairs(std::span<u16> pixels, int delta) const;
    void MixPixels(std::span<const u16> other, std::span<u16> pixels);
    void GrayPixels(std::span<u16> pixels) const;

    // Per-line horizontal offsets of the wave effect: 15 rows of 33 (0x004A06A8).
    static std::span<const i32> WaveOffsets();

private:
    // Placement of the current DrawPicture after clipping.
    struct Placement
    {
        int skipTop = 0;
        int skipBottom = 0;
        int skipLeft = 0;
        int width = 0;  // visible columns
        int rows = 0;   // visible lines
        u16* dest = nullptr;
        int pitch = 0;  // pixels between lines, negative when flipped
        const u8* sourceLine = nullptr;
        int ditherPhase = 0;
        u32 scaleFraction = 0;
        bool scaleUp = false;
        int shearStep = 0;
        int shearDirection = 0;
    };

    // Run of an RLE line handed to the effect operations: kind, the literal pixels and how far its visible part
    // lies from the first drawn column.
    enum class RunKind
    {
        Literal,  // code 0x0000 | n, n pixels follow
        Fill,     // code 0xC000 | n (and 0x4000 | n), the fill colour
        Skip,     // code 0x8000 | n, the skip colour
    };

    // How an effect classifies the run codes of a line (see ForEachChainedRun).
    enum class RunChain
    {
        Checked,          // both kind bits of every code
        LiteralThenSkip,  // the code after a drawn literal run is taken as a 0x4000/0x8000 run unchecked
        Split,            // the alpha map and tint effects: the next kind follows from the previous one
    };

    struct Tint
    {
        u32 red = 0;
        u32 green = 0;
        u32 blue = 0;
        u32 redSaturated = 0;
        u32 greenSaturated = 0;
        u32 blueSaturated = 0;
    };

    struct LineWalk
    {
        bool mirrored = false;
        bool signedPitch = false;  // lines advance by Placement::pitch (else by the screen pitch, ignoring kFlip)
        bool dropLines = false;    // honour DrawOptions::dropLines
        bool advanceWave = true;
        int waveStride = WaveRowBytes;  // bytes between the wave rows (WideWaveStride in some effects)
        bool scaled = false;            // vertical scaling (kScaledGlow)
        RunChain chain = RunChain::Checked;
    };

    using Blitter = int (Painter::*)(int x, int y, int width, int height, const DrawArgument& argument);

    void BuildBlitTables();
    int Dispatch(u32 index, int x, int y, int width, int height, const DrawArgument& argument);
    u16* Pixel(int x, int y) const;
    std::ptrdiff_t RowOffset(int y) const;

    // Pixel helpers.
    u16 Blend(u16 dest, u16 source, int level) const;
    u16 AlphaBlend(u16 dest, u16 source, int level) const;
    u32 AlphaBlendPair(u32 dest, u32 source, int level) const;
    Tint MakeTint(int red, int green, int blue) const;
    Tint CurrentTint() const;
    bool Tinted() const;
    u16 TintPixel(const Tint& tint, u16 dest) const;
    u16 AddTinted(const Tint& tint, u16 source, u16 dest) const;
    u16 Add(u16 source, u16 dest) const;
    u16 SubtractTintedSource(const Tint& tint, u16 source, u16 dest) const;
    u16 SubtractFromTintedDest(const Tint& tint, u16 source, u16 dest) const;
    u16 Gray(u16 dest) const;
    void Smear(u16* at);

    // RLE line walking.
    void WaveBegin(int stride);
    u16* WaveLine(u16* dest);
    void WaveAdvance();
    u32 ScaleStep(u32& fraction) const;
    const u8* ScaledSkipTop(const u8* lines, u32& fraction) const;
    const u8* ScaledNextLine(const u8* line, u32& fraction) const;
    template <typename RunOp>
    void ForEachRun(const u8* codes, int start, int width, RunOp&& op) const;
    template <typename RunOp>
    void ForEachChainedRun(const u8* codes, int start, int width, RunChain chain, RunOp&& op) const;
    template <typename LineOp, typename SkipHook>
    void ForEachLine(const LineWalk& walk, LineOp&& op, SkipHook&& skippedLine);
    template <typename RunOp>
    void WalkRuns(const LineWalk& walk, RunOp&& op);

    // RLE blitters.
    int BlitCopy(int x, int y, int width, int height, const DrawArgument& argument);
    int BlitCopyMirrored(int x, int y, int width, int height, const DrawArgument& argument);
    int BlitAdditive(int x, int y, int width, int height, const DrawArgument& argument);
    int BlitAdditiveMirrored(int x, int y, int width, int height, const DrawArgument& argument);
    int BlitAlphaMap(int x, int y, int width, int height, const DrawArgument& argument);
    int BlitAlphaMapMirrored(int x, int y, int width, int height, const DrawArgument& argument);
    int BlitShadow(int x, int y, int width, int height, const DrawArgument& argument);
    int BlitShadowMirrored(int x, int y, int width, int height, const DrawArgument& argument);
    int BlitTint(int x, int y, int width, int height, const DrawArgument& argument);
    int BlitTintMirrored(int x, int y, int width, int height, const DrawArgument& argument);
    int BlitTranslucent(int x, int y, int width, int height, const DrawArgument& argument);
    int BlitTranslucentMirrored(int x, int y, int width, int height, const DrawArgument& argument);
    int BlitEdgeMark(int x, int y, int width, int height, const DrawArgument& argument);
    int BlitEdgeMarkMirrored(int x, int y, int width, int height, const DrawArgument& argument);
    int BlitFadeDown(int x, int y, int width, int height, const DrawArgument& argument);
    int BlitScaledGlow(int x, int y, int width, int height, const DrawArgument& argument);
    int BlitScaledGlowMirrored(int x, int y, int width, int height, const DrawArgument& argument);
    int BlitSilhouette(int x, int y, int width, int height, const DrawArgument& argument);
    int BlitSilhouetteMirrored(int x, int y, int width, int height, const DrawArgument& argument);
    int BlitGray(int x, int y, int width, int height, const DrawArgument& argument);
    int BlitGrayMirrored(int x, int y, int width, int height, const DrawArgument& argument);
    int BlitSubtractive(int x, int y, int width, int height, const DrawArgument& argument);
    int BlitSubtractiveMirrored(int x, int y, int width, int height, const DrawArgument& argument);
    int BlitSmear(int x, int y, int width, int height, const DrawArgument& argument);
    int BlitSmearMirrored(int x, int y, int width, int height, const DrawArgument& argument);
    template <typename LiteralOp>
    void WalkShear(bool mirrored, int x, int y, int width, LiteralOp&& literal);

    // Raw blitters.
    int BlitRawCopy(int x, int y, int width, int height, const DrawArgument& argument);
    int BlitRawCopyMirrored(int x, int y, int width, int height, const DrawArgument& argument);
    int BlitRawKeyed(int x, int y, int width, int height, const DrawArgument& argument);
    int BlitRawKeyedMirrored(int x, int y, int width, int height, const DrawArgument& argument);
    int BlitRawTranslucent(int x, int y, int width, int height, const DrawArgument& argument);
    int BlitRawGradient(int x, int y, int width, int height, const DrawArgument& argument);

    // Display format.
    static const std::array<i32, 495> WaveTable;
    u32 displayRed = 0;
    u32 displayGreen = 0;
    u32 displayBlue = 0;
    Conversion conversion = Conversion::None;
    Conversion conversionBack = Conversion::None;
    ColorChannel red;
    ColorChannel green;
    ColorChannel blue;
    std::array<u32, 5> halfMaskPairs{};  // channel bits kept by (pair >> (k + 1))
    u32 greenMaskPair = 0;               // edge mark colour (green mask in both halves)
    u32 fillColorPair = 0;
    u32 skipColorPair = 0;
    RleColors rleColors;

    // Screen geometry and target.
    int screenWidth = 0;
    int screenHeight = 0;
    int screenPitch = 0;   // pixels
    int surfacePitch = 0;  // pixels of the last locked surface
    u16* target = nullptr;
    // y * pitch for the screen rows; the rest of the 1024 entries stay 0, so rows below the screen land on row 0.
    std::array<i32, 1024> rowOffsets{};
    int clipX = 0;
    int clipY = 0;
    int clipWidth = 0;
    int clipHeight = 0;

    // Source and options.
    const u8* source = nullptr;
    const u16* palette = nullptr;
    DrawOptions options;
    Placement placement;
    int mirrorStart = 0;  // first drawn picture column of a mirrored walk
    std::array<Blitter, 256> blitters{};

    // Persistent effect state.
    int waveMode = 0;
    int wavePhase = 0;
    int waveStart = 0;
    int wavePosition = 0;
    int waveEnd = 0;
    int smearCounter = 0;
    int zoomAnchorX = 0x28;
    int zoomAnchorY = 0x48;
    int zoomX = 0x600;
    int zoomY = 0x400;
};

}  // namespace swd3
