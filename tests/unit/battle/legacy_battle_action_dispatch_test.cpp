#include "legacy_battle_mon_database_fixture.hpp"
#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "test.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <deque>
#include <memory>
#include <unordered_map>
#include <vector>

namespace {

using openswd3::battle::LegacyBattleActionCallReply;
using openswd3::battle::LegacyBattleActionCallRequest;
using openswd3::battle::LegacyBattleActionMessageProfile;
using openswd3::battle::LegacyBattleGroupAActionExecutionSharedState;
using openswd3::battle::LegacyBattleGroupAActionExecutionState;
using openswd3::battle::LegacyBattleTargetPhaseState;
using openswd3::compat::i32;
using openswd3::compat::u8;
using openswd3::compat::u16;
using openswd3::compat::u32;

class DispatchPort final
    : public openswd3::battle::LegacyBattleActionDispatchPort,
      public openswd3::test::LegacyBattleMonDatabaseFixture {
public:
    [[nodiscard]] LegacyBattleActionCallReply
    invoke(const LegacyBattleActionCallRequest& request) override {
        calls.push_back(request);
        const auto found = replies.find(request.callee_token);
        if (found != replies.end() && !found->second.empty()) {
            const LegacyBattleActionCallReply reply = found->second.front();
            found->second.pop_front();
            return reply;
        }
        if (request.callee_token == 0x004786B0U) {
            return {.eax = action};
        }
        if (request.callee_token == 0x004786C0U) {
            return {.eax = fallback_action};
        }
        if (request.callee_token == 0x0047CE80U) {
            return {.eax = terminal_return};
        }
        if (request.callee_token == 0x00489E90U) {
            return {.eax = 0x90000000U};
        }
        if (request.callee_token == 0x00480AD0U) {
            return {.eax = 0xA0000000U};
        }
        if (request.callee_token == 0x00478620U) {
            return {
                .eax = 0x72000000U,
                .outputs = {0x73000000U, 0x20U, 0x50U},
            };
        }
        if (request.callee_token == 0x00478470U) {
            return {.outputs = {0x30U, 0x40U}};
        }
        if (request.callee_token == 0x004019A0U) {
            return {.eax = 0x74000000U, .resource_words = decoded_pixels};
        }
        return default_reply;
    }

    [[nodiscard]] LegacyBattleActionCallReply
    invoke_special_four_oh_five_update(
        const LegacyBattleActionCallRequest& request,
        openswd3::asset_runtime::LegacyActionRecord& record
    ) override {
        special_four_oh_five_records.push_back(&record);
        return LegacyBattleActionDispatchPort::
            invoke_special_four_oh_five_update(request, record);
    }

    [[nodiscard]] LegacyBattleActionCallReply
    invoke_special_four_oh_six_effect_update(
        const LegacyBattleActionCallRequest& request,
        openswd3::asset_runtime::LegacyActionRecord& record
    ) override {
        special_four_oh_six_effect_records.push_back(&record);
        return LegacyBattleActionDispatchPort::
            invoke_special_four_oh_six_effect_update(request, record);
    }

    [[nodiscard]] LegacyBattleActionCallReply
    invoke_special_four_oh_six_secondary_update(
        const LegacyBattleActionCallRequest& request,
        openswd3::asset_runtime::LegacyActionRecord& record
    ) override {
        special_four_oh_six_secondary_records.push_back(&record);
        return LegacyBattleActionDispatchPort::
            invoke_special_four_oh_six_secondary_update(request, record);
    }

    [[nodiscard]] LegacyBattleActionCallReply
    invoke_special_four_hundred_primary_update(
        const LegacyBattleActionCallRequest& request,
        openswd3::asset_runtime::LegacyActionRecord& record,
        u32& frame_token,
        u32& render_flags,
        u16& draw_x,
        u16& draw_y
    ) override {
        special_four_hundred_primary_records.push_back(&record);
        return LegacyBattleActionDispatchPort::
            invoke_special_four_hundred_primary_update(
                request,
                record,
                frame_token,
                render_flags,
                draw_x,
                draw_y
            );
    }

    [[nodiscard]] LegacyBattleActionCallReply
    invoke_special_four_hundred_workspace_update(
        const LegacyBattleActionCallRequest& request,
        std::span<u8> workspace
    ) override {
        special_four_hundred_workspaces.push_back(workspace.data());
        return LegacyBattleActionDispatchPort::
            invoke_special_four_hundred_workspace_update(request, workspace);
    }

    [[nodiscard]] LegacyBattleActionCallReply
    invoke_special_four_oh_nine_coordinate_update(
        const LegacyBattleActionCallRequest& request,
        openswd3::asset_runtime::LegacyActionRecord& record
    ) override {
        special_four_oh_nine_coordinate_records.push_back(&record);
        return LegacyBattleActionDispatchPort::
            invoke_special_four_oh_nine_coordinate_update(request, record);
    }

    [[nodiscard]] LegacyBattleActionCallReply
    invoke_action_four_oh_two_coordinate_update(
        const LegacyBattleActionCallRequest& request,
        u32& coordinate_x,
        u32& coordinate_y
    ) override {
        action_four_oh_two_coordinate_calls.push_back(request);
        action_four_oh_two_coordinates.push_back({coordinate_x, coordinate_y});
        const auto result = LegacyBattleActionDispatchPort::
            invoke_action_four_oh_two_coordinate_update(
                request, coordinate_x, coordinate_y
            );
        if (coordinate_x_to_change != nullptr &&
            action_four_oh_two_coordinate_calls.size() == 1U) {
            *coordinate_x_to_change = coordinate_x_after_first_update;
        }
        if (coordinate_y_accessible_to_disable != nullptr &&
            action_four_oh_two_coordinate_calls.size() == 1U) {
            *coordinate_y_accessible_to_disable = false;
        }
        return result;
    }

    [[nodiscard]] LegacyBattleActionCallReply
    invoke_action_four_oh_two_particle(
        const openswd3::battle::LegacyBattleActionFourOhTwoParticleCallRequest&
            request,
        u16& spawn_count
    ) override {
        action_four_oh_two_particles.push_back(request);
        spawn_count = static_cast<u16>(spawn_count + 1U);
        return LegacyBattleActionDispatchPort::
            invoke_action_four_oh_two_particle(request, spawn_count);
    }

    [[nodiscard]] LegacyBattleActionCallReply
    invoke_action_four_oh_two_completion(
        const LegacyBattleActionCallRequest& request,
        openswd3::asset_runtime::LegacyActionRecord& record
    ) override {
        action_four_oh_two_completion_records.push_back(&record);
        return LegacyBattleActionDispatchPort::
            invoke_action_four_oh_two_completion(request, record);
    }

    [[nodiscard]] LegacyBattleActionCallReply
    invoke_action_four_direct_effect_update(
        const LegacyBattleActionCallRequest& request,
        openswd3::asset_runtime::LegacyActionRecord& record,
        u32& frame_token,
        u32& render_flags,
        u16& draw_x,
        u16& draw_y
    ) override {
        action_four_direct_effect_records.push_back(&record);
        return LegacyBattleActionDispatchPort::
            invoke_action_four_direct_effect_update(
                request,
                record,
                frame_token,
                render_flags,
                draw_x,
                draw_y
            );
    }

    [[nodiscard]] LegacyBattleActionCallReply
    invoke_special_four_hundred_effect_update(
        const LegacyBattleActionCallRequest& request,
        openswd3::asset_runtime::LegacyActionRecord& record,
        u32& frame_token,
        u32& render_flags,
        u16& draw_x,
        u16& draw_y
    ) override {
        special_four_hundred_effect_records.push_back(&record);
        return LegacyBattleActionDispatchPort::
            invoke_special_four_hundred_effect_update(
                request,
                record,
                frame_token,
                render_flags,
                draw_x,
                draw_y
            );
    }

    [[nodiscard]] openswd3::battle::
        LegacyBattleGroupASummonMaterializationCallReply
        invoke_group_a_summon_materialization(
            const openswd3::battle::
                LegacyBattleGroupASummonMaterializationCallRequest& request
        ) override {
        summon_materialization_calls.push_back(request);
        auto reply = openswd3::battle::LegacyBattleActionDispatchPort::
            invoke_group_a_summon_materialization(request);
        if (request.call ==
            openswd3::battle::LegacyBattleGroupASummonMaterializationCall::
                load_profile) {
            reply.profile_record = summon_profile;
        }
        return reply;
    }

    [[nodiscard]] openswd3::battle::LegacyBattleTextMessageCallReply
    invoke_text_message(
        const openswd3::battle::LegacyBattleTextMessageCallRequest& request
    ) override {
        text_message_calls.push_back(request);
        if (request.call ==
            openswd3::battle::LegacyBattleTextMessageCall::allocate) {
            const u32 token = next_text_message_token;
            next_text_message_token += 0x24U;
            return {.eax = token, .edx = request.edx};
        }
        return {.eax = text_length};
    }

    [[nodiscard]] openswd3::battle::LegacyBattleRetreatCommitCallReply
    invoke_retreat_commit(
        const openswd3::battle::LegacyBattleRetreatCommitCallRequest& request
    ) override {
        retreat_commit_calls.push_back(request);
        if (retreat_commit_replies.empty()) {
            return {.eax = 1U};
        }
        const auto reply = retreat_commit_replies.front();
        retreat_commit_replies.pop_front();
        return reply;
    }

    [[nodiscard]] bool group_b_action_configuration_typed_stop(
        const u32 callee_token
    ) const noexcept override {
        return callee_token == group_b_action_typed_stop_callee;
    }

    [[nodiscard]] std::shared_ptr<const std::array<u8, 0xA4>>
    group_b_action_resource_bytes() const override {
        return group_b_action_definition;
    }

    void push(const u32 callee, const LegacyBattleActionCallReply reply) {
        replies[callee].push_back(reply);
    }

    [[nodiscard]] std::size_t count(const u32 callee) const {
        return static_cast<std::size_t>(std::ranges::count_if(
            calls, [callee](const LegacyBattleActionCallRequest& request) {
                return request.callee_token == callee;
            }
        ));
    }

    u16 action{};
    u16 fallback_action{};
    u32 terminal_return{};
    u32 group_b_action_typed_stop_callee{};
    std::shared_ptr<std::array<u8, 0xA4>> group_b_action_definition;
    LegacyBattleActionCallReply default_reply{.eax = 1U};
    std::unordered_map<u32, std::deque<LegacyBattleActionCallReply>> replies;
    std::vector<LegacyBattleActionCallRequest> calls;
    std::vector<openswd3::asset_runtime::LegacyActionRecord*>
        special_four_oh_five_records;
    std::vector<openswd3::asset_runtime::LegacyActionRecord*>
        special_four_oh_six_effect_records;
    std::vector<openswd3::asset_runtime::LegacyActionRecord*>
        special_four_oh_six_secondary_records;
    std::vector<openswd3::asset_runtime::LegacyActionRecord*>
        special_four_hundred_primary_records;
    std::vector<u8*> special_four_hundred_workspaces;
    std::vector<openswd3::asset_runtime::LegacyActionRecord*>
        special_four_oh_nine_coordinate_records;
    std::vector<LegacyBattleActionCallRequest>
        action_four_oh_two_coordinate_calls;
    std::vector<std::array<u32, 2>> action_four_oh_two_coordinates;
    u16* coordinate_x_to_change{};
    u16 coordinate_x_after_first_update{};
    bool* coordinate_y_accessible_to_disable{};
    std::vector<
        openswd3::battle::LegacyBattleActionFourOhTwoParticleCallRequest>
        action_four_oh_two_particles;
    std::vector<openswd3::asset_runtime::LegacyActionRecord*>
        action_four_oh_two_completion_records;
    std::vector<openswd3::asset_runtime::LegacyActionRecord*>
        action_four_direct_effect_records;
    std::vector<openswd3::asset_runtime::LegacyActionRecord*>
        special_four_hundred_effect_records;
    std::vector<u16> decoded_pixels = std::vector<u16>(0x20U * 0x50U, 0x1234U);
    std::vector<
        openswd3::battle::LegacyBattleGroupASummonMaterializationCallRequest>
        summon_materialization_calls;
    openswd3::battle::LegacyBattleGroupASummonProfileRecord summon_profile{};
    std::vector<openswd3::battle::LegacyBattleTextMessageCallRequest>
        text_message_calls;
    u32 next_text_message_token{0x71000000U};
    u32 text_length{4U};
    std::deque<openswd3::battle::LegacyBattleRetreatCommitCallReply>
        retreat_commit_replies;
    std::vector<openswd3::battle::LegacyBattleRetreatCommitCallRequest>
        retreat_commit_calls;

    [[nodiscard]] openswd3::battle::LegacyBattleMonDatabaseCallReply
    invoke_legacy_battle_mon_database(
        const openswd3::battle::LegacyBattleMonDatabaseCallRequest& request,
        const std::span<u8> destination
    ) override {
        if (group_b_action_typed_stop_callee != 0U &&
            request.call ==
                openswd3::battle::LegacyBattleMonDatabaseCall::
                    allocate_stream) {
            allocation_succeeds = false;
        }
        return openswd3::test::LegacyBattleMonDatabaseFixture::
            invoke_legacy_battle_mon_database(request, destination);
    }

protected:
    [[nodiscard]] std::optional<bool> prepare_definition_record(
        const std::span<u8> destination, const u32
    ) noexcept override {
        if (group_b_action_definition != nullptr) {
            std::copy(
                group_b_action_definition->cbegin(),
                group_b_action_definition->cend(),
                destination.begin()
            );
            return true;
        }
        std::transform(
            summon_profile.cbegin(),
            summon_profile.cend(),
            destination.begin(),
            [](const std::byte value) { return std::to_integer<u8>(value); }
        );
        return true;
    }
};

class ActionStreamProvider final
    : public openswd3::asset_runtime::LegacyActionStreamProvider {
public:
    ActionStreamProvider() {
        constexpr std::array<u16, 8> words{
            0x5246U,
            0x0066U,
            0x5041U,
            0U,
            0x5859U,
            0U,
            0U,
            0x4544U,
        };
        for (const u16 word : words) {
            bytes.push_back(static_cast<u8>(word));
            bytes.push_back(static_cast<u8>(word >> 8U));
        }
    }

    [[nodiscard]] openswd3::asset_runtime::LegacyActionStreamLoadResult
    load_action_stream(u32, u32, bool) override {
        return {
            .status = openswd3::asset_runtime::LegacyActionStreamStatus::ready,
            .stream = bytes,
        };
    }

    std::vector<u8> bytes;
};

class FrameProvider final
    : public openswd3::rendering::LegacyFramePieceProvider {
public:
    FrameProvider() {
        bytes.resize(32U * 32U * sizeof(u16));
        for (std::size_t offset = 0U; offset < bytes.size(); offset += 2U) {
            bytes[offset] = 0x34U;
            bytes[offset + 1U] = 0x12U;
        }
    }

    [[nodiscard]] bool load_frame_piece(
        u32, u32, openswd3::rendering::LegacyFramePiece& piece
    ) noexcept override {
        if (!available) {
            return false;
        }
        piece = {
            .source =
                openswd3::rendering::LegacyBlitSource{
                    .bytes = bytes,
                    .layout =
                        openswd3::rendering::LegacyBlitSourceLayout::direct_16,
                },
            .width = 32U,
            .height = 32U,
        };
        return true;
    }

    std::vector<u8> bytes;
    bool available{true};
};

class RandomPort final
    : public openswd3::battle::LegacyBattleBoundedRandomPort {
public:
    [[nodiscard]] u32 random_bounded(u32) override {
        return value;
    }
    u32 value{};
};

class SoundPort final
    : public openswd3::battle::LegacyBattleIndicatorSoundPort {
public:
    void play_indicator_sound(u16, u16) override {
        ++calls;
    }
    u32 calls{};
};

class CountdownFlags final
    : public openswd3::rendering::LegacyCountdownFlagPorts {
public:
    explicit CountdownFlags(std::array<u8, 16>& bytes) : bytes_(bytes) {}

    [[nodiscard]] bool query_internal_flag(const u32 index) noexcept override {
        return (bytes_[index >> 3U] & static_cast<u8>(1U << (index & 7U))) !=
            0U;
    }

    void set_internal_flag(const u32 index) noexcept override {
        bytes_[index >> 3U] |= static_cast<u8>(1U << (index & 7U));
    }

private:
    std::array<u8, 16>& bytes_;
};

struct Fixture {
    openswd3::rendering::LegacyFramebuffer framebuffer;
    openswd3::rendering::LegacyRasterGeometryState raster;
    openswd3::rendering::LegacyBlitRequest request;
    openswd3::rendering::LegacyBlitEffectState effects;
    openswd3::rendering::LegacyRleRowJitterState jitter;
    ActionStreamProvider stream_provider;
    openswd3::asset_runtime::LegacyActionUpdater action_updater{
        stream_provider
    };
    FrameProvider frame_provider;
    RandomPort random;
    SoundPort sound;
    std::array<u8, 16> flags{};
    CountdownFlags countdown_flags{flags};
    openswd3::battle::LegacyBattleStartupState startup;
    openswd3::battle::LegacyBattleStartupResetBlocks& startup_reset{
        startup.reset
    };
    openswd3::battle::LegacyBattleTextMessageState& text_messages{
        startup.text_messages
    };
    std::array<openswd3::battle::LegacyBattleStartupResetRecord, 0x12>
        attack_order_records{};
    std::array<u32, 0x32> attack_order_party_sources{};
    u32 attack_order_primary_gate{};
    u32 attack_order_secondary_gate{};
    openswd3::battle::LegacyBattleIntensityEffectRecord
        attack_order_adjacent_record{};

    Fixture() {
        startup.group_b_lifecycle = std::make_unique<std::array<
            openswd3::battle::LegacyBattleActorGroupBElementState,
            openswd3::battle::kLegacyBattleActorGroupBElementCount>>();
        startup.party[0U].role_id = 1U;
        startup.party[0U].active = 1U;
        startup.party[0U].configuration.actor_record_token = 0x005029D0U;
        static_cast<void>(
            openswd3::rendering::initialize_legacy_raster_geometry(
                raster, framebuffer.geometry().surface
            )
        );
    }

    [[nodiscard]] openswd3::battle::LegacyBattleActionDispatchContext
    context() {
        return {
            .framebuffer = framebuffer,
            .raster = raster,
            .shared_request = request,
            .shared_effects = effects,
            .jitter = jitter,
            .action_updater = action_updater,
            .frame_provider = frame_provider,
            .bounded_random = random,
            .indicator_sound = sound,
            .countdown_flags = countdown_flags,
            .internal_flags = flags,
            .startup = &startup,
            .startup_reset = &startup_reset,
            .text_messages = &text_messages,
            .attack_order_records = attack_order_records,
            .attack_order_party_sources = attack_order_party_sources,
            .attack_order_primary_gate = &attack_order_primary_gate,
            .attack_order_secondary_gate = &attack_order_secondary_gate,
            .attack_order_adjacent_record = &attack_order_adjacent_record,
            .status_indicator_action_eax_snapshot = 0U,
            .group_a_skip_primary = {},
            .group_a_skip_secondary = {},
            .scripted_resource_release_test_compat = true,
        };
    }
};

void set_summon_profile_word(
    openswd3::battle::LegacyBattleGroupASummonProfileRecord& record,
    const std::size_t offset,
    const u16 value
) noexcept {
    record[offset] = static_cast<std::byte>(static_cast<u8>(value));
    record[offset + 1U] = static_cast<std::byte>(static_cast<u8>(value >> 8U));
}

[[nodiscard]] bool has_call_argument(
    const DispatchPort& port,
    const u32 callee,
    const std::size_t argument,
    const u32 value
) {
    return std::ranges::any_of(
        port.calls, [=](const LegacyBattleActionCallRequest& request) {
            return request.callee_token == callee &&
                request.arguments[argument] == value;
        }
    );
}

}  // namespace

void test_battle_group_b_action_composition_action_caller(
    openswd3::test::Context& test
) {
    using openswd3::battle::LegacyBattleActionDispatchState;
    using openswd3::battle::LegacyBattleActionDispatchStatus;

    {
        auto state = std::make_unique<LegacyBattleActionDispatchState>();
        state->group_a_count = 1;
        state->group_b_count = 1;
        state->group_a_to_actor[0] = 0U;
        state->battle_flags = 0x20U;
        state->stored_group_b_index = 0U;
        state->message_gate = 0x77U;
        auto fixture = std::make_unique<Fixture>();
        auto& actor = (*fixture->startup.group_b_lifecycle)[0U];
        actor.action_composition.derived_words[0U] = 5U;
        DispatchPort port;
        port.action = 25U;
        port.group_b_action_definition =
            std::make_shared<std::array<u8, 0xA4>>();
        (*port.group_b_action_definition)[0U] = 'A';
        (*port.group_b_action_definition)[1U] = 0U;
        (*port.group_b_action_definition)[0x3EU] = 0xEFU;
        (*port.group_b_action_definition)[0x3FU] = 0xBEU;
        (*port.group_b_action_definition)[0x50U] = 0x68U;
        (*port.group_b_action_definition)[0x51U] = 0x24U;
        port.set_profile_word(0x0EU, 3U);
        auto context = fixture->context();

        const auto result = openswd3::battle::dispatch_legacy_battle_action(
            *state, port, context, 0U, 0U
        );
        test.expect_true(
            result.status == LegacyBattleActionDispatchStatus::completed &&
                result.group_b_action_composition_calls == 1U &&
                result.group_b_action_composition.port_calls == 3U &&
                result.attack_order_calls == 1U && state->message_gate == 0U &&
                port.battle_message_state() == 0x2468U &&
                actor.action_composition.action_text[0U] == 'A' &&
                actor.action_composition.derived_words[0U] == 8U &&
                actor.action_composition.action_kind == 0U &&
                actor.action_composition.display_kind == 2U &&
                actor.action_composition.mode_flags == 0x80U &&
                port.count(0x00476DB0U) == 0U &&
                port.count(0x00499168U) == 1U &&
                port.count(0x00476A80U) == 0U && port.open_calls == 1U &&
                port.read_calls == 6U && port.release_calls == 2U &&
                port.requested_definition_ids == std::vector<u32>{0x77U} &&
                port.requested_profile_ids == std::vector<u16>{0xBEEFU} &&
                port.count(0x00476160U) == 0U,
            "action twenty five composes the selected group B actor directly before appending attack order"
        );
        test.expect_true(
            std::ranges::any_of(
                port.calls,
                [](const LegacyBattleActionCallRequest& call) {
                    return call.callee_token == 0x00499168U &&
                        call.arguments[0U] == 0x00527B38U &&
                        call.arguments[1U] == 0x00525518U;
                }
            ),
            "action twenty five preserves the original this pointer definition argument and stale EDX at the reclaimed callsite"
        );
    }

    {
        auto state = std::make_unique<LegacyBattleActionDispatchState>();
        state->group_a_count = 1;
        state->group_b_count = 1;
        state->group_a_to_actor[0] = 0U;
        state->battle_flags = 0x20U;
        state->stored_group_b_index = 0U;
        state->message_gate = 0x55U;
        auto fixture = std::make_unique<Fixture>();
        DispatchPort port;
        port.action = 25U;
        port.group_b_action_definition =
            std::make_shared<std::array<u8, 0xA4>>();
        (*port.group_b_action_definition)[0U] = 'B';
        (*port.group_b_action_definition)[1U] = 0U;
        (*port.group_b_action_definition)[0x50U] = 0x34U;
        (*port.group_b_action_definition)[0x51U] = 0x12U;
        port.group_b_action_typed_stop_callee = 0x00499168U;
        auto context = fixture->context();

        const auto result = openswd3::battle::dispatch_legacy_battle_action(
            *state, port, context, 0U, 0U
        );
        test.expect_true(
            result.status ==
                    LegacyBattleActionDispatchStatus::
                        group_b_action_composition_typed_stop &&
                result.group_b_action_composition.status ==
                    openswd3::battle::
                        LegacyBattleGroupBActionCompositionStatus::
                            resource_load_typed_stop &&
                state->choice_commit == 1U &&
                state->group_b_status_words[0U] == 0x4000U &&
                state->message_gate == 0x55U &&
                port.battle_message_state() == 0U &&
                result.attack_order_calls == 0U &&
                fixture->attack_order_records[0U].value_08 == 0U &&
                port.count(0x00476DB0U) == 0U &&
                port.count(0x00499168U) == 0U &&
                port.count(0x00476A80U) == 0U && port.allocation_calls == 1U &&
                port.release_calls == 0U,
            "action composition typed stop preserves the choice prefix and blocks message cleanup and attack-order suffix"
        );
    }
}

void test_battle_group_b_action_profile_selection_action_caller(
    openswd3::test::Context& test
) {
    using openswd3::battle::LegacyBattleActionDispatchState;
    using openswd3::battle::LegacyBattleActionDispatchStatus;
    using openswd3::battle::LegacyBattleGroupBActionProfileSelectionStatus;

    {
        auto state = std::make_unique<LegacyBattleActionDispatchState>();
        state->group_a_count = 1;
        state->group_b_count = 1;
        state->group_a_to_actor[0U] = 0U;
        state->battle_flags = 0x20U;
        state->stored_group_b_index = 0U;
        state->message_aux = 1U;
        state->choice_state = 2U;
        auto fixture = std::make_unique<Fixture>();
        auto& actor = (*fixture->startup.group_b_lifecycle)[0U];
        actor.resource_token = 0x71000000U;
        actor.resource_bytes[0x76U] = 0x34U;
        actor.resource_bytes[0x77U] = 0x12U;
        actor.action_composition.profile_mode_selector = 0x7777U;
        actor.action_composition.mode_flags = 0x10U;
        DispatchPort port;
        port.action = 25U;
        port.battle_message_state() = 0xDEADU;
        port.set_profile_dword(0x0CU, 2U);
        port.set_profile_word(0x0EU, 0x2468U);
        port.set_profile_word(0x14U, 0x0056U);
        auto context = fixture->context();

        const auto result = openswd3::battle::dispatch_legacy_battle_action(
            *state, port, context, 0U, 0U
        );
        test.expect_true(
            result.status == LegacyBattleActionDispatchStatus::completed &&
                result.group_b_action_profile_selection_calls == 1U &&
                result.group_b_action_profile_selection.status ==
                    LegacyBattleGroupBActionProfileSelectionStatus::completed &&
                result.group_b_action_profile_selection.return_eax == 0U &&
                result.group_b_action_profile_selection.output_value == 0x56U &&
                result.attack_order_calls == 1U &&
                state->group_b_status_words[0U] == 0x8002U &&
                state->message_aux == 1U &&
                port.battle_message_state() == 0x56U &&
                actor.action_composition.profile_mode_selector == 0x7777U &&
                actor.action_composition.derived_words[0U] == 0x2468U &&
                actor.action_composition.action_kind == 0U &&
                actor.action_composition.display_kind == 2U &&
                actor.action_composition.mode_flags == 0x90U &&
                port.count(0x00476A80U) == 0U && port.open_calls == 1U &&
                port.read_calls == 3U && port.release_calls == 1U &&
                port.requested_profile_ids == std::vector<u16>{0x1234U} &&
                port.count(0x00476250U) == 0U,
            "action twenty five directly selects profile mode two with fixed selector zero before attack order"
        );
        test.expect_true(
            port.allocation_calls == 1U &&
                port.requested_profile_ids == std::vector<u16>{0x1234U},
            "action twenty five preserves the typed MON profile identifier after reclaiming 00476250"
        );
    }

    {
        auto state = std::make_unique<LegacyBattleActionDispatchState>();
        state->group_a_count = 1;
        state->group_b_count = 1;
        state->group_a_to_actor[0U] = 0U;
        state->battle_flags = 0x20U;
        state->stored_group_b_index = 0U;
        state->current_actor_index = 5U;
        state->message_aux = 1U;
        auto fixture = std::make_unique<Fixture>();
        auto& actor = (*fixture->startup.group_b_lifecycle)[0U];
        actor.resource_token = 0x72000000U;
        actor.resource_bytes[0x76U] = 0x78U;
        actor.resource_bytes[0x77U] = 0x56U;
        actor.action_configuration.profile_buffer.fill(std::byte{0xFF});
        actor.action_composition.derived_words[0U] = 0x7777U;
        DispatchPort port;
        port.action = 25U;
        port.battle_message_state() = 0x1234U;
        port.allocation_succeeds = false;
        auto context = fixture->context();

        const auto result = openswd3::battle::dispatch_legacy_battle_action(
            *state, port, context, 0U, 0U
        );
        test.expect_true(
            result.status ==
                    LegacyBattleActionDispatchStatus::
                        group_b_action_profile_selection_typed_stop &&
                result.group_b_action_profile_selection.status ==
                    LegacyBattleGroupBActionProfileSelectionStatus::
                        profile_load_typed_stop &&
                state->choice_cursor == 1U && state->choice_commit == 1U &&
                state->group_b_status_words[0U] == 0x8000U &&
                state->current_actor_index == 5U &&
                port.battle_message_state() == 0x1234U &&
                actor.action_configuration.profile_buffer[0U] ==
                    std::byte{0U} &&
                actor.action_composition.derived_words[0U] == 0U &&
                result.attack_order_calls == 0U &&
                fixture->attack_order_records[0U].value_08 == 0U &&
                port.count(0x00476A80U) == 0U && port.allocation_calls == 1U &&
                port.release_calls == 0U && port.count(0x00476250U) == 0U,
            "action profile-selection loader stop preserves choice prefix and blocks status merge attack order and actor cleanup"
        );
    }
}

