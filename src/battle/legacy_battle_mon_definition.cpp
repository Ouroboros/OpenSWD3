#include "openswd3/battle/legacy_battle_mon_definition.hpp"

#include <algorithm>
#include <array>

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
        std::vector<u8>& owned_description,
        LegacyBattleMonDatabasePort& port,
        LegacyBattleMonDatabaseState& database,
        const LegacyBattleMonDefinitionLoadRequest& request,
        const u32 entry_eax,
        const u32 entry_ecx,
        const u32 entry_edx,
        LegacyBattleMonDefinitionLoadResult& result
    ) noexcept
        : stream_(stream), output_(output),
          owned_description_(owned_description), port_(port),
          database_(database), request_(request), eax_(entry_eax),
          ecx_(entry_ecx), edx_(entry_edx), result_(result) {}

    [[nodiscard]] bool parse() {
        while (true) {
            u16 tag = 0U;
            if (!read_stream_word(cursor_, tag)) {
                return false;
            }
            eax_ = (eax_ & 0xFFFF0000U) | tag;
            cursor_ += 2U;
            if (tag == 5U) {
                return true;
            }
            eax_ = tag;

            switch (tag) {
            case 1U:
                if (!copy_stream_to_output(cursor_, 0x50U, 0x4DU)) {
                    return false;
                }
                cursor_ += 0x4DU;
                break;

            case 6U:
                if (!parse_word(0x40U, ValueRegister::edx)) {
                    return false;
                }
                break;

            case 7U:
                if (!parse_word(0x24U, ValueRegister::eax)) {
                    return false;
                }
                break;

            case 8U:
                if (!parse_word(0x26U, ValueRegister::ecx)) {
                    return false;
                }
                break;

            case 9U:
                if (!parse_word(0x2CU, ValueRegister::edx)) {
                    return false;
                }
                break;

            case 10U:
                if (!parse_word(0x32U, ValueRegister::eax)) {
                    return false;
                }
                break;

            case 11U:
                if (!parse_word(0x46U, ValueRegister::ecx)) {
                    return false;
                }
                break;

            case 12U:
                if (!parse_word(0x42U, ValueRegister::edx)) {
                    return false;
                }
                break;

            case 13U:
                if (!parse_word(0x44U, ValueRegister::eax)) {
                    return false;
                }
                break;

            case 14U:
                if (!parse_word(0x50U, ValueRegister::ecx)) {
                    return false;
                }
                break;

            case 15U:
                if (!parse_word(0x28U, ValueRegister::edx)) {
                    return false;
                }
                break;

            case 16U:
                if (!parse_word(0x2AU, ValueRegister::eax)) {
                    return false;
                }
                break;

            case 17U:
                if (!parse_word(0x2EU, ValueRegister::ecx)) {
                    return false;
                }
                break;

            case 18U:
                if (!parse_word(0x30U, ValueRegister::edx)) {
                    return false;
                }
                break;

            case 19U:
                if (!parse_word(0x34U, ValueRegister::ecx)) {
                    return false;
                }
                break;

            case 20U:
                if (!parse_word(0x36U, ValueRegister::edx)) {
                    return false;
                }
                break;

            case 21U:
                if (!parse_word(0x38U, ValueRegister::eax)) {
                    return false;
                }
                break;

            case 22U:
                if (!parse_word(0x48U, ValueRegister::ecx)) {
                    return false;
                }
                break;

            case 25U:
                if (!parse_padded_dword()) {
                    return false;
                }
                break;

            case 26U:
                if (!parse_word(0x3AU, ValueRegister::eax)) {
                    return false;
                }
                break;

            case 27U:
                if (!parse_word(0x3CU, ValueRegister::ecx)) {
                    return false;
                }
                break;

            case 28U:
                if (!parse_byte(0x9BU, ValueRegister::edx)) {
                    return false;
                }
                break;

            case 29U:
                if (!parse_byte(0x9CU, ValueRegister::eax)) {
                    return false;
                }
                break;

            case 30U:
                if (!parse_owned_description()) {
                    return false;
                }
                break;

            case 100U:
                if (!parse_word(0x3EU, ValueRegister::ecx)) {
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

    [[nodiscard]] u32 eax() const noexcept {
        return eax_;
    }

    [[nodiscard]] u32 ecx() const noexcept {
        return ecx_;
    }

    [[nodiscard]] u32 edx() const noexcept {
        return edx_;
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

    enum class ValueRegister : u8 {
        eax,
        ecx,
        edx,
    };

    void
    write_word_register(const ValueRegister target, const u16 value) noexcept {
        switch (target) {
        case ValueRegister::eax:
            eax_ = (eax_ & 0xFFFF0000U) | value;
            break;

        case ValueRegister::ecx:
            ecx_ = (ecx_ & 0xFFFF0000U) | value;
            break;

        case ValueRegister::edx:
            edx_ = (edx_ & 0xFFFF0000U) | value;
            break;
        }
    }

    void
    write_byte_register(const ValueRegister target, const u8 value) noexcept {
        switch (target) {
        case ValueRegister::eax:
            eax_ = (eax_ & 0xFFFFFF00U) | value;
            break;

        case ValueRegister::ecx:
            ecx_ = (ecx_ & 0xFFFFFF00U) | value;
            break;

        case ValueRegister::edx:
            edx_ = (edx_ & 0xFFFFFF00U) | value;
            break;
        }
    }

    [[nodiscard]] bool parse_word(
        const std::size_t output_offset, const ValueRegister target
    ) noexcept {
        u16 value = 0U;
        if (!read_stream_word(cursor_, value)) {
            return false;
        }
        write_word_register(target, value);
        if (!write_output_word(output_offset, value)) {
            return false;
        }
        cursor_ += 2U;
        return true;
    }

    [[nodiscard]] bool parse_byte(
        const std::size_t output_offset, const ValueRegister target
    ) noexcept {
        u8 value = 0U;
        if (!read_stream_byte(cursor_, value)) {
            return false;
        }
        write_byte_register(target, value);
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
        edx_ = value;
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
            eax_ = 0xFFU;
            cursor_ += 0x101U;
            return true;
        }
        eax_ = static_cast<u32>(length);
        ecx_ = 0U;
        edx_ = static_cast<u32>(length);
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
        const auto allocation = port_.invoke_legacy_battle_mon_database(
            {
                .call = LegacyBattleMonDatabaseCall::allocate_definition_text,
                .stream_kind = LegacyBattleMonDatabaseStreamKind::definition,
                .allocation_size = allocation_size,
                .eax = eax_,
                .ecx = ecx_,
                .edx = edx_,
            },
            {}
        );
        ++result_.definition_text_allocation_calls;
        const u32 token = allocation.eax;
        result_.definition_text_token = token;
        result_.definition_text_bytes = allocation_size;
        if (!write_output_dword(0xA0U, token)) {
            return false;
        }

        eax_ = 0U;
        ecx_ = allocation_size / 4U;
        if (ecx_ == 0U) {
            ecx_ = allocation_size & 3U;
        }
        edx_ = allocation_size;
        if (token == 0U) {
            result_.status = LegacyBattleMonDefinitionLoadStatus::
                definition_text_zero_typed_stop;
            return false;
        }

        owned_description_.assign(length + 1U, 0U);
        for (std::size_t index = 0U; index < length; ++index) {
            u8 value = 0U;
            if (!read_stream_byte(source + index, value)) {
                return false;
            }
            owned_description_[index] = value;
        }
        cursor_ += length + 2U;
        const u32 prior_allocation_bytes =
            database_.definition_text_allocation_bytes;
        database_.definition_text_allocation_bytes += allocation_size;
        eax_ = database_.definition_text_allocation_bytes;
        ecx_ = token;
        edx_ = prior_allocation_bytes;
        result_.definition_text_token = token;
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
    std::vector<u8>& owned_description_;
    LegacyBattleMonDatabasePort& port_;
    LegacyBattleMonDatabaseState& database_;
    const LegacyBattleMonDefinitionLoadRequest& request_;
    u32 eax_{};
    u32 ecx_{};
    u32 edx_{};
    std::size_t cursor_{};
    LegacyBattleMonDefinitionLoadResult& result_;
};

}  // namespace

LegacyBattleMonDefinitionLoadResult load_legacy_battle_mon_definition(
    const std::span<u8> output,
    std::vector<u8>& owned_description,
    LegacyBattleMonDatabasePort& port,
    const LegacyBattleMonDefinitionLoadRequest& request
) {
    LegacyBattleMonDefinitionLoadResult result{
        .definition_id = request.definition_id,
        .return_eax = request.entry_eax,
        .return_ecx = request.entry_ecx,
        .return_edx = request.entry_edx,
    };
    auto& database = port.legacy_battle_mon_database_state();

    if (output.size() < kLegacyBattleMonDefinitionBytes) {
        result.status =
            LegacyBattleMonDefinitionLoadStatus::output_access_typed_stop;
        result.stopped_output_offset = 0xA0U;
        return result;
    }

    result.prior_definition_text_token = read_dword(output.subspan(0xA0U, 4U));
    u32 edx = request.entry_edx;
    if (result.prior_definition_text_token != 0U) {
        const auto size_reply = port.invoke_legacy_battle_mon_database(
            {
                .call = LegacyBattleMonDatabaseCall::query_definition_text_size,
                .stream_kind = LegacyBattleMonDatabaseStreamKind::definition,
                .block_token = result.prior_definition_text_token,
                .eax = result.prior_definition_text_token,
                .ecx = request.entry_ecx,
                .edx = edx,
            },
            {}
        );
        ++result.definition_text_size_query_calls;
        database.definition_text_allocation_bytes -= size_reply.eax;
        edx = database.definition_text_allocation_bytes;

        const auto release_reply = port.invoke_legacy_battle_mon_database(
            {
                .call = LegacyBattleMonDatabaseCall::release_definition_text,
                .stream_kind = LegacyBattleMonDatabaseStreamKind::definition,
                .block_token = result.prior_definition_text_token,
                .eax = result.prior_definition_text_token,
                .ecx = size_reply.ecx,
                .edx = edx,
            },
            {}
        );
        ++result.definition_text_release_calls;
        result.return_eax = release_reply.eax;
        result.return_ecx = release_reply.ecx;
        result.return_edx = release_reply.edx;
        std::fill(output.begin() + 0xA0U, output.begin() + 0xA4U, 0U);
        owned_description.clear();
        edx = release_reply.edx;
    }

    std::fill_n(output.begin(), kLegacyBattleMonDefinitionBytes, 0U);
    owned_description.clear();

    u32 eax = database.open ? database.handle : 0U;
    u32 ecx = 0U;
    if (!database.open) {
        const auto open_reply = port.invoke_legacy_battle_mon_database(
            {
                .call = LegacyBattleMonDatabaseCall::open_file,
                .stream_kind = LegacyBattleMonDatabaseStreamKind::definition,
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
            result.status = LegacyBattleMonDefinitionLoadStatus::open_failed;
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
            .stream_kind = LegacyBattleMonDatabaseStreamKind::definition,
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

    std::array<u8, 4U> directory_probe{};
    write_stale_dword(directory_probe, request.stale_directory_probe_value);
    reply = port.invoke_legacy_battle_mon_database(
        {
            .call = LegacyBattleMonDatabaseCall::read_file,
            .stream_kind = LegacyBattleMonDatabaseStreamKind::definition,
            .handle = database.handle,
            .destination_token = request.directory_buffer_token,
            .requested_bytes = 4U,
            .eax = request.number_of_bytes_read_token,
            .ecx = request.directory_buffer_token,
            .edx = database.handle,
        },
        directory_probe
    );
    ++result.read_calls;
    result.directory_probe_value = read_dword(directory_probe);

    const u32 directory_definition_id = result.definition_id & 0xFFFFU;
    const u32 directory_displacement = directory_definition_id * 4U - 4U;
    result.definition_directory_offset = 0x204U + directory_definition_id * 4U;
    reply = port.invoke_legacy_battle_mon_database(
        {
            .call = LegacyBattleMonDatabaseCall::seek_file,
            .stream_kind = LegacyBattleMonDatabaseStreamKind::definition,
            .handle = database.handle,
            .distance = directory_displacement,
            .distance_high_token = 0U,
            .move_method = 1U,
            .eax = database.handle,
            .ecx = directory_definition_id,
            .edx = directory_displacement,
        },
        {}
    );
    ++result.seek_calls;

    std::array<u8, 4U> relative_bytes{};
    write_stale_dword(relative_bytes, request.stale_relative_offset_value);
    reply = port.invoke_legacy_battle_mon_database(
        {
            .call = LegacyBattleMonDatabaseCall::read_file,
            .stream_kind = LegacyBattleMonDatabaseStreamKind::definition,
            .handle = database.handle,
            .destination_token = request.directory_buffer_token,
            .requested_bytes = 4U,
            .eax = database.handle,
            .ecx = request.number_of_bytes_read_token,
            .edx = request.directory_buffer_token,
        },
        relative_bytes
    );
    ++result.read_calls;

    result.definition_relative_offset = read_dword(relative_bytes);
    result.definition_file_offset = result.definition_relative_offset + 0x200U;
    reply = port.invoke_legacy_battle_mon_database(
        {
            .call = LegacyBattleMonDatabaseCall::seek_file,
            .stream_kind = LegacyBattleMonDatabaseStreamKind::definition,
            .handle = database.handle,
            .distance = result.definition_file_offset,
            .distance_high_token = 0U,
            .move_method = 0U,
            .eax = reply.eax,
            .ecx = result.definition_file_offset,
            .edx = database.handle,
        },
        {}
    );
    ++result.seek_calls;

    const auto allocation = port.invoke_legacy_battle_mon_database(
        {
            .call = LegacyBattleMonDatabaseCall::allocate_stream,
            .stream_kind = LegacyBattleMonDatabaseStreamKind::definition,
            .allocation_size = kLegacyBattleMonStreamBytes,
            .eax = reply.eax,
            .ecx = reply.ecx,
            .edx = reply.edx,
        },
        {}
    );
    ++result.stream_allocation_calls;
    result.stream_token = allocation.eax;
    if (result.stream_token == 0U) {
        result.status =
            LegacyBattleMonDefinitionLoadStatus::stream_zero_typed_stop;
        result.return_eax = 0U;
        result.return_ecx = 0x100U;
        result.return_edx = allocation.edx;
        return result;
    }

    std::array<u8, kLegacyBattleMonStreamBytes> stream{};
    reply = port.invoke_legacy_battle_mon_database(
        {
            .call = LegacyBattleMonDatabaseCall::read_file,
            .stream_kind = LegacyBattleMonDatabaseStreamKind::definition,
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

    if (read_word(stream) != 1000U) {
        const auto release = port.invoke_legacy_battle_mon_database(
            {
                .call = LegacyBattleMonDatabaseCall::release_stream,
                .stream_kind = LegacyBattleMonDatabaseStreamKind::definition,
                .block_token = result.stream_token,
                .eax = reply.eax,
                .ecx = result.stream_token,
                .edx = reply.edx,
            },
            {}
        );
        ++result.stream_release_calls;
        result.return_eax = 0U;
        result.return_ecx = release.ecx;
        result.return_edx = release.edx;
        return result;
    }

    DefinitionParser parser(
        stream,
        output.first(kLegacyBattleMonDefinitionBytes),
        owned_description,
        port,
        database,
        request,
        reply.eax,
        reply.ecx,
        reply.edx,
        result
    );
    if (!parser.parse()) {
        result.stream_cursor = parser.cursor();
        result.return_eax = parser.eax();
        result.return_ecx = parser.ecx();
        result.return_edx = parser.edx();
        return result;
    }

    result.stream_cursor = parser.cursor();
    const auto release = port.invoke_legacy_battle_mon_database(
        {
            .call = LegacyBattleMonDatabaseCall::release_stream,
            .stream_kind = LegacyBattleMonDatabaseStreamKind::definition,
            .block_token = result.stream_token,
            .eax = parser.eax(),
            .ecx = parser.ecx(),
            .edx = parser.edx(),
        },
        {}
    );
    ++result.stream_release_calls;
    result.definition_found = true;
    result.return_eax = 1U;
    result.return_ecx = release.ecx;
    result.return_edx = release.edx;
    return result;
}

}  // namespace openswd3::battle
