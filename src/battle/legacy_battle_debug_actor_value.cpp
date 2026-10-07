#include "openswd3/battle/legacy_battle_debug_hotkeys.hpp"
#include "openswd3/battle/legacy_battle_actor_runtime_reset.hpp"

#include <bit>
#include <optional>

namespace openswd3::battle {
namespace {

using compat::u32;
using compat::u16;
using compat::i32;

[[nodiscard]] i32 signed_word(const u32 value) noexcept {
    return std::bit_cast<compat::i16>(static_cast<u16>(value));
}

[[nodiscard]] i32 signed_dword(const u32 value) noexcept {
    return std::bit_cast<i32>(value);
}

class Records final {
public:
    explicit Records(LegacyBattleDebugRecordPort& port) : port_(port) {}

    [[nodiscard]] std::optional<u32>
    read(const u32 token, const std::size_t offset, const std::size_t width) {
        const auto bytes = access(token, offset, width);
        if (bytes.empty()) {
            return std::nullopt;
        }

        return load(bytes);
    }

    [[nodiscard]] bool write(
        const u32 token,
        const std::size_t offset,
        const std::size_t width,
        const u32 value
    ) {
        auto bytes = access(token, offset, width);
        if (bytes.empty()) {
            return false;
        }

        store(bytes, value);
        return true;
    }

    [[nodiscard]] bool
    add_word(const u32 token, const std::size_t offset, const u32 delta) {
        auto bytes = access(token, offset, 2U);
        if (bytes.empty()) {
            return false;
        }

        // One mapped read/modify/write instruction, with WORD truncation.
        store(bytes, load(bytes) + delta);
        return true;
    }

    [[nodiscard]] bool clamp_word_maximum(
        const u32 token, const std::size_t current, const std::size_t maximum
    ) {
        const auto upper = read(token, maximum, 2U);
        if (!upper) {
            return false;
        }

        const auto value = read(token, current, 2U);
        if (!value) {
            return false;
        }

        return signed_word(*value) <= signed_word(*upper) ||
            write(token, current, 2U, *upper);
    }

    [[nodiscard]] bool
    clamp_word_minimum(const u32 token, const std::size_t offset) {
        const auto value = read(token, offset, 2U);
        return value &&
            (signed_word(*value) > 0 || write(token, offset, 2U, 0U));
    }

private:
    [[nodiscard]] std::span<std::byte>
    access(const u32 token, const std::size_t offset, const std::size_t width) {
        auto bytes = port_.debug_record_bytes(token);
        if (offset > bytes.size() || width > bytes.size() - offset) {
            return {};
        }

        return bytes.subspan(offset, width);
    }

    [[nodiscard]] static u32 load(const std::span<std::byte> bytes) {
        u32 value{};
        for (std::size_t i = 0U; i < bytes.size(); ++i) {
            value |= std::to_integer<u32>(bytes[i]) << (i * 8U);
        }

        return value;
    }

    static void store(const std::span<std::byte> bytes, const u32 value) {
        for (std::size_t i = 0U; i < bytes.size(); ++i) {
            bytes[i] = static_cast<std::byte>(value >> (i * 8U));
        }
    }