void test_battle_action_dispatch_invalid_group_a(
    openswd3::test::Context& test
) {
    using openswd3::battle::LegacyBattleActionDispatchState;
    using openswd3::battle::LegacyBattleActionDispatchStatus;

    LegacyBattleActionDispatchState state;
    Fixture fixture;
    DispatchPort port;
    auto context = fixture.context();
    const auto result = openswd3::battle::dispatch_legacy_battle_action(
        state, port, context, 10U, 0U
    );
    test.expect_true(
        result.status ==
                LegacyBattleActionDispatchStatus::group_a_index_typed_stop &&
            result.port_calls == 0U,
        "group A overflow stops at first actor object query"
    );
}

void test_battle_action_dispatch_part_one(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleActionDispatchState;
    using openswd3::battle::LegacyBattleActionDispatchStatus;

    {
        LegacyBattleActionDispatchState state;
        auto fixture = std::make_unique<Fixture>();
        auto port = std::make_unique<DispatchPort>();
        port->action = 1U;
        port->terminal_return = 1U;
        auto context = fixture->context();
        const auto result = openswd3::battle::dispatch_legacy_battle_action(
            state, *port, context, 0U, 99U
        );
        test.expect_true(
            result.status == LegacyBattleActionDispatchStatus::completed &&
                result.return_value == 1U && result.port_calls == 2U,
            "terminal actor returns one before any target access"
        );
    }

    {
        LegacyBattleActionDispatchState state;
        Fixture fixture;
        DispatchPort port;
        port.action = 0U;
        port.fallback_action = 5U;
        auto context = fixture.context();
        const auto result = openswd3::battle::dispatch_legacy_battle_action(
            state, port, context, 0U, 0U
        );
        test.expect_true(
            result.action_code == 5U && result.return_value == 1U &&
                port.count(0x004786C0U) == 1U,
            "zero primary action dispatches the low word returned by fallback query"
        );
    }

    {
        LegacyBattleActionDispatchState state;
        state.stored_group_b_index = 2U;
        state.stored_group_a_index = 3U;
        Fixture fixture;
        DispatchPort port;
        port.action = 0x63U;
        auto context = fixture.context();
        const auto result = openswd3::battle::dispatch_legacy_battle_action(
            state, port, context, 0U, 0U
        );
        test.expect_true(
            result.return_value == 1U && result.terminal_resets == 1U &&
                state.stored_group_b_index == 0xFFFFU &&
                state.stored_group_a_index == 0xFFFFU &&
                state.current_actor_index == 0xFFFFU &&
                state.result_mode == 1U && state.battle_submode == 2U &&
                port.count(0x0047CC40U) == 1U && port.count(0x0047F900U) == 1U,
            "action ninety nine performs exact terminal reset and two mode calls"
        );
    }

    {
        LegacyBattleActionDispatchState state;
        state.action_runtime_flags = 0x8001U;
        state.packed_actor_counter = 0xAABBCCDDU;
        Fixture fixture;
        DispatchPort port;
        port.action = 3U;
        port.actor_metric_state().group_b_count = 0x12U;
        port.retreat_commit_replies = {{.eax = 1U, .edx = 0x87654321U}};
        port.battle_debug_hotkey_state().committed_actor_code = 9U;
        port.battle_debug_overlay_gate() = 9U;
        port.battle_message_state() = 9U;
        auto& actor_list = fixture.startup.party[0U].actor_list;
        actor_list.next_resource_head_token = 0x76000000U;
        actor_list.selected_resource_token = 0x76000010U;
        actor_list.resources = {
            {.token = 0x76000000U, .next_token = 0x76000010U, .name = {}},
            {.token = 0x76000010U,
             .resource_id = 0x34U,
             .secondary_quantity = 2,
             .name = {}},
        };
        auto context = fixture.context();
        context.scripted_resource_release_test_compat = false;

        const auto result = openswd3::battle::dispatch_legacy_battle_action(
            state, port, context, 0U, 0U
        );

        test.expect_true(
            result.status == LegacyBattleActionDispatchStatus::completed &&
                result.retreat_commit_calls == 1U &&
                result.actor_resource_release_calls == 1U &&
                result.actor_resource_release.output_word == 0x34U &&
                actor_list.selected_resource_token == 0U &&
                result.retreat_commit.branch ==
                    openswd3::battle::LegacyBattleRetreatCommitBranch::
                        committed &&
                result.retreat_commit.primary_actor_calls == 1U &&
                port.retreat_commit_calls.size() == 1U &&
                port.count(0x0045EA80U) == 0U &&
                port.count(0x004728D0U) == 0U &&
                state.packed_actor_counter == 0xAABBCC12U &&
                port.retreat_commit_state().completion_gate_a == 1U &&
                port.retreat_commit_state().completion_gate_b == 1U &&
                port.battle_debug_hotkey_state().committed_actor_code == 0U &&
                port.battle_debug_overlay_gate() == 0U &&
                port.battle_message_state() == 0U &&
                port.outcome_resolution_state().darkening_gate == 1U,
            "action three directly finalizes the selected actor and publishes the shared battle state without the old opaque call"
        );
    }

    {
        LegacyBattleActionDispatchState state;
        state.group_a_to_actor[0] = 0U;
        state.action_runtime_flags = 0x8000U;
        state.group_a_action_execution[0U].primary_action_record.field_8c = 1U;
        Fixture fixture;
        DispatchPort port;
        port.battle_pair_primary_value() = 1000U;
        port.action = 1U;
        port.push(0x0047F150U, {.eax = 1U});
        port.push(0x00482E90U, {.eax = 1U});
        auto context = fixture.context();
        const auto result = openswd3::battle::dispatch_legacy_battle_action(
            state, port, context, 0U, 1U
        );
        test.expect_true(
            result.status == LegacyBattleActionDispatchStatus::completed &&
                result.return_value == 0U && state.action_pending == 1U &&
                state.selected_target_index == 1U &&
                state.selected_group_b_identity[1] == 1U &&
                state.frame_refresh_pending == 1U &&
                result.framebuffer_clear_calls == 1U &&
                fixture.framebuffer.physical_pixels()[0] == 0xFFFFU &&
                port.battle_pair_primary_value() == 0U &&
                result.pair_transition_calls == 1U &&
                result.pair_transition.port_calls == 1U,
            "ordinary attack publishes target clears framebuffer pairs actors and returns zero"
        );
    }

    {
        LegacyBattleActionDispatchState state;
        state.side_mode = 1U;
        state.group_a_to_actor[0] = 0U;
        state.action_runtime_flags = 0x8000U;
        state.group_a_action_execution[0U].primary_action_record.field_8c = 1U;
        Fixture fixture;
        DispatchPort port;
        port.battle_pair_primary_value() = 1000U;
        port.action = 1U;
        port.push(0x0047F150U, {.eax = 1U});
        port.push(0x00482E90U, {.eax = 1U});
        auto context = fixture.context();
        const auto result = openswd3::battle::dispatch_legacy_battle_action(
            state, port, context, 0U, 1U
        );
        test.expect_true(
            result.status == LegacyBattleActionDispatchStatus::completed &&
                result.pair_transition_calls == 1U &&
                result.pair_transition.port_calls == 1U &&
                port.battle_pair_primary_value() == 0U,
            "alternate-side ordinary attack directly composes the first pair transition call site"
        );
    }

    {
        LegacyBattleActionDispatchState state;
        state.group_a_to_actor[0] = 0U;
        state.blocking_effect = 1U;
        state.action_runtime_flags = 0x8000U;
        state.group_a_action_execution[0U].primary_action_record.field_8c = 1U;
        Fixture fixture;
        DispatchPort port;
        port.battle_pair_primary_value() = 1000U;
        port.action = 1U;
        port.push(0x00482E90U, {.eax = 8U});
        port.push(0x00482F10U, {.eax = 50U});
        auto context = fixture.context();
        const auto result = openswd3::battle::dispatch_legacy_battle_action(
            state, port, context, 0U, 1U
        );
        test.expect_true(
            result.status == LegacyBattleActionDispatchStatus::completed &&
                has_call_argument(port, 0x0047F150U, 0U, 0U - 550U) &&
                port.count(0x0047CF00U) == 1U && port.count(0x0047CEC0U) == 1U,
            "class eight target attack preserves percentage remainder formula and unsigned negation presentation"
        );
    }

    {
        LegacyBattleActionDispatchState state;
        Fixture fixture;
        DispatchPort port;
        port.action = 2U;
        auto context = fixture.context();
        const auto initialized =
            openswd3::battle::dispatch_legacy_battle_action(
                state, port, context, 0U, 0U
            );
        state.action_runtime_flags |= 1U;
        const auto completed = openswd3::battle::dispatch_legacy_battle_action(
            state, port, context, 0U, 0U
        );
        test.expect_true(
            initialized.status == LegacyBattleActionDispatchStatus::completed &&
                completed.status ==
                    LegacyBattleActionDispatchStatus::completed &&
                port.count(0x00489E90U) == 1U &&
                port.count(0x00489D00U) == 1U && !state.deformation &&
                !state.deformation_active &&
                state.deformation_owner_token == 0U &&
                state.frame_effect.fade_active == 1U,
            "action two constructs deformation after allocation and releases owner only on completed low bit"
        );
    }

    {
        LegacyBattleActionDispatchState state;
        state.group_a_to_actor[0] = 0U;
        auto& actor = state.group_a_action_execution[0U];
        actor.profile_value = 0x123U;
        actor.special_profile_variant = 7U;
        actor.action_runtime_gate = 3U;
        state.group_a_action_shared.action_completion_flags = 1U;
        Fixture fixture;
        DispatchPort port;
        port.battle_pair_primary_value() = 10U;
        port.action = 409U;
        auto context = fixture.context();
        const auto result = openswd3::battle::dispatch_legacy_battle_action(
            state, port, context, 0U, 1U
        );
        test.expect_true(
            result.return_value == 1U && state.current_actor_index == 0xFFFFU &&
                openswd3::compat::u16(state.scan_push_state) == 0x8000U &&
                state.frame_refresh_pending == 1U &&
                result.special_four_oh_nine_calls == 1U &&
                port.count(0x00474E60U) == 0U &&
                port.count(0x0047CC40U) == 1U &&
                has_call_argument(port, 0x0047F150U, 0U, 10U),
            "special action four hundred nine preserves unique screen mode and scan push publication"
        );
    }

    {
        LegacyBattleActionDispatchState state;
        state.group_a_count = 2;
        state.group_b_count = 2;
        state.group_a_to_actor[0] = 0U;
        auto& actor = state.group_a_action_execution[0U];
        actor.profile_value = 0x123U;
        actor.special_profile_variant = 7U;
        actor.action_runtime_gate = 0x8000U;
        actor.special_action_record.field_8c = 1U;
        actor.effect_action_record.field_8c = 1U;
        Fixture fixture;
        DispatchPort port;
        port.action = 0x194U;
        port.push(0x0047D930U, {.eax = 0U});
        auto context = fixture.context();
        const auto result = openswd3::battle::dispatch_legacy_battle_action(
            state, port, context, 0U, 1U
        );
        test.expect_true(
            result.status == LegacyBattleActionDispatchStatus::completed &&
                state.special_phase == 0U &&
                state.frame_effect.split_suppression == 0U &&
                result.action_four_effect_calls == 1U &&
                result.group_a_iterations == 4U &&
                result.group_b_iterations == 4U &&
                port.count(0x004745B0U) == 0U &&
                port.count(0x0047D810U) == 6U && port.count(0x0047D830U) == 6U,
            "special phase four hundred four runs pause action and restore loops in one fallthrough call"
        );
    }

    {
        LegacyBattleActionDispatchState state;
        state.phase_counter = 3U;
        state.phase_condition = 1U;
        state.group_b_count = 1;
        state.packed_actor_counter = 1U;
        Fixture fixture;
        DispatchPort port;
        port.action = 6U;
        auto context = fixture.context();
        const auto result = openswd3::battle::dispatch_legacy_battle_action(
            state, port, context, 0U, 0U
        );
        test.expect_true(
            result.return_value == 1U &&
                openswd3::compat::u16(state.phase_counter) == 0U &&
                state.current_actor_index == 0xFFFFU &&
                state.selected_target_index == 0xFFFFU &&
                state.frame_effect.fade_active == 1U &&
                port.count(0x00485610U) == 1U &&
                result.text_message_calls == 1U &&
                fixture.startup_reset.block_5214f8[0U] == 0x71000000U &&
                fixture.text_messages.allocations[0U].record.text_token ==
                    0x004A77F0U,
            "phase six countdown reaches two emits selected message and clears visual phase state"
        );
    }

    {
        LegacyBattleActionDispatchState state;
        state.group_a_count = 1;
        state.group_b_count = 1;
        state.phase_counter = 1U;
        state.group_b_status_words[0] = 7U;
        state.group_a_target_phases[0U].emitter.initialized = 1;
        state.group_a_target_phases[0U].emitter.remaining_batches = 0U;
        state.group_a_target_phases[0U].emitter.spawned_count = 0;
        Fixture fixture;
        DispatchPort port;
        port.action = 6U;
        port.push(0x00487C10U, {.eax = 0x76000000U});
        port.push(0x00487C10U, {.eax = 0x00630000U});
        auto context = fixture.context();
        const auto result = openswd3::battle::dispatch_legacy_battle_action(
            state, port, context, 0U, 0U
        );
        const auto& item =
            port.world_item_list_state().player_inventory.front();
        test.expect_true(
            result.status == LegacyBattleActionDispatchStatus::completed &&
                result.fixed_count_calls == 1U &&
                result.fixed_count.path ==
                    openswd3::battle::LegacyBattleFixedCountPath::
                        allocated_node &&
                port.count(0x00477710U) == 0U &&
                port.count(0x00487C10U) == 2U &&
                port.legacy_battle_fixed_object_state()
                        .fixed_count_nodes.front()
                        .words[1U] == 0x00010001U &&
                result.player_item_calls == 1U &&
                result.player_item.return_token == 0x0063000CU &&
                item.item_id == 7U && item.quantity_b == 1U,
            "phase six completion directly publishes the target status into the shared player inventory"
        );
    }

    {
        LegacyBattleTargetPhaseState phase;
        LegacyBattleGroupAActionExecutionState actor;
        actor.profile_value = 0x123U;
        LegacyBattleGroupAActionExecutionSharedState shared;
        Fixture fixture;
        DispatchPort port;
        auto context = fixture.context();
        fixture.stream_provider.bytes.clear();
        const auto no_action =
            openswd3::battle::advance_legacy_battle_action_twenty_three(
                &phase,
                &actor,
                &shared,
                port,
                context,
                {.actor_token = 0x12340000U, .opponent_token = 0x525508U}
            );
        actor.primary_action_record = {};
        fixture.stream_provider.bytes = {
            0x46U, 0x52U, 0x66U, 0x00U, 0x41U, 0x50U, 0x00U, 0x00U,
            0x59U, 0x58U, 0x00U, 0x00U, 0x00U, 0x00U, 0x44U, 0x45U,
        };
        fixture.frame_provider.available = false;
        const auto no_frame =
            openswd3::battle::advance_legacy_battle_action_twenty_three(
                &phase,
                &actor,
                &shared,
                port,
                context,
                {.actor_token = 0x12340000U, .opponent_token = 0x525508U}
            );
        actor.primary_action_record = {};
        fixture.frame_provider.available = true;
        const auto no_shared =
            openswd3::battle::advance_legacy_battle_action_twenty_three(
                &phase,
                &actor,
                nullptr,
                port,
                context,
                {.actor_token = 0x12340000U, .opponent_token = 0x525508U}
            );
        actor.primary_action_record = {};
        const auto no_phase =
            openswd3::battle::advance_legacy_battle_action_twenty_three(
                nullptr,
                &actor,
                &shared,
                port,
                context,
                {.actor_token = 0x12340000U, .opponent_token = 0x525508U}
            );
        test.expect_true(
            no_action.return_eax == 0U && no_action.frame_lookup_calls == 0U,
            "action twenty-three returns directly when the updater returns zero"
        );
        test.expect_true(
            no_frame.status ==
                    openswd3::battle::LegacyBattleActionTwentyThreeStatus::
                        frame_owner_typed_stop &&
                no_frame.action_update_calls == 1U &&
                no_frame.frame_lookup_calls == 1U,
            "action twenty-three stops at the original frame dereference"
        );
        test.expect_true(
            no_shared.status ==
                    openswd3::battle::LegacyBattleActionTwentyThreeStatus::
                        shared_state_typed_stop &&
                no_shared.frame_lookup_calls == 1U,
            "action twenty-three publishes the frame token before the shared owner stop"
        );
        test.expect_true(
            no_phase.status ==
                    openswd3::battle::LegacyBattleActionTwentyThreeStatus::
                        phase_state_typed_stop &&
                actor.turn_frame_token == 0x1234254CU && port.calls.empty(),
            "action twenty-three reaches the render toggle owner only after frame publication"
        );
    }

    {
        LegacyBattleTargetPhaseState phase;
        phase.render_toggle_gate = 1U;
        LegacyBattleGroupAActionExecutionState actor;
        actor.profile_value = 0x123U;
        actor.primary_action_record.cached_action_id = 0x123U;
        actor.primary_action_record.cached_base_variant = 0x2BU;
        actor.primary_action_record.draw_offset_x = 4U;
        actor.primary_action_record.draw_offset_y = 0x00010004U;
        actor.primary_action_record.mode_flags = 1U;
        actor.primary_action_record.field_58 = 0x44U;
        LegacyBattleGroupAActionExecutionSharedState shared;
        Fixture fixture;
        fixture.stream_provider.bytes = {
            0x46U,
            0x52U,
            0x66U,
            0x00U,
            0x44U,
            0x45U,
        };
        auto& coordinate_actor =
            (*fixture.startup.group_b_lifecycle)[0U].action_execution;
        coordinate_actor.position_x = 100U;
        coordinate_actor.position_y = 50U;
        DispatchPort port;
        port.push(0x00485610U, {.eax = 0xAAAA1111U, .edx = 0xBBBB2222U});
        auto context = fixture.context();
        const auto result =
            openswd3::battle::advance_legacy_battle_action_twenty_three(
                &phase,
                &actor,
                &shared,
                port,
                context,
                {.actor_token = 0x12340000U, .opponent_token = 0x525508U}
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleActionTwentyThreeStatus::
                        completed &&
                result.return_eax == 0U && result.coordinate_query_calls == 1U &&
                result.sample_play_calls == 1U &&
                result.sample_pan_calls == 1U && result.render_calls == 2U &&
                actor.primary_action_record.draw_offset_x == 28U &&
                actor.primary_action_record.field_1c == 0x8000U &&
                actor.primary_action_record.field_58 == 0U &&
                actor.render_flags == 0x0CU &&
                shared.turn_frame_source_token == 0x1234254CU &&
                shared.draw_height_third == 10U &&
                shared.draw_height_quarter == 8U &&
                shared.draw_motion_a == 0xFFFFFFFAU &&
                shared.draw_motion_b == 0xFFFFFFFAU &&
                shared.draw_motion_c == 0xFFFFFFFAU,
            "action twenty-three updates the primary record and publishes the shared draw state"
        );
        test.expect_true(
            has_call_argument(port, 0x00485610U, 0U, 0x12340044U) &&
                has_call_argument(port, 0x00485650U, 0U, 0xAAAA0044U) &&
                has_call_argument(port, 0x00485650U, 1U, 0xFFFFFFF0U) &&
                has_call_argument(port, 0x004170E0U, 0U, 67U) &&
                has_call_argument(port, 0x004170E0U, 1U, 0x00010028U) &&
                port.calls[3U].arguments[0U] == 72U &&
                port.calls[3U].arguments[1U] == 46U &&
                port.count(0x004783B0U) == 0U,
            "action twenty-three preserves stale sample halves and the asymmetric first-layer y arithmetic"
        );
    }

    {
        LegacyBattleTargetPhaseState phase;
        LegacyBattleGroupAActionExecutionState actor;
        actor.profile_value = 0x123U;
        actor.primary_action_record.cached_action_id = 0x123U;
        actor.primary_action_record.cached_base_variant = 0x2BU;
        actor.primary_action_record.mode_flags = 0x80000003U;
        actor.primary_action_record.field_58 = 0x55U;
        LegacyBattleGroupAActionExecutionSharedState shared;
        Fixture fixture;
        fixture.stream_provider.bytes = {
            0x46U,
            0x52U,
            0x66U,
            0x00U,
            0x44U,
            0x45U,
        };
        auto& coordinate_actor =
            (*fixture.startup.group_b_lifecycle)[0U].action_execution;
        coordinate_actor.position_x = 400U;
        coordinate_actor.position_y = 60U;
        DispatchPort port;
        port.push(0x00485610U, {.eax = 0xAAAA1111U, .edx = 0xBBBB2222U});
        auto context = fixture.context();
        const auto result =
            openswd3::battle::advance_legacy_battle_action_twenty_three(
                &phase,
                &actor,
                &shared,
                port,
                context,
                {.actor_token = 0x12340000U,
                 .opponent_token = 0x525508U,
                 .skip_primary = 1U}
            );
        test.expect_true(
            result.return_eax == 0U && actor.render_flags == 0x8000000FU &&
                shared.draw_motion_a == 0xFFFFFFFFU &&
                has_call_argument(port, 0x00485650U, 0U, 0xBBBB0055U) &&
                has_call_argument(port, 0x00485650U, 1U, 0x10U),
            "action twenty-three uses the returned edx high half for the right-side pan and the skip motion"
        );
    }

    {
        LegacyBattleGroupAActionExecutionState actor;
        LegacyBattleActionMessageProfile empty_profile;
        DispatchPort port;
        const auto empty =
            openswd3::battle::consume_legacy_battle_action_twenty_three_message(
                nullptr,
                &empty_profile,
                port,
                {.profile_token = 0xABCD0000U}
            );
        LegacyBattleActionMessageProfile live_profile{
            .message_code = 0x22U,
            .acceptance_threshold = 3U,
        };
        const auto no_actor =
            openswd3::battle::consume_legacy_battle_action_twenty_three_message(
                nullptr,
                &live_profile,
                port,
                {.actor_token = 0x12340000U, .profile_token = 0xABCD0000U}
            );
        test.expect_true(
            static_cast<u16>(empty.return_eax) == 0x61A8U &&
                empty.port_calls == 0U &&
                no_actor.status ==
                    openswd3::battle::
                        LegacyBattleActionTwentyThreeMessageStatus::
                            actor_state_typed_stop &&
                live_profile.message_code == 0x22U && port.calls.empty(),
            "action twenty-three message query returns the sentinel before actor access and preserves live codes on actor stop"
        );

        port.push(0x00482F10U, {.eax = 0U});
        port.push(0x00439070U, {.eax = 5U});
        const auto rejected =
            openswd3::battle::consume_legacy_battle_action_twenty_three_message(
                &actor,
                &live_profile,
                port,
                {.actor_token = 0x12340000U, .profile_token = 0xABCD0000U}
            );
        test.expect_true(
            static_cast<u16>(rejected.return_eax) == 0U &&
                rejected.percent_refresh_calls == 1U &&
                rejected.random_calls == 1U &&
                rejected.message_code_clears == 0U &&
                live_profile.message_code == 0x22U,
            "action twenty-three message query keeps the code when adjusted random meets the threshold"
        );

        live_profile.acceptance_threshold = 4U;
        port.push(0x00482F10U, {.eax = 50U});
        port.push(0x00439070U, {.eax = 5U});
        const auto accepted =
            openswd3::battle::consume_legacy_battle_action_twenty_three_message(
                &actor,
                &live_profile,
                port,
                {.actor_token = 0x12340000U, .profile_token = 0xABCD0000U}
            );
        test.expect_true(
            static_cast<u16>(accepted.return_eax) == 0x22U &&
                actor.message_percent == 50U && accepted.random_calls == 1U &&
                accepted.message_code_clears == 1U &&
                live_profile.message_code == 0U,
            "action twenty-three message query subtracts the percent quarter and destructively consumes an accepted code"
        );

        live_profile.message_code = 0x44U;
        live_profile.acceptance_threshold = 0U;
        port.push(0x00482F10U, {.eax = 100U});
        const auto certain =
            openswd3::battle::consume_legacy_battle_action_twenty_three_message(
                &actor,
                &live_profile,
                port,
                {.actor_token = 0x12340000U, .profile_token = 0xABCD0000U}
            );
        test.expect_true(
            static_cast<u16>(certain.return_eax) == 0x44U &&
                certain.random_calls == 0U &&
                certain.message_code_clears == 1U &&
                port.count(0x00439070U) == 2U,
            "action twenty-three message query skips random at one hundred and consumes the code"
        );
    }

    {
        LegacyBattleActionDispatchState state;
        state.group_a_count = 1;
        state.group_b_count = 1;
        state.group_a_action_execution[0U].profile_value = 0x123U;
        state.group_a_action_execution[0U]
            .primary_action_record.cached_action_id = 0x123U;
        state.group_a_action_execution[0U]
            .primary_action_record.cached_base_variant = 0x2BU;
        state.group_a_action_execution[0U].primary_action_record.field_8c = 1U;
        state.group_b_message_profiles[0U] = {
            .message_code = 0x22U,
            .acceptance_threshold = 2U,
        };
        Fixture fixture;
        DispatchPort port;
        port.action = 23U;
        port.definition_description = {0x61U};
        port.push(0x00487C10U, {.eax = 0x00640000U});
        auto context = fixture.context();
        const auto result = openswd3::battle::dispatch_legacy_battle_action(
            state, port, context, 0U, 0U
        );
        const auto& item =
            port.world_item_list_state().player_inventory.front();
        test.expect_true(
            result.action_twenty_three_calls == 1U &&
                port.count(0x004721F0U) == 0U,
            "action twenty-three caller removes the old whole-function address"
        );
        test.expect_true(
            result.status == LegacyBattleActionDispatchStatus::completed,
            "action twenty-three caller preserves completed dispatch status"
        );
        test.expect_true(
            result.action_twenty_three.status ==
                    openswd3::battle::LegacyBattleActionTwentyThreeStatus::
                        completed &&
                result.action_twenty_three.return_eax == 1U &&
                result.action_twenty_three.action_record_clears == 1U,
            "action twenty-three caller directly uses the typed completion result"
        );
        test.expect_true(
            result.action_twenty_three_message_calls == 1U &&
                result.action_twenty_three_message.message_code_clears == 1U &&
                state.group_b_message_profiles[0U].message_code == 0U &&
                port.count(0x00472430U) == 0U &&
                port.definition_text_release_calls == 1U &&
                port.legacy_battle_mon_definition_scratch_description()
                    .empty() &&
                result.player_item_calls == 1U &&
                result.player_item.return_token == 0x0064000CU &&
                item.item_id == 0x22U && item.quantity_b == 1U,
            "action twenty-three message path directly publishes selector-one player quantity"
        );
    }

    {
        LegacyBattleActionDispatchState state;
        state.group_a_target_phases[2U].runtime_gate = 1U;
        Fixture fixture;
        DispatchPort port;
        port.action = 13U;
        auto context = fixture.context();
        const auto result = openswd3::battle::dispatch_legacy_battle_action(
            state, port, context, 2U, 3U
        );
        test.expect_true(
            result.return_value == 1U && result.action_thirteen_calls == 1U &&
                result.action_thirteen.return_eax == 1U &&
                port.count(0x004717F0U) == 0U &&
                static_cast<u16>(fixture.startup_reset.block_52022c[10U]) ==
                    4U &&
                state.frame_effect.fade_active == 1U &&
                state.frame_effect.primary_suppression == 0U &&
                state.temporary_record[0x19U] == 0x20U,
            "action thirteen initializes effect then appends target plus one to the shared first-four-row event table"
        );

        LegacyBattleActionDispatchState tail_state;
        tail_state.group_a_target_phases[4U].runtime_gate = 1U;
        Fixture tail_fixture;
        DispatchPort tail_port;
        tail_port.action = 13U;
        auto tail_context = tail_fixture.context();
        const auto tail_result =
            openswd3::battle::dispatch_legacy_battle_action(
                tail_state, tail_port, tail_context, 4U, 2U
            );
        test.expect_true(
            tail_result.return_value == 1U &&
                tail_state.group_a_event_slots_tail[0U] == 3U &&
                tail_fixture.startup_reset.block_52022c[0U] == 0U,
            "action thirteen keeps rows five through ten in the adjacent event-table tail without duplicating the shared prefix"
        );

        LegacyBattleActionDispatchState missing_state;
        missing_state.group_a_target_phases[2U].runtime_gate = 1U;
        Fixture missing_fixture;
        DispatchPort missing_port;
        missing_port.action = 13U;
        auto missing_context = missing_fixture.context();
        missing_context.startup_reset = nullptr;
        const auto missing_result =
            openswd3::battle::dispatch_legacy_battle_action(
                missing_state, missing_port, missing_context, 2U, 3U
            );
        test.expect_true(
            missing_result.status ==
                    LegacyBattleActionDispatchStatus::event_slot_typed_stop &&
                missing_state.frame_effect.fade_active == 1U &&
                missing_state.temporary_record[0x19U] == 0x20U,
            "action thirteen stops at the first shared-prefix slot access after preserving its visual and record prefix"
        );
    }

    {
        LegacyBattleActionDispatchState state;
        state.group_b_count = 1;
        state.status_indicator.tick_counter = 24U;
        state.status_indicator.intensity = 32U;
        state.status_indicator.intensity_countdown = 32U;
        Fixture fixture;
        DispatchPort port;
        port.action = 22U;
        auto context = fixture.context();
        const auto result = openswd3::battle::dispatch_legacy_battle_action(
            state, port, context, 0U, 0U
        );
        test.expect_true(
            result.status == LegacyBattleActionDispatchStatus::completed &&
                result.status_indicator_calls == 1U &&
                result.status_indicator.return_value == 1U &&
                (state.action_runtime_flags & 0x8000U) != 0U &&
                state.scene_value == 1U && state.available_actor_count == 1,
            "action twenty two directly completes status indicator then counts and selects first live opponent"
        );
    }

    {
        LegacyBattleActionDispatchState state;
        state.action_runtime_flags = 0x8001U;
        Fixture fixture;
        DispatchPort port;
        port.action = 22U;
        auto context = fixture.context();
        const auto result = openswd3::battle::dispatch_legacy_battle_action(
            state, port, context, 0U, 0U
        );
        test.expect_true(
            result.return_value == 1U && state.frame_effect.fade_active == 1U &&
                result.status_indicator_calls == 0U,
            "action twenty two completed runtime branch returns one without replaying indicator"
        );
    }

    {
        LegacyBattleActionDispatchState state;
        state.group_a_to_actor[0] = 0U;
        state.action_runtime_flags = 0x8000U;
        state.group_a_action_execution[0U].primary_action_record.field_8c = 1U;
        Fixture fixture;
        fixture.raster.surface.width = 641;
        DispatchPort port;
        port.battle_pair_primary_value() = 9U;
        port.action = 1U;
        auto context = fixture.context();
        const auto result = openswd3::battle::dispatch_legacy_battle_action(
            state, port, context, 0U, 0U
        );
        test.expect_true(
            result.status ==
                    LegacyBattleActionDispatchStatus::framebuffer_typed_stop &&
                result.framebuffer_clear_calls == 1U &&
                state.frame_refresh_pending == 1U &&
                fixture.framebuffer.physical_pixels().front() == 0xFFFFU &&
                fixture.framebuffer.physical_pixels().back() == 0xFFFFU,
            "oversized clear fills owned framebuffer prefix then stops at first out of range pixel"
        );
    }

    {
        LegacyBattleTargetPhaseState phase;
        phase.render_toggle_gate = 1U;
        LegacyBattleGroupAActionExecutionState actor;
        actor.profile_value = 0x123U;
        actor.secondary_auxiliary_word = 5U;
        actor.position_x = 100U;
        actor.position_y = 50U;
        actor.action_flags = 9U;
        actor.copied_runtime_word = 0x1234U;
        actor.primary_action_record.cached_action_id = 0x123U;
        actor.primary_action_record.cached_base_variant = 0x28U;
        actor.primary_action_record.draw_offset_x = 4U;
        actor.primary_action_record.draw_offset_y = 6U;
        actor.primary_action_record.mode_flags = 1U;
        actor.primary_action_record.field_58 = 0x44U;
        LegacyBattleGroupAActionExecutionSharedState shared;
        Fixture fixture;
        fixture.stream_provider.bytes = {
            0x46U, 0x52U, 0x66U, 0x00U, 0x44U, 0x45U,
        };
        DispatchPort port;
        port.push(0x00485610U, {.edx = 0xBBBB2222U});
        auto context = fixture.context();
        const auto special =
            openswd3::battle::advance_legacy_battle_action_twenty_four(
                &phase,
                &actor,
                &shared,
                port,
                context,
                {.actor_token = 0x12340000U}
            );
        test.expect_true(
            static_cast<u16>(special.return_eax) == 0x9234U &&
                special.sample_play_calls == 1U &&
                special.sample_pan_calls == 1U && special.render_calls == 2U &&
                actor.turn_completion_latch == 1U && actor.action_flags == 0U &&
                actor.turn_target_x_offset == 28U &&
                actor.source_x_offset == 27U && actor.turn_render_flags == 0U &&
                actor.render_flags == 0x0DU &&
                shared.draw_height_third == 10U &&
                shared.draw_height_quarter == 8U &&
                shared.draw_motion_a == 0xFFFFFFFAU &&
                actor.primary_action_record.field_58 == 0U,
            "action twenty-four mirrors offsets publishes draw state and returns the copied word with bit fifteen"
        );
        test.expect_true(
            has_call_argument(port, 0x00485610U, 0U, 0x12340044U) &&
                has_call_argument(port, 0x00485650U, 0U, 0xBBBB0044U) &&
                has_call_argument(port, 0x00485650U, 1U, 0xFFFFFFF0U) &&
                has_call_argument(port, 0x004170E0U, 0U, 96U) &&
                has_call_argument(port, 0x004170E0U, 1U, 40U) &&
                port.calls[3U].arguments[0U] == 96U &&
                port.calls[3U].arguments[1U] == 44U,
            "action twenty-four preserves stale sample halves and both original draw coordinate formulas"
        );

        actor.primary_action_record = {};
        actor.primary_action_record.cached_action_id = 0x123U;
        actor.primary_action_record.cached_base_variant = 0x28U;
        actor.primary_action_record.field_8c = 1U;
        actor.action_flags = 0U;
        fixture.frame_provider.available = false;
        const auto missing =
            openswd3::battle::advance_legacy_battle_action_twenty_four(
                &phase,
                &actor,
                &shared,
                port,
                context,
                {.actor_token = 0x12340000U}
            );
        test.expect_true(
            missing.status ==
                    openswd3::battle::LegacyBattleActionTwentyFourStatus::
                        frame_owner_typed_stop &&
                actor.turn_completion_latch == 1U,
            "action twenty-four stops at the original frame dereference after setting its completion latch"
        );

        actor.primary_action_record = {};
        actor.primary_action_record.cached_action_id = 0x123U;
        actor.primary_action_record.cached_base_variant = 0x28U;
        actor.primary_action_record.field_8c = 1U;
        fixture.frame_provider.available = true;
        DispatchPort completion_port;
        const auto completed =
            openswd3::battle::advance_legacy_battle_action_twenty_four(
                &phase,
                &actor,
                &shared,
                completion_port,
                context,
                {.actor_token = 0x12340000U}
            );
        test.expect_true(
            static_cast<u16>(completed.return_eax) == 2U &&
                completed.action_record_clears == 1U &&
                actor.turn_completion_latch == 0U &&
                actor.primary_action_record.action_id == 0U,
            "action twenty-four clears the primary record and latch only after a completed draw"
        );
    }

    {
        LegacyBattleTargetPhaseState phase;
        phase.render_toggle_gate = 1U;
        LegacyBattleGroupAActionExecutionState actor;
        actor.profile_value = 0x123U;
        actor.secondary_auxiliary_word = 5U;
        actor.position_x = 100U;
        actor.position_y = 50U;
        actor.draw_x = 7U;
        actor.draw_y = 8U;
        actor.action_flags = 9U;
        actor.copied_runtime_word = 0x1234U;
        actor.action_twenty_seven_motion_mode = 1U;
        actor.primary_action_record.cached_action_id = 0x123U;
        actor.primary_action_record.cached_base_variant = 0x30U;
        actor.primary_action_record.draw_offset_x = 4U;
        actor.primary_action_record.draw_offset_y = 6U;
        actor.primary_action_record.mode_flags = 1U;
        actor.primary_action_record.field_58 = 0x44U;
        LegacyBattleGroupAActionExecutionSharedState shared;
        Fixture fixture;
        fixture.stream_provider.bytes = {
            0x46U,
            0x52U,
            0x66U,
            0x00U,
            0x44U,
            0x45U,
        };
        auto& coordinate_actor =
            (*fixture.startup.group_b_lifecycle)[0U].action_execution;
        coordinate_actor.position_x = 200U;
        coordinate_actor.position_y = 60U;
        DispatchPort port;
        port.battle_pair_primary_value() = 9U;
        port.push(0x00485610U, {.edx = 0xBBBB2222U});
        port.push(0x00481010U, {.eax = 0x0000FFFFU});
        auto context = fixture.context();
        const auto active =
            openswd3::battle::advance_legacy_battle_action_twenty_seven(
                &phase,
                &actor,
                &shared,
                port,
                context,
                {
                    .actor_token = 0x12340000U,
                    .target_token = 0x00525508U,
                }
            );
        test.expect_true(
            active.return_eax == 0U && active.action_update_calls == 1U &&
                active.frame_lookup_calls == 1U &&
                active.coordinate_query_calls == 1U &&
                active.sample_play_calls == 1U &&
                active.sample_pan_calls == 1U && active.render_calls == 3U &&
                active.target_refresh_calls == 1U &&
                active.effect_compute_calls == 1U &&
                active.effect_publish_calls == 2U &&
                active.secondary_record_calls == 1U &&
                active.effect_value == -1 && shared.last_effect_value == -1 &&
                port.battle_pair_primary_value() == 8U &&
                actor.action_flags == 0U &&
                actor.action_runtime_gate == 0x8000U &&
                actor.effect_action_record.action_id == 0x1234U &&
                actor.effect_action_record.base_variant == 0U,
            "action twenty-seven publishes the signed effect then enters the secondary record phase"
        );
        test.expect_true(
            actor.turn_target_x_offset == 28U && actor.source_x_offset == 27U &&
                actor.turn_render_flags == 0U && actor.render_flags == 0x0CU &&
                shared.draw_height_third == 10U &&
                shared.draw_height_quarter == 8U &&
                shared.draw_motion_a == 0xFFFFFFFFU &&
                has_call_argument(port, 0x00485610U, 0U, 0x12340044U) &&
                has_call_argument(port, 0x00485650U, 0U, 0xBBBB0044U) &&
                has_call_argument(port, 0x00485650U, 1U, 0xFFFFFFF0U) &&
                port.calls[2U].callee_token == 0x004170E0U &&
                port.calls[2U].arguments[0U] == 72U &&
                port.calls[2U].arguments[1U] == 50U &&
                port.calls[3U].arguments[0U] == 72U &&
                port.calls[3U].arguments[1U] == 44U &&
                port.calls[9U].arguments[0U] == 7U &&
                port.calls[9U].arguments[1U] == 8U &&
                port.count(0x004783B0U) == 0U,
            "action twenty-seven preserves mirror offsets stale sample halves and all three draw formulas"
        );

        actor.primary_action_record = {};
        actor.primary_action_record.cached_action_id = 0x123U;
        actor.primary_action_record.cached_base_variant = 0x30U;
        actor.primary_action_record.field_8c = 1U;
        actor.effect_action_record.field_94 = 0xDEADBEEFU;
        DispatchPort completion_port;
        const auto completed =
            openswd3::battle::advance_legacy_battle_action_twenty_seven(
                &phase,
                &actor,
                &shared,
                completion_port,
                context,
                {
                    .actor_token = 0x12340000U,
                    .target_token = 0x00525508U,
                }
            );
        test.expect_true(
            completed.return_eax == 1U &&
                completed.action_record_clears == 2U &&
                completed.effect_compute_calls == 0U &&
                actor.action_runtime_gate == 0U &&
                actor.primary_action_record.action_id == 0U &&
                actor.effect_action_record.field_94 == 0U,
            "action twenty-seven clears both records only after the primary completion flag"
        );
    }

    {
        LegacyBattleTargetPhaseState phase;
        phase.render_toggle_gate = 1U;
        LegacyBattleGroupAActionExecutionState actor;
        actor.profile_value = 0x123U;
        actor.position_x = 100U;
        actor.position_y = 50U;
        actor.action_twenty_seven_motion_mode = 1U;
        LegacyBattleGroupAActionExecutionSharedState shared;
        Fixture fixture;
        fixture.stream_provider.bytes = {
            0x54U, 0x41U, 0x09U, 0x00U, 0x58U, 0x41U, 0x05U,
            0x00U, 0x59U, 0x58U, 0x04U, 0x00U, 0x06U, 0x00U,
            0x46U, 0x52U, 0x44U, 0x00U, 0x32U, 0x4FU,
        };
        auto& coordinate_actor =
            (*fixture.startup.group_b_lifecycle)[0U].action_execution;
        coordinate_actor.position_x = 200U;
        coordinate_actor.position_y = 60U;
        DispatchPort port;
        port.push(
            0x00485610U,
            {.eax = 0xCCCC3333U, .ecx = 0xAAAA1111U, .edx = 0xBBBB2222U}
        );
        auto context = fixture.context();
        const auto completed =
            openswd3::battle::advance_legacy_battle_dual_record_action(
                &phase,
                &actor,
                &shared,
                port,
                context,
                {
                    .actor_token = 0x12340000U,
                    .coordinate_token = 0x00525508U,
                    .secondary_action_id = 0x1965U,
                }
            );
        test.expect_true(
            completed.return_eax == 1U &&
                completed.action_update_calls == 2U &&
                completed.frame_lookup_calls == 2U &&
                completed.coordinate_query_calls == 1U &&
                completed.sample_play_calls == 1U &&
                completed.sample_pan_calls == 1U &&
                completed.render_calls == 3U &&
                completed.action_record_clears == 2U &&
                actor.turn_completion_latch == 1U &&
                actor.primary_action_record.action_id == 0U &&
                actor.effect_action_record.action_id == 0U,
            "dual-record action advances both records and clears them only after secondary completion"
        );
        test.expect_true(
            actor.turn_target_x_offset == 28U && actor.source_x_offset == 27U &&
                actor.turn_render_flags == 1U && actor.render_flags == 0x0DU &&
                shared.draw_height_third == 10U &&
                shared.draw_height_quarter == 8U &&
                shared.draw_motion_a == 0xFFFFFFFFU &&
                has_call_argument(port, 0x00485650U, 0U, 0xAAAA0000U) &&
                has_call_argument(port, 0x00485650U, 1U, 0xFFFFFFF0U) &&
                port.calls[2U].callee_token == 0x004170E0U &&
                port.calls[2U].arguments[0U] == 72U &&
                port.calls[2U].arguments[1U] == 40U &&
                port.calls[3U].arguments[0U] == 72U &&
                port.calls[3U].arguments[1U] == 44U &&
                port.calls[4U].arguments[0U] == 172U &&
                port.calls[4U].arguments[1U] == 54U &&
                port.count(0x004783B0U) == 0U,
            "dual-record action preserves mirror offsets stale sample high halves and all three draw formulas"
        );

        actor = {};
        actor.profile_value = 0x123U;
        fixture.frame_provider.available = false;
        const auto missing =
            openswd3::battle::advance_legacy_battle_dual_record_action(
                &phase,
                &actor,
                &shared,
                port,
                context,
                {
                    .actor_token = 0x12340000U,
                    .coordinate_token = 0x00525508U,
                    .secondary_action_id = 0x1965U,
                }
            );
        test.expect_true(
            missing.status ==
                    openswd3::battle::LegacyBattleDualRecordActionStatus::
                        frame_owner_typed_stop &&
                actor.turn_completion_latch == 1U &&
                actor.turn_frame_token == 0U,
            "dual-record action stops at the original first-frame dereference after publishing the completion latch"
        );
    }

    {
        constexpr std::array<u16, 5> actions{28U, 32U, 34U, 35U, 36U};
        for (const u16 action : actions) {
            LegacyBattleActionDispatchState state;
            state.group_a_action_execution[0U].profile_value = 0x123U;
            Fixture fixture;
            fixture.stream_provider.bytes = {
                0x54U, 0x41U, 0x09U, 0x00U,
                0x46U, 0x52U, 0x44U, 0x00U,
                0x32U, 0x4FU,
            };
            DispatchPort port;
            port.action = action;
            auto context = fixture.context();
            const auto result = openswd3::battle::dispatch_legacy_battle_action(
                state, port, context, 0U, 0U
            );
            test.expect_true(
                result.status == LegacyBattleActionDispatchStatus::completed &&
                    result.dual_record_action_calls == 1U &&
                    result.dual_record_action.return_eax == 1U &&
                    port.count(0x00472CE0U) == 0U,
                "group-A action-family callers advance the typed dual-record action without the opaque call"
            );
        }
    }

    {
        LegacyBattleActionDispatchState state;
        state.group_b_count = 2;
        Fixture fixture;
        fixture.startup.group_b_lifecycle = std::make_unique<std::array<
            openswd3::battle::LegacyBattleActorGroupBElementState,
            openswd3::battle::kLegacyBattleActorGroupBElementCount>>();
        auto& group_b_action =
            (*fixture.startup.group_b_lifecycle)[1U].action_execution;
        group_b_action.profile_value = 0x456U;
        fixture.stream_provider.bytes = {
            0x54U, 0x41U, 0x09U, 0x00U,
            0x46U, 0x52U, 0x44U, 0x00U,
            0x32U, 0x4FU,
        };
        DispatchPort port;
        port.action = 29U;
        auto context = fixture.context();
        const auto result = openswd3::battle::dispatch_legacy_battle_action(
            state, port, context, 0U, 1U
        );
        test.expect_true(
            result.status == LegacyBattleActionDispatchStatus::completed &&
                result.dual_record_action_calls == 1U &&
                result.dual_record_action.return_eax == 1U &&
                group_b_action.turn_completion_latch == 1U &&
                state.group_a_action_execution[0U].turn_completion_latch == 0U &&
                port.count(0x00472CE0U) == 0U,
            "action twenty-nine owns and advances the selected group-B dual-record state"
        );
    }
}

