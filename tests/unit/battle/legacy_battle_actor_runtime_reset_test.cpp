#include "openswd3/battle/legacy_battle_actor_runtime_reset.hpp"

#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "openswd3/battle/legacy_battle_actor_lifecycle.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"
#include "openswd3/battle/legacy_battle_status_indicator.hpp"
#include "test.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstring>
#include <memory>
#include <span>

namespace {

using openswd3::battle::LegacyBattleActorRuntimeResetOwners;
using openswd3::battle::LegacyBattleActorRuntimeResetRequest;
using openswd3::battle::LegacyBattleActorRuntimeResetStatus;
using openswd3::compat::u32;

class Random final : public openswd3::battle::LegacyBattleBoundedRandomPort {
public:
    u32 value{};
    u32 calls{};
    u32 last_bound{};

    [[nodiscard]] u32 random_bounded(const u32 bound) override {
        ++calls;
        last_bound = bound;
        return value;
    }
};

struct Fixture {
    std::unique_ptr<openswd3::battle::LegacyBattleStartupState> startup{
        std::make_unique<openswd3::battle::LegacyBattleStartupState>()
    };
    std::unique_ptr<openswd3::battle::LegacyBattleActionDispatchState> action{
        std::make_unique<openswd3::battle::LegacyBattleActionDispatchState>()
    };

    Fixture() {
        startup->group_b_lifecycle = std::make_shared<std::array<
            openswd3::battle::LegacyBattleActorGroupBElementState,
            openswd3::battle::kLegacyBattleActorGroupBElementCount>>();
    }

    [[nodiscard]] LegacyBattleActorRuntimeResetOwners owners() noexcept {
        return {.action = action.get(), .startup = startup.get()};
    }
};

[[nodiscard]] LegacyBattleActorRuntimeResetRequest
request(const u32 token) noexcept {
    return {
        .actor_token = token,
        .entry_eax = 0x11223344U,
        .entry_edx = 0x55667788U,
        .entry_ebx = 0x99AABBCCU,
        .entry_ebp = 0x12345678U,
        .entry_esi = 0x76543210U,
        .entry_edi = 0x0BADF00DU,
        .entry_esp = 0x0012FF00U,
        .entry_return_address = 0x004527F8U,
        .entry_flags =
            {
                .carry = true,
                .parity = false,
                .auxiliary_carry = true,
                .auxiliary_carry_defined = true,
                .zero = false,
                .sign = true,
                .overflow = true,
            },
        .entry_flags_known = true,
    };
}

void fill_group_a(Fixture& fixture, const std::size_t index) {
    auto& party = fixture.startup->party[index];
    auto& execution = fixture.action->group_a_action_execution[index];
    auto& residual = (*fixture.startup->group_a_runtime_reset)[index];
    std::fill(
        residual.bytes_0174_029f.begin(),
        residual.bytes_0174_029f.end(),
        std::byte{0x5A}
    );
    std::fill(
        residual.bytes_0d34_0d4f.begin(),
        residual.bytes_0d34_0d4f.end(),
        std::byte{0x6B}
    );
    std::fill(
        party.final_processing.profile_buffer.begin(),
        party.final_processing.profile_buffer.end(),
        0x7CU
    );
    std::fill(
        party.final_processing.pre_effect_words.begin(),
        party.final_processing.pre_effect_words.end(),
        0x8D8DU
    );
    std::fill(
        reinterpret_cast<std::byte*>(
            static_cast<openswd3::battle::LegacyBattleActorActionRecordSlots*>(
                &execution
            )
        ),
        reinterpret_cast<std::byte*>(
            static_cast<openswd3::battle::LegacyBattleActorActionRecordSlots*>(
                &execution
            )
        ) + sizeof(openswd3::battle::LegacyBattleActorActionRecordSlots),
        std::byte{0x9E}
    );
    party.position_x = 11;
    party.position_y = 12;
    party.alternate_position_x = 21;
    party.alternate_position_y = 22;
    party.progress.scene_identity = 1U;
    party.progress.mode_gate = 0xA5A51234U;
    party.configuration.source_runtime_value = 0U;
    party.final_processing.actor_flags = 8U;
    execution.special_effect_direct_mode = 8U;
    execution.special_particle_coordinate_suppression = 4U;
    execution.action_twenty_seven_motion_mode = 1U;
}

[[nodiscard]] bool all_zero(const auto& values) {
    return std::all_of(values.begin(), values.end(), [](const auto value) {
        return value == decltype(value){};
    });
}

}  // namespace

