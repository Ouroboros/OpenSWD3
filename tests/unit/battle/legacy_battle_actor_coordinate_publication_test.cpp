#include "openswd3/battle/legacy_battle_actor_coordinate_publication.hpp"

#include <array>
#include <cstddef>
#include <cstring>

#include "test.hpp"

namespace {

using openswd3::battle::LegacyBattleActorCoordinateDestinationRecord;
using openswd3::battle::LegacyBattleActorCoordinateFlags;
using openswd3::battle::LegacyBattleActorCoordinatePublicationRequest;
using openswd3::battle::LegacyBattleActorCoordinatePublicationStatus;
using openswd3::battle::LegacyBattleActorCoordinateSourceRecord;
using openswd3::battle::LegacyBattleActorCoordinatesState;
using openswd3::battle::publish_legacy_battle_actor_coordinates;
using openswd3::battle::view_legacy_battle_actor_coordinates;
using openswd3::compat::u8;
using openswd3::compat::u32;

void seed_records(LegacyBattleActorCoordinatesState& actor) {
    auto* const source_bytes = reinterpret_cast<std::byte*>(
        static_cast<LegacyBattleActorCoordinateSourceRecord*>(&actor)
    );
    auto* const destination_bytes = reinterpret_cast<std::byte*>(
        static_cast<LegacyBattleActorCoordinateDestinationRecord*>(&actor)
    );
    for (u32 index = 0U; index < 0x20U; ++index) {
        source_bytes[index] = static_cast<std::byte>(0x20U + index);
        destination_bytes[index] = static_cast<std::byte>(0xA0U + index);
    }
}

[[nodiscard]] bool
records_equal(const LegacyBattleActorCoordinatesState& actor) {
    return std::memcmp(
               static_cast<const LegacyBattleActorCoordinateSourceRecord*>(
                   &actor
               ),
               static_cast<const LegacyBattleActorCoordinateDestinationRecord*>(
                   &actor
               ),
               0x20U
           ) == 0;
}

[[nodiscard]] bool flags_equal(
    const LegacyBattleActorCoordinateFlags& left,
    const LegacyBattleActorCoordinateFlags& right
) {
    return left.carry == right.carry && left.parity == right.parity &&
        left.auxiliary_carry == right.auxiliary_carry &&
        left.auxiliary_carry_defined == right.auxiliary_carry_defined &&
        left.zero == right.zero && left.sign == right.sign &&
        left.overflow == right.overflow;
}

[[nodiscard]] LegacyBattleActorCoordinatePublicationRequest request() {
    return {
        .actor_token = 0x005029D0U,
        .entry_eax = 0xAAAA1111U,
        .entry_ecx = 0xBBBB2222U,
        .entry_edx = 0xCCCC3333U,
        .entry_esi = 0xDDDD4444U,
        .entry_edi = 0xEEEE5555U,
        .entry_flags = {
            .carry = true,
            .parity = false,
            .auxiliary_carry = true,
            .auxiliary_carry_defined = false,
            .zero = true,
            .sign = false,
            .overflow = true,
        },
    };
}

}  // namespace

