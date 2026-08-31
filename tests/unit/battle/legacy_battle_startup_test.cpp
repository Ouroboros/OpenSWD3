#include "legacy_battle_mon_database_fixture.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <deque>
#include <unordered_map>
#include <vector>

#include "test.hpp"

namespace {

using openswd3::battle::LegacyBattleActionRotationUpdateSnapshot;
using openswd3::battle::LegacyBattleBackgroundImageLoadResult;
using openswd3::battle::LegacyBattleDefinition;
using openswd3::battle::LegacyBattleMutableFrameImage;
using openswd3::battle::LegacyBattleStartupCall;
using openswd3::battle::LegacyBattleStartupCallReply;
using openswd3::battle::LegacyBattleStartupCallRequest;
using openswd3::battle::LegacyBattleStartupRequest;
using openswd3::battle::LegacyBattleStartupState;
using openswd3::compat::i32;
using openswd3::compat::u16;
using openswd3::compat::u32;

class StartupPorts final
    : public openswd3::battle::LegacyBattleStartupPort,
      public openswd3::battle::LegacyBattleDefinitionArchiveFilePort,
      public openswd3::battle::LegacyBattleBackgroundImageLoadPort,
      public openswd3::battle::LegacyBattleActionRotationReleasePort,
      public openswd3::battle::LegacyBattleActionRotationUpdatePort,
      public openswd3::battle::LegacyBattleMutableFrameImagePort,
      public openswd3::test::LegacyBattleMonDatabaseFixture {
public:
    [[nodiscard]] LegacyBattleStartupCallReply
    invoke(const LegacyBattleStartupCallRequest& request) override {
        requests.push_back(request);
        LegacyBattleStartupCallReply reply;
        switch (request.call) {
        case LegacyBattleStartupCall::initialize_control_block:
            reply.outputs[0] = 0x12345678;
            break;
        case LegacyBattleStartupCall::query_value: {
            const auto found = query_values.find(request.arguments[0]);
            reply.return_value =
                found == query_values.end() ? 0U : found->second;
            break;
        }
        case LegacyBattleStartupCall::get_window_rectangle:
            reply.outputs = {1, 2, 641, 482};
            break;
        case LegacyBattleStartupCall::lookup_triplet:
            reply.return_value =
                request.arguments[0] == 0x1FU ? 0xAAAA1234U : 0xBBBB5678U;
            break;
        case LegacyBattleStartupCall::system_metric_height:
            reply.return_value = 1080U;
            break;
        case LegacyBattleStartupCall::system_metric_width:
            reply.return_value = 1920U;
            break;
        case LegacyBattleStartupCall::create_display_surface:
            reply.return_value = 0x70000000U + created_surface_count++;
            break;
        case LegacyBattleStartupCall::notify_no_enemies:
            reply.return_value = no_enemy_return;
            break;
        case LegacyBattleStartupCall::random_below:
            if (!random_values.empty()) {
                reply.return_value = random_values.front();
                random_values.pop_front();
            }
            break;
        case LegacyBattleStartupCall::group_b_load_resource_definition:
        case LegacyBattleStartupCall::reserved_group_b_load_action_profile:
        case LegacyBattleStartupCall::group_b_release_resource_text:
            break;
        case LegacyBattleStartupCall::apply_actor_mode:
            reply.ecx_snapshot = 0xBEEF0000U;
            break;
        case LegacyBattleStartupCall::query_party_actor_mode:
            reply.return_value = party_actor_mode_return;
            break;
        case LegacyBattleStartupCall::
            reserved_apply_party_attribute_aggregation:
            break;
        case LegacyBattleStartupCall::query_primary_ratio:
            reply.outputs = {3, 2, 0, 0};
            break;
        case LegacyBattleStartupCall::query_secondary_ratio:
            reply.outputs = {-3, 2, 0, 0};
            break;
        case LegacyBattleStartupCall::query_tertiary_ratio:
            reply.outputs = {5, 0, 0x13579BDF, 0};
            break;
        case LegacyBattleStartupCall::supplemental_seed:
            if (supplemental_modifier_tokens.empty()) {
                reply.return_value = supplemental_modifier_token;
            } else {
                reply.return_value = supplemental_modifier_tokens.front();
                supplemental_modifier_tokens.pop_front();
            }
            break;
        case LegacyBattleStartupCall::group_a_profile_allocate:
            reply.return_value =
                0x71000000U + profile_allocation_count++ * 0xA4U;
            break;
        case LegacyBattleStartupCall::group_a_profile_load:
            reply.publish_group_a_profile_record = true;
            reply.group_a_profile_record = supplemental_profile;
            break;
        case LegacyBattleStartupCall::query_actor_metric:
            reply.publish_metric_word = true;
            reply.metric_word = 1U;
            break;
        case LegacyBattleStartupCall::group_a_embedded_profile_item_quantity: {
            const auto found =
                embedded_profile_item_quantities.find(request.arguments[1U]);
            reply.return_value = found == embedded_profile_item_quantities.end()
                ? 0U
                : found->second;
            reply.ecx_snapshot =
                (request.ecx & 0xFFFF0000U) | request.arguments[1U];
            reply.edx_snapshot = request.edx;
            break;
        }
        default:
            break;
        }
        return reply;
    }

    [[nodiscard]] bool group_b_action_configuration_typed_stop(
        const LegacyBattleStartupCall call
    ) const noexcept override {
        return call ==
            LegacyBattleStartupCall::group_b_load_resource_definition &&
            !publish_enemy_progress_resource;
    }

    [[nodiscard]] std::shared_ptr<const std::array<openswd3::compat::u8, 0xA4>>
    group_b_action_resource_bytes() const override {
        auto bytes = std::make_shared<std::array<openswd3::compat::u8, 0xA4>>();
        (*bytes)[0x5AU] =
            static_cast<openswd3::compat::u8>(enemy_progress_base_speed);
        (*bytes)[0x5BU] =
            static_cast<openswd3::compat::u8>(enemy_progress_base_speed >> 8U);
        return bytes;
    }

    [[nodiscard]] openswd3::battle::LegacyBattleDefinitionArchiveApiReply
    open_archive_file(
        const openswd3::battle::LegacyBattleDefinitionArchiveOpenRequest&
            request
    ) override {
        archive_open_requests.push_back(request);
        ++archive_open_calls;
        if (!archive_open_replies.empty()) {
            const auto reply = archive_open_replies.front();
            archive_open_replies.pop_front();
            return reply;
        }
        return archive_open_reply;
    }

    [[nodiscard]] openswd3::battle::LegacyBattleDefinitionArchiveReadReply
    read_archive_file(
        const openswd3::battle::LegacyBattleDefinitionArchiveReadRequest&
            request,
        const std::span<openswd3::compat::u8> destination
    ) override {
        archive_read_requests.push_back(request);
        std::ranges::fill(destination, 0U);
        if (request.requested_bytes ==
            openswd3::battle::kLegacyBattleDefinitionArchiveHeaderBytes) {
            for (u32 index = 0U; index < 0x40U; ++index) {
                destination[0x1F44U + index] = 1U;
            }
            if (force_definition_offset_stop) {
                destination[0x1F45U] = 0x80U;
            }
        } else {
            const auto write_u16 = [&](const u32 offset, const u16 value) {
                destination[offset] = static_cast<openswd3::compat::u8>(value);
                destination[offset + 1U] =
                    static_cast<openswd3::compat::u8>(value >> 8U);
            };
            const auto write_u32 = [&](const u32 offset, const u32 value) {
                destination[offset] = static_cast<openswd3::compat::u8>(value);
                destination[offset + 1U] =
                    static_cast<openswd3::compat::u8>(value >> 8U);
                destination[offset + 2U] =
                    static_cast<openswd3::compat::u8>(value >> 16U);
                destination[offset + 3U] =
                    static_cast<openswd3::compat::u8>(value >> 24U);
            };
            write_u32(0x04U, std::bit_cast<u32>(definition.rotation_divisor));
            write_u16(0x24U, definition.secondary_count);
            write_u16(0x28U, definition.background_action_id);
            write_u32(0x58U, definition.background_field_b4);
            write_u32(0x78U, definition.background_field_b8);
            write_u16(0x98U, definition.enemy_count);
            for (u32 index = 0U; index < definition.enemies.size(); ++index) {
                write_u16(
                    0x9CU + index * 4U, definition.enemies[index].role_id
                );
                write_u16(
                    0xBCU + index * 2U, definition.enemies[index].mode_flag
                );
                write_u16(
                    0xCCU + index * 4U, definition.enemies[index].position_x
                );
                write_u16(
                    0xECU + index * 4U, definition.enemies[index].position_y
                );
            }
        }
        ++archive_read_calls;
        auto reply = archive_read_reply;
        reply.bytes_read = static_cast<u32>(destination.size());
        return reply;
    }

    [[nodiscard]] openswd3::battle::LegacyBattleDefinitionArchiveApiReply
    seek_archive_file(
        const openswd3::battle::LegacyBattleDefinitionArchiveSeekRequest&
            request
    ) override {
        archive_seek_requests.push_back(request);
        ++archive_seek_calls;
        return archive_seek_reply;
    }

    [[nodiscard]] openswd3::battle::LegacyBattleDefinitionArchiveApiReply
    close_archive_file(
        const openswd3::battle::LegacyBattleDefinitionArchiveCloseRequest&
            request
    ) override {
        archive_close_requests.push_back(request);
        ++archive_close_calls;
        return archive_close_reply;
    }

    [[nodiscard]] LegacyBattleBackgroundImageLoadResult load_image(
        const std::filesystem::path& archive_path,
        const u32 one_based_resource,
        const u32 variant_index
    ) override {
        background_path = archive_path;
        background_resource = one_based_resource;
        background_variant = variant_index;
        ++background_load_calls;
        return {};
    }

    void release_image(const u32 image_token) noexcept override {
        released_images.push_back(image_token);
    }

    void release_owner(const u32 owner_token) noexcept override {
        released_owners.push_back(owner_token);
    }

    [[nodiscard]] LegacyBattleActionRotationUpdateSnapshot
    update_action(openswd3::asset_runtime::LegacyActionRecord&) override {
        return {};
    }

    [[nodiscard]] LegacyBattleMutableFrameImage
    query_frame_image(const u32, const u32) override {
        return {};
    }

    [[nodiscard]] std::size_t
    call_count(const LegacyBattleStartupCall call) const {
        return static_cast<std::size_t>(std::ranges::count_if(
            requests, [call](const LegacyBattleStartupCallRequest& request) {
                return request.call == call;
            }
        ));
    }

    [[nodiscard]] std::vector<u32>
    actor_tokens_for(const LegacyBattleStartupCall call) const {
        std::vector<u32> tokens;
        for (const auto& request : requests) {
            if (request.call == call) {
                tokens.push_back(request.arguments[0]);
            }
        }
        return tokens;
    }

    LegacyBattleDefinition definition{};
    std::unordered_map<u32, u32> query_values;
    std::unordered_map<u32, u32> embedded_profile_item_quantities;
    std::deque<u32> random_values;
    std::deque<u32> supplemental_modifier_tokens;
    std::vector<LegacyBattleStartupCallRequest> requests;
    openswd3::battle::LegacyBattleGroupASummonProfileRecord
        supplemental_profile{};
    std::vector<openswd3::battle::LegacyBattleDefinitionArchiveOpenRequest>
        archive_open_requests;
    std::vector<openswd3::battle::LegacyBattleDefinitionArchiveReadRequest>
        archive_read_requests;
    std::vector<openswd3::battle::LegacyBattleDefinitionArchiveSeekRequest>
        archive_seek_requests;
    std::vector<openswd3::battle::LegacyBattleDefinitionArchiveCloseRequest>
        archive_close_requests;
    std::deque<openswd3::battle::LegacyBattleDefinitionArchiveApiReply>
        archive_open_replies;
    openswd3::battle::LegacyBattleDefinitionArchiveApiReply archive_open_reply{
        .eax = 0x70000001U,
        .ecx = 0x11111111U,
        .edx = 0x22222222U,
    };
    openswd3::battle::LegacyBattleDefinitionArchiveReadReply archive_read_reply{
        .eax = 1U,
        .ecx = 0x33333333U,
        .edx = 0x44444444U,
    };
    openswd3::battle::LegacyBattleDefinitionArchiveApiReply archive_seek_reply{
        .eax = 0x2714U,
        .ecx = 0x77777777U,
        .edx = 0x88888888U,
    };
    openswd3::battle::LegacyBattleDefinitionArchiveApiReply archive_close_reply{
        .eax = 1U,
        .ecx = 0x55555555U,
        .edx = 0x66666666U,
    };
    std::filesystem::path background_path;
    u32 archive_open_calls{};
    u32 archive_read_calls{};
    u32 archive_seek_calls{};
    u32 archive_close_calls{};
    u32 background_resource{};
    u32 background_variant{};
    u32 background_load_calls{};
    u32 created_surface_count{};
    u32 profile_allocation_count{};
    u32 no_enemy_return{0x87654321U};
    u32 party_actor_mode_return{};
    u32 supplemental_modifier_token{};
    u16 enemy_progress_base_speed{400U};
    bool publish_enemy_progress_resource{true};
    bool force_definition_offset_stop{};
    std::vector<u32> released_images;
    std::vector<u32> released_owners;
};

void poison_reset_blocks(LegacyBattleStartupState& state, StartupPorts& port) {
    auto& reset = state.reset;
    reset.block_525470.fill(1U);
    reset.block_4ff168.fill(1U);
    reset.block_524324.fill(1U);
    reset.block_4fe5d4.fill(1U);
    reset.block_52022c.fill(1U);
    reset.block_5214f8.fill(1U);
    state.text_messages.allocations.push_back({.token = 0x78000000U});
    reset.block_524268.fill(1U);
    reset.block_520e90.fill(1U);
    reset.block_4ff0bc.fill(1U);
    reset.block_5242b0.fill(1U);
    port.actor_publication_state().slots.fill(1U);
    reset.block_524420.fill(1U);
    reset.block_53ae90.fill(1U);
    reset.block_5244e8.fill(1U);
    reset.value_4ff0b0 = 1U;
    reset.value_4fe5cc = 1U;
    reset.value_4ff0b4 = 1U;
    reset.value_4fe5d0 = 1U;
    reset.value_4ff0b8 = 1U;
    reset.value_524414 = 1U;
    reset.values_52544c.fill(1U);
    reset.values_502940.fill(1U);
    reset.values_5244d8.fill(1U);
    reset.value_524418 = 1U;
    reset.value_53c048 = 1U;
    port.actor_metric_state().priority_actor_index = 1U;
    reset.value_53bf22 = 1U;
    port.actor_metric_state().group_b_count = 1U;
    for (auto& record : reset.records_524788) {
        record.value_00 = 1U;
        record.value_04 = 1U;
        record.value_08 = 1U;
        record.value_0a = 1U;
        record.value_0c = 1U;
        record.value_10 = 1U;
        record.value_14 = 1U;
        record.value_18 = 1U;
    }
}

template <typename Range>
[[nodiscard]] bool all_equal(const Range& values, const u32 expected) {
    return std::ranges::all_of(values, [expected](const auto value) {
        return static_cast<u32>(value) == expected;
    });
}

[[nodiscard]] LegacyBattleStartupRequest request(const u32 battle_id) {
    return LegacyBattleStartupRequest{
        .battle_id = battle_id,
        .speed_setting = 11,
        .data_root = "game-data",
        .party_role_ids = {101U, 102U, 103U, 104U},
        .party_values = {
            0x11111111U,
            0x22222222U,
            0x33333333U,
            0x44444444U,
        },
    };
}

[[nodiscard]] bool reset_blocks_match(
    const LegacyBattleStartupState& state, const StartupPorts& port
) {
    const auto& reset = state.reset;
    return all_equal(reset.block_525470, 0U) &&
        all_equal(reset.block_4ff168, 0U) &&
        all_equal(reset.block_524324, 0U) &&
        all_equal(reset.block_4fe5d4, 0U) &&
        all_equal(reset.block_52022c, 0U) &&
        all_equal(reset.block_5214f8, 0U) &&
        state.text_messages.allocations.empty() &&
        all_equal(reset.block_524268, 0U) &&
        all_equal(reset.block_520e90, 0U) &&
        all_equal(reset.block_4ff0bc, 0U) &&
        all_equal(reset.block_5242b0, 0xFFFFFFFFU) &&
        all_equal(port.actor_publication_state().slots, 0xFFFFFFFFU) &&
        all_equal(reset.block_524420, 0xFFFFFFFFU) &&
        all_equal(reset.block_53ae90, 0xFFFFFFFFU) &&
        all_equal(reset.block_5244e8, 0xFFFFFFFFU) &&
        std::ranges::all_of(
               reset.records_524788,
               [](const auto& record) {
                   return record.value_00 == 0xFFFFFFFFU &&
                       record.value_04 == 1U && record.value_08 == 1U &&
                       record.value_0a == 0U && record.value_0c == 0U &&
                       record.value_10 == 1U && record.value_14 == 0U &&
                       record.value_18 == 0U;
               }
        ) &&
        reset.value_4ff0b0 == 0U && reset.value_4fe5cc == 0U &&
        reset.value_4ff0b4 == 0U && reset.value_4fe5d0 == 0U &&
        reset.value_4ff0b8 == 0U && reset.value_524414 == 0U &&
        all_equal(reset.values_52544c, 0U) &&
        all_equal(reset.values_502940, 0U) &&
        all_equal(reset.values_5244d8, 0U) && reset.value_524418 == 0U &&
        reset.value_53c048 == 0U &&
        port.actor_metric_state().priority_actor_index == 0xFFFFFFFFU &&
        port.actor_metric_state().group_b_count == 0U &&
        reset.value_53bf22 == 0U;
}

}  // namespace

