#include "openswd3/battle/legacy_battle_victory_rewards.hpp"

#include <array>
#include <bit>
#include <deque>
#include <map>
#include <memory>
#include <vector>

#include "test.hpp"

namespace {

using openswd3::battle::LegacyBattleVictoryRewardCall;
using openswd3::battle::LegacyBattleVictoryRewardCallReply;
using openswd3::battle::LegacyBattleVictoryRewardCallRequest;
using openswd3::compat::i32;
using openswd3::compat::u8;
using openswd3::compat::u16;
using openswd3::compat::u32;

class ActionStreamProvider final
    : public openswd3::asset_runtime::LegacyActionStreamProvider {
public:
    [[nodiscard]] openswd3::asset_runtime::LegacyActionStreamLoadResult
    load_action_stream(
        const u32 action_id, const u32 variant_index, const bool
    ) override {
        action_ids.push_back(action_id);
        variants.push_back(variant_index);
        constexpr std::array<u16, 8> kWords{
            0x5246U, 0x0077U, 0x5041U, 0U, 0x5859U, 2U, 3U, 0x4544U
        };
        bytes.clear();
        for (const u16 word : kWords) {
            bytes.push_back(static_cast<u8>(word));
            bytes.push_back(static_cast<u8>(word >> 8U));
        }
        return {
            openswd3::asset_runtime::LegacyActionStreamStatus::ready,
            bytes,
            false,
        };
    }

    std::vector<u8> bytes;
    std::vector<u32> action_ids;
    std::vector<u32> variants;
};

class FrameProvider final
    : public openswd3::rendering::LegacyFramePieceProvider {
public:
    [[nodiscard]] bool load_frame_piece(
        const u32 resource_id,
        const u32 piece_index,
        openswd3::rendering::LegacyFramePiece& piece
    ) noexcept override {
        resource_ids.push_back(resource_id);
        piece_indices.push_back(piece_index);
        if (fail || piece_index >= pixels.size()) {
            piece = {};
            return false;
        }
        piece = {
            .source =
                {
                    .bytes = pixels[piece_index],
                    .layout =
                        openswd3::rendering::LegacyBlitSourceLayout::direct_16,
                },
            .width = 1U,
            .height = 1U,
        };
        return true;
    }

    std::array<std::array<u8, 2>, 9> pixels{{
        {1U, 0U},
        {2U, 0U},
        {3U, 0U},
        {4U, 0U},
        {5U, 0U},
        {6U, 0U},
        {7U, 0U},
        {8U, 0U},
        {9U, 0U},
    }};
    std::vector<u32> resource_ids;
    std::vector<u32> piece_indices;
    bool fail{};
};

class RandomPort final
    : public openswd3::battle::LegacyBattleBoundedRandomPort {
public:
    [[nodiscard]] u32 random_bounded(const u32 bound) override {
        bounds.push_back(bound);
        if (publish_group_b_count) {
            metric_state->group_b_count = published_group_b_count;
            publish_group_b_count = false;
        }

        if (values.empty()) {
            return 0U;
        }
        const u32 value = values.front();
        values.pop_front();
        return value;
    }

    void push(const u32 value) {
        values.push_back(value);
    }

    void publish_count_on_next_call(
        openswd3::battle::LegacyBattleActorMetricState& state, const u32 count
    ) noexcept {
        metric_state = &state;
        published_group_b_count = count;
        publish_group_b_count = true;
    }

    std::deque<u32> values;
    std::vector<u32> bounds;
    openswd3::battle::LegacyBattleActorMetricState* metric_state{};
    u32 published_group_b_count{};
    bool publish_group_b_count{};
};

class VictoryPort final
    : public openswd3::battle::LegacyBattleVictoryRewardPort {
public:
    [[nodiscard]] LegacyBattleVictoryRewardCallReply invoke_victory_reward(
        const LegacyBattleVictoryRewardCallRequest& request
    ) override {
        calls.push_back(request);
        auto& index = reply_indices[request.call];
        const auto found = replies.find(request.call);
        if (found == replies.end() || index >= found->second.size()) {
            return {
                .eax = request.eax,
                .ecx = request.ecx,
                .edx = request.edx,
            };
        }
        return found->second[index++];
    }

    [[nodiscard]] openswd3::battle::LegacyBattleActionCallReply invoke(
        const openswd3::battle::LegacyBattleActionCallRequest& request
    ) override {
        action_calls.push_back(request);
        if (request.callee_token == 0x00487C10U && !allocation_tokens.empty()) {
            const u32 token = allocation_tokens.front();
            allocation_tokens.pop_front();
            return {.eax = token};
        }
        return {.eax = request.eax, .ecx = request.ecx, .edx = request.edx};
    }

    [[nodiscard]] openswd3::battle::LegacyBattleInputDispatchCallReply
    invoke_input_dispatch(
        const openswd3::battle::LegacyBattleInputDispatchCallRequest& request
    ) override {
        return {.eax = request.eax, .ecx = request.ecx, .edx = request.edx};
    }

    [[nodiscard]] openswd3::battle::LegacyBattleInputDispatchCallReply
    play_input_sample(
        const u32 sound_id,
        const i32 mix_level,
        const u32 eax,
        const u32 ecx,
        const u32 edx
    ) override {
        samples.push_back({sound_id, std::bit_cast<u32>(mix_level)});
        sample_entry_eax.push_back(eax);
        return {.eax = eax, .ecx = ecx, .edx = edx};
    }

    [[nodiscard]] openswd3::battle::LegacyBattleVictoryRewardRegisters
    begin_music_fade(const u32 eax, const u32 ecx, const u32 edx) override {
        ++music_fades;
        return {.eax = eax, .ecx = ecx, .edx = edx};
    }

    [[nodiscard]] openswd3::battle::LegacyBattleVictoryRewardRegisters
    stop_all_samples(const u32, const u32 ecx, const u32 edx) override {
        ++sample_stops;
        return {.eax = 1U, .ecx = ecx, .edx = edx};
    }

    void reply(
        const LegacyBattleVictoryRewardCall call,
        const LegacyBattleVictoryRewardCallReply& reply
    ) {
        replies[call].push_back(reply);
    }

    [[nodiscard]] u32 count(const LegacyBattleVictoryRewardCall call) const {
        u32 value = 0U;
        for (const auto& request : calls) {
            if (request.call == call) {
                ++value;
            }
        }
        return value;
    }

    std::vector<LegacyBattleVictoryRewardCallRequest> calls;
    std::map<
        LegacyBattleVictoryRewardCall,
        std::vector<LegacyBattleVictoryRewardCallReply>>
        replies;
    std::map<LegacyBattleVictoryRewardCall, std::size_t> reply_indices;
    std::vector<openswd3::battle::LegacyBattleActionCallRequest> action_calls;
    std::deque<u32> allocation_tokens;
    std::vector<std::array<u32, 2>> samples;
    std::vector<u32> sample_entry_eax;
    u32 music_fades{};
    u32 sample_stops{};
};

struct Fixture {
    Fixture() : action_updater(action_streams), raster(framebuffer.geometry()) {
        state.committed_money_word = 5U;
        state.experience_per_party_member = 10U;
        state.reward_experience = 20U;
        state.party_profile_threshold = 5U;
        input.sample_mix_level = -4;
        target.transition_stage = 72U;
        script_variables[0U] = 100U;
        startup.group_b_lifecycle = std::make_shared<std::array<
            openswd3::battle::LegacyBattleActorGroupBElementState,
            8>>();
        for (std::size_t index = 0U; index < startup.group_b_lifecycle->size();
             ++index) {
            (*startup.group_b_lifecycle)[index].resource_token =
                openswd3::battle::
                    kLegacyBattleActorGroupBResourceStateBaseToken +
                static_cast<u32>(index * 0x100U);
        }
    }

