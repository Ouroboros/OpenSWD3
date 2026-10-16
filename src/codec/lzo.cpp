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

LzoDecoder::LzoDecoder(std::span<const std::uint8_t> input, std::span<std::uint8_t> output)
    : input(input), output(output)
{
}

std::size_t LzoDecoder::Written() const
{
    return this->outputPosition;
}

bool LzoDecoder::ReadByte(std::uint32_t& value)
{
    if (this->inputPosition >= this->input.size())
    {
        return false;
    }

    value = this->input[this->inputPosition++];
    return true;
}

// A run length: `base` plus 255 for every zero byte, plus the first non-zero byte.
bool LzoDecoder::ReadLength(std::uint32_t base, std::uint32_t& length)
{
    std::uint32_t value = 0;
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

LzoStatus LzoDecoder::CopyLiterals(std::uint32_t count)
{
    if (count > this->input.size() - this->inputPosition)
    {
        return LzoStatus::InputOverrun;
    }

    if (count > this->output.size() - this->outputPosition)
    {
        return LzoStatus::OutputOverrun;
    }

    for (std::uint32_t i = 0; i < count; ++i)
    {
        this->output[this->outputPosition++] = this->input[this->inputPosition++];
    }

    return LzoStatus::Ok;
}

// Byte by byte, so that a distance shorter than the count repeats the bytes just written.
LzoStatus LzoDecoder::CopyMatch(std::size_t distance, std::uint32_t count)
{
    if (distance == 0 || distance > this->outputPosition)
    {
        return LzoStatus::LookbehindOverrun;
    }

    if (count > this->output.size() - this->outputPosition)
    {
        return LzoStatus::OutputOverrun;
    }

    for (std::uint32_t i = 0; i < count; ++i)
    {
        this->output[this->outputPosition] = this->output[this->outputPosition - distance];
        ++this->outputPosition;
    }

    return LzoStatus::Ok;
}

// The low two bits of the byte two before the read position give the literal count after a match.
std::uint32_t LzoDecoder::TrailingLiterals() const
{
    return this->input[this->inputPosition - 2] & 3u;
}

// swd32.exe 0x004399E0: lzo1x_decompress
LzoStatus LzoDecoder::Run()
{
    this->inputPosition = 0;
    this->outputPosition = 0;
    std::uint32_t instruction = 0;
    Step step = Step::Instruction;

    if (!this->input.empty() && this->input[0] > 17)
    {
        const std::uint32_t count = this->input[0] - 17u;
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

                std::uint32_t count = instruction;

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

                std::uint32_t high = 0;

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
                std::uint32_t count = 0;

                if (instruction >= 64)
                {
                    std::uint32_t high = 0;

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

                    std::uint32_t low = 0;
                    std::uint32_t high = 0;

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

                    std::uint32_t low = 0;
                    std::uint32_t high = 0;

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
                    std::uint32_t high = 0;

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
                const std::uint32_t count = this->TrailingLiterals();

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

}  // namespace swd3
