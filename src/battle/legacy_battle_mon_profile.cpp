#include "openswd3/battle/legacy_battle_mon_profile.hpp"

#include <array>
#include <bit>
#include <stdexcept>

namespace openswd3::battle {
namespace {

using compat::u8;
using compat::u16;
using compat::u32;

[[nodiscard]] u32 read_dword(const std::span<const u8> bytes) noexcept {
    return static_cast<u32>(bytes[0U]) | (static_cast<u32>(bytes[1U]) << 8U) |
        (static_cast<u32>(bytes[2U]) << 16U) |
        (static_cast<u32>(bytes[3U]) << 24U);
}

void write_stale_dword(std::array<u8, 4U>& bytes, const u32 value) noexcept {
    bytes[0U] = static_cast<u8>(value);
    bytes[1U] = static_cast<u8>(value >> 8U);
    bytes[2U] = static_cast<u8>(value >> 16U);
    bytes[3U] = static_cast<u8>(value >> 24U);
}

class ProfileParser final {
public:
    ProfileParser(
        const std::span<const u8> stream,
        const std::span<std::byte> output,
        LegacyBattleMonProfileLoadResult& result
    ) noexcept
        : stream_(stream), output_(output), result_(result) {}

    [[nodiscard]] bool parse() noexcept {
        while (true) {
            u16 tag = 0U;
            if (!read_stream_word(cursor_, tag)) {
                return false;
            }
            cursor_ += 2U;
            if (tag == 5U) {
                return true;
            }
            if (tag > 25U || tag == 1U) {
                continue;
            }

            switch (tag) {
            case 0U:
                if (!parse_pair()) {
                    return false;
                }
                break;

            case 2U:
                if (!parse_byte_24()) {
                    return false;
                }
                break;

            case 3U:
                if (!or_output_dword(0x04U, 0x00000001U)) {
                    return false;
                }
                break;

            case 4U:
                if (!parse_word(0x16U)) {
                    return false;
                }
                break;

            case 6U:
                if (!parse_word_18_with_flag()) {
                    return false;
                }
                break;

            case 7U:
                if (!parse_word(0x18U)) {
                    return false;
                }
                break;

            case 8U:
                if (!parse_word(0x14U)) {
                    return false;
                }
                break;

            case 9U:
                if (!or_output_dword(0x04U, 0x00000002U)) {
                    return false;
                }
                break;

            case 10U:
                if (!parse_word_1e_with_high_bit()) {
                    return false;
                }
                break;

            case 11U:
                if (!or_output_dword(0x04U, 0x00000004U)) {
                    return false;
                }
                break;

            case 12U:
                if (!parse_dword(0x08U)) {
                    return false;
                }
                break;

            case 13U:
                if (!parse_word(0x1CU)) {
                    return false;
                }
                break;

            case 14U:
                if (!parse_word_1a_with_flag()) {
                    return false;
                }
                break;

            case 15U:
                if (!or_output_dword(0x04U, 0x00000010U)) {
                    return false;
                }
                break;

            case 16U:
                if (!or_output_dword(0x04U, 0x00000020U)) {
                    return false;
                }
                break;

            case 17U:
                if (!or_output_dword(0x04U, 0x00000040U)) {
                    return false;
                }
                break;

            case 18U:
                if (!or_output_flags(0x00000100U)) {
                    return false;
                }
                break;

            case 19U:
                if (!or_output_flags(0x00000200U)) {
                    return false;
                }
                break;

            case 20U:
                if (!or_output_flags(0x00000400U)) {
                    return false;
                }
                break;

            case 21U:
                if (!or_output_flags(0x00000800U)) {
                    return false;
                }
                break;

            case 22U:
                if (!parse_word_and_byte_with_flag()) {
                    return false;
                }
                break;

            case 23U:
                if (!parse_word(0x22U)) {
                    return false;
                }
                break;

            case 24U:
                if (!or_output_flags(0x00002000U)) {
                    return false;
                }
                break;

            case 25U:
                if (!or_output_flags(0x00004000U)) {
                    return false;
                }
                break;

            case 1U:
            case 5U:
            default:
                break;
            }
        }
    }

