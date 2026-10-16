#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

namespace swd3
{

// Results of the LZO1X decompressor. The original (the unchecked "fast" variant of LZO 1.x) returns 0, -4 or -8;
// the two overrun results for output and look-behind replace its unchecked memory accesses.
enum class LzoStatus
{
    Ok,                // LZO_E_OK (0)
    InputOverrun,      // LZO_E_INPUT_OVERRUN (-4)
    InputNotConsumed,  // LZO_E_INPUT_NOT_CONSUMED (-8)
    OutputOverrun,
    LookbehindOverrun,
};

// LZO1X decompression of one block (tsw frames, act scripts, fame and save data).
class LzoDecoder
{
public:
    LzoDecoder(std::span<const std::uint8_t> input, std::span<std::uint8_t> output);

    LzoStatus Run();

    std::size_t Written() const;

private:
    bool ReadByte(std::uint32_t& value);
    bool ReadLength(std::uint32_t base, std::uint32_t& length);
    LzoStatus CopyLiterals(std::uint32_t count);
    LzoStatus CopyMatch(std::size_t distance, std::uint32_t count);
    std::uint32_t TrailingLiterals() const;

    std::span<const std::uint8_t> input;
    std::span<std::uint8_t> output;
    std::size_t inputPosition = 0;
    std::size_t outputPosition = 0;
};

}  // namespace swd3