    [[nodiscard]] openswd3::battle::LegacyBattleVictoryRewardBindings
    bindings() {
        return {
            .state = state,
            .startup = startup,
            .metrics = metrics,
            .input_dispatch = input,
            .target_selection = target,
            .party_member_resources = party_resources,
            .script_variables =
                std::span<u32>{script_variables.data(), script_variable_count},
            .framebuffer = framebuffer,
            .raster = raster,
            .shared_effects = effects,
            .jitter = jitter,
            .action_updater = action_updater,
            .frame_provider = frame_provider,
            .bounded_random = random,
        };
    }

    openswd3::battle::LegacyBattleVictoryRewardState state;
    openswd3::battle::LegacyBattleStartupState startup;
    openswd3::battle::LegacyBattleActorMetricState metrics;
    openswd3::battle::LegacyBattleInputDispatchState input;
    openswd3::battle::LegacyBattleTargetSelectionRuntimeState target;
    std::array<openswd3::world_map::LegacyWorldStoryPartyMemberResources, 4>
        party_resources{};
    std::array<u32, 64> script_variables{};
    std::size_t script_variable_count{script_variables.size()};
    openswd3::rendering::LegacyFramebuffer framebuffer{{
        .pitch_bytes = 1280,
        .width = 640,
        .height = 480,
    }};
    ActionStreamProvider action_streams;
    openswd3::asset_runtime::LegacyActionUpdater action_updater;
    openswd3::rendering::LegacyRasterGeometryState raster;
    openswd3::rendering::LegacyBlitEffectState effects;
    openswd3::rendering::LegacyRleRowJitterState jitter;
    FrameProvider frame_provider;
    RandomPort random;
    VictoryPort port;
};

[[nodiscard]] openswd3::battle::LegacyBattleVictoryRewardResult
run(Fixture& fixture,
    const openswd3::battle::LegacyBattleVictoryRewardRequest& request = {}) {
    return openswd3::battle::advance_legacy_battle_victory_rewards(
        fixture.bindings(), fixture.port, request
    );
}

[[nodiscard]] std::vector<u8>
text_bytes(const LegacyBattleVictoryRewardCallRequest& request) {
    return {
        request.text.begin(),
        request.text.begin() + static_cast<std::ptrdiff_t>(request.text_length),
    };
}

void set_profile_word(
    openswd3::battle::LegacyBattleGroupASummonProfileRecord& profile,
    const std::size_t offset,
    const u16 value
) {
    profile[offset] = static_cast<std::byte>(static_cast<u8>(value));
    profile[offset + 1U] = static_cast<std::byte>(static_cast<u8>(value >> 8U));
}

void set_group_b_reward(
    Fixture& fixture,
    const std::size_t actor_index,
    const u16 item_id,
    const u16 threshold
) {
    auto& bytes =
        (*fixture.startup.group_b_lifecycle)[actor_index].resource_bytes;
    bytes[0x82U] = static_cast<u8>(item_id);
    bytes[0x83U] = static_cast<u8>(item_id >> 8U);
    bytes[0x84U] = static_cast<u8>(threshold);
    bytes[0x85U] = static_cast<u8>(threshold >> 8U);
}

}  // namespace

