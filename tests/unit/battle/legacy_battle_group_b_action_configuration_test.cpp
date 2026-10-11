#include "legacy_battle_mon_database_fixture.hpp"
#include "openswd3/battle/legacy_battle_group_b_action_configuration.hpp"
#include "test.hpp"

#include <array>
#include <cstddef>
#include <cstring>
#include <memory>

namespace {

using openswd3::battle::LegacyBattleActorCoordinateDestinationRecord;
using openswd3::battle::LegacyBattleActorCoordinateSourceRecord;
using openswd3::battle::LegacyBattleActorGroupBElementState;
using openswd3::battle::LegacyBattleGroupBActionConfigurationStatus;
using openswd3::battle::LegacyBattleGroupBActionRecord;
using openswd3::compat::u8;
using openswd3::compat::u16;
using openswd3::compat::u32;

class Port final : public openswd3::test::LegacyBattleMonDatabaseFixture {
public:
    openswd3::battle::LegacyBattleMonStreamAllocation
    allocate_mon_stream(const u32 size) override {
        if (drawing_mirror != nullptr) {
            observed_placement_prefix = observed_placement_prefix &&
                *drawing_mirror == expected_mirror &&
                placement_actor->action_record.position_x ==
                    expected_position_x &&
                placement_actor->action_record.runtime_value == 0U;
        }

        return LegacyBattleMonDatabaseFixture::allocate_mon_stream(size);
    }

    void release_mon_text(const u32 block_token) override {
        if (drawing_suppression != nullptr) {
            observed_original_suppression = observed_original_suppression &&
                *drawing_suppression == 0xFFFFFFFFU;
        }

        released_text_token = block_token;
        ++definition_release_calls;
        LegacyBattleMonDatabaseFixture::release_mon_text(block_token);
    }

    const u32* drawing_mirror{};
    const LegacyBattleActorGroupBElementState* placement_actor{};
    u32 expected_mirror{};
    u16 expected_position_x{};
    bool observed_placement_prefix{true};
    const u32* drawing_suppression{};
    bool observed_original_suppression{true};
    u32 released_text_token{};
    u32 definition_release_calls{};
};

void write_word(
    std::array<u8, 0xA4U>& bytes, const std::size_t offset, const u16 value
) {
    bytes[offset] = static_cast<u8>(value);
    bytes[offset + 1U] = static_cast<u8>(value >> 8U);
}

[[nodiscard]] u32
read_dword(const std::array<u8, 0xA4U>& bytes, const std::size_t offset) {
    return static_cast<u32>(bytes[offset]) |
        (static_cast<u32>(bytes[offset + 1U]) << 8U) |
        (static_cast<u32>(bytes[offset + 2U]) << 16U) |
        (static_cast<u32>(bytes[offset + 3U]) << 24U);
}

[[nodiscard]] std::shared_ptr<const std::array<u8, 0xA4U>> resource_snapshot() {
    auto bytes = std::make_shared<std::array<u8, 0xA4U>>();
    (*bytes)[0x20U] = 0x20U;
    write_word(*bytes, 0x50U, 0U);
    write_word(*bytes, 0x56U, 0xFFF8U);
    write_word(*bytes, 0x5AU, 0xFFFDU);
    write_word(*bytes, 0x60U, 0x2468U);
    write_word(*bytes, 0x64U, 0xFF80U);
    (*bytes)[0x90U] = 0x7AU;
    return bytes;
}

[[nodiscard]] LegacyBattleGroupBActionRecord source_record() {
    LegacyBattleGroupBActionRecord source;
    for (std::size_t index = 0U; index < source.prefix.size(); ++index) {
        source.prefix[index] = static_cast<std::byte>(0x40U + index);
    }
    source.action_id = 0x1234U;
    source.position_x = 0x2345U;
    source.position_y = 0x3456U;
    source.reserved_1a = 0x4567U;
    source.runtime_value = 0xABCD5678U;
    return source;
}

}  // namespace

