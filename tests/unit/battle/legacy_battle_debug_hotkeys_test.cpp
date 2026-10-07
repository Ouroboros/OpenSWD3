#include "openswd3/battle/legacy_battle_debug_hotkeys.hpp"
#include "openswd3/battle/legacy_battle_actor_runtime_reset.hpp"
#include "openswd3/battle/legacy_battle_pre_frame.hpp"

#include "test.hpp"

#include <algorithm>
#include <bit>
#include <deque>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <span>
#include <vector>

namespace {

using openswd3::battle::LegacyBattleDebugHotkeyBindings;
using openswd3::battle::LegacyBattleDebugHotkeyCall;
using openswd3::battle::LegacyBattleDebugHotkeyCallReply;
using openswd3::battle::LegacyBattleDebugHotkeyCallRequest;
using openswd3::battle::LegacyBattleDebugHotkeyPort;
using openswd3::battle::LegacyBattleDebugHotkeyState;
using openswd3::battle::LegacyBattleDebugHotkeyStatus;
using openswd3::compat::u32;

class Random final : public openswd3::battle::LegacyBattleBoundedRandomPort {
public:
    [[nodiscard]] u32 random_bounded(const u32 bound) override {
        last_bound = bound;
        ++calls;
        return 0U;
    }

    u32 last_bound{};
    u32 calls{};
};

class DebugPort final : public LegacyBattleDebugHotkeyPort {
public:
    [[nodiscard]] LegacyBattleDebugHotkeyCallReply invoke_debug_hotkey(
        const LegacyBattleDebugHotkeyCallRequest& request
    ) override {
        calls.push_back(request);
        if (stop_call == request.call) {
            return {.eax = 0xDEADBEEFU, .typed_stop = true};
        }

        if (request.call ==
            LegacyBattleDebugHotkeyCall::text_message_allocate) {
            const u32 token = next_text_message_token;
            next_text_message_token += 0x24U;
            return {.eax = token};
        }
        if (request.call == LegacyBattleDebugHotkeyCall::text_message_measure) {
            return {.eax = 4U};
        }
        if (replies.empty()) {
            return {};
        }
        const auto reply = replies.front();
        replies.pop_front();
        return reply;
    }

    void delay_milliseconds(const u32 milliseconds) override {
        delays.push_back(milliseconds);
    }

    [[nodiscard]] std::size_t
    count(const LegacyBattleDebugHotkeyCall call) const {
        return static_cast<std::size_t>(std::ranges::count_if(
            calls, [call](const LegacyBattleDebugHotkeyCallRequest& request) {
                return request.call == call;
            }
        ));
    }

    std::vector<LegacyBattleDebugHotkeyCallRequest> calls;
    std::deque<LegacyBattleDebugHotkeyCallReply> replies;
    std::vector<u32> delays;
    u32 next_text_message_token{0x78000000U};
    std::optional<LegacyBattleDebugHotkeyCall> stop_call;
};

struct Fixture {
    Fixture() {
        action.resolution_latch = 9U;
        startup.group_b_lifecycle = std::make_shared<std::array<
            openswd3::battle::LegacyBattleActorGroupBElementState,
            8>>();
    }

    openswd3::battle::LegacyBattleStartupState startup;
    openswd3::battle::LegacyBattleFinalActorStepState final_actor;
    openswd3::battle::LegacyBattleGroupBFrameState actor_frames;
    openswd3::battle::LegacyBattleActionDispatchState& action{
        actor_frames.shared.action
    };
    Random random;
    openswd3::battle::LegacyBattleActorMetricState actor_metrics;
    openswd3::battle::LegacyBattleActorPublicationState actor_publication;
    openswd3::battle::LegacyBattleEffectCoordinatorState effect_coordinator;
    openswd3::battle::LegacyBattleEffectShiftState effect_shift;
    openswd3::world_map::LegacyWorldPlayerControlState player_control;
    u32 message_state{};

    [[nodiscard]] LegacyBattleDebugHotkeyBindings
    bindings(const bool include_actor_frames = true) {
        return {
            .startup = startup,
            .final_actor = final_actor,
            .action = action,
            .bounded_random = random,
            .actor_metrics = actor_metrics,
            .actor_publication = actor_publication,
            .effect_coordinator = effect_coordinator,
            .effect_shift = effect_shift,
            .actor_frames = include_actor_frames ? &actor_frames : nullptr,
            .player_control = player_control,
            .message_state = message_state,
        };
    }
};

void press(
    openswd3::input_time_rng::LegacyKeyboardSnapshot& keyboard, const u32 code
) {
    keyboard[code] = 0x80U;
}

[[nodiscard]] bool has_call(
    const DebugPort& port,
    const LegacyBattleDebugHotkeyCall call,
    const u32 object_token,
    const std::size_t argument,
    const u32 value
) {
    return std::ranges::any_of(
        port.calls, [=](const LegacyBattleDebugHotkeyCallRequest& request) {
            return request.call == call &&
                request.object_token == object_token &&
                request.arguments[argument] == value;
        }
    );
}

}  // namespace

