#include "openswd3/battle/legacy_battle_actor_profile_preparation.hpp"

#include "test.hpp"

namespace {

class ProfilePort final
    : public openswd3::battle::LegacyBattleActorProfilePreparationPort {
public:
    [[nodiscard]] openswd3::battle::LegacyBattleActorProfilePreparationReply
    build_record(const openswd3::compat::u32 source_value) override {
        ++build_calls;
        source = source_value;
        return {
            .record =
                {.output_value = 0x1234U,
                 .profile_id = 7U,
                 .fallback_value = 0x5678U},
            .eax = 1U,
            .ecx = 2U,
            .edx = 3U
        };
    }
    [[nodiscard]] openswd3::battle::LegacyBattleActorProfilePreparationReply
    resolve_record(
        const openswd3::compat::u32 context_token,
        const openswd3::battle::LegacyBattleActorProfilePreparationRecord&
            record,
        const openswd3::compat::u32 eax,
        const openswd3::compat::u32 ecx,
        const openswd3::compat::u32 edx
    ) override {
        ++resolve_calls;
        context = context_token;
        return {.record = record, .eax = eax, .ecx = ecx, .edx = edx};
    }
    [[nodiscard]] openswd3::battle::LegacyBattleActorProfilePreparationReply
    load_profile(
        const openswd3::compat::u32 buffer_token,
        const openswd3::compat::u16 profile_id,
        const openswd3::compat::u32,
        const openswd3::compat::u32,
        const openswd3::compat::u32
    ) override {
        ++load_calls;
        buffer = buffer_token;
        profile = profile_id;
        return {.eax = 0xAAAAU, .ecx = 0xBBBBU, .edx = 0xCCCCU};
    }
    openswd3::compat::u32 build_calls{};
    openswd3::compat::u32 resolve_calls{};
    openswd3::compat::u32 load_calls{};
    openswd3::compat::u32 source{};
    openswd3::compat::u32 context{};
    openswd3::compat::u32 buffer{};
    openswd3::compat::u16 profile{};
};

}  // namespace

void test_battle_actor_profile_preparation(openswd3::test::Context& test) {
    using namespace openswd3::battle;

    {
        LegacyBattleGroupAFinalProcessingState final_state;
        LegacyBattleGroupAItemEffectApplicationState item_effect;
        ProfilePort port;
        const auto result = prepare_legacy_battle_actor_profile(
            &final_state,
            &item_effect,
            0x005029D0U,
            port,
            {.source_value = 9U, .context_token = 0x71000000U}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleActorProfilePreparationStatus::completed &&
                result.output_value == 0x1234U &&
                result.fallback_writes == 1U &&
                (final_state.profile_buffer[3U] >> 16U) == 0x5678U &&
                (item_effect.mode_flags & 0x80U) != 0U && port.source == 9U &&
                port.context == 0x71000000U && port.buffer == 0x00503760U &&
                port.profile == 7U && result.return_eax == 0xAAAAU &&
                result.return_ecx == 0xBBBBU && result.return_edx == 0xCCCCU,
            "profile preparation builds, resolves, loads, publishes fallback and sets mode bit"
        );
    }

    {
        LegacyBattleGroupAFinalProcessingState final_state;
        final_state.profile_buffer[4U] = 1U;
        final_state.profile_buffer[3U] = 0xABCD0000U;
        LegacyBattleGroupAItemEffectApplicationState item_effect;
        ProfilePort port;
        const auto result = prepare_legacy_battle_actor_profile(
            &final_state, &item_effect, 0x005029D0U, port, {}
        );
        test.expect_true(
            result.fallback_writes == 0U &&
                final_state.profile_buffer[3U] == 0xABCD0000U &&
                result.mode_flag_writes == 1U,
            "nonzero profile word suppresses fallback but not final mode publication"
        );
    }

    {
        ProfilePort port;
        LegacyBattleGroupAItemEffectApplicationState item_effect;
        const auto result = prepare_legacy_battle_actor_profile(
            nullptr, &item_effect, 0x005029D0U, port, {}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleActorProfilePreparationStatus::
                        actor_state_typed_stop &&
                port.build_calls == 1U && port.resolve_calls == 0U,
            "missing actor profile owner preserves the local-build prefix then stops at the first actor read"
        );
    }
}
