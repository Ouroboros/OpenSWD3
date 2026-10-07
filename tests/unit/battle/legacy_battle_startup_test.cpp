#include "legacy_battle_mon_database_fixture.hpp"
#include "openswd3/battle/legacy_battle_action_rotation_resources.hpp"
#include "openswd3/battle/legacy_battle_display_surface_runtime.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"
#include "openswd3/battle/legacy_battle_target_selection_runtime.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <deque>
#include <functional>
#include <memory>
#include <optional>
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
      public virtual openswd3::battle::LegacyBattleTargetSelectionRuntimeStatePort,
      public openswd3::battle::LegacyBattleDefinitionArchiveFilePort,
      public openswd3::battle::LegacyBattleBackgroundImageLoadPort,
      public openswd3::battle::LegacyBattleActionRotationReleasePort,
      public openswd3::battle::LegacyBattleActionRotationUpdatePort,
      public openswd3::battle::LegacyBattleMutableFrameImagePort,
      public openswd3::test::LegacyBattleMonDatabaseFixture {
public:
    std::array<openswd3::battle::LegacyBattleGroupAConfigurationSourceRecord, 4>
        primary_party_sources{};

    std::span<std::byte>
    party_configuration_source(const u32 index) noexcept override {
        return std::as_writable_bytes(
            std::span{primary_party_sources[index].dwords}
        );
    }

    std::array<openswd3::asset_runtime::LegacyActionRecord, 3> dialog_actions{};
    std::vector<openswd3::asset_runtime::LegacyActionRecord> control_snapshots;
    std::vector<std::array<i32, 4>> mouse_rebase_snapshots;
    LegacyBattleStartupState* observed_display_state{};
    std::vector<std::array<u32, 5>> display_snapshots;
    std::deque<u32> display_create_replies;
    u32 unresolved_display_token{};
    u32 stopped_actor_reset_token{};
    std::function<void()> attribute_diagnostic;

    [[nodiscard]] std::optional<u32>
    release_battle_display_surface(const u32 token) override {
        if (token == unresolved_display_token) {
            return std::nullopt;
        }

        return LegacyBattleStartupPort::release_battle_display_surface(token);
    }

    openswd3::input_time_rng::LegacyMouseState mouse_device{
        .sensitivity_scale = 20,
    };
    openswd3::input_time_rng::LegacyMouseDeviceSample mouse_sample{
        .absolute_x = -400,
        .absolute_y = 800,
    };

    [[nodiscard]] openswd3::asset_runtime::LegacyActionRecord&
    battle_control_action() noexcept override {
        return dialog_actions[1];
    }

    StartupPorts() {
        for (auto& enemy : definition.enemies) {
            enemy.position_x = 1U;
            enemy.position_y = 1U;
        }
    }

    [[nodiscard]] LegacyBattleStartupCallReply
    invoke(const LegacyBattleStartupCallRequest& request) override {
        requests.push_back(request);
        if (request.call ==
                LegacyBattleStartupCall::
                    group_a_attribute_missing_primary_diagnostic &&
            attribute_diagnostic) {
            attribute_diagnostic();
        }

        if (observed_display_state != nullptr &&
            (request.call == LegacyBattleStartupCall::release_display_surface ||
             request.call == LegacyBattleStartupCall::create_display_surface)) {
            const auto& state = *observed_display_state;
            display_snapshots.push_back({
                state.display_surfaces[0],
                state.display_surfaces[1],
                state.background.completion_words[0],
                state.background.completion_words[1],
                state.background.completion_words[2],
            });
        }

        LegacyBattleStartupCallReply reply;
        switch (request.call) {
        case LegacyBattleStartupCall::read_transparent_pixel_pair:
            control_snapshots.push_back(battle_control_action());
            reset_observations.push_back({
                actor_metric_state().group_b_count,
                battle_target_selection_runtime_state().special_action_count,
            });
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

        case LegacyBattleStartupCall::rebase_mouse_coordinates:
            mouse_rebase_snapshots.push_back({
                mouse_frame_state().logical_x,
                battle_frame_input_resolution_state().previous_mouse_x,
                mouse_frame_state().logical_y,
                battle_frame_input_resolution_state().previous_mouse_y,
            });
            openswd3::input_time_rng::rebase_mouse_coordinates(
                mouse_device,
                mouse_sample,
                std::bit_cast<i32>(request.arguments[1]),
                std::bit_cast<i32>(request.arguments[2])
            );
            break;

        case LegacyBattleStartupCall::system_metric_height:
            reply.return_value = 1080U;
            break;
        case LegacyBattleStartupCall::system_metric_width:
            reply.return_value = 1920U;
            break;
        case LegacyBattleStartupCall::create_display_surface:
            if (display_create_replies.empty()) {
                reply.return_value = 0x70000000U + created_surface_count++;
            } else {
                reply.return_value = display_create_replies.front();
                display_create_replies.pop_front();
            }

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
        case LegacyBattleStartupCall::reserved_group_b_release_resource_text:
            break;
        case LegacyBattleStartupCall::reset_actor:
            reply.typed_stop =
                request.arguments[0] == stopped_actor_reset_token;
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
        case LegacyBattleStartupCall::reserved_supplemental_seed:
            break;
        case LegacyBattleStartupCall::group_a_profile_allocate:
            reply.return_value =
                0x71000000U + profile_allocation_count++ * 0xA4U;
            break;
        case LegacyBattleStartupCall::group_a_profile_load:
            reply.publish_group_a_profile_record = true;
            reply.group_a_profile_record = supplemental_profile;
            break;
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
            write_u32(0x04U, definition.background_resource);
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
    std::deque<u32> random_values;
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
    std::vector<std::array<u32, 2>> reset_observations;
    u32 party_actor_mode_return{};
    u16 enemy_progress_base_speed{400U};
    bool publish_enemy_progress_resource{true};
    bool force_definition_offset_stop{};
    std::vector<u32> released_images;
    std::vector<u32> released_owners;

protected:
    [[nodiscard]] std::optional<bool> prepare_definition_record(
        const std::span<openswd3::compat::u8> destination, const u32
    ) noexcept override {
        std::ranges::fill(destination, 0U);
        destination[0x5AU] =
            static_cast<openswd3::compat::u8>(enemy_progress_base_speed);
        destination[0x5BU] =
            static_cast<openswd3::compat::u8>(enemy_progress_base_speed >> 8U);
        return true;
    }
};

void poison_reset_blocks(LegacyBattleStartupState& state, StartupPorts& port) {
    port.borrow_actor_metric_state(state.actor_metrics);
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
        all_equal(reset.block_5242b0, 0U) &&
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
        auto state = std::make_unique<LegacyBattleStartupState>();
        auto action = std::make_unique<
            openswd3::battle::LegacyBattleGroupAActionExecutionState>();
        openswd3::world_map::LegacyWorldItemListState items;
        std::array<openswd3::compat::u8, 64U> storage{};
        const openswd3::battle::LegacyBattlePartyNameSources names{
            std::span{storage}.subspan(0U, 16U),
            std::span{storage}.subspan(16U, 16U),
            std::span{storage}.subspan(32U, 16U),
            std::span{storage}.subspan(48U, 16U),
        };
        auto& party = state->party[0U];
        party.value_pair = {0x11111111U, 0x22222222U};
        party.resource_pair = {0x33333333U, 0x44444444U};
        state->action_mode_source.actor_label_indices[0U] = 3U;
        items.party_item_lists[3U]->legacy_head_token = 0xDEADBEEFU;
        items.player_inventory_head_token = 0x77112233U;
        const auto first =
            openswd3::battle::bind_legacy_battle_startup_party_references(
                *state, 0U, items, names, action.get()
            );
        storage[48U] = 0x77U;
        test.expect_true(
            first.status ==
                    openswd3::battle::LegacyBattleStartupStatus::completed &&
                first.value_pair.writes == 2U &&
                first.resource_pair.writes == 2U &&
                action->current_list_index == 0xDEADBEEFU &&
                action->next_list_index == 0xDEADBEEFU &&
                party.actor_list.resource_head_token == 0x004A9940U &&
                party.actor_list.next_resource_head_token == 0x004A9940U &&
                party.name_token == 0x0049E178U &&
                party.name_bytes.data() == storage.data() + 48U &&
                party.name_bytes.front() == 0x77U &&
                party.value_pair.primary_value == 0x11111111U &&
                party.value_pair.secondary_value == 0x22222222U &&
                party.resource_pair.primary_token == 0x33333333U &&
                party.resource_pair.secondary_token == 0x44444444U,
            "startup binds actual action/list fields and borrows name bytes without writing a second pair or dereferencing item roots"
        );
        state->action_mode_source.actor_label_indices[0U] = 1U;
        items.party_item_lists[1U].reset();
        const auto repeated =
            openswd3::battle::bind_legacy_battle_startup_party_references(
                *state, 0U, items, names, action.get()
            );
        test.expect_true(
            repeated.status ==
                    openswd3::battle::LegacyBattleStartupStatus::completed &&
                action->current_list_index == 0U &&
                action->next_list_index == 0U &&
                party.name_token == 0x0049E158U &&
                party.name_bytes.data() == storage.data() + 16U &&
                repeated.resource_pair.return_edx == 1U,
            "repeated binding publishes a null party root and refreshes name mapping while preserving source EDX"
        );
        state->action_mode_source.actor_label_indices[0U] = 4U;
        const auto stopped =
            openswd3::battle::bind_legacy_battle_startup_party_references(
                *state, 0U, items, names, action.get()
            );
        test.expect_true(
            stopped.status ==
                    openswd3::battle::LegacyBattleStartupStatus::
                        party_source_index_out_of_range &&
                stopped.value_pair.writes == 0U &&
                stopped.resource_pair.writes == 0U &&
                party.name_token == 0x0049E158U &&
                action->current_list_index == 0U,
            "unmapped party root table reads stop before overwriting existing references"
        );
    }

    for (const bool unmapped_after_diagnostic : {false, true}) {
        auto state = std::make_unique<LegacyBattleStartupState>();
        StartupPorts ports;
        ports.definition.enemy_count = 1U;
        ports.query_values = {{30U, 1U}, {31U, 1U}};
        u32 diagnostics{};
        bool first_references_visible{};
        ports.attribute_diagnostic = [&] {
            if (diagnostics == 0U) {
                state->action_mode_source.actor_label_indices[0U] =
                    unmapped_after_diagnostic ? 4U : 2U;
            } else {
                first_references_visible =
                    state->party[0U].name_token == 0x0049E168U &&
                    state->party[0U].value_pair.primary_value ==
                        ports.world_item_list_state()
                            .party_item_lists[2U]
                            ->legacy_head_token;
                state->action_mode_source.actor_label_indices[1U] = 3U;
            }

            ++diagnostics;
        };
        const auto result = openswd3::battle::initialize_legacy_battle_startup(
            *state, ports, ports, ports, ports, ports, ports, request(1U)
        );
        if (unmapped_after_diagnostic) {
            test.expect_true(
                result.status ==
                        openswd3::battle::LegacyBattleStartupStatus::
                            party_source_index_out_of_range &&
                    diagnostics == 1U &&
                    result.party_attribute_aggregation_calls == 1U &&
                    result.party_attribute_aggregations[0U]
                            .primary_profile_dwords_copied == 41U &&
                    result.party_value_pair_calls == 0U &&
                    result.party_resource_pair_calls == 0U &&
                    state->party[0U].name_token == 0U &&
                    state->party[1U].name_token == 0U,
                "post-diagnostic source failure retains attributes and prevents references and the next actor"
            );
            continue;
        }

        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleStartupStatus::completed &&
                diagnostics == 2U && first_references_visible &&
                result.party_value_pair_calls == 2U &&
                result.party_value_pairs[0U].return_edx == 2U &&
                result.party_value_pairs[1U].return_edx == 3U &&
                state->party[1U].name_token == 0x0049E178U,
            "each actor binds the post-diagnostic mapping before the next actor begins attributes"
        );
    }

    {
        const openswd3::battle::LegacyBattleDefinition definition{
            .background_resource = 0xFEDC1234U,
            .secondary_count = 0xABCDU,
            .background_action_id = 0x9876U,
            .background_field_b4 = 0x13572468U,
            .background_field_b8 = 0x24681357U,
        };
        for (u32 random_value = 0U; random_value < 4U; ++random_value) {
            const auto background =
                openswd3::battle::make_legacy_battle_startup_background_request(
                    definition, "game-data", random_value
                );
            test.expect_true(
                background.data_root == std::filesystem::path("game-data") &&
                    background.one_based_resource == 0xFEDC1234U &&
                    background.initial_action_id == 0x9876U &&
                    background.field_b4 == 0x13572468U &&
                    background.field_b8 == 0x24681357U &&
                    background.rotation_divisor ==
                        static_cast<i32>(random_value + 1U) &&
                    background.background_action_gate == 0xABCDU,
                "startup keeps the full definition resource and varies only " "the fifth background argument for all four random draws"
            );
        }
    }

    for (const bool stop_enemy : {true, false}) {
        LegacyBattleStartupState state;
        StartupPorts ports;
        ports.definition.enemy_count = 1U;
        ports.query_values = {{30U, 1U}, {31U, 1U}};
        ports.stopped_actor_reset_token = stop_enemy
            ? openswd3::battle::kLegacyBattleActorGroupBBaseToken
            : openswd3::battle::kLegacyBattleActorGroupABaseToken;
        state.enemy_scratch.fill(0xA5U);
        state.party[0].final_processing.completion_latch = 0x12345678U;
        const auto result = openswd3::battle::initialize_legacy_battle_startup(
            state, ports, ports, ports, ports, ports, ports, request(1U)
        );
        test.expect_equal(
            result.status,
            openswd3::battle::LegacyBattleStartupStatus::actor_reset_typed_stop,
            "startup propagates enemy and party reset stops"
        );
        test.expect_equal(
            ports.requests.back().call, LegacyBattleStartupCall::reset_actor,
            "no subsequent host call follows reset stop"
        );
        test.expect_equal(result.enemy_actor_count, stop_enemy ? 0U : 1U,
                          "only configured enemy prefix is counted");
        test.expect_equal(result.party_configuration_calls, 0U,
                          "party configuration does not follow stopped reset");
        test.expect_equal(
            state.party[0].final_processing.completion_latch, 0x12345678U,
            "party reset suffix remains untouched"
        );
        if (stop_enemy) {
            test.expect_true(all_equal(state.enemy_scratch, 0xA5U),
                             "enemy scratch clear follows successful reset");
            test.expect_true(state.group_b_lifecycle == nullptr,
                             "stopped reset does not create replacement actors");
        }
    }

    {
        LegacyBattleStartupState state;
        StartupPorts ports;
        state.display_surfaces = {11U, 22U};
        state.actor_metrics.group_b_count = 7U;
        state.reset.value_4ff0b0 = 9U;
        state.render_geometry.primary_row_offsets = std::make_unique<u32[]>(2U);
        state.render_geometry.primary_row_offsets[0] = 0x1234U;
        const auto* const rows =
            state.render_geometry.primary_row_offsets.get();
        const auto actors = state.group_a_runtime_reset;
        ports.battle_target_selection_runtime_state().special_action_count = 9U;
        openswd3::battle::reset_legacy_battle_startup_blocks(
            state,
            ports.actor_publication_state(),
            state.actor_metrics,
            ports.battle_target_selection_runtime_state()
        );
        test.expect_true(
            state.display_surfaces == std::array<u32, 2>{11U, 22U} &&
                state.render_geometry.primary_row_offsets.get() == rows &&
                state.render_geometry.primary_row_offsets[0] == 0x1234U &&
                state.group_a_runtime_reset == actors &&
                state.actor_metrics.group_b_count == 7U &&
                state.actor_metrics.priority_actor_index == 0xFFFFFFFFU &&
                state.reset.value_4ff0b0 == 0U &&
                ports.battle_target_selection_runtime_state()
                        .special_action_count == 0U,
            "startup selective reset retains old display and actor owners"
        );
    }

    {
        openswd3::rendering::LegacySurfaceGeometry display{
            .pitch_bytes = 6,
            .width = 3,
            .height = 2,
        };
        openswd3::battle::LegacyBattleDisplaySurfaceRuntime surfaces(display);
        LegacyBattleStartupState state;
        std::array<u32, 2> retired{};
        for (u32 cycle = 0U; cycle < 2U; ++cycle) {
            const auto created =
                openswd3::battle::create_legacy_battle_display_surfaces(
                    state, surfaces
                );
            auto* first = surfaces.find(state.display_surfaces[0]);
            auto* second = surfaces.find(state.display_surfaces[1]);
            test.expect_true(
                created.create_calls == 2U &&
                    created.return_value == 0xFFFFFFFFU &&
                    created.completion_write_order ==
                        std::array<openswd3::compat::u8, 3>{0U, 1U, 2U} &&
                    first != nullptr && second != nullptr && first != second &&
                    surfaces.live_surface_count() == 2U &&
                    surfaces.allocated_bytes() == 24U * (cycle + 1U) &&
                    surfaces.find(retired[0]) == nullptr &&
                    surfaces.find(retired[1]) == nullptr,
                "display surfaces own distinct storage across battle entries"
            );
            if (first != nullptr && second != nullptr) {
                test.expect_true(
                    first->geometry.width == 3 && first->geometry.height == 2 &&
                        first->geometry.pitch_bytes == 6 &&
                        first->pixels.size() == 6U &&
                        second->pixels.size() == 6U,
                    "display storage uses the requested 16-bit geometry"
                );
                first->pixels[0] = 0x1234U;
                second->pixels[0] = 0xABCDU;
                test.expect_true(
                    first->pixels[0] == 0x1234U && second->pixels[0] == 0xABCDU,
                    "display snapshots do not alias each other's pixels"
                );
            }

            retired = state.display_surfaces;
            const auto released =
                openswd3::battle::release_legacy_battle_display_surfaces(
                    state, surfaces
                );
            test.expect_true(
                !released.typed_stop && released.release_calls == 2U &&
                    state.display_surfaces == std::array<u32, 2>{0U, 0U} &&
                    surfaces.live_surface_count() == 0U &&
                    surfaces.find(retired[0]) == nullptr &&
                    surfaces.find(retired[1]) == nullptr &&
                    surfaces.allocated_bytes() == 24U * (cycle + 1U),
                "display release destroys backing storage without decrementing"
            );
        }

        // A failed creation is published as zero in both slots and still
        // performs the completion writes. Metrics come from the same owner.
        display.height = 0;
        state.background.completion_words = {1U, 2U, 3U};
        const auto failed =
            openswd3::battle::create_legacy_battle_display_surfaces(
                state, surfaces
            );
        test.expect_true(
            failed.create_calls == 2U &&
                state.display_surfaces == std::array<u32, 2>{0U, 0U} &&
                state.background.completion_words ==
                    std::array<u16, 3>{0xFFFFU, 0xFFFFU, 0xFFFFU} &&
                surfaces.allocated_bytes() == 48U &&
                surfaces.live_surface_count() == 0U &&
                surfaces.create_battle_display_surface(0xFFFFFFFFU, 2U) == 0U &&
                surfaces.create_battle_display_surface(3U, 0xFFFFFFFFU) == 0U,
            "failed display creation publishes zero without a backing object"
        );
        state.display_surfaces = {
            surfaces.create_battle_display_surface(3U, 2U),
            retired[1],
        };
        const auto stopped =
            openswd3::battle::release_legacy_battle_display_surfaces(
                state, surfaces
            );
        test.expect_true(
            stopped.typed_stop && stopped.release_calls == 1U &&
                state.display_surfaces == std::array<u32, 2>{0U, retired[1]} &&
                surfaces.live_surface_count() == 0U,
            "retired display token stops release after its completed prefix"
        );

        // 451A90 overwrites its slots without releasing old objects. A
        // repeated recovery must not introduce an extra Release operation.
        display.height = 2;
        static_cast<void>(
            openswd3::battle::create_legacy_battle_display_surfaces(
                state, surfaces
            )
        );
        const auto overwritten = state.display_surfaces;
        static_cast<void>(
            openswd3::battle::create_legacy_battle_display_surfaces(
                state, surfaces
            )
        );
        test.expect_true(
            surfaces.live_surface_count() == 4U &&
                surfaces.find(overwritten[0]) != nullptr &&
                surfaces.find(overwritten[1]) != nullptr &&
                state.display_surfaces[0] != overwritten[0] &&
                state.display_surfaces[1] != overwritten[1],
            "repeated display creation preserves unreleased old objects"
        );
        static_cast<void>(
            openswd3::battle::release_legacy_battle_display_surfaces(
                state, surfaces
            )
        );
        test.expect_true(
            surfaces.live_surface_count() == 2U &&
                surfaces.find(overwritten[0]) != nullptr &&
                surfaces.find(overwritten[1]) != nullptr,
            "release touches only the two currently published display slots"
        );
    }

    for (const u32 stopped_slot : {0U, 1U}) {
        LegacyBattleStartupState state;
        StartupPorts ports;
        state.display_surfaces = {11U, 22U};
        state.background.completion_words = {1U, 2U, 3U};
        ports.unresolved_display_token = state.display_surfaces[stopped_slot];
        const auto result = openswd3::battle::initialize_legacy_battle_startup(
            state, ports, ports, ports, ports, ports, ports, request(1U)
        );
        const std::array<u32, 2> expected{
            stopped_slot == 0U ? 11U : 0U,
            22U,
        };
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleStartupStatus::
                        display_surface_typed_stop &&
                result.released_display_surfaces == stopped_slot &&
                result.created_display_surfaces == 0U &&
                state.display_surfaces == expected &&
                state.background.completion_words ==
                    std::array<u16, 3>{1U, 2U, 3U} &&
                std::none_of(
                    ports.requests.begin(),
                    ports.requests.end(),
                    [](const auto& call) {
                        return call.call ==
                            LegacyBattleStartupCall::system_metric_height;
                    }
                ),
            "unresolved display release stops before clearing or creating"
        );
    }

    // 451AE6..451AF2 releases before clearing. 451A9D..451ACE creates
    // both slots, including zero results, before publishing completion words.
    for (u32 old_mask = 0U; old_mask < 4U; ++old_mask) {
        for (u32 created_mask = 0U; created_mask < 4U; ++created_mask) {
            LegacyBattleStartupState state;
            StartupPorts ports;
            state.display_surfaces = {
                (old_mask & 1U) != 0U ? 11U : 0U,
                (old_mask & 2U) != 0U ? 22U : 0U,
            };
            state.background.completion_words = {1U, 2U, 3U};
            const std::array<u32, 2> created{
                (created_mask & 1U) != 0U ? 111U : 0U,
                (created_mask & 2U) != 0U ? 222U : 0U,
            };
            ports.observed_display_state = &state;
            ports.display_create_replies = {created[0], created[1]};
            std::vector<std::array<u32, 5>> expected_snapshots;
            std::vector<LegacyBattleStartupCall> expected_calls;
            auto live = state.display_surfaces;
            u32 releases = 0U;
            for (auto& token : live) {
                if (token != 0U) {
                    expected_snapshots.push_back(
                        {live[0], live[1], 1U, 2U, 3U}
                    );
                    expected_calls.push_back(
                        LegacyBattleStartupCall::release_display_surface
                    );
                    token = 0U;
                    ++releases;
                }
            }

            for (std::size_t index = 0U; index < created.size(); ++index) {
                expected_snapshots.push_back({live[0], live[1], 1U, 2U, 3U});
                expected_calls.push_back(
                    LegacyBattleStartupCall::system_metric_height
                );
                expected_calls.push_back(
                    LegacyBattleStartupCall::system_metric_width
                );
                expected_calls.push_back(
                    LegacyBattleStartupCall::create_display_surface
                );
                live[index] = created[index];
            }

            const auto result =
                openswd3::battle::initialize_legacy_battle_startup(
                    state, ports, ports, ports, ports, ports, ports, request(1U)
                );
            std::vector<LegacyBattleStartupCall> actual_calls;
            for (const auto& call : ports.requests) {
                if (call.call ==
                        LegacyBattleStartupCall::release_display_surface ||
                    call.call ==
                        LegacyBattleStartupCall::system_metric_height ||
                    call.call == LegacyBattleStartupCall::system_metric_width ||
                    call.call ==
                        LegacyBattleStartupCall::create_display_surface) {
                    actual_calls.push_back(call.call);
                }
            }

            test.expect_true(
                ports.display_snapshots == expected_snapshots &&
                    actual_calls == expected_calls &&
                    result.released_display_surfaces == releases &&
                    result.created_display_surfaces == 2U &&
                    state.display_surfaces == created &&
                    state.background.completion_words ==
                        std::array<u16, 3>{0xFFFFU, 0xFFFFU, 0xFFFFU},
                "display lifecycle preserves release and failed-create prefixes"
            );
        }
    }

    {
        using Record = openswd3::asset_runtime::LegacyActionRecord;
        using Bytes = std::array<openswd3::compat::u8, sizeof(Record)>;
        LegacyBattleStartupState state;
        StartupPorts ports;
        for (const auto seed : {0U, 0xA5U, 0xFFU}) {
            Bytes original;
            original.fill(static_cast<openswd3::compat::u8>(seed));
            ports.dialog_actions.fill(std::bit_cast<Record>(original));
            auto reset = original;
            // Independent stores in 40DC07..40DC22.
            for (const auto offset : {0x1CU, 0x20U, 0x3CU}) {
                std::fill_n(reset.begin() + offset, 4U, 0xFFU);
            }

            std::fill_n(reset.begin() + 0x42U, 8U, 0U);
            std::fill_n(reset.begin() + 0x90U, 4U, 0U);
            auto published = reset;
            // 451CE5 and 451CEF publish the id and base variant.
            std::fill_n(published.begin(), 4U, 0U);
            published[0] = 0x29U;
            published[1] = 0x23U;
            std::fill_n(published.begin() + 8U, 4U, 0U);
            published[8] = 0x0CU;
            const auto result = openswd3::battle::initialize_legacy_battle_startup(
                state, ports, ports, ports, ports, ports, ports, request(1U)
            );
            test.expect_true(
                result.status == openswd3::battle::LegacyBattleStartupStatus::
                    no_enemies &&
                    std::bit_cast<Bytes>(ports.control_snapshots.back()) == reset &&
                    std::bit_cast<Bytes>(ports.dialog_actions[1]) == published &&
                    std::bit_cast<Bytes>(ports.dialog_actions[0]) == original &&
                    std::bit_cast<Bytes>(ports.dialog_actions[2]) == original,
                "startup resets only the shared dialog-end action fields before "
                "reading the transparent pixel pair, then publishes id "
                "and variant"
            );
        }
    }

    {
        constexpr u32 unused = 0xFFFFFFFFU;
        // Independent paths through 451D36..451D88, ending after AX > 3.
        constexpr std::array<std::array<u32, 4>, 16> scan_paths{{
            {4U, unused, unused, unused},
            {0U, 4U, unused, unused},
            {1U, 4U, unused, unused},
            {0U, 1U, 4U, unused},
            {2U, 4U, unused, unused},
            {0U, 2U, 4U, unused},
            {1U, 2U, 4U, unused},
            {0U, 1U, 2U, 4U},
            {3U, unused, unused, unused},
            {0U, 3U, unused, unused},
            {1U, 3U, unused, unused},
            {0U, 1U, 3U, unused},
            {2U, 3U, unused, unused},
            {0U, 2U, 3U, unused},
            {1U, 2U, 3U, unused},
            {0U, 1U, 2U, 3U},
        }};

        for (u32 mask = 0U; mask < scan_paths.size(); ++mask) {
            for (const u32 old_count :
                 {0U, 1U, 7U, 0xFFFFFFFEU, 0xFFFFFFFFU}) {
                LegacyBattleStartupState state;
                StartupPorts ports;
                state.actor_metrics.group_a_count = old_count;
                state.action_mode_source.actor_label_indices.fill(0xABCD1234U);
                auto expected = state.action_mode_source.actor_label_indices;
                for (u32 source = 0U; source < 4U; ++source) {
                    ports.query_values[30U + source] = (mask >> source) & 1U;
                }

                const u32 count =
                    old_count + static_cast<u32>(std::popcount(mask));
                for (std::size_t index = 0U;
                     index < scan_paths[mask].size(); ++index) {
                    if (index < count && scan_paths[mask][index] != unused) {
                        expected[index] = scan_paths[mask][index];
                    }
                }

                const auto result =
                    openswd3::battle::initialize_legacy_battle_startup(
                        state, ports, ports, ports, ports, ports, ports,
                        request(1U)
                    );
                test.expect_true(
                    result.status == openswd3::battle::LegacyBattleStartupStatus::
                        no_enemies && state.actor_metrics.group_a_count == count &&
                        state.action_mode_source.actor_label_indices == expected,
                    "startup preserves the exhausted source index store, "
                    "wrapped party count, and untouched mapping suffix"
                );
            }
        }
    }

    {
        LegacyBattleStartupState state;
        StartupPorts ports;
        auto& selection = ports.battle_target_selection_runtime_state();
        for (const u32 count : {0U, 0x12345678U, 0xFFFFFFFFU}) {
            state.actor_metrics.group_b_count = count;
            selection.special_action_count = 0xABCD1234U;
            selection.transition_stage = 0x76543210U;
            state.reset.block_5242b0.fill(0xA5A5A5A5U);
            const auto result = openswd3::battle::initialize_legacy_battle_startup(
                state, ports, ports, ports, ports, ports, ports, request(1U)
            );
            test.expect_true(
                ports.reset_observations.back() == std::array<u32, 2>{count, 0U} &&
                    selection.special_action_count == 0U &&
                    selection.transition_stage == 0x76543210U &&
                    all_equal(state.reset.block_5242b0, 0U) &&
                    result.status == openswd3::battle::LegacyBattleStartupStatus::
                        no_enemies && state.actor_metrics.group_b_count == 0U,
                "startup resets special count before callbacks; enemy count "
                "changes only when the definition is published on every entry"
            );
        }
    }