void test_battle_actor_runtime_reset(openswd3::test::Context& test) {
    {
        Fixture fixture;
        const u32 group_a =
            openswd3::battle::kLegacyBattleActorGroupABaseToken +
            3U * openswd3::battle::kLegacyBattleActorGroupAElementSize;
        const u32 group_b =
            openswd3::battle::kLegacyBattleActorGroupBBaseToken +
            5U * openswd3::battle::kLegacyBattleActorGroupBElementSize;
        const auto a =
            openswd3::battle::resolve_legacy_battle_actor_runtime_reset(
                fixture.owners(), group_a
            );
        const auto b =
            openswd3::battle::resolve_legacy_battle_actor_runtime_reset(
                fixture.owners(), group_b
            );
        const auto invalid =
            openswd3::battle::resolve_legacy_battle_actor_runtime_reset(
                fixture.owners(), group_a + 1U
            );
        test.expect_true(
            a.residual == &(*fixture.startup->group_a_runtime_reset)[3U] &&
                a.action_execution ==
                    &fixture.action->group_a_action_execution[3U] &&
                a.shared_action == &fixture.action->group_a_action_shared &&
                a.particle_source_token_owner ==
                    &fixture.action->group_a_target_phases[3U]
                         .decoded_resource_token &&
                a.particle_phase_owner ==
                    &fixture.action->group_a_target_phases[3U] &&
                a.primary_coordinates == &fixture.startup->party[3U] &&
                a.coordinate_alias ==
                    &fixture.action->group_a_action_execution[3U] &&
                a.actor_resource_token_owner ==
                    &fixture.startup->party[3U].configuration.profile_token &&
                a.actor_resource_bytes ==
                    reinterpret_cast<const openswd3::compat::u8*>(
                        fixture.startup->party[3U]
                            .configuration.profile_record.data()
                    ) &&
                a.actor_resource_size == 0xA4U &&
                b.residual ==
                    &(*fixture.startup->group_b_lifecycle)[5U].runtime_reset &&
                b.action_execution ==
                    &(*fixture.startup->group_b_lifecycle)[5U]
                         .action_execution &&
                b.shared_action == &fixture.action->group_a_action_shared &&
                b.particle_source_token_owner ==
                    &(*fixture.action->group_b_fixed_particle_phases)[5U]
                         .decoded_resource_token &&
                b.particle_phase_owner ==
                    &(*fixture.action->group_b_fixed_particle_phases)[5U] &&
                fixture.action->group_b_target_phases[5U][0U] == nullptr &&
                b.actor_resource_token_owner ==
                    &(*fixture.startup->group_b_lifecycle)[5U].resource_token &&
                b.actor_resource_bytes ==
                    (*fixture.startup->group_b_lifecycle)[5U]
                        .resource_bytes.data() &&
                b.actor_resource_size == 0xA4U &&
                b.live_record_group_b_elements ==
                    fixture.startup->group_b_lifecycle->data() &&
                b.live_record_group_b_count ==
                    fixture.startup->group_b_lifecycle->size() &&
                b.live_record_group_b_base_token ==
                    openswd3::battle::
                        kLegacyBattleActorGroupBExternalSourceBaseToken &&
                invalid.residual == nullptr,
            "runtime reset resolves only canonical Group-A and Group-B owners"
        );
    }

    {
        Fixture fixture;
        const auto a =
            openswd3::battle::resolve_legacy_battle_actor_runtime_reset(
                fixture.owners(),
                openswd3::battle::kLegacyBattleActorGroupABaseToken
            );
        const auto b =
            openswd3::battle::resolve_legacy_battle_actor_runtime_reset(
                fixture.owners(),
                openswd3::battle::kLegacyBattleActorGroupBBaseToken
            );
        a.particle_phase_owner->block_0df4[0U] = 0xAABBCCDDU;
        b.particle_phase_owner->block_0df4[0U] = 0x11223344U;
        b.particle_phase_owner->block_0df4[7U] = 0x55667788U;
        openswd3::battle::LegacyBattleActorImage b_image{};
        openswd3::battle::materialize_legacy_battle_actor_image(b, b_image);
        u32 first{};
        u32 last{};
        std::memcpy(&first, b_image.data() + 0x0DF4U, sizeof(first));
        std::memcpy(&last, b_image.data() + 0x0E10U, sizeof(last));
        const u32 second_write = 0xA1B2C3D4U;
        std::memcpy(
            b_image.data() + 0x0DF8U, &second_write, sizeof(second_write)
        );
        openswd3::battle::synchronize_legacy_battle_actor_image_write(
            b, b_image, 0x0DF8U, sizeof(second_write)
        );
        test.expect_true(
            first == 0x11223344U && last == 0x55667788U &&
                b.particle_phase_owner->block_0df4[1U] == second_write &&
                b.particle_phase_owner->block_0df4[0U] == 0x11223344U &&
                b.particle_phase_owner->block_0df4[7U] == 0x55667788U &&
                a.particle_phase_owner->block_0df4[0U] == 0xAABBCCDDU &&
                fixture.action->group_b_target_phases[0U][0U] == nullptr,
            "Group-B fixed emitter 32-byte middle region uses its own canonical owner, not target-grid or Group-A memory"
        );
    }

    {
        Fixture fixture;
        bool resource_mapping_exact = true;
        for (const u32 token : std::array<u32, 2U>{
                 openswd3::battle::kLegacyBattleActorGroupABaseToken,
                 openswd3::battle::kLegacyBattleActorGroupBBaseToken
             }) {
            const auto actor =
                openswd3::battle::resolve_legacy_battle_actor_runtime_reset(
                    fixture.owners(), token
                );
            *actor.actor_resource_token_owner = 0x00720000U;
            openswd3::battle::LegacyBattleActorImage image{};
            openswd3::battle::materialize_legacy_battle_actor_image(
                actor, image
            );
            u32 read_token{};
            std::memcpy(&read_token, image.data() + 0x0CU, sizeof(read_token));
            resource_mapping_exact =
                resource_mapping_exact && read_token == 0x00720000U;
            const u32 new_token = 0x00730000U;
            std::memcpy(image.data() + 0x0CU, &new_token, sizeof(new_token));
            openswd3::battle::synchronize_legacy_battle_actor_image_write(
                actor, image, 0x0CU, sizeof(new_token)
            );
            resource_mapping_exact = resource_mapping_exact &&
                *actor.actor_resource_token_owner == new_token;
        }
        test.expect_true(
            resource_mapping_exact,
            "Group-A and Group-B actor+0x0C resource tokens materialize and commit to independent canonical 0xA4 record owners"
        );
    }

    {
        Fixture fixture;
        bool frame_source_alias_exact = true;
        for (const u32 token : std::array<u32, 2U>{
                 openswd3::battle::kLegacyBattleActorGroupABaseToken,
                 openswd3::battle::kLegacyBattleActorGroupBBaseToken
             }) {
            const auto actor =
                openswd3::battle::resolve_legacy_battle_actor_runtime_reset(
                    fixture.owners(), token
                );
            actor.action_execution->render_source_token = 0x00710000U;
            actor.action_execution->resource.token = 0x00710000U;
            actor.action_execution->resource.value_00_known = true;
            actor.action_execution->resource.value_0c_known = true;
            actor.action_execution->resource.value_0e_known = true;
            openswd3::battle::LegacyBattleActorImage image{};
            openswd3::battle::materialize_legacy_battle_actor_image(
                actor, image
            );
            const u32 unchanged = 0x00710000U;
            std::memcpy(image.data() + 0x2548U, &unchanged, sizeof(unchanged));
            openswd3::battle::synchronize_legacy_battle_actor_image_write(
                actor, image, 0x2548U, sizeof(unchanged)
            );
            frame_source_alias_exact = frame_source_alias_exact &&
                actor.action_execution->resource.value_00_known &&
                actor.action_execution->resource.value_0c_known &&
                actor.action_execution->resource.value_0e_known;
            const u32 changed = 0x00720000U;
            std::memcpy(image.data() + 0x2548U, &changed, sizeof(changed));
            openswd3::battle::synchronize_legacy_battle_actor_image_write(
                actor, image, 0x2548U, sizeof(changed)
            );
            frame_source_alias_exact = frame_source_alias_exact &&
                actor.action_execution->render_source_token == changed &&
                actor.action_execution->resource.token == changed &&
                !actor.action_execution->resource.value_00_known &&
                !actor.action_execution->resource.value_0c_known &&
                !actor.action_execution->resource.value_0e_known;
        }
        test.expect_true(
            frame_source_alias_exact,
            "Group-A and Group-B +0x2548 token writes preserve a matching resource header but invalidate an unrelated first-dword cache"
        );
    }

    {
        Fixture fixture;
        const u32 token = openswd3::battle::kLegacyBattleActorGroupABaseToken;
        const auto actor =
            openswd3::battle::resolve_legacy_battle_actor_runtime_reset(
                fixture.owners(), token
            );
        auto& phase = *actor.particle_phase_owner;
        std::array<openswd3::compat::u16, 2U> pixels{1U, 2U};
        phase.decoded_resource_token = 0x00811000U;
        phase.emitter.source_pixels = std::span{pixels};
        phase.emitter.source_width = 19U;
        phase.emitter.source_height = 23U;
        phase.emitter.flags = 0x56U;
        phase.emitter.head_token = 0x00710000U;
        phase.emitter.tail_token = 0x00720000U;
        openswd3::battle::LegacyBattleActorImage image{};
        openswd3::battle::materialize_legacy_battle_actor_image(actor, image);
        u32 source{};
        openswd3::compat::u16 width{};
        openswd3::compat::u16 height{};
        std::memcpy(&source, image.data() + 0x0E14U, sizeof(source));
        std::memcpy(&width, image.data() + 0x0E18U, sizeof(width));
        std::memcpy(&height, image.data() + 0x0E1AU, sizeof(height));
        bool emitter_mapping_exact = source == 0x00811000U && width == 19U &&
            height == 23U &&
            static_cast<unsigned char>(image[0x0E3CU]) == 0x56U;
        const u32 zero{};
        std::memcpy(image.data() + 0x0E14U, &zero, sizeof(zero));
        openswd3::battle::synchronize_legacy_battle_actor_image_write(
            actor, image, 0x0E14U, sizeof(zero)
        );
        std::memcpy(image.data() + 0x0E18U, &zero, sizeof(zero));
        openswd3::battle::synchronize_legacy_battle_actor_image_write(
            actor, image, 0x0E18U, sizeof(zero)
        );
        std::memcpy(image.data() + 0x0E3CU, &zero, sizeof(zero));
        openswd3::battle::synchronize_legacy_battle_actor_image_write(
            actor, image, 0x0E3CU, sizeof(zero)
        );
        std::memcpy(image.data() + 0x0E64U, &zero, sizeof(zero));
        openswd3::battle::synchronize_legacy_battle_actor_image_write(
            actor, image, 0x0E64U, sizeof(zero)
        );
        emitter_mapping_exact = emitter_mapping_exact &&
            phase.decoded_resource_token == 0U &&
            phase.emitter.source_pixels.empty() &&
            phase.emitter.source_width == 0U &&
            phase.emitter.source_height == 0U && phase.emitter.flags == 0U &&
            phase.emitter.head_token == 0U &&
            phase.emitter.tail_token == 0x00720000U;
        test.expect_true(
            emitter_mapping_exact,
            "Group-A target-phase emitter materializes and synchronizes individual +0x0E14..+0x0E6B physical writes without clearing later fields"
        );
    }

    {
        Fixture fixture;
        bool owner_mapping_exact = true;
        for (const u32 token : std::array<u32, 2U>{
                 openswd3::battle::kLegacyBattleActorGroupABaseToken,
                 openswd3::battle::kLegacyBattleActorGroupBBaseToken
             }) {
            const auto actor =
                openswd3::battle::resolve_legacy_battle_actor_runtime_reset(
                    fixture.owners(), token
                );
            actor.base_initialization->linked_action_head_token = 0x12345678U;
            actor.action_execution->early_latch = 0x23456789U;
            actor.progress->frame_started = 0x3456789AU;
            openswd3::battle::LegacyBattleActorImage image{};
            openswd3::battle::materialize_legacy_battle_actor_image(
                actor, image
            );
            const auto read_dword = [&](const std::size_t offset) {
                u32 value{};
                std::memcpy(&value, image.data() + offset, sizeof(value));
                return value;
            };
            owner_mapping_exact = owner_mapping_exact &&
                image.size() == 0x2B24U && read_dword(0x2584U) == 0x12345678U &&
                read_dword(0x2B1CU) == 0x23456789U &&
                read_dword(0x2B20U) == 0x3456789AU;

            const u32 new_head = 0x456789ABU;
            const u32 new_latch = 0x56789ABCU;
            const u32 new_started = 0x6789ABCDU;
            std::memcpy(image.data() + 0x2584U, &new_head, sizeof(new_head));
            std::memcpy(image.data() + 0x2B1CU, &new_latch, sizeof(new_latch));
            std::memcpy(
                image.data() + 0x2B20U, &new_started, sizeof(new_started)
            );
            openswd3::battle::synchronize_legacy_battle_actor_image_write(
                actor, image, 0x2584U, sizeof(new_head)
            );
            owner_mapping_exact = owner_mapping_exact &&
                actor.base_initialization->linked_action_head_token ==
                    new_head &&
                actor.action_execution->early_latch == 0x23456789U &&
                actor.progress->frame_started == 0x3456789AU;
            openswd3::battle::synchronize_legacy_battle_actor_image_write(
                actor, image, 0x2B1CU, sizeof(new_latch)
            );
            owner_mapping_exact = owner_mapping_exact &&
                actor.action_execution->early_latch == new_latch &&
                actor.progress->frame_started == 0x3456789AU;
            openswd3::battle::synchronize_legacy_battle_actor_image_write(
                actor, image, 0x2B20U, sizeof(new_started)
            );
            owner_mapping_exact = owner_mapping_exact &&
                actor.progress->frame_started == new_started;
        }
        test.expect_true(
            owner_mapping_exact,
            "Group-A and Group-B actor images borrow linked-list head and adjacent latch/frame-started fields from canonical owners with separately committed writes"
        );
    }

    {
        Fixture fixture;
        bool owner_mapping_exact = true;
        for (const u32 token : std::array<u32, 2U>{
                 openswd3::battle::kLegacyBattleActorGroupABaseToken,
                 openswd3::battle::kLegacyBattleActorGroupBBaseToken
             }) {
            const auto actor =
                openswd3::battle::resolve_legacy_battle_actor_runtime_reset(
                    fixture.owners(), token
                );
            actor.action_execution->turn_threshold = 0xA123U;
            actor.base_initialization->field_2a94 = 0xB4U;
            actor.residual->field_2a95 = 0x3CU;
            openswd3::battle::LegacyBattleActorImage image{};
            openswd3::battle::materialize_legacy_battle_actor_image(
                actor, image
            );
            openswd3::compat::u16 materialized_threshold{};
            openswd3::compat::u8 materialized_marker{};
            openswd3::compat::u8 materialized_override{};
            std::memcpy(
                &materialized_threshold,
                image.data() + 0x2958U,
                sizeof(materialized_threshold)
            );
            std::memcpy(
                &materialized_marker,
                image.data() + 0x2A94U,
                sizeof(materialized_marker)
            );
            std::memcpy(
                &materialized_override,
                image.data() + 0x2A95U,
                sizeof(materialized_override)
            );
            owner_mapping_exact = owner_mapping_exact &&
                materialized_threshold == 0xA123U &&
                materialized_marker == 0xB4U && materialized_override == 0x3CU;

            const openswd3::compat::u16 new_threshold = 0xC567U;
            const openswd3::compat::u8 new_marker = 0xD8U;
            const openswd3::compat::u8 new_override = 0xE2U;
            std::memcpy(
                image.data() + 0x2958U, &new_threshold, sizeof(new_threshold)
            );
            std::memcpy(
                image.data() + 0x2A94U, &new_marker, sizeof(new_marker)
            );
            openswd3::battle::synchronize_legacy_battle_actor_image_write(
                actor, image, 0x2958U, sizeof(new_threshold)
            );
            owner_mapping_exact = owner_mapping_exact &&
                actor.action_execution->turn_threshold == new_threshold &&
                actor.base_initialization->field_2a94 == 0xB4U;
            openswd3::battle::synchronize_legacy_battle_actor_image_write(
                actor, image, 0x2A94U, sizeof(new_marker)
            );
            owner_mapping_exact = owner_mapping_exact &&
                actor.base_initialization->field_2a94 == new_marker &&
                actor.residual->field_2a95 == 0x3CU;
            std::memcpy(
                image.data() + 0x2A95U, &new_override, sizeof(new_override)
            );
            openswd3::battle::synchronize_legacy_battle_actor_image_write(
                actor, image, 0x2A95U, sizeof(new_override)
            );
            owner_mapping_exact = owner_mapping_exact &&
                actor.residual->field_2a95 == new_override;
        }
        test.expect_true(
            owner_mapping_exact,
            "Group-A and Group-B images project +0x2958 phase, +0x2A94 selector, and +0x2A95 override to distinct owners and commit independently"
        );
    }

    {
        Fixture fixture;
        bool alias_mapping_exact = true;
        for (const u32 token : std::array<u32, 2U>{
                 openswd3::battle::kLegacyBattleActorGroupABaseToken,
                 openswd3::battle::kLegacyBattleActorGroupBBaseToken
             }) {
            const auto actor =
                openswd3::battle::resolve_legacy_battle_actor_runtime_reset(
                    fixture.owners(), token
                );
            auto& slot = actor.action_execution->frame_source_action_record;
            slot.field_24 = 0x10203040U;
            slot.field_28 = 0x50607080U;
            actor.progress->cache_x = slot.field_24;
            actor.progress->cache_y = slot.field_28;
            openswd3::battle::LegacyBattleActorImage image{};
            openswd3::battle::materialize_legacy_battle_actor_image(
                actor, image
            );
            u32 materialized_x{};
            u32 materialized_y{};
            std::memcpy(&materialized_x, image.data() + 0x02C4U, sizeof(u32));
            std::memcpy(&materialized_y, image.data() + 0x02C8U, sizeof(u32));
            alias_mapping_exact = alias_mapping_exact &&
                materialized_x == slot.field_24 &&
                materialized_y == slot.field_28;

            const u32 new_x = 0xA1B2C3D4U;
            const u32 new_y = 0xE5F60718U;
            std::memcpy(image.data() + 0x02C4U, &new_x, sizeof(new_x));
            std::memcpy(image.data() + 0x02C8U, &new_y, sizeof(new_y));
            openswd3::battle::synchronize_legacy_battle_actor_image_write(
                actor, image, 0x02C4U, sizeof(new_x)
            );
            alias_mapping_exact = alias_mapping_exact &&
                slot.field_24 == new_x && actor.progress->cache_x == new_x &&
                slot.field_28 == 0x50607080U &&
                actor.progress->cache_y == 0x50607080U;
            openswd3::battle::synchronize_legacy_battle_actor_image_write(
                actor, image, 0x02C8U, sizeof(new_y)
            );
            alias_mapping_exact = alias_mapping_exact &&
                slot.field_28 == new_y && actor.progress->cache_y == new_y;
        }
        test.expect_true(
            alias_mapping_exact,
            "Group-A and Group-B slot0 +0x2C4/+0x2C8 record bytes and progress cache aliases commit in separate physical writes"
        );
    }

    {
        Fixture fixture;
        bool reverse_prefix_exact = true;
        for (const u32 token : std::array<u32, 2U>{
                 openswd3::battle::kLegacyBattleActorGroupABaseToken,
                 openswd3::battle::kLegacyBattleActorGroupBBaseToken
             }) {
            const auto actor =
                openswd3::battle::resolve_legacy_battle_actor_runtime_reset(
                    fixture.owners(), token
                );
            auto& execution = *actor.action_execution;
            execution.primary_value = 0x11223344U;
            execution.secondary_value = 0x55667788U;
            execution.primary_action_record.field_24 = 0x11223344U;
            execution.primary_action_record.field_28 = 0x55667788U;
            execution.action_flags = 0x5601U;
            execution.record_mode_flags = 0x56U;
            execution.secondary_auxiliary_word = 0x9ABCU;
            execution.auxiliary_word = 0xDEF0U;
            execution.color_values.fill(7);
            openswd3::battle::LegacyBattleActorImage image{};
            openswd3::battle::materialize_legacy_battle_actor_image(
                actor, image
            );
            constexpr u32 zero = 0U;
            for (u32 index = 0U; index < 29U; ++index) {
                const u32 offset = 0x03D0U - 4U * index;
                std::memcpy(image.data() + offset, &zero, sizeof(zero));
                openswd3::battle::synchronize_legacy_battle_actor_image_write(
                    actor, image, offset, sizeof(zero)
                );
                if (index == 8U) {
                    reverse_prefix_exact = reverse_prefix_exact &&
                        execution.auxiliary_word == 0U &&
                        execution.secondary_auxiliary_word == 0x9ABCU &&
                        std::all_of(execution.color_values.begin(),
                                    execution.color_values.end(),
                                    [](const auto value) {
                                        return value == 0;
                                    });
                }
                if (index == 9U) {
                    reverse_prefix_exact = reverse_prefix_exact &&
                        execution.secondary_auxiliary_word == 0U;
                }
                if (index == 16U) {
                    reverse_prefix_exact = reverse_prefix_exact &&
                        execution.action_flags == 0U &&
                        execution.record_mode_flags == 0U;
                }
            }
            reverse_prefix_exact = reverse_prefix_exact &&
                execution.secondary_value == 0U &&
                execution.primary_value == 0x11223344U &&
                execution.primary_action_record.field_28 == 0U &&
                execution.primary_action_record.field_24 == 0x11223344U;
            constexpr u32 last_offset = 0x035CU;
            std::memcpy(image.data() + last_offset, &zero, sizeof(zero));
            openswd3::battle::synchronize_legacy_battle_actor_image_write(
                actor, image, last_offset, sizeof(zero)
            );
            reverse_prefix_exact = reverse_prefix_exact &&
                execution.primary_value == 0U &&
                execution.primary_action_record.field_24 == 0U;
        }
        test.expect_true(
            reverse_prefix_exact,
            "Group-A and Group-B reverse 38-dword write prefixes synchronize slot1 scalar aliases before the next faultable write"
        );
    }

    LegacyBattleActorRuntimeResetRequest complete_entry{};
    std::size_t complete_accesses{};
    {
        Fixture fixture;
        fill_group_a(fixture, 0U);
        Random random;
        const u32 token = openswd3::battle::kLegacyBattleActorGroupABaseToken;
        complete_entry = request(token);
        const auto result = openswd3::battle::reset_legacy_battle_actor_runtime(
            openswd3::battle::resolve_legacy_battle_actor_runtime_reset(
                fixture.owners(), token
            ),
            random,
            complete_entry
        );
        complete_accesses = result.accesses_completed;
        const auto& party = fixture.startup->party[0U];
        const auto& execution = fixture.action->group_a_action_execution[0U];
        test.expect_true(
            result.status == LegacyBattleActorRuntimeResetStatus::completed &&
                result.returned &&
                result.return_eip == complete_entry.entry_return_address &&
                result.return_esp == complete_entry.entry_esp + 4U &&
                result.return_ebx == complete_entry.entry_ebx &&
                result.return_ebp == complete_entry.entry_ebp &&
                result.return_esi == complete_entry.entry_esi &&
                result.return_edi == complete_entry.entry_edi &&
                result.rep_iterations ==
                    std::array<u32, 8U>{
                        8U, 38U, 38U, 38U, 38U, 304U, 304U, 10U
                    } &&
                result.random_calls == 0U && random.calls == 0U &&
                party.position_x == 21 && party.position_y == 22 &&
                execution.position_x == 21 && execution.position_y == 22 &&
                party.progress.progress == 0U &&
                party.progress.action_complete == 0U &&
                party.base_initialization.field_266c == 0xFFFFFFE0U &&
                (party.progress.mode_gate & 0xFFFFU) == 0x1234U &&
                (*fixture.startup->group_a_runtime_reset)[0U].field_2af0 ==
                    1U &&
                all_zero(party.final_processing.profile_buffer) &&
                all_zero(party.final_processing.pre_effect_words) &&
                std::all_of(
                    execution.target_indices.begin(),
                    execution.target_indices.end(),
                    [](const u32 value) { return value == 0xFFFFFFFFU; }
                ) &&
                result.flags_known && result.flags.carry &&
                !result.flags.zero && result.flags.sign,
            "Group-A full path preserves REP counts, aliases, conditionals, final CMP flags, stack, and callee-saved registers"
        );
    }

    {
        Fixture fixture;
        fill_group_a(fixture, 0U);
        Random random;
        const u32 token = openswd3::battle::kLegacyBattleActorGroupABaseToken;
        fixture.startup->party[0U].configuration.source_runtime_value = 1U;
        const auto full = openswd3::battle::reset_legacy_battle_actor_runtime(
            openswd3::battle::resolve_legacy_battle_actor_runtime_reset(
                fixture.owners(), token
            ),
            random,
            request(token)
        );
        bool stops_exact =
            full.accesses_completed == 852U && full.random_calls == 1U;
        std::array<bool, 4U> stopped_kinds{};
        for (std::size_t ordinal = 0U; ordinal < full.accesses_completed;
             ++ordinal) {
            fill_group_a(fixture, 0U);
            fixture.startup->party[0U].configuration.source_runtime_value = 1U;
            auto entry = request(token);
            entry.stop_before_access = ordinal;
            const auto stopped =
                openswd3::battle::reset_legacy_battle_actor_runtime(
                    openswd3::battle::resolve_legacy_battle_actor_runtime_reset(
                        fixture.owners(), token
                    ),
                    random,
                    entry
                );
            stops_exact = stops_exact && !stopped.returned &&
                stopped.accesses_completed == ordinal &&
                stopped.stopped_access_ordinal == ordinal &&
                stopped.stopped_instruction != 0U;
            switch (stopped.status) {
            case LegacyBattleActorRuntimeResetStatus::actor_read_typed_stop:
                stopped_kinds[0U] = true;
                break;

            case LegacyBattleActorRuntimeResetStatus::actor_write_typed_stop:
                stopped_kinds[1U] = true;
                break;

            case LegacyBattleActorRuntimeResetStatus::stack_read_typed_stop:
                stopped_kinds[2U] = true;
                break;

            case LegacyBattleActorRuntimeResetStatus::stack_write_typed_stop:
                stopped_kinds[3U] = true;
                break;

            default:
                stops_exact = false;
                break;
            }
        }
        test.expect_true(
            stops_exact &&
                std::ranges::all_of(
                    stopped_kinds, [](const bool stopped) { return stopped; }
                ),
            "all 852 maximum-path physical accesses stop at the selected ordinal without committing the failing access"
        );
    }

    {
        Fixture fixture;
        Random random;
        const u32 token = openswd3::battle::kLegacyBattleActorGroupABaseToken;
        bool branches_exact = true;
        for (const u32 value : std::array<u32, 4U>{0U, 1U, 2U, 0xFFFFFFFFU}) {
            fill_group_a(fixture, 0U);
            auto& party = fixture.startup->party[0U];
            auto& execution = fixture.action->group_a_action_execution[0U];
            auto& residual = (*fixture.startup->group_a_runtime_reset)[0U];
            execution.special_effect_direct_mode = 0U;
            execution.special_particle_coordinate_suppression = 0U;
            residual.field_2af0 = 0xCAFEBABEU;
            execution.action_twenty_seven_motion_mode = value;
            party.progress.scene_identity = value;
            party.configuration.source_runtime_value = value;
            party.position_x = 11;
            party.position_y = 12;
            party.alternate_position_x = 21;
            party.alternate_position_y = 22;
            random.value = 0U;
            random.calls = 0U;
            const auto result =
                openswd3::battle::reset_legacy_battle_actor_runtime(
                    openswd3::battle::resolve_legacy_battle_actor_runtime_reset(
                        fixture.owners(), token
                    ),
                    random,
                    request(token)
                );
            branches_exact = branches_exact && result.returned &&
                residual.field_2af0 == (value == 1U ? 1U : 0xCAFEBABEU) &&
                party.position_x == (value == 1U ? 21 : 11) &&
                party.position_y == (value == 1U ? 22 : 12) &&
                result.random_calls == (value == 1U ? 1U : 0U) &&
                random.calls == (value == 1U ? 1U : 0U) &&
                result.return_eax == (value == 1U ? 50U : value);
        }
        test.expect_true(
            branches_exact,
            "full-dword one branches distinguish zero, one, two, and all-bits-set while RNG zero produces fifty"
        );
    }

    {
        Fixture fixture;
        Random random;
        random.value = 139U;
        const u32 token = openswd3::battle::kLegacyBattleActorGroupBBaseToken;
        auto& lifecycle = (*fixture.startup->group_b_lifecycle)[0U];
        lifecycle.action_configuration.source_runtime_value = 1U;
        lifecycle.action_configuration.profile_buffer.fill(std::byte{0x44});
        const auto entry = request(token);
        const auto result = openswd3::battle::reset_legacy_battle_actor_runtime(
            openswd3::battle::resolve_legacy_battle_actor_runtime_reset(
                fixture.owners(), token
            ),
            random,
            entry
        );
        test.expect_true(
            result.returned && result.random_calls == 1U &&
                result.random_bound == 140U && result.random_value == 139U &&
                random.calls == 1U && random.last_bound == 140U &&
                (fixture.startup->enemies[0U].progress.progress & 0xFFFFU) ==
                    189U &&
                result.return_eax == 189U && result.return_edx == 139U &&
                result.return_ecx == entry.random_return_ecx &&
                result.flags_known && !result.flags.carry &&
                !result.flags.zero && !result.flags.sign,
            "Group-B source value one consumes exactly one bounded random call and stores random plus fifty"
        );
    }

    {
        Fixture fixture;
        fill_group_a(fixture, 0U);
        Random random;
        const u32 token = openswd3::battle::kLegacyBattleActorGroupABaseToken;
        auto entry = request(token);
        entry.direction_flag = true;
        const auto result = openswd3::battle::reset_legacy_battle_actor_runtime(
            openswd3::battle::resolve_legacy_battle_actor_runtime_reset(
                fixture.owners(), token
            ),
            random,
            entry
        );
        test.expect_true(
            result.returned && result.direction_flag &&
                result.rep_iterations[0U] == 8U &&
                result.rep_iterations[5U] == 304U &&
                result.rep_iterations[6U] == 304U &&
                all_zero((*fixture.startup->group_a_runtime_reset)[0U]
                             .bytes_0174_029f),
            "entry DF one drives every REP backward without normalizing the direction flag"
        );
    }

    {
        Fixture fixture;
        Random random;
        const u32 token = openswd3::battle::kLegacyBattleActorGroupABaseToken;
        fixture.startup->party[0U].progress.scene_identity = 0U;
        fixture.startup->party[0U].final_processing.profile_buffer.fill(0x77U);
        auto entry = request(token);
        entry.stop_before_access = 550U;
        const auto result = openswd3::battle::reset_legacy_battle_actor_runtime(
            openswd3::battle::resolve_legacy_battle_actor_runtime_reset(
                fixture.owners(), token
            ),
            random,
            entry
        );
        test.expect_true(
            result.status ==
                    LegacyBattleActorRuntimeResetStatus::
                        actor_write_typed_stop &&
                !result.returned && result.rep_iterations[5U] == 304U &&
                result.rep_iterations[6U] > 0U &&
                result.rep_iterations[6U] < 304U &&
                result.rep_iterations[7U] == 0U &&
                fixture.startup->party[0U]
                        .final_processing.profile_buffer[0U] != 0U,
            "typed stop inside the second 304-dword REP keeps the first pass and current partial prefix while suppressing later clears"
        );
    }

    {
        Fixture fixture;
        Random random;
        const u32 token = openswd3::battle::kLegacyBattleActorGroupABaseToken;
        auto stack_entry = request(token);
        stack_entry.stop_before_access = 0U;
        const auto stack_stop =
            openswd3::battle::reset_legacy_battle_actor_runtime(
                openswd3::battle::resolve_legacy_battle_actor_runtime_reset(
                    fixture.owners(), token
                ),
                random,
                stack_entry
            );

        const auto invalid_stop =
            openswd3::battle::reset_legacy_battle_actor_runtime(
                {}, random, request(token + 1U)
            );

        auto random_entry = request(token);
        fixture.startup->party[0U].configuration.source_runtime_value = 1U;
        random_entry.random_callable = false;
        const auto random_stop =
            openswd3::battle::reset_legacy_battle_actor_runtime(
                openswd3::battle::resolve_legacy_battle_actor_runtime_reset(
                    fixture.owners(), token
                ),
                random,
                random_entry
            );

        fill_group_a(fixture, 0U);
        auto return_entry = request(token);
        return_entry.stop_before_access = complete_accesses - 1U;
        const auto return_stop =
            openswd3::battle::reset_legacy_battle_actor_runtime(
                openswd3::battle::resolve_legacy_battle_actor_runtime_reset(
                    fixture.owners(), token
                ),
                random,
                return_entry
            );

        test.expect_true(
            stack_stop.status ==
                    LegacyBattleActorRuntimeResetStatus::
                        stack_write_typed_stop &&
                stack_stop.return_eip == 0x00478850U &&
                stack_stop.return_eax == stack_entry.entry_eax &&
                stack_stop.return_esp == stack_entry.entry_esp &&
                invalid_stop.status ==
                    LegacyBattleActorRuntimeResetStatus::
                        actor_write_typed_stop &&
                invalid_stop.return_eip == 0x00478856U &&
                invalid_stop.stack_writes == 2U &&
                random_stop.status ==
                    LegacyBattleActorRuntimeResetStatus::
                        random_call_typed_stop &&
                random_stop.return_eip == 0x00439070U &&
                random_stop.random_calls == 0U &&
                return_stop.status ==
                    LegacyBattleActorRuntimeResetStatus::
                        stack_read_typed_stop &&
                return_stop.return_eip == 0x00478A61U && !return_stop.returned,
            "stack, invalid actor, nested random call, and RET boundaries stop at their physical instructions with committed prefixes"
        );
    }

    {
        constexpr std::array<u32, 17U> call_addresses{
            0x00452F93U,
            0x00453050U,
            0x00454FAFU,
            0x004567D8U,
            0x00456965U,
            0x00456989U,
            0x004572ABU,
            0x00457791U,
            0x004577CCU,
            0x00457F23U,
            0x00457FD0U,
            0x0045802CU,
            0x0045AE1DU,
            0x0045AEF6U,
            0x0045DC27U,
            0x0045DC58U,
            0x00467139U,
        };
        constexpr std::array<u32, 17U> return_addresses{
            0x00452F98U,
            0x00453055U,
            0x00454FB4U,
            0x004567DDU,
            0x0045696AU,
            0x0045698EU,
            0x004572B0U,
            0x00457796U,
            0x004577D1U,
            0x00457F28U,
            0x00457FD5U,
            0x00458031U,
            0x0045AE22U,
            0x0045AEFBU,
            0x0045DC2CU,
            0x0045DC5DU,
            0x0046713EU,
        };
        Fixture fixture;
        Random random;
        openswd3::battle::LegacyBattleActorRuntimeResetCallTrace trace{};
        openswd3::battle::LegacyBattleActorRuntimeResetCallRequests requests{};
        requests.count = call_addresses.size();
        bool returned = true;
        bool identity_exact = true;
        for (std::size_t index = 0U; index < call_addresses.size(); ++index) {
            const u32 token = index % 2U == 0U
                ? openswd3::battle::kLegacyBattleActorGroupABaseToken
                : openswd3::battle::kLegacyBattleActorGroupBBaseToken;
            returned = returned &&
                openswd3::battle::
                    execute_legacy_battle_actor_runtime_reset_call(
                           fixture.owners(),
                           random,
                           trace,
                           requests,
                           token,
                           static_cast<u32>(index),
                           0xA5000000U + static_cast<u32>(index),
                           call_addresses[index],
                           return_addresses[index]
                    );
            identity_exact = identity_exact &&
                trace.call_addresses[index] == call_addresses[index] &&
                trace.return_addresses[index] == return_addresses[index] &&
                trace.actor_tokens[index] == token;
        }
        test.expect_true(
            returned && identity_exact && trace.calls == call_addresses.size(),
            "all seventeen closed-parent CALL sites retain physical call and return identities in order"
        );
    }

    {
        Fixture fixture;
        Random random;
        openswd3::battle::LegacyBattleActorRuntimeResetCallTrace trace{};
        openswd3::battle::LegacyBattleActorRuntimeResetCallRequests requests{};
        requests.count = 2U;
        requests.requests[1U].stop_before_access = 0U;
        const bool returned =
            openswd3::battle::execute_legacy_battle_actor_runtime_reset_call(
                fixture.owners(),
                random,
                trace,
                requests,
                openswd3::battle::kLegacyBattleActorGroupABaseToken,
                0U,
                0U,
                0x0045AE1DU,
                0x0045AE22U,
                {},
                false,
                1U
            );
        test.expect_true(
            !returned && trace.calls == 1U &&
                trace.call_addresses[0U] == 0x0045AE1DU &&
                trace.last.status ==
                    LegacyBattleActorRuntimeResetStatus::
                        stack_write_typed_stop &&
                trace.last.stopped_access_ordinal == 0U,
            "nested call request offset selects the next parent-level typed-stop request without shifting trace identity"
        );
    }
}
