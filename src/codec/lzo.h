#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "core/types.h"

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
    LzoDecoder(std::span<const u8> input, std::span<u8> output);

    LzoStatus Run();

    std::size_t Written() const;

private:
    bool ReadByte(u32& value);
    bool ReadLength(u32 base, u32& length);
    LzoStatus CopyLiterals(u32 count);
    LzoStatus CopyMatch(std::size_t distance, u32 count);
    u32 TrailingLiterals() const;

    std::span<const u8> input;
    std::span<u8> output;
    std::size_t inputPosition = 0;
    std::size_t outputPosition = 0;
};

// LZO1X-1 compression (the "1" level of LZO 1.0x) with a dictionary of 2^bits entries: 14 bits for the save data, 15
// for the fame block.
class LzoEncoder
{
public:
    explicit LzoEncoder(int dictionaryBits);

    std::vector<u8> Compress(std::span<const u8> input);

private:
    std::size_t CompressCore(std::span<const u8> input, std::vector<u8>& output);
    void EmitLiterals(std::vector<u8>& output, std::span<const u8> literals);

    int dictionaryBits;
    std::vector<i64> dictionary;
};

}  // namespace swd3