void test_battle_action_dispatch_part_two(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleActionDispatchState;
    using openswd3::battle::LegacyBattleActionDispatchStatus;

    {
        LegacyBattleGroupAActionExecutionState actor;
        actor.profile_value = 0x123U;
        actor.special_profile_variant = 7U;
        actor.special_action_record.field_5a = 0x0202U;
        actor.special_action_record.field_24 = 0x456U;
        actor.special_action_record.field_28 = 0x789U;
        actor.special_action_record.field_78 = 0x55U;
        LegacyBattleGroupAActionExecutionSharedState shared;
        DispatchPort port;
        port.push(0x00483B30U, {.eax = 0U});
        const auto staged =
            openswd3::battle::advance_legacy_battle_special_five_hundred(
                &actor,
                &shared,
                port,
                {
                    .actor_token = 0x005029D0U,
                    .source_token = 0x00525508U,
                }
            );
        test.expect_true(
            staged.return_eax == 0U && staged.special_update_calls == 1U &&
                staged.turn_frame_calls == 1U && staged.port_calls == 2U &&
                actor.turn_completion_latch == 1U &&
                actor.special_action_record.action_id == 0x6FFU &&
                actor.special_action_record.base_variant == 7U &&
                actor.special_action_record.field_24 == 0U &&
                actor.special_action_record.field_28 == 0U &&
                actor.special_action_record.field_5a == 0x0200U &&
                actor.special_action_record.external_mode == 1U &&
                actor.action_runtime_gate == 0x4000U &&
                actor.turn_action_record.action_id == 0x456U &&
                actor.turn_action_record.base_variant == 0x789U,
            "special five-hundred transfers the pending event into the turn record before destructive source clearing"
        );
        test.expect_true(
            has_call_argument(port, 0x004831C0U, 0U, 0x00525508U) &&
                has_call_argument(port, 0x004831C0U, 1U, 0x005034C0U) &&
                has_call_argument(port, 0x00483B30U, 0U, 0x00502E38U) &&
                has_call_argument(port, 0x00483B30U, 1U, 0x55U),
            "special five-hundred preserves both pending-callee object tokens and the record word"
        );

        actor.action_runtime_gate = 0U;
        actor.special_action_record.field_5a = 0x0408U;
        actor.special_action_record.field_7a = 0xFFFFU;
        actor.special_action_record.field_7c = 2U;
        actor.special_action_record.field_7e = 3U;
        actor.special_action_record.field_80 = 4U;
        actor.special_action_record.field_82 = 5U;
        actor.special_action_record.field_84 = 6U;
        actor.special_action_record.field_86 = 7U;
        DispatchPort color_port;
        const auto colored =
            openswd3::battle::advance_legacy_battle_special_five_hundred(
                &actor,
                &shared,
                color_port,
                {
                    .actor_token = 0x005029D0U,
                    .source_token = 0x00525508U,
                }
            );
        const auto& color_state = color_port.battle_color_accumulation_state();
        test.expect_true(
            colored.return_eax == 0U &&
                colored.color_initialization_calls == 1U &&
                color_port.battle_color_initialization_gate() == 1U &&
                shared.action_completion_flags == 0x8000U &&
                actor.special_action_record.field_5a == 8U &&
                color_state.current_red == -1.0F &&
                color_state.current_green == 2.0F &&
                color_state.current_blue == 3.0F &&
                color_state.target_red == 4.0F &&
                color_state.target_green == 5.0F &&
                color_state.target_blue == 6.0F && color_state.countdown == 7,
            "special five-hundred initializes signed color channels once then publishes shared bit fifteen"
        );

        shared.action_completion_flags |= 1U;
        actor.action_runtime_gate = 0xDEAD8EEFU;
        const auto completed =
            openswd3::battle::advance_legacy_battle_special_five_hundred(
                &actor,
                &shared,
                color_port,
                {
                    .actor_token = 0x005029D0U,
                    .source_token = 0x00525508U,
                }
            );
        test.expect_true(
            completed.return_eax == 1U &&
                completed.action_record_clears == 1U &&
                actor.action_runtime_gate == 0U &&
                actor.special_action_record.action_id == 0U &&
                actor.special_action_record.field_5a == 0U,
            "special five-hundred clears its runtime gate and special record only after shared bit zero"
        );

        DispatchPort stopped_port;
        const auto stopped =
            openswd3::battle::advance_legacy_battle_special_five_hundred(
                &actor,
                nullptr,
                stopped_port,
                {
                    .actor_token = 0x005029D0U,
                    .source_token = 0x00525508U,
                }
            );
        test.expect_true(
            stopped.status ==
                    openswd3::battle::LegacyBattleSpecialFiveHundredStatus::
                        shared_state_typed_stop &&
                stopped.special_update_calls == 1U &&
                actor.turn_completion_latch == 1U &&
                stopped_port.count(0x004831C0U) == 1U,
            "special five-hundred stops at the original shared-completion access after the pending update"
        );
    }

    {
        LegacyBattleActionDispatchState state;
        state.group_a_count = 1;
        state.group_b_count = 1;
        state.group_a_action_execution[0U].profile_value = 0x123U;
        state.group_a_action_shared.action_completion_flags = 1U;
        Fixture fixture;
        DispatchPort port;
        port.action = 500U;
        auto context = fixture.context();
        const auto result = openswd3::battle::dispatch_legacy_battle_action(
            state, port, context, 0U, 0U
        );
        test.expect_true(
            result.status == LegacyBattleActionDispatchStatus::completed &&
                result.special_five_hundred_calls == 1U &&
                result.special_five_hundred.return_eax == 1U &&
                state.action_pending == 1U &&
                port.count(0x00473010U) == 0U &&
                port.count(0x004831C0U) == 1U,
            "action five-hundred production advances the typed special record without the opaque call"
        );
    }

    {
        LegacyBattleActionDispatchState dispatch;
        LegacyBattleTargetPhaseState phase;
        phase.tick = 3U;
        LegacyBattleGroupAActionExecutionState actor;
        actor.profile_value = 0x123U;
        actor.special_profile_variant = 7U;
        actor.position_x = 100U;
        actor.position_y = 50U;
        actor.source_y = 70U;
        actor.copied_runtime_word = 0xABCDU;
        actor.action_twenty_seven_motion_mode = 1U;
        actor.special_draw_mirror_mode = 1U;
        actor.special_action_record.cached_action_id = 0x6FFU;
        actor.special_action_record.cached_base_variant = 7U;
        actor.special_action_record.draw_offset_x = 4U;
        actor.special_action_record.draw_offset_y = 6U;
        actor.special_action_record.mode_flags = 1U;
        actor.special_action_record.field_58 = 0x44U;
        actor.special_action_record.field_5a = 9U;
        actor.special_action_record.field_76 = 5U;
        actor.special_action_record.field_78 = 8U;
        actor.effect_action_record.field_94 = 0xDEADBEEFU;
        LegacyBattleGroupAActionExecutionSharedState shared;
        Fixture fixture;
        fixture.stream_provider.bytes = {
            0x46U,
            0x52U,
            0x66U,
            0x00U,
            0x44U,
            0x45U,
        };
        auto& coordinate_actor =
            (*fixture.startup.group_b_lifecycle)[0U].action_execution;
        coordinate_actor.position_x = 200U;
        coordinate_actor.position_y = 60U;
        DispatchPort port;
        port.battle_pair_primary_value() = 9U;
        port.push(0x00481010U, {.eax = 0x0000FFFFU});
        port.push(0x0047CEC0U, {.eax = 1U});
        port.push(0x0047CEC0U, {.eax = 1U});
        port.push(0x0047CEC0U, {.eax = 1U, .edx = 0xBEEF1234U});
        auto context = fixture.context();
        const auto completed =
            openswd3::battle::advance_legacy_battle_special_four_oh_five(
                dispatch,
                &phase,
                &actor,
                &shared,
                port,
                context,
                {
                    .actor_token = 0x12340000U,
                    .target_token = 0x00525508U,
                }
            );
        test.expect_true(
            completed.return_eax == 1U &&
                completed.action_update_calls == 1U &&
                completed.frame_lookup_calls == 1U &&
                completed.sample_play_calls == 1U &&
                completed.sample_pan_calls == 1U &&
                completed.render_calls == 2U &&
                completed.frame_refresh_calls == 1U &&
                completed.coordinate_query_calls == 1U &&
                completed.effect_update_calls == 1U &&
                completed.target_refresh_calls == 1U &&
                completed.effect_compute_calls == 1U &&
                completed.effect_publish_calls == 11U &&
                completed.action_record_clears == 2U &&
                completed.effect_value == -1 && shared.last_effect_value == 1 &&
                port.battle_pair_primary_value() == 8U && phase.tick == 0U &&
                actor.special_action_record.action_id == 0U &&
                actor.effect_action_record.field_94 == 0U,
            "special four-oh-five completes its signed effect publication then clears both records"
        );
        test.expect_true(
            actor.turn_target_x_offset == 28U && actor.source_x_offset == 27U &&
                actor.turn_render_flags == 0U && actor.render_flags == 0x0CU &&
                shared.draw_height_third == 10U &&
                shared.draw_height_quarter == 8U &&
                shared.draw_motion_a == 0xFFFFFFFFU &&
                port.special_four_oh_five_records.size() == 1U &&
                port.special_four_oh_five_records[0U] ==
                    &actor.effect_action_record &&
                has_call_argument(port, 0x00485610U, 0U, 0x12340044U) &&
                has_call_argument(port, 0x0047F940U, 0U, 0x00525508U) &&
                has_call_argument(port, 0x0047F940U, 1U, 0x12340630U) &&
                has_call_argument(port, 0x0047F940U, 3U, 0xABCDU) &&
                has_call_argument(port, 0x0047F940U, 4U, 99U) &&
                has_call_argument(port, 0x0047F940U, 5U, 52U) &&
                has_call_argument(port, 0x0047F940U, 6U, 70U) &&
                port.calls[2U].callee_token == 0x004170E0U &&
                port.calls[2U].arguments[0U] == 67U &&
                port.calls[2U].arguments[1U] == 40U &&
                port.calls[3U].arguments[0U] == 72U &&
                port.calls[3U].arguments[1U] == 44U &&
                port.calls.back().callee_token == 0x0047F150U &&
                port.calls.back().arguments[3U] == 0xBEEF0008U,
            "special four-oh-five preserves mirror offsets two draw formulas pending-effect arguments and low-word commit"
        );

        LegacyBattleGroupAActionExecutionState pending_actor;
        pending_actor.profile_value = 0x123U;
        pending_actor.special_profile_variant = 7U;
        pending_actor.special_action_record.cached_action_id = 0x6FFU;
        pending_actor.special_action_record.cached_base_variant = 7U;
        pending_actor.special_action_record.field_5a = 9U;
        LegacyBattleTargetPhaseState pending_phase;
        coordinate_actor.position_x = 10U;
        coordinate_actor.position_y = 20U;
        DispatchPort pending_port;
        pending_port.push(0x0047F940U, {.eax = 0U});
        const auto pending =
            openswd3::battle::advance_legacy_battle_special_four_oh_five(
                dispatch,
                &pending_phase,
                &pending_actor,
                &shared,
                pending_port,
                context,
                {
                    .actor_token = 0x12340000U,
                    .target_token = 0x00525508U,
                }
            );
        test.expect_true(
            pending.return_eax == 0U && pending.effect_update_calls == 1U &&
                pending.target_refresh_calls == 0U &&
                pending.action_record_clears == 0U &&
                pending_phase.tick == 1U &&
                pending_actor.special_action_record.action_id == 0x6FFU,
            "special four-oh-five retains the incremented phase tick and both records while the pending effect is incomplete"
        );

        LegacyBattleGroupAActionExecutionState stopped_actor;
        stopped_actor.profile_value = 0x123U;
        stopped_actor.special_profile_variant = 7U;
        stopped_actor.special_action_record.cached_action_id = 0x6FFU;
        stopped_actor.special_action_record.cached_base_variant = 7U;
        stopped_actor.special_action_record.field_5a = 9U;
        fixture.frame_provider.available = false;
        DispatchPort missing_port;
        const auto missing =
            openswd3::battle::advance_legacy_battle_special_four_oh_five(
                dispatch,
                &phase,
                &stopped_actor,
                &shared,
                missing_port,
                context,
                {
                    .actor_token = 0x12340000U,
                    .target_token = 0x56780000U,
                }
            );
        test.expect_true(
            missing.status == openswd3::battle::
                    LegacyBattleSpecialFourOhFiveStatus::
                        frame_owner_typed_stop &&
                stopped_actor.turn_completion_latch == 1U &&
                stopped_actor.turn_frame_token == 0U,
            "special four-oh-five stops at the original frame dereference after the action update"
        );

        fixture.frame_provider.available = true;
        LegacyBattleGroupAActionExecutionState shared_stop_actor;
        shared_stop_actor.profile_value = 0x123U;
        shared_stop_actor.special_profile_variant = 7U;
        shared_stop_actor.special_action_record.cached_action_id = 0x6FFU;
        shared_stop_actor.special_action_record.cached_base_variant = 7U;
        const auto shared_stop =
            openswd3::battle::advance_legacy_battle_special_four_oh_five(
                dispatch,
                &phase,
                &shared_stop_actor,
                nullptr,
                missing_port,
                context,
                {
                    .actor_token = 0x12340000U,
                    .target_token = 0x56780000U,
                }
            );
        test.expect_true(
            shared_stop.status == openswd3::battle::
                    LegacyBattleSpecialFourOhFiveStatus::
                        shared_state_typed_stop &&
                shared_stop_actor.turn_frame_token == 0x1234254CU,
            "special four-oh-five stops at the first shared draw-state write after publishing the frame token"
        );

        LegacyBattleGroupAActionExecutionState phase_stop_actor;
        phase_stop_actor.profile_value = 0x123U;
        phase_stop_actor.special_profile_variant = 7U;
        phase_stop_actor.special_action_record.cached_action_id = 0x6FFU;
        phase_stop_actor.special_action_record.cached_base_variant = 7U;
        phase_stop_actor.special_action_record.field_5a = 9U;
        DispatchPort phase_stop_port;
        const auto phase_stop =
            openswd3::battle::advance_legacy_battle_special_four_oh_five(
                dispatch,
                nullptr,
                &phase_stop_actor,
                &shared,
                phase_stop_port,
                context,
                {
                    .actor_token = 0x12340000U,
                    .target_token = 0x00525508U,
                }
            );
        test.expect_true(
            phase_stop.status == openswd3::battle::
                    LegacyBattleSpecialFourOhFiveStatus::
                        phase_state_typed_stop &&
                phase_stop.coordinate_query_calls == 1U &&
                phase_stop.effect_update_calls == 0U,
            "special four-oh-five stops at the original phase-tick access after drawing and querying coordinates"
        );
    }

    {
        LegacyBattleActionDispatchState state;
        state.group_a_count = 1;
        state.group_b_count = 1;
        auto& actor = state.group_a_action_execution[0U];
        actor.profile_value = 0x123U;
        actor.special_profile_variant = 7U;
        actor.special_action_record.cached_action_id = 0x6FFU;
        actor.special_action_record.cached_base_variant = 7U;
        actor.special_action_record.field_5a = 9U;
        Fixture fixture;
        fixture.stream_provider.bytes = {
            0x46U,
            0x52U,
            0x66U,
            0x00U,
            0x44U,
            0x45U,
        };
        auto& coordinate_actor =
            (*fixture.startup.group_b_lifecycle)[0U].action_execution;
        coordinate_actor.position_x = 10U;
        coordinate_actor.position_y = 20U;
        DispatchPort port;
        port.action = 405U;
        port.push(0x00481010U, {.eax = 1U});
        auto context = fixture.context();
        const auto result = openswd3::battle::dispatch_legacy_battle_action(
            state, port, context, 0U, 0U
        );
        test.expect_true(
            result.status == LegacyBattleActionDispatchStatus::completed &&
                result.special_four_oh_five_calls == 1U &&
                result.special_four_oh_five.return_eax == 1U &&
                state.action_pending == 1U &&
                port.count(0x004731A0U) == 0U &&
                port.count(0x0047F940U) == 1U,
            "action four-oh-five production advances the typed effect-and-render path without the opaque call"
        );
    }

    {
        const auto prepare_actor = [] {
            LegacyBattleGroupAActionExecutionState actor;
            actor.profile_value = 0x123U;
            actor.special_profile_variant = 7U;
            actor.position_x = 100U;
            actor.position_y = 50U;
            actor.special_action_record.cached_action_id = 0x6FFU;
            actor.special_action_record.cached_base_variant = 7U;
            actor.special_action_record.draw_offset_x = 4U;
            actor.special_action_record.draw_offset_y = 6U;
            actor.special_action_record.mode_flags = 1U;
            actor.special_action_record.field_58 = 0x44U;
            actor.special_action_record.field_76 = 5U;
            actor.special_action_record.field_78 = 8U;
            actor.special_secondary_action_record.cached_action_id = 0x17FEU;
            actor.special_secondary_action_record.draw_offset_x = 2U;
            actor.special_secondary_action_record.mode_flags = 2U;
            actor.special_secondary_action_record.field_58 = 0x55U;
            actor.special_secondary_action_record.field_76 = 4U;
            actor.special_secondary_action_record.field_78 = 3U;
            return actor;
        };
        LegacyBattleGroupAActionExecutionSharedState shared;
        Fixture fixture;
        fixture.stream_provider.bytes = {
            0x46U, 0x52U, 0x66U, 0x00U, 0x44U, 0x45U,
        };
        auto context = fixture.context();

        auto base_actor = prepare_actor();
        DispatchPort base_port;
        const auto base =
            openswd3::battle::advance_legacy_battle_special_four_oh_six(
                &base_actor,
                &shared,
                base_port,
                context,
                {
                    .actor_token = 0x12340000U,
                    .target_token = 0x56780000U,
                }
            );
        test.expect_true(
            base.return_eax == 0U && base.action_update_calls == 1U &&
                base.frame_lookup_calls == 1U &&
                base.sample_play_calls == 1U && base.sample_pan_calls == 1U &&
                base.render_calls == 2U && base_actor.action_runtime_gate == 0U &&
                base_actor.special_action_record.field_58 == 0U &&
                base_port.calls[2U].arguments[0U] == 91U &&
                base_port.calls[2U].arguments[1U] == 40U &&
                base_port.calls[3U].arguments[0U] == 96U &&
                base_port.calls[3U].arguments[1U] == 44U,
            "special four-oh-six draws the two base layers only while the runtime gate is zero"
        );

        auto staged_actor = prepare_actor();
        staged_actor.special_draw_mirror_mode = 1U;
        staged_actor.special_action_record.field_5a = 8U;
        DispatchPort staged_port;
        const auto staged =
            openswd3::battle::advance_legacy_battle_special_four_oh_six(
                &staged_actor,
                &shared,
                staged_port,
                context,
                {
                    .actor_token = 0x12340000U,
                    .target_token = 0x56780000U,
                }
            );
        test.expect_true(
            staged.return_eax == 0U && staged.action_update_calls == 2U &&
                staged.frame_lookup_calls == 2U &&
                staged.sample_play_calls == 1U &&
                staged.sample_pan_calls == 1U && staged.render_calls == 2U &&
                staged_actor.action_runtime_gate == 3U &&
                staged_actor.turn_threshold == 0xFFFFU &&
                staged_actor.motion_word == 0xFFE2U &&
                staged_actor.source_x_offset == 27U &&
                staged_actor.secondary_target_x_offset == 30U &&
                staged_actor.secondary_source_x_offset == 28U &&
                staged_port.calls[0U].arguments[0U] == 72U &&
                staged_port.calls[0U].arguments[1U] == 44U &&
                staged_port.calls.back().arguments[0U] == 71U &&
                staged_port.calls.back().arguments[1U] == 49U,
            "special four-oh-six starts both low-bit phases in one frame and advances their signed words independently"
        );

        auto pending_actor = prepare_actor();
        pending_actor.action_runtime_gate = 4U;
        pending_actor.effect_action_record.field_94 = 0xAABBCCDDU;
        DispatchPort pending_port;
        pending_port.push(0x0047F940U, {.eax = 0U});
        const auto pending =
            openswd3::battle::advance_legacy_battle_special_four_oh_six(
                &pending_actor,
                &shared,
                pending_port,
                context,
                {
                    .actor_token = 0x12340000U,
                    .target_token = 0x56780000U,
                }
            );
        test.expect_true(
            pending.return_eax == 0U && pending.effect_update_calls == 1U &&
                pending.target_refresh_calls == 0U &&
                pending_actor.action_runtime_gate == 4U &&
                pending_actor.effect_action_record.field_94 == 0xAABBCCDDU &&
                pending_port.special_four_oh_six_effect_records.size() == 1U &&
                pending_port.special_four_oh_six_effect_records[0U] ==
                    &pending_actor.effect_action_record &&
                has_call_argument(pending_port, 0x0047F940U, 3U, 0x17FEU),
            "special four-oh-six preserves gate four and the effect record while the pending update is incomplete"
        );

        auto effect_actor = prepare_actor();
        effect_actor.action_runtime_gate = 4U;
        DispatchPort effect_port;
        effect_port.battle_pair_primary_value() = 9U;
        effect_port.push(0x0047F940U, {.eax = 1U});
        effect_port.push(0x00481010U, {.eax = 0x0000FFFFU});
        effect_port.push(0x00483DB0U, {.eax = 0U});
        const auto effect =
            openswd3::battle::advance_legacy_battle_special_four_oh_six(
                &effect_actor,
                &shared,
                effect_port,
                context,
                {
                    .actor_token = 0x12340000U,
                    .target_token = 0x56780000U,
                    .stale_stack_word_8 = 0x1234U,
                    .stale_stack_word_6 = 0x5678U,
                }
            );
        test.expect_true(
            effect.return_eax == 0U && effect.effect_update_calls == 1U &&
                effect.target_refresh_calls == 1U &&
                effect.effect_compute_calls == 1U &&
                effect.effect_publish_calls == 2U &&
                effect.secondary_update_calls == 1U &&
                effect.effect_value == -1 &&
                effect_actor.action_runtime_gate == 0x4000U &&
                effect_actor.turn_threshold == 0xFFE1U &&
                effect_port.battle_pair_primary_value() == 8U &&
                effect_port.special_four_oh_six_secondary_records.size() == 1U &&
                has_call_argument(effect_port, 0x00481010U, 1U, 0x5678U) &&
                has_call_argument(effect_port, 0x00481010U, 2U, 0x1234U),
            "special four-oh-six publishes the signed effect then waits at the fourth-record gate with stale stack words preserved"
        );

        auto final_actor = prepare_actor();
        final_actor.action_runtime_gate = 0x4000U;
        final_actor.turn_threshold = 0xFFE1U;
        DispatchPort final_port;
        final_port.push(0x00483DB0U, {.eax = 1U});
        const auto final_frame =
            openswd3::battle::advance_legacy_battle_special_four_oh_six(
                &final_actor,
                &shared,
                final_port,
                context,
                {
                    .actor_token = 0x12340000U,
                    .target_token = 0x56780000U,
                }
            );
        test.expect_true(
            final_frame.return_eax == 0U &&
                final_frame.secondary_update_calls == 1U &&
                final_frame.render_calls == 1U &&
                final_actor.action_runtime_gate == 0x2000U &&
                final_actor.turn_threshold == 0xFFE3U &&
                final_port.special_four_oh_six_secondary_records[0U] ==
                    &final_actor.effect_secondary_action_record,
            "special four-oh-six transitions the fourth record to the final signed two-step draw in the same frame"
        );

        auto completed_actor = prepare_actor();
        completed_actor.action_runtime_gate = 0x2000U;
        completed_actor.turn_threshold = 1U;
        completed_actor.special_secondary_action_record.field_94 = 1U;
        completed_actor.effect_action_record.field_94 = 2U;
        completed_actor.effect_secondary_action_record.field_94 = 3U;
        DispatchPort completed_port;
        const auto completed =
            openswd3::battle::advance_legacy_battle_special_four_oh_six(
                &completed_actor,
                &shared,
                completed_port,
                context,
                {
                    .actor_token = 0x12340000U,
                    .target_token = 0x56780000U,
                }
            );
        test.expect_true(
            completed.return_eax == 1U &&
                completed.action_record_clears == 4U &&
                completed_actor.action_runtime_gate == 0U &&
                completed_actor.turn_threshold == 0U &&
                completed_actor.special_action_record.action_id == 0U &&
                completed_actor.special_secondary_action_record.field_94 == 0U &&
                completed_actor.effect_action_record.field_94 == 0U &&
                completed_actor.effect_secondary_action_record.field_94 == 0U,
            "special four-oh-six clears all four records only after the final threshold becomes positive"
        );

        auto missing_actor = prepare_actor();
        fixture.frame_provider.available = false;
        DispatchPort missing_port;
        const auto missing =
            openswd3::battle::advance_legacy_battle_special_four_oh_six(
                &missing_actor,
                &shared,
                missing_port,
                context,
                {
                    .actor_token = 0x12340000U,
                    .target_token = 0x56780000U,
                }
            );
        test.expect_true(
            missing.status == openswd3::battle::
                    LegacyBattleSpecialFourOhSixStatus::frame_owner_typed_stop &&
                missing_actor.turn_completion_latch == 1U &&
                missing_actor.turn_frame_token == 0U,
            "special four-oh-six stops at the original frame dereference after preserving initialization"
        );

        fixture.frame_provider.available = true;
        auto shared_stop_actor = prepare_actor();
        const auto shared_stop =
            openswd3::battle::advance_legacy_battle_special_four_oh_six(
                &shared_stop_actor,
                nullptr,
                missing_port,
                context,
                {
                    .actor_token = 0x12340000U,
                    .target_token = 0x56780000U,
                }
            );
        test.expect_true(
            shared_stop.status == openswd3::battle::
                    LegacyBattleSpecialFourOhSixStatus::shared_state_typed_stop &&
                shared_stop_actor.turn_frame_token == 0x1234254CU,
            "special four-oh-six stops at its first shared-frame publication after the frame token write"
        );
    }

    {
        LegacyBattleActionDispatchState state;
        state.group_a_count = 1;
        state.group_b_count = 1;
        auto& actor = state.group_a_action_execution[0U];
        actor.profile_value = 0x123U;
        actor.special_profile_variant = 7U;
        actor.special_action_record.cached_action_id = 0x6FFU;
        actor.special_action_record.cached_base_variant = 7U;
        actor.action_runtime_gate = 0x2000U;
        actor.turn_threshold = 1U;
        Fixture fixture;
        fixture.stream_provider.bytes = {
            0x46U, 0x52U, 0x66U, 0x00U, 0x44U, 0x45U,
        };
        DispatchPort port;
        port.action = 406U;
        auto context = fixture.context();
        const auto result = openswd3::battle::dispatch_legacy_battle_action(
            state, port, context, 0U, 0U
        );
        test.expect_true(
            result.status == LegacyBattleActionDispatchStatus::completed &&
                result.special_four_oh_six_calls == 1U &&
                result.special_four_oh_six.return_eax == 1U &&
                state.action_pending == 1U &&
                port.count(0x004735B0U) == 0U,
            "action four-oh-six production advances the typed four-record state machine without the opaque call"
        );
    }

    {
        const auto prepare_actor = [] {
            LegacyBattleGroupAActionExecutionState actor;
            actor.profile_value = 0x123U;
            actor.special_profile_variant = 7U;
            actor.special_action_record.field_4a = 1U;
            actor.special_action_record.field_4c = 1U;
            actor.special_primary_draw_x = 100U;
            actor.special_primary_draw_y = 50U;
            actor.turn_render_flags = 1U;
            actor.special_target_action_record.cached_action_id = 0x6FFU;
            actor.special_target_action_record.cached_base_variant = 0x2FU;
            return actor;
        };
        LegacyBattleGroupAActionExecutionSharedState shared;
        openswd3::battle::LegacyBattleActorProgressState progress;
        Fixture fixture;
        auto context = fixture.context();

        auto gated_actor = prepare_actor();
        gated_actor.start_gate = 1U;
        DispatchPort gated_port;
        const auto gated =
            openswd3::battle::advance_legacy_battle_special_four_hundred(
                &gated_actor,
                &progress,
                &shared,
                gated_port,
                context,
                {
                    .actor_token = 0x12340000U,
                    .target_token = 0x56780000U,
                }
            );
        test.expect_true(
            gated.return_eax == 0U && gated.port_calls == 0U &&
                gated_actor.turn_completion_latch == 0U,
            "special four hundred preserves the two entry gates before every side effect"
        );

        auto outward_actor = prepare_actor();
        outward_actor.special_action_record.field_5a = 0x0108U;
        outward_actor.special_target_action_record.field_4c = 2U;
        DispatchPort outward_port;
        const auto outward =
            openswd3::battle::advance_legacy_battle_special_four_hundred(
                &outward_actor,
                &progress,
                &shared,
                outward_port,
                context,
                {
                    .actor_token = 0x12340000U,
                    .target_token = 0x56780000U,
                }
            );
        test.expect_true(
            outward.return_eax == 0U &&
                outward.special_update_calls == 1U &&
                outward.render_calls == 2U &&
                outward_actor.turn_countdown == 10 &&
                outward_actor.special_action_record.external_mode == 1U &&
                outward_actor.special_target_action_record.external_mode == 1U &&
                outward_actor.special_four_hundred_workspace != nullptr &&
                (*outward_actor.special_four_hundred_workspace)[0x92U] == 1U &&
                (*outward_actor.special_four_hundred_workspace)[0x04U] == 0xFFU &&
                (*outward_actor.special_four_hundred_workspace)[0x05U] == 0xFFU &&
                shared.special_render_mode == 8U &&
                shared.draw_motion_c == 10U &&
                outward_port.special_four_hundred_primary_records.size() == 1U,
            "special four hundred initializes the first workspace and advances the outward ten-pixel phase"
        );

        auto handoff_actor = prepare_actor();
        handoff_actor.special_action_record.field_5a = 0x0108U;
        handoff_actor.special_target_action_record.field_4c = 2U;
        handoff_actor.special_target_action_record.field_8c = 1U;
        handoff_actor.turn_countdown = 140;
        DispatchPort handoff_port;
        const auto handoff =
            openswd3::battle::advance_legacy_battle_special_four_hundred(
                &handoff_actor,
                &progress,
                &shared,
                handoff_port,
                context,
                {
                    .actor_token = 0x12340000U,
                    .target_token = 0x56780000U,
                }
            );
        test.expect_true(
            handoff.return_eax == 0U && handoff.render_calls == 4U &&
                handoff_actor.turn_countdown == 140 &&
                handoff_actor.special_four_hundred_phase == 1U &&
                handoff_actor.special_action_record.field_5a == 0U &&
                handoff_actor.special_target_action_record.external_mode == 1U,
            "special four hundred reaches signed one-fifty and begins the reverse phase in the same frame"
        );

        auto target_actor = prepare_actor();
        target_actor.special_four_hundred_phase = 1U;
        target_actor.special_target_action_record.field_58 = 0x44U;
        target_actor.special_target_action_record.field_76 = 4U;
        auto& coordinate_actor =
            (*fixture.startup.group_b_lifecycle)[0U].action_execution;
        coordinate_actor.position_x = 200U;
        coordinate_actor.position_y = 60U;
        DispatchPort target_port;
        const auto target =
            openswd3::battle::advance_legacy_battle_special_four_hundred(
                &target_actor,
                &progress,
                &shared,
                target_port,
                context,
                {
                    .actor_token = 0x12340000U,
                    .target_token = 0x00525508U,
                    .coordinate_output_x_token = 0x11112222U,
                    .coordinate_output_y_token = 0x33334444U,
                    .coordinate_output_x_initial = 0xAAAA5555U,
                    .coordinate_output_y_initial = 0xBBBB6666U,
                }
            );
        test.expect_true(
            target.return_eax == 0U && target.action_update_calls == 1U &&
                target.frame_lookup_calls == 2U &&
                target.coordinate_query_calls == 1U &&
                target.coordinate_output_x == 0xAAAA00C8U &&
                target.coordinate_output_y == 0xBBBB003CU &&
                target.coordinate_query.return_eax == 0x11112222U &&
                target.coordinate_query.return_edx == 0x33334444U &&
                target.workspace_update_calls == 1U &&
                target.sample_play_calls == 1U && target.render_calls == 1U &&
                target_actor.special_target_action_record.field_58 == 0U &&
                target_actor.special_four_hundred_workspace != nullptr &&
                (*target_actor.special_four_hundred_workspace)[0x98U + 0x90U] ==
                    2U &&
                (*target_actor.special_four_hundred_workspace)[0x98U + 0x94U] ==
                    3U &&
                (*target_actor.special_four_hundred_workspace)[0x98U + 0x95U] ==
                    1U &&
                target_port.special_four_hundred_workspaces.size() == 1U &&
                target_port.special_four_hundred_workspaces[0U] ==
                    target_actor.special_four_hundred_workspace->data() +
                        0x98U &&
                target_port.count(0x004783B0U) == 0U,
            "special four hundred updates the target action and passes the initialized secondary workspace to its narrow callee"
        );

        auto partial_actor = prepare_actor();
        partial_actor.special_four_hundred_phase = 1U;
        partial_actor.special_target_action_record.field_58 = 0x44U;
        partial_actor.special_target_action_record.field_76 = 4U;
        coordinate_actor.position_y_read_accessible = false;
        DispatchPort partial_port;
        const auto partial =
            openswd3::battle::advance_legacy_battle_special_four_hundred(
                &partial_actor,
                &progress,
                &shared,
                partial_port,
                context,
                {
                    .actor_token = 0x12340000U,
                    .target_token = 0x00525508U,
                    .coordinate_output_x_token = 0x11112222U,
                    .coordinate_output_y_token = 0x33334444U,
                    .coordinate_output_x_initial = 0xAAAA5555U,
                    .coordinate_output_y_initial = 0xBBBB6666U,
                }
            );
        test.expect_true(
            partial.status ==
                    openswd3::battle::LegacyBattleSpecialFourHundredStatus::
                        actor_coordinate_typed_stop &&
                partial.coordinate_query.status ==
                    openswd3::battle::LegacyBattleActorCoordinateQueryStatus::
                        primary_y_read_typed_stop &&
                partial.coordinate_query.output_writes == 1U &&
                partial.coordinate_output_x == 0xAAAA00C8U &&
                partial.coordinate_output_y == 0xBBBB6666U &&
                partial.workspace_update_calls == 0U &&
                partial.render_calls == 0U && partial.sample_play_calls == 1U,
            "special four hundred preserves the arg-four high word and the written X slot when Y faults"
        );
        coordinate_actor.position_y_read_accessible = true;

        auto layered_actor = prepare_actor();
        layered_actor.special_four_hundred_phase = 1U;
        layered_actor.copied_runtime_word = 0x0777U;
        layered_actor.special_target_action_record.field_5a = 8U;
        layered_actor.special_target_action_record.field_76 = 4U;
        DispatchPort layered_port;
        const auto layered =
            openswd3::battle::advance_legacy_battle_special_four_hundred(
                &layered_actor,
                &progress,
                &shared,
                layered_port,
                context,
                {
                    .actor_token = 0x12340000U,
                    .target_token = 0x00525508U,
                    .entry_eax = 0xAAAA0000U,
                    .entry_ecx = 0xBBBB0000U,
                    .entry_edx = 0xCCCC0000U,
                }
            );
        test.expect_true(
            layered.return_eax == 0U && layered.action_update_calls == 2U &&
                layered.frame_lookup_calls == 3U &&
                layered.sample_play_calls == 2U &&
                layered.sample_pan_calls == 1U &&
                layered.render_calls == 2U &&
                (layered_actor.action_runtime_gate & 0x800U) != 0U &&
                layered_actor.effect_secondary_action_record.action_id ==
                    0x0777U &&
                layered_actor.special_target_action_record.field_5a == 0U,
            "special four hundred starts and draws the optional layered action while preserving its gate"
        );

        auto effect_actor = prepare_actor();
        effect_actor.special_action_record.field_5a = 1U;
        effect_actor.effect_action_record.field_4a = 1U;
        effect_actor.effect_action_record.field_4c = 1U;
        effect_actor.render_flags = 5U;
        effect_actor.draw_x = 12U;
        effect_actor.draw_y = 34U;
        effect_actor.resource.value_04 = 0xCAFEBABEU;
        shared.shared_motion_word = 9U;
        DispatchPort effect_port;
        const auto effect =
            openswd3::battle::advance_legacy_battle_special_four_hundred(
                &effect_actor,
                &progress,
                &shared,
                effect_port,
                context,
                {
                    .actor_token = 0x12340000U,
                    .target_token = 0x56780000U,
                }
            );
        test.expect_true(
            effect.return_eax == 0U && effect.target_event_calls == 1U &&
                effect.effect_update_calls == 1U &&
                effect.frame_lookup_calls == 1U && effect.render_calls == 1U &&
                effect_port.count(0x00474FC0U) == 0U &&
                effect_port.count(0x00477830U) == 0U &&
                effect_port.count(0x00478780U) == 1U &&
                effect_port.count(0x00481010U) == 1U &&
                (effect_actor.action_runtime_gate & 0x8000U) != 0U &&
                effect_actor.special_action_record.field_5a == 0U &&
                shared.shared_motion_word == 0U &&
                effect_port.special_four_hundred_effect_records.size() == 1U &&
                effect_port.special_four_hundred_effect_records[0U] ==
                    &effect_actor.effect_action_record &&
                effect_port.calls.back().arguments[5U] == 0xCAFEBABEU,
            "special four hundred consumes the primary event and draws the pending effect record with its resource"
        );

        auto completed_actor = prepare_actor();
        completed_actor.action_runtime_gate = 0x8000U;
        completed_actor.special_action_record.field_8c = 1U;
        completed_actor.effect_action_record.field_8c = 1U;
        completed_actor.special_target_action_record.field_94 = 1U;
        completed_actor.turn_action_record.field_94 = 2U;
        completed_actor.effect_secondary_action_record.field_94 = 3U;
        completed_actor.special_four_hundred_workspace =
            std::make_unique<std::array<u8, 0x4C0>>();
        completed_actor.special_four_hundred_workspace->fill(0xAAU);
        completed_actor.target_indices.fill(0U);
        completed_actor.motion_word = 9U;
        completed_actor.special_four_hundred_tail_word = 7U;
        progress.action_complete = 1U;
        shared.completion_counter = 0xFFU;
        shared.profile_mode_active = 1U;
        DispatchPort completed_port;
        const auto completed =
            openswd3::battle::advance_legacy_battle_special_four_hundred(
                &completed_actor,
                &progress,
                &shared,
                completed_port,
                context,
                {
                    .actor_token = 0x12340000U,
                    .target_token = 0x56780000U,
                }
            );
        test.expect_true(
            completed.return_eax == 1U &&
                completed.action_record_clears == 5U &&
                completed.workspace_bytes_cleared == 0x4C0U &&
                std::ranges::all_of(
                    *completed_actor.special_four_hundred_workspace,
                    [](const u8 value) { return value == 0U; }
                ) &&
                std::ranges::all_of(
                    completed_actor.target_indices,
                    [](const u32 value) { return value == 0xFFFFFFFFU; }
                ) &&
                progress.action_complete == 0U &&
                completed_actor.action_runtime_gate == 0U &&
                completed_actor.motion_word == 0U &&
                completed_actor.special_four_hundred_tail_word == 0U &&
                shared.completion_counter == 0U &&
                shared.profile_mode_active == 0U,
            "special four hundred clears five records and the contiguous workspace only after both completion gates close"
        );

        auto missing_actor = prepare_actor();
        missing_actor.special_action_record.field_5a = 0x0108U;
        missing_actor.special_target_action_record.field_4c = 2U;
        fixture.frame_provider.available = false;
        DispatchPort missing_port;
        const auto missing =
            openswd3::battle::advance_legacy_battle_special_four_hundred(
                &missing_actor,
                &progress,
                &shared,
                missing_port,
                context,
                {
                    .actor_token = 0x12340000U,
                    .target_token = 0x56780000U,
                }
            );
        test.expect_true(
            missing.status == openswd3::battle::
                    LegacyBattleSpecialFourHundredStatus::frame_owner_typed_stop &&
                missing_actor.turn_completion_latch == 1U &&
                missing_actor.special_four_hundred_workspace != nullptr &&
                (*missing_actor.special_four_hundred_workspace)[0x92U] == 1U &&
                missing_actor.turn_frame_token == 0U,
            "special four hundred stops at the original first frame dereference after workspace initialization"
        );
    }

    {
        LegacyBattleActionDispatchState state;
        state.group_a_count = 1;
        state.group_b_count = 1;
        auto& actor = state.group_a_action_execution[0U];
        actor.profile_value = 0x123U;
        actor.special_profile_variant = 7U;
        actor.action_runtime_gate = 0x8000U;
        actor.special_action_record.field_8c = 1U;
        actor.effect_action_record.field_8c = 1U;
        Fixture fixture;
        DispatchPort port;
        port.action = 400U;
        auto context = fixture.context();
        const auto result = openswd3::battle::dispatch_legacy_battle_action(
            state, port, context, 0U, 0U
        );
        test.expect_true(
            result.status == LegacyBattleActionDispatchStatus::completed &&
                result.special_four_hundred_calls == 1U &&
                result.special_four_hundred.return_eax == 1U &&
                state.action_pending == 1U &&
                port.count(0x00473C10U) == 0U,
            "action four hundred production advances the typed special state machine without the opaque call"
        );
    }

    {
        const auto prepare_actor = [] {
            auto actor =
                std::make_unique<LegacyBattleGroupAActionExecutionState>();
            actor->profile_value = 0x123U;
            actor->special_profile_variant = 7U;
            actor->special_action_record.field_4a = 1U;
            actor->special_action_record.field_4c = 1U;
            actor->effect_action_record.field_4a = 1U;
            actor->effect_action_record.field_4c = 1U;
            actor->render_flags = 5U;
            actor->draw_x = 12U;
            actor->draw_y = 34U;
            actor->resource.value_04 = 0xCAFEBABEU;
            return actor;
        };
        static LegacyBattleGroupAActionExecutionSharedState shared;
        static openswd3::battle::LegacyBattleActorProgressState progress;
        static Fixture fixture;
        static auto context = fixture.context();

        static auto gated_actor_owner = prepare_actor();
        auto& gated_actor = *gated_actor_owner;
        gated_actor.start_gate = 1U;
        static DispatchPort gated_port;
        static const auto gated =
            openswd3::battle::advance_legacy_battle_action_four_effect(
                &gated_actor,
                &progress,
                &shared,
                gated_port,
                context,
                {
                    .actor_token = 0x12340000U,
                    .target_token = 0x56780000U,
                }
            );
        test.expect_true(
            gated.return_eax == 0U && gated.port_calls == 0U &&
                gated_actor.turn_completion_latch == 0U,
            "action four effect preserves both entry gates before the completion latch"
        );

        static auto transfer_actor_owner = prepare_actor();
        auto& transfer_actor = *transfer_actor_owner;
        transfer_actor.special_action_record.field_5a = 0x0202U;
        transfer_actor.special_action_record.field_24 = 0x1111U;
        transfer_actor.special_action_record.field_28 = 0x2222U;
        static DispatchPort transfer_port;
        transfer_port.push(0x00483B30U, {.eax = 1U});
        static const auto transfer =
            openswd3::battle::advance_legacy_battle_action_four_effect(
                &transfer_actor,
                &progress,
                &shared,
                transfer_port,
                context,
                {
                    .actor_token = 0x12340000U,
                    .target_token = 0x56780000U,
                }
            );
        test.expect_true(
            transfer.return_eax == 0U && transfer.turn_frame_calls == 1U &&
                transfer_actor.turn_action_record.action_id == 0x1111U &&
                transfer_actor.turn_action_record.base_variant == 0x2222U &&
                transfer_actor.special_action_record.field_24 == 0U &&
                transfer_actor.special_action_record.field_28 == 0U &&
                transfer_actor.special_action_record.field_5a == 0U &&
                transfer_actor.special_action_record.external_mode == 0U &&
                (transfer_actor.action_runtime_gate & 0x4000U) == 0U,
            "action four effect transfers the optional turn record then clears bit fourteen only on callee completion"
        );

        static auto color_actor_owner = prepare_actor();
        auto& color_actor = *color_actor_owner;
        color_actor.special_action_record.field_5a = 0x0408U;
        color_actor.special_action_record.field_7a = 0xFFFFU;
        color_actor.special_action_record.field_7c = 2U;
        color_actor.special_action_record.field_7e = 3U;
        color_actor.special_action_record.field_80 = 4U;
        color_actor.special_action_record.field_82 = 5U;
        color_actor.special_action_record.field_84 = 6U;
        color_actor.special_action_record.field_86 = 7U;
        static DispatchPort color_port;
        static const auto colored =
            openswd3::battle::advance_legacy_battle_action_four_effect(
                &color_actor,
                &progress,
                &shared,
                color_port,
                context,
                {
                    .actor_token = 0x12340000U,
                    .target_token = 0x56780000U,
                }
            );
        const auto& color_state = color_port.battle_color_accumulation_state();
        test.expect_true(
            colored.return_eax == 0U &&
                colored.color_initialization_calls == 1U &&
                colored.frame_refresh_calls == 1U &&
                colored.effect_update_calls == 1U &&
                colored.frame_lookup_calls == 1U &&
                colored.render_calls == 1U &&
                color_port.battle_color_initialization_gate() == 1U &&
                color_actor.special_action_record.field_5a == 0U &&
                (color_actor.action_runtime_gate & 0x8000U) != 0U &&
                color_state.current_red == -1.0F &&
                color_state.target_blue == 6.0F && color_state.countdown == 7,
            "action four effect initializes signed color channels before starting the effect record"
        );

        static auto direct_actor_owner = prepare_actor();
        auto& direct_actor = *direct_actor_owner;
        direct_actor.action_runtime_gate = 0x8000U;
        direct_actor.special_effect_direct_mode = 1U;
        direct_actor.special_action_record.field_8c = 1U;
        direct_actor.effect_curve_value_a = 1U;
        direct_actor.effect_curve_value_b = 1U;
        direct_actor.position_x = 100U;
        direct_actor.position_y = 50U;
        direct_actor.turn_target_x_offset = 4U;
        direct_actor.source_x_offset = 3U;
        direct_actor.source_y = 8U;
        direct_actor.auxiliary_word = 2U;
        direct_actor.primary_action_record.draw_offset_y = 6U;
        shared.shared_motion_word = 9U;
        static DispatchPort direct_port;
        direct_port.push(0x0047F940U, {.eax = 1U});
        static const auto direct =
            openswd3::battle::advance_legacy_battle_action_four_effect(
                &direct_actor,
                &progress,
                &shared,
                direct_port,
                context,
                {
                    .actor_token = 0x12340000U,
                    .target_token = 0x56780000U,
                }
            );
        test.expect_true(
            direct.return_eax == 0U && direct.effect_update_calls == 1U &&
                direct.target_event_calls == 1U &&
                direct.frame_lookup_calls == 1U && direct.render_calls == 1U &&
                direct_port.count(0x00474FC0U) == 0U &&
                direct_port.count(0x00477830U) == 0U &&
                direct_port.count(0x00478780U) == 1U &&
                direct_port.count(0x00481010U) == 1U &&
                direct_actor.motion_word == 0xFFF8U &&
                direct_actor.primary_action_record.field_8c == 1U &&
                direct_actor.effect_action_record.field_8c == 1U &&
                shared.shared_motion_word == 1U &&
                direct_port.action_four_direct_effect_records.size() == 1U &&
                has_call_argument(direct_port, 0x0047F940U, 4U, 99U) &&
                has_call_argument(direct_port, 0x0047F940U, 5U, 46U),
            "action four direct effect preserves signed coordinates, consumes bit zero, and advances the minus-eight fade"
        );

        static auto suppressed_actor_owner = prepare_actor();
        auto& suppressed_actor = *suppressed_actor_owner;
        suppressed_actor.action_runtime_gate = 0xC000U;
        static DispatchPort suppressed_port;
        suppressed_port.push(0x00483B30U, {.eax = 0U});
        static const auto suppressed =
            openswd3::battle::advance_legacy_battle_action_four_effect(
                &suppressed_actor,
                &progress,
                &shared,
                suppressed_port,
                context,
                {
                    .actor_token = 0x12340000U,
                    .target_token = 0x56780000U,
                }
            );
        test.expect_true(
            suppressed.return_eax == 0U && suppressed.turn_frame_calls == 1U &&
                suppressed.effect_update_calls == 1U &&
                suppressed.frame_lookup_calls == 1U &&
                suppressed.render_calls == 0U &&
                suppressed_actor.action_runtime_gate == 0xC000U,
            "action four effect preserves the pending turn gate and suppresses fallback drawing while bit fourteen remains set"
        );

        static auto completed_actor_owner = prepare_actor();
        auto& completed_actor = *completed_actor_owner;
        completed_actor.action_runtime_gate = 0x8000U;
        completed_actor.special_action_record.field_8c = 1U;
        completed_actor.effect_action_record.field_8c = 1U;
        completed_actor.special_target_action_record.field_94 = 1U;
        completed_actor.turn_action_record.field_94 = 2U;
        completed_actor.special_four_hundred_workspace =
            std::make_unique<std::array<u8, 0x4C0>>();
        completed_actor.special_four_hundred_workspace->fill(0xAAU);
        completed_actor.target_indices.fill(0U);
        progress.action_complete = 1U;
        shared.completion_counter = 0xFFU;
        shared.profile_mode_active = 1U;
        static DispatchPort completed_port;
        static const auto completed =
            openswd3::battle::advance_legacy_battle_action_four_effect(
                &completed_actor,
                &progress,
                &shared,
                completed_port,
                context,
                {
                    .actor_token = 0x12340000U,
                    .target_token = 0x56780000U,
                }
            );
        test.expect_true(
            completed.return_eax == 1U &&
                completed.action_record_clears == 4U &&
                completed.workspace_bytes_cleared == 0x4C0U &&
                std::ranges::all_of(
                    *completed_actor.special_four_hundred_workspace,
                    [](const u8 value) { return value == 0U; }
                ) &&
                std::ranges::all_of(
                    completed_actor.target_indices,
                    [](const u32 value) { return value == 0xFFFFFFFFU; }
                ) &&
                progress.action_complete == 0U &&
                completed_actor.action_runtime_gate == 0U &&
                shared.completion_counter == 0U &&
                shared.profile_mode_active == 0U,
            "action four effect clears four records and the shared workspace only after the special and effect records complete"
        );

        static auto missing_actor_owner = prepare_actor();
        auto& missing_actor = *missing_actor_owner;
        missing_actor.action_runtime_gate = 0x8000U;
        fixture.frame_provider.available = false;
        static DispatchPort missing_port;
        static const auto missing =
            openswd3::battle::advance_legacy_battle_action_four_effect(
                &missing_actor,
                &progress,
                &shared,
                missing_port,
                context,
                {
                    .actor_token = 0x12340000U,
                    .target_token = 0x56780000U,
                }
            );
        test.expect_true(
            missing.status == openswd3::battle::
                    LegacyBattleActionFourEffectStatus::frame_owner_typed_stop &&
                missing_actor.turn_completion_latch == 1U &&
                missing_actor.turn_frame_token == 0U,
            "action four effect stops at the original effect-frame dereference after preserving all prior calls"
        );
    }
}