    [[nodiscard]] u32 cursor() const noexcept {
        return static_cast<u32>(cursor_);
    }

private:
    [[nodiscard]] bool stream_available(
        const std::size_t offset, const std::size_t size
    ) noexcept {
        if (offset <= stream_.size() && size <= stream_.size() - offset) {
            return true;
        }
        result_.status =
            LegacyBattleMonProfileLoadStatus::stream_access_typed_stop;
        result_.stopped_stream_offset = static_cast<u32>(offset);
        return false;
    }

    [[nodiscard]] bool output_available(
        const std::size_t offset, const std::size_t size
    ) noexcept {
        if (offset <= output_.size() && size <= output_.size() - offset) {
            return true;
        }
        result_.status =
            LegacyBattleMonProfileLoadStatus::output_access_typed_stop;
        result_.stopped_output_offset = static_cast<u32>(offset);
        return false;
    }

    [[nodiscard]] bool
    read_stream_byte(const std::size_t offset, u8& value) noexcept {
        if (!stream_available(offset, 1U)) {
            return false;
        }
        value = stream_[offset];
        return true;
    }

    [[nodiscard]] bool
    read_stream_word(const std::size_t offset, u16& value) noexcept {
        if (!stream_available(offset, 2U)) {
            return false;
        }
        value = static_cast<u16>(stream_[offset]) |
            static_cast<u16>(static_cast<u16>(stream_[offset + 1U]) << 8U);
        return true;
    }

    [[nodiscard]] bool
    read_stream_dword(const std::size_t offset, u32& value) noexcept {
        if (!stream_available(offset, 4U)) {
            return false;
        }
        value = read_dword(stream_.subspan(offset, 4U));
        return true;
    }

    [[nodiscard]] bool
    read_output_dword(const std::size_t offset, u32& value) noexcept {
        if (!output_available(offset, 4U)) {
            return false;
        }
        value = std::to_integer<u32>(output_[offset]) |
            (std::to_integer<u32>(output_[offset + 1U]) << 8U) |
            (std::to_integer<u32>(output_[offset + 2U]) << 16U) |
            (std::to_integer<u32>(output_[offset + 3U]) << 24U);
        return true;
    }

    [[nodiscard]] bool
    write_output_byte(const std::size_t offset, const u8 value) noexcept {
        if (!output_available(offset, 1U)) {
            return false;
        }
        output_[offset] = static_cast<std::byte>(value);
        return true;
    }

    [[nodiscard]] bool
    write_output_word(const std::size_t offset, const u16 value) noexcept {
        if (!output_available(offset, 2U)) {
            return false;
        }
        output_[offset] = static_cast<std::byte>(value);
        output_[offset + 1U] = static_cast<std::byte>(value >> 8U);
        return true;
    }

    [[nodiscard]] bool
    write_output_dword(const std::size_t offset, const u32 value) noexcept {
        if (!output_available(offset, 4U)) {
            return false;
        }
        output_[offset] = static_cast<std::byte>(value);
        output_[offset + 1U] = static_cast<std::byte>(value >> 8U);
        output_[offset + 2U] = static_cast<std::byte>(value >> 16U);
        output_[offset + 3U] = static_cast<std::byte>(value >> 24U);
        return true;
    }

    [[nodiscard]] bool
    or_output_dword(const std::size_t offset, const u32 mask) noexcept {
        u32 value = 0U;
        if (!read_output_dword(offset, value)) {
            return false;
        }
        return write_output_dword(offset, value | mask);
    }

    [[nodiscard]] bool or_output_flags(const u32 mask) noexcept {
        return or_output_dword(0x04U, mask);
    }

    [[nodiscard]] bool parse_pair() noexcept {
        u32 first_value = 0U;
        if (!read_stream_dword(cursor_, first_value)) {
            return false;
        }

        cursor_ += 8U;
        if (!write_output_dword(0x0CU, first_value)) {
            return false;
        }

        u32 second_value = 0U;
        if (!read_stream_dword(cursor_ - 4U, second_value)) {
            return false;
        }

        return write_output_dword(0x10U, second_value);
    }

