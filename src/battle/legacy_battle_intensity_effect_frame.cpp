#include "openswd3/battle/legacy_battle_effect_frame.hpp"

#include <algorithm>
#include <bit>

namespace openswd3::battle {
namespace {

using compat::i16;
using compat::i32;
using compat::i8;
using compat::u16;
using compat::u32;
using compat::u8;

constexpr u32 kCallQueryCoordinates = 0x004783B0U;
constexpr u32 kCallInitializeRecord = 0x004321E0U;
constexpr u32 kCallLookupResource = 0x004315D0U;
constexpr u32 kCallRenderResource = 0x004170E0U;
constexpr u32 kRecordBaseToken = 0x00524980U;

[[nodiscard]] constexpr i16 signed_word(const u32 value) noexcept {
    return std::bit_cast<i16>(static_cast<u16>(value));
}

[[nodiscard]] constexpr u32 to_bits(const i32 value) noexcept {
    return std::bit_cast<u32>(value);
}

void replace_low_word(u32& destination, const u16 value) noexcept {
    destination = (destination & 0xFFFF0000U) | value;
}

[[nodiscard]] constexpr u32 record_token(const u32 slot) noexcept {
    return kRecordBaseToken + slot * kLegacyBattleEffectRecordStride;
}

}  // namespace

LegacyBattleIntensityEffectFrameResult
advance_legacy_battle_intensity_effect_frame(
    LegacyBattleEffectFrameState& state,
    LegacyBattleEffectCallPort& port,
    const u32 actor_token,
    const u32 source_value,
    const u32 secondary_value,
    const u32 slot_index
) {
    LegacyBattleIntensityEffectFrameResult result{};
    result.final_edx = secondary_value;
    if (source_value == 0U) {
        result.return_value = 1U;
        return result;
    }
    if (slot_index >= state.intensity_records.size()) {
        result.status =
            LegacyBattleIntensityEffectFrameStatus::slot_index_typed_stop;
        return result;
    }
    auto& intensity = state.intensity_values[slot_index];
    if (intensity <= static_cast<i8>(-32)) {
        intensity = 0;
        result.return_value = 1U;
        return result;
    }

    auto invoke = [&](const u32 callee,
                      const std::initializer_list<u32> arguments = {}) {
        LegacyBattleEffectCallRequest request{};
        request.callee_token = callee;
        std::copy(
            arguments.begin(), arguments.end(), request.arguments.begin()
        );
        ++result.port_calls;
        return port.invoke(request);
    };

    const auto coordinates = invoke(kCallQueryCoordinates, {actor_token});
    u32 x = coordinates.outputs[0];
    u32 y = coordinates.outputs[1];

    auto& record = state.intensity_records[slot_index];
    record.source_value = source_value;
    record.secondary_value = secondary_value;
    record.mode_snapshot = state.global_mode == 1U ? 1U : 0U;
    const auto initialized =
        invoke(kCallInitializeRecord, {record_token(slot_index)});
    result.final_edx = initialized.edx;
    if (initialized.eax == 0U) {
        result.return_value = initialized.eax;
        return result;
    }

    u32 lookup_key_b = initialized.edx;
    replace_low_word(lookup_key_b, record.lookup_key_b);
    u32 lookup_key_a = initialized.eax;
    replace_low_word(lookup_key_a, record.lookup_key_a);
    const auto resource =
        invoke(kCallLookupResource, {lookup_key_a, lookup_key_b});
    if (resource.eax == 0U) {
        result.status =
            LegacyBattleIntensityEffectFrameStatus::resource_owner_typed_stop;
        result.final_edx = resource.edx;
        return result;
    }
    state.current_resource_value_token = resource.outputs[0];

    const i32 signed_intensity = static_cast<i32>(intensity);
    state.render_intensity_a = signed_intensity;
    state.render_intensity_b = signed_intensity;
    state.render_intensity_c = signed_intensity;

    const u32 render_x =
        to_bits(static_cast<i32>(signed_word(x))) - record.x_offset;
    const u32 render_y =
        to_bits(static_cast<i32>(signed_word(y))) - record.y_offset;
    const auto rendered = invoke(
        kCallRenderResource,
        {render_x,
         render_y,
         static_cast<u16>(resource.outputs[1]),
         static_cast<u16>(resource.outputs[2]),
         record.render_flags,
         resource.outputs[3]}
    );
    result.final_edx = rendered.edx;
    const u8 decremented = static_cast<u8>(std::bit_cast<u8>(intensity) - 4U);
    intensity = std::bit_cast<i8>(decremented);
    result.return_value = 0U;
    return result;
}

}  // namespace openswd3::battle