#ifdef OPENSWD3_GAME_DATA_ROOT
    {
        LegacyBattleStartupState state;
        StartupPorts ports;
        openswd3::battle::LegacyBattleDefinitionArchiveFileRuntime archive;
        openswd3::battle::LegacyBattleArchiveBackgroundImageLoadPort images;
        openswd3::asset_runtime::LegacyActRuntime act_runtime{
            OPENSWD3_GAME_DATA_ROOT
        };
        openswd3::asset_runtime::LegacyActActionStreamProvider act_provider{
            act_runtime
        };
        openswd3::asset_runtime::LegacyActionUpdater updater{act_provider};
        openswd3::battle::LegacyBattleActionUpdaterRotationPort action_port{
            updater, 1U
        };
        openswd3::asset_runtime::LegacyTswRuntime tsw_runtime{
            OPENSWD3_GAME_DATA_ROOT
        };
        openswd3::battle::LegacyBattleActionRotationResources frames{tsw_runtime};
        // Stop at the following actor-resource boundary. This fixture does
        // not stand in for the missing SDL action/actor resource ports.
        ports.publish_enemy_progress_resource = false;
        ports.allocation_succeeds = false;
        ports.random_values = {0U, 1U};
        auto startup_request = request(1U);
        startup_request.data_root = OPENSWD3_GAME_DATA_ROOT;
        const auto result = openswd3::battle::initialize_legacy_battle_startup(
            state, ports, archive, images, frames, action_port, frames,
            startup_request
        );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleStartupStatus::
                        enemy_action_configuration_typed_stop &&
                result.definition_archive_header.bytes_read == 0x2714U &&
                result.definition_archive_record.record_bytes_read == 0x10CU &&
                result.definition.enemy_count == 1U &&
                result.definition.enemies[0U].role_id == 109U &&
                ports.archive_open_calls == 0U &&
                ports.background_load_calls == 0U,
            "startup reads the real battle definition and background before " "the controlled actor-resource stop"
        );
        test.expect_true(
            result.background.status ==
                    openswd3::battle::
                        LegacyBattleBackgroundInitializationStatus::completed &&
                result.background.rotation_shift == 640 &&
                result.background.image_rotation.width == 640U &&
                result.background.image_rotation.height == 400U &&
                !state.background.image.empty() &&
                state.background.completion_words ==
                    std::array<u16, 3>{0xFFFFU, 0xFFFFU, 0xFFFFU} &&
                state.background_rotation_cache.stored_action_id == 15003U &&
                result.background.action_rotation.status ==
                    openswd3::battle::LegacyBattleActionRotationCacheStatus::
                        completed &&
                result.background.action_rotation.frame_query_calls > 0U &&
                frames.live_owner_count() ==
                    result.background.action_rotation.frame_query_calls &&
                frames.live_image_count() == frames.live_owner_count(),
            "real background bytes and definition parameters reach startup " "without calling the synthetic archive/image ports"
        );
        if (result.status != openswd3::battle::LegacyBattleStartupStatus::
                enemy_action_configuration_typed_stop) {
            std::cerr << "real startup status="
                      << static_cast<unsigned>(result.status)
                      << " rotation="
                      << static_cast<unsigned>(
                             result.background.action_rotation.status
                         )
                      << " resource="
                      << result.background.action_rotation.last_resource_id
                      << " frame="
                      << result.background.action_rotation.last_frame_index
                      << '\n';
        }

        for (u32 random_value = 1U; random_value < 4U; ++random_value) {
            const auto previous_owners = frames.live_owner_count();
            const auto background_request =
                openswd3::battle::make_legacy_battle_startup_background_request(
                    result.definition, startup_request.data_root, random_value
                );
            const auto repeated =
                openswd3::battle::initialize_legacy_battle_background(
                    state.background,
                    state.background_rotation_cache,
                    frames,
                    action_port,
                    frames,
                    startup_request.pixel_conversion,
                    background_request
                );
            test.expect_true(
                repeated.status ==
                        openswd3::battle::
                            LegacyBattleBackgroundInitializationStatus::
                                completed &&
                    repeated.previous_image_released &&
                    repeated.cache_release.image_release_calls ==
                        previous_owners &&
                    repeated.cache_release.owner_release_calls ==
                        previous_owners &&
                    repeated.rotation_shift ==
                        640 / static_cast<i32>(random_value + 1U) &&
                    background_request.one_based_resource == 4U &&
                    frames.live_owner_count() ==
                        repeated.action_rotation.frame_query_calls &&
                    frames.live_image_count() == frames.live_owner_count(),
                "repeated real background entry releases the prior cache and " "uses the same resource for divisors two through four"
            );
        }

        static_cast<void>(openswd3::battle::
            release_legacy_battle_action_rotation_cache(
                state.background_rotation_cache, frames
            ));
        test.expect_true(
            frames.live_owner_count() == 0U && frames.live_image_count() == 0U,
            "real startup releases every rotation image and record"
        );
    }
