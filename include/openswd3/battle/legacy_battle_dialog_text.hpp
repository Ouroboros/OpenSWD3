#pragma once

#include "openswd3/battle/legacy_battle_mon_definition.hpp"
#include "openswd3/asset_runtime/legacy_action_record.hpp"
#include "openswd3/story_scene/legacy_dialog_text.hpp"

#include <array>
#include <span>

namespace openswd3::battle {

// The mapped portion of the shared 4B8A14 scratch used by a 512-byte dialog.
// Other users must borrow this storage rather than duplicate its contents.
struct LegacyBattleDialogTextState {
    std::array<compat::u8, 512U> reference_name{};
};

enum class LegacyBattleDialogTextStatus : compat::u8 {
    completed,
    memory_access_typed_stop,
    decimal_typed_stop,
    mon_load_typed_stop,
    mon_release_typed_stop,
};

struct LegacyBattleDialogTextResult {
    LegacyBattleDialogTextStatus status{
        LegacyBattleDialogTextStatus::completed
    };
    compat::u32 metrics{};
    compat::u32 cursor{};
    compat::u32 diagnostics{};
    compat::u32 mon_load_calls{};
    compat::u32 release_calls{};
};

// sub_40B7F0 for the original zero-filled 512-byte dialog allocation.
// Names are borrowed from the live 49E148/49E158 owners, including their NUL.
[[nodiscard]] LegacyBattleDialogTextResult prepare_legacy_battle_dialog_text(
    std::span<compat::u8, 512U> text,
    std::span<const compat::u8> first_name,
    std::span<const compat::u8> second_name,
    LegacyBattleDialogTextState& state,
    LegacyBattleMonDatabasePort& mon
);

class LegacyBattleDialogFormatPort
    : public virtual LegacyBattleMonDatabasePort {
public:
    [[nodiscard]] virtual compat::u32
    allocate_battle_dialog_storage(compat::u32 bytes) = 0;
    [[nodiscard]] virtual bool
    update_battle_dialog_action(asset_runtime::LegacyActionRecord& action) = 0;
};

struct LegacyBattleDialogFormatBindings {
    compat::u32& scale;
    compat::u32& character_delay_base;
    std::span<asset_runtime::LegacyActionRecord, 4U> frame_actions;
    std::span<asset_runtime::LegacyActionRecord, 4U> caption_actions;
    std::span<const compat::u8> first_name;
    std::span<const compat::u8> second_name;
    LegacyBattleDialogTextState& text_state;
};

struct LegacyBattleDialogFormatRequest {
    std::span<const compat::u8> payload;
    std::span<const compat::u8> caption;
    compat::u32 mode{};
    compat::u32 x{};
    compat::u32 y{};
};

enum class LegacyBattleDialogFormatStatus : compat::u8 {
    completed,
    unsupported_call,
    memory_access_typed_stop,
    allocation_typed_stop,
    text_preparation_typed_stop,
};

struct LegacyBattleDialogFormatResult {
    LegacyBattleDialogFormatStatus status{
        LegacyBattleDialogFormatStatus::completed
    };
    LegacyBattleDialogTextResult preparation;
    compat::u32 diagnostics{};
    compat::u32 allocations{};
    compat::u32 action_updates{};
};

// sub_40AFF0 restricted to the actual case2/44/59 calls: flags=800h and
// mode=1 or 10000h. This mutates the existing, caller-owned message in order.
[[nodiscard]] LegacyBattleDialogFormatResult format_legacy_battle_dialog(
    story_scene::LegacyDialogMessage& message,
    const LegacyBattleDialogFormatRequest& request,
    LegacyBattleDialogFormatBindings bindings,
    LegacyBattleDialogFormatPort& port
);

}  // namespace openswd3::battle
