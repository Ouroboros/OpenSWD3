#include "codec/lzo.h"

namespace swd3
{

namespace
{

// Largest distance of the two-byte match form (M2_MAX_OFFSET).
constexpr std::size_t kShortMatchDistance = 0x800;
// Distance offset of the 16..31 instructions (M3_MAX_OFFSET + 1 style window base).
constexpr std::size_t kFarMatchBase = 0x4000;

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

        length += 255;
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
    return this->input[this->inputPosition - 2] & 3u;
}

// swd32.exe 0x004399E0: lzo1x_decompress
LzoStatus LzoDecoder::Run()
{
    this->inputPosition = 0;
    this->outputPosition = 0;
    u32 instruction = 0;
    Step step = Step::Instruction;

    if (!this->input.empty() && this->input[0] > 17)
    {
        const u32 count = this->input[0] - 17u;
        this->inputPosition = 1;

        if (const LzoStatus status = this->CopyLiterals(count); status != LzoStatus::Ok)
        {
            return status;
        }

        if (count < 4)
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

                if (instruction >= 16)
                {
                    step = Step::Match;
                    break;
                }

                u32 count = instruction;

                if (count == 0 && !this->ReadLength(15, count))
                {
                    return LzoStatus::InputOverrun;
                }

                if (const LzoStatus status = this->CopyLiterals(count + 3); status != LzoStatus::Ok)
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

                if (instruction >= 16)
                {
                    step = Step::Match;
                    break;
                }

                u32 high = 0;

                if (!this->ReadByte(high))
                {
                    return LzoStatus::InputOverrun;
                }

                const std::size_t distance = 1 + kShortMatchDistance + (instruction >> 2) + (high << 2);

                if (const LzoStatus status = this->CopyMatch(distance, 3); status != LzoStatus::Ok)
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

                if (instruction >= 64)
                {
                    u32 high = 0;

                    if (!this->ReadByte(high))
                    {
                        return LzoStatus::InputOverrun;
                    }

                    distance = 1 + ((instruction >> 2) & 7) + (high << 3);
                    count = (instruction >> 5) + 1;
                }
                else if (instruction >= 32)
                {
                    count = instruction & 31;

                    if (count == 0 && !this->ReadLength(31, count))
                    {
                        return LzoStatus::InputOverrun;
                    }

                    u32 low = 0;
                    u32 high = 0;

                    if (!this->ReadByte(low) || !this->ReadByte(high))
                    {
                        return LzoStatus::InputOverrun;
                    }

                    distance = 1 + ((low | (high << 8)) >> 2);
                    count += 2;
                }
                else if (instruction >= 16)
                {
                    std::size_t far = (instruction & 8) << 11;
                    count = instruction & 7;

                    if (count == 0 && !this->ReadLength(7, count))
                    {
                        return LzoStatus::InputOverrun;
                    }

                    u32 low = 0;
                    u32 high = 0;

                    if (!this->ReadByte(low) || !this->ReadByte(high))
                    {
                        return LzoStatus::InputOverrun;
                    }

                    far += (low | (high << 8)) >> 2;

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
                    count += 2;
                }
                else
                {
                    u32 high = 0;

                    if (!this->ReadByte(high))
                    {
                        return LzoStatus::InputOverrun;
                    }

                    distance = 1 + (instruction >> 2) + (high << 2);
                    count = 2;
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
constexpr u32 kM2MaxOffset = 0x800;
constexpr u32 kM3MaxOffset = 0x4000;
constexpr u32 kMaxOffset = 0xBFFF;
constexpr u32 kM3MaxLength = 0x21;
constexpr u32 kM4MaxLength = 9;
constexpr u8 kM3Marker = 0x20;
constexpr u8 kM4Marker = 0x10;

// Appends a length beyond the bits of the opcode: a zero byte per 255 and the rest (the zeros add one each to the
// final byte, as the original's wrap-around count does).
void PushLongLength(std::vector<u8>& output, u32 length)
{
    u8 last = static_cast<u8>(length);

    if (length > 0xFF)
    {
        const u32 zeros = (length - 1) / 0xFF;
        output.insert(output.end(), zeros, 0);
        last = static_cast<u8>(last + zeros);
    }

    output.push_back(last);
}

void PushOffset(std::vector<u8>& output, u32 offset)
{
    output.push_back(static_cast<u8>(4 * offset));
    output.push_back(static_cast<u8>(offset >> 6));
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

    if (count <= 3)
    {
        output[output.size() - 2] |= static_cast<u8>(count);
    }
    else if (count <= 0x12)
    {
        output.push_back(static_cast<u8>(count - 3));
    }
    else
    {
        output.push_back(0);
        PushLongLength(output, count - 0x12);
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
    const u32 secondary = (1u << (this->dictionaryBits - 1)) | 0x1F;
    std::size_t literal = 0;
    std::size_t ip = 4;

    do
    {
        for (;;)
        {
            const u32 fourth = input[ip + 3];
            const u32 hash = 0x21 * (input[ip] ^ (0x20 * (input[ip + 1] ^ (0x20 * (input[ip + 2] ^ (fourth << 6))))));
            u32 index = (hash >> 5) & mask;
            i64 match = this->dictionary[index];
            bool found = false;
            u32 offset = 0;

            if (match >= 0)
            {
                offset = static_cast<u32>(static_cast<i64>(ip) - match);

                if (static_cast<i64>(ip) != match && offset <= kMaxOffset)
                {
                    if (offset <= kM2MaxOffset || input[static_cast<std::size_t>(match) + 3] == fourth)
                    {
                        found = true;
                    }
                    else
                    {
                        index = ((hash >> 5) & 0x7FF) ^ secondary;
                        match = this->dictionary[index];

                        if (match >= 0)
                        {
                            offset = static_cast<u32>(static_cast<i64>(ip) - match);
                            found = static_cast<i64>(ip) != match && offset <= kMaxOffset &&
                                    (offset <= kM2MaxOffset || input[static_cast<std::size_t>(match) + 3] == fourth);
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

            std::size_t end = ip + 3;
            bool longMatch = true;

            for (std::size_t k = 3; k <= 8; ++k)
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
                std::size_t at = from + 9;

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
                        output.push_back(static_cast<u8>((length - 2) | ((far >> 11) & 8) | kM4Marker));
                    }
                    else
                    {
                        output.push_back(static_cast<u8>(((far >> 11) & 8) | kM4Marker));
                        PushLongLength(output, length - kM4MaxLength);
                    }

                    PushOffset(output, far);
                }
                else
                {
                    const u32 near = offset - 1;

                    if (length <= kM3MaxLength)
                    {
                        output.push_back(static_cast<u8>((length - 2) | kM3Marker));
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
                        output.push_back(static_cast<u8>((length - 2) | ((far >> 11) & 8) | kM4Marker));
                        PushOffset(output, far);
                    }
                    else
                    {
                        output.push_back(static_cast<u8>((length - 2) | kM3Marker));
                        PushOffset(output, offset - 1);
                    }
                }
                else
                {
                    const u32 near = offset - 1;
                    output.push_back(static_cast<u8>((4 * (near & 7)) | (0x20 * (length + 7))));
                    output.push_back(static_cast<u8>(near >> 3));
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

        if (output.empty() && trailing <= 0xEE)
        {
            output.push_back(static_cast<u8>(trailing + 0x11));
            output.insert(output.end(), literals.begin(), literals.end());
        }
        else
        {
            this->EmitLiterals(output, literals);
        }
    }

    output.push_back(0x11);
    output.push_back(0);
    output.push_back(0);
    return output;
}

}  // namespace swd3