void test_battle_group_b_action_configuration(openswd3::test::Context& test) {
    using openswd3::battle::configure_legacy_battle_group_b_action;

    for (const u32 index : {0U, 7U}) {
        for (const bool mirrored : {false, true}) {
            for (const bool suppress_drawing : {false, true}) {
                for (const u32 failure : {0U, 1U, 2U}) {
                    LegacyBattleActorGroupBElementState actor{
                        .object_token = 0x00525508U + index * 0x2B28U,
                        .resource_token = 0x73000000U + index * 0xA4U,
                    };

                    actor.runtime_reset.field_2af0 = 0xFFFFFFFFU;
                    actor.runtime_reset.field_2af4 = 0x89ABCDEFU;
                    Port port;
                    port.definition = *resource_snapshot();
                    port.definition_description = {0x41U, 0x42U};
                    port.drawing_suppression = &actor.runtime_reset.field_2af0;
                    if (failure == 1U) {
                        port.allocation_succeeds = false;
                    } else if (failure == 2U) {
                        port.allocation_results = {true, false};
                    }

                    u32 drawing_mirror = 0xFFFFFFFFU;
                    port.drawing_mirror = &drawing_mirror;
                    port.placement_actor = &actor;
                    port.expected_mirror = mirrored ? 1U : 0xFFFFFFFFU;
                    port.expected_position_x = mirrored ? 0x250U : 0x30U;
                    const auto result = openswd3::battle::
                        configure_legacy_battle_group_b_startup_placement(
                            actor,
                            {
                                .role_id = 0x11U,
                                .position_x = 0x30U,
                                .position_y = 0x50U,
                                .mirrored = mirrored,
                                .suppress_drawing = suppress_drawing,
                            },
                            port,
                            drawing_mirror,
                            0x005213A0U + index * 0x20U
                        );
                    test.expect_true(
                        result.status ==
                                (failure == 0U
                                     ? LegacyBattleGroupBActionConfigurationStatus::
                                           completed
                                     : failure == 1U
                                     ? LegacyBattleGroupBActionConfigurationStatus::
                                           resource_load_typed_stop
                                     : LegacyBattleGroupBActionConfigurationStatus::
                                           profile_load_typed_stop) &&
                            actor.runtime_reset.field_2af0 ==
                                (failure == 0U && suppress_drawing
                                     ? 1U
                                     : 0xFFFFFFFFU) &&
                            drawing_mirror == port.expected_mirror &&
                            port.observed_placement_prefix &&
                            actor.runtime_reset.field_2af4 == 0x89ABCDEFU &&
                            actor.action_record.position_x ==
                                (mirrored ? 0x250U : 0x30U) &&
                            actor.action_record.position_y == 0x50U &&
                            actor.action_record.runtime_value == 0U &&
                            port.observed_original_suppression &&
                            (failure != 0U ||
                             port.definition_release_calls == 1U),
                        "enemy placement writes full drawing suppression only after successful configuration and preserves the prior value on failure or disabled flag"
                    );
                }
            }
        }
    }

    {
        LegacyBattleActorGroupBElementState actor{};
        Port port;
        const auto result = configure_legacy_battle_group_b_action(
            &actor, nullptr, port, 0xBEEF0011U, 0x00525508U, 0x005213A0U
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionConfigurationStatus::
                        source_record_typed_stop &&
                port.open_calls == 0U,
            "group B action configuration stops before all ports for a null source"
        );
    }

    {
        const auto source = source_record();
        LegacyBattleActorGroupBElementState actor{
            .object_token = 0x00525508U,
            .resource_token = 0x73000000U,
        };
        Port port;
        port.profile.fill(std::byte{0x6AU});
        port.definition = *resource_snapshot();
        port.definition_description = {0x41U, 0x42U};
        const auto result = configure_legacy_battle_group_b_action(
            &actor, &source, port, 0xBEEF0011U, actor.object_token, 0x005213A0U
        );
        const auto& state = actor.action_configuration;
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionConfigurationStatus::completed &&
                result.copied_dwords == 16U && port.open_calls == 1U &&
                port.seek_calls == 6U && port.read_calls == 6U &&
                port.release_calls == 2U &&
                port.definition_release_calls == 1U &&
                port.released_text_token == 0x72000000U &&
                std::memcmp(
                    static_cast<const LegacyBattleActorCoordinateSourceRecord*>(
                        &actor.action_execution
                    ),
                    &source,
                    sizeof(source)
                ) == 0 &&
                std::memcmp(
                    static_cast<
                        const LegacyBattleActorCoordinateDestinationRecord*>(
                        &actor.action_execution
                    ),
                    &source,
                    sizeof(source)
                ) == 0 &&
                state.source_runtime_value == 0xABCD5678U &&
                state.resource_mode == 0x7AU &&
                actor.action_execution.position_x == 0x2345U &&
                actor.action_execution.position_y == 0x3456U &&
                actor.action_execution.alternate_position_x == 0x2345U &&
                actor.action_execution.alternate_position_y == 0x3456U &&
                actor.action_execution.profile_value == 0x1234U &&
                state.timing_value == 0U,
            "group B action configuration copies state and loads the typed MON profile"
        );
        test.expect_true(
            read_dword(actor.resource_bytes, 0x4CU) == 0xFFFFFF80U &&
                read_dword(actor.resource_bytes, 0xA0U) == 0U &&
                actor.resource_description.empty(),
            "group B action configuration preserves resource arithmetic and released description storage"
        );
    }

    {
        const auto source = source_record();
        LegacyBattleActorGroupBElementState actor{
            .object_token = 0x00525508U,
            .resource_token = 0x73000000U,
        };
        Port port;
        port.allocation_succeeds = false;
        const auto result = configure_legacy_battle_group_b_action(
            &actor, &source, port, 7U, actor.object_token, 0x005213A0U
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionConfigurationStatus::
                        resource_load_typed_stop &&
                actor.action_execution.position_x == 0x2345U &&
                actor.action_execution.position_y == 0x3456U &&
                actor.action_execution.alternate_position_x == 0x2345U &&
                actor.action_execution.alternate_position_y == 0x3456U,
            "group B action configuration keeps both copied coordinate pairs before a later stop"
        );
    }

    {
        const auto source = source_record();
        LegacyBattleActorGroupBElementState actor{
            .object_token = 0x00525508U,
            .resource_token = 0x73000000U,
        };
        Port port;
        port.definition = *resource_snapshot();
        port.allocation_results = {true, false};
        const auto result = configure_legacy_battle_group_b_action(
            &actor, &source, port, 7U, actor.object_token, 0x005213A0U
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionConfigurationStatus::
                        profile_load_typed_stop &&
                port.release_calls == 1U,
            "group B action configuration stops at the MON zero-allocation access point"
        );
    }
}