void test_battle_debug_hotkeys(openswd3::test::Context& test) {
    class RecordPort final
        : public LegacyBattleDebugHotkeyPort,
          public openswd3::battle::LegacyBattleDebugRecordPort {
    public:
        explicit RecordPort(Fixture& fixture) : fixture_(fixture) {}

        std::span<std::byte> debug_record_bytes(const u32 token) override {
            ++accesses;
            if (accesses == stop_at) {
                return {};
            }

            if (before_access) {
                before_access(accesses);
            }

            const auto found = records.find(token);
            return found == records.end() ? std::span<std::byte>{}
                                          : found->second;
        }

        LegacyBattleDebugHotkeyCallReply invoke_debug_hotkey(
            const LegacyBattleDebugHotkeyCallRequest& request
        ) override {
            if (request.call ==
                LegacyBattleDebugHotkeyCall::publish_actor_value) {
                return openswd3::battle::apply_legacy_battle_debug_actor_values(
                    fixture_.startup,
                    fixture_.action,
                    fixture_.random,
                    *this,
                    request
                );
            }

            if (request.call ==
                LegacyBattleDebugHotkeyCall::query_actor_status) {
                const auto actor =
                    openswd3::battle::resolve_legacy_battle_actor_runtime_reset(
                        {.action = &fixture_.action,
                         .startup = &fixture_.startup},
                        request.object_token
                    );
                const auto status =
                    openswd3::battle::invoke_legacy_battle_pre_frame_actor_call(
                        actor,
                        {.call = openswd3::battle::LegacyBattlePreFrameCall::
                             query_group_b_actor,
                         .actor_token = request.object_token}
                    );
                return {
                    .eax = status.eax,
                    .ecx = status.ecx,
                    .edx = status.edx,
                    .typed_stop = status.typed_stop
                };
            }

            return openswd3::battle::
                invoke_legacy_battle_debug_group_a_record_call(
                    fixture_.startup.party[0U].configuration,
                    fixture_.startup.party[0U].progress,
                    fixture_.action.group_a_action_execution[0U],
                    *this,
                    request
                );
        }

        Fixture& fixture_;
        std::map<u32, std::span<std::byte>> records;
        std::function<void(u32)> before_access;
        u32 accesses{};
        u32 stop_at{};
    };

    {
        auto fixture = std::make_unique<Fixture>();
        fixture->actor_metrics.group_a_count = 1U;
        fixture->actor_metrics.group_b_count = 1U;
        auto& configuration = fixture->startup.party[0U].configuration;
        configuration.source_runtime_value = 1U;
        configuration.actor_record_token = 0x78001000U;
        configuration.actor_record[1U] = 1000U | (20U << 16U);
        configuration.actor_record[2U] = 20U | (1000U << 16U);
        configuration.actor_record[3U] = 30U | (30U << 16U);
        auto& enemy = (*fixture->startup.group_b_lifecycle)[0U];
        enemy.action_configuration.source_runtime_value = 2U;
        enemy.resource_token = 0x78002000U;
        enemy.resource_bytes[0x4CU] = 100U;
        enemy.resource_bytes[0x64U] = 100U;
        RecordPort port(*fixture);
        port.records.emplace(
            0x78001000U,
            std::as_writable_bytes(std::span{configuration.actor_record})
        );
        port.records.emplace(
            0x78002000U, std::as_writable_bytes(std::span{enemy.resource_bytes})
        );
        LegacyBattleDebugHotkeyState state;
        state.developer_tools_enabled = 1U;
        openswd3::input_time_rng::LegacyKeyboardSnapshot keyboard{};
        for (const u32 key : {0x1DU, 0x20U, 0x21U, 0x2FU, 0x11U}) {
            press(keyboard, key);
        }

        const auto result =
            openswd3::battle::coordinate_legacy_battle_debug_hotkeys(
                keyboard, state, fixture->bindings(), port
            );
        test.expect_true(
            result.status == LegacyBattleDebugHotkeyStatus::completed &&
                result.port_calls == 5U &&
                configuration.actor_record[1U] == (420U | (5U << 16U)) &&
                (configuration.actor_record[2U] & 0xFFFFU) == 5U &&
                enemy.resource_bytes[0x64U] == 0U &&
                enemy.resource_bytes[0x65U] == 0U &&
                fixture->startup.enemies[0U].progress.presentation_enabled ==
                    1U &&
                fixture->random.calls == 1U &&
                fixture->random.last_bound == 10U &&
                fixture->actor_publication.slots[0U] == 0U &&
                fixture->startup.reset.block_5242b0[0U] == 0U,
            "D/F/V/W share live records, status queries, death random and caller publication"
        );
    }

    struct PartyValueCase {
        u32 hp, hp_max, mp, mp_max, sp, sp_max;
        u32 damage, mp_delta, sp_delta, ai;
        u32 expected_hp, expected_mp, expected_sp, expected_death_byte;
    };
    for (const auto sample :
         {PartyValueCase{
              0x7FFFU,
              0x7FFFU,
              0x7FFFU,
              0x7FFFU,
              0xFFFFU,
              100U,
              0xFFFFFFFFU,
              1U,
              0U,
              0U,
              0U,
              0U,
              0U,
              0U
          },
          PartyValueCase{
              100U,
              0xFFFFU,
              100U,
              0xFFFFU,
              100U,
              0xFFFFU,
              0U,
              0U,
              0U,
              1U,
              0U,
              0U,
              0U,
              6U
          },
          PartyValueCase{
              100U,
              200U,
              100U,
              50U,
              100U,
              50U,
              0U,
              1U,
              1U,
              0U,
              100U,
              50U,
              50U,
              0U
          },
          PartyValueCase{
              1U,
              100U,
              0U,
              100U,
              0U,
              100U,
              1U,
              0xFFFFFFFFU,
              0xFFFFFFFFU,
              1U,
              0U,
              0U,
              0U,
              6U
          }}) {
        auto fixture = std::make_unique<Fixture>();
        auto& party = fixture->startup.party[0U];
        auto& configuration = party.configuration;
        configuration.source_runtime_value = 1U;
        configuration.actor_record_token = 0x78001000U;
        configuration.actor_record[1U] = sample.hp | (sample.mp << 16U);
        configuration.actor_record[2U] = sample.sp | (sample.hp_max << 16U);
        configuration.actor_record[3U] = sample.mp_max | (sample.sp_max << 16U);
        fixture->action.group_a_action_execution[0U]
            .action_twenty_seven_motion_mode = sample.ai;
        RecordPort port(*fixture);
        port.records.emplace(
            0x78001000U,
            std::as_writable_bytes(std::span{configuration.actor_record})
        );
        const auto result = port.invoke_debug_hotkey({
            .call = LegacyBattleDebugHotkeyCall::publish_actor_value,
            .object_token = 0x005029D0U,
            .arguments = {sample.damage, sample.mp_delta, sample.sp_delta},
        });
        test.expect_true(
            !result.typed_stop &&
                result.eax == (sample.expected_hp == 0U ? 1U : 0U) &&
                configuration.actor_record[1U] ==
                    (sample.expected_hp | (sample.expected_mp << 16U)) &&
                (configuration.actor_record[2U] & 0xFFFFU) ==
                    sample.expected_sp &&
                party.base_initialization.field_2a94 ==
                    sample.expected_death_byte &&
                fixture->random.calls == 0U,
            "party values preserve signed WORD bounds, overflow-to-death and AI death byte six"
        );
    }

    {
        auto fixture = std::make_unique<Fixture>();
        auto& configuration = fixture->startup.party[0U].configuration;
        auto& peer = fixture->startup.party[1U].configuration.actor_record;
        configuration.source_runtime_value = 1U;
        configuration.actor_record_token = 0x78001000U;
        configuration.actor_record[1U] = 5U | (5U << 16U);
        peer[1U] = 200U | (20U << 16U);
        peer[2U] = 20U | (100U << 16U);
        peer[3U] = 30U | (30U << 16U);
        RecordPort port(*fixture);
        port.records.emplace(
            0x78001000U,
            std::as_writable_bytes(std::span{configuration.actor_record})
        );
        port.records.emplace(
            0x78002000U, std::as_writable_bytes(std::span{peer})
        );
        port.before_access = [&](const u32 access) {
            if (access == 1U) {
                configuration.actor_record_token = 0x78002000U;
            }
        };
        const auto result = port.invoke_debug_hotkey({
            .call = LegacyBattleDebugHotkeyCall::publish_actor_value,
            .object_token = 0x005029D0U,
            .arguments = {10U, 0xFFFFFFF6U, 0xFFFFFFF6U},
        });
        test.expect_true(
            !result.typed_stop && result.eax == 0U &&
                configuration.actor_record[1U] == 0x0005FFFBU &&
                peer[1U] == (100U | (10U << 16U)) &&
                (peer[2U] & 0xFFFFU) == 10U,
            "RMW retains its loaded reference and the next original load observes the changed actor record"
        );

        configuration.actor_record_token = 0x78001000U;
        configuration.actor_record[2U] = 5U | (123U << 16U);
        peer[1U] = 55U | (5U << 16U);
        peer[3U] = 456U | (789U << 16U);
        port.accesses = 0U;
        const auto reset = port.invoke_debug_hotkey({
            .call = LegacyBattleDebugHotkeyCall::reset_group_a_primary,
            .arguments = {0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU},
        });
        test.expect_true(
            !reset.typed_stop && port.accesses == 6U &&
                configuration.actor_record[1U] == (123U | (5U << 16U)) &&
                peer[1U] == (55U | (456U << 16U)) &&
                (peer[2U] & 0xFFFFU) == 789U,
            "minus-one reset retains the HP reference then reloads the MP and SP references"
        );
    }

    {
        auto fixture = std::make_unique<Fixture>();
        auto& configuration = fixture->startup.party[0U].configuration;
        configuration.source_runtime_value = 1U;
        configuration.source_record_token = 0x004AB790U;
        std::array<u32, 14> source;
        source.fill(0xAABBCCDDU);
        RecordPort port(*fixture);
        port.records.emplace(
            0x004AB790U, std::as_writable_bytes(std::span{source}).first(0x28U)
        );
        const auto result = port.invoke_debug_hotkey({
            .call = LegacyBattleDebugHotkeyCall::configure_group_a,
            .arguments = {9900U, 155U, 200U},
        });
        test.expect_true(
            result.typed_stop && port.accesses == 2U &&
                source[9U] == 0x26ACCCDDU && source[10U] == 0xAABBCCDDU &&
                source[5U] == 0xAABBCCDDU,
            "short base configuration keeps the first WORD store and stops before later fields"
        );

        port.accesses = 0U;
        const auto absent = port.invoke_debug_hotkey({
            .call = LegacyBattleDebugHotkeyCall::reset_group_a_primary,
            .arguments = {0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU},
        });
        test.expect_true(
            !absent.typed_stop && port.accesses == 0U,
            "the original null party record skips reset without a mapped access"
        );
    }

    // 47F1D0..47F246: three RMWs, signed bounds, then death publication.
    for (u32 stop_at = 0U; stop_at <= 15U; ++stop_at) {
        auto fixture = std::make_unique<Fixture>();
        auto& configuration = fixture->startup.party[0U].configuration;
        configuration.source_runtime_value = 1U;
        configuration.actor_record_token = 0x78001000U;
        configuration.actor_record[1U] = 5U | (5U << 16U);
        configuration.actor_record[2U] = 5U | (100U << 16U);
        configuration.actor_record[3U] = 10U | (10U << 16U);
        RecordPort port(*fixture);
        port.stop_at = stop_at;
        port.records.emplace(
            0x78001000U,
            std::as_writable_bytes(std::span{configuration.actor_record})
        );
        const auto result = port.invoke_debug_hotkey({
            .call = LegacyBattleDebugHotkeyCall::publish_actor_value,
            .object_token = 0x005029D0U,
            .arguments = {10U, 0xFFFFFFF6U, 0xFFFFFFF6U},
        });
        const auto after = [=](const u32 access) {
            return stop_at == 0U || stop_at > access;
        };
        const u32 hp = after(15U) ? 0U : (after(1U) ? 0xFFFBU : 5U);
        const u32 mp = after(11U) ? 0U : (after(4U) ? 0xFFFBU : 5U);
        const u32 sp = after(13U) ? 0U : (after(7U) ? 0xFFFBU : 5U);
        test.expect_true(
            result.typed_stop == (stop_at != 0U) &&
                port.accesses == (stop_at == 0U ? 15U : stop_at) &&
                configuration.actor_record[1U] == (hp | (mp << 16U)) &&
                (configuration.actor_record[2U] & 0xFFFFU) == sp &&
                fixture->startup.party[0U].progress.presentation_enabled ==
                    (stop_at == 0U ? 1U : 0U) &&
                fixture->random.calls == 0U &&
                (stop_at != 0U || result.eax == 1U),
            "actor values retain each original record-write prefix before a mapped access stops"
        );
    }

    struct SharedDamageCase {
        u32 gate;
        u32 currency;
        u32 hp;
        u32 remaining;
    };
    for (const auto sample :
         {SharedDamageCase{1U, 5U, 100U, 0xFFFFFFB5U},
          SharedDamageCase{1U, 80U, 100U, 0U},
          SharedDamageCase{1U, 0U, 20U, 0U},
          SharedDamageCase{2U, 5U, 20U, 5U}}) {
        auto fixture = std::make_unique<Fixture>();
        auto& configuration = fixture->startup.party[0U].configuration;
        configuration.source_runtime_value = 1U;
        configuration.actor_record_token = 0x78001000U;
        configuration.actor_record[1U] = 100U | (20U << 16U);
        configuration.actor_record[2U] = 20U | (100U << 16U);
        configuration.actor_record[3U] = 30U | (30U << 16U);
        const auto actor =
            openswd3::battle::resolve_legacy_battle_actor_runtime_reset(
                {.action = &fixture->action, .startup = &fixture->startup},
                0x005029D0U
            );
        actor.residual->field_2b18 = sample.gate;
        actor.shared_action->decimal_value =
            static_cast<openswd3::compat::i32>(sample.currency);
        RecordPort port(*fixture);
        port.records.emplace(
            0x78001000U,
            std::as_writable_bytes(std::span{configuration.actor_record})
        );
        const auto result = port.invoke_debug_hotkey({
            .call = LegacyBattleDebugHotkeyCall::publish_actor_value,
            .object_token = 0x005029D0U,
            .arguments = {80U, 0xFFFFFFF6U, 0xFFFFFFF6U},
        });
        test.expect_true(
            !result.typed_stop && result.eax == 0U &&
                configuration.actor_record[1U] == (sample.hp | (10U << 16U)) &&
                (configuration.actor_record[2U] & 0xFFFFU) == 10U &&
                std::bit_cast<u32>(actor.shared_action->decimal_value) ==
                    sample.remaining,
            "shared damage uses unsigned nonzero currency and wrapping subtraction before MP/SP"
        );
    }

    struct GuardCase {
        u32 damage;
        u32 mode;
        u32 flags;
        u32 ready;
        u32 status;
        bool blocked;
    };
    for (const auto sample :
         {GuardCase{80U, 0x8000U, 0U, 0U, 0U, true},
          GuardCase{0U, 0x8000U, 0U, 0U, 0U, false},
          GuardCase{0xFFFFFFFFU, 0x8000U, 0U, 0U, 0U, false},
          GuardCase{80U, 0U, 0x02000000U, 0U, 0U, true},
          GuardCase{80U, 0U, 0U, 1U, 0U, true},
          GuardCase{80U, 0U, 0U, 2U, 0U, false},
          GuardCase{80U, 0U, 0U, 0U, 8U, true},
          GuardCase{80U, 0U, 0U, 0U, 0x800U, false}}) {
        auto fixture = std::make_unique<Fixture>();
        auto& party = fixture->startup.party[0U];
        party.configuration.source_runtime_value = 1U;
        party.progress.mode_gate = sample.mode;
        party.progress.special_ready = sample.ready;
        party.attribute_aggregation.embedded_profile_application.status_bits =
            sample.status;
        fixture->action.group_a_action_execution[0U].field_26c0 = sample.flags;
        RecordPort port(*fixture);
        const auto result = port.invoke_debug_hotkey({
            .call = LegacyBattleDebugHotkeyCall::publish_actor_value,
            .object_token = 0x005029D0U,
            .arguments = {sample.damage, 0U, 0U},
        });
        test.expect_true(
            result.typed_stop != sample.blocked &&
                port.accesses == (sample.blocked ? 0U : 1U) &&
                (result.typed_stop || result.eax == 0U),
            "damage gates preserve signed and exact comparisons before touching an invalid record"
        );
    }

    struct EnemyValueCase {
        u32 metric;
        u32 maximum;
        u32 hp;
        u32 damage;
        u32 next_metric;
        u32 next_hp;
        bool dead;
    };
    for (const auto sample :
         {EnemyValueCase{100U, 100U, 50U, 10U, 90U, 50U, false},
          EnemyValueCase{5U, 100U, 50U, 10U, 0U, 0U, true},
          EnemyValueCase{0xFFFFFFFFU, 100U, 50U, 10U, 0U, 0U, true},
          EnemyValueCase{100U, 0xFFFFFFFFU, 50U, 10U, 0U, 0U, true},
          EnemyValueCase{0U, 100U, 100U, 10U, 0U, 90U, false},
          EnemyValueCase{0U, 100U, 5U, 10U, 0U, 0U, true},
          EnemyValueCase{0U, 100U, 0x7FFFU, 0xFFFFFFFFU, 0U, 100U, false},
          EnemyValueCase{0U, 0xFFFFFFFFU, 5U, 0U, 0U, 0U, true},
          EnemyValueCase{0U, 100U, 0xFFFFU, 0U, 0U, 0U, true}}) {
        for (const u32 previous : {0U, 7U}) {
            auto fixture = std::make_unique<Fixture>();
            auto& enemy = (*fixture->startup.group_b_lifecycle)[0U];
            enemy.action_configuration.source_runtime_value = 2U;
            enemy.action_configuration.timing_value = sample.metric;
            enemy.resource_token = 0x78002000U;
            for (std::size_t i = 0U; i < 4U; ++i) {
                enemy.resource_bytes[0x4CU + i] =
                    static_cast<openswd3::compat::u8>(
                        sample.maximum >> (i * 8U)
                    );
            }

            enemy.resource_bytes[0x64U] =
                static_cast<openswd3::compat::u8>(sample.hp);
            enemy.resource_bytes[0x65U] =
                static_cast<openswd3::compat::u8>(sample.hp >> 8U);
            const auto actor =
                openswd3::battle::resolve_legacy_battle_actor_runtime_reset(
                    {.action = &fixture->action, .startup = &fixture->startup},
                    0x00525508U
                );
            actor.base_initialization->field_2a94 =
                static_cast<openswd3::compat::u8>(previous);
            actor.action_execution->turn_threshold = 77U;
            RecordPort port(*fixture);
            port.records.emplace(
                0x78002000U,
                std::as_writable_bytes(std::span{enemy.resource_bytes})
            );
            const auto result = port.invoke_debug_hotkey({
                .call = LegacyBattleDebugHotkeyCall::publish_actor_value,
                .object_token = 0x00525508U,
                .arguments = {sample.damage, 0xFFFFFFFFU, 0xFFFFFFFFU},
            });
            const u32 hp = enemy.resource_bytes[0x64U] |
                (static_cast<u32>(enemy.resource_bytes[0x65U]) << 8U);
            test.expect_true(
                !result.typed_stop && result.eax == (sample.dead ? 1U : 0U) &&
                    enemy.action_configuration.timing_value ==
                        sample.next_metric &&
                    hp == sample.next_hp &&
                    actor.progress->presentation_enabled ==
                        (sample.dead ? 1U : 0U) &&
                    enemy.action_configuration.presentation_enabled ==
                        (sample.dead ? 1U : 0U) &&
                    actor.action_execution->turn_threshold ==
                        (sample.dead ? 0U : 77U) &&
                    fixture->random.calls ==
                        (sample.dead && previous == 0U ? 1U : 0U) &&
                    actor.base_initialization->field_2a94 ==
                        (sample.dead && previous == 0U ? 1U : previous),
                "enemy values preserve DWORD/WORD bounds, negative-WORD restoration and conditional death random"
            );
        }
    }

    for (u32 stop_at = 0U; stop_at <= 11U; ++stop_at) {
        auto fixture = std::make_unique<Fixture>();
        fixture->actor_metrics.group_a_count = 1U;
        auto& configuration = fixture->startup.party[0U].configuration;
        configuration.source_runtime_value = 1U;
        configuration.actor_record_token = 0x78001000U;
        configuration.source_record_token = 0x004AB790U;
        configuration.auxiliary_record_token = 0x004ACF50U;
        configuration.actor_record[1U] = 100U | (200U << 16U);
        configuration.actor_record[2U] = 300U | (1000U << 16U);
        configuration.actor_record[3U] = 500U | (600U << 16U);
        std::array<u32, 14> source{};
        source[9U] = 0xAAAA5555U;
        source[10U] = 0xBBBB1234U;
        source[5U] = 0xCCCC7777U;
        auto& auxiliary = fixture->startup.group_a_auxiliary_sources[0U];
        auxiliary.dwords[0U] = 3U;
        RecordPort port(*fixture);
        port.stop_at = stop_at;
        port.records.emplace(
            0x78001000U,
            std::as_writable_bytes(std::span{configuration.actor_record})
        );
        port.records.emplace(
            0x004AB790U, std::as_writable_bytes(std::span{source})
        );
        port.records.emplace(
            0x004ACF50U, std::as_writable_bytes(std::span{&auxiliary, 1U})
        );
        LegacyBattleDebugHotkeyState state;
        state.developer_tools_enabled = 1U;
        openswd3::input_time_rng::LegacyKeyboardSnapshot keyboard{};
        press(keyboard, 0x1DU);
        press(keyboard, 0x2CU);
        const auto result =
            openswd3::battle::coordinate_legacy_battle_debug_hotkeys(
                keyboard, state, fixture->bindings(), port
            );
        const auto written = [=](const u32 access) {
            return stop_at == 0U || stop_at > access;
        };
        test.expect_true(
            result.status ==
                    (stop_at == 0U ? LegacyBattleDebugHotkeyStatus::completed
                                   : LegacyBattleDebugHotkeyStatus::
                                         port_call_typed_stop) &&
                port.accesses == (stop_at == 0U ? 11U : stop_at) &&
                (configuration.actor_record[1U] & 0xFFFFU) ==
                    (written(2U) ? 1000U : 100U) &&
                (configuration.actor_record[1U] >> 16U) ==
                    (written(4U) ? 500U : 200U) &&
                (configuration.actor_record[2U] & 0xFFFFU) ==
                    (written(6U) ? 600U : 300U) &&
                auxiliary.dwords[0U] == (written(7U) ? 56U : 3U) &&
                source[9U] == (written(9U) ? 0x26AC5555U : 0xAAAA5555U) &&
                source[10U] == (written(10U) ? 0xBBBB009BU : 0xBBBB1234U) &&
                source[5U] == (written(11U) ? 0x00C87777U : 0xCCCC7777U),
            "Control Z writes live, auxiliary and base records in LST order and preserves each failed prefix"
        );
    }

    {
        auto fixture = std::make_unique<Fixture>();
        auto& configuration = fixture->startup.party[0U].configuration;
        configuration.source_runtime_value = 2U;
        configuration.profile_token = 0x78002000U;
        std::array<u32, 41> mon;
        mon.fill(0xAABBCCDDU);
        RecordPort port(*fixture);
        port.records.emplace(
            0x78002000U, std::as_writable_bytes(std::span{mon})
        );
        const auto restored = port.invoke_debug_hotkey({
            .call = LegacyBattleDebugHotkeyCall::reset_group_a_primary,
            .arguments = {0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU},
        });
        const auto configured = port.invoke_debug_hotkey({
            .call = LegacyBattleDebugHotkeyCall::configure_group_a,
            .arguments = {9900U, 155U, 200U},
        });
        test.expect_true(
            !restored.typed_stop && !configured.typed_stop &&
                mon[19U] == 0xFFFFFFFFU && mon[25U] == 0xAABBFFFFU &&
                mon[21U] == 0x26ACCCDDU && mon[22U] == 0x00C8009BU,
            "non-party Z calls preserve signed minus one and the three MON WORD writes"
        );

        configuration.profile_token = 0U;
        port.accesses = 0U;
        const auto absent = port.invoke_debug_hotkey({
            .call = LegacyBattleDebugHotkeyCall::reset_group_a_primary,
            .arguments = {0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU},
        });
        test.expect_true(
            !absent.typed_stop && port.accesses == 0U,
            "the original nullable MON reset reference remains a no-op"
        );
    }

    for (const auto gates :
         {std::array<u32, 2>{1U, 0U},
          std::array<u32, 2>{0U, 1U},
          std::array<u32, 2>{2U, 2U}}) {
        auto fixture = std::make_unique<Fixture>();
        auto& configuration = fixture->startup.party[0U].configuration;
        configuration.auxiliary_record_token = 0x004ACF50U;
        configuration.source_runtime_value_read_accessible = false;
        fixture->action.group_a_action_execution[0U]
            .action_twenty_seven_motion_mode = gates[0U];
        fixture->startup.party[0U].progress.scene_identity = gates[1U];
        auto& auxiliary = fixture->startup.group_a_auxiliary_sources[0U];
        auxiliary.dwords[0U] = 7U;
        RecordPort port(*fixture);
        port.records.emplace(
            0x004ACF50U, std::as_writable_bytes(std::span{&auxiliary, 1U})
        );
        const auto result = port.invoke_debug_hotkey({
            .call = LegacyBattleDebugHotkeyCall::reset_group_a_secondary,
            .arguments = {0xFFFFFFFFU},
        });
        const bool skipped = gates[0U] == 1U || gates[1U] == 1U;
        test.expect_true(
            !result.typed_stop && port.accesses == (skipped ? 0U : 2U) &&
                auxiliary.dwords[0U] == (skipped ? 7U : 56U),
            "secondary reset checks exact AI gates without reading actor kind"
        );
    }

    for (const auto gates :
         {std::array<u32, 2>{1U, 0U},
          std::array<u32, 2>{0U, 1U},
          std::array<u32, 2>{2U, 2U}}) {
        auto fixture = std::make_unique<Fixture>();
        fixture->actor_metrics.group_a_count = 1U;
        fixture->action.group_a_action_execution[0U]
            .action_twenty_seven_motion_mode = gates[0U];
        fixture->startup.party[0U].progress.scene_identity = gates[1U];
        LegacyBattleDebugHotkeyState state;
        state.developer_tools_enabled = 1U;
        DebugPort port;
        openswd3::input_time_rng::LegacyKeyboardSnapshot keyboard{};
        for (const u32 key : {0x1DU, 0x20U, 0x21U}) {
            press(keyboard, key);
        }

        const auto result =
            openswd3::battle::coordinate_legacy_battle_debug_hotkeys(
                keyboard, state, fixture->bindings(false), port
            );
        const bool skipped = gates[0U] == 1U || gates[1U] == 1U;
        test.expect_true(
            result.status == LegacyBattleDebugHotkeyStatus::completed &&
                result.port_calls == (skipped ? 0U : 2U) &&
                port.delays == std::vector<u32>{200U, 100U},
            "D and F read actual actor AI fields without requiring the frame copy"
        );
    }

    struct TextCase {
        u32 token;
        std::array<openswd3::compat::u8, 9> bytes;
        std::size_t size;
    };

    constexpr std::array text_cases{
        TextCase{
            0x004A7820U,
            {0xB5U, 0xB4U, 0xB9U, 0xEFU, 0xC6U, 0x46U, 0xABU, 0xB4U, 0U},
            9U
        },
        TextCase{
            0x004A782CU,
            {0xA5U, 0xBFU, 0xB1U, 0x60U, 0xC6U, 0x46U, 0xABU, 0xB4U, 0U},
            9U
        },
        TextCase{
            0x004A7838U, {0xB1U, 0x6AU, 0xA7U, 0xF0U, 0xC0U, 0xBBU, 0U}, 7U
        },
    };
    for (const auto& entry : text_cases) {
        const auto bytes =
            openswd3::battle::legacy_battle_debug_text_bytes(entry.token);
        test.expect_true(
            bytes.size() == entry.size &&
                std::equal(bytes.begin(), bytes.end(), entry.bytes.begin()),
            "fixed debug text preserves the original encoded bytes and terminator"
        );
    }

    test.expect_true(
        openswd3::battle::legacy_battle_debug_text_bytes(0U).empty() &&
            openswd3::battle::legacy_battle_debug_text_bytes(0x004A7821U)
                .empty(),
        "unknown debug text tokens are not replaced by an empty string"
    );

    struct StopCase {
        u32 key;
        LegacyBattleDebugHotkeyCall call;
        std::size_t calls;
    };

    constexpr std::array stop_cases{
        StopCase{0x3DU, LegacyBattleDebugHotkeyCall::suspend_audio_output, 1U},
        StopCase{0x2CU, LegacyBattleDebugHotkeyCall::reset_group_a_primary, 1U},
        StopCase{
            0x2CU, LegacyBattleDebugHotkeyCall::reset_group_a_secondary, 2U
        },
        StopCase{0x2CU, LegacyBattleDebugHotkeyCall::configure_group_a, 3U},
        StopCase{0x20U, LegacyBattleDebugHotkeyCall::publish_actor_value, 1U},
        StopCase{0x21U, LegacyBattleDebugHotkeyCall::publish_actor_value, 1U},
        StopCase{0x2FU, LegacyBattleDebugHotkeyCall::publish_actor_value, 1U},
        StopCase{0x3FU, LegacyBattleDebugHotkeyCall::suspend_audio_output, 1U},
        StopCase{0x3FU, LegacyBattleDebugHotkeyCall::restart_battle_music, 2U},
        StopCase{0x11U, LegacyBattleDebugHotkeyCall::query_actor_status, 1U},
        StopCase{0x11U, LegacyBattleDebugHotkeyCall::publish_actor_value, 2U},
        StopCase{0x25U, LegacyBattleDebugHotkeyCall::text_message_allocate, 1U},
        StopCase{0x25U, LegacyBattleDebugHotkeyCall::text_message_measure, 2U},
        StopCase{0x3CU, LegacyBattleDebugHotkeyCall::text_message_allocate, 1U},
        StopCase{0x3CU, LegacyBattleDebugHotkeyCall::text_message_measure, 2U},
    };
    for (const auto& entry : stop_cases) {
        auto fixture = std::make_unique<Fixture>();
        fixture->actor_metrics.group_a_count = 1U;
        fixture->actor_metrics.group_b_count = 1U;
        fixture->actor_publication.slots[0U] = 91U;
        fixture->startup.reset.block_5242b0[0U] = 92U;
        const u32 shared_gate = 1U;
        auto bindings = fixture->bindings();
        bindings.developer_tools_enabled = &shared_gate;
        LegacyBattleDebugHotkeyState state;
        state.screenshot_request = 7U;
        DebugPort port;
        port.stop_call = entry.call;
        openswd3::input_time_rng::LegacyKeyboardSnapshot keyboard{};
        for (const u32 key : {0x1DU, 0x3BU, entry.key, 0x19U}) {
            press(keyboard, key);
        }

        const auto result =
            openswd3::battle::coordinate_legacy_battle_debug_hotkeys(
                keyboard, state, bindings, port
            );
        test.expect_true(
            result.status ==
                    LegacyBattleDebugHotkeyStatus::port_call_typed_stop &&
                result.stopped_call == entry.call &&
                result.port_calls == entry.calls &&
                port.calls.size() == entry.calls &&
                state.toggle_5244e0 == (entry.key == 0x3DU ? 0U : 1U) &&
                state.screenshot_request == 7U && !result.full_reset_applied,
            "unreturned debug calls keep the preceding key writes and suppress the suffix"
        );
        if (entry.key == 0x11U) {
            const bool published =
                entry.call == LegacyBattleDebugHotkeyCall::publish_actor_value;
            test.expect_true(
                fixture->actor_publication.slots[0U] ==
                        (published ? 0U : 91U) &&
                    fixture->startup.reset.block_5242b0[0U] ==
                        (published ? 0U : 92U),
                "W preserves publication only when its status query returned"
            );
        }

        if (entry.key == 0x25U || entry.key == 0x3CU) {
            const bool allocated =
                entry.call == LegacyBattleDebugHotkeyCall::text_message_measure;
            test.expect_true(
                fixture->startup.text_messages.allocations.size() ==
                        (allocated ? 1U : 0U) &&
                    fixture->startup.reset.block_5214f8[0U] == 0U &&
                    (entry.key == 0x25U ? state.message_latch_53ceb8 == 1U
                                        : state.battle_mode_flags_53bc24 == 2U),
                "debug text call failures keep latch and allocation prefixes without appending"
            );
            if (allocated) {
                const auto& record =
                    fixture->startup.text_messages.allocations[0U].record;
                test.expect_true(
                    record.value_04 == 0x208U && record.value_08 == 10U &&
                        record.kind == 30U && record.flags == 0U &&
                        record.text_length == 0U,
                    "failed text measurement keeps fields written before strlen"
                );
            }
        }
    }

    for (const u32 shared_gate : {0U, 1U, 2U, 0xFFFFFFFFU}) {
        auto fixture = std::make_unique<Fixture>();
        fixture->actor_metrics.group_a_count = 1U;
        fixture->actor_metrics.group_b_count = 1U;
        fixture->startup.party[0U].position_x = 0xFFF9U;
        (*fixture->startup.group_b_lifecycle)[0U].action_execution.position_x =
            5U;
        auto bindings = fixture->bindings();
        bindings.developer_tools_enabled = &shared_gate;
        LegacyBattleDebugHotkeyState state;
        state.developer_tools_enabled = shared_gate == 1U ? 0U : 1U;
        state.toggle_5244e0 = 9U;
        state.toggle_53af68 = 2U;
        state.screenshot_request = 7U;
        DebugPort port;
        openswd3::input_time_rng::LegacyKeyboardSnapshot keyboard{};
        for (const u32 key :
             {0x1DU, 0x3BU, 0x2DU, 0x43U, 0x23U, 0x24U, 0x19U}) {
            press(keyboard, key);
        }

        const auto result =
            openswd3::battle::coordinate_legacy_battle_debug_hotkeys(
                keyboard,
                state,
                bindings,
                port,
                {.actor_adjustment_entry_edx = 0xFACE1234U,
                 .actor_adjustment_entry_edx_known = false}
            );
        const bool enabled = shared_gate == 1U;
        test.expect_true(
            result.status == LegacyBattleDebugHotkeyStatus::completed &&
                result.return_value == 1U &&
                result.raw_key_queries == (enabled ? 18U : 1U) &&
                !result.actor_coordinate_registers_known &&
                (!enabled ||
                 result.actor_coordinate_adjustment.return_edx ==
                     0xFACE0000U) &&
                port.calls.empty() &&
                port.delays ==
                    (enabled ? std::vector<u32>{200U, 200U, 200U}
                             : std::vector<u32>{}) &&
                state.toggle_5244e0 == (enabled ? 0U : 9U) &&
                state.toggle_53af68 == (enabled ? 0U : 2U) &&
                fixture->player_control.speed_mode == (enabled ? 1U : 0U) &&
                fixture->effect_shift.actor_delta == (enabled ? -10 : 0) &&
                fixture->startup.party[0U].position_x == 0xFFF9U &&
                (*fixture->startup.group_b_lifecycle)[0U]
                        .action_execution.position_x == 5U &&
                state.screenshot_request == 1U,
            "live shared debug gate controls canonical coordinates and switches with P outside the gate"
        );
    }

    {
        auto fixture = std::make_unique<Fixture>();
        LegacyBattleDebugHotkeyState state;
        state.developer_tools_enabled = 1U;
        LegacyBattleDebugHotkeyPort port;
        openswd3::input_time_rng::LegacyKeyboardSnapshot keyboard{};
        press(keyboard, 0x1DU);
        press(keyboard, 0x3DU);
        const auto result =
            openswd3::battle::coordinate_legacy_battle_debug_hotkeys(
                keyboard, state, fixture->bindings(), port
            );
        test.expect_true(
            result.status ==
                    LegacyBattleDebugHotkeyStatus::port_call_typed_stop &&
                result.port_calls == 1U,
            "an unbound production debug port stops at the first actual call"
        );
    }

    {
        Fixture fixture;
        fixture.actor_metrics.group_a_count = 1U;
        fixture.startup.party[0].position_x = 9U;
        LegacyBattleDebugHotkeyState state;
        state.screenshot_request = 7U;
        DebugPort port;
        openswd3::input_time_rng::LegacyKeyboardSnapshot keyboard{};
        press(keyboard, 0x23U);
        press(keyboard, 0x19U);
        const auto result =
            openswd3::battle::coordinate_legacy_battle_debug_hotkeys(
                keyboard, state, fixture.bindings(), port
            );
        test.expect_true(
            result.return_value == 1U && result.raw_key_queries == 1U &&
                result.port_calls == 0U &&
                result.actor_coordinate_adjustment_calls == 0U &&
                fixture.startup.party[0].position_x == 9U &&
                fixture.effect_shift.actor_delta == 0 &&
                state.screenshot_request == 1U && port.calls.empty(),
            "disabled developer tools skip control H and J while retaining the P tail"
        );
    }

    {
        Fixture fixture;
        fixture.actor_metrics.group_a_count = 2U;
        fixture.actor_metrics.group_b_count = 1U;
        fixture.startup.party[0].position_x = 0xFFFBU;
        fixture.startup.party[0].position_y = 101U;
        fixture.startup.party[1].position_x = 20U;
        fixture.startup.party[1].position_y = 202U;
        auto& group_b =
            (*fixture.startup.group_b_lifecycle)[0].action_execution;
        group_b.position_x = 30U;
        group_b.position_y = 303U;
        LegacyBattleDebugHotkeyState state;
        state.developer_tools_enabled = 1U;
        state.screenshot_request = 7U;
        DebugPort port;
        openswd3::input_time_rng::LegacyKeyboardSnapshot keyboard{};
        press(keyboard, 0x23U);
        press(keyboard, 0x19U);
        const auto result =
            openswd3::battle::coordinate_legacy_battle_debug_hotkeys(
                keyboard,
                state,
                fixture.bindings(),
                port,
                {.actor_adjustment_entry_edx = 0xABCD1234U}
            );
        test.expect_true(
            result.status == LegacyBattleDebugHotkeyStatus::completed &&
                result.return_value == 1U && result.raw_key_queries == 5U &&
                result.actor_adjust_iterations == 3U &&
                result.actor_coordinate_adjustment_calls == 3U &&
                fixture.startup.party[0].position_x == 5U &&
                fixture.startup.party[0].position_y == 101U &&
                fixture.startup.party[1].position_x == 30U &&
                fixture.startup.party[1].position_y == 202U &&
                group_b.position_x == 40U && group_b.position_y == 303U &&
                fixture.effect_shift.actor_delta == 10 &&
                state.screenshot_request == 1U &&
                result.actor_coordinate_adjustment.return_eax == 10U &&
                result.actor_coordinate_adjustment.return_ecx == 0x00525508U &&
                result.actor_coordinate_adjustment.return_edx == 0xABCD0000U &&
                port.count(
                    LegacyBattleDebugHotkeyCall::reserved_adjust_actor_slot
                ) == 0U,
            "enabled H updates canonical group-A then group-B coordinates before P without an opaque call"
        );
    }

    {
        Fixture fixture;
        LegacyBattleDebugHotkeyState state;
        state.developer_tools_enabled = 1U;
        DebugPort port;
        openswd3::input_time_rng::LegacyKeyboardSnapshot keyboard{};
        press(keyboard, 0x1DU);
        press(keyboard, 0x12U);
        press(keyboard, 0x23U);
        const auto result =
            openswd3::battle::coordinate_legacy_battle_debug_hotkeys(
                keyboard, state, fixture.bindings(), port
            );
        test.expect_true(
            result.return_value == 0U && result.early_return_zero &&
                result.control_chord_active && result.raw_key_queries == 11U &&
                result.actor_adjust_iterations == 0U,
            "control plus E returns zero before C and suppresses the H J P tail"
        );
    }

    {
        Fixture fixture;
        fixture.player_control.speed_mode = 0xFFFFFFFFU;
        LegacyBattleDebugHotkeyState state;
        state.developer_tools_enabled = 1U;
        state.toggle_5244e0 = 0U;
        state.toggle_53af68 = 4U;
        state.battle_mode_flags_53bc24 = 0xABCD0000U;
        DebugPort port;
        openswd3::input_time_rng::LegacyKeyboardSnapshot keyboard{};
        for (const u32 key : {0x1DU, 0x3BU, 0x2DU, 0x25U, 0x43U, 0x3CU}) {
            press(keyboard, key);
        }
        const auto result =
            openswd3::battle::coordinate_legacy_battle_debug_hotkeys(
                keyboard, state, fixture.bindings(), port
            );
        test.expect_true(
            result.return_value == 1U && state.toggle_5244e0 == 1U &&
                state.toggle_53af68 == 0U && state.message_latch_53ceb8 == 1U &&
                fixture.player_control.speed_mode == 0U &&
                state.text_mode_toggle_53c02c == 1U &&
                state.battle_mode_flags_53bc24 == 0xABCD0002U &&
                result.delay_calls == 4U &&
                port.delays == std::vector<u32>{200U, 200U, 200U, 200U} &&
                result.text_message_calls == 2U &&
                fixture.startup.reset.block_5214f8[0U] == 0x78000000U &&
                fixture.startup.text_messages.allocations.size() == 2U,
            "control toggles preserve blocking delays speed signed modulo and low-byte text bit updates"
        );
    }

    {
        Fixture fixture;
        fixture.actor_metrics.group_a_count = 2U;
        fixture.actor_metrics.group_b_count = 1U;
        fixture.action.group_a_action_execution[1U]
            .action_twenty_seven_motion_mode = 1U;
        fixture.actor_frames.shared.actor_ai_primary.fill(1U);
        LegacyBattleDebugHotkeyState state;
        state.developer_tools_enabled = 1U;
        DebugPort port;
        openswd3::input_time_rng::LegacyKeyboardSnapshot keyboard{};
        for (const u32 key : {0x1DU, 0x2CU, 0x20U, 0x21U, 0x2FU}) {
            press(keyboard, key);
        }
        const auto result =
            openswd3::battle::coordinate_legacy_battle_debug_hotkeys(
                keyboard, state, fixture.bindings(), port
            );
        test.expect_true(
            result.status == LegacyBattleDebugHotkeyStatus::completed &&
                result.delay_calls == 4U &&
                port.count(
                    LegacyBattleDebugHotkeyCall::reset_group_a_primary
                ) == 2U &&
                port.count(
                    LegacyBattleDebugHotkeyCall::reset_group_a_secondary
                ) == 2U &&
                port.count(LegacyBattleDebugHotkeyCall::configure_group_a) ==
                    2U &&
                port.count(LegacyBattleDebugHotkeyCall::publish_actor_value) ==
                    3U &&
                has_call(
                    port,
                    LegacyBattleDebugHotkeyCall::publish_actor_value,
                    0x005029D0U,
                    0U,
                    80U
                ) &&
                has_call(
                    port,
                    LegacyBattleDebugHotkeyCall::publish_actor_value,
                    0x00525508U,
                    0U,
                    10U
                ),
            "Z D F V preserve dynamic group loops AI skip and actor-specific numeric arguments"
        );
    }

    {
        Fixture fixture;
        fixture.actor_metrics.priority_actor_index = 9U;
        fixture.actor_frames.shared.action_block_gate = 1U;
        LegacyBattleDebugHotkeyState state;
        state.developer_tools_enabled = 1U;
        state.actor_retarget_gate_53bf64 = 1U;
        state.selection_status_word_53c050 = 0xABCD0000U;
        state.special_actor_action_target.action_target = 2U;
        DebugPort port;
        openswd3::input_time_rng::LegacyKeyboardSnapshot keyboard{};
        press(keyboard, 0x1DU);
        press(keyboard, 0x2EU);
        const auto result =
            openswd3::battle::coordinate_legacy_battle_debug_hotkeys(
                keyboard,
                state,
                fixture.bindings(),
                port,
                {.special_action_target_request = {.entry_edx = 0x11223344U}}
            );
        test.expect_true(
            result.status ==
                    LegacyBattleDebugHotkeyStatus::
                        actor_runtime_reset_typed_stop &&
                fixture.action.resolution_latch == 9U &&
                state.selection_status_word_53c050 == 0xABCD0001U &&
                state.actor_retarget_gate_53bf64 == 0U &&
                fixture.action.selection_cache_gate_b == 0U &&
                fixture.action.action_pending_aux == 0U &&
                fixture.final_actor.selection_gate == 0U &&
                fixture.actor_frames.shared.action_block_gate == 1U &&
                fixture.actor_metrics.priority_actor_index == 0xFFFFFFFFU &&
                result.actor_action_target_calls == 1U &&
                result.actor_action_target.return_eax == 2U &&
                result.actor_action_target.return_ecx == 0x004E80FCU &&
                result.actor_action_target.return_edx == 0x11223344U &&
                result.actor_action_target.return_eip == 0x0045DBE8U &&
                result.actor_action_target.field_token == 0x004EAA9EU &&
                result.actor_action_target.flags_known &&
                result.actor_gate_decay.calls == 1U &&
                result.actor_gate_decay.call_addresses[0U] == 0x0045DC03U &&
                result.actor_gate_decay.return_addresses[0U] == 0x0045DC08U &&
                result.actor_gate_decay.actor_tokens[0U] == 0x0052AB58U &&
                result.actor_gate_decay.last.returned &&
                port.count(
                    LegacyBattleDebugHotkeyCall::
                        reserved_query_special_action_target
                ) == 0U &&
                port.count(LegacyBattleDebugHotkeyCall::reset_actor) == 0U &&
                port.calls.empty() && result.actor_runtime_reset.calls == 1U &&
                result.actor_runtime_reset.call_addresses[0U] == 0x0045DC27U &&
                result.actor_runtime_reset.actor_tokens[0U] == 0x004E80FCU &&
                result.actor_runtime_reset.last.status ==
                    openswd3::battle::LegacyBattleActorRuntimeResetStatus::
                        actor_write_typed_stop &&
                result.actor_runtime_reset.last.return_eip == 0x00478856U,
            "C preserves low-word status update typed gate decay and special-actor reset stop ordering"
        );
    }

    {
        Fixture fixture;
        fixture.actor_metrics.priority_actor_index = 9U;
        fixture.actor_frames.shared.action_block_gate = 1U;
        LegacyBattleDebugHotkeyState state;
        state.developer_tools_enabled = 1U;
        state.actor_retarget_gate_53bf64 = 1U;
        state.selection_status_word_53c050 = 0xABCD0000U;
        state.special_actor_action_target.action_target = 2U;
        DebugPort port;
        openswd3::input_time_rng::LegacyKeyboardSnapshot keyboard{};
        press(keyboard, 0x1DU);
        press(keyboard, 0x2EU);
        openswd3::battle::LegacyBattleDebugHotkeyRequest request;
        request.special_action_target_request.entry_edx = 0x11223344U;
        request.actor_gate_decay_requests.count = 1U;
        request.actor_gate_decay_requests.calls[0U].access.start_gate_readable =
            false;
        const auto result =
            openswd3::battle::coordinate_legacy_battle_debug_hotkeys(
                keyboard, state, fixture.bindings(), port, request
            );
        test.expect_true(
            result.status ==
                    LegacyBattleDebugHotkeyStatus::
                        actor_gate_decay_typed_stop &&
                fixture.action.resolution_latch == 9U &&
                state.selection_status_word_53c050 == 0xABCD0001U &&
                state.actor_retarget_gate_53bf64 == 0U &&
                fixture.action.selection_cache_gate_b == 0U &&
                fixture.action.action_pending_aux == 0U &&
                fixture.final_actor.selection_gate == 0U &&
                fixture.actor_frames.shared.action_block_gate == 1U &&
                fixture.actor_metrics.priority_actor_index == 0xFFFFFFFFU &&
                result.actor_action_target_calls == 1U &&
                result.actor_gate_decay.calls == 1U &&
                result.actor_gate_decay.call_addresses[0U] == 0x0045DC03U &&
                result.actor_gate_decay.last.status ==
                    openswd3::battle::LegacyBattleActorGateDecayStatus::
                        start_gate_read_typed_stop &&
                result.actor_runtime_reset.calls == 0U && port.calls.empty(),
            "debug gate-decay read stop preserves the retarget prefix and suppresses actor reset and action-block suffixes"
        );
    }

    {
        Fixture fixture;
        fixture.actor_frames.shared.action_block_gate = 1U;
        LegacyBattleDebugHotkeyState state;
        state.developer_tools_enabled = 1U;
        state.actor_retarget_gate_53bf64 = 1U;
        DebugPort port;
        openswd3::input_time_rng::LegacyKeyboardSnapshot keyboard{};
        press(keyboard, 0x1DU);
        press(keyboard, 0x2EU);
        const auto result =
            openswd3::battle::coordinate_legacy_battle_debug_hotkeys(
                keyboard,
                state,
                fixture.bindings(),
                port,
                {.special_action_target_request = {
                     .access = {.action_target_readable = false},
                 }}
            );
        test.expect_true(
            result.status ==
                    LegacyBattleDebugHotkeyStatus::
                        actor_action_target_typed_stop &&
                result.actor_action_target_calls == 1U &&
                result.actor_action_target.return_eip == 0x004786E0U &&
                result.actor_action_target.action_target_reads == 0U &&
                fixture.actor_frames.shared.action_block_gate == 1U &&
                port.count(LegacyBattleDebugHotkeyCall::reset_actor) == 0U,
            "debug target stop preserves the retarget prefix and suppresses reset and action-block suffixes"
        );
    }

    {
        Fixture fixture;
        LegacyBattleDebugHotkeyState state;
        state.developer_tools_enabled = 1U;
        DebugPort port;
        openswd3::input_time_rng::LegacyKeyboardSnapshot keyboard{};
        press(keyboard, 0x1DU);
        press(keyboard, 0x2EU);
        const auto result =
            openswd3::battle::coordinate_legacy_battle_debug_hotkeys(
                keyboard, state, fixture.bindings(false), port
            );
        test.expect_true(
            result.status ==
                    LegacyBattleDebugHotkeyStatus::
                        actor_frame_state_typed_stop &&
                fixture.actor_metrics.priority_actor_index == 0xFFFFFFFFU &&
                fixture.action.selection_cache_gate_b == 0U &&
                fixture.action.action_pending_aux == 0U &&
                fixture.action.resolution_latch == 9U,
            "missing actor-frame state stops at the original action-block read after C prefix stores"
        );
    }

    {
        Fixture fixture;
        fixture.actor_metrics.group_b_count = 2U;
        fixture.actor_publication.slots.fill(9U);
        fixture.startup.reset.block_5242b0.fill(9U);
        fixture.final_actor.actor_order.fill(9U);
        fixture.message_state = 9U;
        fixture.action.resolution_latch = 9U;
        LegacyBattleDebugHotkeyState state;
        state.developer_tools_enabled = 1U;
        fixture.action.opponent_workspace.fill(9U);
        state.committed_actor_code = 9U;
        for (auto& record : fixture.startup.reset.records_524788) {
            record = {
                .value_00 = 9U,
                .value_04 = 9U,
                .value_08 = 9U,
                .value_0a = 9U,
                .value_0c = 9U,
                .value_10 = 9U,
                .value_14 = 9U,
                .value_18 = 9U,
            };
        }
        DebugPort port;
        openswd3::input_time_rng::LegacyKeyboardSnapshot keyboard{};
        press(keyboard, 0x1DU);
        press(keyboard, 0x11U);
        const auto result =
            openswd3::battle::coordinate_legacy_battle_debug_hotkeys(
                keyboard, state, fixture.bindings(), port
            );
        test.expect_true(
            result.status == LegacyBattleDebugHotkeyStatus::completed &&
                result.full_reset_applied && result.group_b_iterations == 2U &&
                fixture.actor_publication.slots[0] == 0U &&
                fixture.actor_publication.slots[1] == 1U &&
                fixture.startup.reset.block_5242b0[0] == 0U &&
                fixture.effect_coordinator.group_a_render_count == 2U &&
                fixture.effect_coordinator.completed_count == 0U &&
                fixture.effect_coordinator.group_a_feedback_actor == 0xFFFFU &&
                fixture.actor_frames.shared.target_ready_gate == 1U &&
                fixture.action.selection_cache_gate_b == 1U &&
                fixture.action.resolution_latch == 0U &&
                fixture.action.action_pending_aux == 1U &&
                fixture.final_actor.queued_actor_code == 0U &&
                fixture.actor_metrics.priority_actor_index == 0xFFFFFFFFU &&
                fixture.message_state == 0U &&
                std::ranges::all_of(
                    fixture.final_actor.actor_order,
                    [](const auto value) { return value == 0U; }
                ) &&
                std::ranges::all_of(
                    std::span<const u32>{fixture.action.opponent_workspace}
                        .first(10U),
                    [](const auto value) { return value == 0U; }
                ) &&
                fixture.action.opponent_workspace[10U] == 9U &&
                std::ranges::all_of(
                    fixture.startup.reset.records_524788,
                    [](const auto& record) {
                        return record.value_00 == 0xFFFFFFFFU &&
                            record.value_04 == 0U && record.value_08 == 0U &&
                            record.value_0a == 0U && record.value_0c == 0U &&
                            record.value_10 == 0U && record.value_14 == 0U &&
                            record.value_18 == 0U;
                    }
                ),
            "W publishes eligible opponents then resets every fixed workspace in original order"
        );
    }

    {
        Fixture fixture;
        fixture.actor_metrics.group_b_count = 19U;
        LegacyBattleDebugHotkeyState state;
        state.developer_tools_enabled = 1U;
        DebugPort port;
        openswd3::input_time_rng::LegacyKeyboardSnapshot keyboard{};
        press(keyboard, 0x1DU);
        press(keyboard, 0x11U);
        const auto result =
            openswd3::battle::coordinate_legacy_battle_debug_hotkeys(
                keyboard, state, fixture.bindings(), port
            );
        test.expect_true(
            result.status ==
                    LegacyBattleDebugHotkeyStatus::
                        group_b_publication_typed_stop &&
                result.group_b_iterations == 18U &&
                port.count(LegacyBattleDebugHotkeyCall::query_actor_status) ==
                    19U &&
                port.count(LegacyBattleDebugHotkeyCall::publish_actor_value) ==
                    18U &&
                !result.full_reset_applied,
            "the nineteenth W publication stops after its actor query and preserves eighteen completed prefixes"
        );
    }

    {
        Fixture fixture;
        fixture.actor_metrics.group_a_count = 2U;
        fixture.startup.party[0].position_x = 10U;
        fixture.startup.party[1].position_x = 20U;
        fixture.startup.party[1].position_x_write_accessible = false;
        LegacyBattleDebugHotkeyState state;
        state.developer_tools_enabled = 1U;
        DebugPort port;
        openswd3::input_time_rng::LegacyKeyboardSnapshot keyboard{};
        press(keyboard, 0x23U);
        const auto result =
            openswd3::battle::coordinate_legacy_battle_debug_hotkeys(
                keyboard,
                state,
                fixture.bindings(),
                port,
                {.actor_adjustment_entry_edx = 0x98765432U}
            );
        test.expect_true(
            result.status ==
                    LegacyBattleDebugHotkeyStatus::
                        actor_coordinate_adjustment_typed_stop &&
                result.actor_coordinate_adjustment_calls == 2U &&
                result.actor_adjust_iterations == 1U &&
                fixture.startup.party[0].position_x == 20U &&
                fixture.startup.party[1].position_x == 20U &&
                result.actor_coordinate_adjustment.status ==
                    openswd3::battle::
                        LegacyBattleActorCoordinateAdjustmentStatus::
                            position_x_add_typed_stop &&
                result.actor_coordinate_adjustment.return_eax == 10U &&
                result.actor_coordinate_adjustment.return_ecx == 0x00505904U &&
                result.actor_coordinate_adjustment.return_edx == 0x98760000U &&
                result.actor_coordinate_adjustment.flags.carry &&
                result.actor_coordinate_adjustment.flags.parity &&
                result.actor_coordinate_adjustment.flags.auxiliary_carry &&
                result.actor_coordinate_adjustment.flags
                    .auxiliary_carry_defined &&
                !result.actor_coordinate_adjustment.flags.zero &&
                result.actor_coordinate_adjustment.flags.sign &&
                !result.actor_coordinate_adjustment.flags.overflow,
            "the second H actor enters with index-minus-count CMP flags and stops before its X write"
        );
    }

    {
        Fixture fixture;
        fixture.actor_metrics.group_a_count = 1U;
        fixture.startup.party[0].position_x = 20U;
        fixture.effect_shift.actor_delta = 99;
        LegacyBattleDebugHotkeyState state;
        state.developer_tools_enabled = 1U;
        state.screenshot_request = 7U;
        DebugPort port;
        openswd3::input_time_rng::LegacyKeyboardSnapshot keyboard{};
        press(keyboard, 0x23U);
        press(keyboard, 0x19U);
        const auto result =
            openswd3::battle::coordinate_legacy_battle_debug_hotkeys(
                keyboard,
                state,
                fixture.bindings(),
                port,
                {
                    .actor_adjustment_entry_edx = 0x12345678U,
                    .actor_adjustment_x_argument_readable = false,
                }
            );
        test.expect_true(
            result.status ==
                    LegacyBattleDebugHotkeyStatus::
                        actor_coordinate_adjustment_typed_stop &&
                result.raw_key_queries == 3U &&
                result.actor_coordinate_adjustment_calls == 1U &&
                result.actor_adjust_iterations == 0U &&
                result.actor_coordinate_adjustment.status ==
                    openswd3::battle::
                        LegacyBattleActorCoordinateAdjustmentStatus::
                            x_argument_read_typed_stop &&
                result.actor_coordinate_adjustment.return_eax == 1U &&
                result.actor_coordinate_adjustment.return_ecx == 0x005029D0U &&
                result.actor_coordinate_adjustment.return_edx == 0x12345678U &&
                !result.actor_coordinate_adjustment.flags.carry &&
                !result.actor_coordinate_adjustment.flags.parity &&
                !result.actor_coordinate_adjustment.flags.auxiliary_carry &&
                result.actor_coordinate_adjustment.flags
                    .auxiliary_carry_defined &&
                !result.actor_coordinate_adjustment.flags.zero &&
                !result.actor_coordinate_adjustment.flags.sign &&
                !result.actor_coordinate_adjustment.flags.overflow &&
                result.actor_coordinate_adjustment.argument_reads == 0U &&
                fixture.startup.party[0].position_x == 20U &&
                fixture.effect_shift.actor_delta == 99 &&
                state.screenshot_request == 7U,
            "H argument typed-stop exposes the exact leaf result and suppresses delta J and P"
        );
    }

    {
        Fixture fixture;
        fixture.actor_metrics.group_a_count = 2U;
        fixture.actor_metrics.group_b_count = 1U;
        fixture.startup.party[0].position_x = 1U;
        fixture.startup.party[1].position_x = 2U;
        auto& group_b =
            (*fixture.startup.group_b_lifecycle)[0].action_execution;
        group_b.position_x = 3U;
        group_b.position_y = 13U;
        group_b.position_y_write_accessible = false;
        fixture.effect_shift.actor_delta = 77;
        LegacyBattleDebugHotkeyState state;
        state.developer_tools_enabled = 1U;
        state.screenshot_request = 7U;
        DebugPort port;
        openswd3::input_time_rng::LegacyKeyboardSnapshot keyboard{};
        press(keyboard, 0x23U);
        press(keyboard, 0x19U);
        const auto result =
            openswd3::battle::coordinate_legacy_battle_debug_hotkeys(
                keyboard,
                state,
                fixture.bindings(),
                port,
                {.actor_adjustment_entry_edx = 0xA5A51234U}
            );
        test.expect_true(
            result.status ==
                    LegacyBattleDebugHotkeyStatus::
                        actor_coordinate_adjustment_typed_stop &&
                result.raw_key_queries == 3U &&
                result.actor_coordinate_adjustment_calls == 3U &&
                result.actor_adjust_iterations == 2U &&
                fixture.startup.party[0].position_x == 11U &&
                fixture.startup.party[1].position_x == 12U &&
                group_b.position_x == 13U && group_b.position_y == 13U &&
                result.actor_coordinate_adjustment.status ==
                    openswd3::battle::
                        LegacyBattleActorCoordinateAdjustmentStatus::
                            position_y_add_typed_stop &&
                result.actor_coordinate_adjustment.return_eax == 10U &&
                result.actor_coordinate_adjustment.return_ecx == 0x00525508U &&
                result.actor_coordinate_adjustment.return_edx == 0xA5A50000U &&
                result.actor_coordinate_adjustment.coordinate_adds == 1U &&
                fixture.effect_shift.actor_delta == 77 &&
                state.screenshot_request == 7U,
            "H completes group-A before group-B and a Y fault keeps the current X prefix without committing the tail"
        );
    }

    {
        Fixture fixture;
        fixture.actor_metrics.group_a_count = 1U;
        fixture.actor_metrics.group_b_count = 1U;
        fixture.startup.party[0].position_x = 2U;
        fixture.startup.party[0].position_y = 3U;
        auto& group_b =
            (*fixture.startup.group_b_lifecycle)[0].action_execution;
        group_b.position_x = 0xFFFCU;
        group_b.position_y = 5U;
        LegacyBattleDebugHotkeyState state;
        state.developer_tools_enabled = 1U;
        DebugPort port;
        openswd3::input_time_rng::LegacyKeyboardSnapshot keyboard{};
        press(keyboard, 0x23U);
        press(keyboard, 0x24U);
        const auto result =
            openswd3::battle::coordinate_legacy_battle_debug_hotkeys(
                keyboard,
                state,
                fixture.bindings(),
                port,
                {.actor_adjustment_entry_edx = 0xCAFE9876U}
            );
        test.expect_true(
            result.status == LegacyBattleDebugHotkeyStatus::completed &&
                result.actor_adjust_iterations == 4U &&
                result.actor_coordinate_adjustment_calls == 4U &&
                fixture.startup.party[0].position_x == 2U &&
                fixture.startup.party[0].position_y == 3U &&
                group_b.position_x == 0xFFFCU && group_b.position_y == 5U &&
                fixture.effect_shift.actor_delta == -10 &&
                result.actor_coordinate_adjustment.return_eax == 0xFFF6U &&
                result.actor_coordinate_adjustment.return_ecx == 0x00525508U &&
                result.actor_coordinate_adjustment.return_edx == 0xCAFE0000U &&
                port.count(
                    LegacyBattleDebugHotkeyCall::reserved_adjust_actor_slot
                ) == 0U,
            "J runs after H restores every actor word modulo 65536 and commits negative ten"
        );
    }

    {
        Fixture fixture;
        LegacyBattleDebugHotkeyState state;
        state.developer_tools_enabled = 1U;
        DebugPort port;
        openswd3::input_time_rng::LegacyKeyboardSnapshot keyboard{};
        press(keyboard, 0x9DU);
        press(keyboard, 0x3DU);
        press(keyboard, 0x3FU);
        const auto result =
            openswd3::battle::coordinate_legacy_battle_debug_hotkeys(
                keyboard, state, fixture.bindings(), port
            );
        test.expect_true(
            result.control_chord_active && result.raw_key_queries == 19U &&
                port.count(LegacyBattleDebugHotkeyCall::suspend_audio_output) ==
                    2U &&
                port.count(LegacyBattleDebugHotkeyCall::restart_battle_music) ==
                    1U &&
                has_call(
                    port,
                    LegacyBattleDebugHotkeyCall::restart_battle_music,
                    0U,
                    0U,
                    0x0053C198U
                ),
            "right control reaches both audio suspension sites and the fixed battle music restart"
        );
    }
}