void test_battle_startup(openswd3::test::Context& test) {
    {
        LegacyBattleStartupState state;
        StartupPorts ports;
        poison_reset_blocks(state, ports);
        state.display_surfaces = {0x11110000U, 0x22220000U};
        state.mode_flags = 0xA5000000U;
        ports.query_values = {
            {30U, 1U},
            {32U, 1U},
            {0x00C9U, 7U},
            {0x1BB0U, 1U},
        };
        static_cast<void>(
            openswd3::battle::initialize_legacy_battle_render_geometry_binding(
                state.render_binding_object
            )
        );
        auto startup_request = request(0xABCD0001U);
        startup_request.window_token = 0x12340000U;
        startup_request.archive_number_of_bytes_read_token = 0x11112222U;
        startup_request.archive_entry_edx_snapshot = 0x33334444U;
        startup_request.definition_record_number_of_bytes_read_token =
            0x55556666U;
        startup_request.definition_record_entry_edx_snapshot = 0x77778888U;

        const auto result = openswd3::battle::initialize_legacy_battle_startup(
            state, ports, ports, ports, ports, ports, ports, startup_request
        );

        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleStartupStatus::no_enemies &&
                result.return_value == 0x87654321U &&
                result.no_enemy_notification_calls == 1U &&
                result.action_threshold == 900 &&
                state.window_token == 0x12340000U &&
                state.battle_id_word == 0x0001U &&
                reset_blocks_match(state, ports) && state.party_count == 2U &&
                state.party_presence ==
                    std::array<openswd3::compat::u8, 4>{1U, 0U, 1U, 0U} &&
                state.action_mode_source.actor_label_indices[0] == 0U &&
                state.action_mode_source.actor_label_indices[1] == 2U &&
                state.mode_flags == 0xA5000002U &&
                state.action_delay == 0x12U &&
                state.control_switches == std::array<u32, 4>{1U, 1U, 1U, 1U} &&
                state.control_value_a == 0x2329U &&
                state.control_value_b == 0x0CU &&
                state.runtime_handle == 0x12345678U &&
                state.window_rectangle == std::array<i32, 4>{1, 2, 641, 482} &&
                state.primary_text_color == 0x1234U &&
                state.secondary_text_color == 0x5678U &&
                state.logical_width == 320U && state.logical_height == 200U &&
                result.released_display_surfaces == 2U &&
                result.created_display_surfaces == 2U &&
                state.display_surfaces ==
                    std::array<u32, 2>{0x70000000U, 0x70000001U} &&
                state.background.completion_words ==
                    std::array<u16, 3>{0xFFFFU, 0xFFFFU, 0xFFFFU} &&
                result.display_surface_return_snapshot == 0xFFFFFFFFU &&
                result.display_completion_write_order ==
                    std::array<openswd3::compat::u8, 3>{0U, 1U, 2U} &&
                result.definition_archive_header.status ==
                    openswd3::battle::
                        LegacyBattleDefinitionArchiveHeaderLoadStatus::
                            completed &&
                result.definition_archive_header.open_calls == 1U &&
                result.definition_archive_header.read_calls == 1U &&
                result.definition_archive_header.close_calls == 1U &&
                result.definition_archive_record.status ==
                    openswd3::battle::
                        LegacyBattleDefinitionArchiveRecordLoadStatus::
                            completed &&
                result.definition_archive_record.battle_index == 1U &&
                result.definition_archive_record.combined_record_index == 0U &&
                result.definition_archive_record.file_offset == 0x2714U &&
                ports.archive_open_calls == 2U &&
                ports.archive_read_calls == 3U &&
                ports.archive_seek_calls == 1U &&
                ports.archive_close_calls == 2U &&
                ports.archive_open_requests.size() == 2U &&
                ports.archive_open_requests[0].path ==
                    std::filesystem::path("game-data/battle.ffd") &&
                ports.archive_open_requests[0].desired_access == 0x80000000U &&
                ports.archive_open_requests[0].share_mode == 0U &&
                ports.archive_open_requests[0].creation_disposition == 3U &&
                ports.archive_open_requests[0].flags_and_attributes == 0x80U &&
                ports.archive_open_requests[0].entry_eax == 0x004AAED0U &&
                ports.archive_open_requests[0].entry_ecx == 0x004FF5B8U &&
                ports.archive_open_requests[0].entry_edx == 0x33334444U &&
                ports.archive_open_requests[1].entry_edx == 0x77778888U &&
                ports.archive_read_requests.size() == 3U &&
                ports.archive_read_requests[0].destination_token ==
                    0x004FF5BCU &&
                ports.archive_read_requests[0].entry_ecx == 0x11112222U &&
                ports.archive_read_requests[1].destination_token ==
                    0x004FF5BCU &&
                ports.archive_read_requests[1].entry_ecx == 0x55556666U &&
                ports.archive_read_requests[2].destination_token ==
                    0x004FF1E0U &&
                ports.archive_read_requests[2].requested_bytes == 0x010CU &&
                ports.archive_seek_requests.size() == 1U &&
                ports.archive_seek_requests[0].distance == 0x2714U &&
                ports.archive_seek_requests[0].move_method == 0U &&
                ports.archive_close_requests.size() == 2U &&
                ports.archive_close_requests[0].entry_eax == 0x005241FCU &&
                ports.archive_close_requests[1].entry_eax == 1U &&
                ports.archive_close_requests[1].entry_ecx == 0x33333333U &&
                ports.archive_close_requests[1].entry_edx == 0x44444444U &&
                state.archive_header_index_token == 0x00501500U &&
                state.render_binding_object.battle_header_bytes.front() == 0U &&
                state.render_binding_object.battle_header_bytes.back() == 0U &&
                state.render_binding_object.index_records.back().ordinal ==
                    29U &&
                state.render_binding_object.index_records.back()
                        .five_step_quarter == 36 &&
                result.definition_load_calls == 1U &&
                result.definition.enemy_count == 0U &&
                ports.call_count(
                    LegacyBattleStartupCall::release_display_surface
                ) == 2U &&
                ports.call_count(
                    LegacyBattleStartupCall::create_display_surface
                ) == 2U &&
                ports.call_count(LegacyBattleStartupCall::notify_no_enemies) ==
                    1U &&
                ports.background_load_calls == 0U,
            "battle startup preserves reset prefix low-word identity display lifecycle and no-enemy eax"
        );
    }

    {
        LegacyBattleStartupState state;
        state.mirror_mode = 1U;
        state.final_subtract_word = 1U;
        state.group_b_lifecycle = std::make_shared<std::array<
            openswd3::battle::LegacyBattleActorGroupBElementState,
            openswd3::battle::kLegacyBattleActorGroupBElementCount>>();
        (*state.group_b_lifecycle)[0U].action_record.prefix[2U] =
            std::byte{0xCA};
        state.group_a_description_record_tokens.fill(0xDEADBEEFU);
        state.group_a_description_text_indices.fill(0xBEEFU);
        state.group_a_configuration_sources[0U].dwords[1U] = 12000U;
        state.group_a_configuration_sources[0U].dwords[4U] = 0x56781234U;
        state.group_a_configuration_sources[1U].dwords[1U] = 9000U;
        state.party[0U]
            .attribute_aggregation.embedded_profile_application.status_bits =
            0xFFFFFFFFU;
        state.party[0U].actor_list.primary_required = 0xFFFFU;
        state.party[0U].actor_list.secondary_required = 0xFFFFU;
        state.party[0U].actor_list.selected_resource_token = 0xFFFFFFFFU;
        state.party[0U].final_processing.completion_latch = 0xFFFFFFFFU;
        state.party[0U].final_processing.profile_buffer.fill(0xFFFFFFFFU);
        state.party[0U].item_effect_application = {
            .cached_profile_item_id = 0x1111U,
            .effect_flags = 0xFFFFFFFFU,
            .action_kind = 0xFFFFU,
            .display_kind = 0x2222U,
            .mode_flags = 0xFFU,
            .activation_latch = 0xEEU,
            .derived_words = {0xFFFFU, 0x3333U, 0x4444U, 0x5555U},
        };
        StartupPorts ports;
        ports.supplemental_modifier_token = 0x004AB790U;
        ports.archive_open_replies.push_back({
            .eax = 0xFFFFFFFFU,
            .ecx = 0x77777777U,
            .edx = 0x88888888U,
        });
        ports.party_actor_mode_return = 1U;
        ports.query_values = {
            {30U, 1U},
            {31U, 1U},
            {34U, 1U},
            {35U, 1U},
        };
        ports.random_values = {1U, 2U, 0U};
        ports.definition.rotation_divisor = 4U;
        ports.definition.enemy_count = 2U;
        ports.definition.enemies[0] = {
            .role_id = 11U,
            .position_x = 100U,
            .position_y = 200U,
            .mode_flag = 1U,
        };
        ports.definition.enemies[1] = {
            .role_id = 12U,
            .position_x = 300U,
            .position_y = 400U,
        };
        auto& player_items = ports.world_item_list_state();
        for (u32 index = 0U; index < player_items.role_item_lists.size();
             ++index) {
            auto& sentinel = player_items.role_item_lists[index]->sentinel;
            sentinel.legacy_token = 0x00620000U + index * 0xB0U;
            sentinel.definition_snapshot[0x48U] = 1U;
        }
        player_items.role_item_lists[0U]->sentinel.definition_snapshot[0x48U] =
            0U;
        auto& embedded_word_profile =
            player_items.role_item_lists[7U]->sentinel.definition_snapshot;
        embedded_word_profile[0x48U] = 52U;
        embedded_word_profile[0x49U] = 0U;
        embedded_word_profile[0x50U] = 9U;
        embedded_word_profile[0x51U] = 0U;
        ports.embedded_profile_item_quantities = {{9U, 20U}};
        player_items.player_inventory_head_token = 0x00600000U;
        auto& high_item = player_items.player_inventory.emplace_back();
        high_item.legacy_token = 0x00600000U;
        high_item.legacy_next_token = 0x006000B0U;
        high_item.item_id = 9U;
        high_item.selected_count = 3U;
        auto& low_item = player_items.player_inventory.emplace_back();
        low_item.legacy_token = 0x006000B0U;
        low_item.item_id = 3U;
        low_item.selected_count = 4U;
        auto& party_items = *player_items.party_item_lists[0U];
        party_items.sentinel.legacy_next_token = 0x00610000U;
        auto& high_party_item = party_items.nodes.emplace_back();
        high_party_item.legacy_token = 0x00610000U;
        high_party_item.legacy_next_token = 0x006100B0U;
        high_party_item.item_id = 8U;
        auto& low_party_item = party_items.nodes.emplace_back();
        low_party_item.legacy_token = 0x006100B0U;
        low_party_item.item_id = 2U;

        const auto result = openswd3::battle::initialize_legacy_battle_startup(
            state, ports, ports, ports, ports, ports, ports, request(7U)
        );
        const auto attribute_diagnostic = std::ranges::find_if(
            ports.requests, [](const LegacyBattleStartupCallRequest& call) {
                return call.call ==
                    LegacyBattleStartupCall::
                        group_a_attribute_missing_primary_diagnostic;
            }
        );
        const auto embedded_profile_quantity = std::ranges::find_if(
            ports.requests, [](const LegacyBattleStartupCallRequest& call) {
                return call.call ==
                    LegacyBattleStartupCall::
                        group_a_embedded_profile_item_quantity;
            }
        );

        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleStartupStatus::completed &&
                result.definition_archive_header.status ==
                    openswd3::battle::
                        LegacyBattleDefinitionArchiveHeaderLoadStatus::
                            open_failed &&
                result.definition_archive_header.read_calls == 0U &&
                result.definition_archive_header.close_calls == 1U &&
                result.definition_archive_record.status ==
                    openswd3::battle::
                        LegacyBattleDefinitionArchiveRecordLoadStatus::
                            completed &&
                result.definition_load_calls == 1U &&
                result.background.status ==
                    openswd3::battle::
                        LegacyBattleBackgroundInitializationStatus::
                            image_load_failed &&
                ports.background_path ==
                    std::filesystem::path("game-data/all_map2.tsw") &&
                ports.background_resource == 2U &&
                ports.background_variant == 0U &&
                result.enemy_actor_count == 2U &&
                state.group_b_lifecycle != nullptr &&
                (*state.group_b_lifecycle)[0U].action_record.action_id == 11U &&
                (*state.group_b_lifecycle)[0U].action_record.prefix[2U] ==
                    std::byte{0xCA} &&
                (*state.group_b_lifecycle)[0U].action_record.position_x ==
                    540U &&
                (*state.group_b_lifecycle)[1U].action_record.position_x ==
                    340U &&
                ports.call_count(
                    LegacyBattleStartupCall::reserved_configure_enemy_actor
                ) == 0U &&
                ports.call_count(
                    LegacyBattleStartupCall::group_b_load_resource_definition
                ) == 2U &&
                ports.call_count(
                    LegacyBattleStartupCall::
                        reserved_group_b_load_action_profile
                ) == 0U &&
                ports.call_count(
                    LegacyBattleStartupCall::group_b_release_resource_text
                ) == 2U &&
                result.initial_party_actor_count == 2U &&
                result.party_configuration_calls == 2U &&
                result.party_configurations[0U].status ==
                    openswd3::battle::LegacyBattleGroupAConfigurationStatus::
                        completed &&
                result.party_configurations[0U]
                        .workspace_reset.upper_workspace_dwords_zeroed ==
                    0xBEU &&
                state.party[0U].configuration.placement_primary[5U] ==
                    0x00960065U &&
                state.party[0U].configuration.placement_primary[6U] ==
                    0x00000113U &&
                state.party[0U].configuration.source_record_token ==
                    0x004AB790U &&
                state.party[1U].configuration.source_record_token ==
                    0x004AB7C8U &&
                static_cast<u16>(
                    state.group_a_configuration_sources[0U].dwords[1U]
                ) == 9999U &&
                static_cast<u16>(
                    state.party[0U].configuration.actor_record[1U]
                ) == 12000U &&
                ports.call_count(
                    LegacyBattleStartupCall::reserved_configure_party_actor
                ) == 0U &&
                ports.call_count(
                    LegacyBattleStartupCall::
                        group_a_missing_placement_diagnostic
                ) == 0U &&
                result.player_item_order.swaps == 1U &&
                result.player_item_order.comparisons == 2U &&
                player_items.player_inventory_head_token == 0x006000B0U &&
                player_items.player_inventory.front().item_id == 3U &&
                player_items.player_inventory.back().item_id == 9U &&
                player_items.player_inventory.front().selected_count == 0U &&
                player_items.player_inventory.back().selected_count == 0U &&
                result.party_item_order.swaps == 1U &&
                result.party_item_order.comparisons == 2U &&
                party_items.sentinel.legacy_next_token == 0x006100B0U &&
                party_items.nodes.front().item_id == 2U &&
                party_items.nodes.back().item_id == 8U &&
                state.party[0].role_id == 101U &&
                state.party[0].position_x == 150U &&
                state.party[0].position_y == 275U &&
                state.party[1].role_id == 102U &&
                state.party[1].position_x == 85U &&
                state.party[1].position_y == 370U &&
                state.party[0].workspace.object_token == 0x005029D0U &&
                state.party[1].workspace.object_token == 0x00505904U &&
                state.group_a_profiles.profile_tokens[0U] ==
                    0x004ACF50U +
                        state.action_mode_source.actor_label_indices[0U] *
                            0x60U &&
                state.group_a_profiles.profile_kinds[0U] == 0x38U &&
                result.party_attribute_aggregation_calls == 2U &&
                result.party_attribute_aggregations[0U].status ==
                    openswd3::battle::
                        LegacyBattleGroupAAttributeAggregationStatus::
                            completed &&
                result.party_attribute_aggregations[0U]
                        .source_records_visited == 16U &&
                result.party_attribute_aggregations[0U]
                        .embedded_profile_apply_calls == 2U &&
                result.party_attribute_aggregations[0U]
                        .embedded_profile_applications[0U]
                        .actor_word_writes == 1U &&
                state.party[0U]
                        .attribute_aggregation.embedded_profile_application
                        .status_bits == 0U &&
                state.party[0U].actor_list.primary_required == 0U &&
                state.party[0U].actor_list.secondary_required == 0U &&
                state.party[0U].actor_list.selected_resource_token == 0U &&
                state.party[0U].final_processing.completion_latch == 0U &&
                state.party[0U].final_processing.profile_buffer[0U] == 0U &&
                state.party[0U].item_effect_application.effect_flags == 0U &&
                state.party[0U].item_effect_application.action_kind == 0U &&
                state.party[0U].item_effect_application.derived_words[0U] ==
                    0U &&
                state.party[0U]
                        .item_effect_application.cached_profile_item_id ==
                    0x1111U &&
                state.party[0U].item_effect_application.display_kind ==
                    0x2222U &&
                state.party[0U].item_effect_application.mode_flags == 0xFFU &&
                state.party[0U].item_effect_application.activation_latch ==
                    0xEEU &&
                state.party[0U].item_effect_application.derived_words[1U] ==
                    0x3333U &&
                state.party[0U].item_effect_application.derived_words[2U] ==
                    0x4444U &&
                state.party[0U].item_effect_application.derived_words[3U] ==
                    0x5555U &&
                state.party[0U].workspace.tail_words[5U] ==
                    openswd3::world_map::kLegacyItemSentinelId &&
                ports.call_count(
                    LegacyBattleStartupCall::
                        reserved_apply_party_attribute_aggregation
                ) == 0U &&
                ports.call_count(
                    LegacyBattleStartupCall::
                        reserved_group_a_embedded_profile_apply
                ) == 0U &&
                ports.call_count(
                    LegacyBattleStartupCall::
                        group_a_embedded_profile_item_quantity
                ) == 1U &&
                ports.call_count(
                    LegacyBattleStartupCall::
                        group_a_attribute_missing_primary_diagnostic
                ) == 1U &&
                attribute_diagnostic != ports.requests.end() &&
                attribute_diagnostic->arguments ==
                    std::array<u32, 4>{0U, 0x004A7C94U, 0x004A7C44U, 0x182U} &&
                attribute_diagnostic->eax ==
                    openswd3::world_map::kLegacyItemSentinelId &&
                embedded_profile_quantity != ports.requests.end() &&
                embedded_profile_quantity->arguments ==
                    std::array<u32, 4>{0x004B8A00U, 9U, 0U, 0U} &&
                embedded_profile_quantity->eax == 9U &&
                embedded_profile_quantity->ecx == 0x005029D0U &&
                embedded_profile_quantity->edx == 0x005029D0U &&
                result.party_value_pair_calls == 2U &&
                state.party[0U].value_pair.primary_value == 0x11111111U &&
                state.party[0U].value_pair.secondary_value == 0x11111111U &&
                result.party_value_pairs[0U].writes == 2U &&
                result.party_value_pairs[0U].return_eax == 0x11111111U &&
                result.party_value_pairs[0U].return_ecx == 0x005029D0U &&
                result.party_value_pairs[0U].return_edx == 0U &&
                ports.call_count(
                    LegacyBattleStartupCall::reserved_apply_party_value
                ) == 0U &&
                result.party_resource_pair_calls == 2U &&
                state.party[0U].resource_pair.primary_token == 0x004A9940U &&
                state.party[0U].resource_pair.secondary_token == 0x004A9940U &&
                result.party_resource_pairs[0U].writes == 2U &&
                result.party_resource_pairs[0U].return_eax == 0x004A9940U &&
                result.party_resource_pairs[0U].return_ecx == 0x005029D0U &&
                result.party_resource_pairs[0U].return_edx == 0U &&
                ports.call_count(
                    LegacyBattleStartupCall::reserved_apply_party_palette
                ) == 0U &&
                state.group_a_description_record_tokens[0U] == 0U &&
                state.group_a_description_record_tokens[1U] == 0U &&
                state.group_a_description_record_tokens[2U] == 0xDEADBEEFU &&
                state.group_a_description_text_indices[0U] == 0U &&
                state.group_a_description_text_indices[1U] == 0U &&
                state.group_a_description_text_indices[2U] == 0xBEEFU &&
                state.party_offsets[0] == 124 && state.party_offsets[2] == 64 &&
                result.supplemental_actor_count == 2U &&
                result.supplemental_materialization_calls == 2U &&
                result.supplemental_materializations[0U].status ==
                    openswd3::battle::
                        LegacyBattleGroupANpcMaterializationStatus::completed &&
                result.supplemental_materializations[1U].status ==
                    openswd3::battle::
                        LegacyBattleGroupANpcMaterializationStatus::completed &&
                ports.call_count(
                    LegacyBattleStartupCall::
                        reserved_configure_supplemental_actor
                ) == 0U &&
                ports.call_count(
                    LegacyBattleStartupCall::group_a_profile_allocate
                ) == 2U &&
                ports.call_count(
                    LegacyBattleStartupCall::group_a_profile_load
                ) == 2U &&
                ports.call_count(
                    LegacyBattleStartupCall::group_a_profile_release
                ) == 2U &&
                state.party_count == 4U && state.party[2].role_id == 3U &&
                state.party[3].role_id == 4U &&
                state.party[2].position_x == 0xFF92U &&
                state.party[3].position_x == 0xFF92U &&
                state.party[2].configuration.profile_token == 0x71000000U &&
                state.party[3].configuration.profile_token == 0x710000A4U &&
                state.party[2].configuration.source_record_token ==
                    state.party[2].configuration.actor_record_token &&
                state.supplemental_count_word == 2U &&
                state.party_metrics[0].primary_ratio_a == 84U &&
                state.party_metrics[0].primary_ratio_b == 84U &&
                state.party_metrics[0].primary_numerator == 3 &&
                state.party_metrics[0].secondary_ratio_a == 0xFFFFFFACU &&
                state.party_metrics[0].secondary_numerator == -3 &&
                state.party_metrics[0].tertiary_ratio_a == 0U &&
                state.party_metrics[0].tertiary_numerator == 5 &&
                state.party_metrics[0].actor_value_a == 0x13579BDFU &&
                result.enemy_action_advance_calls == 2U &&
                ports.call_count(
                    LegacyBattleStartupCall::reserved_advance_enemy_action
                ) == 0U &&
                state.group_b_lifecycle != nullptr &&
                (*state.group_b_lifecycle)[0U].resource_token == 0x73000000U &&
                state.enemies[0U].progress.progress == 200U &&
                result.finalized_party_actor_count == 4U &&
                state.party_actor_mode_count == 2U &&
                result.return_value == 1U && result.message_state_published &&
                ports.battle_message_state() == 0x67U &&
                ports.call_count(LegacyBattleStartupCall::set_enemy_mode) ==
                    1U &&
                std::ranges::any_of(
                    ports.requests,
                    [](const LegacyBattleStartupCallRequest& call) {
                        return call.call ==
                            LegacyBattleStartupCall::
                                group_b_load_resource_definition &&
                            call.arguments[1] == 0xBEEF000BU;
                    }
                ) &&
                ports.call_count(LegacyBattleStartupCall::apply_actor_mode) ==
                    4U &&
                ports.call_count(LegacyBattleStartupCall::query_actor_metric) ==
                    6U &&
                result.actor_metric_calls == 6U &&
                result.actor_order_selections == 6U &&
                result.group_b_order_copies == 2U &&
                ports.actor_metric_state().group_b_order[0] == 0U &&
                ports.actor_metric_state().group_b_order[1] == 1U,
            "battle startup continues after background load zero and preserves enemy party ratio supplement and final unsigned state"
        );
    }

    {
        LegacyBattleStartupState state;
        state.supplemental_count_word = 1U;
        state.party[0U].configuration.source_record_token = 0x004AB790U;
        state.group_a_configuration_sources[0U].dwords[4U] = 0x12345678U;
        StartupPorts ports;
        ports.supplemental_modifier_tokens = {0x004AB790U, 0x005029D0U};
        ports.query_values = {{34U, 1U}, {35U, 1U}};
        ports.random_values = {0U, 1U, 1U, 0U, 0U};
        ports.definition.enemy_count = 1U;

        const auto result = openswd3::battle::initialize_legacy_battle_startup(
            state, ports, ports, ports, ports, ports, ports, request(8U)
        );

        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleStartupStatus::completed &&
                result.initial_party_actor_count == 0U &&
                result.supplemental_actor_count == 2U &&
                result.supplemental_materialization_calls == 2U &&
                result.supplemental_materializations[0U].status ==
                    openswd3::battle::
                        LegacyBattleGroupANpcMaterializationStatus::completed &&
                result.supplemental_materializations[1U].status ==
                    openswd3::battle::
                        LegacyBattleGroupANpcMaterializationStatus::completed &&
                state.party_count == 2U && state.party[0].role_id == 4U &&
                state.party[1].role_id == 3U &&
                state.supplemental_used[1] == 1U &&
                state.supplemental_used[0] == 1U &&
                ports.call_count(LegacyBattleStartupCall::random_below) == 5U &&
                ports.call_count(
                    LegacyBattleStartupCall::
                        reserved_configure_supplemental_actor
                ) == 0U &&
                ports.call_count(
                    LegacyBattleStartupCall::group_a_profile_allocate
                ) == 2U &&
                ports.call_count(
                    LegacyBattleStartupCall::group_a_profile_load
                ) == 2U &&
                ports.call_count(
                    LegacyBattleStartupCall::group_a_profile_release
                ) == 2U &&
                ports.call_count(LegacyBattleStartupCall::apply_actor_mode) ==
                    2U &&
                result.return_value == 0U && result.message_state_published &&
                ports.battle_message_state() == 0x67U,
            "stale supplemental word selects random branch and materializes both retry-selected NPC actors"
        );
    }

    {
        LegacyBattleStartupState state;
        state.supplemental_count_word = 1U;
        StartupPorts ports;
        ports.supplemental_modifier_token = 0U;
        ports.query_values = {{34U, 1U}};
        ports.random_values = {0U, 0U};
        ports.definition.enemy_count = 1U;

        const auto result = openswd3::battle::initialize_legacy_battle_startup(
            state, ports, ports, ports, ports, ports, ports, request(8U)
        );

        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleStartupStatus::
                        supplemental_materialization_typed_stop &&
                result.supplemental_actor_count == 0U &&
                result.supplemental_materialization_calls == 1U &&
                result.supplemental_materializations[0U].status ==
                    openswd3::battle::
                        LegacyBattleGroupANpcMaterializationStatus::
                            modifier_record_typed_stop &&
                state.party_count == 0U && state.party[0].role_id == 3U &&
                state.party[0].configuration.profile_token == 0x71000000U &&
                state.party[0].configuration.placement_word == 3U &&
                ports.call_count(
                    LegacyBattleStartupCall::
                        reserved_configure_supplemental_actor
                ) == 0U &&
                ports.call_count(LegacyBattleStartupCall::apply_actor_mode) ==
                    0U &&
                !result.message_state_published,
            "zero supplemental modifier stops after profile and placement publication before actor activation"
        );
    }

    {
        constexpr std::array<std::array<u16, 8>, 4> expected_positions{
            std::array<u16, 8>{0x020FU, 0x011FU, 0U, 0U, 0U, 0U, 0U, 0U},
            std::array<u16, 8>{
                0x01EAU,
                0x0113U,
                0x022BU,
                0x0172U,
                0U,
                0U,
                0U,
                0U,
            },
            std::array<u16, 8>{
                0x01F8U,
                0x0110U,
                0x0235U,
                0x0161U,
                0x01CEU,
                0x00E0U,
                0U,
                0U,
            },
            std::array<u16, 8>{
                0x020EU,
                0x012AU,
                0x01F1U,
                0x0115U,
                0x01D0U,
                0x00D9U,
                0x024CU,
                0x0167U,
            },
        };
        bool positions_match = true;
        for (u32 count = 1U; count <= 4U; ++count) {
            LegacyBattleStartupState state;
            StartupPorts ports;
            for (u32 index = 0U; index < count; ++index) {
                ports.query_values[30U + index] = 1U;
            }
            ports.random_values = {0U, 0U};
            ports.definition.enemy_count = 1U;
            const auto result =
                openswd3::battle::initialize_legacy_battle_startup(
                    state,
                    ports,
                    ports,
                    ports,
                    ports,
                    ports,
                    ports,
                    request(9U + count)
                );
            positions_match = positions_match &&
                result.status ==
                    openswd3::battle::LegacyBattleStartupStatus::completed;
            for (u32 index = 0U; index < 4U; ++index) {
                positions_match = positions_match &&
                    state.party[index].position_x ==
                        expected_positions[count - 1U][index * 2U] &&
                    state.party[index].position_y ==
                        expected_positions[count - 1U][index * 2U + 1U];
            }
        }
        test.expect_true(
            positions_match,
            "one through four party layouts preserve all fixed coordinate branches and stale unused slots"
        );
    }

    {
        LegacyBattleStartupState state;
        StartupPorts ports;
        ports.query_values[30U] = 1U;
        ports.random_values = {0U, 0U};
        ports.definition.enemy_count = 1U;
        auto startup_request = request(14U);
        startup_request.party_role_ids[0U] = 0U;
        startup_request.window_token = 0x76543210U;
        const auto result = openswd3::battle::initialize_legacy_battle_startup(
            state, ports, ports, ports, ports, ports, ports, startup_request
        );
        const auto diagnostic = std::ranges::find_if(
            ports.requests, [](const LegacyBattleStartupCallRequest& call) {
                return call.call ==
                    LegacyBattleStartupCall::
                        group_a_missing_placement_diagnostic;
            }
        );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleStartupStatus::completed &&
                result.party_configuration_calls == 1U &&
                result.party_configurations[0U].diagnostic_calls == 1U &&
                diagnostic != ports.requests.end() &&
                diagnostic->arguments ==
                    std::array<u32, 4>{
                        0x76543210U, 0x004A7C2CU, 0x004A7C44U, 0xDEU
                    } &&
                diagnostic->eax == 0U &&
                ports.call_count(
                    LegacyBattleStartupCall::reserved_configure_party_actor
                ) == 0U,
            "startup directly configures group-A actors and forwards the zero-role diagnostic in place"
        );
    }

    {
        LegacyBattleStartupState state;
        StartupPorts ports;
        ports.definition.enemy_count = 1U;
        ports.random_values = {0U};
        ports.world_item_list_state().player_inventory_head_token = 0x00700000U;
        const auto result = openswd3::battle::initialize_legacy_battle_startup(
            state, ports, ports, ports, ports, ports, ports, request(20U)
        );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleStartupStatus::
                        player_item_order_typed_stop &&
                result.player_item_order.status ==
                    openswd3::battle::LegacyBattlePlayerItemOrderStatus::
                        item_node_typed_stop &&
                result.player_item_order.fault_token == 0x00700000U &&
                result.party_item_order.lists_visited == 0U,
            "player-item order typed stop blocks party-item sorting and later startup phases"
        );
    }

    {
        LegacyBattleStartupState state;
        StartupPorts ports;
        ports.query_values = {{30U, 1U}};
        ports.definition.enemy_count = 1U;
        ports.random_values = {0U};
        ports.world_item_list_state().party_item_lists[0U].reset();
        const auto result = openswd3::battle::initialize_legacy_battle_startup(
            state, ports, ports, ports, ports, ports, ports, request(21U)
        );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleStartupStatus::
                        party_item_order_typed_stop &&
                result.party_item_order.status ==
                    openswd3::battle::LegacyBattlePartyItemOrderStatus::
                        list_root_typed_stop &&
                result.party_item_order.fault_list_index == 0U &&
                result.initial_party_actor_count == 1U &&
                ports.call_count(
                    LegacyBattleStartupCall::
                        reserved_apply_party_attribute_aggregation
                ) == 0U,
            "party-item root typed stop blocks profile binding after preserving actor configuration"
        );
    }

    {
        LegacyBattleStartupState state;
        StartupPorts ports;
        ports.query_values = {{30U, 1U}};
        ports.definition.enemy_count = 1U;
        ports.random_values = {0U};
        ports.world_item_list_state().role_item_lists[0U].reset();
        const auto result = openswd3::battle::initialize_legacy_battle_startup(
            state, ports, ports, ports, ports, ports, ports, request(22U)
        );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleStartupStatus::
                        party_attribute_aggregation_typed_stop &&
                result.party_attribute_aggregation_calls == 1U &&
                result.party_attribute_aggregations[0U].status ==
                    openswd3::battle::
                        LegacyBattleGroupAAttributeAggregationStatus::
                            source_record_typed_stop &&
                result.party_attribute_aggregations[0U].fault_source_index ==
                    0U &&
                result.party_attribute_aggregations[0U]
                        .embedded_profile_dwords_zeroed == 82U &&
                result.party_value_pair_calls == 0U &&
                ports.call_count(
                    LegacyBattleStartupCall::
                        reserved_apply_party_attribute_aggregation
                ) == 0U &&
                ports.call_count(
                    LegacyBattleStartupCall::
                        reserved_group_a_embedded_profile_apply
                ) == 0U &&
                ports.call_count(
                    LegacyBattleStartupCall::
                        group_a_embedded_profile_item_quantity
                ) == 0U,
            "missing role-item sentinel stops the direct attribute aggregation before value and resource publication"
        );
    }

    {
        LegacyBattleStartupState state;
        StartupPorts ports;
        ports.publish_enemy_progress_resource = false;
        ports.definition.enemy_count = 1U;
        ports.random_values = {0U, 1U};
        const auto result = openswd3::battle::initialize_legacy_battle_startup(
            state, ports, ports, ports, ports, ports, ports, request(20U)
        );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleStartupStatus::
                        enemy_action_configuration_typed_stop &&
                result.enemy_action_advance_calls == 0U &&
                ports.call_count(
                    LegacyBattleStartupCall::reserved_configure_enemy_actor
                ) == 0U &&
                ports.call_count(
                    LegacyBattleStartupCall::reserved_advance_enemy_action
                ) == 0U &&
                state.group_b_lifecycle != nullptr &&
                (*state.group_b_lifecycle)[0U]
                        .action_configuration.timing_value == 0U &&
                state.enemies[0U].progress.progress == 0U,
            "startup propagates the group B resource loader stop after the record copy prefix"
        );
    }

    {
        LegacyBattleStartupState state;
        StartupPorts ports;
        ports.definition.enemy_count = 9U;
        ports.random_values = {0U};
        const auto result = openswd3::battle::initialize_legacy_battle_startup(
            state, ports, ports, ports, ports, ports, ports, request(20U)
        );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleStartupStatus::
                        enemy_index_out_of_range &&
                result.enemy_actor_count == 8U &&
                ports.actor_tokens_for(LegacyBattleStartupCall::reset_actor) ==
                    std::vector<u32>{
                        0x00525508U,
                        0x00528030U,
                        0x0052AB58U,
                        0x0052D680U,
                        0x005301A8U,
                        0x00532CD0U,
                        0x005357F8U,
                        0x00538320U,
                    },
            "ninth enemy stops at the first actor object access after eight legacy side-effect prefixes"
        );
    }

    {
        LegacyBattleStartupState state;
        StartupPorts ports;
        ports.force_definition_offset_stop = true;
        const auto result = openswd3::battle::initialize_legacy_battle_startup(
            state, ports, ports, ports, ports, ports, ports, request(20U)
        );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleStartupStatus::
                        definition_archive_typed_stop &&
                result.definition_archive_record.status ==
                    openswd3::battle::
                        LegacyBattleDefinitionArchiveRecordLoadStatus::
                            offset_table_typed_stop &&
                result.definition_load_calls == 0U &&
                result.no_enemy_notification_calls == 0U &&
                ports.background_load_calls == 0U,
            "definition offset typed stop preserves the loaded header and blocks every later startup phase"
        );
    }
}
