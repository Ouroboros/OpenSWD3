#include "openswd3/battle/legacy_battle_group_a_embedded_profile_application.hpp"

#include "test.hpp"

#include <array>
#include <cstddef>
#include <utility>
#include <vector>

namespace {

using openswd3::battle::LegacyBattleGroupAEmbeddedProfileApplicationPort;
using openswd3::battle::LegacyBattleGroupAEmbeddedProfileItemQuantityReply;
using openswd3::battle::LegacyBattleGroupAEmbeddedProfileItemQuantityRequest;
using openswd3::battle::LegacyBattleGroupASummonProfileRecord;
using openswd3::compat::u8;
using openswd3::compat::u16;
using openswd3::compat::u32;

struct ItemQuantityPort final
    : LegacyBattleGroupAEmbeddedProfileApplicationPort {
    [[nodiscard]] LegacyBattleGroupAEmbeddedProfileItemQuantityReply
    lookup_embedded_profile_item_quantity(
        const LegacyBattleGroupAEmbeddedProfileItemQuantityRequest& request
    ) override {
        requests.push_back(request);
        return reply;
    }

    LegacyBattleGroupAEmbeddedProfileItemQuantityReply reply{};
    std::vector<LegacyBattleGroupAEmbeddedProfileItemQuantityRequest> requests;
};

void set_profile_byte(
    LegacyBattleGroupASummonProfileRecord& profile,
    const std::size_t offset,
    const u8 value
) {
    profile[offset] = static_cast<std::byte>(value);
}

void set_profile_word(
    LegacyBattleGroupASummonProfileRecord& profile,
    const std::size_t offset,
    const u16 value
) {
    set_profile_byte(profile, offset, static_cast<u8>(value));
    set_profile_byte(profile, offset + 1U, static_cast<u8>(value >> 8U));
}

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

[[nodiscard]] u8
actor_byte(const std::array<u32, 14>& actor, const std::size_t offset) {
    return static_cast<u8>(
        actor[offset / 4U] >> static_cast<u32>((offset & 3U) * 8U)
    );
}

[[nodiscard]] u16
actor_word(const std::array<u32, 14>& actor, const std::size_t offset) {
    return static_cast<u16>(actor_byte(actor, offset)) |
        static_cast<u16>(
               static_cast<u16>(actor_byte(actor, offset + 1U)) << 8U
        );
}

}  // namespace

