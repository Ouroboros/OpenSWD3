#include "openswd3/battle/legacy_battle_pair_transition.hpp"

#include <algorithm>
#include <bit>
#include <initializer_list>

namespace openswd3::battle {
namespace {

using compat::i16;
using compat::i32;
using compat::u16;
using compat::u32;

[[nodiscard]] constexpr u16 low_word(const u32 value) noexcept {
    return static_cast<u16>(value);
}

[[nodiscard]] constexpr u32 sign_extend_word(const u32 value) noexcept {
    return std::bit_cast<u32>(
        static_cast<i32>(std::bit_cast<i16>(low_word(value)))
    );
}

void replace_high_word(u32& value, const u16 high) noexcept {
    value = (value & 0x0000FFFFU) | (static_cast<u32>(high) << 16U);
}

struct Registers {
    u32 eax{};
    u32 ecx{};
    u32 edx{};
};

class Runner final {
public:
    Runner(
        LegacyBattlePairTransitionPort& port,
        LegacyBattlePairTransitionResult& result,
        const Registers registers
    )
        : port_(port), result_(result), registers_(registers) {}

    [[nodiscard]] LegacyBattlePairTransitionCallReply invoke(
        const LegacyBattlePairTransitionCall call,
        const u32 object_token,
        const std::initializer_list<u32> arguments = {}
    ) {
        LegacyBattlePairTransitionCallRequest request{};
        request.call = call;
        request.object_token = object_token;
        request.eax = registers_.eax;
        request.ecx = object_token;
        request.edx = registers_.edx;
        std::copy(
            arguments.begin(), arguments.end(), request.arguments.begin()
        );
        const auto reply = port_.invoke_pair_transition(request);
        ++result_.port_calls;
        registers_ = {
            .eax = reply.eax,
            .ecx = reply.ecx,
            .edx = reply.edx,
        };
        if (reply.publish_primary_value) {
            port_.battle_pair_primary_value() = reply.primary_value;
        }
        if (reply.publish_secondary_value) {
            port_.battle_pair_secondary_value() = reply.secondary_value;
        }
        if (reply.publish_packed_reward_high) {
            replace_high_word(
                port_.effect_shift_state().packed_reward,
                reply.packed_reward_high
            );
        }
        return reply;
    }

    void overwrite_eax(const u32 value) noexcept {
        registers_.eax = value;
    }
    void overwrite_edx(const u32 value) noexcept {
        registers_.edx = value;
    }

