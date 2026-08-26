#include "openswd3/battle/legacy_battle_action_rotation_cache.hpp"

#include <array>
#include <cstring>
#include <vector>

namespace openswd3::battle {
namespace {

struct CycleSnapshot {
    asset_runtime::LegacyActionRecord action_record{};
    std::array<compat::u16, 3> local_frame_slots{};
    std::array<compat::u32, 3> frame_owner_tokens{};
    compat::u32 field_bc{};
    std::uint64_t domain_token{};
};

[[nodiscard]] bool same_cycle_snapshot(
    const CycleSnapshot& left, const CycleSnapshot& right
) noexcept {
    return std::memcmp(
               &left.action_record,
               &right.action_record,
               sizeof(left.action_record)
           ) == 0 &&
        left.local_frame_slots == right.local_frame_slots &&
        left.frame_owner_tokens == right.frame_owner_tokens &&
        left.field_bc == right.field_bc &&
        left.domain_token == right.domain_token;
}

[[nodiscard]] bool rotation_returned_normally(
    const LegacyBattleImageRotationStatus status
) noexcept {
    return status == LegacyBattleImageRotationStatus::completed ||
        status == LegacyBattleImageRotationStatus::shift_not_positive ||
        status == LegacyBattleImageRotationStatus::magic_mismatch ||
        status ==
        LegacyBattleImageRotationStatus::first_row_flags_unsupported ||
        status == LegacyBattleImageRotationStatus::mode_out_of_range;
}

}  // namespace

LegacyBattleActionRotationCacheResult
initialize_legacy_battle_action_rotation_cache(
    LegacyBattleActionRotationCacheState& state,
    LegacyBattleActionRotationUpdatePort& update_port,
    LegacyBattleMutableFrameImagePort& image_port,
    const compat::u32,
    const compat::u32 field_b4,
    const compat::u32 field_b8,
    const compat::u32 initial_action_id,
    const compat::u32 rotation_divisor
) {
    LegacyBattleActionRotationCacheResult result;
    state.stored_action_id = static_cast<compat::u16>(initial_action_id);
    state.field_b4 = field_b4;
    state.field_b8 = field_b8;
    state.action_record.action_id =
        static_cast<compat::u32>(state.stored_action_id);
    state.action_record.base_variant = 0U;

    LegacyBattleActionRotationUpdateSnapshot update =
        update_port.update_action(state.action_record);
    ++result.action_update_calls;
    if (update.eax == 0U) {
        result.status = LegacyBattleActionRotationCacheStatus::
            initial_action_update_stopped;
        return result;
    }

    std::vector<CycleSnapshot> seen;
    for (;;) {
        const CycleSnapshot current{
            .action_record = state.action_record,
            .local_frame_slots = result.local_frame_slots,
            .frame_owner_tokens = state.frame_owner_tokens,
            .field_bc = state.field_bc,
            .domain_token = update.domain_token,
        };
        bool repeated = false;
        for (const CycleSnapshot& prior : seen) {
            if (same_cycle_snapshot(prior, current)) {
                repeated = true;
                break;
            }
        }
        if (repeated) {
            result.status = LegacyBattleActionRotationCacheStatus::
                action_loop_nonterminating;
            return result;
        }
        seen.push_back(current);
        ++result.loop_iterations;

        compat::u32 frame_eax = (update.eax & 0xFFFFFF00U) |
            static_cast<compat::u32>(state.action_record.field_88);
        if (state.action_record.field_88 != 0U) {
            frame_eax &= 0xFFU;
            state.field_bc = frame_eax;
            ++result.field_bc_writes;
        }
        frame_eax = (frame_eax & 0xFFFF0000U) |
            static_cast<compat::u32>(state.action_record.field_4c);
        const compat::u32 frame_index = frame_eax;
        const compat::u16 local_index = state.action_record.field_4c;
        result.last_frame_index = frame_index;
        if (local_index >= result.local_frame_slots.size()) {
            result.status =
                LegacyBattleActionRotationCacheStatus::frame_index_out_of_range;
            return result;
        }

        if (result.local_frame_slots[local_index] == 0xFFFFU) {
            const compat::u32 resource_id = (update.edx & 0xFFFF0000U) |
                static_cast<compat::u32>(state.action_record.field_4a);
            result.last_resource_id = resource_id;
            LegacyBattleMutableFrameImage image =
                image_port.query_frame_image(resource_id, frame_index);
            ++result.frame_query_calls;
            state.frame_owner_tokens[local_index] = image.owner_token;
            result.local_frame_slots[local_index] = local_index;

            const compat::u16 divisor =
                static_cast<compat::u16>(rotation_divisor);
            if (divisor == 0U) {
                result.status =
                    LegacyBattleActionRotationCacheStatus::division_by_zero;
                return result;
            }
            result.rotation_shift = 640 / static_cast<compat::i32>(divisor);
            if (!image.pointer_valid) {
                result.status = LegacyBattleActionRotationCacheStatus::
                    frame_image_pointer_invalid;
                return result;
            }

            result.rotation = rotate_legacy_battle_literal_image(
                image.bytes,
                LegacyBattleImageRotationMode::pixels_right,
                result.rotation_shift
            );
            ++result.rotation_calls;
            if (!rotation_returned_normally(result.rotation.status)) {
                result.status =
                    LegacyBattleActionRotationCacheStatus::rotation_typed_stop;
                return result;
            }
        } else {
            ++result.skipped_cached_frames;
        }

        if (state.action_record.command_cursor == 0U) {
            std::memset(
                &state.action_record, 0, asset_runtime::kLegacyActionRecordSize
            );
            ++result.record_clear_calls;
            return result;
        }

        state.action_record.base_variant = 0U;
        state.action_record.action_id =
            static_cast<compat::u32>(state.stored_action_id);
        update = update_port.update_action(state.action_record);
        ++result.action_update_calls;
        if (update.eax == 0U) {
            result.status =
                LegacyBattleActionRotationCacheStatus::action_update_stopped;
            return result;
        }
    }
}

}  // namespace openswd3::battle
