#include "openswd3/battle/legacy_battle_group_a_attribute_effect.hpp"

#include "test.hpp"

#include <array>
#include <bit>
#include <cstddef>
#include <vector>

namespace {

using openswd3::battle::LegacyBattleGroupAAttributeEffectCallReply;
using openswd3::battle::LegacyBattleGroupAAttributeEffectCallRequest;
using openswd3::battle::LegacyBattleGroupAAttributeEffectPort;
using openswd3::compat::i16;
using openswd3::compat::i32;
using openswd3::compat::u8;
using openswd3::compat::u16;
using openswd3::compat::u32;

struct EffectPort final : LegacyBattleGroupAAttributeEffectPort {
    [[nodiscard]] LegacyBattleGroupAAttributeEffectCallReply
    invoke_group_a_attribute_effect(
        const LegacyBattleGroupAAttributeEffectCallRequest& request
    ) override {
        requests.push_back(request);
        const u32 count = static_cast<u32>(requests.size());
        return {
            .eax = 0xA0000000U + count,
            .ecx = 0xB0000000U + count,
            .edx = 0xC0000000U + count,
        };
    }

    std::vector<LegacyBattleGroupAAttributeEffectCallRequest> requests;
};

void set_record_byte(
    std::array<u32, 14>& record, const std::size_t offset, const u8 value
) {
    const std::size_t index = offset / 4U;
    const u32 shift = static_cast<u32>((offset & 3U) * 8U);
    record[index] = (record[index] & ~(0xFFU << shift)) |
        (static_cast<u32>(value) << shift);
}

void set_record_word(
    std::array<u32, 14>& record, const std::size_t offset, const u16 value
) {
    set_record_byte(record, offset, static_cast<u8>(value));
    set_record_byte(record, offset + 1U, static_cast<u8>(value >> 8U));
}

[[nodiscard]] constexpr u32 to_bits(const i32 value) {
    return std::bit_cast<u32>(value);
}

}  // namespace