    [[nodiscard]] const Registers& registers() const noexcept {
        return registers_;
    }

private:
    LegacyBattlePairTransitionPort& port_;
    LegacyBattlePairTransitionResult& result_;
    Registers registers_{};
};

}  // namespace

LegacyBattlePairTransitionResult advance_legacy_battle_pair_transition(
    LegacyBattlePairTransitionPort& port,
    const LegacyBattlePairTransitionRequest& request
) {
    LegacyBattlePairTransitionResult result;
    Runner runner(
        port,
        result,
        {.eax = request.eax, .ecx = request.ecx, .edx = request.edx}
    );

    u32 current_value = port.battle_pair_primary_value();
    if (current_value == 0U) {
        result.primary_value_was_zero = true;
        result.return_eax = request.eax;
        result.return_ecx = request.ecx;
        result.return_edx = request.edx;
        return result;
    }

    const auto kind = runner.invoke(
        LegacyBattlePairTransitionCall::query_kind, request.primary_object_token
    );
    result.transition_kind = low_word(kind.eax);

    if (result.transition_kind == 1U) {
        static_cast<void>(runner.invoke(
            LegacyBattlePairTransitionCall::publish_action_id,
            request.primary_object_token,
            {0x246FU}
        ));
        current_value = 0U - current_value;
        static_cast<void>(runner.invoke(
            LegacyBattlePairTransitionCall::publish_value,
            request.primary_object_token,
            {current_value}
        ));
        static_cast<void>(runner.invoke(
            LegacyBattlePairTransitionCall::publish_mode,
            request.primary_object_token,
            {1U}
        ));
        static_cast<void>(runner.invoke(
            LegacyBattlePairTransitionCall::commit_visual,
            request.primary_object_token,
            {current_value, 0U, 0U}
        ));
    } else if (result.transition_kind == 2U) {
        result.mode_two_path = true;
        u32 unused_output{};
        u32 candidate_output{};
        const auto outputs = runner.invoke(
            LegacyBattlePairTransitionCall::query_mode_two_values,
            request.secondary_object_token
        );
        if ((outputs.output_write_mask & 1U) != 0U) {
            unused_output = outputs.outputs[0];
        }
        if ((outputs.output_write_mask & 2U) != 0U) {
            candidate_output = outputs.outputs[1];
        }
        static_cast<void>(unused_output);

        const u32 candidate = sign_extend_word(candidate_output);
        runner.overwrite_eax(candidate);
        const u32 delta = candidate - current_value;
        runner.overwrite_edx(delta);
        if (std::bit_cast<i32>(delta) <= 0) {
            current_value = candidate;
            static_cast<void>(runner.invoke(
                LegacyBattlePairTransitionCall::publish_value,
                request.secondary_object_token,
                {current_value}
            ));
        }
        static_cast<void>(runner.invoke(
            LegacyBattlePairTransitionCall::publish_action_id,
            request.secondary_object_token,
            {0x235EU}
        ));
        static_cast<void>(runner.invoke(
            LegacyBattlePairTransitionCall::publish_mode,
            request.secondary_object_token,
            {1U}
        ));
        static_cast<void>(runner.invoke(
            LegacyBattlePairTransitionCall::publish_action_id,
            request.primary_object_token,
            {0x2367U}
        ));
        static_cast<void>(runner.invoke(
            LegacyBattlePairTransitionCall::publish_value,
            request.primary_object_token,
            {current_value}
        ));
        static_cast<void>(runner.invoke(
            LegacyBattlePairTransitionCall::publish_mode,
            request.primary_object_token,
            {1U}
        ));
        static_cast<void>(runner.invoke(
            LegacyBattlePairTransitionCall::commit_visual,
            request.primary_object_token,
            {0U, current_value, 0U}
        ));

        const u32 negated = 0U - current_value;
        port.battle_pair_secondary_value() = low_word(negated);
        result.secondary_value_published = true;
        port.battle_pair_primary_value() = 0U;
        result.primary_value_cleared = true;
    } else if (result.transition_kind == 4U) {
        result.mode_four_path = true;
        u32 unused_output{};
        u32 candidate_output{};
        const auto outputs = runner.invoke(
            LegacyBattlePairTransitionCall::query_mode_four_values,
            request.secondary_object_token
        );
        if ((outputs.output_write_mask & 1U) != 0U) {
            unused_output = outputs.outputs[0];
        }
        if ((outputs.output_write_mask & 2U) != 0U) {
            candidate_output = outputs.outputs[1];
        }
        static_cast<void>(unused_output);

        const u32 candidate = sign_extend_word(candidate_output);
        runner.overwrite_eax(candidate);
        const u32 delta = candidate - current_value;
        runner.overwrite_edx(delta);
        if (std::bit_cast<i32>(delta) <= 0) {
            current_value = candidate;
            static_cast<void>(runner.invoke(
                LegacyBattlePairTransitionCall::publish_value,
                request.secondary_object_token,
                {current_value}
            ));
        }
        static_cast<void>(runner.invoke(
            LegacyBattlePairTransitionCall::publish_action_id,
            request.secondary_object_token,
            {0x235EU}
        ));
        static_cast<void>(runner.invoke(
            LegacyBattlePairTransitionCall::publish_mode,
            request.secondary_object_token,
            {1U}
        ));
        static_cast<void>(runner.invoke(
            LegacyBattlePairTransitionCall::publish_action_id,
            request.primary_object_token,
            {0x2366U}
        ));
        static_cast<void>(runner.invoke(
            LegacyBattlePairTransitionCall::publish_value,
            request.primary_object_token,
            {current_value}
        ));
        static_cast<void>(runner.invoke(
            LegacyBattlePairTransitionCall::publish_mode,
            request.primary_object_token,
            {1U}
        ));
        static_cast<void>(runner.invoke(
            LegacyBattlePairTransitionCall::commit_visual,
            request.primary_object_token,
            {0U, 0U, current_value}
        ));

        const u32 negated = 0U - current_value;
        port.battle_pair_primary_value() = 0U;
        result.primary_value_cleared = true;
        replace_high_word(
            port.effect_shift_state().packed_reward, low_word(negated)
        );
        result.packed_reward_high_published = true;
    }

    const auto& registers = runner.registers();
    result.return_eax = registers.eax;
    result.return_ecx = registers.ecx;
    result.return_edx = registers.edx;
    return result;
}

}  // namespace openswd3::battle
