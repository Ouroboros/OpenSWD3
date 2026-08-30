#include "openswd3/battle/legacy_battle_group_b_coordinate_offsets.hpp"
#include "test.hpp"

#include <array>

namespace {

using openswd3::compat::u16;
using openswd3::compat::u32;
using openswd3::compat::u8;

void write_word(
    std::array<u8, 0xA4>& bytes, const std::size_t offset, const u16 value
) noexcept {
    bytes[offset] = static_cast<u8>(value);
    bytes[offset + 1U] = static_cast<u8>(value >> 8U);
}

}  // namespace

void test_battle_group_b_coordinate_offsets(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleActorGroupBElementState;
    using openswd3::battle::LegacyBattleGroupBCoordinateOffsetStatus;
    using openswd3::battle::read_legacy_battle_group_b_coordinate_offsets;

    constexpr openswd3::battle::LegacyBattleGroupBCoordinateOffsetRequest
        request{
            .actor_token = 0x0052D680U,
            .first_output_token = 0x0012FF20U,
            .second_output_token = 0x0012FF24U,
            .entry_eax = 0x0012FF20U,
            .entry_edx = 0x0012FF24U,
        };

    {
        u16 first = 0xAAAAU;
        u16 second = 0xBBBBU;
        const auto result = read_legacy_battle_group_b_coordinate_offsets(
            nullptr, &first, &second, request
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBCoordinateOffsetStatus::
                        actor_state_typed_stop &&
                result.outputs_written == 0U && first == 0xAAAAU &&
                second == 0xBBBBU && result.return_eax == 0x0012FF20U &&
                result.return_ecx == 0x0052D680U &&
                result.return_edx == 0x0012FF24U,
            "group B coordinate offsets stop at the first actor resource read"
        );
    }

    {
        LegacyBattleActorGroupBElementState actor{
            .object_token = request.actor_token,
        };
        u16 first = 0xAAAAU;
        u16 second = 0xBBBBU;
        const auto result = read_legacy_battle_group_b_coordinate_offsets(
            &actor, &first, &second, request
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBCoordinateOffsetStatus::
                        resource_read_typed_stop &&
                result.outputs_written == 0U && first == 0xAAAAU &&
                second == 0xBBBBU && result.return_eax == 0U &&
                result.return_ecx == request.actor_token &&
                result.return_edx == request.entry_edx,
            "group B coordinate offsets stop at the first resource word read"
        );
    }

    LegacyBattleActorGroupBElementState actor{
        .object_token = request.actor_token,
        .resource_token = 0x73000148U,
    };
    write_word(actor.resource_bytes, 0x62U, 0xA1B2U);
    write_word(actor.resource_bytes, 0x8AU, 0xC3D4U);

    {
        u16 second = 0xBBBBU;
        const auto result = read_legacy_battle_group_b_coordinate_offsets(
            &actor, nullptr, &second, request
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBCoordinateOffsetStatus::
                        first_output_typed_stop &&
                result.outputs_written == 0U && second == 0xBBBBU &&
                result.first_value == 0xA1B2U &&
                result.return_eax == request.first_output_token &&
                result.return_ecx == request.actor_token &&
                result.return_edx == 0x0012A1B2U,
            "group B coordinate offsets preserve the first write fault registers"
        );
    }

    {
        u16 first = 0xAAAAU;
        const auto result = read_legacy_battle_group_b_coordinate_offsets(
            &actor, &first, nullptr, request
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBCoordinateOffsetStatus::
                        second_output_typed_stop &&
                result.outputs_written == 1U && first == 0xA1B2U &&
                result.second_value == 0xC3D4U &&
                result.return_eax == request.second_output_token &&
                result.return_ecx == actor.resource_token &&
                result.return_edx == 0x0012C3D4U,
            "group B coordinate offsets preserve the first output before the second write fault"
        );
    }

    {
        u16 first = 0U;
        u16 second = 0U;
        const auto result = read_legacy_battle_group_b_coordinate_offsets(
            &actor, &first, &second, request
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBCoordinateOffsetStatus::completed &&
                result.outputs_written == 2U && first == 0xA1B2U &&
                second == 0xC3D4U && result.first_value == first &&
                result.second_value == second &&
                result.return_eax == request.second_output_token &&
                result.return_ecx == actor.resource_token &&
                result.return_edx == 0x0012C3D4U,
            "group B coordinate offsets copy both resource words and preserve terminal registers"
        );
    }
}
