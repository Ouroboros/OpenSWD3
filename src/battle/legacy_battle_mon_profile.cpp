#include "openswd3/battle/legacy_battle_mon_profile.hpp"

#include <array>
#include <bit>

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
        const u32 stream_token,
        const u32 entry_edx,
        LegacyBattleMonProfileLoadResult& result
    ) noexcept
        : stream_(stream), output_(output), stream_token_(stream_token),
          edx_(entry_edx), result_(result) {}

    [[nodiscard]] bool parse() noexcept {
        while (true) {
            u16 tag = 0U;
            if (!read_stream_word(cursor_, tag)) {
                return false;
            }
            edx_ = (edx_ & 0xFFFF0000U) | tag;
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
                if (!or_output_dword_through_edx(0x00000100U)) {
                    return false;
                }
                break;

            case 19U:
                if (!or_output_dword_through_edx(0x00000200U)) {
                    return false;
                }
                break;

            case 20U:
                if (!or_output_dword_through_edx(0x00000400U)) {
                    return false;
                }
                break;

            case 21U:
                if (!or_output_dword_through_edx(0x00000800U)) {
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
                if (!or_output_dword_through_edx(0x00002000U)) {
                    return false;
                }
                break;

            case 25U:
                if (!or_output_dword_through_edx(0x00004000U)) {
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

    [[nodiscard]] u32 ecx() const noexcept {
        return stream_token_ + static_cast<u32>(cursor_);
    }

    [[nodiscard]] u32 edx() const noexcept {
        return edx_;
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

    [[nodiscard]] bool or_output_dword_through_edx(const u32 mask) noexcept {
        if (!read_output_dword(0x04U, edx_)) {
            return false;
        }
        edx_ |= mask;
        return write_output_dword(0x04U, edx_);
    }

    [[nodiscard]] bool parse_pair() noexcept {
        if (!read_stream_dword(cursor_, edx_)) {
            return false;
        }
        cursor_ += 8U;
        if (!write_output_dword(0x0CU, edx_)) {
            return false;
        }
        if (!read_stream_dword(cursor_ - 4U, edx_)) {
            return false;
        }
        return write_output_dword(0x10U, edx_);
    }

    [[nodiscard]] bool parse_byte_24() noexcept {
        u8 value = 0U;
        if (!read_stream_byte(cursor_, value)) {
            return false;
        }
        edx_ = (edx_ & 0xFFFFFF00U) | value;
        cursor_ += 2U;
        return write_output_byte(0x24U, value);
    }

    [[nodiscard]] bool parse_word(const std::size_t output_offset) noexcept {
        u16 value = 0U;
        if (!read_stream_word(cursor_, value)) {
            return false;
        }
        edx_ = (edx_ & 0xFFFF0000U) | value;
        cursor_ += 2U;
        return write_output_word(output_offset, value);
    }

    [[nodiscard]] bool parse_word_18_with_flag() noexcept {
        if (!parse_word(0x18U)) {
            return false;
        }
        if (!read_output_dword(0x04U, edx_)) {
            return false;
        }
        edx_ |= 0x00000080U;
        return write_output_dword(0x04U, edx_);
    }

    [[nodiscard]] bool parse_word_1e_with_high_bit() noexcept {
        if (!parse_word(0x1EU)) {
            return false;
        }
        if (!output_available(0x1FU, 1U)) {
            return false;
        }
        const u8 value = std::to_integer<u8>(output_[0x1FU]);
        output_[0x1FU] = static_cast<std::byte>(value | 0x80U);
        return true;
    }

    [[nodiscard]] bool parse_dword(const std::size_t output_offset) noexcept {
        if (!read_stream_dword(cursor_, edx_)) {
            return false;
        }
        cursor_ += 4U;
        return write_output_dword(output_offset, edx_);
    }

    [[nodiscard]] bool parse_word_1a_with_flag() noexcept {
        if (!read_output_dword(0x04U, edx_)) {
            return false;
        }
        edx_ |= 0x00000008U;
        if (!write_output_dword(0x04U, edx_)) {
            return false;
        }
        u16 value = 0U;
        if (!read_stream_word(cursor_, value)) {
            return false;
        }
        edx_ = (edx_ & 0xFFFF0000U) | value;
        if (!write_output_word(0x1AU, value)) {
            return false;
        }
        cursor_ += 2U;
        return true;
    }

    [[nodiscard]] bool parse_word_and_byte_with_flag() noexcept {
        if (!read_output_dword(0x04U, edx_)) {
            return false;
        }
        edx_ |= 0x00001000U;
        cursor_ += 4U;
        if (!write_output_dword(0x04U, edx_)) {
            return false;
        }
        u16 word = 0U;
        if (!read_stream_word(cursor_ - 4U, word)) {
            return false;
        }
        edx_ = (edx_ & 0xFFFF0000U) | word;
        if (!write_output_word(0x20U, word)) {
            return false;
        }
        u8 byte = 0U;
        if (!read_stream_byte(cursor_ - 2U, byte)) {
            return false;
        }
        edx_ = (edx_ & 0xFFFFFF00U) | byte;
        return write_output_byte(0x24U, byte);
    }

    std::span<const u8> stream_;
    std::span<std::byte> output_;
    u32 stream_token_{};
    u32 edx_{};
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

LegacyBattleMonDatabaseCallReply
LegacyBattleMonDatabasePort::invoke_legacy_battle_mon_database(
    const LegacyBattleMonDatabaseCallRequest& request,
    const std::span<compat::u8> destination
) {
    static_cast<void>(destination);
    if (request.call == LegacyBattleMonDatabaseCall::open_file) {
        return {.eax = 0xFFFFFFFFU, .ecx = request.ecx, .edx = request.edx};
    }
    return {.eax = request.eax, .ecx = request.ecx, .edx = request.edx};
}

LegacyBattleMonProfileLoadResult load_legacy_battle_mon_profile(
    const std::span<std::byte> output,
    LegacyBattleMonDatabasePort& port,
    const LegacyBattleMonProfileLoadRequest& request
) {
    LegacyBattleMonProfileLoadResult result;
    auto& database = port.legacy_battle_mon_database_state();
    u32 eax = database.open ? database.handle : 0U;
    u32 ecx = request.entry_ecx;
    u32 edx = request.entry_edx;

    if (!database.open) {
        const auto open_reply = port.invoke_legacy_battle_mon_database(
            {
                .call = LegacyBattleMonDatabaseCall::open_file,
                .path = &request.path,
                .desired_access = 0x80000000U,
                .share_mode = 1U,
                .security_attributes_token = 0U,
                .creation_disposition = 4U,
                .flags_and_attributes = 0x80U,
                .template_file_token = 0U,
                .eax = request.file_name_token,
                .ecx = ecx,
                .edx = edx,
            },
            {}
        );
        ++result.open_calls;
        database.handle = open_reply.eax;
        result.handle = database.handle;
        eax = open_reply.eax;
        ecx = open_reply.ecx;
        edx = open_reply.edx;
        if (database.handle == 0xFFFFFFFFU) {
            result.status = LegacyBattleMonProfileLoadStatus::open_failed;
            result.return_eax = 0U;
            result.return_ecx = ecx;
            result.return_edx = edx;
            return result;
        }
        database.open = true;
    } else {
        result.handle = database.handle;
    }

    auto reply = port.invoke_legacy_battle_mon_database(
        {
            .call = LegacyBattleMonDatabaseCall::seek_file,
            .handle = database.handle,
            .distance = 0x204U,
            .distance_high_token = 0U,
            .move_method = 0U,
            .eax = eax,
            .ecx = ecx,
            .edx = edx,
        },
        {}
    );
    ++result.seek_calls;

    std::array<u8, 4U> root_bytes{};
    write_stale_dword(root_bytes, request.stale_root_buffer_value);
    reply = port.invoke_legacy_battle_mon_database(
        {
            .call = LegacyBattleMonDatabaseCall::read_file,
            .handle = database.handle,
            .destination_token = request.root_buffer_token,
            .requested_bytes = 4U,
            .eax = request.number_of_bytes_read_token,
            .ecx = request.root_buffer_token,
            .edx = database.handle,
        },
        root_bytes
    );
    ++result.read_calls;

    result.profile_id = request.profile_id & 0xFFFFU;
    result.auxiliary_root = read_dword(root_bytes);
    const u32 profile_offset_entry =
        result.auxiliary_root + result.profile_id * 4U + 0x200U;
    reply = port.invoke_legacy_battle_mon_database(
        {
            .call = LegacyBattleMonDatabaseCall::seek_file,
            .handle = database.handle,
            .distance = profile_offset_entry,
            .distance_high_token = 0U,
            .move_method = 0U,
            .eax = database.handle,
            .ecx = result.auxiliary_root,
            .edx = profile_offset_entry,
        },
        {}
    );
    ++result.seek_calls;

    std::array<u8, 4U> relative_bytes = root_bytes;
    reply = port.invoke_legacy_battle_mon_database(
        {
            .call = LegacyBattleMonDatabaseCall::read_file,
            .handle = database.handle,
            .destination_token = request.root_buffer_token,
            .requested_bytes = 4U,
            .eax = database.handle,
            .ecx = request.number_of_bytes_read_token,
            .edx = request.root_buffer_token,
        },
        relative_bytes
    );
    ++result.read_calls;

    result.profile_relative_offset = read_dword(relative_bytes);
    result.profile_file_offset = result.profile_relative_offset + 0x200U;
    reply = port.invoke_legacy_battle_mon_database(
        {
            .call = LegacyBattleMonDatabaseCall::seek_file,
            .handle = database.handle,
            .distance = result.profile_file_offset,
            .distance_high_token = 0U,
            .move_method = 0U,
            .eax = reply.eax,
            .ecx = result.profile_file_offset,
            .edx = database.handle,
        },
        {}
    );
    ++result.seek_calls;

    const auto allocation = port.invoke_legacy_battle_mon_database(
        {
            .call = LegacyBattleMonDatabaseCall::allocate_stream,
            .allocation_size = kLegacyBattleMonStreamBytes,
            .eax = reply.eax,
            .ecx = reply.ecx,
            .edx = reply.edx,
        },
        {}
    );
    ++result.allocation_calls;
    result.stream_token = allocation.eax;
    if (result.stream_token == 0U) {
        result.status =
            LegacyBattleMonProfileLoadStatus::stream_zero_typed_stop;
        result.return_eax = 0U;
        result.return_ecx = 0x100U;
        result.return_edx = allocation.edx;
        return result;
    }

    std::array<u8, kLegacyBattleMonStreamBytes> stream{};
    reply = port.invoke_legacy_battle_mon_database(
        {
            .call = LegacyBattleMonDatabaseCall::read_file,
            .handle = database.handle,
            .destination_token = result.stream_token,
            .requested_bytes = kLegacyBattleMonStreamBytes,
            .eax = request.number_of_bytes_read_token,
            .ecx = database.handle,
            .edx = allocation.edx,
        },
        stream
    );
    ++result.read_calls;

    if ((static_cast<u16>(stream[0U]) |
         static_cast<u16>(static_cast<u16>(stream[1U]) << 8U)) != 0U) {
        const auto release = port.invoke_legacy_battle_mon_database(
            {
                .call = LegacyBattleMonDatabaseCall::release_stream,
                .block_token = result.stream_token,
                .eax = reply.eax,
                .ecx = result.stream_token,
                .edx = reply.edx,
            },
            {}
        );
        ++result.release_calls;
        result.return_eax = 0U;
        result.return_ecx = release.ecx;
        result.return_edx = release.edx;
        return result;
    }

    ProfileParser parser(
        stream, output, result.stream_token, reply.edx, result
    );
    if (!parser.parse()) {
        result.stream_cursor = parser.cursor();
        result.return_eax = request.output_token;
        result.return_ecx = parser.ecx();
        result.return_edx = parser.edx();
        return result;
    }

    result.stream_cursor = parser.cursor();
    const auto release = port.invoke_legacy_battle_mon_database(
        {
            .call = LegacyBattleMonDatabaseCall::release_stream,
            .block_token = result.stream_token,
            .eax = request.output_token,
            .ecx = parser.ecx(),
            .edx = parser.edx(),
        },
        {}
    );
    ++result.release_calls;
    result.return_eax = 1U;
    result.return_ecx = release.ecx;
    result.return_edx = release.edx;
    return result;
}

}  // namespace openswd3::battle
