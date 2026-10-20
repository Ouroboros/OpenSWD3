#include "draw/weave.h"

#include <array>

#if defined(__SSE2__)
#include <emmintrin.h>
#endif

#include "platform/display.h"
#include "platform/surface.h"

namespace swd3
{

void Weave(std::span<const Surface* const> parts, int scale, std::span<Pixel> to)
{
    const auto blocks = static_cast<std::size_t>(scale);
    const std::size_t pitch = static_cast<std::size_t>(Display::Width) * blocks;

    constexpr auto kWidth = static_cast<std::size_t>(Display::Width);
    std::array<const Pixel*, MaxWeaveScale> sources = {};

    for (int y = 0; y < Display::Height; ++y)
    {
        for (std::size_t row = 0; row < blocks; ++row)
        {
            Pixel* line = to.data() + (static_cast<std::size_t>(y) * blocks + row) * pitch;

            for (std::size_t column = 0; column < blocks; ++column)
            {
                sources[column] = parts[row * blocks + column]->Row(y);
            }

            std::size_t x = 0;

#if defined(__SSE2__)
            // Eight pixels of each part at a time, interleaved by unpacking.
            constexpr std::size_t kLanes = sizeof(__m128i) / sizeof(Pixel);
            constexpr std::size_t kPairBlocks = 2;  // the scales woven by unpacking pairs, then pairs of pairs
            constexpr std::size_t kQuadBlocks = 4;

            if (blocks == kPairBlocks)
            {
                for (; x + kLanes <= kWidth; x += kLanes)
                {
                    const __m128i first = _mm_loadu_si128(reinterpret_cast<const __m128i*>(sources[0] + x));
                    const __m128i second = _mm_loadu_si128(reinterpret_cast<const __m128i*>(sources[1] + x));
                    auto* out = reinterpret_cast<__m128i*>(line + kPairBlocks * x);
                    _mm_storeu_si128(out, _mm_unpacklo_epi16(first, second));
                    _mm_storeu_si128(out + 1, _mm_unpackhi_epi16(first, second));
                }
            }
            else if (blocks == kQuadBlocks)
            {
                for (; x + kLanes <= kWidth; x += kLanes)
                {
                    const __m128i first = _mm_loadu_si128(reinterpret_cast<const __m128i*>(sources[0] + x));
                    const __m128i second = _mm_loadu_si128(reinterpret_cast<const __m128i*>(sources[1] + x));
                    const __m128i third = _mm_loadu_si128(reinterpret_cast<const __m128i*>(sources[2] + x));
                    const __m128i fourth = _mm_loadu_si128(reinterpret_cast<const __m128i*>(sources[3] + x));
                    const __m128i lowPairs = _mm_unpacklo_epi16(first, second);
                    const __m128i highPairs = _mm_unpackhi_epi16(first, second);
                    const __m128i lowOthers = _mm_unpacklo_epi16(third, fourth);
                    const __m128i highOthers = _mm_unpackhi_epi16(third, fourth);
                    auto* out = reinterpret_cast<__m128i*>(line + kQuadBlocks * x);
                    _mm_storeu_si128(out, _mm_unpacklo_epi32(lowPairs, lowOthers));
                    _mm_storeu_si128(out + 1, _mm_unpackhi_epi32(lowPairs, lowOthers));
                    _mm_storeu_si128(out + 2, _mm_unpacklo_epi32(highPairs, highOthers));
                    _mm_storeu_si128(out + 3, _mm_unpackhi_epi32(highPairs, highOthers));
                }
            }
#endif

            for (; x < kWidth; ++x)
            {
                for (std::size_t column = 0; column < blocks; ++column)
                {
                    line[x * blocks + column] = sources[column][x];
                }
            }
        }
    }
}

}  // namespace swd3
