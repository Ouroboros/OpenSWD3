#include "codec/lzo.h"

#include <climits>

namespace swd3
{

namespace
{

// Largest distance of the two-byte match form (M2_MAX_OFFSET).
constexpr std::size_t kShortMatchDistance = 0x800;
// Distance offset of the 16..31 instructions (M3_MAX_OFFSET + 1 style window base).
constexpr std::size_t kFarMatchBase = 0x4000;

// The LZO1X format: an instruction byte below kM4Marker is a literal run; from kM4Marker a match of kind M4, from
// kM3Marker M3, from kM2Marker M2. Lengths beyond the bits of an instruction continue in zero bytes worth
// kLengthByte each; distances keep kDistanceLowBits bits in the instruction.
constexpr u32 kM2Marker = 0x40;
constexpr u32 kM3Marker = 0x20;
constexpr u32 kM4Marker = 0x10;
constexpr u32 kM2MaxOffset = kShortMatchDistance;
constexpr u32 kM3MaxOffset = kFarMatchBase;
constexpr u32 kFirstLiteralBias = 0x11;  // a first byte above it starts with that many literals less it
constexpr u32 kEndMarker = 0x11;         // an M4 match of distance 0
constexpr u32 kMaxFirstRun = 0xFF - kFirstLiteralBias;
constexpr u32 kShortFirstRun = 4;
constexpr u32 kLiteralLengthMask = 0xF;
constexpr u32 kM3LengthMask = 0x1F;
constexpr u32 kM4LengthMask = 7;
constexpr u32 kMinLiteralRun = 3;
constexpr u32 kMaxPackedLiterals = 3;  // literals after a match kept in the low bits of its instruction
constexpr u32 kMaxShortLiterals = kLiteralLengthMask + kMinLiteralRun;
constexpr u32 kTrailingMask = kMaxPackedLiterals;
constexpr u32 kM1Length = 3;          // the match right after a literal run
constexpr u32 kShortMatchLength = 2;  // a short match of a byte instruction
constexpr u32 kMinMatchExtra = 2;     // a match length is its count plus this
constexpr u32 kLengthByte = 0xFF;
constexpr int kDistanceLowBits = 2;
constexpr u32 kM2DistanceMask = 7;
constexpr int kM2HighShift = 3;
constexpr int kM2LengthShift = 5;
constexpr u32 kM2MaxLength = 8;
constexpr u32 kM4FarBit = 8;
constexpr int kM4FarShift = 11;
constexpr std::size_t kOffsetBytes = 2;  // the two distance bytes of a match; the first keeps the trailing literals

enum class Step
{
    Instruction,
    AfterLiteralRun,
    Match,
    MatchDone,
};

}  // namespace

LzoDecoder::LzoDecoder(std::span<const u8> input, std::span<u8> output)
    : input(input),
      output(output)
{
}

std::size_t LzoDecoder::Written() const
{
    return this->outputPosition;
}

bool LzoDecoder::ReadByte(u32& value)
{
    if (this->inputPosition >= this->input.size())
    {
        return false;
    }

    value = this->input[this->inputPosition++];
    return true;
}

// A run length: `base` plus 255 for every zero byte, plus the first non-zero byte.
bool LzoDecoder::ReadLength(u32 base, u32& length)
{
    u32 value = 0;
    length = 0;

    while (true)
    {
        if (!this->ReadByte(value))
        {
            return false;
        }

        if (value != 0)
        {
            break;
        }

        length += kLengthByte;
    }

    length += base + value;
    return true;
}

LzoStatus LzoDecoder::CopyLiterals(u32 count)
{
    if (count > this->input.size() - this->inputPosition)
    {
        return LzoStatus::InputOverrun;
    }

    if (count > this->output.size() - this->outputPosition)
    {
        return LzoStatus::OutputOverrun;
    }

    for (u32 i = 0; i < count; ++i)
    {
        this->output[this->outputPosition++] = this->input[this->inputPosition++];
    }

    return LzoStatus::Ok;
}

// Byte by byte, so that a distance shorter than the count repeats the bytes just written.
LzoStatus LzoDecoder::CopyMatch(std::size_t distance, u32 count)
{
    if (distance == 0 || distance > this->outputPosition)
    {
        return LzoStatus::LookbehindOverrun;
    }

    if (count > this->output.size() - this->outputPosition)
    {
        return LzoStatus::OutputOverrun;
    }

    for (u32 i = 0; i < count; ++i)
    {
        this->output[this->outputPosition] = this->output[this->outputPosition - distance];
        ++this->outputPosition;
    }

    return LzoStatus::Ok;
}

// The low two bits of the byte two before the read position give the literal count after a match.
u32 LzoDecoder::TrailingLiterals() const
{
    return this->input[this->inputPosition - kOffsetBytes] & kTrailingMask;
}

// swd32.exe 0x00439C20: lzo1x_decompress_asm_fast, called by nothing: the output of Run (the original copies in dwords
// without checking the ends, so it may write up to three bytes past the output)
LzoStatus LzoDecoder::RunFast()
{
    return this->Run();
}

// swd32.exe 0x004399E0: lzo1x_decompress
LzoStatus LzoDecoder::Run()
{
    this->inputPosition = 0;
    this->outputPosition = 0;
    u32 instruction = 0;
    Step step = Step::Instruction;

    if (!this->input.empty() && this->input[0] > kFirstLiteralBias)
    {
        const u32 count = this->input[0] - kFirstLiteralBias;
        this->inputPosition = 1;

        if (const LzoStatus status = this->CopyLiterals(count); status != LzoStatus::Ok)
        {
            return status;
        }

        if (count < kShortFirstRun)
        {
            if (!this->ReadByte(instruction))
            {
                return LzoStatus::InputOverrun;
            }

            step = Step::Match;
        }
        else
        {
            step = Step::AfterLiteralRun;
        }
    }

    while (true)
    {
        switch (step)
        {
            case Step::Instruction:
            {
                if (!this->ReadByte(instruction))
                {
                    return LzoStatus::InputOverrun;
                }

                if (instruction >= kM4Marker)
                {
                    step = Step::Match;
                    break;
                }

                u32 count = instruction;

                if (count == 0 && !this->ReadLength(kLiteralLengthMask, count))
                {
                    return LzoStatus::InputOverrun;
                }

                if (const LzoStatus status = this->CopyLiterals(count + kMinLiteralRun); status != LzoStatus::Ok)
                {
                    return status;
                }

                step = Step::AfterLiteralRun;
                break;
            }

            case Step::AfterLiteralRun:
            {
                if (!this->ReadByte(instruction))
                {
                    return LzoStatus::InputOverrun;
                }

                if (instruction >= kM4Marker)
                {
                    step = Step::Match;
                    break;
                }

                u32 high = 0;

                if (!this->ReadByte(high))
                {
                    return LzoStatus::InputOverrun;
                }

                const std::size_t distance =
                    1 + kShortMatchDistance + (instruction >> kDistanceLowBits) + (high << kDistanceLowBits);

                if (const LzoStatus status = this->CopyMatch(distance, kM1Length); status != LzoStatus::Ok)
                {
                    return status;
                }

                step = Step::MatchDone;
                break;
            }

            case Step::Match:
            {
                std::size_t distance = 0;
                u32 count = 0;

                if (instruction >= kM2Marker)
                {
                    u32 high = 0;

                    if (!this->ReadByte(high))
                    {
                        return LzoStatus::InputOverrun;
                    }

                    distance = 1 + ((instruction >> kDistanceLowBits) & kM2DistanceMask) + (high << kM2HighShift);
                    count = (instruction >> kM2LengthShift) + 1;
                }
                else if (instruction >= kM3Marker)
                {
                    count = instruction & kM3LengthMask;

                    if (count == 0 && !this->ReadLength(kM3LengthMask, count))
                    {
                        return LzoStatus::InputOverrun;
                    }

                    u32 low = 0;
                    u32 high = 0;

                    if (!this->ReadByte(low) || !this->ReadByte(high))
                    {
                        return LzoStatus::InputOverrun;
                    }

                    distance = 1 + ((low | (high << CHAR_BIT)) >> kDistanceLowBits);
                    count += kMinMatchExtra;
                }
                else if (instruction >= kM4Marker)
                {
                    std::size_t far = (instruction & kM4FarBit) << kM4FarShift;
                    count = instruction & kM4LengthMask;

                    if (count == 0 && !this->ReadLength(kM4LengthMask, count))
                    {
                        return LzoStatus::InputOverrun;
                    }

                    u32 low = 0;
                    u32 high = 0;

                    if (!this->ReadByte(low) || !this->ReadByte(high))
                    {
                        return LzoStatus::InputOverrun;
                    }

                    far += (low | (high << CHAR_BIT)) >> kDistanceLowBits;

                    if (far == 0)
                    {
                        if (this->inputPosition == this->input.size())
                        {
                            return LzoStatus::Ok;
                        }

                        return this->inputPosition < this->input.size() ? LzoStatus::InputNotConsumed
                                                                        : LzoStatus::InputOverrun;
                    }

                    distance = far + kFarMatchBase;
                    count += kMinMatchExtra;
                }
                else
                {
                    u32 high = 0;

                    if (!this->ReadByte(high))
                    {
                        return LzoStatus::InputOverrun;
                    }

                    distance = 1 + (instruction >> kDistanceLowBits) + (high << kDistanceLowBits);
                    count = kShortMatchLength;
                }

                if (const LzoStatus status = this->CopyMatch(distance, count); status != LzoStatus::Ok)
                {
                    return status;
                }

                step = Step::MatchDone;
                break;
            }

            case Step::MatchDone:
            {
                const u32 count = this->TrailingLiterals();

                if (count == 0)
                {
                    step = Step::Instruction;
                    break;
                }

                if (const LzoStatus status = this->CopyLiterals(count); status != LzoStatus::Ok)
                {
                    return status;
                }

                if (!this->ReadByte(instruction))
                {
                    return LzoStatus::InputOverrun;
                }

                step = Step::Match;
                break;
            }
        }
    }
}

namespace
{

constexpr std::size_t kMinCompress = 0xD;
constexpr std::size_t kTailMargin = 0xD;
constexpr u32 kMaxOffset = 0xBFFF;
constexpr u32 kM3MaxLength = 0x21;
constexpr u32 kM4MaxLength = 9;
constexpr u32 kSecondaryProbe = 0x1F;  // the second dictionary slot of a hash
constexpr u32 kSecondaryMask = 0x7FF;
constexpr std::size_t kFirstProbe = 4;  // the first bytes are never matched
constexpr std::size_t kHashBytes = 4;
constexpr u32 kHashMultiplier = 0x21;
constexpr u32 kHashStep = 0x20;
constexpr int kHashFourthShift = 6;
constexpr int kHashShift = 5;

// Appends a length beyond the bits of the opcode: a zero byte per 255 and the rest (the zeros add one each to the
// final byte, as the original's wrap-around count does).
void PushLongLength(std::vector<u8>& output, u32 length)
{
    u8 last = static_cast<u8>(length);

    if (length > kLengthByte)
    {
        const u32 zeros = (length - 1) / kLengthByte;
        output.insert(output.end(), zeros, 0);
        last = static_cast<u8>(last + zeros);
    }

    output.push_back(last);
}

void PushOffset(std::vector<u8>& output, u32 offset)
{
    output.push_back(static_cast<u8>(offset << kDistanceLowBits));
    output.push_back(static_cast<u8>(offset >> (CHAR_BIT - kDistanceLowBits)));
}

}  // namespace

LzoEncoder::LzoEncoder(int dictionaryBits)
    : dictionaryBits(dictionaryBits)
{
}

// The literals before a match or at the end: up to 3 go into the low bits of the opcode two bytes back, up to 18 take
// one length byte, more a zero byte and a long length.
void LzoEncoder::EmitLiterals(std::vector<u8>& output, std::span<const u8> literals)
{
    const auto count = static_cast<u32>(literals.size());

    if (count <= kMaxPackedLiterals)
    {
        output[output.size() - kOffsetBytes] |= static_cast<u8>(count);
    }
    else if (count <= kMaxShortLiterals)
    {
        output.push_back(static_cast<u8>(count - kMinLiteralRun));
    }
    else
    {
        output.push_back(0);
        PushLongLength(output, count - kMaxShortLiterals);
    }

    output.insert(output.end(), literals.begin(), literals.end());
}

// swd32.exe 0x00439210 (14-bit dictionary)
// swd32.exe 0x00439670 (15-bit dictionary)
// The core loop: hashes four bytes into the dictionary (a second probe at a fixed alternative slot for far offsets),
// emits the literals before every match of three or more bytes and the match (M2 up to 8 bytes and 0x800 back, M3 up
// to 0x4000, M4 beyond); stops 13 bytes before the end. Returns the count of trailing literals. The dictionary is
// uninitialised heap memory in the original (debug fill, pointing outside the input); here it starts invalid.
std::size_t LzoEncoder::CompressCore(std::span<const u8> input, std::vector<u8>& output)
{
    const std::size_t size = input.size();
    const std::size_t limit = size - kTailMargin;
    const u32 mask = (1u << this->dictionaryBits) - 1;
    const u32 secondary = (1u << (this->dictionaryBits - 1)) | kSecondaryProbe;
    std::size_t literal = 0;
    std::size_t ip = kFirstProbe;

    do
    {
        for (;;)
        {
            const u32 fourth = input[ip + kHashBytes - 1];
            const u32 hash =
                kHashMultiplier *
                (input[ip] ^
                 (kHashStep * (input[ip + 1] ^ (kHashStep * (input[ip + 2] ^ (fourth << kHashFourthShift))))));
            u32 index = (hash >> kHashShift) & mask;
            i64 match = this->dictionary[index];
            bool found = false;
            u32 offset = 0;

            if (match >= 0)
            {
                offset = static_cast<u32>(static_cast<i64>(ip) - match);

                if (static_cast<i64>(ip) != match && offset <= kMaxOffset)
                {
                    if (offset <= kM2MaxOffset || input[static_cast<std::size_t>(match) + kHashBytes - 1] == fourth)
                    {
                        found = true;
                    }
                    else
                    {
                        index = ((hash >> kHashShift) & kSecondaryMask) ^ secondary;
                        match = this->dictionary[index];

                        if (match >= 0)
                        {
                            offset = static_cast<u32>(static_cast<i64>(ip) - match);
                            found = static_cast<i64>(ip) != match && offset <= kMaxOffset &&
                                    (offset <= kM2MaxOffset ||
                                     input[static_cast<std::size_t>(match) + kHashBytes - 1] == fourth);
                        }
                    }
                }
            }

            const auto from = static_cast<std::size_t>(match);

            if (!found || input[from] != input[ip] || input[from + 1] != input[ip + 1] ||
                input[from + 2] != input[ip + 2])
            {
                this->dictionary[index] = static_cast<i64>(ip);
                ++ip;
                break;
            }

            this->dictionary[index] = static_cast<i64>(ip);

            if (ip > literal)
            {
                this->EmitLiterals(output, input.subspan(literal, ip - literal));
                literal = ip;
            }

            std::size_t end = ip + kM1Length;
            bool longMatch = true;

            for (std::size_t k = kM1Length; k <= kM2MaxLength; ++k)
            {
                ++end;

                if (input[from + k] != input[ip + k])
                {
                    longMatch = false;
                    break;
                }
            }

            if (longMatch)
            {
                std::size_t at = from + kM2MaxLength + 1;

                while (end < size && input[at] == input[end])
                {
                    ++end;
                    ++at;
                }

                const auto length = static_cast<u32>(end - literal);

                if (offset > kM3MaxOffset)
                {
                    const u32 far = offset - kM3MaxOffset;

                    if (length <= kM4MaxLength)
                    {
                        output.push_back(
                            static_cast<u8>((length - kMinMatchExtra) | ((far >> kM4FarShift) & kM4FarBit) | kM4Marker)
                        );
                    }
                    else
                    {
                        output.push_back(static_cast<u8>(((far >> kM4FarShift) & kM4FarBit) | kM4Marker));
                        PushLongLength(output, length - kM4MaxLength);
                    }

                    PushOffset(output, far);
                }
                else
                {
                    const u32 near = offset - 1;

                    if (length <= kM3MaxLength)
                    {
                        output.push_back(static_cast<u8>((length - kMinMatchExtra) | kM3Marker));
                    }
                    else
                    {
                        output.push_back(kM3Marker);
                        PushLongLength(output, length - kM3MaxLength);
                    }

                    PushOffset(output, near);
                }
            }
            else
            {
                --end;
                const auto length = static_cast<u8>(end - literal);

                if (offset > kM2MaxOffset)
                {
                    if (offset > kM3MaxOffset)
                    {
                        const u32 far = offset - kM3MaxOffset;
                        output.push_back(
                            static_cast<u8>((length - kMinMatchExtra) | ((far >> kM4FarShift) & kM4FarBit) | kM4Marker)
                        );
                        PushOffset(output, far);
                    }
                    else
                    {
                        output.push_back(static_cast<u8>((length - kMinMatchExtra) | kM3Marker));
                        PushOffset(output, offset - 1);
                    }
                }
                else
                {
                    const u32 near = offset - 1;
                    output.push_back(
                        static_cast<u8>(
                            ((near & kM2DistanceMask) << kDistanceLowBits) | ((length - 1) << kM2LengthShift)
                        )
                    );
                    output.push_back(static_cast<u8>(near >> kM2HighShift));
                }
            }

            ip = end;
            literal = ip;

            if (ip >= limit)
            {
                return size - literal;
            }
        }
    } while (ip < limit);

    return size - literal;
}

// swd32.exe 0x00439120 (14-bit dictionary)
// swd32.exe 0x00439580 (15-bit dictionary)
// The whole block: the core for inputs above 13 bytes, then the trailing literals (a first literal run of at most 238
// bytes in one opcode) and the end marker 0x11 0 0. The callers' output buffers may be smaller than the worst case and
// are written past for incompressible data; here the output grows as needed.
std::vector<u8> LzoEncoder::Compress(std::span<const u8> input)
{
    this->dictionary.assign(static_cast<std::size_t>(1) << this->dictionaryBits, -1);
    std::vector<u8> output;
    std::size_t trailing = input.size();

    if (input.size() > kMinCompress)
    {
        trailing = this->CompressCore(input, output);
    }

    if (trailing != 0)
    {
        const std::span<const u8> literals = input.subspan(input.size() - trailing);

        if (output.empty() && trailing <= kMaxFirstRun)
        {
            output.push_back(static_cast<u8>(trailing + kFirstLiteralBias));
            output.insert(output.end(), literals.begin(), literals.end());
        }
        else
        {
            this->EmitLiterals(output, literals);
        }
    }

    output.push_back(kEndMarker);
    output.push_back(0);
    output.push_back(0);
    return output;
}

}  // namespace swd3