    [[nodiscard]] bool parse_byte_24() noexcept {
        u8 value = 0U;
        if (!read_stream_byte(cursor_, value)) {
            return false;
        }

        cursor_ += 2U;
        return write_output_byte(0x24U, value);
    }

    [[nodiscard]] bool parse_word(const std::size_t output_offset) noexcept {
        u16 value = 0U;
        if (!read_stream_word(cursor_, value)) {
            return false;
        }

        cursor_ += 2U;
        return write_output_word(output_offset, value);
    }

    [[nodiscard]] bool parse_word_18_with_flag() noexcept {
        return parse_word(0x18U) && or_output_flags(0x00000080U);
    }

    [[nodiscard]] bool parse_word_1e_with_high_bit() noexcept {
        if (!parse_word(0x1EU) || !output_available(0x1FU, 1U)) {
            return false;
        }

        const u8 value = std::to_integer<u8>(output_[0x1FU]);
        output_[0x1FU] = static_cast<std::byte>(value | 0x80U);
        return true;
    }

    [[nodiscard]] bool parse_dword(const std::size_t output_offset) noexcept {
        u32 value = 0U;
        if (!read_stream_dword(cursor_, value)) {
            return false;
        }

        cursor_ += 4U;
        return write_output_dword(output_offset, value);
    }

    [[nodiscard]] bool parse_word_1a_with_flag() noexcept {
        if (!or_output_flags(0x00000008U)) {
            return false;
        }

        u16 value = 0U;
        if (!read_stream_word(cursor_, value) ||
            !write_output_word(0x1AU, value)) {
            return false;
        }

        cursor_ += 2U;
        return true;
    }

    [[nodiscard]] bool parse_word_and_byte_with_flag() noexcept {
        u32 flags = 0U;
        if (!read_output_dword(0x04U, flags)) {
            return false;
        }

        cursor_ += 4U;
        if (!write_output_dword(0x04U, flags | 0x00001000U)) {
            return false;
        }

        u16 word = 0U;
        if (!read_stream_word(cursor_ - 4U, word) ||
            !write_output_word(0x20U, word)) {
            return false;
        }

        u8 byte = 0U;
        if (!read_stream_byte(cursor_ - 2U, byte)) {
            return false;
        }

        return write_output_byte(0x24U, byte);
    }

