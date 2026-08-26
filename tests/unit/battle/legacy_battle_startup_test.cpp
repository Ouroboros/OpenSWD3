#include "openswd3/battle/legacy_battle_startup.hpp"

#include <algorithm>
#include <array>
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
      public openswd3::battle::LegacyBattleDefinitionLoadPort,
      public openswd3::battle::LegacyBattleBackgroundImageLoadPort,
      public openswd3::battle::LegacyBattleActionRotationReleasePort,
      public openswd3::battle::LegacyBattleActionRotationUpdatePort,
      public openswd3::battle::LegacyBattleMutableFrameImagePort {
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
        case LegacyBattleStartupCall::apply_actor_mode:
            reply.ecx_snapshot = 0xBEEF0000U;
            break;
        case LegacyBattleStartupCall::query_party_actor_mode:
            reply.return_value = party_actor_mode_return;
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
            reply.return_value = 0x2468ACE0U;
            break;
        case LegacyBattleStartupCall::query_actor_metric:
            reply.publish_metric_word = true;
            reply.metric_word = 1U;
            break;
        default:
            break;
        }
        return reply;
    }

    [[nodiscard]] u32 open_archive(
        const std::filesystem::path& archive_path,
        const u32 archive_object_token,
        const u32 scratch_token
    ) override {
        opened_path = archive_path;
        opened_archive_object_token = archive_object_token;
        opened_scratch_token = scratch_token;
        ++archive_open_calls;
        return 0xA5A5A5A5U;
    }

    [[nodiscard]] LegacyBattleDefinition load_definition(
        const std::filesystem::path& archive_path,
        const u32 archive_object_token,
        const u32 definition_token,
        const u32 battle_id,
        const u32 variant_index
    ) override {
        loaded_path = archive_path;
        loaded_archive_object_token = archive_object_token;
        loaded_definition_token = definition_token;
        loaded_battle_id = battle_id;
        loaded_variant = variant_index;
        ++definition_load_calls;
        return definition;
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
    std::filesystem::path opened_path;
    std::filesystem::path loaded_path;
    std::filesystem::path background_path;
    u32 opened_archive_object_token{};
    u32 opened_scratch_token{};
    u32 archive_open_calls{};
    u32 loaded_archive_object_token{};
    u32 loaded_definition_token{};
    u32 loaded_battle_id{};
    u32 loaded_variant{};
    u32 definition_load_calls{};
    u32 background_resource{};
    u32 background_variant{};
    u32 background_load_calls{};
    u32 created_surface_count{};
    u32 no_enemy_return{0x87654321U};
    u32 party_actor_mode_return{};
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
    reset.value_53ae70 = 1U;
    reset.value_53bf22 = 1U;
    reset.value_53c4b0 = 1U;
    for (auto& record : reset.records_524788) {
        record.value_00 = 1U;
        record.value_0a = 1U;
        record.value_0c = 1U;
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
                       record.value_0a == 0U && record.value_0c == 0U &&
                       record.value_14 == 0U && record.value_18 == 0U;
               }
        ) &&
        reset.value_4ff0b0 == 0U && reset.value_4fe5cc == 0U &&
        reset.value_4ff0b4 == 0U && reset.value_4fe5d0 == 0U &&
        reset.value_4ff0b8 == 0U && reset.value_524414 == 0U &&
        all_equal(reset.values_52544c, 0U) &&
        all_equal(reset.values_502940, 0U) &&
        all_equal(reset.values_5244d8, 0U) && reset.value_524418 == 0U &&
        reset.value_53c048 == 0U && reset.value_53ae70 == 0xFFFFFFFFU &&
        reset.value_53bf22 == 0U && reset.value_53c4b0 == 0U;
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

        const auto result = openswd3::battle::initialize_legacy_battle_startup(
            state,
            ports,
            ports,
            ports,
            ports,
            ports,
            ports,
            request(0xABCD1234U)
        );

        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleStartupStatus::no_enemies &&
                result.return_value == 0x87654321U &&
                result.no_enemy_notification_calls == 1U &&
                result.action_threshold == 900 &&
                state.battle_id_word == 0x1234U &&
                reset_blocks_match(state, ports) && state.party_count == 2U &&
                state.party_presence ==
                    std::array<openswd3::compat::u8, 4>{1U, 0U, 1U, 0U} &&
                state.party_source_indices[0] == 0U &&
                state.party_source_indices[1] == 2U &&
                state.mode_flags == 0xA5000002U &&
                state.action_delay == 0x12U &&
                state.control_switches == std::array<u32, 4>{1U, 1U, 1U, 1U} &&
                state.control_value_a == 0x2329U &&
                state.control_value_b == 0x0CU &&
                state.runtime_handle == 0x12345678U &&
                state.window_rectangle == std::array<i32, 4>{1, 2, 641, 482} &&
                state.frame_value_a == 0x1234U &&
                state.frame_value_b == 0x5678U && state.logical_width == 320U &&
                state.logical_height == 200U &&
                result.released_display_surfaces == 2U &&
                result.created_display_surfaces == 2U &&
                state.display_surfaces ==
                    std::array<u32, 2>{0x70000000U, 0x70000001U} &&
                state.background.completion_words ==
                    std::array<u16, 3>{0xFFFFU, 0xFFFFU, 0xFFFFU} &&
                result.display_surface_return_snapshot == 0xFFFFFFFFU &&
                result.display_completion_write_order ==
                    std::array<openswd3::compat::u8, 3>{0U, 1U, 2U} &&
                ports.opened_path ==
                    std::filesystem::path("game-data/battle.ffd") &&
                ports.opened_archive_object_token == 0x004FF5B8U &&
                ports.opened_scratch_token == 0x005241FCU &&
                ports.archive_open_calls == 1U &&
                ports.loaded_path ==
                    std::filesystem::path("game-data/battle.ffd") &&
                ports.loaded_archive_object_token == 0x004FF5B8U &&
                ports.loaded_definition_token == 0x004FF1E0U &&
                ports.loaded_battle_id == 0xABCD1234U &&
                ports.loaded_variant == 0U &&
                ports.definition_load_calls == 1U &&
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
        StartupPorts ports;
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

        const auto result = openswd3::battle::initialize_legacy_battle_startup(
            state, ports, ports, ports, ports, ports, ports, request(7U)
        );

        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleStartupStatus::completed &&
                result.background.status ==
                    openswd3::battle::
                        LegacyBattleBackgroundInitializationStatus::
                            image_load_failed &&
                ports.background_path ==
                    std::filesystem::path("game-data/all_map2.tsw") &&
                ports.background_resource == 2U &&
                ports.background_variant == 0U &&
                result.enemy_actor_count == 2U &&
                state.enemies[0].role_id == 11U &&
                state.enemies[0].position_x == 540U &&
                state.enemies[1].position_x == 340U &&
                result.initial_party_actor_count == 2U &&
                state.party[0].role_id == 101U &&
                state.party[0].position_x == 150U &&
                state.party[0].position_y == 275U &&
                state.party[1].role_id == 102U &&
                state.party[1].position_x == 85U &&
                state.party[1].position_y == 370U &&
                state.party_offsets[0] == 124 && state.party_offsets[2] == 64 &&
                result.supplemental_actor_count == 2U &&
                state.party_count == 4U && state.party[2].role_id == 3U &&
                state.party[3].role_id == 4U &&
                state.party[2].position_x == 0xFF92U &&
                state.party[3].position_x == 0xFF92U &&
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
                result.finalized_party_actor_count == 4U &&
                state.party_actor_mode_count == 2U &&
                result.return_value == 1U &&
                result.completion_status_published &&
                state.completion_status == 0x67U &&
                ports.call_count(LegacyBattleStartupCall::set_enemy_mode) ==
                    1U &&
                std::ranges::any_of(
                    ports.requests,
                    [](const LegacyBattleStartupCallRequest& call) {
                        return call.call ==
                            LegacyBattleStartupCall::configure_enemy_actor &&
                            call.arguments[2] == 0xBEEF000BU;
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
        StartupPorts ports;
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
                state.party_count == 2U && state.party[0].role_id == 4U &&
                state.party[1].role_id == 3U &&
                state.supplemental_used[1] == 1U &&
                state.supplemental_used[0] == 1U &&
                ports.call_count(LegacyBattleStartupCall::random_below) == 5U &&
                ports.call_count(LegacyBattleStartupCall::apply_actor_mode) ==
                    2U &&
                result.return_value == 0U &&
                result.completion_status_published &&
                state.completion_status == 0x67U,
            "stale supplemental word selects random branch and duplicate random candidate retries without cap"
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
}