void test_battle_actor_coordinate_publication(openswd3::test::Context& test) {
    {
        LegacyBattleActorCoordinatesState actor{};
        seed_records(actor);
        const auto entry = request();

        const auto result = publish_legacy_battle_actor_coordinates(
            view_legacy_battle_actor_coordinates(actor),
            0x1234FEDCU,
            0x56789ABCU,
            entry
        );

        test.expect_true(
            result.status ==
                    LegacyBattleActorCoordinatePublicationStatus::completed &&
                actor.position_x == 0xFEDCU && actor.position_y == 0x9ABCU &&
                records_equal(actor) && result.return_eax == 0xAAAAFEDCU &&
                result.return_edx == 0xCCCC9ABCU && result.return_ecx == 0U &&
                result.return_esi == entry.entry_esi &&
                result.return_edi == entry.entry_edi &&
                result.stack_writes == 2U && result.coordinate_writes == 2U &&
                result.source_dword_reads == 8U &&
                result.destination_dword_writes == 8U &&
                result.stack_reads == 2U &&
                result.stopped_dword_index == 0xFFFFFFFFU &&
                flags_equal(result.flags, entry.entry_flags),
            "coordinate publication replaces AX/DX, writes X then Y, copies the updated source record forward, restores ESI/EDI, drains ECX, and preserves flags"
        );
    }

    {
        LegacyBattleActorCoordinatesState actor{};
        seed_records(actor);
        actor.position_y_write_accessible = false;
        const auto entry = request();

        const auto result = publish_legacy_battle_actor_coordinates(
            view_legacy_battle_actor_coordinates(actor), 0x1111U, 0x2222U, entry
        );

        test.expect_true(
            result.status ==
                    LegacyBattleActorCoordinatePublicationStatus::
                        position_y_write_typed_stop &&
                actor.position_x == 0x1111U && actor.position_y != 0x2222U &&
                result.coordinate_writes == 1U &&
                result.source_dword_reads == 0U &&
                result.return_eax == 0xAAAA1111U &&
                result.return_edx == 0xCCCC2222U &&
                result.return_ecx == entry.entry_ecx &&
                result.return_esi == entry.entry_esi &&
                result.return_edi == entry.entry_edi &&
                flags_equal(result.flags, entry.entry_flags),
            "a Y write fault preserves the committed X write and the pre-LEA register and flag residue"
        );
    }

    {
        LegacyBattleActorCoordinatesState actor{};
        seed_records(actor);
        actor.publication_source_dword_read_accessible[3] = false;
        std::array<std::byte, 0x20> original_destination{};
        std::memcpy(
            original_destination.data(),
            static_cast<LegacyBattleActorCoordinateDestinationRecord*>(&actor),
            original_destination.size()
        );
        const auto entry = request();

        const auto result = publish_legacy_battle_actor_coordinates(
            view_legacy_battle_actor_coordinates(actor), 0x0102U, 0x0304U, entry
        );
        const auto* const source = reinterpret_cast<const std::byte*>(
            static_cast<const LegacyBattleActorCoordinateSourceRecord*>(&actor)
        );
        const auto* const destination = reinterpret_cast<const std::byte*>(
            static_cast<const LegacyBattleActorCoordinateDestinationRecord*>(
                &actor
            )
        );

        test.expect_true(
            result.status ==
                    LegacyBattleActorCoordinatePublicationStatus::
                        source_dword_read_typed_stop &&
                result.stopped_dword_index == 3U &&
                result.source_dword_reads == 3U &&
                result.destination_dword_writes == 3U &&
                std::memcmp(source, destination, 12U) == 0 &&
                std::memcmp(
                    destination + 12U,
                    original_destination.data() + 12U,
                    0x20U - 12U
                ) == 0 &&
                result.return_esi == entry.actor_token + 0x0D5CU &&
                result.return_edi == entry.actor_token + 0x0D7CU &&
                result.return_ecx == 5U,
            "a source fault stops before the current MOVSD and preserves the three-dword destination prefix and REP residue"
        );
    }

    {
        LegacyBattleActorCoordinatesState actor{};
        seed_records(actor);
        actor.publication_destination_dword_write_accessible[3] = false;
        std::array<std::byte, 0x20> original_destination{};
        std::memcpy(
            original_destination.data(),
            static_cast<LegacyBattleActorCoordinateDestinationRecord*>(&actor),
            original_destination.size()
        );
        const auto entry = request();

        const auto result = publish_legacy_battle_actor_coordinates(
            view_legacy_battle_actor_coordinates(actor), 0x0506U, 0x0708U, entry
        );
        const auto* const source = reinterpret_cast<const std::byte*>(
            static_cast<const LegacyBattleActorCoordinateSourceRecord*>(&actor)
        );
        const auto* const destination = reinterpret_cast<const std::byte*>(
            static_cast<const LegacyBattleActorCoordinateDestinationRecord*>(
                &actor
            )
        );

        test.expect_true(
            result.status ==
                    LegacyBattleActorCoordinatePublicationStatus::
                        destination_dword_write_typed_stop &&
                result.stopped_dword_index == 3U &&
                result.source_dword_reads == 4U &&
                result.destination_dword_writes == 3U &&
                std::memcmp(source, destination, 12U) == 0 &&
                std::memcmp(
                    destination + 12U, original_destination.data() + 12U, 4U
                ) == 0 &&
                result.return_esi == entry.actor_token + 0x0D5CU &&
                result.return_edi == entry.actor_token + 0x0D7CU &&
                result.return_ecx == 5U,
            "a destination fault preserves the current source read without committing or advancing the fourth MOVSD"
        );
    }

    {
        LegacyBattleActorCoordinatesState actor{};
        seed_records(actor);
        auto entry = request();
        entry.edi_restore_readable = false;

        const auto result = publish_legacy_battle_actor_coordinates(
            view_legacy_battle_actor_coordinates(actor), 0x1111U, 0x2222U, entry
        );

        test.expect_true(
            result.status ==
                    LegacyBattleActorCoordinatePublicationStatus::
                        edi_restore_typed_stop &&
                records_equal(actor) && result.return_ecx == 0U &&
                result.return_esi == entry.actor_token + 0x0D70U &&
                result.return_edi == entry.actor_token + 0x0D90U &&
                result.stack_reads == 0U,
            "an EDI restore fault keeps the completed copy and the post-REP ESI/EDI residue"
        );
    }

    {
        LegacyBattleActorCoordinatesState actor{};
        seed_records(actor);
        auto entry = request();
        entry.esi_restore_readable = false;

        const auto result = publish_legacy_battle_actor_coordinates(
            view_legacy_battle_actor_coordinates(actor), 0x3333U, 0x4444U, entry
        );

        test.expect_true(
            result.status ==
                    LegacyBattleActorCoordinatePublicationStatus::
                        esi_restore_typed_stop &&
                records_equal(actor) && result.return_ecx == 0U &&
                result.return_esi == entry.actor_token + 0x0D70U &&
                result.return_edi == entry.entry_edi &&
                result.stack_reads == 1U,
            "an ESI restore fault preserves the already restored EDI and the completed coordinate record copy"
        );
    }

    {
        LegacyBattleActorCoordinatesState actor{};
        auto entry = request();
        entry.x_argument_readable = false;
        const auto x_stop = publish_legacy_battle_actor_coordinates(
            view_legacy_battle_actor_coordinates(actor), 1U, 2U, entry
        );
        entry.x_argument_readable = true;
        entry.y_argument_readable = false;
        const auto y_stop = publish_legacy_battle_actor_coordinates(
            view_legacy_battle_actor_coordinates(actor), 1U, 2U, entry
        );
        entry.y_argument_readable = true;
        entry.esi_save_writable = false;
        const auto esi_push_stop = publish_legacy_battle_actor_coordinates(
            view_legacy_battle_actor_coordinates(actor), 1U, 2U, entry
        );
        entry.esi_save_writable = true;
        entry.edi_save_writable = false;
        const auto edi_push_stop = publish_legacy_battle_actor_coordinates(
            view_legacy_battle_actor_coordinates(actor), 1U, 2U, entry
        );
        entry.edi_save_writable = true;
        actor.position_x_write_accessible = false;
        const auto x_write_stop = publish_legacy_battle_actor_coordinates(
            view_legacy_battle_actor_coordinates(actor), 1U, 2U, entry
        );

        test.expect_true(
            x_stop.status ==
                    LegacyBattleActorCoordinatePublicationStatus::
                        x_argument_read_typed_stop &&
                x_stop.return_eax == entry.entry_eax &&
                y_stop.status ==
                    LegacyBattleActorCoordinatePublicationStatus::
                        y_argument_read_typed_stop &&
                y_stop.return_eax == 0xAAAA0001U &&
                y_stop.return_edx == entry.entry_edx &&
                esi_push_stop.status ==
                    LegacyBattleActorCoordinatePublicationStatus::
                        esi_save_typed_stop &&
                esi_push_stop.stack_writes == 0U &&
                edi_push_stop.status ==
                    LegacyBattleActorCoordinatePublicationStatus::
                        edi_save_typed_stop &&
                edi_push_stop.stack_writes == 1U &&
                x_write_stop.status ==
                    LegacyBattleActorCoordinatePublicationStatus::
                        position_x_write_typed_stop &&
                x_write_stop.stack_writes == 2U &&
                x_write_stop.coordinate_writes == 0U &&
                x_write_stop.return_eax == 0xAAAA0001U &&
                x_write_stop.return_edx == 0xCCCC0002U,
            "argument, stack-save, and first-coordinate stops preserve the exact committed register and stack prefix"
        );
    }
}
