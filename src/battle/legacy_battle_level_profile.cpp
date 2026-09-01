#include "openswd3/battle/legacy_battle_level_profile.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <new>

namespace openswd3::battle {
namespace {

using compat::i8;
using compat::i16;
using compat::u8;
using compat::u16;
using compat::u32;
using world_map::LegacyWorldItemNode;
using world_map::LegacyWorldSentinelItemList;
using world_map::LegacyWorldStoryPartyMemberResources;

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

[[nodiscard]] u32 definition_text_token(
    const std::array<u8, kLegacyBattleMonDefinitionBytes>& bytes
) noexcept {
    return read_dword(bytes, 0xA0U);
}

class ProfileParser final {
public:
    ProfileParser(
        const std::span<const u8> stream,
        LegacyWorldStoryPartyMemberResources& output,
        std::array<u8, 24U>& growth_caption,
        u32& transition_mode,
        LegacyBattleLevelProfilePort& port,
        const LegacyBattleLevelProfileLoadRequest& request,
        LegacyBattleLevelProfileLoadResult& result,
        const u32 entry_eax,
        const u32 entry_ecx,
        const u32 entry_edx
    ) noexcept
        : stream_(stream),
          output_(reinterpret_cast<u8*>(&output), sizeof(output)),
          growth_caption_(growth_caption), transition_mode_(transition_mode),
          port_(port), request_(request), result_(result), eax_(entry_eax),
          ecx_(entry_ecx), edx_(entry_edx) {}