#endif

    {
        LegacyBattleStartupState state;
        StartupPorts ports;
        poison_reset_blocks(state, ports);
        state.display_surfaces = {0x11110000U, 0x22220000U};
        ports.mouse_frame_state() = {-123, 456, 0xA5U};
        ports.battle_frame_input_resolution_state().previous_mouse_x = -77;
        ports.battle_frame_input_resolution_state().previous_mouse_y = 88;
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
                reset_blocks_match(state, ports) &&
                &ports.actor_metric_state() == &state.actor_metrics &&
                state.actor_metrics.group_a_count == 2U &&
                state.party_presence ==
                    std::array<openswd3::compat::u8, 4>{1U, 0U, 1U, 0U} &&
                state.action_mode_source.actor_label_indices[0] == 0U &&
                state.action_mode_source.actor_label_indices[1] == 2U &&
                state.mode_flags == 0xA5000002U &&
                state.action_delay == 0x12U &&
                state.control_switches == std::array<u32, 4>{1U, 1U, 1U, 1U} &&
                ports.battle_control_action().action_id == 0x2329U &&
                ports.battle_control_action().base_variant == 0x0CU &&
                state.transparent_pixel_pair == 0x12345678U &&
                state.window_rectangle == std::array<i32, 4>{1, 2, 641, 482} &&
                state.primary_text_color == 0x1234U &&
                state.secondary_text_color == 0x5678U &&
                ports.mouse_rebase_snapshots ==
                    std::vector<std::array<i32, 4>>{{320, 320, 200, 200}} &&
                ports.mouse_frame_state().button_mask == 0xA5U &&
                ports.mouse_device.absolute_x_baseline == -560 &&
                ports.mouse_device.absolute_y_baseline == 700 &&
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
        state.party[0U].progress.progress = 0xAAAA0000U;
        state.party[1U].progress.progress = 0xBBBB0000U;
        state.party[2U].progress.progress = 0xCCCC0000U;
        state.party[3U].progress.progress = 0xDDDD0000U;
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
        ports.primary_party_sources[0U].dwords[1U] = 12000U;
        ports.primary_party_sources[0U].dwords[4U] = 0x56781234U;
        ports.primary_party_sources[1U].dwords[1U] = 9000U;
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
        ports.random_values = {1U, 2U, 0U, 0U, 1U, 2U, 8U};
        ports.definition.background_resource = 4U;
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
            player_items.role_item_lists[index]->legacy_head_token =
                sentinel.legacy_token;
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
        auto& embedded_quantity_root =
            ports.legacy_battle_fixed_object_state().object_words[2U];
        embedded_quantity_root[1U] = 9U;
        embedded_quantity_root[2U] = 20U;
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
                ports.background_resource == 4U &&
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
                ) == 0U &&
                ports.call_count(
                    LegacyBattleStartupCall::
                        reserved_group_b_load_action_profile
                ) == 0U &&
                ports.call_count(
                    LegacyBattleStartupCall::
                        reserved_group_b_release_resource_text
                ) == 0U &&
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
                static_cast<u16>(ports.primary_party_sources[0U].dwords[1U]) ==
                    9999U &&
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
                // 46E6D2 clears actor+2F12 before attribute aggregation.
                state.party[0U]
                        .item_effect_application.cached_profile_item_id == 0U &&
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
                        reserved_group_a_embedded_profile_item_quantity
                ) == 0U &&
                ports.call_count(
                    LegacyBattleStartupCall::
                        group_a_attribute_missing_primary_diagnostic
                ) == 1U &&
                attribute_diagnostic != ports.requests.end() &&
                attribute_diagnostic->arguments ==
                    std::array<u32, 4>{0U, 0x004A7C94U, 0x004A7C44U, 0x182U} &&
                attribute_diagnostic->eax ==
                    openswd3::world_map::kLegacyItemSentinelId &&
                result.party_attribute_aggregations[0U]
                        .embedded_profile_applications[0U]
                        .fixed_curve_query_count == 1U &&
                result.party_attribute_aggregations[0U]
                        .embedded_profile_applications[0U]
                        .fixed_curve.return_eax == 0x004B0014U &&
                result.party_attribute_aggregations[0U]
                        .embedded_profile_applications[0U]
                        .fixed_curve.return_ecx == 0x00500009U &&
                result.party_attribute_aggregations[0U]
                        .embedded_profile_applications[0U]
                        .fixed_curve.return_edx == 0x005029D0U &&
                result.party_value_pair_calls == 2U &&
                state.party[0U].value_pair.primary_value ==
                    party_items.legacy_head_token &&
                state.party[0U].value_pair.secondary_value ==
                    party_items.legacy_head_token &&
                result.party_value_pairs[0U].writes == 2U &&
                result.party_value_pairs[0U].return_eax ==
                    party_items.legacy_head_token &&
                result.party_value_pairs[0U].return_ecx == 0x005029D0U &&
                result.party_value_pairs[0U].return_edx == 0U &&
                ports.call_count(
                    LegacyBattleStartupCall::reserved_apply_party_value
                ) == 0U &&
                state.party[0U].name_token == 0x0049E148U &&
                state.party[1U].name_token == 0x0049E158U &&
                ports.call_count(LegacyBattleStartupCall::apply_party_name) ==
                    0U &&
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
                result.supplemental_record_selection_calls == 2U &&
                result.supplemental_record_selections[0U].status ==
                    openswd3::battle::LegacyBattleActorRecordSelectionStatus::
                        completed &&
                result.supplemental_record_selections[1U].status ==
                    openswd3::battle::LegacyBattleActorRecordSelectionStatus::
                        completed &&
                result.supplemental_record_selections[0U].return_eax ==
                    state.party[0U].configuration.actor_record_token &&
                result.supplemental_record_selections[1U].return_eax ==
                    state.party[0U].configuration.actor_record_token &&
                result.supplemental_record_selections[0U].return_ecx ==
                    0x005029D0U &&
                result.supplemental_record_selections[0U].return_edx == 0U &&
                result.supplemental_record_selections[0U].return_esp ==
                    0x70001008U &&
                result.supplemental_record_selections[0U].return_eip ==
                    0x0045264BU &&
                result.supplemental_record_selections[0U].stack_reads[0U] ==
                    1U &&
                result.supplemental_record_selections[0U]
                    .selected_actor_record &&
                !result.supplemental_record_selections[0U]
                     .selected_source_record &&
                ports.call_count(
                    LegacyBattleStartupCall::reserved_supplemental_seed
                ) == 0U &&
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
                ) == 0U &&
                ports.call_count(
                    LegacyBattleStartupCall::reserved_group_a_profile_release
                ) == 0U &&
                state.actor_metrics.group_a_count == 4U &&
                state.party[2].role_id == 3U && state.party[3].role_id == 4U &&
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
                result.party_progress_initialization_calls == 4U &&
                state.party[0U].progress.progress == 0xAAAA01C2U &&
                state.party[1U].progress.progress == 0xBBBB0177U &&
                state.party[2U].progress.progress == 0xCCCC015EU &&
                state.party[3U].progress.progress == 0xDDDD013CU &&
                result.party_progress_initializations[3U].return_eax == 316U &&
                result.party_progress_initializations[3U].return_ecx == 9U &&
                result.party_progress_initializations[3U].return_edx == 6U &&
                ports.call_count(LegacyBattleStartupCall::random_below) == 7U &&
                ports.call_count(
                    LegacyBattleStartupCall::
                        reserved_initialize_party_actor_progress
                ) == 0U &&
                state.party_actor_mode_count == 2U &&
                result.return_value == 1U && result.message_state_published &&
                ports.battle_message_state() == 0x67U &&
                ports.call_count(LegacyBattleStartupCall::set_enemy_mode) ==
                    1U &&
                ports.requested_definition_ids ==
                    std::vector<u32>{0x000BU, 0x000CU, 0x0003U, 0x0004U} &&
                ports.call_count(LegacyBattleStartupCall::apply_actor_mode) ==
                    4U &&
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
        StartupPorts ports;
        ports.primary_party_sources[0U].dwords[4U] = 0x12345678U;
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
                result.supplemental_record_selection_calls == 2U &&
                result.supplemental_record_selections[0U].return_eax ==
                    0x005029D0U &&
                result.supplemental_record_selections[1U].return_eax ==
                    0x005029D0U &&
                result.supplemental_record_selections[0U].return_eip ==
                    0x00452516U &&
                result.supplemental_record_selections[1U].return_eip ==
                    0x00452516U &&
                result.supplemental_record_selections[0U].stack_reads[0U] ==
                    1U &&
                result.supplemental_record_selections[1U].stack_reads[0U] ==
                    1U &&
                result.supplemental_record_selections[0U]
                    .selected_actor_record &&
                !result.supplemental_record_selections[0U]
                     .selected_source_record &&
                ports.call_count(
                    LegacyBattleStartupCall::reserved_supplemental_seed
                ) == 0U &&
                result.supplemental_materialization_calls == 2U &&
                result.supplemental_materializations[0U].status ==
                    openswd3::battle::
                        LegacyBattleGroupANpcMaterializationStatus::completed &&
                result.supplemental_materializations[1U].status ==
                    openswd3::battle::
                        LegacyBattleGroupANpcMaterializationStatus::completed &&
                state.actor_metrics.group_a_count == 2U &&
                state.party[0].role_id == 4U &&
                state.party[1].role_id == 3U &&
                state.supplemental_used[1] == 1U &&
                state.supplemental_used[0] == 1U &&
                ports.call_count(LegacyBattleStartupCall::random_below) == 7U &&
                result.party_progress_initialization_calls == 2U &&
                state.party[0U].progress.progress == 450U &&
                state.party[1U].progress.progress == 450U &&
                ports.call_count(
                    LegacyBattleStartupCall::
                        reserved_configure_supplemental_actor
                ) == 0U &&
                ports.call_count(
                    LegacyBattleStartupCall::group_a_profile_allocate
                ) == 2U &&
                ports.call_count(
                    LegacyBattleStartupCall::group_a_profile_load
                ) == 0U &&
                ports.call_count(
                    LegacyBattleStartupCall::reserved_group_a_profile_release
                ) == 0U &&
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
        ports.query_values = {{34U, 1U}};
        ports.random_values = {0U, 0U};
        ports.definition.enemy_count = 1U;
        auto selector_stop_request = request(8U);
        selector_stop_request.supplemental_record_selection.entry_edx =
            0xAABBCCDDU;
        selector_stop_request.supplemental_record_selection.entry_esp =
            0x76001000U;
        selector_stop_request.supplemental_record_selection.access
            .final_return_address_readable = false;

        const auto result = openswd3::battle::initialize_legacy_battle_startup(
            state,
            ports,
            ports,
            ports,
            ports,
            ports,
            ports,
            selector_stop_request
        );

        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleStartupStatus::
                        supplemental_record_selection_typed_stop &&
                result.supplemental_actor_count == 0U &&
                result.supplemental_record_selection_calls == 1U &&
                result.supplemental_record_selections[0U].status ==
                    openswd3::battle::LegacyBattleActorRecordSelectionStatus::
                        final_return_address_read_typed_stop &&
                result.supplemental_record_selections[0U].return_eax ==
                    0x005029D0U &&
                result.supplemental_record_selections[0U].return_ecx ==
                    0x005029D0U &&
                result.supplemental_record_selections[0U].return_edx ==
                    0xAABBCCDDU &&
                result.supplemental_record_selections[0U].return_esp ==
                    0x76001000U &&
                result.supplemental_record_selections[0U].return_eip ==
                    0x0047868AU &&
                result.supplemental_record_selections[0U].argument_reads ==
                    1U &&
                result.supplemental_record_selections[0U].actor_field_reads ==
                    1U &&
                result.supplemental_record_selections[0U].tests_executed ==
                    2U &&
                !result.supplemental_record_selections[0U].returned &&
                result.supplemental_materialization_calls == 0U &&
                state.actor_metrics.group_a_count == 0U &&
                state.party[0].role_id == 3U &&
                state.party[0].configuration.actor_record_token ==
                    0x005029D0U &&
                state.party[0].configuration.profile_token == 0U &&
                state.party[0].configuration.placement_word == 0U &&
                ports.call_count(
                    LegacyBattleStartupCall::reserved_supplemental_seed
                ) == 0U &&
                ports.call_count(
                    LegacyBattleStartupCall::group_a_profile_allocate
                ) == 0U &&
                ports.call_count(
                    LegacyBattleStartupCall::
                        reserved_configure_supplemental_actor
                ) == 0U &&
                ports.call_count(LegacyBattleStartupCall::apply_actor_mode) ==
                    0U &&
                !result.message_state_published,
            "supplemental selector return typed stop suppresses current materialization and remaining startup suffix"
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
        state.party[0U].progress.progress = 0xFACE0011U;
        state.party[0U].progress.progress_write_accessible = false;
        StartupPorts ports;
        ports.query_values[30U] = 1U;
        ports.random_values = {0U, 0U, 8U};
        ports.definition.enemy_count = 1U;
        const auto result = openswd3::battle::initialize_legacy_battle_startup(
            state, ports, ports, ports, ports, ports, ports, request(14U)
        );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleStartupStatus::
                        party_progress_initialization_typed_stop &&
                result.party_progress_initialization_calls == 1U &&
                result.party_progress_initializations[0U].status ==
                    openswd3::battle::
                        LegacyBattleActorProgressInitializationStatus::
                            actor_progress_write_typed_stop &&
                result.party_progress_initializations[0U].random_value == 8U &&
                result.party_progress_initializations[0U].return_eax == 316U &&
                result.party_progress_initializations[0U].return_ecx == 9U &&
                result.party_progress_initializations[0U].return_edx == 6U &&
                result.party_progress_initializations[0U].progress_writes ==
                    0U &&
                result.party_progress_typed_stop.return_eax == 316U &&
                result.party_progress_typed_stop.return_ecx == 9U &&
                result.party_progress_typed_stop.return_edx == 6U &&
                state.party[0U].progress.progress == 0xFACE0011U &&
                ports.call_count(LegacyBattleStartupCall::random_below) == 3U &&
                ports.call_count(
                    LegacyBattleStartupCall::
                        reserved_initialize_party_actor_progress
                ) == 0U &&
                !result.message_state_published && result.return_value == 0U,
            "party progress write stop preserves the RNG and division prefix and suppresses the startup suffix"
        );
    }

    {
        LegacyBattleStartupState state;
        StartupPorts ports;
        ports.query_values[30U] = 1U;
        ports.random_values = {0U, 0U};
        ports.definition.enemy_count = 1U;
        auto startup_request = request(15U);
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
                result.initial_party_actor_count == 0U &&
                result.party_item_order.lists_visited == 0U,
            "even without party actors, player-item order stops block party-item sorting and later startup phases"
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
                        reserved_group_a_embedded_profile_item_quantity
                ) == 0U,
            "missing role-item sentinel stops the direct attribute aggregation before value and resource publication"
        );
    }

    {
        LegacyBattleStartupState state;
        StartupPorts ports;
        ports.publish_enemy_progress_resource = false;
        ports.allocation_succeeds = false;
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