    LegacyBattleDebugRecordPort& port_;
};

}  // namespace

LegacyBattleDebugHotkeyCallReply apply_legacy_battle_debug_actor_values(
    LegacyBattleStartupState& startup,
    LegacyBattleActionDispatchState& action,
    LegacyBattleBoundedRandomPort& random,
    LegacyBattleDebugRecordPort& record_port,
    const LegacyBattleDebugHotkeyCallRequest& request
) {
    LegacyBattleDebugHotkeyCallReply reply{
        .eax = request.eax,
        .ecx = request.ecx,
        .edx = request.edx,
        .typed_stop = true
    };
    const auto finish = [&](const u32 value) {
        reply.eax = value;
        reply.typed_stop = false;
        return reply;
    };
    if (request.call != LegacyBattleDebugHotkeyCall::publish_actor_value) {
        return reply;
    }

    const auto actor = resolve_legacy_battle_actor_runtime_reset(
        {.action = &action, .startup = &startup}, request.object_token
    );
    if (actor.progress == nullptr) {
        return reply;
    }

    const u32 damage = request.arguments[0U];
    if (signed_dword(damage) > 0) {
        if ((actor.progress->mode_gate & 0x8000U) != 0U) {
            return finish(0U);
        }

        if (actor.action_execution == nullptr) {
            return reply;
        }

        if ((static_cast<u32>(actor.action_execution->field_26c0) &
             0x02000000U) != 0U) {
            return finish(0U);
        }
    }

    if (!actor.progress->special_ready_read_accessible) {
        return reply;
    }

    if (actor.progress->special_ready == 1U) {
        return finish(0U);
    }

    u32* kind{};
    bool kind_readable{};
    u32* metric{};
    u32* live_token{};
    u32 protection{};
    if (actor.group_a_configuration != nullptr) {
        const auto index = (request.object_token - 0x005029D0U) / 0x2F34U;
        auto& party = startup.party[index];
        kind = &party.configuration.source_runtime_value;
        kind_readable =
            party.configuration.source_runtime_value_read_accessible;
        metric = &party.primary_metric_override;
        live_token = &party.configuration.actor_record_token;
        protection = party.attribute_aggregation.embedded_profile_application
                         .status_bits;
    } else if (actor.group_b_configuration != nullptr) {
        const auto index = (request.object_token - 0x00525508U) / 0x2B28U;
        auto& enemy = (*startup.group_b_lifecycle)[index];
        kind = &enemy.action_configuration.source_runtime_value;
        kind_readable =
            enemy.action_configuration.source_runtime_value_read_accessible;
        metric = &enemy.action_configuration.timing_value;
        live_token = &enemy.live_record_token;
        protection = action.group_b_reward_scale[index].status_bits;
    } else {
        return reply;
    }

    if ((protection & 8U) != 0U) {
        return finish(0U);
    }

    if (!kind_readable) {
        return reply;
    }

    Records records(record_port);
    const auto mark_dead = [&]() {
        actor.progress->presentation_enabled = 1U;
        if (actor.group_b_configuration != nullptr) {
            actor.group_b_configuration->presentation_enabled = 1U;
        }
    };
    if (*kind == 1U) {
        if (actor.residual == nullptr) {
            return reply;
        }

        bool shared_damage = false;
        if (actor.residual->field_2b18 == 1U) {
            if (actor.shared_action == nullptr) {
                return reply;
            }

            const u32 value =
                std::bit_cast<u32>(actor.shared_action->decimal_value);
            if (value != 0U) {
                actor.shared_action->decimal_value =
                    signed_dword(value - damage);
                shared_damage = true;
            }
        }

        if (!shared_damage &&
            (!records.add_word(*live_token, 4U, 0U - damage) ||
             !records.clamp_word_maximum(*live_token, 4U, 10U))) {
            return reply;
        }

        if (!records.add_word(*live_token, 6U, request.arguments[1U]) ||
            !records.clamp_word_maximum(*live_token, 6U, 12U) ||
            !records.add_word(*live_token, 8U, request.arguments[2U]) ||
            !records.clamp_word_maximum(*live_token, 8U, 14U) ||
            !records.clamp_word_minimum(*live_token, 6U) ||
            !records.clamp_word_minimum(*live_token, 8U)) {
            return reply;
        }

        const u32 token = *live_token;
        const auto hp = records.read(token, 4U, 2U);
        if (!hp) {
            return reply;
        }

        if (signed_word(*hp) > 0) {
            return finish(0U);
        }

        if (!records.write(token, 4U, 2U, 0U) ||
            actor.action_execution == nullptr) {
            return reply;
        }

        const auto ai = actor.action_execution->action_twenty_seven_motion_mode;
        mark_dead();
        if (ai == 1U) {
            if (actor.base_initialization == nullptr) {
                return reply;
            }

            actor.base_initialization->field_2a94 = 6U;
        }

        return finish(1U);
    }

    if (actor.actor_resource_token_owner == nullptr) {
        return reply;
    }

    if (*metric != 0U) {
        *metric = signed_dword(damage) >= signed_dword(*metric)
            ? 0U
            : *metric - damage;
        const u32 token = *actor.actor_resource_token_owner;
        const u32 value = *metric;
        const auto maximum = records.read(token, 0x4CU, 4U);
        if (!maximum) {
            return reply;
        }

        if (signed_dword(value) > signed_dword(*maximum)) {
            *metric = *maximum;
        }

        if (signed_dword(*metric) > 0) {
            return finish(0U);
        }

        if (!records.write(token, 0x64U, 2U, 0U)) {
            return reply;
        }

        *metric = 0U;
    } else {
        u32 token = *actor.actor_resource_token_owner;
        const auto hp = records.read(token, 0x64U, 2U);
        if (!hp ||
            !records.write(
                token,
                0x64U,
                2U,
                signed_dword(damage) >= signed_word(*hp) ? 0U : *hp - damage
            )) {
            return reply;
        }

        token = *actor.actor_resource_token_owner;
        const auto current = records.read(token, 0x64U, 2U);
        if (!current) {
            return reply;
        }

        if (signed_word(*current) < 0) {
            const auto maximum = records.read(token, 0x4CU, 2U);
            if (!maximum || !records.write(token, 0x64U, 2U, *maximum)) {
                return reply;
            }
        }

        token = *actor.actor_resource_token_owner;
        const auto bounded = records.read(token, 0x64U, 2U);
        if (!bounded) {
            return reply;
        }

        const auto maximum = records.read(token, 0x4CU, 4U);
        if (!maximum) {
            return reply;
        }

        if (signed_word(*bounded) > signed_dword(*maximum) &&
            !records.write(token, 0x64U, 2U, *maximum)) {
            return reply;
        }

        token = *actor.actor_resource_token_owner;
        const auto final_hp = records.read(token, 0x64U, 2U);
        if (!final_hp) {
            return reply;
        }

        if (signed_word(*final_hp) > 0) {
            return finish(0U);
        }

        if (!records.write(token, 0x64U, 2U, 0U)) {
            return reply;
        }
    }

    if (actor.base_initialization == nullptr) {
        return reply;
    }

    const auto previous = actor.base_initialization->field_2a94;
    mark_dead();
    if (actor.action_execution == nullptr) {
        return reply;
    }

    actor.action_execution->turn_threshold = 0U;
    if (previous == 0U) {
        actor.base_initialization->field_2a94 =
            static_cast<compat::u8>(random.random_bounded(10U) + 1U);
    }

    return finish(1U);
}

}  // namespace openswd3::battle