    [[nodiscard]] bool parse() {
        while (true) {
            edx_ = result_.stream_token + static_cast<u32>(cursor_);
            u8 tag_byte = 0U;
            if (!read_stream_byte(cursor_, tag_byte)) {
                return false;
            }
            const u16 sign_extended =
                static_cast<u16>(static_cast<i16>(std::bit_cast<i8>(tag_byte)));
            eax_ = (eax_ & 0xFFFF0000U) | sign_extended;
            cursor_ += 2U;
            if (static_cast<u16>(eax_) == 5U) {
                return true;
            }

            eax_ &= 0xFFFFU;
            if (eax_ == 0U) {
                if (!parse_profile_record()) {
                    return false;
                }
                continue;
            }
            --eax_;
            if (eax_ == 0U && !parse_item_reference()) {
                return false;
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
            LegacyBattleLevelProfileLoadStatus::stream_access_typed_stop;
        result_.stopped_stream_offset = static_cast<u32>(offset);
        return false;
    }

    [[nodiscard]] bool output_available(
        const std::size_t offset, const std::size_t size
    ) noexcept {
        const std::size_t accessible = std::min<std::size_t>(
            request_.output_accessible_bytes, output_.size()
        );
        if (offset <= accessible && size <= accessible - offset) {
            return true;
        }
        result_.status =
            LegacyBattleLevelProfileLoadStatus::output_access_typed_stop;
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
        value = read_word(stream_, offset);
        return true;
    }

    [[nodiscard]] bool copy_dword_to_output(
        const std::size_t source, const std::size_t destination
    ) noexcept {
        if (!stream_available(source, 4U)) {
            return false;
        }
        if (!output_available(destination, 4U)) {
            return false;
        }
        std::copy_n(
            stream_.begin() + static_cast<std::ptrdiff_t>(source),
            4U,
            output_.begin() + static_cast<std::ptrdiff_t>(destination)
        );
        result_.output_bytes_copied += 4U;
        ++result_.output_write_count;
        return true;
    }

    [[nodiscard]] bool copy_word_to_output(
        const std::size_t source, const std::size_t destination
    ) noexcept {
        if (!stream_available(source, 2U)) {
            return false;
        }
        if (!output_available(destination, 2U)) {
            return false;
        }
        output_[destination] = stream_[source];
        output_[destination + 1U] = stream_[source + 1U];
        result_.output_bytes_copied += 2U;
        ++result_.output_write_count;
        return true;
    }

    [[nodiscard]] bool
    write_output_byte(const std::size_t offset, const u8 value) noexcept {
        if (!output_available(offset, 1U)) {
            return false;
        }
        output_[offset] = value;
        ++result_.output_write_count;
        return true;
    }

    [[nodiscard]] bool
    write_output_word(const std::size_t offset, const u16 value) noexcept {
        if (!output_available(offset, 2U)) {
            return false;
        }
        output_[offset] = static_cast<u8>(value);
        output_[offset + 1U] = static_cast<u8>(value >> 8U);
        ++result_.output_write_count;
        return true;
    }

    [[nodiscard]] bool parse_profile_record() noexcept {
        constexpr std::size_t kPayloadBytes = 0x1AU;
        const std::size_t payload = cursor_;
        const u32 source_token =
            result_.stream_token + static_cast<u32>(payload);
        const u32 destination_token = request_.output_token + 0x0AU;
        eax_ = destination_token;
        ecx_ = 6U;
        edx_ = source_token;

        std::size_t source = payload;
        std::size_t destination = 0x0AU;
        while (ecx_ != 0U) {
            if (!copy_dword_to_output(source, destination)) {
                return false;
            }
            source += 4U;
            destination += 4U;
            --ecx_;
        }

        ecx_ = source_token + kPayloadBytes;
        edx_ = (source_token & 0xFFFFFF00U) | static_cast<u8>(request_.level);
        if (!copy_word_to_output(source, destination)) {
            return false;
        }
        if (!output_available(0x0AU, 2U)) {
            return false;
        }
        const u16 field_0a = read_word(output_, 0x0AU);
        eax_ = (eax_ & 0xFFFF0000U) | field_0a;

        cursor_ = payload + kPayloadBytes;
        if (!output_available(0x0CU, 2U)) {
            return false;
        }
        ecx_ = (ecx_ & 0xFFFF0000U) | read_word(output_, 0x0CU);
        if (!write_output_byte(0x2CU, static_cast<u8>(request_.level))) {
            return false;
        }
        if (!output_available(0x0EU, 2U)) {
            return false;
        }
        edx_ = (edx_ & 0xFFFF0000U) | read_word(output_, 0x0EU);
        if (!write_output_word(0x04U, field_0a) ||
            !write_output_word(0x06U, static_cast<u16>(ecx_)) ||
            !write_output_word(0x08U, static_cast<u16>(edx_))) {
            return false;
        }
        return true;
    }

    [[nodiscard]] LegacyWorldItemNode*
    find_node(LegacyWorldSentinelItemList& list, const u32 token) noexcept {
        for (auto& node : list.nodes) {
            if (node.legacy_token == token) {
                return &node;
            }
        }
        return nullptr;
    }

    [[nodiscard]] bool parse_item_reference() {
        u16 item_id = 0U;
        if (!read_stream_word(cursor_, item_id)) {
            return false;
        }
        const u32 party = request_.party_number_one_based;
        if (party == 0U || party > world_map::kLegacyPartyItemListCount) {
            result_.status =
                LegacyBattleLevelProfileLoadStatus::party_index_typed_stop;
            return false;
        }
        result_.temporary_party_head_token = 0U;
        result_.party_root_restored = false;
        auto* const item_lists = port_.battle_level_profile_item_list_state();
        if (item_lists == nullptr) {
            result_.status =
                LegacyBattleLevelProfileLoadStatus::party_sentinel_typed_stop;
            return false;
        }
        auto& optional = item_lists->party_item_lists[party - 1U];
        if (!optional.has_value()) {
            result_.status =
                LegacyBattleLevelProfileLoadStatus::party_sentinel_typed_stop;
            return false;
        }
        auto& list = *optional;
        original_root_token_ = list.legacy_head_token != 0U
            ? list.legacy_head_token
            : list.sentinel.legacy_token;
        list.legacy_head_token = original_root_token_;
        LegacyWorldItemNode* current =
            original_root_token_ == list.sentinel.legacy_token
            ? &list.sentinel
            : find_node(list, original_root_token_);
        ecx_ = original_root_token_;
        result_.temporary_party_head_token = original_root_token_;

        if (item_id == 0U) {
            const auto reply = port_.invoke_level_profile({
                .call = LegacyBattleLevelProfileCall::report_zero_item,
                .window_token = request_.window_token,
                .text_token = kLegacyBattleLevelZeroItemTextToken,
                .flags = 0U,
                .source_token = kLegacyBattleLevelZeroItemSourceToken,
                .source_line = kLegacyBattleLevelZeroItemSourceLine,
                .eax = eax_,
                .ecx = ecx_,
                .edx = edx_,
            });
            ++result_.diagnostic_calls;
            eax_ = reply.eax;
            ecx_ = reply.ecx;
            edx_ = reply.edx;
            if (reply.typed_stop) {
                result_.status =
                    LegacyBattleLevelProfileLoadStatus::diagnostic_typed_stop;
                return false;
            }
        }

        eax_ = result_.temporary_party_head_token;
        if (current == nullptr) {
            result_.status =
                LegacyBattleLevelProfileLoadStatus::item_node_typed_stop;
            result_.stopped_item_token = result_.temporary_party_head_token;
            return false;
        }
        while (current->item_id != item_id) {
            ecx_ = result_.temporary_party_head_token;
            const u32 next = current->legacy_next_token;
            eax_ = next;
            if (next == 0U) {
                if (item_id == 0x8000U) {
                    return restore_root_and_advance();
                }
                return append_item(list, *current, item_id);
            }
            result_.temporary_party_head_token = next;
            list.legacy_head_token = next;
            current = find_node(list, next);
            if (current == nullptr) {
                result_.status =
                    LegacyBattleLevelProfileLoadStatus::item_node_typed_stop;
                result_.stopped_item_token = next;
                return false;
            }
            ++result_.traversed_item_nodes;
        }
        return restore_root_and_advance();
    }

    [[nodiscard]] bool append_item(
        LegacyWorldSentinelItemList& list,
        LegacyWorldItemNode& previous,
        const u16 item_id
    ) {
        const auto allocation = port_.invoke_level_profile({
            .call = LegacyBattleLevelProfileCall::allocate_item_node,
            .allocation_size = kLegacyBattleLevelItemNodeBytes,
            .eax = eax_,
            .ecx = result_.temporary_party_head_token,
            .edx = edx_,
        });
        ++result_.item_allocation_calls;
        const u32 allocation_token = allocation.publish_allocation_token
            ? allocation.allocation_token
            : allocation.eax;
        edx_ = previous.legacy_token;
        previous.legacy_next_token = allocation_token;
        if (allocation.allocation_failed || allocation_token == 0U) {
            eax_ = 0U;
            ecx_ = 0x2CU;
            result_.status =
                LegacyBattleLevelProfileLoadStatus::item_allocation_typed_stop;
            return false;
        }
        if (!request_.host_item_node_allocation_succeeds) {
            result_.status = LegacyBattleLevelProfileLoadStatus::
                host_item_allocation_typed_stop;
            return false;
        }
        try {
            list.nodes.emplace_back();
        } catch (const std::bad_alloc&) {
            result_.status = LegacyBattleLevelProfileLoadStatus::
                host_item_allocation_typed_stop;
            return false;
        }
        auto& appended = list.nodes.back();
        appended.legacy_token = allocation_token;
        list.legacy_head_token = allocation_token;
        result_.temporary_party_head_token = allocation_token;
        appended.item_id = item_id;
        ++result_.appended_item_nodes;

        std::array<u8, kLegacyBattleMonDefinitionBytes> definition{};
        std::copy(
            appended.definition_snapshot.cbegin(),
            appended.definition_snapshot.cend(),
            definition.begin()
        );
        write_dword(definition, 0xA0U, appended.legacy_description_token);
        ecx_ = item_id;
        eax_ = allocation_token + kLegacyBattleLevelItemDefinitionOffset;
        result_.mon_definition_load = load_legacy_battle_mon_definition(
            definition,
            appended.description,
            port_,
            {
                .path = request_.mon_path,
                .output_token = eax_,
                .definition_id = item_id,
                .file_name_token = request_.mon_file_name_token,
                .directory_buffer_token = request_.mon_directory_buffer_token,
                .stale_directory_probe_value =
                    request_.mon_stale_directory_probe_value,
                .stale_relative_offset_value =
                    request_.mon_stale_relative_offset_value,
                .number_of_bytes_read_token =
                    request_.mon_number_of_bytes_read_token,
                .entry_eax = eax_,
                .entry_ecx = ecx_,
                .entry_edx = edx_,
            }
        );
        ++result_.item_definition_load_calls;
        eax_ = result_.mon_definition_load.return_eax;
        ecx_ = result_.mon_definition_load.return_ecx;
        edx_ = result_.mon_definition_load.return_edx;
        std::copy_n(
            definition.cbegin(),
            appended.definition_snapshot.size(),
            appended.definition_snapshot.begin()
        );
        appended.legacy_description_token = definition_text_token(definition);
        if (legacy_battle_mon_definition_load_stopped(
                result_.mon_definition_load.status
            )) {
            result_.status = LegacyBattleLevelProfileLoadStatus::
                mon_definition_load_typed_stop;
            return false;
        }

        edx_ = request_.transition_mode_token;
        if (!request_.transition_mode_accessible) {
            result_.status =
                LegacyBattleLevelProfileLoadStatus::transition_mode_typed_stop;
            return false;
        }
        transition_mode_ = 1U;
        ++result_.transition_mode_writes;
        if (!copy_caption(definition)) {
            return false;
        }
        return restore_root_and_advance();
    }

    [[nodiscard]] bool copy_caption(
        const std::array<u8, kLegacyBattleMonDefinitionBytes>& definition
    ) noexcept {
        eax_ = request_.caption_token;
        for (std::size_t index = 0U;; ++index) {
            if (index >= definition.size()) {
                result_.status = LegacyBattleLevelProfileLoadStatus::
                    caption_source_typed_stop;
                result_.stopped_caption_offset = static_cast<u32>(index);
                return false;
            }
            if (index >= growth_caption_.size() ||
                index >= request_.caption_accessible_bytes) {
                result_.status = LegacyBattleLevelProfileLoadStatus::
                    caption_destination_typed_stop;
                result_.stopped_caption_offset = static_cast<u32>(index);
                return false;
            }
            const u8 value = definition[index];
            growth_caption_[index] = value;
            ++result_.caption_bytes_copied;
            if (value == 0U) {
                return true;
            }
        }
    }

    [[nodiscard]] bool restore_root_and_advance() noexcept {
        result_.party_root_restored = true;
        auto& list =
            *port_.battle_level_profile_item_list_state()
                 ->party_item_lists[request_.party_number_one_based - 1U];
        list.legacy_head_token = original_root_token_;
        ecx_ = original_root_token_;
        cursor_ += 2U;
        eax_ = result_.stream_token + static_cast<u32>(cursor_);
        return true;
    }

    std::span<const u8> stream_;
    std::span<u8> output_;
    std::array<u8, 24U>& growth_caption_;
    u32& transition_mode_;
    LegacyBattleLevelProfilePort& port_;
    const LegacyBattleLevelProfileLoadRequest& request_;
    LegacyBattleLevelProfileLoadResult& result_;
    std::size_t cursor_{};
    u32 eax_{};
    u32 ecx_{};
    u32 edx_{};
    u32 original_root_token_{};
};

}  // namespace

LegacyBattleLevelProfileLoadResult load_legacy_battle_level_profile(
    LegacyWorldStoryPartyMemberResources& output,
    std::array<u8, 24U>& growth_caption,
    u32& transition_mode,
    LegacyBattleLevelProfilePort& port,
    const LegacyBattleLevelProfileLoadRequest& request
) {
    LegacyBattleLevelProfileLoadResult result;
    result.party_number_one_based = request.party_number_one_based;
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
            result.status = LegacyBattleLevelProfileLoadStatus::open_failed;
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

    const u32 party_times_5 = request.party_number_one_based * 5U;
    const u32 party_times_25 = party_times_5 * 5U;
    const u32 directory_index = request.level + party_times_25 * 4U;
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
    ++result.stream_allocation_calls;
    result.stream_token = allocation.eax;
    if (result.stream_token == 0U) {
        result.status =
            LegacyBattleLevelProfileLoadStatus::stream_zero_typed_stop;
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
                .eax = reply.eax,
                .ecx = reply.ecx,
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

    result.record_found = true;
    ProfileParser parser(
        stream,
        output,
        growth_caption,
        transition_mode,
        port,
        request,
        result,
        reply.eax,
        reply.ecx,
        reply.edx
    );
    if (!parser.parse()) {
        result.stream_cursor = parser.cursor();
        result.return_eax = parser.eax();
        result.return_ecx = parser.ecx();
        result.return_edx = parser.edx();
        return result;
    }
    result.stream_cursor = parser.cursor();

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
    ++result.stream_release_calls;
    result.return_eax = 1U;
    result.return_ecx = release.ecx;
    result.return_edx = release.edx;
    return result;
}

}  // namespace openswd3::battle
