#pragma once

#include "openswd3/compat/types.hpp"

namespace openswd3::resource_io {

class LegacyDbcsTextBuffer;

}  // namespace openswd3::resource_io

namespace openswd3::input_time_rng {

inline constexpr compat::u32 kLegacyInputLanguageChangeMessage = 0x0051U;
inline constexpr compat::u32 kLegacyKeyDownMessage = 0x0100U;
inline constexpr compat::u32 kLegacyCharacterMessage = 0x0102U;
inline constexpr compat::u32 kLegacyImeCharacterMessage = 0x0286U;
inline constexpr compat::u32 kLegacyPercentInputSoundId = 0x008CU;

struct LegacyTextInputDriverState {
    compat::u32 dbcs_lead_byte_latch{};
};

class LegacyTextInputPorts {
public:
    virtual ~LegacyTextInputPorts() = default;

    [[nodiscard]] virtual bool
    is_ime_keyboard_layout(compat::u32 keyboard_layout) = 0;
    virtual void play_sound_effect(compat::u32 sound_id) = 0;
};

[[nodiscard]] compat::i32
legacy_text_input_enabled(resource_io::LegacyDbcsTextBuffer& buffer) noexcept;

[[nodiscard]] compat::i32 legacy_set_text_input_enabled(
    resource_io::LegacyDbcsTextBuffer& buffer, compat::i32 requested_state
) noexcept;

[[nodiscard]] compat::u32 legacy_move_text_cursor_previous(
    resource_io::LegacyDbcsTextBuffer& buffer
) noexcept;

[[nodiscard]] compat::u32 legacy_move_text_cursor_next(
    resource_io::LegacyDbcsTextBuffer& buffer
) noexcept;

[[nodiscard]] compat::u32 legacy_insert_text_bytes(
    resource_io::LegacyDbcsTextBuffer& buffer, const compat::u8* text
);

[[nodiscard]] compat::u32 legacy_delete_text_at_cursor(
    resource_io::LegacyDbcsTextBuffer& buffer
) noexcept;

[[nodiscard]] compat::u32 filter_legacy_text_input_message(
    resource_io::LegacyDbcsTextBuffer& buffer,
    LegacyTextInputDriverState& state,
    compat::u32 message,
    compat::u32 first_parameter,
    compat::u32 second_parameter,
    LegacyTextInputPorts& ports
);

}  // namespace openswd3::input_time_rng
