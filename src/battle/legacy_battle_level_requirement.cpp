#include "openswd3/battle/legacy_battle_level_requirement.hpp"

#include <array>
#include <bit>
#include <cstddef>

namespace openswd3::battle {
namespace {

using compat::u8;
using compat::u16;
using compat::u32;

[[nodiscard]] u16
read_word(const std::span<const u8> bytes, const std::size_t offset) noexcept {
    return static_cast<u16>(bytes[offset]) |
        static_cast<u16>(static_cast<u16>(bytes[offset + 1U]) << 8U);
}

[[nodiscard]] u32
read_dword(const std::span<const u8> bytes, const std::size_t offset) noexcept {
    return static_cast<u32>(bytes[offset]) |
        (static_cast<u32>(bytes[offset + 1U]) << 8U) |
        (static_cast<u32>(bytes[offset + 2U]) << 16U) |
        (static_cast<u32>(bytes[offset + 3U]) << 24U);
}

void write_dword(
    const std::span<u8> bytes, const std::size_t offset, const u32 value
) noexcept {
    bytes[offset] = static_cast<u8>(value);
    bytes[offset + 1U] = static_cast<u8>(value >> 8U);
    bytes[offset + 2U] = static_cast<u8>(value >> 16U);
    bytes[offset + 3U] = static_cast<u8>(value >> 24U);
}

class RequirementParser final {
public:
    RequirementParser(
        const std::span<const u8> stream,
        const u32 stream_token,
        const u32 output_token,
        const bool output_accessible,
        u32& output,
        const u32 entry_ecx,
        LegacyBattleLevelRequirementLoadResult& result
    ) noexcept
        : stream_(stream), stream_token_(stream_token),
          output_accessible_(output_accessible), output_(output),
          ecx_(entry_ecx), edx_(output_token), result_(result) {}

    [[nodiscard]] bool parse() noexcept {
        while (true) {
            u16 tag = 0U;
            if (!read_stream_word(cursor_, tag)) {
                return false;
            }
            ecx_ = (ecx_ & 0xFFFF0000U) | tag;
            cursor_ += 2U;
            if (tag == 5U) {
                return true;
            }

            ecx_ &= 0xFFFFU;
            if (ecx_ == 0U) {
                if (!parse_requirement_record()) {
                    return false;
                }
                continue;
            }

            --ecx_;
            if (ecx_ == 0U) {
                cursor_ += 2U;
            }
        }
    }

    [[nodiscard]] std::size_t cursor() const noexcept {
        return cursor_;
    }

    [[nodiscard]] u32 eax() const noexcept {
        return stream_token_ + static_cast<u32>(cursor_);
    }

    [[nodiscard]] u32 ecx() const noexcept {
        return ecx_;
    }

    [[nodiscard]] u32 edx() const noexcept {
        return edx_;
    }

private:
    [[nodiscard]] bool
    read_stream_word(const std::size_t offset, u16& value) noexcept {
        if (offset + 2U > stream_.size()) {
            result_.status = LegacyBattleLevelRequirementLoadStatus::
                stream_access_typed_stop;
            result_.stopped_stream_offset = static_cast<u32>(offset);
            return false;
        }
        value = read_word(stream_, offset);
        return true;
    }

    [[nodiscard]] bool parse_requirement_record() noexcept {
        constexpr std::size_t kRecordBytes = 0x1AU;
        constexpr std::size_t kDwordCount = 6U;
        const std::size_t payload_start = cursor_;
        cursor_ += kRecordBytes;
        ecx_ = kDwordCount;

        std::array<u8, kRecordBytes> local_record{};
        std::size_t source = payload_start;
        std::size_t destination = 0U;
        while (ecx_ != 0U) {
            if (source + 4U > stream_.size()) {
                result_.status = LegacyBattleLevelRequirementLoadStatus::
                    stream_access_typed_stop;
                result_.stopped_stream_offset = static_cast<u32>(source);
                result_.copied_record_bytes = static_cast<u32>(destination);
                return false;
            }
            for (std::size_t index = 0U; index < 4U; ++index) {
                local_record[destination + index] = stream_[source + index];
            }
            source += 4U;
            destination += 4U;
            --ecx_;
        }
        if (source + 2U > stream_.size()) {
            result_.status = LegacyBattleLevelRequirementLoadStatus::
                stream_access_typed_stop;
            result_.stopped_stream_offset = static_cast<u32>(source);
            result_.copied_record_bytes = static_cast<u32>(destination);
            return false;
        }
        local_record[destination] = stream_[source];
        local_record[destination + 1U] = stream_[source + 1U];
        result_.copied_record_bytes = static_cast<u32>(local_record.size());
        ecx_ = read_dword(local_record, 0x16U);
        if (!output_accessible_) {
            result_.status = LegacyBattleLevelRequirementLoadStatus::
                output_access_typed_stop;
            return false;
        }
        output_ = ecx_;
        result_.output_value = output_;
        ++result_.output_write_count;
        return true;
    }

