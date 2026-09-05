#include "legacy_battle_mon_database_fixture.hpp"
#include "openswd3/battle/legacy_battle_actor_profile_preparation.hpp"

#include "test.hpp"

namespace {

using namespace openswd3::battle;
using openswd3::compat::u32;

class ProfilePort final
    : public openswd3::test::LegacyBattleMonDatabaseFixture {
public:
    [[nodiscard]] LegacyBattleMonDatabaseCallReply
    invoke_legacy_battle_mon_database(
        const LegacyBattleMonDatabaseCallRequest& request,
        const std::span<openswd3::compat::u8> destination
    ) override {
        if (request.stream_kind == LegacyBattleMonDatabaseStreamKind::profile &&
            request.call == LegacyBattleMonDatabaseCall::seek_file &&
            request.distance == 0x204U) {
            profile_entry = request;
            if (observed_output != nullptr) {
                output_before_profile = *observed_output;
            }
        }

        auto reply =
            LegacyBattleMonDatabaseFixture::invoke_legacy_battle_mon_database(
                request, destination
            );
        if (request.stream_kind == LegacyBattleMonDatabaseStreamKind::profile &&
            request.call == LegacyBattleMonDatabaseCall::release_stream) {
            reply.ecx = 0x13572468U;
            reply.edx = 0x24681357U;
        }

        return reply;
    }

    [[nodiscard]] LegacyBattleMonDefinitionTextReleaseCallReply
    release_legacy_battle_mon_definition_text(
        const LegacyBattleMonDefinitionTextReleaseCallRequest& request
    ) override {
        if (stop_text_release) {
            return {
                .eax = 0x11111111U,
                .ecx = 0x22222222U,
                .edx = 0x33333333U,
                .typed_stop = true
            };
        }

        return LegacyBattleMonDatabasePort::
            release_legacy_battle_mon_definition_text(request);
    }

    bool stop_text_release{};
    u32* observed_output{};
    u32 output_before_profile{};
    LegacyBattleMonDatabaseCallRequest profile_entry{};
};

void seed_definition(ProfilePort& port) {
    port.definition[0x50U] = 0x34U;
    port.definition[0x51U] = 0x12U;
    port.definition[0x52U] = 0xFEU;
    port.definition[0x53U] = 0xCAU;
    port.definition[0x3EU] = 7U;
    port.definition[0x40U] = 0xEFU;
    port.definition[0x41U] = 0xBEU;
    port.definition[0x34U] = 1U;
    port.definition[0x35U] = 0x80U;
}

LegacyBattleGroupAConfigurationState configuration_with_text() {
    LegacyBattleGroupAConfigurationState configuration;
    configuration.profile_token = 0x71001000U;
    configuration.profile_record[0xA1U] = std::byte{0x10U};
    configuration.profile_record[0xA3U] = std::byte{0x72U};
    configuration.profile_description = {0x61U};
    return configuration;
}

constexpr LegacyBattleActorProfilePreparationRequest request{
    .source_value = 9U,
    .entry_edx = 0xA1B2C3D4U,
    .definition_output_token = 0xFACEA000U,
    .output_token = 0x004FE5D4U,
};

}  // namespace

