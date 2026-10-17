#pragma once

#include <climits>
#include <cstddef>
#include <span>

#include "core/types.h"

namespace swd3
{

// Little-endian fields read in order from a byte buffer; bytes past its end read as 0.
class ByteReader
{
public:
    explicit ByteReader(std::span<const u8> bytes)
        : bytes(bytes)
    {
    }

    u8 Byte()
    {
        const u8 value = this->at < this->bytes.size() ? this->bytes[this->at] : u8{0};
        ++this->at;
        return value;
    }

    u16 Word16()
    {
        const u16 low = this->Byte();
        return static_cast<u16>(low | this->Byte() << CHAR_BIT);
    }

    u32 Word32()
    {
        const u32 low = this->Word16();
        return low | static_cast<u32>(this->Word16()) << (CHAR_BIT * sizeof(u16));
    }

    void Skip(std::size_t count)
    {
        this->at += count;
    }

    // The word at `at`, 0 unless all of it is inside `bytes`.
    static u16 Word16At(std::span<const u8> bytes, std::size_t at)
    {
        if (at + sizeof(u16) > bytes.size())
        {
            return 0;
        }

        ByteReader reader(bytes.subspan(at));
        return reader.Word16();
    }

    // The dword at `at`, 0 unless all of it is inside `bytes`.
    static u32 Word32At(std::span<const u8> bytes, std::size_t at)
    {
        if (at + sizeof(u32) > bytes.size())
        {
            return 0;
        }

        ByteReader reader(bytes.subspan(at));
        return reader.Word32();
    }

private:
    std::span<const u8> bytes;
    std::size_t at = 0;
};

}  // namespace swd3
