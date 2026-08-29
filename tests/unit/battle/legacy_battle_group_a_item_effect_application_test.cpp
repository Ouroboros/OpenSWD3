#include "openswd3/battle/legacy_battle_group_a_item_effect_application.hpp"

#include "test.hpp"

#include <array>
#include <bit>
#include <cstddef>
#include <deque>
#include <vector>

namespace {

using openswd3::battle::LegacyBattleGroupAItemEffectApplicationCall;
using openswd3::battle::LegacyBattleGroupAItemEffectApplicationCallReply;
using openswd3::battle::LegacyBattleGroupAItemEffectApplicationCallRequest;
using openswd3::battle::LegacyBattleGroupAItemEffectApplicationPort;
using openswd3::compat::i16;
using openswd3::compat::i32;
using openswd3::compat::u8;
using openswd3::compat::u16;
using openswd3::compat::u32;

struct ItemEffectPort final : LegacyBattleGroupAItemEffectApplicationPort {
    [[nodiscard]] LegacyBattleGroupAItemEffectApplicationCallReply
    invoke_group_a_item_effect_application(
        const LegacyBattleGroupAItemEffectApplicationCallRequest& request
    ) override {
        requests.push_back(request);
        if (replies.empty()) {
            return {};
        }
        const auto reply = replies.front();
        replies.pop_front();
        return reply;
    }

    std::deque<LegacyBattleGroupAItemEffectApplicationCallReply> replies;
    std::vector<LegacyBattleGroupAItemEffectApplicationCallRequest> requests;
};

void set_actor_byte(
    std::array<u32, 14>& actor, const std::size_t offset, const u8 value
) {
    const std::size_t index = offset / 4U;
    const u32 shift = static_cast<u32>((offset & 3U) * 8U);
    actor[index] =
        (actor[index] & ~(0xFFU << shift)) | (static_cast<u32>(value) << shift);
}

void set_actor_word(
    std::array<u32, 14>& actor, const std::size_t offset, const u16 value
) {
    set_actor_byte(actor, offset, static_cast<u8>(value));
    set_actor_byte(actor, offset + 1U, static_cast<u8>(value >> 8U));
}

[[nodiscard]] constexpr u32 bits(const i32 value) {
    return std::bit_cast<u32>(value);
}

}  // namespace