void test_battle_action_dispatch_part_three(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleActionDispatchState;
    using openswd3::battle::LegacyBattleActionDispatchStatus;

    {
        static LegacyBattleActionDispatchState state;
        state.group_a_count = 1;
        state.group_b_count = 1;
        auto& actor = state.group_a_action_execution[0U];
        actor.profile_value = 0x123U;
        actor.special_profile_variant = 7U;
        actor.action_runtime_gate = 0x8000U;
        actor.special_action_record.field_8c = 1U;
        actor.effect_action_record.field_8c = 1U;
        static Fixture fixture;
        static DispatchPort port;
        port.action = 4U;
        static auto context = fixture.context();
        static const auto result = openswd3::battle::dispatch_legacy_battle_action(
            state, port, context, 0U, 0U
        );
        test.expect_true(
            result.status == LegacyBattleActionDispatchStatus::completed &&
                result.action_four_effect_calls == 1U &&
                result.action_four_effect.return_eax == 1U &&
                state.action_pending == 1U &&
                port.count(0x004745B0U) == 0U,
            "ordinary action four production advances the typed effect state machine without the opaque call"
        );
    }

    {
        static LegacyBattleActionDispatchState state;
        state.group_a_count = 1;
        state.group_b_count = 1;
        state.special_phase = 2U;
        auto& actor = state.group_a_action_execution[0U];
        actor.profile_value = 0x123U;
        actor.special_profile_variant = 7U;
        actor.action_runtime_gate = 0x8000U;
        actor.special_action_record.field_8c = 1U;
        actor.effect_action_record.field_8c = 1U;
        static Fixture fixture;
        static DispatchPort port;
        port.action = 0x194U;
        static auto context = fixture.context();
        static const auto result = openswd3::battle::dispatch_legacy_battle_action(
            state, port, context, 0U, 0U
        );
        test.expect_true(
            result.status == LegacyBattleActionDispatchStatus::completed &&
                result.action_four_effect_calls == 1U &&
                result.action_four_effect.return_eax == 1U &&
                state.action_pending == 1U &&
                port.count(0x004745B0U) == 0U,
            "special action four hundred four shares the typed action-four effect state machine without the opaque call"
        );
    }

    {
        LegacyBattleActionDispatchState state;
        state.group_b_count = 1;
        state.group_a_action_execution[0U].profile_value = 0x123U;
        state.group_a_action_execution[0U].primary_action_record.cached_action_id =
            0x123U;
        state.group_a_action_execution[0U].primary_action_record.cached_base_variant =
            0x30U;
        Fixture fixture;
        fixture.stream_provider.bytes = {
            0x46U,
            0x52U,
            0x66U,
            0x00U,
            0x44U,
            0x45U,
        };
        auto& coordinate_actor =
            (*fixture.startup.group_b_lifecycle)[0U].action_execution;
        coordinate_actor.position_x = 200U;
        coordinate_actor.position_y = 60U;
        DispatchPort port;
        port.action = 27U;
        auto context = fixture.context();
        const auto result = openswd3::battle::dispatch_legacy_battle_action(
            state, port, context, 0U, 0U
        );
        test.expect_true(
            result.status == LegacyBattleActionDispatchStatus::completed &&
                result.action_twenty_seven_calls == 1U &&
                result.action_twenty_seven.return_eax == 0U &&
                port.count(0x004728E0U) == 0U && port.count(0x004783B0U) == 0U,
            "action twenty seven production advances the typed frame without the opaque call"
        );
    }

    {
        LegacyBattleActionDispatchState state;
        state.group_b_count = 2;
        state.blocking_effect = 1U;
        state.group_a_action_execution[0U].profile_value = 0x123U;
        state.group_a_action_execution[0U].action_flags = 1U;
        state.group_a_action_execution[0U].copied_runtime_word = 1U;
        Fixture fixture;
        DispatchPort port;
        port.action = 24U;
        port.push(0x00481010U, {.eax = 100U});
        port.push(0x00481010U, {.eax = 200U});
        auto context = fixture.context();
        const auto result = openswd3::battle::dispatch_legacy_battle_action(
            state, port, context, 0U, 0U
        );
        test.expect_true(
            result.action_twenty_four_calls == 1U &&
                result.action_twenty_four.return_eax == 0x8001U &&
                port.count(0x004724D0U) == 0U &&
                result.group_b_iterations == 2U && state.message_aux == 1U &&
                openswd3::compat::u16(state.packed_action_state) == 0U &&
                port.count(0x00481010U) == 2U && port.count(0x00478780U) == 2U,
            "action twenty four scans every live target accumulates signed values and clears nonterminal low word"
        );
    }

    {
        LegacyBattleActionDispatchState state;
        Fixture fixture;
        auto& coordinate_actor =
            (*fixture.startup.group_b_lifecycle)[0U].action_execution;
        coordinate_actor.position_x = 321U;
        coordinate_actor.position_y = 123U;
        DispatchPort port;
        port.action = 31U;
        fixture.flags[0x4BU >> 3U] = static_cast<u8>(1U << (0x4BU & 7U));
        auto context = fixture.context();
        const auto result = openswd3::battle::dispatch_legacy_battle_action(
            state, port, context, 0U, 0U
        );
        test.expect_true(
            result.return_value == 1U && result.coordinate_query_calls == 1U &&
                result.action_record_clear_calls == 1U &&
                state.countdown.secondary_ticks == 150U &&
                state.message_gate == 0U &&
                state.message_coordinate_x == 321U &&
                state.message_coordinate_y == 123U &&
                state.current_actor_index == 0xFFFFU &&
                port.count(0x004783B0U) == 0U &&
                (fixture.flags[0x4BU >> 3U] &
                 static_cast<u8>(1U << (0x4BU & 7U))) == 0U,
            "action thirty one initializes five second countdown then escape clears bit before target access"
        );
    }

    {
        LegacyBattleActionDispatchState state;
        Fixture fixture;
        DispatchPort port;
        port.action = 31U;
        fixture.flags[0x4BU >> 3U] = static_cast<u8>(1U << (0x4BU & 7U));
        auto context = fixture.context();
        context.startup = nullptr;
        context.shared_action_dispatch = nullptr;
        const auto result = openswd3::battle::dispatch_legacy_battle_action(
            state, port, context, 0U, 1U
        );
        test.expect_true(
            result.status ==
                    LegacyBattleActionDispatchStatus::
                        actor_coordinate_typed_stop &&
                result.coordinate_query_calls == 1U &&
                result.coordinate_query.status ==
                    openswd3::battle::LegacyBattleActorCoordinateQueryStatus::
                        actor_gate_read_typed_stop &&
                result.coordinate_query.flags.parity &&
                !result.coordinate_query.flags.auxiliary_carry &&
                !result.coordinate_query.flags.zero &&
                state.message_gate == 0x80000000U &&
                state.countdown.secondary_ticks == 150U &&
                result.action_record_clear_calls == 0U &&
                (fixture.flags[0x4BU >> 3U] &
                 static_cast<u8>(1U << (0x4BU & 7U))) != 0U &&
                port.count(0x004783B0U) == 0U,
            "action thirty one preserves SUB flags and suppresses clear and escape suffixes when the actor owner is missing"
        );
    }

    {
        static DispatchPort actor_stop_port;
        static const auto actor_stop =
            openswd3::battle::apply_legacy_battle_target_effect(
                nullptr,
                nullptr,
                actor_stop_port,
                {
                    .actor_token = 0x12340000U,
                    .target_token = 0x56780000U,
                    .mode = 1U,
                    .entry_eax = 0x11111111U,
                    .entry_ecx = 0x22222222U,
                    .entry_edx = 0x33333333U,
                }
            );
        test.expect_true(
            actor_stop.status == openswd3::battle::
                    LegacyBattleTargetEffectStatus::actor_state_typed_stop &&
                actor_stop.return_eax == 0x11111111U &&
                actor_stop.return_ecx == 0x22222222U &&
                actor_stop.return_edx == 0x33333333U &&
                actor_stop.port_calls == 0U,
            "target effect stops at the first actor motion write without changing entry registers"
        );

        static auto shared_stop_actor_owner =
            std::make_unique<LegacyBattleGroupAActionExecutionState>();
        auto& shared_stop_actor = *shared_stop_actor_owner;
        shared_stop_actor.motion_word = 9U;
        static DispatchPort shared_stop_port;
        static const auto shared_stop =
            openswd3::battle::apply_legacy_battle_target_effect(
                &shared_stop_actor,
                nullptr,
                shared_stop_port,
                {
                    .actor_token = 0x12340000U,
                    .target_token = 0x56780000U,
                    .entry_eax = 0x11111111U,
                    .entry_ecx = 0x22222222U,
                    .entry_edx = 0x33333333U,
                }
            );
        test.expect_true(
            shared_stop.status == openswd3::battle::
                    LegacyBattleTargetEffectStatus::shared_state_typed_stop &&
                shared_stop_actor.motion_word == 0U &&
                shared_stop.return_eax == 0x11111111U &&
                shared_stop.return_ecx == 0x22222222U &&
                shared_stop.return_edx == 0x33333333U &&
                shared_stop.port_calls == 0U,
            "target effect stops at the first shared motion write after clearing actor motion"
        );

        static auto curve_stop_actor_owner =
            std::make_unique<LegacyBattleGroupAActionExecutionState>();
        auto& curve_stop_actor = *curve_stop_actor_owner;
        curve_stop_actor.effect_curve_value_a = 3U;
        curve_stop_actor.effect_curve_value_b = 2U;
        curve_stop_actor.effect_curve_index = 9U;
        static LegacyBattleGroupAActionExecutionSharedState curve_stop_shared;
        static DispatchPort curve_stop_port;
        auto& curve_stop_state =
            curve_stop_port.legacy_battle_fixed_object_state();
        curve_stop_state.object_words[1U][0U] = 0x7A000000U;
        curve_stop_state.object_words[1U][1U] = 8U;
        curve_stop_state.fixed_count_nodes.push_back({
            .legacy_token = 0x7A000000U,
            .words = {0U, 9U, 0U, 0U, 0U},
            .accessible_bytes = 7U,
        });
        static const auto curve_stop =
            openswd3::battle::apply_legacy_battle_target_effect(
                &curve_stop_actor,
                &curve_stop_shared,
                curve_stop_port,
                {
                    .actor_token = 0x12340000U,
                    .target_token = 0x56780000U,
                    .entry_eax = 0xAAAA0000U,
                    .entry_ecx = 0xBBBB0000U,
                    .entry_edx = 0xCCCC0000U,
                }
            );
        test.expect_true(
            curve_stop.status ==
                    openswd3::battle::LegacyBattleTargetEffectStatus::
                        fixed_curve_typed_stop &&
                curve_stop.fixed_curve.status ==
                    openswd3::battle::LegacyBattleFixedCountStatus::
                        record_access_typed_stop &&
                curve_stop.fixed_curve.stopped_token == 0x7A000000U &&
                curve_stop.fixed_curve.stopped_offset == 6U &&
                curve_stop.return_eax == 0x7A000000U &&
                curve_stop.return_ecx == 0xBBBB0002U &&
                curve_stop.return_edx == 0xCCCC0009U &&
                curve_stop_actor.motion_word == 0U &&
                curve_stop_shared.shared_motion_word == 0U &&
                curve_stop_actor.effect_application_latch == 0U &&
                curve_stop_port.count(0x00477830U) == 0U,
            "target effect stops at the fixed curve count access after preserving the caller prefix"
        );

        static auto skip_actor_owner =
            std::make_unique<LegacyBattleGroupAActionExecutionState>();
        auto& skip_actor = *skip_actor_owner;
        skip_actor.effect_curve_value_a = 0xFFFFU;
        skip_actor.effect_curve_value_b = 0xFFFFU;
        skip_actor.effect_curve_index = 0x3333U;
        skip_actor.effect_direction_flags = 0x80U;
        static LegacyBattleGroupAActionExecutionSharedState skip_shared;
        static DispatchPort skip_port;
        skip_port.legacy_battle_fixed_object_state().object_words[1U][1U] =
            (0x8000U << 16U) | 0x3333U;
        skip_port.push(
            0x0047CD60U,
            {.eax = 0xAAAAAAAAU, .ecx = 0xBBBBBBBBU, .edx = 0xCCCCCCCCU}
        );
        static const auto skipped =
            openswd3::battle::apply_legacy_battle_target_effect(
                &skip_actor,
                &skip_shared,
                skip_port,
                {
                    .actor_token = 0x12340000U,
                    .target_token = 0x56780000U,
                    .mode = 1U,
                    .entry_eax = 0xAAAA0000U,
                    .entry_ecx = 0xBBBB0000U,
                    .entry_edx = 0xCCCC0000U,
                }
            );
        test.expect_true(
            skipped.return_eax == 0xAAAAAAAAU &&
                skipped.return_ecx == 0xBBBBBBBBU &&
                skipped.return_edx == 0xCCCCCCCCU &&
                skipped.curve_query_calls == 1U &&
                skipped.skip_gate_calls == 1U &&
                skipped.target_refresh_calls == 0U &&
                skip_shared.shared_motion_word == 0x8001U &&
                skip_actor.motion_word == 0U &&
                skip_actor.effect_application_latch == 1U &&
                skipped.fixed_curve.path ==
                    openswd3::battle::LegacyBattleFixedCountPath::
                        existing_root &&
                skipped.fixed_curve.count == 0x8001U &&
                skipped.fixed_curve.scale == 50U &&
                skip_port.count(0x00477830U) == 0U &&
                has_call_argument(skip_port, 0x0047CD60U, 0U, 8U),
            "target effect directly advances the fixed curve and honors the mode-one direction skip gate"
        );

        static auto full_actor_owner =
            std::make_unique<LegacyBattleGroupAActionExecutionState>();
        auto& full_actor = *full_actor_owner;
        full_actor.effect_curve_value_a = 0xFFFCU;
        full_actor.effect_curve_value_b = 1U;
        static LegacyBattleGroupAActionExecutionSharedState full_shared;
        static DispatchPort full_port;
        full_port.battle_pair_primary_value() = 10U;
        full_port.push(0x00478780U, {.eax = 0x10U, .ecx = 0x20U, .edx = 0x30U});
        full_port.push(0x00481010U, {.eax = 10U, .ecx = 0x40U, .edx = 0x50U});
        full_port.push(0x0047D640U, {.eax = 0x60U, .ecx = 0x70U, .edx = 0x80U});
        full_port.push(0x0047CEC0U, {.eax = 0x90U, .ecx = 0xA0U, .edx = 0xB0U});
        static const auto full =
            openswd3::battle::apply_legacy_battle_target_effect(
                &full_actor,
                &full_shared,
                full_port,
                {
                    .actor_token = 0x12340000U,
                    .target_token = 0x56780000U,
                    .mode = 0U,
                }
            );
        test.expect_true(
            full.return_eax == 0x90U && full.return_ecx == 0xA0U &&
                full.return_edx == 0xB0U && full.effect_value == 6 &&
                full_shared.last_effect_value == 6 &&
                full_port.battle_pair_primary_value() == 16U &&
                full.fixed_curve.return_eax == 0xFFFCU &&
                full.fixed_curve.scale == 100U &&
                full_port.count(0x00477830U) == 0U &&
                full.effect_apply_calls == 1U &&
                full.effect_property_calls == 1U &&
                has_call_argument(full_port, 0x0047D640U, 0U, 6U) &&
                has_call_argument(full_port, 0x0047CEC0U, 0U, 1U),
            "target effect sign-extends both words, accumulates the result, and publishes it to the target"
        );

        static auto clamp_actor_owner =
            std::make_unique<LegacyBattleGroupAActionExecutionState>();
        auto& clamp_actor = *clamp_actor_owner;
        static LegacyBattleGroupAActionExecutionSharedState clamp_shared;
        static DispatchPort clamp_port;
        clamp_port.push(0x00481010U, {.eax = 0x2710U});
        static const auto clamped =
            openswd3::battle::apply_legacy_battle_target_effect(
                &clamp_actor,
                &clamp_shared,
                clamp_port,
                {
                    .actor_token = 0x12340000U,
                    .target_token = 0x56780000U,
                }
            );
        test.expect_true(
            clamped.effect_value == 0x270F &&
                clamp_shared.last_effect_value == 0x270F &&
                clamp_port.battle_pair_primary_value() == 0x270FU &&
                has_call_argument(clamp_port, 0x0047D640U, 0U, 0x270FU),
            "target effect clamps signed values at nine thousand nine hundred ninety nine before accumulation"
        );

        static auto sentinel_actor_owner =
            std::make_unique<LegacyBattleGroupAActionExecutionState>();
        auto& sentinel_actor = *sentinel_actor_owner;
        static LegacyBattleGroupAActionExecutionSharedState sentinel_shared;
        static DispatchPort sentinel_port;
        sentinel_port.battle_pair_primary_value() = 10U;
        sentinel_port.push(0x00481010U, {.eax = 0xFFFFU});
        static const auto sentinel =
            openswd3::battle::apply_legacy_battle_target_effect(
                &sentinel_actor,
                &sentinel_shared,
                sentinel_port,
                {
                    .actor_token = 0x12340000U,
                    .target_token = 0x56780000U,
                }
            );
        test.expect_true(
            sentinel.return_eax == 0xFFFFFFFFU && sentinel.return_edx == 9U &&
                sentinel.effect_value == -1 &&
                sentinel_shared.last_effect_value == -1 &&
                sentinel_port.battle_pair_primary_value() == 9U &&
                sentinel.effect_apply_calls == 0U &&
                sentinel.effect_property_calls == 0U &&
                sentinel_port.count(0x00474FC0U) == 0U,
            "target effect accumulates negative one before skipping target publication"
        );
    }

    {
        const auto prepare_actor = [] {
            auto actor =
                std::make_unique<LegacyBattleGroupAActionExecutionState>();
            actor->profile_value = 0x123U;
            actor->special_profile_variant = 7U;
            return actor;
        };
        static LegacyBattleGroupAActionExecutionSharedState shared;
        static Fixture coordinate_fixture;
        auto& coordinate_actor =
            (*coordinate_fixture.startup.group_b_lifecycle)[0U]
                .action_execution;
        coordinate_actor.position_x = 100U;
        coordinate_actor.position_y = 200U;
        const openswd3::battle::LegacyBattleActorCoordinateOwners
            coordinate_owners{
                .startup = &coordinate_fixture.startup,
            };

        static DispatchPort actor_stop_port;
        static const auto actor_stop =
            openswd3::battle::advance_legacy_battle_special_four_oh_nine(
                nullptr,
                nullptr,
                actor_stop_port,
                {
                    .actor_token = 0x12340000U,
                    .target_token = 0x56780000U,
                    .entry_eax = 0x11111111U,
                    .entry_ecx = 0x22222222U,
                    .entry_edx = 0x33333333U,
                }
            );
        test.expect_true(
            actor_stop.status == openswd3::battle::
                    LegacyBattleSpecialFourOhNineStatus::actor_state_typed_stop &&
                actor_stop.return_eax == 0U && actor_stop.return_ecx == 0U &&
                actor_stop.return_edx == 0U && actor_stop.port_calls == 0U,
            "special four-oh-nine stops at the first actor read after clearing the legacy registers"
        );

        static auto stage_zero_actor_owner = prepare_actor();
        auto& stage_zero_actor = *stage_zero_actor_owner;
        shared.action_completion_flags = 0U;
        static DispatchPort stage_zero_port;
        static const auto stage_zero =
            openswd3::battle::advance_legacy_battle_special_four_oh_nine(
                &stage_zero_actor,
                &shared,
                stage_zero_port,
                {
                    .actor_token = 0x12340000U,
                    .target_token = 0x56780000U,
                }
            );
        test.expect_true(
            stage_zero.return_eax == 0U && stage_zero.return_ecx == 0U &&
                stage_zero.return_edx == 0U &&
                stage_zero.special_update_calls == 1U &&
                stage_zero_actor.turn_completion_latch == 1U &&
                stage_zero_actor.special_action_record.action_id == 0x6FFU &&
                stage_zero_actor.special_action_record.base_variant == 7U &&
                stage_zero_actor.action_runtime_gate == 0U,
            "special four-oh-nine gate zero updates the main record before reading shared completion flags"
        );

        static auto stage_one_actor_owner = prepare_actor();
        auto& stage_one_actor = *stage_one_actor_owner;
        stage_one_actor.action_runtime_gate = 1U;
        shared.action_completion_flags = 0U;
        static DispatchPort stage_one_port;
        static const auto stage_one =
            openswd3::battle::advance_legacy_battle_special_four_oh_nine(
                &stage_one_actor,
                &shared,
                stage_one_port,
                {
                    .actor_token = 0x12340000U,
                    .target_token = 0x00525508U,
                },
                coordinate_owners
            );
        test.expect_true(
            stage_one.return_eax == 0U && stage_one.return_ecx == 1U &&
                stage_one.return_edx == 0U &&
                stage_one.coordinate_query_calls == 1U &&
                stage_one.coordinate_update_calls == 1U &&
                stage_one_actor.special_action_record.base_variant == 8U &&
                stage_one_port.special_four_oh_nine_coordinate_records.size() ==
                    1U &&
                stage_one_port.special_four_oh_nine_coordinate_records[0U] ==
                    &stage_one_actor.special_action_record &&
                stage_one_port.count(0x004783B0U) == 0U &&
                has_call_argument(stage_one_port, 0x00484230U, 0U, 100U) &&
                has_call_argument(stage_one_port, 0x00484230U, 1U, 200U),
            "special four-oh-nine gate one increments the variant and passes target coordinates with the main record"
        );

        static auto coordinate_stop_actor_owner = prepare_actor();
        auto& coordinate_stop_actor = *coordinate_stop_actor_owner;
        coordinate_stop_actor.action_runtime_gate = 1U;
        shared.action_completion_flags = 0U;
        static DispatchPort coordinate_stop_port;
        static const auto coordinate_stop =
            openswd3::battle::advance_legacy_battle_special_four_oh_nine(
                &coordinate_stop_actor,
                &shared,
                coordinate_stop_port,
                {
                    .actor_token = 0x12340000U,
                    .target_token = 0x00525508U,
                    .coordinate_output_x_token = 0x11112222U,
                    .coordinate_output_y_token = 0x33334444U,
                }
            );
        test.expect_true(
            coordinate_stop.status ==
                    openswd3::battle::LegacyBattleSpecialFourOhNineStatus::
                        actor_coordinate_typed_stop &&
                coordinate_stop.coordinate_query.status ==
                    openswd3::battle::LegacyBattleActorCoordinateQueryStatus::
                        actor_gate_read_typed_stop &&
                !coordinate_stop.coordinate_query.flags.carry &&
                !coordinate_stop.coordinate_query.flags.parity &&
                !coordinate_stop.coordinate_query.flags.auxiliary_carry &&
                coordinate_stop.return_eax == 0x33334444U &&
                coordinate_stop.return_ecx == 0x00525508U &&
                coordinate_stop.return_edx == 8U &&
                coordinate_stop_actor.special_action_record.base_variant ==
                    8U &&
                coordinate_stop.coordinate_update_calls == 0U &&
                coordinate_stop.stage_two_calls == 0U &&
                coordinate_stop.action_record_clears == 0U &&
                coordinate_stop_port.count(0x004783B0U) == 0U,
            "special four-oh-nine preserves INC flags and suppresses the coordinate-update suffix without an owner"
        );

        static auto stage_two_actor_owner = prepare_actor();
        auto& stage_two_actor = *stage_two_actor_owner;
        stage_two_actor.action_runtime_gate = 2U;
        shared.action_completion_flags = 0U;
        static DispatchPort stage_two_port;
        stage_two_port.push(0x00482840U, {.eax = 1U});
        static const auto stage_two =
            openswd3::battle::advance_legacy_battle_special_four_oh_nine(
                &stage_two_actor,
                &shared,
                stage_two_port,
                {
                    .actor_token = 0x12340000U,
                    .target_token = 0x56780000U,
                }
            );
        test.expect_true(
            stage_two.return_eax == 0U && stage_two.stage_two_calls == 1U &&
                stage_two_actor.action_runtime_gate == 3U &&
                (stage_two_actor.special_effect_direct_mode & 8U) != 0U &&
                stage_two_actor.special_action_record.base_variant == 9U &&
                has_call_argument(stage_two_port, 0x00482840U, 0U, 0x56780000U) &&
                has_call_argument(stage_two_port, 0x00482840U, 1U, 0x6FFU) &&
                has_call_argument(stage_two_port, 0x00482840U, 2U, 9U),
            "special four-oh-nine gate two publishes mode bit three and advances to gate three only on callee completion"
        );

        static auto event_actor_owner = prepare_actor();
        auto& event_actor = *event_actor_owner;
        event_actor.special_action_record.field_5a = 8U;
        shared.action_completion_flags = 0U;
        static DispatchPort event_port;
        static const auto event =
            openswd3::battle::advance_legacy_battle_special_four_oh_nine(
                &event_actor,
                &shared,
                event_port,
                {
                    .actor_token = 0x12340000U,
                    .target_token = 0x56780000U,
                }
            );
        test.expect_true(
            event.return_eax == 0U &&
                shared.action_completion_flags == 0x8000U &&
                event_actor.special_action_record.field_5a == 0U,
            "special four-oh-nine consumes main bit three and publishes shared bit fifteen once"
        );

        static auto completed_actor_owner = prepare_actor();
        auto& completed_actor = *completed_actor_owner;
        completed_actor.action_runtime_gate = 2U;
        completed_actor.special_action_record.field_8c = 1U;
        shared.action_completion_flags = 1U;
        static DispatchPort completed_port;
        completed_port.push(0x00482840U, {.eax = 1U});
        static const auto completed =
            openswd3::battle::advance_legacy_battle_special_four_oh_nine(
                &completed_actor,
                &shared,
                completed_port,
                {
                    .actor_token = 0x12340000U,
                    .target_token = 0x56780000U,
                }
            );
        test.expect_true(
            completed.return_eax == 1U && completed.return_ecx == 0U &&
                completed.action_record_clears == 2U &&
                completed_actor.action_runtime_gate == 0U &&
                completed_actor.special_action_record.action_id == 0U &&
                (completed_actor.special_effect_direct_mode & 8U) != 0U,
            "special four-oh-nine permits stage two, main completion, gate increment, and shared completion in one frame"
        );

        static auto shared_stop_actor_owner = prepare_actor();
        auto& shared_stop_actor = *shared_stop_actor_owner;
        shared_stop_actor.action_runtime_gate = 1U;
        static DispatchPort shared_stop_port;
        static const auto shared_stop =
            openswd3::battle::advance_legacy_battle_special_four_oh_nine(
                &shared_stop_actor,
                nullptr,
                shared_stop_port,
                {
                    .actor_token = 0x12340000U,
                    .target_token = 0x00525508U,
                },
                coordinate_owners
            );
        test.expect_true(
            shared_stop.status == openswd3::battle::
                    LegacyBattleSpecialFourOhNineStatus::shared_state_typed_stop &&
                shared_stop.return_eax == 2U &&
                shared_stop.return_ecx == 1U &&
                shared_stop.coordinate_update_calls == 1U &&
                shared_stop_actor.special_action_record.base_variant == 8U,
            "special four-oh-nine stops at the original shared flag read after preserving gate-one side effects"
        );
    }

    {
        const auto prepare_actor = [] {
            auto actor =
                std::make_unique<LegacyBattleGroupAActionExecutionState>();
            actor->profile_value = 0x123U;
            actor->special_profile_variant = 7U;
            actor->position_x = 10U;
            actor->position_y = 20U;
            actor->copied_runtime_word = 0x1234U;
            return actor;
        };
        static Fixture coordinate_fixture;
        auto& coordinate_actor =
            (*coordinate_fixture.startup.group_b_lifecycle)[0U]
                .action_execution;
        coordinate_actor.position_x = 100U;
        coordinate_actor.position_y = 200U;
        const openswd3::battle::LegacyBattleActorCoordinateOwners
            coordinate_owners{
                .startup = &coordinate_fixture.startup,
            };

        static auto transfer_actor_owner = prepare_actor();
        auto& transfer_actor = *transfer_actor_owner;
        transfer_actor.special_action_record.field_5a = 0x0202U;
        transfer_actor.special_action_record.field_24 = 0x1111U;
        transfer_actor.special_action_record.field_28 = 0x2222U;
        static DispatchPort transfer_port;
        transfer_port.push(0x00483B30U, {.eax = 1U});
        static const auto transfer =
            openswd3::battle::advance_legacy_battle_action_four_oh_two(
                &transfer_actor,
                transfer_port,
                {
                    .actor_token = 0x12340000U,
                    .target_token = 0x56780000U,
                }
            );
        test.expect_true(
            transfer.return_eax == 0U && transfer.turn_frame_calls == 1U &&
                transfer_actor.turn_action_record.action_id == 0x1111U &&
                transfer_actor.turn_action_record.base_variant == 0x2222U &&
                transfer_actor.special_action_record.field_24 == 0U &&
                transfer_actor.special_action_record.field_28 == 0U &&
                transfer_actor.special_action_record.field_5a == 0U &&
                transfer_actor.special_action_record.external_mode == 0U &&
                (transfer_actor.action_runtime_gate & 0x4000U) == 0U,
            "action four-oh-two transfers the optional turn record and clears bit fourteen only on completion"
        );

        static auto event_actor_owner = prepare_actor();
        auto& event_actor = *event_actor_owner;
        event_actor.special_action_record.field_5a = 1U;
        static DispatchPort event_port;
        event_port.push(0x0047FC40U, {.eax = 0U});
        static const auto event =
            openswd3::battle::advance_legacy_battle_action_four_oh_two(
                &event_actor,
                event_port,
                {
                    .actor_token = 0x12340000U,
                    .target_token = 0x00525508U,
                },
                coordinate_owners
            );
        test.expect_true(
            event.return_eax == 0U && event.coordinate_query_calls == 2U &&
                event.coordinate_update_calls == 2U &&
                event.particle_spawn_calls == 1U &&
                event.particle_commit_calls == 1U &&
                event.sample_play_calls == 1U && event.completion_calls == 1U &&
                event_port.count(0x004783B0U) == 0U &&
                event_actor.special_particle_sequence_index == 0U &&
                event_actor.special_particle_sequence_count == 1U &&
                event_actor.special_particle_spawn_count == 1U &&
                event_actor.turn_threshold == 1U &&
                event_actor.action_runtime_gate == 0x8000U &&
                event_port.action_four_oh_two_particles.size() == 1U &&
                event_port.action_four_oh_two_particles[0U].arguments[0U] ==
                    0x1234U &&
                event_port.action_four_oh_two_particles[0U].arguments[1U] ==
                    0U &&
                event_port.action_four_oh_two_particles[0U].arguments[2U] ==
                    10U &&
                event_port.action_four_oh_two_particles[0U].arguments[3U] ==
                    0xFFFFFFB0U &&
                event_port.action_four_oh_two_particles[0U].arguments[4U] ==
                    100U &&
                event_port.action_four_oh_two_particles[0U].arguments[5U] ==
                    160U &&
                event_port.action_four_oh_two_particles[0U].arguments[6U] ==
                    1U &&
                event_port.action_four_oh_two_particles[0U].arguments[7U] ==
                    0x34U &&
                event_port.action_four_oh_two_particles[0U].arguments[8U] == 0U,
            "action four-oh-two consumes the event, advances the sequence, and emits the exact nine-argument first particle"
        );

        static auto partial_actor_owner = prepare_actor();
        auto& partial_actor = *partial_actor_owner;
        partial_actor.special_action_record.field_5a = 1U;
        coordinate_actor.position_x = 100U;
        coordinate_actor.position_y = 200U;
        coordinate_actor.position_y_read_accessible = true;
        static DispatchPort partial_port;
        partial_port.coordinate_x_to_change = &coordinate_actor.position_x;
        partial_port.coordinate_x_after_first_update = 300U;
        partial_port.coordinate_y_accessible_to_disable =
            &coordinate_actor.position_y_read_accessible;
        static const auto partial =
            openswd3::battle::advance_legacy_battle_action_four_oh_two(
                &partial_actor,
                partial_port,
                {
                    .actor_token = 0x12340000U,
                    .target_token = 0x00525508U,
                    .coordinate_output_x_token = 0x11112222U,
                    .coordinate_output_y_token = 0x33334444U,
                },
                coordinate_owners
            );
        test.expect_true(
            partial.status ==
                    openswd3::battle::LegacyBattleActionFourOhTwoStatus::
                        actor_coordinate_typed_stop &&
                partial.coordinate_query_calls == 2U &&
                partial.coordinate_queries[0U].status ==
                    openswd3::battle::LegacyBattleActorCoordinateQueryStatus::
                        completed &&
                partial.coordinate_queries[1U].status ==
                    openswd3::battle::LegacyBattleActorCoordinateQueryStatus::
                        primary_y_read_typed_stop &&
                partial.coordinate_queries[1U].output_writes == 1U &&
                partial.coordinate_output_x == 300U &&
                partial.coordinate_output_y == 200U &&
                partial.coordinate_update_calls == 1U &&
                partial.particle_spawn_calls == 0U &&
                partial.completion_calls == 0U &&
                partial.return_eax == 0x11112222U &&
                partial.return_ecx == 0x00525508U &&
                partial.return_edx == 0x33334444U,
            "action four-oh-two preserves the first shared slots when the second query writes X then faults on Y"
        );
        coordinate_actor.position_x = 100U;
        coordinate_actor.position_y_read_accessible = true;

        static auto suppressed_actor_owner = prepare_actor();
        auto& suppressed_actor = *suppressed_actor_owner;
        suppressed_actor.special_action_record.field_5a = 8U;
        suppressed_actor.special_particle_coordinate_suppression = 2U;
        static DispatchPort suppressed_port;
        suppressed_port.push(0x0047FC40U, {.eax = 0U});
        static const auto suppressed =
            openswd3::battle::advance_legacy_battle_action_four_oh_two(
                &suppressed_actor,
                suppressed_port,
                {
                    .actor_token = 0x12340000U,
                    .target_token = 0x00525508U,
                },
                coordinate_owners
            );
        test.expect_true(
            suppressed.return_eax == 0U &&
                suppressed_port.action_four_oh_two_coordinates.size() == 2U &&
                suppressed_port.action_four_oh_two_coordinates[0U][0U] == 0U &&
                suppressed_port.action_four_oh_two_coordinates[0U][1U] == 0U &&
                suppressed_port.action_four_oh_two_particles[0U]
                        .arguments[4U] == 0U &&
                suppressed_port.action_four_oh_two_particles[0U]
                        .arguments[5U] == 0xFFFFFFD8U &&
                suppressed_port.count(0x004783B0U) == 0U,
            "action four-oh-two zeroes both coordinates before each transform when suppression bit one is set"
        );

        static auto completed_actor_owner = prepare_actor();
        auto& completed_actor = *completed_actor_owner;
        completed_actor.action_runtime_gate = 0x8000U;
        completed_actor.special_action_record.field_8c = 1U;
        completed_actor.effect_action_record.field_94 = 0xAABBCCDDU;
        completed_actor.special_particle_spawn_count = 8U;
        completed_actor.special_particle_sequence_index = 3U;
        completed_actor.special_particle_sequence_count = 4U;
        static DispatchPort completed_port;
        completed_port.push(0x0047FC40U, {.eax = 1U});
        static const auto completed =
            openswd3::battle::advance_legacy_battle_action_four_oh_two(
                &completed_actor,
                completed_port,
                {
                    .actor_token = 0x12340000U,
                    .target_token = 0x56780000U,
                }
            );
        test.expect_true(
            completed.return_eax == 1U &&
                completed.action_record_clears == 2U &&
                completed_actor.action_runtime_gate == 0U &&
                completed_actor.turn_threshold == 0U &&
                completed_actor.special_particle_sequence_index == 0U &&
                completed_actor.special_particle_sequence_count == 0U &&
                completed_actor.effect_action_record.field_94 == 0U &&
                completed_actor.special_action_record.field_8c == 0U,
            "action four-oh-two clears both records and sequence counters only after the completion callee and main record finish"
        );
    }

    {
        static LegacyBattleActionDispatchState state;
        state.group_a_count = 1;
        state.group_b_count = 1;
        auto& actor = state.group_a_action_execution[0U];
        actor.profile_value = 0x123U;
        actor.special_profile_variant = 7U;
        actor.action_runtime_gate = 0x8000U;
        actor.special_action_record.field_8c = 1U;
        actor.special_particle_spawn_count = 8U;
        static Fixture fixture;
        static DispatchPort port;
        port.action = 0x192U;
        port.push(0x0047FC40U, {.eax = 1U});
        static auto context = fixture.context();
        static const auto result =
            openswd3::battle::dispatch_legacy_battle_action(
                state, port, context, 0U, 0U
            );
        test.expect_true(
            result.status == LegacyBattleActionDispatchStatus::completed &&
                result.action_four_oh_two_calls == 1U &&
                result.action_four_oh_two.return_eax == 1U &&
                state.frame_effect.red_factor == -12 &&
                state.frame_effect.green_factor == -12 &&
                state.frame_effect.blue_factor == -12 &&
                state.frame_effect.primary_suppression == 1U &&
                state.frame_effect.alternate_surface_mode == 1U &&
                state.action_pending == 1U &&
                port.count(0x00474BA0U) == 0U,
            "special action four hundred two production preserves frame-effect setup and uses the typed particle state machine"
        );
    }

    {
        static RandomPort random;
        random.value = 10U;
        auto checked =
            openswd3::battle::check_legacy_battle_target_property_chance(
                random, {.value = 0U}
            );
        test.expect_true(
            checked.return_eax == 1U && checked.random_calls == 1U &&
                checked.sampled_value == 10U && checked.scaled_value == 0 &&
                checked.quotient == 0 && checked.threshold == 10U &&
                checked.return_ecx == 0U && checked.return_edx == 10U,
            "target property chance accepts equality at the minimum ten-percent threshold"
        );

        random.value = 11U;
        checked = openswd3::battle::check_legacy_battle_target_property_chance(
            random, {.value = 0U}
        );
        test.expect_true(
            checked.return_eax == 0U && checked.threshold == 10U,
            "target property chance rejects the first sample above the inclusive threshold"
        );

        random.value = 80U;
        checked = openswd3::battle::check_legacy_battle_target_property_chance(
            random, {.value = 100U}
        );
        test.expect_true(
            checked.return_eax == 1U && checked.scaled_value == 7000 &&
                checked.quotient == 70 && checked.threshold == 80U,
            "target property chance scales one hundred to an inclusive eighty-percent threshold"
        );

        random.value = 10U;
        checked = openswd3::battle::check_legacy_battle_target_property_chance(
            random, {.value = 0xFFFFFFFFU}
        );
        test.expect_true(
            checked.return_eax == 1U && checked.scaled_value == -70 &&
                checked.quotient == 0 && checked.threshold == 10U &&
                checked.return_ecx == 0xFFFFFFBAU &&
                checked.return_edx == 10U,
            "target property chance preserves wrapped multiplication and signed truncation toward zero"
        );
    }

    {
        LegacyBattleActionDispatchState state;
        auto& ready = state.group_a_action_execution[0U];
        ready.profile_value = 1U;
        ready.primary_action_record.cached_action_id = 1U;
        ready.primary_action_record.cached_base_variant = 0x30U;
        ready.primary_action_record.field_5a = 9U;
        ready.action_runtime_gate = 2U;
        Fixture fixture;
        DispatchPort port;
        port.action = 33U;
        port.push(0x00480AD0U, {.eax = 0U});
        auto context = fixture.context();
        const auto result = openswd3::battle::dispatch_legacy_battle_action(
            state, port, context, 0U, 1U
        );
        test.expect_true(
            result.status ==
                    LegacyBattleActionDispatchStatus::
                        target_object_typed_stop &&
                state.current_actor_index == 0xFFFFU &&
                port.count(0x00482F10U) == 0U,
            "null resolved target stops at first flags dereference after current actor clear"
        );
    }

    {
        auto state = std::make_unique<LegacyBattleActionDispatchState>();
        auto& ready = state->group_a_action_execution[0U];
        ready.profile_value = 1U;
        ready.primary_action_record.cached_action_id = 1U;
        ready.primary_action_record.cached_base_variant = 0x30U;
        ready.primary_action_record.field_5a = 9U;
        ready.action_runtime_gate = 2U;
        auto fixture = std::make_unique<Fixture>();
        fixture->random.value = 10U;
        DispatchPort port;
        port.action = 33U;
        port.push(0x00482F10U, {.eax = 0U});
        auto context = fixture->context();
        const auto result = openswd3::battle::dispatch_legacy_battle_action(
            *state, port, context, 0U, 1U
        );
        test.expect_true(
            result.status == LegacyBattleActionDispatchStatus::completed &&
                result.return_value == 1U && result.target_ready_calls == 1U &&
                result.target_ready.return_eax == 1U &&
                result.target_property_chance_calls == 1U &&
                result.target_property_chance.sampled_value == 10U &&
                result.target_property_chance.threshold == 10U &&
                result.target_property_chance.return_eax == 1U &&
                state->selected_target_index == 1U &&
                state->frame_refresh_pending == 1U &&
                port.count(0x004751C0U) == 0U && port.count(0x00474B60U) == 0U,
            "action thirty-three directly advances target ready and the inclusive target-property chance without either opaque call"
        );
    }

    {
        LegacyBattleActionDispatchState state;
        state.group_a_count = 1;
        state.group_b_count = 1;
        state.group_a_to_actor[0] = 0U;
        Fixture fixture;
        fixture.attack_order_records[0].value_00 = 0U;
        DispatchPort port;
        port.action = 7U;
        port.push(0x00479850U, {.eax = 1U});
        auto context = fixture.context();

        const auto result = openswd3::battle::dispatch_legacy_battle_action(
            state, port, context, 0U, 0U
        );

        test.expect_true(
            result.status == LegacyBattleActionDispatchStatus::completed &&
                result.attack_order_remove_calls == 1U &&
                result.attack_order_remove.matched &&
                fixture.attack_order_records[0].value_00 == 0xFFFFFFFFU &&
                (state.packed_actor_counter & 0xFFU) == 1U &&
                port.count(0x0045EFB0U) == 0U && result.return_value == 1U,
            "action seven removes the opponent directly from the shared attack order before publishing completion"
        );
    }

    {
        LegacyBattleActionDispatchState state;
        state.group_a_count = 1;
        state.group_b_count = 1;
        state.group_a_to_actor[0] = 0U;
        Fixture fixture;
        fixture.attack_order_records[17].value_00 = 0U;
        DispatchPort port;
        port.action = 7U;
        port.push(0x00479850U, {.eax = 1U});
        auto context = fixture.context();
        context.attack_order_adjacent_record = nullptr;

        const auto result = openswd3::battle::dispatch_legacy_battle_action(
            state, port, context, 0U, 0U
        );

        test.expect_true(
            result.status ==
                    LegacyBattleActionDispatchStatus::
                        attack_order_remove_typed_stop &&
                result.attack_order_remove.status ==
                    openswd3::battle::LegacyBattleAttackOrderRemoveStatus::
                        adjacent_record_typed_stop &&
                (state.packed_actor_counter & 0xFFU) == 0U &&
                result.return_value == 0U,
            "attack-order one-past stop preserves the ready and delay prefix then blocks action completion"
        );
    }

    {
        DispatchPort port;
        static_cast<void>(port.invoke_group_a_summon_materialization({
            .call =
                openswd3::battle::LegacyBattleGroupASummonMaterializationCall::
                    report_missing_role,
            .window_token = 0x12340000U,
            .diagnostic_text_token = 0x004A7C68U,
            .diagnostic_source_token = 0x004A7C44U,
            .diagnostic_source_line = 0x123U,
        }));
        test.expect_true(
            port.count(0x00431150U) == 1U &&
                has_call_argument(port, 0x00431150U, 0U, 0x12340000U) &&
                has_call_argument(port, 0x00431150U, 1U, 0x004A7C68U) &&
                has_call_argument(port, 0x00431150U, 2U, 0U) &&
                has_call_argument(port, 0x00431150U, 3U, 0x004A7C44U) &&
                has_call_argument(port, 0x00431150U, 4U, 0x123U),
            "summon diagnostic adapter preserves the fixed window, text, flags, source, and line arguments"
        );
    }

    {
        LegacyBattleActionDispatchState state;
        state.group_a_count = 1;
        state.group_b_count = 1;
        state.phase_counter = 0xAABB0000U;
        state.group_a_status_words[0U] = 2U;
        Fixture fixture;
        fixture.startup.window_token = 0x12340000U;
        auto& summon = fixture.startup.party[2U];
        summon.placement_prefix = {1U, 2U, 3U, 4U, 5U};
        summon.role_id = 7U;
        summon.position_x = 0x2345U;
        summon.position_y = 0x3456U;
        summon.placement_field_1a = 0x4567U;
        summon.active = 1U;
        summon.configuration.actor_record_token = 0x005029D0U + 2U * 0x2F34U;
        DispatchPort port;
        port.action = 15U;
        port.push(0x00487C10U, {.eax = 0x71000000U});
        set_summon_profile_word(port.summon_profile, 0x56U, 0x1111U);
        set_summon_profile_word(port.summon_profile, 0x60U, 0x2222U);
        set_summon_profile_word(port.summon_profile, 0x64U, 0x3333U);
        port.definition_description = {0x41U};
        auto context = fixture.context();

        const auto result = openswd3::battle::dispatch_legacy_battle_action(
            state, port, context, 0U, 0U
        );

        test.expect_true(
            result.status == LegacyBattleActionDispatchStatus::completed &&
                result.return_value == 0U &&
                result.summon_materialization_calls == 1U &&
                result.summon_materialization.status ==
                    openswd3::battle::
                        LegacyBattleGroupASummonMaterializationStatus::
                            completed &&
                result.summon_materialization.return_eax == 0x71000000U &&
                result.summon_materialization.return_edx ==
                    summon.configuration.actor_record_token &&
                port.summon_materialization_calls.size() == 1U &&
                port.count(0x0046E890U) == 0U &&
                port.count(0x00487C10U) == 1U &&
                port.count(0x00476DB0U) == 0U &&
                port.count(0x00478220U) == 0U &&
                port.definition_text_release_calls == 1U &&
                port.requested_definition_ids == std::vector<u32>{7U} &&
                summon.configuration.profile_token == 0x71000000U &&
                summon.configuration.placement_primary[5U] == 0x23450007U &&
                summon.configuration.source_record_token ==
                    summon.configuration.actor_record_token &&
                (summon.configuration.actor_record[9U] >> 16U) == 0x1111U &&
                summon.configuration.profile_field_f2 == 0x2222U &&
                static_cast<u16>(summon.configuration.actor_record[1U]) ==
                    0x3333U &&
                state.phase_counter == 0xAABB0001U && state.summon_x == 0U &&
                state.summon_y == 0U && result.summon_frame_calls == 1U &&
                result.summon_frame.return_eax == 0U &&
                state.group_a_action_execution[0U].turn_threshold == 2U &&
                state.group_a_action_shared.draw_motion_a == 0xFFFFFFE1U &&
                port.count(0x00471D60U) == 0U,
            "action fifteen materializes the selected summon from the shared startup record before beginning its frame phase"
        );
    }

    {
        LegacyBattleActionDispatchState state;
        state.group_a_count = 1;
        state.group_b_count = 1;
        state.group_a_status_words[0U] = 1U;
        Fixture fixture;
        fixture.startup.window_token = 0x76543210U;
        fixture.startup.party[1U].role_id = 0U;
        fixture.startup.party[1U].configuration.actor_record_token =
            0x005029D0U + 0x2F34U;
        DispatchPort port;
        port.action = 15U;
        port.push(0x00487C10U, {.eax = 0x72000000U});
        auto context = fixture.context();

        const auto result = openswd3::battle::dispatch_legacy_battle_action(
            state, port, context, 0U, 0U
        );

        test.expect_true(
            result.status == LegacyBattleActionDispatchStatus::completed &&
                result.summon_materialization.diagnostic_calls == 1U &&
                port.count(0x00431150U) == 1U &&
                has_call_argument(port, 0x00431150U, 0U, 0x76543210U) &&
                has_call_argument(port, 0x00431150U, 1U, 0x004A7C68U) &&
                has_call_argument(port, 0x00431150U, 2U, 0U) &&
                has_call_argument(port, 0x00431150U, 3U, 0x004A7C44U) &&
                has_call_argument(port, 0x00431150U, 4U, 0x123U),
            "zero summon role forwards the fixed diagnostic payload after profile release"
        );
    }

    {
        LegacyBattleActionDispatchState state;
        state.group_a_count = 1;
        state.group_b_count = 1;
        state.group_a_status_words[0U] = 0U;
        Fixture fixture;
        DispatchPort port;
        port.action = 15U;
        port.push(0x00487C10U, {.eax = 0x73000000U});
        auto context = fixture.context();
        context.startup = nullptr;

        const auto result = openswd3::battle::dispatch_legacy_battle_action(
            state, port, context, 0U, 0U
        );

        test.expect_true(
            result.status ==
                    LegacyBattleActionDispatchStatus::
                        summon_materialization_typed_stop &&
                result.summon_materialization.status ==
                    openswd3::battle::
                        LegacyBattleGroupASummonMaterializationStatus::
                            actor_state_typed_stop &&
                result.summon_materialization.allocation_calls == 1U &&
                result.summon_materialization.load_calls == 0U &&
                port.count(0x00487C10U) == 1U &&
                port.count(0x0046E890U) == 0U &&
                static_cast<u16>(state.phase_counter) == 0U,
            "missing shared summon owner stops after allocation and clear before the first actor write"
        );
    }

    {
        openswd3::battle::LegacyBattleTargetPhaseState phase;
        openswd3::battle::LegacyBattleGroupAActionExecutionState actor;
        actor.position_x = 240U;
        actor.position_y = 220U;
        actor.target_phase_y_adjustment = 20;
        actor.source_x_offset = 40U;
        actor.source_y_offset = 10U;
        phase.group_a_mode_flags = 0x80U;
        phase.runtime_gate = 1U;
        phase.block_0df4.fill(1U);
        phase.action_record.action_id = 2U;
        phase.spawn_action_records[0U].action_id = 3U;
        Fixture fixture;
        DispatchPort port;
        const auto result = openswd3::battle::start_legacy_battle_target_phase(
            &phase,
            &actor,
            &fixture.startup.render_geometry,
            port,
            {
                .target_token = 0x00525508U,
                .surface_width = 640,
                .surface_height = 480,
                .entry_eax = 0xAAAAAAAAU,
                .entry_ecx = 0xBBBBBBBBU,
                .entry_edx = 0xCCCCCCCCU,
            }
        );
        const auto& emitter = phase.emitter;
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleTargetPhaseStartStatus::
                        completed &&
                result.port_calls == 4U && result.resource_query_calls == 1U &&
                result.coordinate_query_calls == 1U &&
                result.decode_calls == 1U &&
                result.property_query_calls == 1U &&
                result.presentation_dwords_zeroed == 0x16U &&
                result.tail_dwords_zeroed == 0xECU &&
                result.host_surface_calls == 1U && result.return_eax == 0U &&
                result.return_ecx == 0U &&
                phase.resource_token == 0x72000000U &&
                phase.decoded_resource_token == 0x74000000U &&
                emitter.source_pixels.size() == 0x20U * 0x50U &&
                emitter.source_width == 0x20U &&
                emitter.source_height == 0x50U &&
                emitter.source_origin_x == 0x2F &&
                emitter.source_origin_y == 0x40 &&
                emitter.target_origin_x == 220 &&
                emitter.target_origin_y == 240 && emitter.target_width == 1 &&
                emitter.target_height == 1 &&
                emitter.distance_offset_base == 0x14U &&
                emitter.lifetime_divisor == 0x1EU &&
                emitter.remaining_batches == 0x46U &&
                emitter.spawn_divisor == 0x28U && emitter.flags == 0x57U &&
                emitter.published_value_2c == 5 &&
                emitter.published_value_30 == 5 &&
                emitter.published_value_34 == 5 &&
                phase.group_a_mode_flags == 0x88U &&
                phase.runtime_gate == 0U &&
                std::ranges::all_of(
                    phase.block_0df4,
                    [](const u32 value) { return value == 0U; }
                ) &&
                fixture.startup.render_geometry.surface_width == 640 &&
                fixture.startup.render_geometry.surface_height == 480,
            "target phase start publishes the decoded emitter, geometry and exact clear suffix"
        );
    }

    {
        openswd3::battle::LegacyBattleTargetPhaseState phase;
        openswd3::battle::LegacyBattleGroupAActionExecutionState actor;
        phase.emitter.flags = 0xFFFFU;
        phase.block_0df4.fill(9U);
        Fixture fixture;
        DispatchPort port;
        port.push(0x00478620U, {.eax = 0U});
        const auto result = openswd3::battle::start_legacy_battle_target_phase(
            &phase,
            &actor,
            &fixture.startup.render_geometry,
            port,
            {.target_token = 0x00525508U,
             .surface_width = 640,
             .surface_height = 480}
        );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleTargetPhaseStartStatus::
                        resource_object_typed_stop &&
                result.resource_query_calls == 1U &&
                result.coordinate_query_calls == 1U &&
                result.decode_calls == 0U &&
                result.presentation_dwords_zeroed == 0x16U &&
                phase.emitter.flags == 0U && phase.block_0df4[0U] == 9U,
            "target phase resource stop preserves both callee calls and the emitter clear prefix"
        );
    }
}