void test_battle_actor_profile_preparation(openswd3::test::Context& test) {
    for (const u32 gate : {0U, 1U, 0x00010000U}) {
        auto configuration = configuration_with_text();
        LegacyBattleGroupAActionExecutionState actor;
        LegacyBattleGroupAItemEffectApplicationState item;
        item.mode_flags = 0x25U;
        ProfilePort port;
        seed_definition(port);
        port.set_profile_dword(0x0CU, 0xABCD0028U);
        port.set_profile_dword(0x10U, gate);
        u32 output = 0xFFFFFFFFU;
        port.observed_output = &output;
        const auto result = prepare_legacy_battle_actor_profile(
            &configuration, &actor, &item, &output, 0x005029D0U, port, request
        );
        const bool fallback = static_cast<openswd3::compat::u16>(gate) == 0U;
        test.expect_true(
            result.status ==
                    LegacyBattleActorProfilePreparationStatus::completed &&
                result.build_calls == 1U && result.release_calls == 1U &&
                result.profile_load_calls == 1U && result.output_writes == 1U &&
                output == 0x1234U && result.output_value == output &&
                port.output_before_profile == output &&
                result.profile_argument == 0xBEEF0007U &&
                port.profile_entry.ecx == request.output_token &&
                port.profile_entry.edx == 0xBEEF0007U &&
                port.calls.front().ecx == 0U &&
                port.calls.front().edx == request.entry_edx &&
                port.requested_definition_ids == std::vector<u32>{9U} &&
                port.requested_profile_ids ==
                    std::vector<openswd3::compat::u16>{7U} &&
                result.fallback_writes == (fallback ? 1U : 0U) &&
                actor.profile_buffer[3U] ==
                    (fallback ? 0x80010028U : 0xABCD0028U) &&
                actor.profile_buffer[4U] == gate && item.mode_flags == 0xA5U &&
                result.return_eax == 1U &&
                result.return_ecx == (fallback ? 0x13578001U : 0x13572468U) &&
                result.return_edx == 0x24681357U && port.read_calls == 6U &&
                port.definition_text_release_calls == 1U &&
                configuration.profile_description.empty(),
            "profile preparation preserves ordered output, full profile argument and fallback CX"
        );
    }

    {
        auto configuration = configuration_with_text();
        LegacyBattleGroupAActionExecutionState actor;
        actor.profile_buffer[3U] = 0xFFFFFFFFU;
        LegacyBattleGroupAItemEffectApplicationState item;
        ProfilePort port;
        seed_definition(port);
        port.set_profile_dword(0x0CU, 0xABCD0028U);
        port.observed_output = &actor.profile_buffer[3U];
        auto alias_request = request;
        alias_request.output_token = 0x005029D0U + 0x0D9CU;
        const auto result = prepare_legacy_battle_actor_profile(
            &configuration,
            &actor,
            &item,
            &actor.profile_buffer[3U],
            0x005029D0U,
            port,
            alias_request
        );
        test.expect_true(
            result.output_writes == 1U &&
                port.output_before_profile == 0x1234U &&
                actor.profile_buffer[3U] == 0x80010028U,
            "an aliased output DWORD is written before profile load and fallback overwrite it"
        );
    }

    for (const bool missing_output : {false, true}) {
        auto configuration = configuration_with_text();
        LegacyBattleGroupAItemEffectApplicationState item;
        item.mode_flags = 0x25U;
        ProfilePort port;
        seed_definition(port);
        u32 output = 0xFFFFFFFFU;
        const auto result = prepare_legacy_battle_actor_profile(
            &configuration,
            nullptr,
            &item,
            missing_output ? nullptr : &output,
            0x005029D0U,
            port,
            request
        );
        test.expect_true(
            result.status ==
                    (missing_output
                         ? LegacyBattleActorProfilePreparationStatus::
                               output_access_typed_stop
                         : LegacyBattleActorProfilePreparationStatus::
                               profile_load_typed_stop) &&
                result.release_calls == 1U &&
                port.definition_text_release_calls == 1U &&
                result.output_writes == (missing_output ? 0U : 1U) &&
                output == (missing_output ? 0xFFFFFFFFU : 0x1234U) &&
                result.profile_load_calls == (missing_output ? 0U : 1U) &&
                port.read_calls == (missing_output ? 3U : 6U) &&
                result.fallback_writes == 0U && result.mode_flag_writes == 0U &&
                item.mode_flags == 0x25U,
            "output and profile faults retain distinct release and output prefixes"
        );
        if (missing_output) {
            test.expect_true(
                result.return_eax == 0x1234U &&
                    result.return_ecx == request.output_token &&
                    result.return_edx == 0xBEEF0007U,
                "output store fault preserves the three reached caller registers"
            );
        }
    }

    {
        auto configuration = configuration_with_text();
        LegacyBattleGroupAActionExecutionState actor;
        ProfilePort port;
        seed_definition(port);
        u32 output = 0U;
        const auto result = prepare_legacy_battle_actor_profile(
            &configuration, &actor, nullptr, &output, 0x005029D0U, port, request
        );
        test.expect_true(
            result.status ==
                    LegacyBattleActorProfilePreparationStatus::
                        actor_state_typed_stop &&
                output == 0x1234U && result.fallback_writes == 1U &&
                result.mode_flag_writes == 0U && result.return_eax == 1U &&
                result.return_ecx == 0x13578001U &&
                result.return_edx == 0x24681357U,
            "missing mode BYTE stops after the output, profile and fallback CX write"
        );
    }

    {
        auto configuration = configuration_with_text();
        LegacyBattleGroupAActionExecutionState actor;
        LegacyBattleGroupAItemEffectApplicationState item;
        ProfilePort port;
        port.stop_text_release = true;
        u32 output = 0xDEADBEEFU;
        const auto result = prepare_legacy_battle_actor_profile(
            &configuration, &actor, &item, &output, 0x005029D0U, port, request
        );
        test.expect_true(
            result.status ==
                    LegacyBattleActorProfilePreparationStatus::
                        definition_release_typed_stop &&
                result.output_writes == 0U && output == 0xDEADBEEFU &&
                result.profile_load_calls == 0U &&
                result.mode_flag_writes == 0U &&
                result.return_eax == 0x11111111U &&
                result.return_ecx == 0x22222222U &&
                result.return_edx == 0x33333333U &&
                !configuration.profile_description.empty(),
            "text release fault retains callee registers and suppresses every later write"
        );
    }

    {
        auto configuration = configuration_with_text();
        LegacyBattleGroupAActionExecutionState actor;
        LegacyBattleGroupAItemEffectApplicationState item;
        ProfilePort port;
        port.open_succeeds = false;
        port.definition_text_sizes[0x72002000U] = 8U;
        port.legacy_battle_mon_database_state()
            .definition_text_allocation_bytes = 0x20U;
        auto stale_request = request;
        stale_request.initial_definition_bytes[0xA1U] = 0x20U;
        stale_request.initial_definition_bytes[0xA3U] = 0x72U;
        stale_request.initial_definition_bytes[0x50U] = 0xFFU;
        stale_request.initial_definition_bytes[0x51U] = 0x80U;
        stale_request.initial_definition_bytes[0x3EU] = 0xFEU;
        stale_request.initial_definition_bytes[0x41U] = 0xABU;
        stale_request.initial_definition_bytes[0x34U] = 0x34U;
        stale_request.initial_definition_bytes[0x35U] = 0x12U;
        u32 output = 0U;
        const auto result = prepare_legacy_battle_actor_profile(
            &configuration,
            &actor,
            &item,
            &output,
            0x005029D0U,
            port,
            stale_request
        );
        test.expect_true(
            result.status ==
                    LegacyBattleActorProfilePreparationStatus::completed &&
                port.open_calls == 2U && port.read_calls == 0U &&
                output == 0U && result.profile_argument == 0U &&
                result.return_eax == 0U && result.return_ecx == 0x004F0000U &&
                result.return_edx == 0U && item.mode_flags == 0x80U,
            "definition clearing precedes open failure and the caller still runs its suffix"
        );
        test.expect_true(
            port.calls.front().call ==
                    LegacyBattleMonDatabaseCall::query_definition_text_size &&
                port.calls.front().block_token == 0x72002000U &&
                port.calls.front().ecx == request.definition_output_token &&
                port.calls.front().edx == request.entry_edx &&
                port.definition_text_release_calls == 2U &&
                port.legacy_battle_mon_database_state()
                        .definition_text_allocation_bytes == 0x18U,
            "definition loading consumes captured local text before clearing its stack record"
        );
    }
}