void test_battle_group_a_attribute_effect(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleGroupAAttributeEffectCall;
    using openswd3::battle::LegacyBattleGroupAAttributeEffectState;
    using openswd3::battle::LegacyBattleGroupAAttributeEffectStatus;
    using openswd3::battle::LegacyBattleGroupAWorkspaceState;
    using openswd3::battle::apply_legacy_battle_group_a_attribute_effects;

    {
        LegacyBattleGroupAAttributeEffectState state;
        LegacyBattleGroupAWorkspaceState workspace;
        workspace.tail_words[7U] = 200U;
        workspace.tail_words[8U] = 400U;
        workspace.tail_words[9U] = 500U;
        std::array<u32, 14> source{};
        set_record_word(source, 0x0AU, 25U);
        set_record_word(source, 0x0CU, 300U);
        set_record_word(source, 0x0EU, std::bit_cast<u16>(i16{-300}));
        EffectPort port;

        const auto result = apply_legacy_battle_group_a_attribute_effects(
            &state,
            workspace,
            &source,
            0x005029D0U,
            0x004AB790U,
            port,
            {.entry_eax = 0x12345678U, .entry_edx = 0x87654321U}
        );

        test.expect_true(
            result.status ==
                    LegacyBattleGroupAAttributeEffectStatus::completed &&
                result.port_calls == 15U && result.active_channels == 3U &&
                result.forced_minimums == 0U && result.temporary_writes == 3U &&
                result.temporary_clears == 3U &&
                result.computed_words ==
                    std::array<u16, 3>{0xFFCEU, 0x04B0U, 0xFA24U} &&
                state.temporary_values == std::array<u16, 3>{0U, 0U, 0U} &&
                port.requests.size() == 15U &&
                port.requests[0U].call ==
                    LegacyBattleGroupAAttributeEffectCall::
                        publish_channel_effect &&
                port.requests[0U].arguments ==
                    std::array<u32, 3>{to_bits(-50), 0U, 0U} &&
                port.requests[5U].arguments ==
                    std::array<u32, 3>{0U, 0x000104B0U, 0U} &&
                port.requests[10U].arguments ==
                    std::array<u32, 3>{0U, 0U, 0xFFFDFA24U} &&
                port.requests[1U].arguments[0U] == 0x246FU &&
                port.requests[6U].arguments[0U] == 0x2367U &&
                port.requests[11U].arguments[0U] == 0x2366U &&
                port.requests[2U].arguments[0U] == to_bits(-50) &&
                port.requests[7U].arguments[0U] == 1200U &&
                port.requests[12U].arguments[0U] == to_bits(-1500) &&
                port.requests[3U].arguments[0U] == 0U &&
                port.requests[8U].arguments[0U] == 6U &&
                port.requests[13U].arguments[0U] == 12U &&
                port.requests[4U].arguments[0U] == 1U &&
                port.requests[9U].arguments[0U] == 1U &&
                port.requests[14U].arguments[0U] == 1U &&
                result.return_eax == 0xA000000FU &&
                result.return_ecx == 0xB000000FU &&
                result.return_edx == 0xC000000FU,
            "three active attribute channels preserve signed percentages, stale high words, fixed resources, offsets, and callee order"
        );
    }

    {
        LegacyBattleGroupAAttributeEffectState state{
            .temporary_values = {0x1111U, 0x2222U, 0x3333U},
        };
        LegacyBattleGroupAWorkspaceState workspace;
        workspace.tail_words[7U] = 1U;
        workspace.tail_words[8U] = 2U;
        workspace.tail_words[9U] = 3U;
        std::array<u32, 14> source{};
        set_record_byte(source, 0x25U, 0x80U);
        EffectPort port;

        const auto result = apply_legacy_battle_group_a_attribute_effects(
            &state,
            workspace,
            &source,
            0x005029D0U,
            0x004AB790U,
            port,
            {.entry_eax = 0x12345678U, .entry_edx = 0x87654321U}
        );

        test.expect_true(
            result.status ==
                    LegacyBattleGroupAAttributeEffectStatus::completed &&
                result.active_channels == 0U && port.requests.empty() &&
                state.temporary_values ==
                    std::array<u16, 3>{0x1111U, 0x2222U, 0x3333U} &&
                result.return_eax == 0x12345678U &&
                result.return_ecx == 0x004AB790U &&
                result.return_edx == 0x87654321U,
            "source bit seven returns before reading channel totals or changing temporary words"
        );
    }

    {
        LegacyBattleGroupAAttributeEffectState state{
            .temporary_values = {0x1111U, 0x2222U, 0x3333U},
        };
        LegacyBattleGroupAWorkspaceState workspace;
        std::array<u32, 14> source{};
        EffectPort port;

        const auto result = apply_legacy_battle_group_a_attribute_effects(
            &state,
            workspace,
            &source,
            0x005029D0U,
            0x004AB790U,
            port,
            {.entry_eax = 0x12345678U, .entry_edx = 0x87654321U}
        );

        test.expect_true(
            result.status ==
                    LegacyBattleGroupAAttributeEffectStatus::completed &&
                result.active_channels == 0U && port.requests.empty() &&
                state.temporary_values ==
                    std::array<u16, 3>{0x1111U, 0x2222U, 0x3333U} &&
                result.return_eax == 0x12340000U &&
                result.return_ecx == 0x004AB790U &&
                result.return_edx == 0x87654321U,
            "three zero totals preserve temporary words and leave the final eax low word zero"
        );
    }

    {
        LegacyBattleGroupAAttributeEffectState state{
            .temporary_values = {0xAAAAU, 0xBBBBU, 0xCCCCU},
        };
        LegacyBattleGroupAWorkspaceState workspace;
        workspace.tail_words[7U] = 1U;
        std::array<u32, 14> source{};
        EffectPort port;

        const auto result = apply_legacy_battle_group_a_attribute_effects(
            &state,
            workspace,
            &source,
            0x005029D0U,
            0x004AB790U,
            port,
            {.entry_eax = 0x12345678U, .entry_edx = 0x87654321U}
        );

        test.expect_true(
            result.status ==
                    LegacyBattleGroupAAttributeEffectStatus::completed &&
                result.active_channels == 1U && result.forced_minimums == 1U &&
                result.computed_words[0U] == 0xFFFFU &&
                state.temporary_values ==
                    std::array<u16, 3>{0U, 0xBBBBU, 0xCCCCU} &&
                port.requests.size() == 5U &&
                port.requests[0U].arguments[0U] == to_bits(-1) &&
                result.return_eax == 0xA0000000U &&
                result.return_ecx == 0xB0000005U &&
                result.return_edx == 0xC0000005U,
            "zero percentage forces one, negates channel zero, clears only its temporary, and preserves final callee high eax"
        );
    }

    {
        LegacyBattleGroupAAttributeEffectState state{
            .temporary_values = {1U, 2U, 3U},
        };
        LegacyBattleGroupAWorkspaceState workspace;
        std::array<u32, 14> source{};
        EffectPort actor_port;
        const auto actor_stop = apply_legacy_battle_group_a_attribute_effects(
            nullptr,
            workspace,
            &source,
            0x005029D0U,
            0x004AB790U,
            actor_port,
            {.entry_eax = 0x12345678U, .entry_edx = 0x87654321U}
        );
        EffectPort source_port;
        const auto source_stop = apply_legacy_battle_group_a_attribute_effects(
            &state,
            workspace,
            nullptr,
            0x005029D0U,
            0U,
            source_port,
            {.entry_eax = 0x12345678U, .entry_edx = 0x87654321U}
        );

        test.expect_true(
            actor_stop.status ==
                    LegacyBattleGroupAAttributeEffectStatus::
                        actor_state_typed_stop &&
                source_stop.status ==
                    LegacyBattleGroupAAttributeEffectStatus::
                        source_record_typed_stop &&
                state.temporary_values == std::array<u16, 3>{1U, 2U, 3U} &&
                actor_port.requests.empty() && source_port.requests.empty() &&
                source_stop.return_eax == 0x12345678U &&
                source_stop.return_ecx == 0U &&
                source_stop.return_edx == 0x87654321U,
            "missing actor or source owner stops before the first flag access without changing temporaries"
        );
    }
}
