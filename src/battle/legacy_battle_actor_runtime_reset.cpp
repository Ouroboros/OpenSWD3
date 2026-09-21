#include "openswd3/battle/legacy_battle_actor_runtime_reset.hpp"

#include "openswd3/asset_runtime/legacy_action_record.hpp"
#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "openswd3/battle/legacy_battle_actor_lifecycle.hpp"
#include "openswd3/battle/legacy_battle_actor_progress.hpp"
#include "openswd3/battle/legacy_battle_group_a_configuration.hpp"
#include "openswd3/battle/legacy_battle_group_a_final_processing_state.hpp"
#include "openswd3/battle/legacy_battle_group_a_item_effect_application.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"
#include "openswd3/battle/legacy_battle_status_indicator.hpp"

#include <bit>
#include <cstring>
#include <type_traits>

namespace openswd3::battle {
namespace {

using compat::u8;
using compat::u16;
using compat::u32;

inline constexpr std::size_t kActorImageSize = 0x2B18U;
inline constexpr u32 kActionRecordBase = 0x02A0U;
inline constexpr u32 kCoordinateBase = 0x0D50U;
inline constexpr u32 kProfileBase = 0x0D90U;
inline constexpr u32 kActionTextBase = 0x2630U;

using ActorImage = std::array<std::byte, kActorImageSize>;

template <typename Value>
void store_value(
    ActorImage& image, const u32 offset, const Value value
) noexcept {
    static_assert(std::is_trivially_copyable_v<Value>);
    std::memcpy(image.data() + offset, &value, sizeof(Value));
}

template <typename Value>
[[nodiscard]] Value
load_value(const ActorImage& image, const u32 offset) noexcept {
    static_assert(std::is_trivially_copyable_v<Value>);
    Value value{};
    std::memcpy(&value, image.data() + offset, sizeof(Value));
    return value;
}

[[nodiscard]] constexpr bool overlaps(
    const u32 left_offset,
    const u32 left_size,
    const u32 right_offset,
    const u32 right_size
) noexcept {
    return left_offset < right_offset + right_size &&
        right_offset < left_offset + left_size;
}

void copy_overlap_to_owner(
    std::byte* const destination,
    const u32 destination_offset,
    const u32 destination_size,
    const ActorImage& image,
    const u32 write_offset,
    const u32 write_size
) noexcept {
    const u32 start =
        write_offset > destination_offset ? write_offset : destination_offset;
    const u32 write_end = write_offset + write_size;
    const u32 destination_end = destination_offset + destination_size;
    const u32 stop = write_end < destination_end ? write_end : destination_end;
    if (start >= stop) {
        return;
    }

    std::memcpy(
        destination + (start - destination_offset),
        image.data() + start,
        stop - start
    );
}

void materialize_actor(
    const LegacyBattleActorRuntimeResetView& actor, ActorImage& image
) noexcept {
    if (actor.residual == nullptr || actor.progress == nullptr ||
        actor.action_execution == nullptr ||
        actor.primary_coordinates == nullptr ||
        actor.base_initialization == nullptr) {
        return;
    }

    std::memcpy(
        image.data() + 0x0174U,
        actor.residual->bytes_0174_029f.data(),
        actor.residual->bytes_0174_029f.size()
    );
    std::memcpy(
        image.data() + 0x0D34U,
        actor.residual->bytes_0d34_0d4f.data(),
        actor.residual->bytes_0d34_0d4f.size()
    );

    auto* const action_records =
        static_cast<LegacyBattleActorActionRecordSlots*>(
            actor.action_execution
        );
    std::memcpy(
        image.data() + kActionRecordBase,
        action_records,
        sizeof(*action_records)
    );

    auto* const coordinate_source =
        static_cast<LegacyBattleActorCoordinateSourceRecord*>(
            actor.primary_coordinates
        );
    auto* const coordinate_destination =
        static_cast<LegacyBattleActorCoordinateDestinationRecord*>(
            actor.primary_coordinates
        );
    std::memcpy(
        image.data() + kCoordinateBase,
        coordinate_source,
        sizeof(*coordinate_source)
    );
    std::memcpy(
        image.data() + kCoordinateBase + sizeof(*coordinate_source),
        coordinate_destination,
        sizeof(*coordinate_destination)
    );

    if (actor.group_a_final_processing != nullptr) {
        std::memcpy(
            image.data() + kProfileBase,
            actor.group_a_final_processing->profile_buffer.data(),
            sizeof(actor.group_a_final_processing->profile_buffer)
        );
        std::memcpy(
            image.data() + kActionTextBase,
            actor.group_a_final_processing->pre_effect_words.data(),
            sizeof(actor.group_a_final_processing->pre_effect_words)
        );
    } else if (
        actor.group_b_configuration != nullptr &&
        actor.group_b_composition != nullptr
    ) {
        std::memcpy(
            image.data() + kProfileBase,
            actor.group_b_configuration->profile_buffer.data(),
            actor.group_b_configuration->profile_buffer.size()
        );
        std::memcpy(
            image.data() + kActionTextBase,
            actor.group_b_composition->action_text.data(),
            actor.group_b_composition->action_text.size()
        );
    }

    store_value(image, 0x2A12U, static_cast<u16>(actor.progress->progress));
    store_value(image, 0x2AACU, actor.action_execution->turn_completion_latch);
    store_value(
        image, 0x0D9CU, actor.action_execution->special_effect_direct_mode
    );
    store_value(image, 0x2AB0U, actor.progress->action_complete);
    store_value(image, 0x2AB4U, actor.action_execution->idle_state_latch);
    store_value(image, 0x2AA8U, actor.residual->field_2aa8);
    store_value(image, 0x2AE0U, actor.residual->field_2ae0);
    store_value(
        image, 0x2B14U, actor.action_execution->effect_application_latch
    );
    store_value(image, 0x2A8CU, actor.action_execution->alternate_mode);
    store_value(image, 0x2A86U, actor.action_execution->action_override_flags);
    store_value(image, 0x2AC8U, actor.residual->field_2ac8);
    store_value(image, 0x2AD4U, actor.residual->field_2ad4);
    store_value(image, 0x2ACCU, actor.residual->field_2acc);
    store_value(
        image,
        0x2B0CU,
        actor.group_a_final_processing != nullptr
            ? actor.group_a_final_processing->completion_latch
            : actor.residual->field_2b0c
    );
    store_value(image, 0x2AF4U, actor.residual->field_2af4);
    store_value(image, 0x2A6CU, actor.action_execution->action_kind);
    store_value(image, 0x2A72U, actor.residual->field_2a72);
    store_value(image, 0x2A7AU, actor.residual->field_2a7a);

    if (actor.group_a_item_effect != nullptr) {
        std::memcpy(
            image.data() + 0x29A4U,
            actor.group_a_item_effect->derived_words.data(),
            sizeof(actor.group_a_item_effect->derived_words)
        );
        store_value(image, 0x2A70U, actor.group_a_item_effect->display_kind);
        store_value(image, 0x26CCU, actor.group_a_item_effect->effect_flags);
    } else if (actor.group_b_composition != nullptr) {
        std::memcpy(
            image.data() + 0x29A4U,
            actor.group_b_composition->derived_words.data(),
            sizeof(actor.group_b_composition->derived_words)
        );
        store_value(image, 0x2A70U, actor.group_b_composition->display_kind);
        store_value(
            image, 0x2A8CU, actor.group_b_composition->profile_mode_selector
        );
        store_value(image, 0x26CCU, actor.residual->field_26cc);
    }

    store_value(image, 0x2668U, actor.action_execution->turn_countdown);
    store_value(image, 0x266CU, actor.base_initialization->field_266c);
    store_value(image, 0x2954U, actor.action_execution->motion_word);
    store_value(image, 0x2A68U, actor.base_initialization->field_2a68);
    store_value(image, 0x2A6AU, actor.base_initialization->field_2a6a);
    store_value(
        image, 0x2674U, actor.action_execution->spawn_completion_offset
    );
    store_value(
        image, 0x2A0EU, actor.action_execution->profile_variant_override
    );
    store_value(image, 0x2690U, actor.residual->field_2690);
    store_value(image, 0x2670U, actor.residual->field_2670);
    store_value(
        image, 0x2A8AU, actor.action_execution->special_profile_variant
    );
    store_value(image, 0x0316U, actor.action_execution->render_x_base);
    store_value(image, 0x0318U, actor.action_execution->render_y_base);
    store_value(image, 0x2AF0U, actor.residual->field_2af0);
    store_value(
        image,
        0x0D94U,
        actor.action_execution->special_particle_coordinate_suppression
    );
    store_value(image, 0x26D0U, static_cast<u16>(actor.progress->mode_gate));
    store_value(
        image, 0x2B00U, actor.action_execution->action_twenty_seven_motion_mode
    );
    store_value(image, 0x2B04U, actor.progress->scene_identity);
    std::memcpy(
        image.data() + 0x2A56U,
        actor.action_execution->target_indices.data(),
        sizeof(actor.action_execution->target_indices)
    );

    u32 source_runtime_value{};
    if (actor.group_a_configuration != nullptr) {
        source_runtime_value =
            actor.group_a_configuration->source_runtime_value;
    } else if (actor.group_b_configuration != nullptr) {
        source_runtime_value =
            actor.group_b_configuration->source_runtime_value;
    }
    store_value(image, 0x2AA0U, source_runtime_value);
}

void synchronize_actor_write(
    const LegacyBattleActorRuntimeResetView& actor,
    const ActorImage& image,
    const u32 offset,
    const u32 size
) noexcept {
    copy_overlap_to_owner(
        actor.residual->bytes_0174_029f.data(),
        0x0174U,
        static_cast<u32>(actor.residual->bytes_0174_029f.size()),
        image,
        offset,
        size
    );
    copy_overlap_to_owner(
        actor.residual->bytes_0d34_0d4f.data(),
        0x0D34U,
        static_cast<u32>(actor.residual->bytes_0d34_0d4f.size()),
        image,
        offset,
        size
    );

    auto* const action_records =
        static_cast<LegacyBattleActorActionRecordSlots*>(
            actor.action_execution
        );
    copy_overlap_to_owner(
        reinterpret_cast<std::byte*>(action_records),
        kActionRecordBase,
        sizeof(*action_records),
        image,
        offset,
        size
    );

    const auto synchronize_coordinates =
        [&](LegacyBattleActorCoordinatesState* coordinates) {
            if (coordinates == nullptr) {
                return;
            }

            auto* const source =
                static_cast<LegacyBattleActorCoordinateSourceRecord*>(
                    coordinates
                );
            auto* const destination =
                static_cast<LegacyBattleActorCoordinateDestinationRecord*>(
                    coordinates
                );
            copy_overlap_to_owner(
                reinterpret_cast<std::byte*>(source),
                kCoordinateBase,
                sizeof(*source),
                image,
                offset,
                size
            );
            copy_overlap_to_owner(
                reinterpret_cast<std::byte*>(destination),
                kCoordinateBase + sizeof(*source),
                sizeof(*destination),
                image,
                offset,
                size
            );
        };
    synchronize_coordinates(actor.primary_coordinates);
    synchronize_coordinates(actor.coordinate_alias);

    if (actor.group_a_final_processing != nullptr) {
        copy_overlap_to_owner(
            reinterpret_cast<std::byte*>(
                actor.group_a_final_processing->profile_buffer.data()
            ),
            kProfileBase,
            sizeof(actor.group_a_final_processing->profile_buffer),
            image,
            offset,
            size
        );
        copy_overlap_to_owner(
            reinterpret_cast<std::byte*>(
                actor.group_a_final_processing->pre_effect_words.data()
            ),
            kActionTextBase,
            sizeof(actor.group_a_final_processing->pre_effect_words),
            image,
            offset,
            size
        );
    } else if (
        actor.group_b_configuration != nullptr &&
        actor.group_b_composition != nullptr
    ) {
        copy_overlap_to_owner(
            actor.group_b_configuration->profile_buffer.data(),
            kProfileBase,
            static_cast<u32>(
                actor.group_b_configuration->profile_buffer.size()
            ),
            image,
            offset,
            size
        );
        copy_overlap_to_owner(
            reinterpret_cast<std::byte*>(
                actor.group_b_composition->action_text.data()
            ),
            kActionTextBase,
            static_cast<u32>(actor.group_b_composition->action_text.size()),
            image,
            offset,
            size
        );
    }

    const auto changed = [&](const u32 field_offset, const u32 field_size) {
        return overlaps(offset, size, field_offset, field_size);
    };
    const auto replace_low_word = [](u32& value, const u16 word) {
        value = (value & 0xFFFF0000U) | word;
    };

    if (changed(0x2A12U, sizeof(u16))) {
        const u16 value = load_value<u16>(image, 0x2A12U);
        replace_low_word(actor.progress->progress, value);
        actor.action_execution->completion_delay_word = value;
    }
    if (changed(0x2AACU, sizeof(u32))) {
        actor.action_execution->turn_completion_latch =
            load_value<u32>(image, 0x2AACU);
    }
    if (changed(0x0D9CU, sizeof(u8))) {
        actor.action_execution->special_effect_direct_mode =
            load_value<u8>(image, 0x0D9CU);
    }
    if (actor.group_a_final_processing != nullptr &&
        changed(0x0D9CU, sizeof(u16))) {
        actor.group_a_final_processing->actor_flags =
            load_value<u16>(image, 0x0D9CU);
    }
    if (changed(0x2AB0U, sizeof(u32))) {
        const u32 value = load_value<u32>(image, 0x2AB0U);
        actor.progress->action_complete = value;
        actor.action_execution->turn_completion_aux = value;
    }
    if (changed(0x2AB4U, sizeof(u32))) {
        actor.action_execution->idle_state_latch =
            load_value<u32>(image, 0x2AB4U);
    }
    if (changed(0x2AA8U, sizeof(u32))) {
        actor.residual->field_2aa8 = load_value<u32>(image, 0x2AA8U);
    }
    if (changed(0x2AE0U, sizeof(u32))) {
        actor.residual->field_2ae0 = load_value<u32>(image, 0x2AE0U);
    }
    if (changed(0x2B14U, sizeof(u32))) {
        actor.action_execution->effect_application_latch =
            load_value<u32>(image, 0x2B14U);
    }
    if (changed(0x2A8CU, sizeof(u16))) {
        const u16 value = load_value<u16>(image, 0x2A8CU);
        actor.action_execution->alternate_mode = value;
        if (actor.group_b_composition != nullptr) {
            actor.group_b_composition->profile_mode_selector = value;
        }
    }
    if (changed(0x2A86U, sizeof(u16))) {
        actor.action_execution->action_override_flags =
            load_value<u16>(image, 0x2A86U);
    }
    if (changed(0x2AC8U, sizeof(u32))) {
        actor.residual->field_2ac8 = load_value<u32>(image, 0x2AC8U);
    }
    if (changed(0x2AD4U, sizeof(u32))) {
        actor.residual->field_2ad4 = load_value<u32>(image, 0x2AD4U);
    }
    if (changed(0x2ACCU, sizeof(u32))) {
        actor.residual->field_2acc = load_value<u32>(image, 0x2ACCU);
    }
    if (changed(0x2B0CU, sizeof(u32))) {
        const u32 value = load_value<u32>(image, 0x2B0CU);
        actor.residual->field_2b0c = value;
        if (actor.group_a_final_processing != nullptr) {
            actor.group_a_final_processing->completion_latch = value;
        }
    }
    if (changed(0x2AF4U, sizeof(u32))) {
        actor.residual->field_2af4 = load_value<u32>(image, 0x2AF4U);
    }
    if (changed(0x2A6CU, sizeof(u16))) {
        const u16 value = load_value<u16>(image, 0x2A6CU);
        actor.action_execution->action_kind = value;
        if (actor.group_a_item_effect != nullptr) {
            actor.group_a_item_effect->action_kind = value;
        }
        if (actor.group_b_composition != nullptr) {
            actor.group_b_composition->action_kind = value;
        }
    }
    if (changed(0x2A70U, sizeof(u16))) {
        const u16 value = load_value<u16>(image, 0x2A70U);
        if (actor.group_a_item_effect != nullptr) {
            actor.group_a_item_effect->display_kind = value;
        }
        if (actor.group_b_composition != nullptr) {
            actor.group_b_composition->display_kind = value;
        }
    }
    if (changed(0x2A72U, sizeof(u16))) {
        actor.residual->field_2a72 = load_value<u16>(image, 0x2A72U);
    }
    if (changed(0x2A7AU, sizeof(u16))) {
        actor.residual->field_2a7a = load_value<u16>(image, 0x2A7AU);
    }
    if (changed(0x29A4U, 4U * sizeof(u16))) {
        if (actor.group_a_item_effect != nullptr) {
            std::memcpy(
                actor.group_a_item_effect->derived_words.data(),
                image.data() + 0x29A4U,
                sizeof(actor.group_a_item_effect->derived_words)
            );
        }
        if (actor.group_b_composition != nullptr) {
            std::memcpy(
                actor.group_b_composition->derived_words.data(),
                image.data() + 0x29A4U,
                sizeof(actor.group_b_composition->derived_words)
            );
        }
    }
    if (changed(0x2668U, sizeof(u32))) {
        actor.action_execution->turn_countdown =
            load_value<compat::i32>(image, 0x2668U);
    }
    if (changed(0x266CU, sizeof(u32))) {
        actor.base_initialization->field_266c = load_value<u32>(image, 0x266CU);
    }
    if (changed(0x2954U, sizeof(u16))) {
        actor.action_execution->motion_word = load_value<u16>(image, 0x2954U);
    }
    if (changed(0x2A68U, sizeof(u16))) {
        actor.base_initialization->field_2a68 = load_value<u16>(image, 0x2A68U);
    }
    if (changed(0x2A6AU, sizeof(u16))) {
        actor.base_initialization->field_2a6a = load_value<u16>(image, 0x2A6AU);
    }
    if (changed(0x2674U, sizeof(u32))) {
        actor.action_execution->spawn_completion_offset =
            load_value<u32>(image, 0x2674U);
    }
    if (changed(0x2A0EU, sizeof(u16))) {
        actor.action_execution->profile_variant_override =
            load_value<u16>(image, 0x2A0EU);
    }
    if (changed(0x2690U, sizeof(u32))) {
        actor.residual->field_2690 = load_value<u32>(image, 0x2690U);
    }
    if (changed(0x2670U, sizeof(u32))) {
        actor.residual->field_2670 = load_value<u32>(image, 0x2670U);
    }
    if (changed(0x2A8AU, sizeof(u16))) {
        const u16 value = load_value<u16>(image, 0x2A8AU);
        actor.action_execution->special_profile_variant = value;
        if (actor.group_a_final_processing != nullptr) {
            actor.group_a_final_processing->applied_mode_value = value;
        }
    }
    if (changed(0x0316U, sizeof(u16))) {
        actor.action_execution->render_x_base = load_value<u16>(image, 0x0316U);
    }
    if (changed(0x0318U, sizeof(u16))) {
        actor.action_execution->render_y_base = load_value<u16>(image, 0x0318U);
    }
    if (changed(0x26CCU, sizeof(u32))) {
        const u32 value = load_value<u32>(image, 0x26CCU);
        actor.residual->field_26cc = value;
        if (actor.group_a_item_effect != nullptr) {
            actor.group_a_item_effect->effect_flags = value;
        }
    }
    if (changed(0x2AF0U, sizeof(u32))) {
        actor.residual->field_2af0 = load_value<u32>(image, 0x2AF0U);
    }
    if (changed(0x0D94U, sizeof(u8))) {
        actor.action_execution->special_particle_coordinate_suppression =
            load_value<u8>(image, 0x0D94U);
    }
    if (changed(0x26D0U, sizeof(u16))) {
        const u16 value = load_value<u16>(image, 0x26D0U);
        replace_low_word(actor.progress->mode_gate, value);
        actor.action_execution->retreat_ready_flags = value;
    }
    if (changed(0x2B00U, sizeof(u32))) {
        const u32 value = load_value<u32>(image, 0x2B00U);
        actor.action_execution->action_twenty_seven_motion_mode = value;
        if (actor.group_a_final_processing != nullptr) {
            actor.group_a_final_processing->transition_gate_a = value;
        }
    }
    if (changed(0x2B04U, sizeof(u32))) {
        const u32 value = load_value<u32>(image, 0x2B04U);
        actor.progress->scene_identity = value;
        if (actor.group_a_final_processing != nullptr) {
            actor.group_a_final_processing->transition_gate_b = value;
        }
    }
    if (changed(0x2A56U, 4U * sizeof(u32))) {
        std::memcpy(
            actor.action_execution->target_indices.data(),
            image.data() + 0x2A56U,
            sizeof(actor.action_execution->target_indices)
        );
    }
    if (changed(0x2AA0U, sizeof(u32))) {
        const u32 value = load_value<u32>(image, 0x2AA0U);
        if (actor.group_a_configuration != nullptr) {
            actor.group_a_configuration->source_runtime_value = value;
        }
        if (actor.group_b_configuration != nullptr) {
            actor.group_b_configuration->source_runtime_value = value;
        }
    }
}

[[nodiscard]] constexpr bool even_parity(const u8 value) noexcept {
    return (std::popcount(value) & 1) == 0;
}

[[nodiscard]] constexpr LegacyBattleActorCoordinateFlags
logical_flags(const u32 value, const u32 sign_mask) noexcept {
    return {
        .carry = false,
        .parity = even_parity(static_cast<u8>(value)),
        .auxiliary_carry = false,
        .auxiliary_carry_defined = false,
        .zero = value == 0U,
        .sign = (value & sign_mask) != 0U,
        .overflow = false,
    };
}

[[nodiscard]] constexpr LegacyBattleActorCoordinateFlags
subtract_flags(const u32 left, const u32 right) noexcept {
    const u32 value = left - right;
    return {
        .carry = left < right,
        .parity = even_parity(static_cast<u8>(value)),
        .auxiliary_carry = ((left ^ right ^ value) & 0x10U) != 0U,
        .auxiliary_carry_defined = true,
        .zero = value == 0U,
        .sign = (value & 0x80000000U) != 0U,
        .overflow = ((left ^ right) & (left ^ value) & 0x80000000U) != 0U,
    };
}

[[nodiscard]] constexpr LegacyBattleActorCoordinateFlags
add_flags(const u32 left, const u32 right) noexcept {
    const u32 value = left + right;
    return {
        .carry = value < left,
        .parity = even_parity(static_cast<u8>(value)),
        .auxiliary_carry = ((left ^ right ^ value) & 0x10U) != 0U,
        .auxiliary_carry_defined = true,
        .zero = value == 0U,
        .sign = (value & 0x80000000U) != 0U,
        .overflow = ((~(left ^ right) & (left ^ value)) & 0x80000000U) != 0U,
    };
}

[[nodiscard]] constexpr bool resolve_index(
    const u32 token,
    const u32 base,
    const u32 stride,
    const std::size_t count,
    std::size_t& index
) noexcept {
    if (token < base) {
        return false;
    }

    const u32 delta = token - base;
    if (delta % stride != 0U) {
        return false;
    }

    index = delta / stride;
    return index < count;
}

[[nodiscard]] bool
valid_view(const LegacyBattleActorRuntimeResetView& actor) noexcept {
    return actor.residual != nullptr && actor.progress != nullptr &&
        actor.action_execution != nullptr &&
        actor.primary_coordinates != nullptr &&
        actor.base_initialization != nullptr;
}

}  // namespace

LegacyBattleActorRuntimeResetView resolve_legacy_battle_actor_runtime_reset(
    const LegacyBattleActorRuntimeResetOwners& owners, const u32 actor_token
) noexcept {
    std::size_t index{};
    if (resolve_index(
            actor_token,
            kLegacyBattleActorGroupABaseToken,
            kLegacyBattleActorGroupAElementSize,
            kLegacyBattleActorGroupAElementCount,
            index
        ) &&
        owners.startup != nullptr && owners.action != nullptr &&
        owners.startup->group_a_runtime_reset != nullptr) {
        auto& party = owners.startup->party[index];
        auto& execution = owners.action->group_a_action_execution[index];
        return {
            .residual = &(*owners.startup->group_a_runtime_reset)[index],
            .progress = &party.progress,
            .action_execution = &execution,
            .primary_coordinates = &party,
            .coordinate_alias = &execution,
            .base_initialization = &party.base_initialization,
            .group_a_configuration = &party.configuration,
            .group_a_final_processing = &party.final_processing,
            .group_a_item_effect = &party.item_effect_application,
        };
    }

    if (resolve_index(
            actor_token,
            kLegacyBattleActorGroupBBaseToken,
            kLegacyBattleActorGroupBElementSize,
            kLegacyBattleActorGroupBElementCount,
            index
        ) &&
        owners.startup != nullptr &&
        owners.startup->group_b_lifecycle != nullptr) {
        auto& lifecycle = (*owners.startup->group_b_lifecycle)[index];
        return {
            .residual = &lifecycle.runtime_reset,
            .progress = &owners.startup->enemies[index].progress,
            .action_execution = &lifecycle.action_execution,
            .primary_coordinates = &lifecycle.action_execution,
            .base_initialization = &lifecycle.base_initialization,
            .group_b_configuration = &lifecycle.action_configuration,
            .group_b_composition = &lifecycle.action_composition,
        };
    }

    return {};
}

LegacyBattleActorRuntimeResetResult reset_legacy_battle_actor_runtime(
    const LegacyBattleActorRuntimeResetView actor,
    LegacyBattleBoundedRandomPort& random,
    const LegacyBattleActorRuntimeResetRequest& request
) noexcept {
    LegacyBattleActorRuntimeResetResult result{
        .return_eax = request.entry_eax,
        .return_ecx = request.actor_token,
        .return_edx = request.entry_edx,
        .return_ebx = request.entry_ebx,
        .return_ebp = request.entry_ebp,
        .return_esi = request.entry_esi,
        .return_edi = request.entry_edi,
        .return_esp = request.entry_esp,
        .flags = request.entry_flags,
        .flags_known = request.entry_flags_known,
        .direction_flag = request.direction_flag,
    };
    ActorImage image{};
    materialize_actor(actor, image);

    u32 eax = request.entry_eax;
    u32 ecx = request.actor_token;
    u32 edx = request.entry_edx;
    u32 ebx = request.entry_ebx;
    u32 ebp = request.entry_ebp;
    u32 esi = request.entry_esi;
    u32 edi = request.entry_edi;
    u32 esp = request.entry_esp;
    u32 eip = kLegacyBattleActorRuntimeResetAddress;
    auto flags = request.entry_flags;
    bool flags_known = request.entry_flags_known;

    const auto finish = [&]() {
        result.return_eax = eax;
        result.return_ecx = ecx;
        result.return_edx = edx;
        result.return_ebx = ebx;
        result.return_ebp = ebp;
        result.return_esi = esi;
        result.return_edi = edi;
        result.return_esp = esp;
        result.return_eip = eip;
        result.flags = flags;
        result.flags_known = flags_known;
        return result;
    };

    const auto stop = [&](const LegacyBattleActorRuntimeResetAccessKind kind,
                          const u32 instruction,
                          const u32 token) {
        switch (kind) {
        case LegacyBattleActorRuntimeResetAccessKind::actor_read:
            result.status =
                LegacyBattleActorRuntimeResetStatus::actor_read_typed_stop;
            break;

        case LegacyBattleActorRuntimeResetAccessKind::actor_write:
            result.status =
                LegacyBattleActorRuntimeResetStatus::actor_write_typed_stop;
            break;

        case LegacyBattleActorRuntimeResetAccessKind::stack_read:
            result.status =
                LegacyBattleActorRuntimeResetStatus::stack_read_typed_stop;
            break;

        case LegacyBattleActorRuntimeResetAccessKind::stack_write:
            result.status =
                LegacyBattleActorRuntimeResetStatus::stack_write_typed_stop;
            break;
        }
        result.stopped_access_ordinal = result.accesses_completed;
        result.stopped_access_kind = kind;
        result.stopped_instruction = instruction;
        result.stopped_token = token;
        eip = instruction;
    };

    const auto touch = [&](const LegacyBattleActorRuntimeResetAccessKind kind,
                           const u32 instruction,
                           const u32 token,
                           const u32 value) {
        const bool actor_access =
            kind == LegacyBattleActorRuntimeResetAccessKind::actor_read ||
            kind == LegacyBattleActorRuntimeResetAccessKind::actor_write;
        if (result.accesses_completed == request.stop_before_access ||
            (actor_access && !valid_view(actor))) {
            stop(kind, instruction, token);
            return false;
        }

        ++result.accesses_completed;
        switch (kind) {
        case LegacyBattleActorRuntimeResetAccessKind::actor_read:
            ++result.actor_reads;
            break;

        case LegacyBattleActorRuntimeResetAccessKind::actor_write:
            ++result.actor_writes;
            break;

        case LegacyBattleActorRuntimeResetAccessKind::stack_read:
            ++result.stack_reads;
            break;

        case LegacyBattleActorRuntimeResetAccessKind::stack_write:
            ++result.stack_writes;
            break;
        }

        if (!actor_access &&
            result.stack_trace_count < result.stack_tokens.size()) {
            const std::size_t index = result.stack_trace_count++;
            result.stack_tokens[index] = token;
            result.stack_values[index] = value;
            result.stack_kinds[index] = kind;
        }
        return true;
    };

    const auto push = [&](const u32 instruction, const u32 value) {
        const u32 token = esp - 4U;
        if (!touch(
                LegacyBattleActorRuntimeResetAccessKind::stack_write,
                instruction,
                token,
                value
            )) {
            return false;
        }
        esp = token;
        return true;
    };

    const auto pop = [&](const u32 instruction, const u32 value, u32& output) {
        if (!touch(
                LegacyBattleActorRuntimeResetAccessKind::stack_read,
                instruction,
                esp,
                value
            )) {
            return false;
        }
        output = value;
        esp += 4U;
        return true;
    };

    const auto read_byte =
        [&](const u32 instruction, const u32 offset, u8& value) {
            if (!touch(
                    LegacyBattleActorRuntimeResetAccessKind::actor_read,
                    instruction,
                    request.actor_token + offset,
                    0U
                )) {
                return false;
            }
            value = load_value<u8>(image, offset);
            return true;
        };

    const auto read_word =
        [&](const u32 instruction, const u32 offset, u16& value) {
            if (!touch(
                    LegacyBattleActorRuntimeResetAccessKind::actor_read,
                    instruction,
                    request.actor_token + offset,
                    0U
                )) {
                return false;
            }
            value = load_value<u16>(image, offset);
            return true;
        };

    const auto read_dword =
        [&](const u32 instruction, const u32 offset, u32& value) {
            if (!touch(
                    LegacyBattleActorRuntimeResetAccessKind::actor_read,
                    instruction,
                    request.actor_token + offset,
                    0U
                )) {
                return false;
            }
            value = load_value<u32>(image, offset);
            return true;
        };

    const auto write_word =
        [&](const u32 instruction, const u32 offset, const u16 value) {
            if (!touch(
                    LegacyBattleActorRuntimeResetAccessKind::actor_write,
                    instruction,
                    request.actor_token + offset,
                    value
                )) {
                return false;
            }
            store_value(image, offset, value);
            synchronize_actor_write(actor, image, offset, sizeof(value));
            return true;
        };

    const auto write_dword =
        [&](const u32 instruction, const u32 offset, const u32 value) {
            if (!touch(
                    LegacyBattleActorRuntimeResetAccessKind::actor_write,
                    instruction,
                    request.actor_token + offset,
                    value
                )) {
                return false;
            }
            store_value(image, offset, value);
            synchronize_actor_write(actor, image, offset, sizeof(value));
            return true;
        };

    if (!push(0x00478850U, ebx)) {
        return finish();
    }
    ebx = ecx;
    eax = 0U;
    flags = logical_flags(eax, 0x80000000U);
    flags_known = true;
    if (!push(0x00478855U, ebp)) {
        return finish();
    }
    if (!write_word(0x00478856U, 0x2A12U, static_cast<u16>(eax)) ||
        !write_dword(0x0047885DU, 0x2AACU, eax)) {
        return finish();
    }
    u8 byte_value{};
    if (!read_byte(0x00478863U, 0x0D9CU, byte_value)) {
        return finish();
    }
    edx = (edx & 0xFFFFFF00U) | byte_value;
    if (!write_dword(0x00478869U, 0x2AB0U, eax) ||
        !write_dword(0x0047886FU, 0x2AB4U, eax) ||
        !write_dword(0x00478875U, 0x2AA8U, eax) ||
        !write_dword(0x0047887BU, 0x2AE0U, eax) ||
        !write_dword(0x00478881U, 0x2B14U, eax) ||
        !write_word(0x00478887U, 0x2A8CU, static_cast<u16>(eax)) ||
        !write_word(0x0047888EU, 0x2A86U, static_cast<u16>(eax)) ||
        !write_dword(0x00478895U, 0x2AC8U, eax)) {
        return finish();
    }
    ecx = 8U;
    if (!write_dword(0x004788A0U, 0x2AD4U, eax)) {
        return finish();
    }
    ebp = 1U;
    if (!write_dword(0x004788ABU, 0x2ACCU, eax) ||
        !write_dword(0x004788B1U, 0x2B0CU, eax)) {
        return finish();
    }
    const u8 test_d9c = static_cast<u8>(ecx) & static_cast<u8>(edx);
    flags = logical_flags(test_d9c, 0x80U);
    if (!write_dword(0x004788B9U, 0x2AF4U, eax) ||
        !write_word(0x004788BFU, 0x2A6CU, static_cast<u16>(eax)) ||
        !write_word(0x004788C6U, 0x2A70U, static_cast<u16>(eax)) ||
        !write_word(0x004788CDU, 0x2A72U, static_cast<u16>(eax)) ||
        !write_word(0x004788D4U, 0x2A7AU, static_cast<u16>(eax)) ||
        !write_word(0x004788DBU, 0x29A4U, static_cast<u16>(eax)) ||
        !write_dword(0x004788E2U, 0x2668U, 0x0FU) ||
        !write_dword(0x004788ECU, 0x266CU, ebp) ||
        !write_word(0x004788F2U, 0x2954U, static_cast<u16>(eax)) ||
        !write_word(0x004788F9U, 0x2A68U, 2U) ||
        !write_word(0x00478902U, 0x2A6AU, static_cast<u16>(ecx)) ||
        !write_dword(0x00478909U, 0x2674U, eax) ||
        !write_word(0x0047890FU, 0x2A0EU, static_cast<u16>(eax)) ||
        !write_dword(0x00478916U, 0x2690U, eax) ||
        !write_dword(0x0047891CU, 0x2670U, eax) ||
        !write_word(0x00478922U, 0x2A8AU, static_cast<u16>(eax)) ||
        !write_word(0x00478929U, 0x29A6U, static_cast<u16>(eax)) ||
        !write_word(0x00478930U, 0x29A8U, static_cast<u16>(eax)) ||
        !write_word(0x00478937U, 0x29AAU, static_cast<u16>(eax)) ||
        !write_word(0x0047893EU, 0x0316U, static_cast<u16>(eax)) ||
        !write_word(0x00478945U, 0x0318U, static_cast<u16>(eax)) ||
        !write_dword(0x0047894CU, 0x26CCU, eax)) {
        return finish();
    }
    if (!flags.zero && !write_dword(0x00478954U, 0x2AF0U, eax)) {
        return finish();
    }

    if (!read_byte(0x0047895AU, 0x0D94U, byte_value)) {
        return finish();
    }
    flags = logical_flags(static_cast<u8>(byte_value & 4U), 0x80U);
    if (!flags.zero) {
        if (!write_dword(0x00478963U, 0x2AF0U, eax)) {
            return finish();
        }
        u16 word_value{};
        if (!read_word(0x00478969U, 0x26D0U, word_value)) {
            return finish();
        }
        eax = word_value;
        eax &= 0x0000FFF7U;
        flags = logical_flags(eax, 0x80000000U);
        if (!write_dword(0x00478975U, 0x266CU, 0xFFFFFFE0U)) {
            return finish();
        }
        const u8 ah = static_cast<u8>((eax >> 8U) | 2U);
        eax = (eax & 0xFFFF00FFU) | (static_cast<u32>(ah) << 8U);
        flags = logical_flags(ah, 0x80U);
        if (!write_word(0x00478982U, 0x26D0U, static_cast<u16>(eax))) {
            return finish();
        }
    }

    u32 dword_value{};
    if (!read_dword(0x00478989U, 0x2B00U, dword_value)) {
        return finish();
    }
    flags = subtract_flags(dword_value, ebp);
    if (flags.zero && !write_dword(0x00478991U, 0x2AF0U, ebp)) {
        return finish();
    }

    if (!read_dword(0x00478997U, 0x2B04U, eax)) {
        return finish();
    }
    if (!push(0x0047899DU, edi)) {
        return finish();
    }
    flags = subtract_flags(eax, ebp);
    if (flags.zero) {
        if (!push(0x004789A2U, esi)) {
            return finish();
        }
        esi = ebx + 0x0D70U;
        edi = ebx + 0x0D50U;
        while (ecx != 0U) {
            const u32 source_offset = esi - request.actor_token;
            const u32 destination_offset = edi - request.actor_token;
            u32 copied{};
            if (!read_dword(0x004789AFU, source_offset, copied) ||
                !write_dword(0x004789AFU, destination_offset, copied)) {
                return finish();
            }
            const u32 step = request.direction_flag ? 0xFFFFFFFCU : 4U;
            esi += step;
            edi += step;
            --ecx;
            ++result.rep_iterations[0U];
        }
        if (!pop(0x004789B1U, request.entry_esi, esi)) {
            return finish();
        }
    }

    const auto repeat_store = [&](const std::size_t rep_index,
                                  const u32 instruction) {
        while (ecx != 0U) {
            const u32 destination_offset = edi - request.actor_token;
            if (!write_dword(instruction, destination_offset, eax)) {
                return false;
            }
            edi += request.direction_flag ? 0xFFFFFFFCU : 4U;
            --ecx;
            ++result.rep_iterations[rep_index];
        }
        return true;
    };

    ecx = 0x26U;
    eax = 0U;
    flags = logical_flags(eax, 0x80000000U);
    edi = ebx + 0x0338U;
    if (!repeat_store(1U, 0x004789C5U)) {
        return finish();
    }
    ecx = 0x26U;
    edi = ebx + 0x03D0U;
    if (!repeat_store(2U, 0x004789D2U)) {
        return finish();
    }
    ecx = 0x26U;
    edi = ebx + 0x0468U;
    if (!repeat_store(3U, 0x004789DFU)) {
        return finish();
    }
    ecx = 0x26U;
    edi = ebx + 0x0500U;
    if (!repeat_store(4U, 0x004789ECU)) {
        return finish();
    }
    edx = ebx + 0x0630U;
    ecx = 0x130U;
    edi = edx;
    if (!repeat_store(5U, 0x004789F5U)) {
        return finish();
    }
    eax = ebx + 0x2630U;
    ecx = 0U;
    flags = logical_flags(ecx, 0x80000000U);
    if (!write_dword(0x00478A01U, 0x2630U, ecx) ||
        !write_dword(0x00478A03U, 0x2634U, ecx) ||
        !write_dword(0x00478A06U, 0x2638U, ecx) ||
        !write_dword(0x00478A09U, 0x263CU, ecx)) {
        return finish();
    }
    ecx = 0x130U;
    eax = 0U;
    flags = logical_flags(eax, 0x80000000U);
    edi = edx;
    if (!repeat_store(6U, 0x00478A13U)) {
        return finish();
    }
    ecx = 0x0AU;
    edi = ebx + 0x0D90U;
    if (!repeat_store(7U, 0x00478A20U)) {
        return finish();
    }
    ecx = 0xFFFFFFFFU;
    flags = logical_flags(ecx, 0x80000000U);
    if (!pop(0x00478A25U, request.entry_edi, edi)) {
        return finish();
    }
    if (!write_dword(0x00478A26U, 0x2A56U, ecx) ||
        !write_dword(0x00478A2CU, 0x2A5AU, ecx) ||
        !write_dword(0x00478A32U, 0x2A5EU, ecx) ||
        !write_dword(0x00478A38U, 0x2A62U, ecx)) {
        return finish();
    }
    if (!read_dword(0x00478A3EU, 0x2AA0U, eax)) {
        return finish();
    }
    flags = subtract_flags(eax, ebp);
    if (flags.zero) {
        if (!push(0x00478A48U, 0x8CU) ||
            !push(
                kLegacyBattleActorRuntimeResetRandomCallAddress,
                kLegacyBattleActorRuntimeResetRandomReturnAddress
            )) {
            return finish();
        }
        if (!request.random_callable) {
            result.status =
                LegacyBattleActorRuntimeResetStatus::random_call_typed_stop;
            eip = 0x00439070U;
            return finish();
        }
        result.random_bound = 0x8CU;
        result.random_value = random.random_bounded(result.random_bound);
        result.random_calls = 1U;
        eax = result.random_value;
        ecx = request.random_return_ecx;
        edx = result.random_value;
        esp += 4U;
        esp += 4U;
        const u32 before_add = eax;
        eax += 0x32U;
        flags = add_flags(before_add, 0x32U);
        if (!write_word(0x00478A58U, 0x2A12U, static_cast<u16>(eax))) {
            return finish();
        }
    }

    if (!pop(0x00478A5FU, request.entry_ebp, ebp) ||
        !pop(0x00478A60U, request.entry_ebx, ebx)) {
        return finish();
    }
    if (!touch(
            LegacyBattleActorRuntimeResetAccessKind::stack_read,
            kLegacyBattleActorRuntimeResetReturnAddress,
            esp,
            request.entry_return_address
        )) {
        return finish();
    }
    esp += 4U;
    eip = request.entry_return_address;
    result.returned = true;
    return finish();
}

bool execute_legacy_battle_actor_runtime_reset_call(
    const LegacyBattleActorRuntimeResetOwners& owners,
    LegacyBattleBoundedRandomPort& random,
    LegacyBattleActorRuntimeResetCallTrace& trace,
    const LegacyBattleActorRuntimeResetCallRequests& requests,
    const u32 actor_token,
    const u32 entry_eax,
    const u32 entry_edx,
    const u32 call_address,
    const u32 return_address,
    const LegacyBattleActorCoordinateFlags& entry_flags,
    const bool entry_flags_known,
    const std::size_t request_offset
) noexcept {
    const std::size_t trace_index = trace.calls;
    const std::size_t request_index = request_offset + trace_index;
    LegacyBattleActorRuntimeResetRequest request{};
    if (request_index < requests.count) {
        request = requests.requests[request_index];
    }
    request.actor_token = actor_token;
    request.entry_eax = entry_eax;
    request.entry_edx = entry_edx;
    request.entry_return_address = return_address;
    request.entry_flags = entry_flags;
    request.entry_flags_known = entry_flags_known;

    trace.last = reset_legacy_battle_actor_runtime(
        resolve_legacy_battle_actor_runtime_reset(owners, actor_token),
        random,
        request
    );
    if (trace_index < trace.call_addresses.size()) {
        trace.call_addresses[trace_index] = call_address;
        trace.return_addresses[trace_index] = return_address;
        trace.actor_tokens[trace_index] = actor_token;
    }
    ++trace.calls;
    return trace.last.returned;
}

}  // namespace openswd3::battle