void test_battle_victory_rewards(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleVictoryRewardStatus;

    {
        Fixture fixture;
        fixture.state.committed_money_word = 0x8005U;
        fixture.target.transition_timer = 9U;
        const auto result = run(fixture);
        test.expect_true(
            result.status == LegacyBattleVictoryRewardStatus::completed &&
                result.rectangle_calls == 1U &&
                result.tiled_frame_calls == 2U &&
                result.text_draw_calls == 4U && result.music_fade_calls == 0U &&
                result.sample_calls == 0U &&
                fixture.script_variables[0U] == 100U &&
                fixture.target.transition_timer == 9U &&
                fixture.state.panel_action_record.action_id == 0x233BU &&
                fixture.action_streams.action_ids ==
                    std::vector<u32>{0x233BU} &&
                fixture.port.count(LegacyBattleVictoryRewardCall::draw_text) ==
                    4U &&
                result.transition_stage.return_eax == 1U,
            "committed victory rewards still draw the settled summary without distributing twice"
        );
        const auto& title = fixture.port.calls[0U];
        test.expect_true(
            text_bytes(title) ==
                    std::vector<u8>{
                        0xBEU,
                        0xD4U,
                        0xB0U,
                        0xABU,
                        0xB3U,
                        0xD3U,
                        0xA7U,
                        0x51U,
                    } &&
                title.arguments[1U] == 0xF8U && title.arguments[2U] == 0xB4U,
            "victory title preserves its CP950 bytes and fixed coordinates"
        );
    }

    {
        Fixture fixture;
        fixture.metrics.group_b_count = 2U;
        fixture.metrics.group_a_count = 2U;
        fixture.startup.action_mode_source.actor_label_indices[0U] = 0U;
        fixture.startup.action_mode_source.actor_label_indices[1U] = 1U;
        fixture.party_resources[0U].field_2c = 0U;
        fixture.party_resources[1U].field_2c = 9U;
        auto& alias =
            fixture.port.world_item_list_state().player_inventory_head_alias;
        alias.item_id = 7U;
        set_group_b_reward(fixture, 0U, 7U, 20U);
        set_group_b_reward(fixture, 1U, 7U, 20U);
        fixture.random.push(0U);
        fixture.random.push(19U);
        fixture.port.reply(
            LegacyBattleVictoryRewardCall::query_group_a_reward_block,
            {.eax = 0U}
        );
        fixture.port.reply(
            LegacyBattleVictoryRewardCall::query_group_a_reward_block,
            {.eax = 0U}
        );
        auto& actor_profile = fixture.startup.party[0U]
                                  .attribute_aggregation.embedded_profiles[0U];
        set_profile_word(actor_profile, 0x04U, 10U);
        set_profile_word(actor_profile, 0x10U, 5U);
        fixture.port.group_a_reward_profile_state().head.item_id = 5U;
        fixture.port.reply(
            LegacyBattleVictoryRewardCall::
                reserved_transition_stage_advance_slot,
            {.eax = 1U}
        );
        const auto result = run(fixture, {.local_text_token = 0xABCDEF00U});
        test.expect_true(
            result.status == LegacyBattleVictoryRewardStatus::completed &&
                result.music_fade_calls == 1U &&
                result.stop_all_sample_calls == 1U &&
                result.sample_calls == 1U &&
                fixture.port.samples ==
                    std::vector<std::array<u32, 2>>{{0x12CU, 0xFFFFFFFCU}} &&
                fixture.port.sample_entry_eax ==
                    std::vector<u32>{0xFFFFFFFCU} &&
                result.group_b_query_calls == 2U &&
                fixture.random.bounds == std::vector<u32>{20U, 20U} &&
                fixture.port.count(
                    LegacyBattleVictoryRewardCall::reserved_query_group_b_item
                ) == 0U &&
                result.group_a_query_calls == 2U &&
                result.player_item_quantity_calls == 2U &&
                fixture.target.transition_sample_word == 1U &&
                fixture.state.collected_item_ids[0U] == 7U &&
                fixture.state.collected_item_quantities[0U] == 2U &&
                fixture.state.player_item_tokens[0U] == 0x004A994CU &&
                alias.quantity_b == 2U,
            "victory distribution merges duplicate drops while preserving both inventory quantity calls"
        );
        test.expect_true(
            fixture.party_resources[0U].field_00 == 10U &&
                fixture.party_resources[1U].field_00 == 0U &&
                fixture.state.party_reward_counters[0U] == 1U &&
                fixture.state.party_reward_counters[1U] == 1U &&
                result.group_a_reward_profile_calls == 2U &&
                result.group_a_reward_profiles[0U].matched_profiles == 1U &&
                result.group_a_reward_profiles[0U].return_eax == 1U &&
                result.group_a_reward_profiles[1U].return_eax == 0U &&
                fixture.port.group_a_reward_profile_state().head.quantity ==
                    10U &&
                fixture.port.group_a_reward_profile_state().head.percentage ==
                    100U &&
                fixture.state.actor_reward_gate == 1U &&
                fixture.state.committed_money_word == 0x8005U &&
                fixture.script_variables[0U] == 105U &&
                fixture.target.transition_timer == 0U &&
                fixture.port.count(
                    LegacyBattleVictoryRewardCall::prepare_group_a_actor
                ) == 2U &&
                fixture.port.count(
                    LegacyBattleVictoryRewardCall::configure_group_a_actor
                ) == 2U,
            "victory distribution applies thresholded party experience, actor completion and one-time money"
        );
        test.expect_true(
            fixture.port.count(
                LegacyBattleVictoryRewardCall::reserved_apply_group_a_reward
            ) == 0U &&
                fixture.port.calls[2U].call ==
                    LegacyBattleVictoryRewardCall::prepare_group_a_actor &&
                fixture.port.calls[2U].eax == 0x004ACF54U &&
                fixture.port.calls[2U].edx == 2U,
            "victory party caller directly applies reward profiles before preserving the counter-address register snapshot"
        );
        test.expect_true(
            result.text_draw_calls == 4U && fixture.port.calls.size() >= 10U &&
                text_bytes(
                    fixture.port.calls[fixture.port.calls.size() - 3U]
                ) ==
                    std::vector<u8>{
                        0xA8U,
                        0x43U,
                        0xA4U,
                        0x48U,
                        0xB1U,
                        0x6FU,
                        0xA8U,
                        0xECU,
                        0xB8U,
                        0x67U,
                        0xC5U,
                        0xE7U,
                        0xADU,
                        0xC8U,
                        0x3AU,
                        0x31U,
                        0x30U,
                    } &&
                text_bytes(
                    fixture.port.calls[fixture.port.calls.size() - 2U]
                ) ==
                    std::vector<u8>{
                        0xB1U,
                        0x6FU,
                        0xA8U,
                        0xECU,
                        0xBBU,
                        0xC8U,
                        0xB9U,
                        0xF4U,
                        0x3AU,
                        0x35U,
                    } &&
                text_bytes(fixture.port.calls.back()) ==
                    std::vector<u8>{
                        0xB1U,
                        0x6FU,
                        0xA8U,
                        0xECU,
                        0xAAU,
                        0x6BU,
                        0xC4U,
                        0x5FU,
                        0xB8U,
                        0x67U,
                        0xC5U,
                        0xE7U,
                        0xADU,
                        0xC8U,
                        0x3AU,
                        0x32U,
                        0x30U,
                    },
            "victory summary formats the three CP950 reward lines into the shared 64-byte stack buffer"
        );
    }

    {
        Fixture fixture;
        fixture.metrics.group_a_count = 1U;
        fixture.startup.action_mode_source.actor_label_indices[0U] = 0U;
        fixture.port.reply(
            LegacyBattleVictoryRewardCall::query_group_a_reward_block,
            {.eax = 0U}
        );
        auto& actor_profile = fixture.startup.party[0U]
                                  .attribute_aggregation.embedded_profiles[0U];
        set_profile_word(actor_profile, 0x04U, 10U);
        set_profile_word(actor_profile, 0x10U, 7U);
        fixture.port.group_a_reward_profile_state().head.legacy_next_token =
            0x00DEAD00U;
        const auto result = run(fixture);
        test.expect_true(
            result.status ==
                    LegacyBattleVictoryRewardStatus::
                        group_a_reward_profile_typed_stop &&
                result.group_a_reward_profile_calls == 1U &&
                result.group_a_reward_profiles[0U].status ==
                    openswd3::battle::
                        LegacyBattleGroupARewardProfileApplicationStatus::
                            profile_node_typed_stop &&
                fixture.state.party_reward_counters[0U] == 0U &&
                fixture.state.committed_money_word == 5U &&
                fixture.port.count(
                    LegacyBattleVictoryRewardCall::reserved_apply_group_a_reward
                ) == 0U,
            "victory caller propagates a direct reward-profile token stop before counters and money commit"
        );
    }

    {
        Fixture fixture;
        fixture.metrics.group_b_count = 3U;
        fixture.metrics.group_a_count = 1U;
        fixture.startup.action_mode_source.actor_label_indices[0U] = 0U;
        fixture.port.reply(
            LegacyBattleVictoryRewardCall::query_group_a_reward_block,
            {.eax = 0U}
        );
        const auto result = run(fixture);
        test.expect_true(
            result.status == LegacyBattleVictoryRewardStatus::completed &&
                fixture.state.party_reward_counters[0U] == 2U,
            "three or more group-B actors add the original second party reward counter increment"
        );
    }

    {
        Fixture fixture;
        fixture.metrics.group_b_count = 1U;
        fixture.target.transition_sample_word = 10U;
        auto& alias =
            fixture.port.world_item_list_state().player_inventory_head_alias;
        alias.item_id = 7U;
        set_group_b_reward(fixture, 0U, 7U, 20U);
        fixture.random.push(0U);
        const auto result = run(fixture);
        test.expect_true(
            result.status ==
                    LegacyBattleVictoryRewardStatus::
                        collected_item_quantity_typed_stop &&
                result.player_item_quantity_calls == 1U &&
                alias.quantity_b == 1U &&
                fixture.target.transition_sample_word == 10U &&
                fixture.state.committed_money_word == 5U,
            "an eleventh unique drop stops at the first quantity-table access after preserving inventory side effects"
        );
    }

    {
        Fixture fixture;
        fixture.metrics.group_b_count = 1U;
        set_group_b_reward(fixture, 0U, 1U, 0U);
        fixture.random.push(0U);
        fixture.random.publish_count_on_next_call(fixture.metrics, 9U);
        const auto result = run(fixture);
        test.expect_true(
            result.status ==
                    LegacyBattleVictoryRewardStatus::group_b_actor_typed_stop &&
                result.group_b_query_calls == 8U &&
                fixture.random.bounds == std::vector<u32>{20U} &&
                fixture.port.count(
                    LegacyBattleVictoryRewardCall::reserved_query_group_b_item
                ) == 0U,
            "group-B reward traversal reloads its live signed bound until the ninth actor reaches the first query"
        );
    }

    {
        Fixture fixture;
        fixture.metrics.group_b_count = 1U;
        fixture.startup.group_b_lifecycle.reset();
        const auto result = run(fixture);
        test.expect_true(
            result.status ==
                    LegacyBattleVictoryRewardStatus::
                        group_b_reward_item_typed_stop &&
                result.group_b_query_calls == 1U &&
                result.group_b_reward_items[0U].status ==
                    openswd3::battle::
                        LegacyBattleGroupBRewardItemSelectionStatus::
                            actor_state_typed_stop &&
                fixture.random.bounds.empty() &&
                fixture.port.count(
                    LegacyBattleVictoryRewardCall::reserved_query_group_b_item
                ) == 0U,
            "victory reward propagation stops at the reclaimed actor resource-pointer access without invoking the old query"
        );
    }

    {
        Fixture fixture;
        fixture.metrics.group_a_count = 11U;
        for (u32 index = 0U; index < 10U; ++index) {
            fixture.port.reply(
                LegacyBattleVictoryRewardCall::query_group_a_reward_block,
                {.eax = 1U}
            );
        }
        const auto result = run(fixture);
        test.expect_true(
            result.status ==
                    LegacyBattleVictoryRewardStatus::group_a_actor_typed_stop &&
                result.group_a_query_calls == 10U,
            "group-A reward traversal stops on the eleventh actor's first physical skip-field access"
        );
    }

    {
        Fixture fixture;
        fixture.metrics.group_a_count = 1U;
        fixture.startup.action_mode_source.actor_label_indices[0U] = 17U;
        fixture.port.reply(
            LegacyBattleVictoryRewardCall::query_group_a_reward_block,
            {.eax = 0U}
        );
        const auto result = run(fixture);
        test.expect_true(
            result.status ==
                    LegacyBattleVictoryRewardStatus::
                        party_member_resource_typed_stop &&
                fixture.state.party_reward_counters[0U] == 0U,
            "an invalid party profile label stops at its first 56-byte record access before counters and actor calls"
        );
    }

    {
        Fixture fixture;
        fixture.script_variable_count = 0U;
        const auto result = run(fixture);
        test.expect_true(
            result.status ==
                    LegacyBattleVictoryRewardStatus::
                        script_variable_typed_stop &&
                fixture.state.committed_money_word == 0x8005U &&
                fixture.target.transition_timer == 0U,
            "missing shared money storage stops only after publishing the committed bit and clearing the timer"
        );
    }

    {
        Fixture fixture;
        fixture.raster.surface.pitch_bytes = 1279;
        const auto result = run(fixture);
        test.expect_true(
            result.status ==
                    LegacyBattleVictoryRewardStatus::rectangle_typed_stop &&
                result.rectangle_calls == 1U &&
                result.tiled_frame_calls == 0U &&
                fixture.state.panel_action_record.action_id == 0x233BU,
            "invalid panel raster geometry stops after preserving action preparation"
        );

        Fixture tiled;
        tiled.frame_provider.fail = true;
        const auto tiled_result = run(tiled);
        test.expect_true(
            tiled_result.status ==
                    LegacyBattleVictoryRewardStatus::title_frame_typed_stop &&
                tiled_result.rectangle_calls == 1U &&
                tiled_result.tiled_frame_calls == 1U &&
                tiled_result.text_draw_calls == 0U,
            "missing title frame stops before title text and reward distribution"
        );
    }
}
