#include "test.hpp"

#include "openswd3/world_map/legacy_world_head_sign_actions.hpp"

#include <array>
#include <vector>

namespace {

using openswd3::asset_runtime::LegacyActionDrawPorts;
using openswd3::asset_runtime::LegacyActionRecord;
using openswd3::asset_runtime::LegacyActionUpdateStatus;
using openswd3::compat::i32;
using openswd3::compat::u16;
using openswd3::compat::u32;
using openswd3::rendering::LegacyBlitExecutionStatus;
using openswd3::rendering::LegacyFramePiece;
using openswd3::world_map::advance_legacy_world_head_sign_actions;
using openswd3::world_map::kLegacyWorldHeadSignActionBaseAddress;
using openswd3::world_map::kLegacyWorldHeadSignActionCount;
using openswd3::world_map::kLegacyWorldHeadSignActionId;
using openswd3::world_map::legacy_world_head_sign_action_token;
using openswd3::world_map::LegacyWorldHeadSignActionsState;
using openswd3::world_map::LegacyWorldHeadSignActionsStatus;
using openswd3::world_map::resolve_legacy_world_head_sign_action;

class RecordingActionPorts final : public LegacyActionDrawPorts {
public:
    [[nodiscard]] LegacyActionUpdateStatus
    update_action_record(LegacyActionRecord& record) override {
        variants.push_back(record.base_variant);
        if (record.base_variant == failed_variant) {
            return LegacyActionUpdateStatus::stream_load_failed;
        }
        return LegacyActionUpdateStatus::completed;
    }

    [[nodiscard]] bool
    load_frame_piece(const u16, const u16, LegacyFramePiece&) override {
        return false;
    }

    [[nodiscard]] LegacyBlitExecutionStatus draw_frame_piece(
        const LegacyFramePiece&, const i32, const i32, const u32, const i32
    ) noexcept override {
        return LegacyBlitExecutionStatus::completed;
    }

    u32 failed_variant{0xFFFFFFFFU};
    std::vector<u32> variants;
};

void test_initial_layout_and_reverse_update(openswd3::test::Context& test) {
    LegacyWorldHeadSignActionsState state;
    RecordingActionPorts ports;

    bool layout_matches =
        state.records.size() == kLegacyWorldHeadSignActionCount;
    for (std::size_t index = 0U; index < state.records.size(); ++index) {
        const LegacyActionRecord& record = state.records[index];
        const bool active = index < 4U;
        layout_matches = layout_matches &&
            record.action_id == (active ? kLegacyWorldHeadSignActionId : 0U) &&
            record.base_variant == (active ? static_cast<u32>(index) : 0U) &&
            record.field_1c == 0xFFFFFFFFU &&
            record.one_shot_base_variant == 0xFFFFFFFFU &&
            record.one_shot_variant_delta == 0xFFFFFFFFU;
    }

    const auto result = advance_legacy_world_head_sign_actions(state, ports);
    test.expect_true(
        layout_matches, "startup owns eight initialized records and four signs"
    );
    test.expect_equal(
        ports.variants,
        std::vector<u32>({3U, 2U, 1U, 0U}),
        "active head signs update from the highest slot down"
    );
    test.expect_true(
        result.status == LegacyWorldHeadSignActionsStatus::completed &&
            result.visited_count == 8U && result.active_count == 4U &&
            result.update_count == 4U && result.update_failure_count == 0U,
        "zero action ids are visited but do not call the updater"
    );
}

void test_failure_is_diagnostic_only(openswd3::test::Context& test) {
    LegacyWorldHeadSignActionsState state;
    RecordingActionPorts ports;
    ports.failed_variant = 2U;

    const auto result = advance_legacy_world_head_sign_actions(state, ports);
    test.expect_equal(
        ports.variants,
        std::vector<u32>({3U, 2U, 1U, 0U}),
        "a failed update does not stop lower-address records"
    );
    test.expect_true(
        result.status ==
                LegacyWorldHeadSignActionsStatus::
                    completed_with_update_failures &&
            result.visited_count == 8U && result.active_count == 4U &&
            result.update_count == 4U && result.update_failure_count == 1U,
        "the nullsub diagnostic branch remains nonfatal and observable"
    );
}

void test_original_address_tokens_resolve_exact_slots(
    openswd3::test::Context& test
) {
    LegacyWorldHeadSignActionsState state;

    test.expect_true(
        legacy_world_head_sign_action_token(0U) ==
                kLegacyWorldHeadSignActionBaseAddress &&
            legacy_world_head_sign_action_token(3U) ==
                kLegacyWorldHeadSignActionBaseAddress + 3U * 0x98U &&
            resolve_legacy_world_head_sign_action(
                state, legacy_world_head_sign_action_token(3U)
            ) == &state.records[3] &&
            resolve_legacy_world_head_sign_action(
                state, kLegacyWorldHeadSignActionBaseAddress + 1U
            ) == nullptr &&
            resolve_legacy_world_head_sign_action(
                state, legacy_world_head_sign_action_token(8U)
            ) == nullptr,
        "original head-sign addresses map only to the eight aligned slots"
    );
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_initial_layout_and_reverse_update(test);
    test_failure_is_diagnostic_only(test);
    test_original_address_tokens_resolve_exact_slots(test);
    return test.exit_code();
}