void test_battle_group_a_embedded_profile_application(
    openswd3::test::Context& test
) {
    using openswd3::battle::LegacyBattleGroupAEmbeddedProfileApplicationState;
    using openswd3::battle::LegacyBattleGroupAEmbeddedProfileApplicationStatus;
    using openswd3::battle::LegacyBattleGroupAConfigurationState;
    using openswd3::battle::apply_legacy_battle_group_a_embedded_profile;
    using openswd3::battle::kLegacyBattleEmbeddedProfileItemListToken;

    {
        LegacyBattleGroupAEmbeddedProfileApplicationState state{
            .status_bits = 0xA5B6C780U,
        };
        LegacyBattleGroupAConfigurationState configuration;
        LegacyBattleGroupASummonProfileRecord profile{};
        ItemQuantityPort port;
        set_profile_word(profile, 0x48U, 49U);
        const auto default_result =
            apply_legacy_battle_group_a_embedded_profile(
                nullptr,
                configuration,
                &profile,
                0U,
                0x00502B28U,
                port,
                {.entry_eax = 0x12345678U, .entry_edx = 0x87654321U}
            );

        const std::array<std::pair<u16, u8>, 6> flag_cases{{
            {50U, 0x01U},
            {53U, 0x04U},
            {54U, 0x08U},
            {55U, 0x10U},
            {56U, 0x20U},
            {57U, 0x40U},
        }};
        bool flag_results_match = true;
        for (const auto [profile_kind, mask] : flag_cases) {
            set_profile_word(profile, 0x48U, profile_kind);
            const u32 before = state.status_bits;
            const auto result = apply_legacy_battle_group_a_embedded_profile(
                &state,
                configuration,
                &profile,
                0x005029D0U,
                0x00502B28U,
                port,
                {.entry_eax = 0x12345678U, .entry_edx = 0x87654321U}
            );
            flag_results_match = flag_results_match &&
                result.status ==
                    LegacyBattleGroupAEmbeddedProfileApplicationStatus::
                        completed &&
                result.status_writes == 1U &&
                state.status_bits == (before | mask) &&
                result.return_eax == state.status_bits &&
                result.return_ecx == 0x005029D0U &&
                result.return_edx == 0x87654321U;
        }

        test.expect_true(
            default_result.status ==
                    LegacyBattleGroupAEmbeddedProfileApplicationStatus::
                        completed &&
                default_result.return_eax == 0xFFFFFFFFU &&
                default_result.return_ecx == 0U &&
                default_result.return_edx == 0x87654321U &&
                flag_results_match && state.status_bits == 0xA5B6C7FDU &&
                port.requests.empty(),
            "default kind leaves the actor untouched while six fixed kinds OR only their original low-byte status masks"
        );
    }

    {
        LegacyBattleGroupAEmbeddedProfileApplicationState state;
        LegacyBattleGroupAConfigurationState configuration{
            .actor_record_token = 0x72000000U,
        };
        set_actor_word(configuration.actor_record, 0x26U, 1000U);
        LegacyBattleGroupASummonProfileRecord profile{};
        set_profile_word(profile, 0x48U, 52U);
        set_profile_word(profile, 0x50U, 0x1234U);
        ItemQuantityPort port;
        port.reply = {
            .eax = 0x7100000BU,
            .ecx = 0xDEADBEEFU,
            .edx = 0xCAFEBABEU,
        };

        const auto result = apply_legacy_battle_group_a_embedded_profile(
            &state,
            configuration,
            &profile,
            0x005029D0U,
            0x00502B28U,
            port,
            {.entry_eax = 0x00502B28U, .entry_edx = 0x87654321U}
        );

        test.expect_true(
            result.status ==
                    LegacyBattleGroupAEmbeddedProfileApplicationStatus::
                        completed &&
                result.port_calls == 1U && result.actor_word_writes == 1U &&
                actor_word(configuration.actor_record, 0x26U) == 1150U &&
                port.requests.size() == 1U &&
                port.requests[0U].item_list_token ==
                    kLegacyBattleEmbeddedProfileItemListToken &&
                port.requests[0U].item_id == 0x1234U &&
                port.requests[0U].eax == 0x1234U &&
                port.requests[0U].ecx == 0x005029D0U &&
                port.requests[0U].edx == 0x87654321U &&
                result.return_eax == 0x00000AF0U && result.return_ecx == 0U &&
                result.return_edx == 0x0050047EU,
            "word kind adds ten percent plus half the queried quantity percentage and preserves the profile-token high word in edx"
        );
    }

    {
        LegacyBattleGroupAEmbeddedProfileApplicationState state;
        LegacyBattleGroupAConfigurationState configuration{
            .actor_record_token = 0x72000000U,
        };
        set_actor_byte(configuration.actor_record, 0x2FU, 10U);
        LegacyBattleGroupASummonProfileRecord profile{};
        set_profile_word(profile, 0x48U, 51U);
        set_profile_word(profile, 0x50U, 0x2222U);
        set_profile_byte(profile, 0x94U, 3U);
        ItemQuantityPort port;
        port.reply = {
            .eax = 0x71000032U,
            .ecx = 0x11223344U,
            .edx = 0x55667788U,
        };

        const auto result = apply_legacy_battle_group_a_embedded_profile(
            &state,
            configuration,
            &profile,
            0x005029D0U,
            0x00502B28U,
            port,
            {.entry_eax = 0x00502B28U, .entry_edx = 0xAABBCCDDU}
        );

        test.expect_true(
            result.status ==
                    LegacyBattleGroupAEmbeddedProfileApplicationStatus::
                        completed &&
                result.bytes_scanned == 3U &&
                result.modified_byte_index == 2U &&
                result.actor_byte_writes == 1U &&
                actor_byte(configuration.actor_record, 0x2FU) == 114U &&
                port.requests.size() == 1U && port.requests[0U].eax == 1U &&
                port.requests[0U].ecx == 0x005029D0U &&
                port.requests[0U].edx == 0xAABB2222U &&
                result.return_eax == 0x99990072U && result.return_ecx == 2U &&
                result.return_edx == 0xFFFFE668U,
            "byte kind stops at the first nonzero byte and preserves both low-byte product truncations and wrapped negative delta"
        );
    }

    {
        LegacyBattleGroupAConfigurationState configuration;
        LegacyBattleGroupASummonProfileRecord profile{};
        set_profile_word(profile, 0x48U, 51U);
        set_profile_word(profile, 0x50U, 0x3333U);
        ItemQuantityPort port;
        port.reply = {
            .eax = 0xAABB0014U,
            .ecx = 0x11223344U,
            .edx = 0x55667788U,
        };

        const auto result = apply_legacy_battle_group_a_embedded_profile(
            nullptr, configuration, &profile, 0x005029D0U, 0x00502B28U, port
        );

        test.expect_true(
            result.status ==
                    LegacyBattleGroupAEmbeddedProfileApplicationStatus::
                        completed &&
                result.bytes_scanned == 9U && result.actor_byte_writes == 0U &&
                result.return_eax == 8U && result.return_ecx == 9U &&
                result.return_edx == 0x55667700U,
            "nine zero bytes return without dereferencing the missing actor record and retain the query edx upper bytes"
        );
    }

    {
        LegacyBattleGroupAEmbeddedProfileApplicationState state;
        LegacyBattleGroupAConfigurationState configuration;
        LegacyBattleGroupASummonProfileRecord profile{};
        ItemQuantityPort profile_port;
        const auto profile_stop = apply_legacy_battle_group_a_embedded_profile(
            &state,
            configuration,
            nullptr,
            0x005029D0U,
            0U,
            profile_port,
            {.entry_eax = 0x12345678U, .entry_edx = 0x87654321U}
        );

        set_profile_word(profile, 0x48U, 50U);
        ItemQuantityPort actor_port;
        const auto actor_stop = apply_legacy_battle_group_a_embedded_profile(
            nullptr,
            configuration,
            &profile,
            0U,
            0x00502B28U,
            actor_port,
            {.entry_eax = 0x12345678U, .entry_edx = 0x87654321U}
        );

        set_profile_word(profile, 0x48U, 52U);
        set_profile_word(profile, 0x50U, 0x1234U);
        ItemQuantityPort nonzero_record_port;
        nonzero_record_port.reply = {
            .eax = 0x7100000BU,
            .ecx = 0x11223344U,
            .edx = 0x55667788U,
        };
        const auto nonzero_record_stop =
            apply_legacy_battle_group_a_embedded_profile(
                &state,
                configuration,
                &profile,
                0x005029D0U,
                0x00502B28U,
                nonzero_record_port
            );

        ItemQuantityPort zero_record_port;
        zero_record_port.reply = {
            .eax = 0x71000001U,
            .ecx = 0x11223344U,
            .edx = 0x55667788U,
        };
        const auto zero_record_stop =
            apply_legacy_battle_group_a_embedded_profile(
                &state,
                configuration,
                &profile,
                0x005029D0U,
                0x00502B28U,
                zero_record_port
            );

        test.expect_true(
            profile_stop.status ==
                    LegacyBattleGroupAEmbeddedProfileApplicationStatus::
                        profile_typed_stop &&
                profile_stop.return_eax == 0x12345678U &&
                profile_stop.return_ecx == 0x005029D0U &&
                profile_stop.return_edx == 0x87654321U &&
                actor_stop.status ==
                    LegacyBattleGroupAEmbeddedProfileApplicationStatus::
                        actor_state_typed_stop &&
                actor_stop.return_eax == 0U &&
                nonzero_record_stop.status ==
                    LegacyBattleGroupAEmbeddedProfileApplicationStatus::
                        actor_record_typed_stop &&
                nonzero_record_stop.return_eax == 5U &&
                nonzero_record_stop.return_ecx == 0U &&
                nonzero_record_stop.return_edx == 0U &&
                zero_record_stop.status ==
                    LegacyBattleGroupAEmbeddedProfileApplicationStatus::
                        actor_record_typed_stop &&
                zero_record_stop.return_eax == 0x71000000U &&
                zero_record_stop.return_ecx == 0x11223344U &&
                zero_record_stop.return_edx == 0x55667788U,
            "typed stops occur only at the original profile, actor, and rate-dependent actor-record accesses"
        );
    }
}
