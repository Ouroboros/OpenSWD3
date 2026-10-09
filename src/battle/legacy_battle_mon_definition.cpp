#include "openswd3/battle/legacy_battle_mon_definition.hpp"

#include <algorithm>
#include <array>
#include <bit>

namespace openswd3::battle {
namespace {

using compat::u8;
using compat::u16;
using compat::u32;

[[nodiscard]] u16 read_word(const std::span<const u8> bytes) noexcept {
    return static_cast<u16>(bytes[0U]) |
        static_cast<u16>(static_cast<u16>(bytes[1U]) << 8U);
}

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

class DefinitionParser final {
public:
    DefinitionParser(
        const std::span<const u8> stream,
        const std::span<u8> output,
        LegacyBattleMonText& owned_description,
        LegacyBattleMonDatabasePort& port,
        LegacyBattleMonDatabaseState& database,
        const LegacyBattleMonDefinitionLoadRequest& request,
        LegacyBattleMonDefinitionLoadResult& result
    ) noexcept
        : stream_(stream), output_(output),
          owned_description_(owned_description), port_(port),
          database_(database), request_(request), result_(result) {}

    [[nodiscard]] bool parse() {
        while (true) {
            u16 tag = 0U;
            if (!read_stream_word(cursor_, tag)) {
                return false;
            }
            cursor_ += 2U;
            if (tag == 5U) {
                return true;
            }

            switch (tag) {
            case 1U:
                if (!copy_stream_to_output(cursor_, 0x50U, 0x4DU)) {
                    return false;
                }
                cursor_ += 0x4DU;
                break;

            case 6U:
                if (!parse_word(0x40U)) {
                    return false;
                }
                break;

            case 7U:
                if (!parse_word(0x24U)) {
                    return false;
                }
                break;

            case 8U:
                if (!parse_word(0x26U)) {
                    return false;
                }
                break;

            case 9U:
                if (!parse_word(0x2CU)) {
                    return false;
                }
                break;

            case 10U:
                if (!parse_word(0x32U)) {
                    return false;
                }
                break;

            case 11U:
                if (!parse_word(0x46U)) {
                    return false;
                }
                break;

            case 12U:
                if (!parse_word(0x42U)) {
                    return false;
                }
                break;

            case 13U:
                if (!parse_word(0x44U)) {
                    return false;
                }
                break;

            case 14U:
                if (!parse_word(0x50U)) {
                    return false;
                }
                break;

            case 15U:
                if (!parse_word(0x28U)) {
                    return false;
                }
                break;

            case 16U:
                if (!parse_word(0x2AU)) {
                    return false;
                }
                break;

            case 17U:
                if (!parse_word(0x2EU)) {
                    return false;
                }
                break;

            case 18U:
                if (!parse_word(0x30U)) {
                    return false;
                }
                break;

            case 19U:
                if (!parse_word(0x34U)) {
                    return false;
                }
                break;

            case 20U:
                if (!parse_word(0x36U)) {
                    return false;
                }
                break;

            case 21U:
                if (!parse_word(0x38U)) {
                    return false;
                }
                break;

            case 22U:
                if (!parse_word(0x48U)) {
                    return false;
                }
                break;

            case 25U:
                if (!parse_padded_dword()) {
                    return false;
                }
                break;

            case 26U:
                if (!parse_word(0x3AU)) {
                    return false;
                }
                break;

            case 27U:
                if (!parse_word(0x3CU)) {
                    return false;
                }
                break;

            case 28U:
                if (!parse_byte(0x9BU)) {
                    return false;
                }
                break;

            case 29U:
                if (!parse_byte(0x9CU)) {
                    return false;
                }
                break;

            case 30U:
                if (!parse_owned_description()) {
                    return false;
                }
                break;

            case 100U:
                if (!parse_word(0x3EU)) {
                    return false;
                }
                break;

            case 1000U:
                if (!parse_name()) {
                    return false;
                }
                break;

            case 2000U:
                if (!parse_extended_parameters()) {
                    return false;
                }
                break;

            case 0U:
            case 2U:
            case 3U:
            case 4U:
            case 5U:
            case 23U:
            case 24U:
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
            LegacyBattleMonDefinitionLoadStatus::stream_access_typed_stop;
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
            LegacyBattleMonDefinitionLoadStatus::output_access_typed_stop;
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
        value = read_word(stream_.subspan(offset, 2U));
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
    write_output_byte(const std::size_t offset, const u8 value) noexcept {
        if (!output_available(offset, 1U)) {
            return false;
        }
        output_[offset] = value;
        return true;
    }

    [[nodiscard]] bool
    write_output_word(const std::size_t offset, const u16 value) noexcept {
        if (!output_available(offset, 2U)) {
            return false;
        }
        output_[offset] = static_cast<u8>(value);
        output_[offset + 1U] = static_cast<u8>(value >> 8U);
        return true;
    }

    [[nodiscard]] bool
    write_output_dword(const std::size_t offset, const u32 value) noexcept {
        if (!output_available(offset, 4U)) {
            return false;
        }
        output_[offset] = static_cast<u8>(value);
        output_[offset + 1U] = static_cast<u8>(value >> 8U);
        output_[offset + 2U] = static_cast<u8>(value >> 16U);
        output_[offset + 3U] = static_cast<u8>(value >> 24U);
        return true;
    }

    [[nodiscard]] bool copy_stream_to_output(
        const std::size_t source,
        const std::size_t destination,
        const std::size_t size
    ) noexcept {
        for (std::size_t index = 0U; index < size; ++index) {
            u8 value = 0U;
            if (!read_stream_byte(source + index, value) ||
                !write_output_byte(destination + index, value)) {
                return false;
            }
        }
        return true;
    }

    [[nodiscard]] bool parse_word(const std::size_t output_offset) noexcept {
        u16 value = 0U;
        if (!read_stream_word(cursor_, value)) {
            return false;
        }
        if (!write_output_word(output_offset, value)) {
            return false;
        }
        cursor_ += 2U;
        return true;
    }

    [[nodiscard]] bool parse_byte(const std::size_t output_offset) noexcept {
        u8 value = 0U;
        if (!read_stream_byte(cursor_, value)) {
            return false;
        }
        if (!write_output_byte(output_offset, value)) {
            return false;
        }
        ++cursor_;
        return true;
    }

    [[nodiscard]] bool parse_padded_dword() noexcept {
        u32 value = 0U;
        if (!read_stream_dword(cursor_ + 2U, value)) {
            return false;
        }
        if (!write_output_dword(0x20U, value)) {
            return false;
        }
        cursor_ += 6U;
        return true;
    }

    enum class TerminatorScan : u8 {
        found,
        not_found,
        stopped,
    };

    [[nodiscard]] TerminatorScan
    scan_terminator(const std::size_t source, std::size_t& length) noexcept {
        for (length = 0U; length < 0xFFU; ++length) {
            u8 first = 0U;
            if (!read_stream_byte(source + length, first)) {
                return TerminatorScan::stopped;
            }
            if (first != 0x24U) {
                continue;
            }
            u8 second = 0U;
            if (!read_stream_byte(source + length + 1U, second)) {
                return TerminatorScan::stopped;
            }
            if (second == 0x24U) {
                return TerminatorScan::found;
            }
        }
        return TerminatorScan::not_found;
    }

    [[nodiscard]] bool parse_name() noexcept {
        const std::size_t source = cursor_;
        std::size_t length = 0U;
        const TerminatorScan scan = scan_terminator(source, length);
        if (scan == TerminatorScan::stopped) {
            return false;
        }
        if (scan == TerminatorScan::not_found) {
            cursor_ += 0x101U;
            return true;
        }
        if (!copy_stream_to_output(source, 0U, length)) {
            return false;
        }
        cursor_ += length + 2U;
        return true;
    }

    [[nodiscard]] bool parse_owned_description() {
        const std::size_t source = cursor_;
        std::size_t length = 0U;
        const TerminatorScan scan = scan_terminator(source, length);
        if (scan == TerminatorScan::stopped) {
            return false;
        }

        if (scan == TerminatorScan::not_found) {
            cursor_ += 0xFFU;
            return true;
        }

        const u32 allocation_size = static_cast<u32>(length + 1U);
        const auto allocation = port_.allocate_mon_text(allocation_size);
        ++result_.definition_text_allocation_calls;
        result_.definition_text_token = allocation.block_token;
        result_.definition_text_bytes = allocation_size;
        if (!write_output_dword(0xA0U, allocation.block_token)) {
            return false;
        }

        if (allocation.block_token == 0U) {
            result_.status = LegacyBattleMonDefinitionLoadStatus::
                definition_text_zero_typed_stop;
            return false;
        }

        owned_description_.bind(allocation.storage, allocation.release);
        u32 cleared = 0U;
        for (u32 blocks = allocation_size / 4U; blocks != 0U; --blocks) {
            if (cleared > owned_description_.size() ||
                owned_description_.size() - cleared < 4U) {
                result_.status = LegacyBattleMonDefinitionLoadStatus::
                    definition_text_access_typed_stop;
                result_.stopped_definition_text_offset = cleared;
                return false;
            }

            for (u32 byte = 0U; byte < 4U; ++byte) {
                owned_description_[cleared + byte] = 0U;
            }

            cleared += 4U;
        }

        for (u32 bytes = allocation_size & 3U; bytes != 0U; --bytes) {
            if (cleared >= owned_description_.size()) {
                result_.status = LegacyBattleMonDefinitionLoadStatus::
                    definition_text_access_typed_stop;
                result_.stopped_definition_text_offset = cleared;
                return false;
            }

            owned_description_[cleared++] = 0U;
        }

        for (std::size_t index = 0U; index < length; ++index) {
            u8 value = 0U;
            if (!read_stream_byte(source + index, value)) {
                return false;
            }

            owned_description_[index] = value;
        }

        cursor_ += length + 2U;
        database_.definition_text_allocation_bytes += allocation_size;
        return true;
    }

    [[nodiscard]] bool parse_extended_parameters() noexcept {
        if ((request_.definition_id & 0xFFFFU) == 0x0126U) {
            result_.definition_id = 0x0126U;
        }
        if (!copy_stream_to_output(cursor_, 0x92U, 9U)) {
            return false;
        }
        u8 byte = 0U;
        if (!read_stream_byte(cursor_ + 9U, byte) ||
            !write_output_byte(0x9BU, byte)) {
            return false;
        }
        if (!read_stream_byte(cursor_ + 10U, byte) ||
            !write_output_byte(0x9CU, byte)) {
            return false;
        }
        u16 value = 0U;
        if (!read_stream_word(cursor_ + 11U, value) ||
            !write_output_word(0x54U, value)) {
            return false;
        }
        if (!read_stream_word(cursor_ + 13U, value) ||
            !write_output_word(0x52U, value)) {
            return false;
        }
        cursor_ += 15U;
        return true;
    }

    std::span<const u8> stream_;
    std::span<u8> output_;
    LegacyBattleMonText& owned_description_;
    LegacyBattleMonDatabasePort& port_;
    LegacyBattleMonDatabaseState& database_;
    const LegacyBattleMonDefinitionLoadRequest& request_;
    std::size_t cursor_{};
    LegacyBattleMonDefinitionLoadResult& result_;
};

}  // namespace

LegacyBattleMonDefinitionLoadResult load_legacy_battle_mon_definition(
    const std::span<u8> output,
    LegacyBattleMonText& owned_description,
    LegacyBattleMonDatabasePort& port,
    const LegacyBattleMonDefinitionLoadRequest& request
) {
    LegacyBattleMonDefinitionLoadResult result{
        .definition_id = request.definition_id,
    };
    auto& database = port.legacy_battle_mon_database_state();
    if (output.size() < kLegacyBattleMonDefinitionBytes) {
        result.status =
            LegacyBattleMonDefinitionLoadStatus::output_access_typed_stop;
        result.stopped_output_offset = 0xA0U;
        return result;
    }

    result.prior_definition_text_token = read_dword(output.subspan(0xA0U, 4U));
    if (result.prior_definition_text_token != 0U) {
        const auto size =
            port.mon_text_size(result.prior_definition_text_token);
        ++result.definition_text_size_query_calls;
        database.definition_text_allocation_bytes -= size;
        port.release_mon_text(result.prior_definition_text_token);
        ++result.definition_text_release_calls;
        std::fill(output.begin() + 0xA0U, output.begin() + 0xA4U, 0U);
        owned_description.clear();
    }

    std::fill_n(output.begin(), kLegacyBattleMonDefinitionBytes, 0U);
    owned_description.clear();
    if (!database.open) {
        database.handle = port.open_mon_file(request.path);
        ++result.open_calls;
        result.handle = database.handle;
        if (database.handle == 0xFFFFFFFFU) {
            result.status = LegacyBattleMonDefinitionLoadStatus::open_failed;
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
    std::array<u8, 4U> directory_probe{};
    write_stale_dword(directory_probe, request.stale_directory_probe_value);
    static_cast<void>(port.read_mon_file(database.handle, directory_probe, 4U));
    ++result.read_calls;
    result.directory_probe_value = read_dword(directory_probe);

    const u32 directory_id = result.definition_id & 0xFFFFU;
    const u32 displacement = directory_id * 4U - 4U;
    result.definition_directory_offset = 0x204U + directory_id * 4U;
    static_cast<void>(port.seek_mon_file(
        database.handle,
        std::bit_cast<compat::i32>(displacement),
        LegacyBattleMonSeekOrigin::current
    ));
    ++result.seek_calls;
    std::array<u8, 4U> relative_bytes{};
    write_stale_dword(relative_bytes, request.stale_relative_offset_value);
    static_cast<void>(port.read_mon_file(database.handle, relative_bytes, 4U));
    ++result.read_calls;

    result.definition_relative_offset = read_dword(relative_bytes);
    result.definition_file_offset = result.definition_relative_offset + 0x200U;
    static_cast<void>(port.seek_mon_file(
        database.handle,
        std::bit_cast<compat::i32>(result.definition_file_offset),
        LegacyBattleMonSeekOrigin::begin
    ));
    ++result.seek_calls;
    const auto allocation =
        port.allocate_mon_stream(kLegacyBattleMonStreamBytes);
    ++result.stream_allocation_calls;
    result.stream_token = allocation.block_token;
    if (result.stream_token == 0U) {
        result.status =
            LegacyBattleMonDefinitionLoadStatus::stream_zero_typed_stop;
        return result;
    }

    for (u32 offset = 0U; offset < kLegacyBattleMonStreamBytes; offset += 4U) {
        if (offset > allocation.bytes.size() ||
            allocation.bytes.size() - offset < 4U) {
            result.status =
                LegacyBattleMonDefinitionLoadStatus::stream_access_typed_stop;
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
    if (read_word(stream) != 1000U) {
        port.release_mon_stream(result.stream_token);
        ++result.stream_release_calls;
        return result;
    }

    DefinitionParser parser(
        stream,
        output.first(kLegacyBattleMonDefinitionBytes),
        owned_description,
        port,
        database,
        request,
        result
    );
    if (!parser.parse()) {
        result.stream_cursor = parser.cursor();
        return result;
    }

    result.stream_cursor = parser.cursor();
    port.release_mon_stream(result.stream_token);
    ++result.stream_release_calls;
    result.definition_found = true;
    return result;
}

}  // namespace openswd3::battle
