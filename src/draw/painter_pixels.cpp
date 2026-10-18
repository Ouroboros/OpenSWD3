#include <algorithm>
#include <array>
#include <bit>
#include <cstring>

#if defined(__SSE2__)
#include <immintrin.h>
#if defined(__clang__) || defined(__GNUC__)
#include <cpuid.h>
#endif
#endif

#include "core/words.h"
#include "draw/painter.h"

namespace swd3
{

namespace
{

constexpr u32 kLowHalf = 0xFFFF;  // the low and the high pixel of a pair
constexpr u32 kHighHalf = ~kLowHalf;
constexpr std::size_t kPairPixels = 2;
constexpr int kPixelBits = 16;  // the bits of a display pixel

u32 ReadPair(const u16* pixels)
{
    u32 value = 0;
    std::memcpy(&value, pixels, sizeof(value));
    return value;
}

void WritePair(u16* pixels, u32 value)
{
    std::memcpy(pixels, &value, sizeof(value));
}

// One channel of two pixels per step (0x00420600 / 0x004206F0). Only the high pixel's overflow is detected with the
// inverse mask shifted up, the low pixel's on the low word; going down, a borrow of the low pixel runs into the high
// pixel's channel. The original steps the count by 2 and stops only at exactly 0, running past the buffer for an odd
// count; here a single trailing pixel is left as it is.
void ShiftChannelPairs(std::span<u16> pixels, int delta, const ColorChannel& channel)
{
    const u32 highInverse = channel.inverseMask << kWordBits;
    const u32 maskPair = channel.mask | (channel.mask << kWordBits);
    const u32 otherBits = ~maskPair;
    const auto step = static_cast<i32>(channel.halves[ColorChannel::TintHalf]);
    u32 offset = static_cast<u32>(delta >= 0 ? step * delta : -(step * delta));
    offset |= offset << kWordBits;
    u32 saturated = channel.mask << kWordBits;

    for (std::size_t i = 0; i + 1 < pixels.size(); i += kPairPixels)
    {
        const u32 dest = ReadPair(&pixels[i]);
        u32 value = 0;

        if (delta >= 0)
        {
            value = (dest & maskPair) + offset;

            if ((otherBits & value & kLowHalf) != 0)
            {
                value = (value & kHighHalf) | (channel.mask & kLowHalf);
            }

            if ((highInverse & value) != 0)
            {
                saturated = (saturated & kHighHalf) | (value & kLowHalf);
                value = saturated;
            }
        }
        else
        {
            value = (dest & maskPair) - offset;

            if ((otherBits & value & kLowHalf) != 0)
            {
                value &= kHighHalf;
            }

            if ((highInverse & value) != 0)
            {
                value &= kLowHalf;
            }
        }

        WritePair(&pixels[i], (dest & otherBits) | value);
    }
}

// A channel moved to the top of the 16 bits: a level then adds or subtracts with 16-bit saturation, which keeps it
// within the channel (a level of the channel's whole range saturates every value alike, so the levels are held to
// it).
struct TintLane
{
    u16 mask;      // the channel's bits
    int lift;      // how far it is moved up
    u16 top;       // its bits moved up
    u16 add;       // the level as a step at the top, when raising
    u16 subtract;  // when lowering
};

using TintLanes = std::array<TintLane, 3>;

Painter::TintPath tintPath = Painter::TintPath::Best;

#if defined(__SSE2__) && (defined(__clang__) || defined(__GNUC__))
#define SWD3_TINT_AVX2 1

// Whether the processor and the system run AVX2 (CPUID 7 EBX bit 5, AVX and OSXSAVE in CPUID 1, the AVX state in
// XCR0).
bool HasAvx2()
{
    constexpr unsigned kOsSaves = 1u << 27;
    constexpr unsigned kAvx = 1u << 28;
    constexpr unsigned kAvx2 = 1u << 5;
    constexpr unsigned kAvxState = 0x6;  // the SSE and AVX registers saved by the system
    unsigned eax = 0;
    unsigned ebx = 0;
    unsigned ecx = 0;
    unsigned edx = 0;

    if (__get_cpuid(1, &eax, &ebx, &ecx, &edx) == 0 || (ecx & (kOsSaves | kAvx)) != (kOsSaves | kAvx))
    {
        return false;
    }

    unsigned low = 0;
    unsigned high = 0;
    __asm__("xgetbv" : "=a"(low), "=d"(high) : "c"(0));

    if ((low & kAvxState) != kAvxState || __get_cpuid_count(7, 0, &eax, &ebx, &ecx, &edx) == 0)
    {
        return false;
    }

    return (ebx & kAvx2) != 0;
}

// 16 pixels at a time; the pixels processed (a multiple of 16).
__attribute__((target("avx2"))) std::size_t TintAvx2(std::span<u16> pixels, const TintLanes& lanes)
{
    constexpr std::size_t kLanes = sizeof(__m256i) / sizeof(u16);
    std::array<__m256i, 3> masks{};
    std::array<__m256i, 3> tops{};
    std::array<__m256i, 3> adds{};
    std::array<__m256i, 3> subtracts{};
    std::array<__m128i, 3> lifts{};

    for (std::size_t at = 0; at < lanes.size(); ++at)
    {
        masks[at] = _mm256_set1_epi16(static_cast<short>(lanes[at].mask));
        tops[at] = _mm256_set1_epi16(static_cast<short>(lanes[at].top));
        adds[at] = _mm256_set1_epi16(static_cast<short>(lanes[at].add));
        subtracts[at] = _mm256_set1_epi16(static_cast<short>(lanes[at].subtract));
        lifts[at] = _mm_cvtsi32_si128(lanes[at].lift);
    }

    std::size_t index = 0;

    for (; index + kLanes <= pixels.size(); index += kLanes)
    {
        auto* place = reinterpret_cast<__m256i*>(pixels.data() + index);
        const __m256i pixel = _mm256_loadu_si256(place);
        __m256i result = _mm256_setzero_si256();

        for (std::size_t at = 0; at < lanes.size(); ++at)
        {
            __m256i value = _mm256_sll_epi16(_mm256_and_si256(pixel, masks[at]), lifts[at]);
            value = _mm256_subs_epu16(_mm256_adds_epu16(value, adds[at]), subtracts[at]);
            result = _mm256_or_si256(result, _mm256_srl_epi16(_mm256_and_si256(value, tops[at]), lifts[at]));
        }

        _mm256_storeu_si256(place, result);
    }

    return index;
}
#endif
}  // namespace

// swd32.exe 0x00420490: tint steps added to every pixel, saturating like the tint effect. The original treats a
// count of 0 as 2^32; an empty span tints nothing here. Longer runs go through TintPixelsFast, which gives the same
// pixels.
void Painter::TintPixels(std::span<u16> pixels, int red, int green, int blue) const
{
    constexpr std::size_t kFastPixels = 0x40;

    if (pixels.size() >= kFastPixels)
    {
        this->TintPixelsFast(pixels, red, green, blue);
        return;
    }

    this->TintPixelsEach(pixels, red, green, blue);
}

// The pixels one at a time (0x00420490 as the original runs it).
void Painter::TintPixelsEach(std::span<u16> pixels, int red, int green, int blue) const
{
    const Tint tint = this->MakeTint(red, green, blue);

    for (u16& pixel : pixels)
    {
        pixel = this->TintPixel(tint, pixel);
    }
}

void Painter::SetTintPath(TintPath path)
{
    tintPath = path;
}

void Painter::TintPixelsFast(std::span<u16> pixels, int red, int green, int blue) const
{
    using Lane = TintLane;

    auto lane = [](const ColorChannel& channel, int level, bool& fits)
    {
        const u32 lowest = channel.mask & (0u - channel.mask);
        const int shift = channel.mask != 0 ? std::countr_zero(channel.mask) : 0;
        const u32 values = channel.mask >> shift;
        const int width = std::popcount(values);
        fits = fits && channel.mask != 0 && channel.halves[ColorChannel::TintHalf] == lowest &&
               (values & (values + 1)) == 0 && shift + width <= kPixelBits;

        if (!fits)
        {
            return Lane{};
        }

        const int lift = kPixelBits - shift - width;
        const auto most = static_cast<int>(values);
        const int held = std::clamp(level, -most, most);
        const auto step = static_cast<u16>(static_cast<u32>(held < 0 ? -held : held) << (shift + lift));
        return Lane{
            static_cast<u16>(channel.mask),
            lift,
            static_cast<u16>(channel.mask << lift),
            held > 0 ? step : static_cast<u16>(0),
            held < 0 ? step : static_cast<u16>(0),
        };
    };

    bool fits = true;
    const TintLanes lanes = {lane(this->red, red, fits), lane(this->green, green, fits), lane(this->blue, blue, fits)};

    if (!fits)
    {
        this->TintPixelsEach(pixels, red, green, blue);
        return;
    }

    auto tint = [&lanes](u16 pixel)
    {
        u32 result = 0;

        for (const Lane& channel : lanes)
        {
            u32 value = static_cast<u32>(pixel & channel.mask) << channel.lift;
            value = std::min<u32>(value + channel.add, 0xFFFF);
            value = value > channel.subtract ? value - channel.subtract : 0;
            result |= (value & channel.top) >> channel.lift;
        }

        return static_cast<u16>(result);
    };

    std::size_t index = 0;
    const TintPath path = tintPath;

#if defined(SWD3_TINT_AVX2)
    static const bool avx2 = HasAvx2();

    if (avx2 && path == TintPath::Best)
    {
        index = TintAvx2(pixels, lanes);
    }
#endif

#if defined(__SSE2__)
    constexpr std::size_t kLanes = sizeof(__m128i) / sizeof(u16);
    std::array<__m128i, 3> masks{};
    std::array<__m128i, 3> tops{};
    std::array<__m128i, 3> adds{};
    std::array<__m128i, 3> subtracts{};
    std::array<__m128i, 3> lifts{};

    for (std::size_t at = 0; at < lanes.size(); ++at)
    {
        masks[at] = _mm_set1_epi16(static_cast<short>(lanes[at].mask));
        tops[at] = _mm_set1_epi16(static_cast<short>(lanes[at].top));
        adds[at] = _mm_set1_epi16(static_cast<short>(lanes[at].add));
        subtracts[at] = _mm_set1_epi16(static_cast<short>(lanes[at].subtract));
        lifts[at] = _mm_cvtsi32_si128(lanes[at].lift);
    }

    for (; path != TintPath::Scalar && index + kLanes <= pixels.size(); index += kLanes)
    {
        auto* place = reinterpret_cast<__m128i*>(pixels.data() + index);
        const __m128i pixel = _mm_loadu_si128(place);
        __m128i result = _mm_setzero_si128();

        for (std::size_t at = 0; at < lanes.size(); ++at)
        {
            __m128i value = _mm_sll_epi16(_mm_and_si128(pixel, masks[at]), lifts[at]);
            value = _mm_subs_epu16(_mm_adds_epu16(value, adds[at]), subtracts[at]);
            result = _mm_or_si128(result, _mm_srl_epi16(_mm_and_si128(value, tops[at]), lifts[at]));
        }

        _mm_storeu_si128(place, result);
    }
#endif

    for (; index < pixels.size(); ++index)
    {
        pixels[index] = tint(pixels[index]);
    }
}

// swd32.exe 0x00420560: the red channel moved by `delta` tint steps, saturating at the mask or 0
void Painter::ShiftRed(std::span<u16> pixels, int delta) const
{
    const u32 mask = this->red.mask;
    const u32 otherBits = 0xFFFFFFFFu - mask;
    const auto step = static_cast<i32>(this->red.halves[ColorChannel::TintHalf]);

    for (u16& pixel : pixels)
    {
        const u32 dest = pixel;
        u32 value = 0;

        if (delta >= 0)
        {
            value = (dest & mask) + static_cast<u32>(step * delta);

            if ((otherBits & value) != 0)
            {
                value = mask;
            }
        }
        else
        {
            value = (dest & mask) - static_cast<u32>(-(step * delta));

            if ((otherBits & value) != 0)
            {
                value = 0;
            }
        }

        pixel = static_cast<u16>((dest & otherBits) | value);
    }
}

// swd32.exe 0x00420600: the green channel, two pixels per step
void Painter::ShiftGreenPairs(std::span<u16> pixels, int delta) const
{
    ShiftChannelPairs(pixels, delta, this->green);
}

// swd32.exe 0x004206F0: the blue channel, two pixels per step
void Painter::ShiftBluePairs(std::span<u16> pixels, int delta) const
{
    ShiftChannelPairs(pixels, delta, this->blue);
}

// swd32.exe 0x004207E0: every pixel becomes the per-channel saturated sum of itself and the matching pixel of `other`
// (through the channel ramps). The channel masks are cut to 16 bits first, as in the original.
void Painter::MixPixels(std::span<const u16> other, std::span<u16> pixels)
{
    this->red.mask = LowWord(this->red.mask);
    this->green.mask = LowWord(this->green.mask);
    this->blue.mask = LowWord(this->blue.mask);

    for (std::size_t i = 0; i < pixels.size(); ++i)
    {
        const u32 added = other[i];
        const u32 own = pixels[i];

        auto sum = [added, own](const ColorChannel& channel)
        {
            return channel.Ramp(static_cast<int>(((channel.mask & own) + (channel.mask & added)) >> channel.shift));
        };

        pixels[i] = static_cast<u16>(sum(this->blue) + sum(this->green) + sum(this->red));
    }
}

// swd32.exe 0x00421FB0: every pixel grey ((r + g + b) / 4 in each channel)
void Painter::GrayPixels(std::span<u16> pixels) const
{
    for (u16& pixel : pixels)
    {
        pixel = this->Gray(pixel);
    }
}

}  // namespace swd3