    std::span<const u8> stream_;
    std::span<std::byte> output_;
    std::size_t cursor_{};
    LegacyBattleMonProfileLoadResult& result_;
};

}  // namespace

LegacyBattleMonDatabaseState&
LegacyBattleMonDatabasePort::legacy_battle_mon_database_state() noexcept {
    return mon_database_state_;
}

LegacyBattleMonProfile&
LegacyBattleMonDatabasePort::legacy_battle_mon_profile_scratch() noexcept {
    return mon_profile_scratch_;
}

std::array<compat::u8, kLegacyBattleMonDefinitionScratchBytes>&
LegacyBattleMonDatabasePort::legacy_battle_mon_definition_scratch() noexcept {
    return mon_definition_scratch_;
}

LegacyBattleMonText& LegacyBattleMonDatabasePort::
    legacy_battle_mon_definition_scratch_description() noexcept {
    return mon_definition_scratch_description_;
}

compat::u32
LegacyBattleMonDatabasePort::open_mon_file(const std::filesystem::path&) {
    throw std::logic_error("MON file access is not bound");
}

compat::u32 LegacyBattleMonDatabasePort::seek_mon_file(
    compat::u32, compat::i32, LegacyBattleMonSeekOrigin
) {
    throw std::logic_error("MON file access is not bound");
}

LegacyBattleMonReadResult LegacyBattleMonDatabasePort::read_mon_file(
    compat::u32, std::span<compat::u8>, compat::u32
) {
    throw std::logic_error("MON file access is not bound");
}

LegacyBattleMonStreamAllocation
LegacyBattleMonDatabasePort::allocate_mon_stream(compat::u32) {
    throw std::logic_error("MON stream storage is not bound");
}

void LegacyBattleMonDatabasePort::release_mon_stream(compat::u32) {
    throw std::logic_error("MON stream storage is not bound");
}

compat::u32 LegacyBattleMonDatabasePort::mon_text_size(compat::u32) {
    throw std::logic_error("MON text storage is not bound");
}

LegacyBattleMonTextAllocation
LegacyBattleMonDatabasePort::allocate_mon_text(compat::u32) {
    throw std::logic_error("MON text storage is not bound");
}

void LegacyBattleMonDatabasePort::release_mon_text(compat::u32) {
    throw std::logic_error("MON text storage is not bound");
}

LegacyBattleMonProfileLoadResult load_legacy_battle_mon_profile(
    const std::span<std::byte> output,
    LegacyBattleMonDatabasePort& port,
    const LegacyBattleMonProfileLoadRequest& request
) {
    LegacyBattleMonProfileLoadResult result;
    auto& database = port.legacy_battle_mon_database_state();
    if (!database.open) {
        database.handle = port.open_mon_file(request.path);
        ++result.open_calls;
        result.handle = database.handle;
        if (database.handle == 0xFFFFFFFFU) {
            result.status = LegacyBattleMonProfileLoadStatus::open_failed;
            return result;
        }

        database.open = true;
    } else {
        result.handle = database.handle;
    }

    static_cast<void>(port.seek_mon_file(
        database.handle, 0x204, LegacyBattleMonSeekOrigin::begin
    ));
    ++result.seek_calls;
    std::array<u8, 4U> root_bytes{};
    write_stale_dword(root_bytes, request.stale_root_buffer_value);
    static_cast<void>(port.read_mon_file(database.handle, root_bytes, 4U));
    ++result.read_calls;

    result.profile_id = request.profile_id & 0xFFFFU;
    result.auxiliary_root = read_dword(root_bytes);
    const u32 profile_offset_entry =
        result.auxiliary_root + result.profile_id * 4U + 0x200U;
    static_cast<void>(port.seek_mon_file(
        database.handle,
        std::bit_cast<compat::i32>(profile_offset_entry),
        LegacyBattleMonSeekOrigin::begin
    ));
    ++result.seek_calls;
    std::array<u8, 4U> relative_bytes = root_bytes;
    static_cast<void>(port.read_mon_file(database.handle, relative_bytes, 4U));
    ++result.read_calls;

    result.profile_relative_offset = read_dword(relative_bytes);
    result.profile_file_offset = result.profile_relative_offset + 0x200U;
    static_cast<void>(port.seek_mon_file(
        database.handle,
        std::bit_cast<compat::i32>(result.profile_file_offset),
        LegacyBattleMonSeekOrigin::begin
    ));
    ++result.seek_calls;

    const auto allocation =
        port.allocate_mon_stream(kLegacyBattleMonStreamBytes);
    ++result.allocation_calls;
    result.stream_token = allocation.block_token;
    if (result.stream_token == 0U) {
        result.status =
            LegacyBattleMonProfileLoadStatus::stream_zero_typed_stop;
        return result;
    }

    for (u32 offset = 0U; offset < kLegacyBattleMonStreamBytes; offset += 4U) {
        if (offset > allocation.bytes.size() ||
            allocation.bytes.size() - offset < 4U) {
            result.status =
                LegacyBattleMonProfileLoadStatus::stream_access_typed_stop;
            result.stopped_stream_offset = offset;
            return result;
        }

        for (auto& byte : allocation.bytes.subspan(offset, 4U)) {
            byte = 0U;
        }
    }

    const auto stream = allocation.bytes.first(kLegacyBattleMonStreamBytes);
    static_cast<void>(
        port.read_mon_file(database.handle, stream, kLegacyBattleMonStreamBytes)
    );
    ++result.read_calls;
    if ((static_cast<u16>(stream[0U]) |
         static_cast<u16>(static_cast<u16>(stream[1U]) << 8U)) != 0U) {
        port.release_mon_stream(result.stream_token);
        ++result.release_calls;
        return result;
    }

    ProfileParser parser(stream, output, result);
    if (!parser.parse()) {
        result.stream_cursor = parser.cursor();
        return result;
    }

    result.stream_cursor = parser.cursor();
    port.release_mon_stream(result.stream_token);
    ++result.release_calls;
    result.profile_found = true;
    return result;
}

}  // namespace openswd3::battle