    std::span<const u8> stream_;
    u32 stream_token_{};
    bool output_accessible_{};
    u32& output_;
    u32 ecx_{};
    u32 edx_{};
    std::size_t cursor_{};
    LegacyBattleLevelRequirementLoadResult& result_;
};

}  // namespace

LegacyBattleLevelDatabaseState&
LegacyBattleLevelDatabasePort::legacy_battle_level_database_state() noexcept {
    return level_database_state_;
}

LegacyBattleLevelDatabaseCallReply
LegacyBattleLevelDatabasePort::invoke_legacy_battle_level_database(
    const LegacyBattleLevelDatabaseCallRequest& request,
    const std::span<compat::u8> destination
) {
    static_cast<void>(destination);
    if (request.call == LegacyBattleLevelDatabaseCall::open_file) {
        return {.eax = 0xFFFFFFFFU, .ecx = request.ecx, .edx = request.edx};
    }
    return {.eax = request.eax, .ecx = request.ecx, .edx = request.edx};
}

LegacyBattleLevelRequirementLoadResult load_legacy_battle_level_requirement(
    u32& output,
    LegacyBattleLevelDatabasePort& port,
    const LegacyBattleLevelRequirementLoadRequest& request
) {
    LegacyBattleLevelRequirementLoadResult result;
    result.group = request.group;
    result.level = request.level;

    auto& database = port.legacy_battle_level_database_state();
    u32 eax = database.open ? database.handle : request.entry_eax;
    u32 ecx = request.entry_ecx;
    u32 edx = request.entry_edx;

    if (!database.open) {
        const auto open_reply = port.invoke_legacy_battle_level_database(
            {
                .call = LegacyBattleLevelDatabaseCall::open_file,
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
            result.status = LegacyBattleLevelRequirementLoadStatus::open_failed;
            result.return_eax = 0U;
            result.return_ecx = ecx;
            result.return_edx = edx;
            return result;
        }
        database.open = true;
    } else {
        result.handle = database.handle;
        eax = database.handle;
    }

    const u32 group_times_5 = request.group * 5U;
    const u32 group_times_25 = group_times_5 * 5U;
    const u32 directory_index = request.level + group_times_25 * 4U;
    result.directory_entry_offset = 0x70U + directory_index * 4U;

    auto reply = port.invoke_legacy_battle_level_database(
        {
            .call = LegacyBattleLevelDatabaseCall::seek_file,
            .handle = database.handle,
            .distance = result.directory_entry_offset,
            .distance_high_token = 0U,
            .move_method = 0U,
            .eax = eax,
            .ecx = directory_index,
            .edx = result.directory_entry_offset,
        },
        {}
    );
    ++result.seek_calls;

    std::array<u8, 4U> offset_bytes{};
    write_dword(offset_bytes, 0U, request.stale_directory_offset);
    reply = port.invoke_legacy_battle_level_database(
        {
            .call = LegacyBattleLevelDatabaseCall::read_file,
            .handle = database.handle,
            .destination_token = kLegacyBattleLevelRootBufferToken,
            .requested_bytes = 4U,
            .eax = request.number_of_bytes_read_token,
            .ecx = kLegacyBattleLevelRootBufferToken,
            .edx = database.handle,
        },
        offset_bytes
    );
    ++result.read_calls;

    result.record_relative_offset = read_dword(offset_bytes, 0U);
    result.record_file_offset = result.record_relative_offset + 0x200U;
    reply = port.invoke_legacy_battle_level_database(
        {
            .call = LegacyBattleLevelDatabaseCall::seek_file,
            .handle = database.handle,
            .distance = result.record_file_offset,
            .distance_high_token = 0U,
            .move_method = 0U,
            .eax = result.record_file_offset,
            .ecx = database.handle,
            .edx = reply.edx,
        },
        {}
    );
    ++result.seek_calls;

    const auto allocation = port.invoke_legacy_battle_level_database(
        {
            .call = LegacyBattleLevelDatabaseCall::allocate_stream,
            .allocation_size = kLegacyBattleLevelStreamBytes,
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
            LegacyBattleLevelRequirementLoadStatus::stream_zero_typed_stop;
        result.return_eax = 0U;
        result.return_ecx = 0x100U;
        result.return_edx = request.number_of_bytes_read_token;
        return result;
    }

    std::array<u8, kLegacyBattleLevelStreamBytes> stream{};
    reply = port.invoke_legacy_battle_level_database(
        {
            .call = LegacyBattleLevelDatabaseCall::read_file,
            .handle = database.handle,
            .destination_token = result.stream_token,
            .requested_bytes = kLegacyBattleLevelStreamBytes,
            .eax = database.handle,
            .ecx = 0U,
            .edx = request.number_of_bytes_read_token,
        },
        stream
    );
    ++result.read_calls;

    if (read_word(stream, 0U) != 0U) {
        const auto release = port.invoke_legacy_battle_level_database(
            {
                .call = LegacyBattleLevelDatabaseCall::release_stream,
                .block_token = result.stream_token,
                .eax = result.stream_token,
                .ecx = reply.ecx,
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

    RequirementParser parser(
        stream,
        result.stream_token,
        request.output_token,
        request.output_accessible,
        output,
        reply.ecx,
        result
    );
    if (!parser.parse()) {
        result.stream_cursor = static_cast<u32>(parser.cursor());
        result.return_eax = parser.eax();
        result.return_ecx = parser.ecx();
        result.return_edx = parser.edx();
        return result;
    }

    result.stream_cursor = static_cast<u32>(parser.cursor());
    const auto release = port.invoke_legacy_battle_level_database(
        {
            .call = LegacyBattleLevelDatabaseCall::release_stream,
            .block_token = result.stream_token,
            .eax = parser.eax(),
            .ecx = parser.ecx(),
            .edx = parser.edx(),
        },
        {}
    );
    ++result.release_calls;
    result.record_found = true;
    result.return_eax = 1U;
    result.return_ecx = release.ecx;
    result.return_edx = release.edx;
    return result;
}

}  // namespace openswd3::battle