void test_battle_action_dispatch_part_four(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleActionDispatchState;
    using openswd3::battle::LegacyBattleActionDispatchStatus;

    {
        openswd3::battle::LegacyBattleTargetPhaseState phase;
        LegacyBattleActionDispatchState shared;
        Fixture fixture;
        DispatchPort port;
        auto& actor = shared.group_a_action_execution[0U];
        const auto stopped =
            openswd3::battle::advance_legacy_battle_target_phase_spawn_frame(
                &phase,
                &actor,
                &shared.group_a_action_shared,
                port,
                fixture.action_updater,
                fixture.frame_provider,
                {.actor_token = 0x005029D0U,
                 .action_id = 0x186AU,
                 .action_variant = 1U,
                 .slot = 5U,
                 .entry_eax = 0x11U,
                 .entry_ecx = 0x22U,
                 .entry_edx = 0x33U}
            );
        fixture.frame_provider.available = false;
        const auto missing =
            openswd3::battle::advance_legacy_battle_target_phase_spawn_frame(
                &phase,
                &actor,
                &shared.group_a_action_shared,
                port,
                fixture.action_updater,
                fixture.frame_provider,
                {.actor_token = 0x005029D0U,
                 .action_id = 0x186AU,
                 .action_variant = 2U,
                 .slot = 0U}
            );
        fixture.frame_provider.available = true;
        const auto no_shared =
            openswd3::battle::advance_legacy_battle_target_phase_spawn_frame(
                &phase,
                &actor,
                nullptr,
                port,
                fixture.action_updater,
                fixture.frame_provider,
                {.actor_token = 0x005029D0U,
                 .action_id = 0x186AU,
                 .action_variant = 3U,
                 .slot = 1U}
            );
        test.expect_true(
            stopped.status ==
                    openswd3::battle::
                        LegacyBattleTargetPhaseSpawnFrameStatus::
                            actor_state_typed_stop &&
                stopped.return_eax == 0x11U && stopped.return_ecx == 0x22U &&
                stopped.return_edx == 0x33U &&
                phase.spawn_action_records[0U].action_id == 0x186AU &&
                missing.status ==
                    openswd3::battle::
                        LegacyBattleTargetPhaseSpawnFrameStatus::
                            frame_owner_typed_stop &&
                missing.action_update_calls == 1U &&
                missing.frame_lookup_calls == 1U && missing.port_calls == 0U &&
                no_shared.status ==
                    openswd3::battle::
                        LegacyBattleTargetPhaseSpawnFrameStatus::
                            shared_state_typed_stop &&
                no_shared.action_update_calls == 1U &&
                no_shared.frame_lookup_calls == 1U &&
                phase.spawn_action_records[1U].action_id == 0x186AU &&
                actor.turn_frame_token == 0x00504F1CU,
            "target phase spawn stops at the original slot and frame accesses"
        );
    }

    {
        openswd3::battle::LegacyBattleTargetPhaseState phase;
        LegacyBattleActionDispatchState shared;
        Fixture fixture;
        DispatchPort port;
        auto& actor = shared.group_a_action_execution[0U];
        actor.position_x = 100U;
        phase.spawn_counters[0U] = 0x1234FFFFU;
        phase.spawn_action_records[0U].field_58 = 0x44U;
        const auto result =
            openswd3::battle::advance_legacy_battle_target_phase_spawn_frame(
                &phase,
                &actor,
                &shared.group_a_action_shared,
                port,
                fixture.action_updater,
                fixture.frame_provider,
                {.actor_token = 0x005029D0U,
                 .action_id = 0x186AU,
                 .action_variant = 3U,
                 .slot = 0U,
                 .target_x = 0U,
                 .target_y = 0U,
                 .iterations = 0U}
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::
                        LegacyBattleTargetPhaseSpawnFrameStatus::completed &&
                result.return_eax == 0U && result.line_raster_calls == 0U &&
                result.sample_calls == 1U && result.render_calls == 1U,
            "target phase spawn keeps zero iterations noncomplete"
        );
        test.expect_true(
            phase.spawn_counters[0U] == 0x12350000U &&
                phase.spawn_action_records[0U].field_58 == 0U,
            "target phase spawn increments the counter and clears the sample word"
        );
        test.expect_true(
            port.calls[0U].callee_token == 0x00485610U &&
                port.calls[0U].arguments[0U] == 0x12350000U &&
                port.calls[1U].callee_token == 0x004170E0U &&
                port.calls[1U].arguments[0U] == 0U,
            "target phase spawn preserves the incremented counter high half in the sample argument before drawing"
        );
    }

    {
        openswd3::battle::LegacyBattleTargetPhaseState phase;
        LegacyBattleActionDispatchState shared;
        Fixture fixture;
        DispatchPort port;
        auto& actor = shared.group_a_action_execution[0U];
        phase.spawn_action_records[0U].field_58 = 0x55U;
        const auto result =
            openswd3::battle::advance_legacy_battle_target_phase_spawn_frame(
                &phase,
                &actor,
                &shared.group_a_action_shared,
                port,
                fixture.action_updater,
                fixture.frame_provider,
                {.actor_token = 0x005029D0U,
                 .action_id = 0x186AU,
                 .action_variant = 1U,
                 .slot = 0U,
                 .target_x = 0U,
                 .target_y = 0U,
                 .iterations = 8U}
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::
                        LegacyBattleTargetPhaseSpawnFrameStatus::completed &&
                result.return_eax == 1U && phase.spawn_counters[0U] == 0U &&
                phase.block_0df4[0U] == 0U &&
                phase.spawn_action_records[0U].action_id == 0x186AU &&
                phase.spawn_action_records[0U].base_variant == 1U &&
                phase.spawn_action_records[0U].field_58 == 0U &&
                port.calls[1U].arguments[0U] == 0U,
            "target phase spawn clamps the completed draw without clearing its slot action record"
        );
    }

    {
        openswd3::battle::LegacyBattleTargetPhaseState phase;
        phase.tick = 39U;
        phase.emitter.initialized = 1;
        phase.emitter.remaining_batches = 1U;
        phase.emitter.spawn_divisor = 1U;
        phase.emitter.source_width = 40U;
        phase.emitter.source_height = 1U;
        phase.emitter.source_origin_x = 100;
        phase.emitter.source_origin_y = 200;
        phase.emitter.spawned_count = 1;
        LegacyBattleActionDispatchState shared;
        Fixture fixture;
        DispatchPort port;
        const auto result =
            openswd3::battle::advance_legacy_battle_target_phase(
                &phase,
                &shared.group_a_action_execution[0U],
                &shared.group_a_action_shared,
                &fixture.action_updater,
                &fixture.frame_provider,
                &shared.target_phase_particle_nodes,
                &shared.target_phase_particle_rng,
                &shared.target_phase_particle_shared,
                &shared.target_phase_particle_diagnostics,
                {},
                &fixture.effects.pixel_conversion,
                port,
                {.target_token = 0x005029D0U, .time_seed = 0x12345678U}
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleTargetPhaseAdvanceStatus::
                        completed &&
                result.particle_frame_calls == 1U &&
                result.particle_frame.legacy_return_value == 0 &&
                result.spawn_calls == 5U && result.spawn_frame_calls == 5U &&
                phase.tick == 40U && phase.active_gate == 1U &&
                result.return_eax == 0U && port.count(0x00471FC0U) == 0U &&
                phase.spawn_action_records[0U].action_id == 0x186AU &&
                phase.spawn_action_records[0U].base_variant == 1U &&
                phase.spawn_action_records[1U].base_variant == 2U &&
                phase.spawn_action_records[2U].base_variant == 3U &&
                phase.spawn_action_records[3U].base_variant == 1U &&
                phase.spawn_action_records[4U].base_variant == 1U &&
                result.spawn_frames[0U].sample_calls == 1U &&
                result.spawn_frames[4U].render_calls == 1U,
            "target phase tick forty repeats all five threshold particle calls with the original coordinates"
        );
    }

    {
        openswd3::battle::LegacyBattleTargetPhaseState phase;
        phase.tick = 0x7FFFU;
        phase.emitter.initialized = 1;
        phase.emitter.remaining_batches = 1U;
        phase.emitter.spawn_divisor = 1U;
        phase.emitter.source_height = 1U;
        phase.emitter.spawned_count = 1;
        LegacyBattleActionDispatchState shared;
        Fixture fixture;
        DispatchPort port;
        const auto result =
            openswd3::battle::advance_legacy_battle_target_phase(
                &phase,
                &shared.group_a_action_execution[0U],
                &shared.group_a_action_shared,
                &fixture.action_updater,
                &fixture.frame_provider,
                &shared.target_phase_particle_nodes,
                &shared.target_phase_particle_rng,
                &shared.target_phase_particle_shared,
                &shared.target_phase_particle_diagnostics,
                {},
                &fixture.effects.pixel_conversion,
                port,
                {.target_token = 0x005029D0U}
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleTargetPhaseAdvanceStatus::
                        completed &&
                phase.tick == 0x8000U && result.spawn_calls == 1U,
            "target phase signed tick overflow suppresses the four threshold branches while keeping the unconditional spawn"
        );
    }

    {
        openswd3::battle::LegacyBattleTargetPhaseState phase;
        phase.tick = 9U;
        phase.active_gate = 1U;
        phase.decoded_resource_token = 0x74000000U;
        phase.emitter.initialized = 1;
        phase.emitter.remaining_batches = 0U;
        phase.emitter.spawned_count = 0;
        phase.spawn_counters.fill(7U);
        phase.block_0df4.fill(8U);
        phase.action_record.action_id = 9U;
        phase.spawn_action_records[0U].action_id = 10U;
        LegacyBattleActionDispatchState shared;
        Fixture fixture;
        DispatchPort port;
        const auto result =
            openswd3::battle::advance_legacy_battle_target_phase(
                &phase,
                &shared.group_a_action_execution[0U],
                &shared.group_a_action_shared,
                &fixture.action_updater,
                &fixture.frame_provider,
                &shared.target_phase_particle_nodes,
                &shared.target_phase_particle_rng,
                &shared.target_phase_particle_shared,
                &shared.target_phase_particle_diagnostics,
                {},
                &fixture.effects.pixel_conversion,
                port,
                {.target_token = 0x005029D0U}
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleTargetPhaseAdvanceStatus::
                        completed &&
                result.particle_frame.legacy_return_value == 1 &&
                result.resource_release_calls == 1U &&
                result.presentation_dwords_zeroed == 0x16U &&
                result.spawn_counter_clears == 5U &&
                result.tail_dwords_zeroed == 0x2EU && result.return_eax == 1U &&
                phase.tick == 0U && phase.active_gate == 0U &&
                phase.decoded_resource_token == 0U &&
                std::ranges::all_of(
                    phase.spawn_counters,
                    [](const u32 value) { return value == 0U; }
                ) &&
                phase.block_0df4[0U] == 0U &&
                phase.action_record.action_id == 0U &&
                phase.spawn_action_records[0U].action_id == 10U &&
                port.count(0x004885A0U) == 1U,
            "target phase completion releases the decoded buffer and clears only the original presentation, counters and two tail blocks"
        );
    }

    {
        openswd3::battle::LegacyBattleTargetPhaseState phase;
        openswd3::battle::LegacyBattleGroupAActionExecutionState actor;
        openswd3::battle::LegacyBattleGroupAActionExecutionSharedState shared;
        Fixture fixture;
        DispatchPort port;
        auto context = fixture.context();
        const auto stopped =
            openswd3::battle::advance_legacy_battle_action_thirteen(
                &phase,
                nullptr,
                &shared,
                port,
                context,
                {.actor_token = 0x005029D0U,
                 .opponent_token = 0x00525508U,
                 .entry_eax = 0x11U,
                 .entry_ecx = 0x22U,
                 .entry_edx = 0x33U}
            );
        fixture.frame_provider.available = false;
        const auto missing =
            openswd3::battle::advance_legacy_battle_action_thirteen(
                &phase,
                &actor,
                &shared,
                port,
                context,
                {.actor_token = 0x005029D0U,
                 .opponent_token = 0x00525508U}
            );
        test.expect_true(
            stopped.status ==
                    openswd3::battle::LegacyBattleActionThirteenStatus::
                        actor_state_typed_stop &&
                stopped.return_eax == 0x11U && stopped.return_ecx == 0x22U &&
                stopped.return_edx == 0x33U &&
                missing.status ==
                    openswd3::battle::LegacyBattleActionThirteenStatus::
                        frame_owner_typed_stop &&
                missing.action_update_calls == 1U &&
                missing.frame_lookup_calls == 1U && missing.port_calls == 0U &&
                actor.turn_frame_token == 0U &&
                phase.action_record.action_id == 0x186BU,
            "action thirteen stops at the original actor and frame reads after preserving the initialized record prefix"
        );
    }

    {
        openswd3::battle::LegacyBattleTargetPhaseState phase;
        openswd3::battle::LegacyBattleGroupAActionExecutionState actor;
        openswd3::battle::LegacyBattleGroupAActionExecutionSharedState shared;
        phase.runtime_gate = 1U;
        phase.render_toggle_gate = 1U;
        Fixture fixture;
        DispatchPort port;
        auto context = fixture.context();
        const auto completed =
            openswd3::battle::advance_legacy_battle_action_thirteen(
                &phase,
                &actor,
                &shared,
                port,
                context,
                {.actor_token = 0x005029D0U,
                 .opponent_token = 0x00525508U}
            );
        test.expect_true(
            completed.status ==
                    openswd3::battle::LegacyBattleActionThirteenStatus::
                        completed &&
                completed.return_eax == 1U &&
                completed.action_update_calls == 1U &&
                completed.frame_lookup_calls == 1U &&
                completed.coordinate_query_calls == 2U &&
                completed.line_raster_calls == 1U &&
                completed.sample_calls == 1U && completed.render_calls == 1U &&
                completed.port_calls == 3U && phase.runtime_gate == 0U &&
                phase.action_record.action_id == 0U &&
                phase.block_0df4[0U] == 0U &&
                actor.turn_frame_token == 0x00504F1CU &&
                actor.turn_target_x_offset == 32U &&
                (actor.turn_render_flags & 1U) == 1U &&
                shared.turn_frame_source_token == actor.turn_frame_token &&
                port.count(0x004321E0U) == 0U &&
                port.count(0x004315D0U) == 0U &&
                port.count(0x00434350U) == 0U &&
                port.count(0x004717F0U) == 0U && port.count(0x004783B0U) == 0U,
            "action thirteen directly updates the record and frame, toggles bit zero, completes the first raster step and clears both physical owners"
        );
    }

    {
        openswd3::battle::LegacyBattleTargetPhaseState phase;
        openswd3::battle::LegacyBattleGroupAActionExecutionState actor;
        openswd3::battle::LegacyBattleGroupAActionExecutionSharedState shared;
        Fixture fixture;
        auto& coordinate_actor =
            (*fixture.startup.group_b_lifecycle)[0U].action_execution;
        coordinate_actor.position_x = 0x1234U;
        coordinate_actor.position_y = 0x5678U;
        coordinate_actor.position_y_read_accessible = false;
        DispatchPort port;
        port.push(0x00478400U, {.outputs = {0xAAAA0000U, 0xBBBB0000U}});
        auto context = fixture.context();
        const auto stopped =
            openswd3::battle::advance_legacy_battle_action_thirteen(
                &phase,
                &actor,
                &shared,
                port,
                context,
                {
                    .actor_token = 0x005029D0U,
                    .opponent_token = 0x00525508U,
                    .coordinate_output_x_token = 0x11112222U,
                    .coordinate_output_y_token = 0x33334444U,
                }
            );
        test.expect_true(
            stopped.status ==
                    openswd3::battle::LegacyBattleActionThirteenStatus::
                        actor_coordinate_typed_stop &&
                stopped.coordinate_query.status ==
                    openswd3::battle::LegacyBattleActorCoordinateQueryStatus::
                        primary_y_read_typed_stop &&
                stopped.coordinate_query.output_writes == 1U &&
                stopped.coordinate_output_x == 0xAAAA1234U &&
                stopped.coordinate_output_y == 0xBBBB0000U &&
                stopped.return_eax == 0x11112222U &&
                stopped.return_ecx == 0x00525508U &&
                stopped.return_edx == 0x33334444U &&
                stopped.line_raster_calls == 0U && stopped.sample_calls == 0U &&
                stopped.render_calls == 0U && port.count(0x004783B0U) == 0U,
            "action thirteen preserves both offset-slot high words and the X write when Y faults"
        );
    }

    {
        openswd3::battle::LegacyBattleTargetPhaseState phase;
        openswd3::battle::LegacyBattleGroupAActionExecutionState actor;
        openswd3::battle::LegacyBattleGroupAActionExecutionSharedState shared;
        actor.position_x = 100U;
        actor.position_y = 80U;
        Fixture fixture;
        DispatchPort port;
        port.push(0x00478400U, {.outputs = {2U, 3U}});
        port.push(0x00478470U, {.outputs = {10U, 20U}});
        auto context = fixture.context();
        const auto result =
            openswd3::battle::advance_legacy_battle_action_thirteen(
                &phase,
                &actor,
                &shared,
                port,
                context,
                {.actor_token = 0x005029D0U,
                 .opponent_token = 0x00525508U}
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleActionThirteenStatus::
                        completed &&
                result.return_eax == 0U &&
                result.coordinate_query_calls == 2U &&
                result.line_raster_calls == 0U &&
                phase.runtime_gate == 1U &&
                phase.action_record.action_id == 0x186BU &&
                port.count(0x00478400U) == 1U &&
                port.count(0x00478470U) == 1U &&
                port.count(0x004783B0U) == 0U &&
                port.count(0x004170E0U) == 1U,
            "action thirteen preserves the nonzero offset branch and renders the start point before the next raster frame"
        );
    }

    {
        openswd3::battle::LegacyBattleTargetPhaseState phase;
        openswd3::battle::LegacyBattleGroupAActionExecutionState actor;
        openswd3::battle::LegacyBattleGroupAActionExecutionSharedState shared;
        Fixture fixture;
        DispatchPort port;
        auto context = fixture.context();
        const auto stopped =
            openswd3::battle::advance_legacy_battle_action_fourteen(
                &phase,
                nullptr,
                &shared,
                port,
                context,
                {.actor_token = 0x005029D0U,
                 .opponent_token = 0x00525508U,
                 .entry_eax = 0x11U,
                 .entry_ecx = 0x22U,
                 .entry_edx = 0x33U}
            );
        fixture.frame_provider.available = false;
        const auto missing =
            openswd3::battle::advance_legacy_battle_action_fourteen(
                &phase,
                &actor,
                &shared,
                port,
                context,
                {.actor_token = 0x005029D0U,
                 .opponent_token = 0x00525508U}
            );
        test.expect_true(
            stopped.status ==
                    openswd3::battle::LegacyBattleActionFourteenStatus::
                        actor_state_typed_stop &&
                stopped.return_eax == 0x11U && stopped.return_ecx == 0x22U &&
                stopped.return_edx == 0x33U &&
                missing.status ==
                    openswd3::battle::LegacyBattleActionFourteenStatus::
                        frame_owner_typed_stop &&
                missing.action_update_calls == 1U &&
                missing.frame_lookup_calls == 1U && missing.port_calls == 0U &&
                phase.action_record.action_id == 0x186BU &&
                phase.action_record.base_variant == 1U,
            "action fourteen stops at the original actor and frame reads after preserving variant one"
        );
    }

    {
        openswd3::battle::LegacyBattleTargetPhaseState phase;
        openswd3::battle::LegacyBattleGroupAActionExecutionState actor;
        openswd3::battle::LegacyBattleGroupAActionExecutionSharedState shared;
        phase.runtime_gate = 1U;
        Fixture fixture;
        DispatchPort port;
        auto context = fixture.context();
        const auto completed =
            openswd3::battle::advance_legacy_battle_action_fourteen(
                &phase,
                &actor,
                &shared,
                port,
                context,
                {.actor_token = 0x005029D0U,
                 .opponent_token = 0x00525508U}
            );
        test.expect_true(
            completed.status ==
                    openswd3::battle::LegacyBattleActionFourteenStatus::
                        completed &&
                completed.return_eax == 1U &&
                completed.coordinate_query_calls == 2U &&
                completed.line_raster_calls == 1U &&
                completed.sample_calls == 1U && completed.render_calls == 1U &&
                completed.port_calls == 3U && phase.runtime_gate == 0U &&
                phase.action_record.action_id == 0U &&
                phase.block_0df4[0U] == 0U &&
                actor.turn_frame_token == 0x00504F1CU &&
                shared.turn_frame_source_token == actor.turn_frame_token &&
                port.count(0x004321E0U) == 0U &&
                port.count(0x004315D0U) == 0U &&
                port.count(0x00434350U) == 0U &&
                port.count(0x00471AD0U) == 0U && port.count(0x004783B0U) == 0U,
            "action fourteen directly completes the reverse first raster step and clears both physical owners"
        );
    }

    {
        openswd3::battle::LegacyBattleTargetPhaseState phase;
        openswd3::battle::LegacyBattleGroupAActionExecutionState actor;
        openswd3::battle::LegacyBattleGroupAActionExecutionSharedState shared;
        actor.position_x = 100U;
        actor.position_y = 80U;
        actor.render_x_base = 5U;
        actor.render_y_base = 6U;
        actor.source_y_offset = 2U;
        Fixture fixture;
        DispatchPort port;
        port.push(0x00478400U, {.outputs = {2U, 3U}});
        port.push(0x00478470U, {.outputs = {10U, 20U}});
        auto context = fixture.context();
        const auto result =
            openswd3::battle::advance_legacy_battle_action_fourteen(
                &phase,
                &actor,
                &shared,
                port,
                context,
                {.actor_token = 0x005029D0U,
                 .opponent_token = 0x00525508U}
            );
        const auto raster = std::bit_cast<
            openswd3::battle::LegacyBattleLineRaster>(phase.block_0df4);
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleActionFourteenStatus::
                        completed &&
                result.return_eax == 0U &&
                result.coordinate_query_calls == 2U &&
                result.line_raster_calls == 0U &&
                phase.runtime_gate == 1U && raster.start_x == 12 &&
                raster.start_y == 23 && raster.end_x == 103 &&
                raster.end_y == 86 && port.count(0x00478400U) == 1U &&
                port.count(0x00478470U) == 1U &&
                port.count(0x004783B0U) == 0U &&
                port.count(0x004170E0U) == 1U,
            "action fourteen preserves the nonzero offset branch and reverses the raster from target to actor"
        );
    }

    {
        LegacyBattleActionDispatchState state;
        state.group_a_count = 1;
        state.group_b_count = 1;
        state.group_a_target_phases[0U].runtime_gate = 1U;
        Fixture fixture;
        DispatchPort port;
        port.action = 14U;
        auto context = fixture.context();
        const auto result = openswd3::battle::dispatch_legacy_battle_action(
            state, port, context, 0U, 0U
        );
        test.expect_true(
            result.status == LegacyBattleActionDispatchStatus::completed &&
                result.action_fourteen_calls == 1U &&
                result.action_fourteen.return_eax == 1U &&
                port.count(0x00471AD0U) == 0U && result.return_value == 1U &&
                state.frame_effect.primary_suppression == 0U &&
                state.frame_effect.red_factor == 0 &&
                state.frame_effect.green_factor == 0 &&
                state.frame_effect.blue_factor == 0 &&
                state.frame_effect.fade_active == 1U &&
                static_cast<u16>(state.phase_counter) == 0U &&
                port.battle_message_state() == 0x62U &&
                state.current_actor_index == 0xFFFFU,
            "action fourteen production caller completes the typed reverse frame before publishing message ninety eight"
        );
    }

    {
        openswd3::battle::LegacyBattleTargetPhaseState phase;
        openswd3::battle::LegacyBattleGroupAActionExecutionState actor;
        openswd3::battle::LegacyBattleGroupAActionExecutionSharedState shared;
        Fixture fixture;
        DispatchPort port;
        auto context = fixture.context();
        const auto stopped =
            openswd3::battle::advance_legacy_battle_summon_frame(
                &phase,
                nullptr,
                &shared,
                port,
                context.action_updater,
                context.frame_provider,
                {.actor_token = 0x005029D0U,
                 .position_x = 100U,
                 .position_y = 80U,
                 .entry_eax = 0x11U,
                 .entry_ecx = 0x22U,
                 .entry_edx = 0x33U}
            );
        fixture.frame_provider.available = false;
        const auto missing =
            openswd3::battle::advance_legacy_battle_summon_frame(
                &phase,
                &actor,
                &shared,
                port,
                context.action_updater,
                context.frame_provider,
                {.actor_token = 0x005029D0U,
                 .position_x = 100U,
                 .position_y = 80U}
            );
        test.expect_true(
            stopped.status ==
                    openswd3::battle::LegacyBattleSummonFrameStatus::
                        actor_state_typed_stop &&
                stopped.return_eax == 0x11U && stopped.return_ecx == 0x22U &&
                stopped.return_edx == 0x33U &&
                missing.status ==
                    openswd3::battle::LegacyBattleSummonFrameStatus::
                        frame_owner_typed_stop &&
                missing.action_update_calls == 1U &&
                missing.frame_lookup_calls == 1U && missing.sample_calls == 1U &&
                missing.port_calls == 1U &&
                actor.summon_render_flags == 1U &&
                actor.summon_x_offset == 0U && actor.turn_sample_word == 0U &&
                phase.action_record.base_variant == 0x24U,
            "summon frame stops at the original actor and frame reads after preserving variant thirty six"
        );
    }

    {
        openswd3::battle::LegacyBattleTargetPhaseState phase;
        openswd3::battle::LegacyBattleGroupAActionExecutionState actor;
        openswd3::battle::LegacyBattleGroupAActionExecutionSharedState shared;
        actor.summon_action_id = 0x1234U;
        actor.turn_threshold = 62U;
        Fixture fixture;
        DispatchPort port;
        auto context = fixture.context();
        const auto result =
            openswd3::battle::advance_legacy_battle_summon_frame(
                &phase,
                &actor,
                &shared,
                port,
                context.action_updater,
                context.frame_provider,
                {.actor_token = 0x005029D0U,
                 .position_x = 100U,
                 .position_y = 80U}
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleSummonFrameStatus::
                        completed &&
                result.return_eax == 0U && result.sample_calls == 1U &&
                result.render_calls == 1U && result.port_calls == 2U &&
                actor.summon_phase == 1U && actor.turn_threshold == 62U &&
                phase.tick == 1U && shared.draw_motion_a == 1U &&
                shared.draw_motion_b == 1U && shared.draw_motion_c == 1U &&
                actor.summon_render_flags == 1U &&
                actor.summon_x_offset == 32U &&
                actor.turn_sample_word == 0U &&
                phase.action_record.action_id == 0x1234U &&
                port.count(0x00471D60U) == 0U &&
                port.count(0x004321E0U) == 0U &&
                port.count(0x004315D0U) == 0U,
            "summon frame preserves the zero-to-one same-frame phase transition and signed motion publication"
        );
    }

    {
        openswd3::battle::LegacyBattleTargetPhaseState phase;
        openswd3::battle::LegacyBattleGroupAActionExecutionState actor;
        openswd3::battle::LegacyBattleGroupAActionExecutionSharedState shared;
        actor.summon_phase = 1U;
        actor.turn_threshold = 2U;
        actor.summon_completion_word = 9U;
        phase.tick = 7U;
        phase.spawn_action_records[0U].action_id = 0xAAAAAAAAU;
        Fixture fixture;
        DispatchPort port;
        auto context = fixture.context();
        const auto result =
            openswd3::battle::advance_legacy_battle_summon_frame(
                &phase,
                &actor,
                &shared,
                port,
                context.action_updater,
                context.frame_provider,
                {.actor_token = 0x005029D0U,
                 .position_x = 100U,
                 .position_y = 80U}
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleSummonFrameStatus::
                        completed &&
                result.return_eax == 1U && result.sample_calls == 2U &&
                result.render_calls == 1U && result.port_calls == 3U &&
                shared.draw_motion_a == 0xFFFFFFE2U &&
                actor.turn_threshold == 0U && actor.summon_phase == 0U &&
                actor.summon_render_flags == 0U &&
                actor.summon_x_offset == 0U && phase.tick == 0U &&
                actor.summon_completion_word == 0U &&
                phase.action_record.action_id == 0U &&
                phase.spawn_action_records[0U].action_id == 0U &&
                port.calls[1U].callee_token == 0x004170E0U &&
                port.calls[2U].callee_token == 0x00485610U &&
                port.calls[2U].arguments[0U] == 0x6AU,
            "summon frame renders the terminal phase before the fixed sample and exact owner clear"
        );
    }

    {
        LegacyBattleGroupAActionExecutionState actor;
        actor.profile_level = 20U;
        LegacyBattleActionMessageProfile profile{
            .phase_flags = 0x800U,
            .phase_limit = 0x15U,
            .level = 10U,
        };
        DispatchPort port;
        port.push(0x00484500U, {.outputs = {7U, 9U}});
        const auto result = openswd3::battle::check_legacy_battle_target_phase(
            &actor, &profile, port, {.target_token = 0x00525508U}
        );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleTargetPhaseCheckStatus::
                        completed &&
                result.value_query_calls == 1U && result.random_calls == 0U &&
                result.return_eax == 1U && port.count(0x00484500U) == 1U &&
                port.count(0x00439070U) == 0U,
            "target phase check accepts an actor advantage of ten after the mandatory value query"
        );
    }

    {
        LegacyBattleGroupAActionExecutionState actor;
        actor.profile_level = 10U;
        LegacyBattleActionMessageProfile profile{
            .phase_flags = 0x800U,
            .level = 18U,
        };
        DispatchPort port;
        port.push(
            0x00484500U,
            {
                .outputs = {
                    std::bit_cast<u32>(-2),
                    std::bit_cast<u32>(-9),
                },
            }
        );
        const auto result = openswd3::battle::check_legacy_battle_target_phase(
            &actor, &profile, port, {.target_token = 0x00525508U}
        );
        test.expect_true(
            result.return_eax == 1U && result.level_delta == 8 &&
                result.sampled_argument == -9 && result.random_calls == 0U,
            "target phase check preserves signed divide-by-four truncation for a seven-to-eleven target advantage"
        );
    }

    {
        LegacyBattleGroupAActionExecutionState actor;
        actor.profile_level = 15U;
        LegacyBattleActionMessageProfile profile{
            .phase_flags = 0x800U,
            .level = 10U,
        };
        DispatchPort port;
        port.push(0x00484500U, {.outputs = {4U, 9U}});
        port.push(0x00439070U, {.eax = 80U});
        const auto result = openswd3::battle::check_legacy_battle_target_phase(
            &actor, &profile, port, {.target_token = 0x00525508U}
        );
        test.expect_true(
            result.return_eax == 1U && result.random_calls == 1U &&
                port.count(0x00439070U) == 1U,
            "target phase check keeps the inclusive eighty random threshold for an actor advantage of five"
        );
    }

    {
        LegacyBattleGroupAActionExecutionState actor;
        actor.profile_level = 10U;
        LegacyBattleActionMessageProfile profile{
            .phase_flags = 0x800U,
            .level = 12U,
        };
        DispatchPort port;
        port.push(0x00484500U, {.outputs = {4U, 9U}});
        port.push(0x00439070U, {.eax = 11U});
        const auto result = openswd3::battle::check_legacy_battle_target_phase(
            &actor, &profile, port, {.target_token = 0x00525508U}
        );
        profile.phase_flags = 0x820U;
        DispatchPort gated_port;
        gated_port.push(0x00484500U, {.outputs = {1U, 99U}});
        const auto gated = openswd3::battle::check_legacy_battle_target_phase(
            nullptr,
            &profile,
            gated_port,
            {.target_token = 0x00525508U}
        );
        profile.phase_flags = 0x800U;
        DispatchPort stopped_port;
        stopped_port.push(0x00484500U, {.outputs = {1U, 99U}});
        const auto stopped = openswd3::battle::check_legacy_battle_target_phase(
            nullptr,
            &profile,
            stopped_port,
            {.target_token = 0x00525508U}
        );
        test.expect_true(
            result.return_eax == 0U && result.random_calls == 1U &&
                gated.status ==
                    openswd3::battle::LegacyBattleTargetPhaseCheckStatus::
                        completed &&
                gated.value_query_calls == 1U &&
                stopped.status ==
                    openswd3::battle::LegacyBattleTargetPhaseCheckStatus::
                        actor_profile_typed_stop &&
                stopped.value_query_calls == 1U,
            "target phase check keeps the level-scaled random rejection and original actor access point"
        );
    }

    {
        DispatchPort port;
        const auto stopped = openswd3::battle::check_legacy_battle_target_phase(
            nullptr, nullptr, port, {.target_token = 0x00525508U}
        );
        test.expect_true(
            stopped.status ==
                    openswd3::battle::LegacyBattleTargetPhaseCheckStatus::
                        target_profile_typed_stop &&
                stopped.value_query_calls == 0U,
            "target phase check stops before the value query when target profile resolution is unavailable"
        );
    }

    {
        LegacyBattleActionDispatchState state;
        state.group_a_count = 1;
        state.group_b_count = 1;
        state.group_a_action_execution[0U].profile_level = 20U;
        state.group_b_message_profiles[0U] = {
            .phase_flags = 0x800U,
            .phase_limit = 0x15U,
            .level = 10U,
        };
        state.group_a_action_execution[0U].position_x = 240U;
        state.group_a_action_execution[0U].position_y = 220U;
        state.group_a_action_execution[0U].source_x_offset = 40U;
        state.group_a_action_execution[0U].source_y_offset = 10U;
        Fixture fixture;
        DispatchPort port;
        port.action = 6U;
        port.push(
            0x0047F910U, {.eax = 7U, .ecx = 0xAABBCCDDU, .edx = 0x12340000U}
        );
        port.legacy_battle_fixed_object_state().object_words[0U][1U] =
            (20U << 16U) | 7U;
        auto context = fixture.context();
        const auto result = openswd3::battle::dispatch_legacy_battle_action(
            state, port, context, 0U, 0U
        );
        test.expect_true(
            result.status == LegacyBattleActionDispatchStatus::completed &&
                result.fixed_count_lookup_calls == 1U &&
                result.fixed_count_lookup.path ==
                    openswd3::battle::LegacyBattleFixedCountPath::
                        existing_root &&
                result.fixed_count_lookup.return_eax == 20U &&
                state.phase_condition_aux == 1U &&
                port.count(0x00477800U) == 0U &&
                result.target_phase_check_calls == 1U &&
                result.target_phase_check.return_eax == 1U &&
                port.count(0x00472730U) == 0U &&
                port.count(0x00484500U) == 1U &&
                result.target_phase_start_calls == 1U &&
                result.target_phase_start.port_calls == 4U &&
                port.count(0x004710D0U) == 0U &&
                port.count(0x00478620U) == 1U &&
                port.count(0x00478470U) == 1U &&
                port.count(0x004019A0U) == 1U &&
                port.count(0x0047CE70U) == 1U && state.phase_condition == 1U &&
                static_cast<u16>(state.phase_counter) == 1U &&
                result.target_phase_advance_calls == 1U &&
                port.count(0x00471270U) == 0U &&
                state.group_a_target_phases[0U].decoded_resource_token ==
                    0x74000000U,
            "action six directly looks up the target code count before starting the typed target phase without either opaque whole-function call"
        );
    }

    {
        auto state = std::make_unique<LegacyBattleActionDispatchState>();
        state->group_a_count = 1;
        state->group_b_count = 1;
        auto fixture = std::make_unique<Fixture>();
        auto port = std::make_unique<DispatchPort>();
        port->action = 6U;
        port->push(
            0x0047F910U, {.eax = 7U, .ecx = 0xAABBCCDDU, .edx = 0x12340000U}
        );
        port->legacy_battle_fixed_object_state().object_words[0U][0U] =
            0x7A001234U;
        auto context = fixture->context();
        const auto stopped = openswd3::battle::dispatch_legacy_battle_action(
            *state, *port, context, 0U, 0U
        );
        test.expect_true(
            stopped.status ==
                    LegacyBattleActionDispatchStatus::fixed_count_typed_stop &&
                stopped.fixed_count_lookup_calls == 1U &&
                stopped.fixed_count_lookup.stopped_token == 0x7A001234U &&
                stopped.fixed_count_lookup.return_eax == 0U &&
                stopped.fixed_count_lookup.return_ecx == 0x7A001234U &&
                stopped.fixed_count_lookup.return_edx == 0x12340007U &&
                stopped.target_phase_check_calls == 0U &&
                stopped.target_phase_start_calls == 0U,
            "action six preserves the target-code lookup prefix and blocks every target-phase suffix at an unmapped fixed-count successor"
        );
    }

    {
        LegacyBattleActionMessageProfile profile;
        const auto stopped =
            openswd3::battle::query_legacy_battle_action_twenty_five_ready(
                nullptr
            );
        const auto ready =
            openswd3::battle::query_legacy_battle_action_twenty_five_ready(
                &profile
            );
        test.expect_true(
            stopped.status ==
                    openswd3::battle::LegacyBattleActionTwentyFiveReadyStatus::
                        target_profile_typed_stop &&
                ready.status ==
                    openswd3::battle::LegacyBattleActionTwentyFiveReadyStatus::
                        completed &&
                ready.return_eax == 1U,
            "action twenty-five ready preserves the target profile access boundary and constant return"
        );
    }

    {
        bool actions_complete = true;
        constexpr std::array<u16, 27> actions{
            1U,  2U,  3U,  4U,  5U,  6U,  7U,  11U, 12U,
            13U, 14U, 15U, 17U, 22U, 23U, 24U, 25U, 26U,
            27U, 28U, 29U, 31U, 32U, 33U, 34U, 35U, 36U,
        };
        for (const u16 action : actions) {
            LegacyBattleActionDispatchState state;
            state.group_a_count = 1;
            state.group_b_count = 1;
            state.group_a_to_actor[0] = 0U;
            state.battle_flags = 0x20U;
            Fixture fixture;
            DispatchPort port;
            port.action = action;
            auto context = fixture.context();
            const auto result = openswd3::battle::dispatch_legacy_battle_action(
                state, port, context, 0U, 0U
            );
            actions_complete = actions_complete &&
                result.status == LegacyBattleActionDispatchStatus::completed &&
                result.action_code == action &&
                port.count(0x00472710U) == 0U;
        }
        test.expect_true(
            actions_complete,
            "all twenty seven populated ordinary switch entries execute without default fallthrough"
        );
    }

    {
        LegacyBattleActionDispatchState state;
        state.group_a_count = 1;
        state.group_b_count = 1;
        state.group_a_to_actor[0] = 0U;
        state.battle_flags = 0x20U;
        state.stored_group_b_index = 0U;
        Fixture fixture;
        DispatchPort port;
        port.action = 25U;
        auto context = fixture.context();

        const auto result = openswd3::battle::dispatch_legacy_battle_action(
            state, port, context, 0U, 0U
        );

        test.expect_true(
            result.status == LegacyBattleActionDispatchStatus::completed &&
                result.attack_order_calls == 1U &&
                result.attack_order.written_index == 0U &&
                fixture.attack_order_records[0].value_00 == 0U &&
                fixture.attack_order_records[0].value_08 == 2U &&
                port.count(0x0045EDF0U) == 0U &&
                state.current_actor_index == 0xFFFFU &&
                result.return_value == 1U,
            "action twenty five directly appends the selected opponent to the shared attack order"
        );
    }

    {
        LegacyBattleActionDispatchState state;
        state.group_a_count = 1;
        state.group_b_count = 1;
        state.group_a_to_actor[0] = 0U;
        state.battle_flags = 0x20U;
        state.stored_group_b_index = 0U;
        Fixture fixture;
        DispatchPort port;
        port.action = 25U;
        auto context = fixture.context();
        context.attack_order_records = {};

        const auto result = openswd3::battle::dispatch_legacy_battle_action(
            state, port, context, 0U, 0U
        );

        test.expect_true(
            result.status ==
                    LegacyBattleActionDispatchStatus::attack_order_typed_stop &&
                state.group_b_status_words[0] != 0U &&
                result.return_value == 0U,
            "attack-order typed stop preserves the choice status write then blocks action completion"
        );
    }

    {
        bool special_actions_complete = true;
        constexpr std::array<u16, 10> actions{
            100U,
            200U,
            300U,
            400U,
            402U,
            404U,
            405U,
            406U,
            409U,
            500U,
        };
        for (const u16 action : actions) {
            LegacyBattleActionDispatchState state;
            state.group_a_count = 1;
            state.group_b_count = 1;
            state.group_a_to_actor[0] = 0U;
            Fixture fixture;
            DispatchPort port;
            port.action = action;
            auto context = fixture.context();
            const auto result = openswd3::battle::dispatch_legacy_battle_action(
                state, port, context, 0U, 0U
            );
            special_actions_complete = special_actions_complete &&
                result.status == LegacyBattleActionDispatchStatus::completed &&
                result.action_code == action;
        }
        test.expect_true(
            special_actions_complete,
            "all ten populated special action codes execute their predispatch branches"
        );
    }

    {
        bool defaults_match = true;
        constexpr std::array<u16, 10> actions{
            8U,
            9U,
            10U,
            16U,
            18U,
            19U,
            20U,
            21U,
            30U,
            37U,
        };
        for (const u16 action : actions) {
            LegacyBattleActionDispatchState state;
            Fixture fixture;
            DispatchPort port;
            port.action = action;
            auto context = fixture.context();
            const auto result = openswd3::battle::dispatch_legacy_battle_action(
                state, port, context, 0U, 99U
            );
            defaults_match = defaults_match && result.return_value == 0U &&
                result.port_calls == 2U;
        }
        test.expect_true(
            defaults_match,
            "nine sparse switch holes and out of range action return zero before target access"
        );
    }

    {
        bool cases_match = true;
        constexpr std::array<u16, 3> actions{34U, 35U, 36U};
        for (const u16 action : actions) {
            LegacyBattleActionDispatchState state;
            state.selection_word = 7U;
            state.selection_high_word = 9U;
            state.group_a_action_execution[0U].profile_value = 0x123U;
            Fixture fixture;
            fixture.stream_provider.bytes = {
                0x54U, 0x41U, 0x09U, 0x00U,
                0x46U, 0x52U, 0x44U, 0x00U,
                0x32U, 0x4FU,
            };
            DispatchPort port;
            port.action = action;
            port.push(0x00481010U, {.eax = 5U});
            auto context = fixture.context();
            const auto result = openswd3::battle::dispatch_legacy_battle_action(
                state, port, context, 0U, 0U
            );
            cases_match = cases_match && result.return_value == 1U &&
                state.current_actor_index == 0xFFFFU &&
                port.count(0x0047D640U) == 1U && port.count(0x0047F150U) == 1U;
        }
        test.expect_true(
            cases_match,
            "actions thirty four through thirty six preserve separate signed component presentation tails"
        );
    }
}

void test_battle_action_dispatch(openswd3::test::Context& test) {
    test_battle_action_dispatch_part_one(test);
    test_battle_action_dispatch_part_two(test);
    test_battle_action_dispatch_part_three(test);
    test_battle_action_dispatch_part_four(test);
}