void test_battle_group_a_item_effect_application(
    openswd3::test::Context& test
) {
    using openswd3::battle::LegacyBattleActorProgressState;
    using openswd3::battle::LegacyBattleGroupAConfigurationState;
    using openswd3::battle::LegacyBattleGroupAItemEffectApplicationRequest;
    using openswd3::battle::LegacyBattleGroupAItemEffectApplicationState;
    using openswd3::battle::LegacyBattleGroupAItemEffectApplicationStatus;
    using openswd3::battle::apply_legacy_battle_group_a_item_effect;
    using openswd3::battle::kLegacyBattleGroupAItemEffectListToken;

    constexpr u32 actor_token = 0x00505890U;

    {
        LegacyBattleActorProgressState progress;
        LegacyBattleGroupAConfigurationState configuration;
        ItemEffectPort port;
        const auto result = apply_legacy_battle_group_a_item_effect(
            nullptr,
            progress,
            configuration,
            0U,
            port,
            {
                .effect_kind = 21U,
                .entry_eax = 0x12345678U,
                .entry_edx = 0x87654321U,
            }
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupAItemEffectApplicationStatus::
                        actor_state_typed_stop &&
                result.return_eax == 0x12345678U && result.return_ecx == 0U &&
                result.return_edx == 0x87654321U && port.requests.empty(),
            "group-A item effect stops at the first actor state access without consuming the entry register prefix"
        );
    }

    {
        LegacyBattleGroupAItemEffectApplicationState state;
        LegacyBattleActorProgressState progress{.progress_multiplier = 77U};
        LegacyBattleGroupAConfigurationState configuration;
        ItemEffectPort port;
        port.replies.push_back({
            .eax = 0xABCD1234U,
            .ecx = 0x11112222U,
            .edx = 0x33334444U,
        });
        const auto result = apply_legacy_battle_group_a_item_effect(
            &state,
            progress,
            configuration,
            actor_token,
            port,
            {
                .effect_kind = 20U,
                .entry_eax = 0x55556666U,
                .entry_edx = 0x77778888U,
            }
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupAItemEffectApplicationStatus::completed &&
                result.switch_index == 0xFFFFFFFFU &&
                result.cache_lookup_calls == 1U && result.cache_writes == 1U &&
                state.cached_profile_item_id == 0x1234U &&
                result.item_delta_calls == 0U &&
                result.return_eax == 0xFFFFFFFFU &&
                result.return_ecx == 0x11112222U &&
                result.return_edx == 0x33334444U &&
                port.requests.size() == 1U &&
                port.requests[0U].call ==
                    LegacyBattleGroupAItemEffectApplicationCall::
                        lookup_embedded_profile_item_id &&
                port.requests[0U].effect_kind == 20U &&
                port.requests[0U].eax == 0x55556666U &&
                port.requests[0U].ecx == actor_token &&
                port.requests[0U].edx == 0x77778888U,
            "group-A item effect lazily caches the profile item id before preserving the unsigned default switch return"
        );
    }

    struct SimpleCase {
        u32 kind;
        u32 flag_mask;
        u16 action_kind;
        u16 display_kind;
        bool mode_flag;
        bool refresh;
        bool activation;
    };
    constexpr std::array<SimpleCase, 12> simple_cases{{
        {21U, 0x001U, 1U, 0xBEEFU, false, true, true},
        {22U, 0x002U, 0U, 22U, true, false, false},
        {23U, 0x004U, 23U, 0xBEEFU, false, false, false},
        {24U, 0x008U, 24U, 0xBEEFU, false, true, false},
        {25U, 0x010U, 0U, 25U, true, false, false},
        {26U, 0x020U, 26U, 0xBEEFU, false, false, false},
        {27U, 0x040U, 27U, 0xBEEFU, false, false, false},
        {28U, 0x000U, 28U, 0xBEEFU, false, true, false},
        {29U, 0x100U, 29U, 0xBEEFU, false, false, false},
        {30U, 0x200U, 30U, 0xBEEFU, false, false, false},
        {32U, 0x000U, 32U, 0xBEEFU, false, true, false},
        {33U, 0x000U, 33U, 0xBEEFU, false, true, false},
    }};
    for (const auto& entry : simple_cases) {
        LegacyBattleGroupAItemEffectApplicationState state{
            .cached_profile_item_id = 1U,
            .effect_flags = 0xA5B60080U,
            .action_kind = 0xAAAAU,
            .display_kind = 0xBEEFU,
            .mode_flags = 0x80U,
            .activation_latch = 0x66U,
        };
        LegacyBattleActorProgressState progress{.progress_multiplier = 100U};
        LegacyBattleGroupAConfigurationState configuration;
        ItemEffectPort port;
        if (entry.refresh) {
            port.replies.push_back({
                .eax = 0x10101010U + entry.kind,
                .ecx = 0x20202020U + entry.kind,
                .edx = 0x30303030U + entry.kind,
                .publish_progress_multiplier = true,
                .progress_multiplier = static_cast<u16>(200U + entry.kind),
            });
        }
        port.replies.push_back({
            .eax = 0x41414141U + entry.kind,
            .ecx = 0x51515151U + entry.kind,
            .edx = 0x61616161U + entry.kind,
        });
        const auto result = apply_legacy_battle_group_a_item_effect(
            &state,
            progress,
            configuration,
            actor_token,
            port,
            {
                .effect_kind = entry.kind,
                .entry_eax = 0x70000000U + entry.kind,
                .entry_edx = 0x80000000U + entry.kind,
            }
        );
        const u32 expected_flags = 0xA5B60080U | entry.flag_mask;
        const std::size_t post_index = entry.refresh ? 1U : 0U;
        test.expect_true(
            result.status ==
                    LegacyBattleGroupAItemEffectApplicationStatus::completed &&
                state.effect_flags == expected_flags &&
                state.action_kind == entry.action_kind &&
                state.display_kind == entry.display_kind &&
                state.mode_flags ==
                    static_cast<u8>(entry.mode_flag ? 0x84U : 0x80U) &&
                state.activation_latch ==
                    static_cast<u8>(entry.activation ? 1U : 0x66U) &&
                result.multiplier_refresh_calls ==
                    static_cast<u32>(entry.refresh) &&
                result.item_delta_calls == 1U &&
                port.requests.size() == post_index + 1U &&
                port.requests[post_index].call ==
                    LegacyBattleGroupAItemEffectApplicationCall::
                        apply_profile_item_quantity_delta &&
                port.requests[post_index].item_list_token ==
                    kLegacyBattleGroupAItemEffectListToken &&
                port.requests[post_index].effect_kind == entry.kind &&
                port.requests[post_index].quantity_delta == 5U &&
                port.requests[post_index].ecx == actor_token &&
                result.return_eax == 0x41414141U + entry.kind &&
                result.return_ecx == 0x51515151U + entry.kind &&
                result.return_edx == 0x61616161U + entry.kind,
            "group-A item effect preserves every simple switch branch and common item-delta tail"
        );
    }

    {
        LegacyBattleGroupAItemEffectApplicationState state{
            .cached_profile_item_id = 1U,
            .action_kind = 0xAAAAU,
            .display_kind = 0xBBBBU,
            .mode_flags = 0x80U,
            .derived_words = {0xCCCCU, 0xDDDDU, 0xEEEEU, 0xFFFFU},
        };
        LegacyBattleActorProgressState progress{.progress_multiplier = 1234U};
        LegacyBattleGroupAConfigurationState configuration;
        ItemEffectPort port;
        port.replies.push_back({
            .eax = 0x11111111U,
            .ecx = 0x22222222U,
            .edx = 0x33333333U,
        });
        port.replies.push_back({
            .eax = 0x44444444U,
            .ecx = 0x55555555U,
            .edx = 0x66666666U,
        });
        const auto result = apply_legacy_battle_group_a_item_effect(
            &state,
            progress,
            configuration,
            actor_token,
            port,
            {.effect_kind = 31U, .entry_eax = 31U, .entry_edx = 0x77777777U}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupAItemEffectApplicationStatus::completed &&
                state.action_kind == 0U && state.display_kind == 31U &&
                state.mode_flags == 0x84U && state.derived_words[0U] == 50U &&
                state.derived_words[1U] == 0xDDDDU &&
                result.derived_word_writes == 1U &&
                port.requests.size() == 2U && port.requests[1U].eax == 0U &&
                port.requests[1U].edx == 49U,
            "type 31 derives four percent plus one with the original magic-product register tail"
        );
    }

    struct DerivedCase {
        u32 kind;
        std::size_t record_offset;
        i16 record_value;
        u16 expected_multiplier;
        i32 expected_derived;
        i32 expected_post_edx;
    };
    constexpr std::array<DerivedCase, 3> derived_cases{{
        {34U, 0x0AU, -250, 400U, -1025, -1026},
        {35U, 0x0CU, 300, 200U, 630, 630},
        {36U, 0x08U, 250, 400U, 1025, 1025},
    }};
    for (const auto& entry : derived_cases) {
        LegacyBattleGroupAItemEffectApplicationState state{
            .cached_profile_item_id = 1U,
            .derived_words = {0x1111U, 0x2222U, 0x3333U, 0x4444U},
        };
        LegacyBattleActorProgressState progress{.progress_multiplier = 1000U};
        LegacyBattleGroupAConfigurationState configuration;
        configuration.actor_record_token = 0x004AB790U;
        set_actor_word(
            configuration.actor_record,
            entry.record_offset,
            std::bit_cast<u16>(entry.record_value)
        );
        ItemEffectPort port;
        port.replies.push_back({
            .eax = 0x10101010U,
            .ecx = 0x20202020U,
            .edx = 0x30303030U,
        });
        port.replies.push_back({
            .eax = 0x70707070U,
            .ecx = 0x80808080U,
            .edx = 0x90909090U,
        });
        const auto result = apply_legacy_battle_group_a_item_effect(
            &state,
            progress,
            configuration,
            actor_token,
            port,
            {.effect_kind = entry.kind, .entry_eax = entry.kind}
        );
        const std::size_t derived_index = entry.kind - 33U;
        test.expect_true(
            result.status ==
                    LegacyBattleGroupAItemEffectApplicationStatus::completed &&
                progress.progress_multiplier == entry.expected_multiplier &&
                state.derived_words[derived_index] ==
                    static_cast<u16>(entry.expected_derived) &&
                result.progress_multiplier_writes == 1U &&
                result.derived_word_writes == 1U &&
                port.requests.size() == 2U &&
                port.requests[1U].eax == bits(entry.expected_derived) &&
                port.requests[1U].edx == bits(entry.expected_post_edx) &&
                result.return_eax == 0x70707070U &&
                result.return_ecx == 0x80808080U &&
                result.return_edx == 0x90909090U,
            "types 34 through 36 preserve multiplier reduction signed record scaling and stale magic registers"
        );
    }

    for (const auto& entry : derived_cases) {
        LegacyBattleGroupAItemEffectApplicationState state{
            .cached_profile_item_id = 1U,
        };
        LegacyBattleActorProgressState progress{.progress_multiplier = 1000U};
        LegacyBattleGroupAConfigurationState configuration;
        ItemEffectPort port;
        port.replies.push_back({
            .eax = 0xAAAAAAAAU,
            .ecx = 0xBBBBBBBBU,
            .edx = 0xCCCCCCCCU,
        });
        const auto result = apply_legacy_battle_group_a_item_effect(
            &state,
            progress,
            configuration,
            actor_token,
            port,
            {.effect_kind = entry.kind, .entry_eax = entry.kind}
        );
        const u32 expected_eax =
            entry.kind == 35U ? 0U : entry.expected_multiplier;
        const u32 expected_edx =
            entry.kind == 35U ? entry.expected_multiplier : 100U;
        test.expect_true(
            result.status ==
                    LegacyBattleGroupAItemEffectApplicationStatus::
                        actor_record_typed_stop &&
                progress.progress_multiplier == entry.expected_multiplier &&
                result.derived_word_writes == 0U &&
                result.item_delta_calls == 0U && port.requests.size() == 1U &&
                result.return_eax == expected_eax && result.return_ecx == 0U &&
                result.return_edx == expected_edx,
            "derived item effects stop at the original base-record dereference after preserving multiplier reduction"
        );
    }
}
