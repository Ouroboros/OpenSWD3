#include "openswd3/battle/legacy_battle_guard_panel_frame.hpp"

#include <algorithm>
#include <array>
#include <functional>
#include <ranges>
#include <vector>

#include "test.hpp"

namespace {

using openswd3::compat::u8;
using openswd3::compat::u16;
using openswd3::compat::u32;
using Call = openswd3::battle::LegacyBattleGridFrameCall;
using Reply = openswd3::battle::LegacyBattleGridFrameCallReply;
using Request = openswd3::battle::LegacyBattleGridFrameCallRequest;

class ActionStreamProvider final
    : public openswd3::asset_runtime::LegacyActionStreamProvider {
public:
    [[nodiscard]] openswd3::asset_runtime::LegacyActionStreamLoadResult
    load_action_stream(const u32 action_id, const u32, const bool) override {
        constexpr std::array<u16, 8> kWords{
            0x5246U, 0x0077U, 0x5041U, 0U, 0x5859U, 2U, 3U, 0x4544U
        };
        bytes.clear();
        for (const u16 word : kWords) {
            bytes.push_back(static_cast<u8>(word));
            bytes.push_back(static_cast<u8>(word >> 8U));
        }
        if (action_id == failing_action_id) {
            return {};
        }
        return {
            openswd3::asset_runtime::LegacyActionStreamStatus::ready,
            bytes,
            false,
        };
    }

    std::vector<u8> bytes;
    u32 failing_action_id{0xFFFFFFFFU};
};

class FrameProvider final
    : public openswd3::rendering::LegacyFramePieceProvider {
public:
    FrameProvider() {
        for (std::size_t index = 0U; index < storage.size(); ++index) {
            const u16 color = static_cast<u16>(0x6100U + index);
            storage[index] = {
                static_cast<u8>(color), static_cast<u8>(color >> 8U)
            };
        }
    }

    [[nodiscard]] bool load_frame_piece(
        const u32 resource_id,
        const u32 piece_index,
        openswd3::rendering::LegacyFramePiece& piece
    ) noexcept override {
        resource_ids.push_back(resource_id);
        if (resource_id == failing_resource || piece_index >= storage.size()) {
            piece = {};
            return false;
        }
        piece = {
            .source =
                {
                    .bytes = storage[piece_index],
                    .layout =
                        openswd3::rendering::LegacyBlitSourceLayout::direct_16,
                },
            .width = 1U,
            .height = 1U,
        };
        return true;
    }

    std::array<std::vector<u8>, 9> storage;
    std::vector<u32> resource_ids;
    u32 failing_resource{0xFFFFFFFFU};
};

class GridPort final : public openswd3::battle::LegacyBattleGridFramePort {
public:
    void reply(const Call call, const Reply& value) {
        replies[static_cast<std::size_t>(call)].push_back(value);
    }

    [[nodiscard]] Reply invoke_grid_frame(const Request& request) override {
        calls.push_back(request);
        if (on_call) {
            on_call(request);
        }
        const std::size_t index = static_cast<std::size_t>(request.call);
        const std::size_t offset = reply_offsets[index]++;
        return offset < replies[index].size() ? replies[index][offset]
                                              : Reply{};
    }

    [[nodiscard]] std::vector<Request> calls_of(const Call call) const {
        std::vector<Request> selected;
        std::ranges::copy_if(
            calls,
            std::back_inserter(selected),
            [call](const Request& request) { return request.call == call; }
        );
        return selected;
    }

    std::vector<Request> calls;
    std::array<std::vector<Reply>, 13> replies;
    std::array<std::size_t, 13> reply_offsets{};
    std::function<void(const Request&)> on_call;
};

struct Fixture {
    Fixture() : action_updater(action_streams), raster(framebuffer.geometry()) {
        panel_action_record.field_30 = 0xDEADBEEFU;
    }

    [[nodiscard]] openswd3::battle::LegacyBattleGuardPanelFrameBindings
    bindings() {
        return {
            .group_b_row_selection = group_b_row_selection,
            .group_a_count = group_a_count,
            .target_effect_value = target_effect_value,
            .panel_action_record = panel_action_record,
            .framebuffer = framebuffer,
            .raster = raster,
            .shared_effects = effects,
            .jitter = jitter,
            .action_updater = action_updater,
            .frame_provider = frame_provider,
        };
    }

    u32 group_b_row_selection{2U};
    u32 group_a_count{4U};
    u32 target_effect_value{2U << 16U};
    openswd3::asset_runtime::LegacyActionRecord panel_action_record;
    openswd3::rendering::LegacyFramebuffer framebuffer{{
        .pitch_bytes = 1280,
        .width = 640,
        .height = 480,
    }};
    ActionStreamProvider action_streams;
    openswd3::asset_runtime::LegacyActionUpdater action_updater;
    openswd3::rendering::LegacyRasterGeometryState raster;
    openswd3::rendering::LegacyBlitEffectState effects;
    openswd3::rendering::LegacyRleRowJitterState jitter;
    FrameProvider frame_provider;
    GridPort port;
};

[[nodiscard]] openswd3::battle::LegacyBattleGuardPanelFrameRequest request() {
    openswd3::battle::LegacyBattleGuardPanelFrameRequest value{
        .origin_x = 196U,
        .origin_y = 206U,
        .entry_eax = 0x11111111U,
        .entry_ecx = 0x22222222U,
        .entry_edx = 0x33333333U,
        .panel_rectangle_return_registers =
            {
                .eax = 0xAAAA1234U,
                .ecx = 0x44000002U,
                .edx = 0x44000003U,
            },
        .selection_rectangle_return_registers = {
            .eax = 0x77000001U,
            .ecx = 0x77000002U,
            .edx = 0x77000003U,
        },
    };
    value.tiled_frame_return_registers[0U] = {
        .eax = 0x55000001U,
        .ecx = 0x55000002U,
        .edx = 0x55000003U,
    };
    value.tiled_frame_return_registers[1U] = {
        .eax = 0x66000001U,
        .ecx = 0x66000002U,
        .edx = 0x66000003U,
    };
    return value;
}

void prepare_two_rows(Fixture& fixture) {
    fixture.port.reply(
        Call::draw_text, {.eax = 0x101U, .ecx = 0x102U, .edx = 0xBBBB5678U}
    );
    fixture.port.reply(
        Call::query_guard_actor_label,
        {.eax = 0x00501000U, .ecx = 0x202U, .edx = 0x203U}
    );
    fixture.port.reply(Call::draw_text, {});
    fixture.port.reply(
        Call::query_guard_actor_label,
        {.eax = 0x00502000U, .ecx = 0x302U, .edx = 0x303U}
    );
    fixture.port.reply(Call::draw_text, {});
}

}  // namespace

void test_battle_guard_panel_frame(openswd3::test::Context& test) {
    {
        Fixture fixture;
        prepare_two_rows(fixture);
        const auto result =
            openswd3::battle::draw_legacy_battle_guard_panel_frame(
                fixture.bindings(), fixture.port, request()
            );
        const auto labels =
            fixture.port.calls_of(Call::query_guard_actor_label);
        const auto widths = fixture.port.calls_of(Call::configure_font_width);
        const auto draws = fixture.port.calls_of(Call::draw_text);
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleGuardPanelFrameStatus::
                        completed &&
                result.panel_action_update_calls == 1U &&
                result.panel_rectangle_calls == 1U &&
                result.tiled_frame_calls == 2U &&
                result.selection_rectangle_calls == 1U &&
                result.actor_label_query_calls == 2U &&
                result.font_width_calls == 3U && result.text_draw_calls == 3U &&
                result.displayed_rows == 2U && !result.missing_row_drawn,
            "guard panel draws its fixed frame, selection and two configured actor rows"
        );
        test.expect_true(
            fixture.panel_action_record.action_id == 0x233BU &&
                fixture.panel_action_record.base_variant == 0U &&
                !fixture.frame_provider.resource_ids.empty() &&
                fixture.frame_provider.resource_ids.front() == 0xAAAA0077U &&
                fixture.frame_provider.resource_ids.back() == 0xBBBB0077U,
            "guard panel preserves untouched action-record fields and distinct stale resource high words"
        );
        test.expect_true(
            result.rows[0U].actor_index == 2U && result.rows[0U].x == 208U &&
                result.rows[0U].y == 214U &&
                result.rows[1U].actor_index == 3U &&
                result.rows[1U].x == 208U && result.rows[1U].y == 236U &&
                labels.size() == 2U && labels[0U].eax == 0x7DEU &&
                labels[0U].edx == 0x179AU && widths.size() == 3U &&
                widths[0U].arguments[0U] == 0x12U &&
                widths[1U].arguments[0U] == 0x12U &&
                widths[2U].arguments[0U] == 0x10U,
            "guard panel enumerates the final group-A segment with per-row width eighteen"
        );
        test.expect_true(
            draws.size() == 3U && draws[0U].arguments[1U] == 252U &&
                draws[0U].arguments[2U] == 180U &&
                draws[0U].text_token ==
                    openswd3::battle::kLegacyBattleGuardPanelTitleToken &&
                draws[1U].text_token == 0x00501000U &&
                draws[1U].arguments[1U] == 208U &&
                draws[1U].arguments[2U] == 214U && draws[1U].eax == 214U &&
                draws[1U].edx == 0U && draws[2U].text_token == 0x00502000U &&
                draws[2U].eax == 236U && draws[2U].edx == 11U,
            "guard panel preserves fixed title and row draw callsite register shapes"
        );
    }

    {
        Fixture fixture;
        fixture.group_a_count = 1U;
        fixture.target_effect_value = 1U << 16U;
        fixture.port.reply(
            Call::draw_text, {.eax = 0x101U, .ecx = 0x102U, .edx = 0xBBBB5678U}
        );
        fixture.port.reply(Call::query_guard_actor_label, {.eax = 0x00501000U});
        fixture.port.reply(
            Call::draw_text, {.eax = 0xABCDEF01U, .ecx = 0x202U, .edx = 0x203U}
        );
        const auto result =
            openswd3::battle::draw_legacy_battle_guard_panel_frame(
                fixture.bindings(), fixture.port, request()
            );
        const auto draws = fixture.port.calls_of(Call::draw_text);
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleGuardPanelFrameStatus::
                        completed &&
                result.displayed_rows == 1U && result.missing_row_drawn &&
                draws.size() == 3U &&
                draws.back().text_token ==
                    openswd3::battle::kLegacyBattleMissingGuardTextToken &&
                draws.back().arguments[1U] == 208U &&
                draws.back().arguments[2U] == 236U &&
                draws.back().eax == 0xABCDEF01U &&
                draws.back().edx == 0x004CD76CU,
            "guard panel adds the fixed missing row only when the zero-extended count is one"
        );
    }

    {
        Fixture fixture;
        fixture.target_effect_value = 0U;
        fixture.port.reply(
            Call::draw_text, {.eax = 0x101U, .ecx = 0x102U, .edx = 0xBBBB5678U}
        );
        const auto result =
            openswd3::battle::draw_legacy_battle_guard_panel_frame(
                fixture.bindings(), fixture.port, request()
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleGuardPanelFrameStatus::
                        completed &&
                result.actor_label_query_calls == 0U &&
                result.font_width_calls == 1U && result.text_draw_calls == 1U,
            "guard panel zero count skips actor rows but retains selection and final width"
        );
    }

    {
        Fixture fixture;
        fixture.action_streams.failing_action_id = 0x233BU;
        fixture.target_effect_value = 0U;
        fixture.port.reply(Call::draw_text, {.edx = 0xBBBB5678U});
        const auto result =
            openswd3::battle::draw_legacy_battle_guard_panel_frame(
                fixture.bindings(), fixture.port, request()
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleGuardPanelFrameStatus::
                        completed &&
                result.panel_action_update.return_value == 0U &&
                result.tiled_frame_calls == 2U,
            "guard panel action update failure remains non-branching"
        );
    }

    {
        Fixture fixture;
        fixture.raster.surface.pitch_bytes = 1;
        const auto result =
            openswd3::battle::draw_legacy_battle_guard_panel_frame(
                fixture.bindings(), fixture.port, request()
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleGuardPanelFrameStatus::
                        panel_rectangle_typed_stop &&
                result.panel_action_update_calls == 1U &&
                result.tiled_frame_calls == 0U,
            "guard panel rectangle stop preserves only the action prefix"
        );
    }

    {
        Fixture fixture;
        fixture.frame_provider.failing_resource = 0xAAAA0077U;
        const auto result =
            openswd3::battle::draw_legacy_battle_guard_panel_frame(
                fixture.bindings(), fixture.port, request()
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleGuardPanelFrameStatus::
                        first_tiled_frame_typed_stop &&
                result.tiled_frame_calls == 1U && result.text_draw_calls == 0U,
            "guard panel first tiled stop blocks title and body"
        );
    }

    {
        Fixture fixture;
        fixture.frame_provider.failing_resource = 0xBBBB0077U;
        fixture.port.reply(Call::draw_text, {.edx = 0xBBBB5678U});
        const auto result =
            openswd3::battle::draw_legacy_battle_guard_panel_frame(
                fixture.bindings(), fixture.port, request()
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleGuardPanelFrameStatus::
                        second_tiled_frame_typed_stop &&
                result.tiled_frame_calls == 2U && result.text_draw_calls == 1U,
            "guard panel second tiled stop preserves title draw"
        );
    }

    {
        Fixture fixture;
        fixture.group_b_row_selection = 2U;
        fixture.port.reply(Call::draw_text, {.edx = 0xBBBB5678U});
        fixture.port.on_call = [&fixture](const Request& call) {
            if (call.call == Call::draw_text) {
                fixture.raster.surface.pitch_bytes = 600;
            }
        };
        const auto result =
            openswd3::battle::draw_legacy_battle_guard_panel_frame(
                fixture.bindings(), fixture.port, request()
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleGuardPanelFrameStatus::
                        selection_rectangle_typed_stop &&
                result.selection_rectangle_calls == 1U &&
                result.actor_label_query_calls == 0U &&
                result.return_eax == 0x77000001U &&
                result.return_ecx == 0x77000002U &&
                result.return_edx == 0x77000003U,
            "guard panel selection stop blocks actor rows and preserves rectangle return registers"
        );
    }

    {
        Fixture fixture;
        fixture.group_a_count = 1U;
        fixture.target_effect_value = 2U << 16U;
        fixture.port.reply(Call::draw_text, {.edx = 0xBBBB5678U});
        const auto result =
            openswd3::battle::draw_legacy_battle_guard_panel_frame(
                fixture.bindings(), fixture.port, request()
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleGuardPanelFrameStatus::
                        group_a_actor_typed_stop &&
                result.font_width_calls == 1U &&
                result.actor_label_query_calls == 0U &&
                result.return_eax == 0xFFFFFC11U &&
                result.return_ecx == 0x004FFA9CU &&
                result.return_edx == 0xFFFFF433U,
            "guard panel invalid tail index stops after width eighteen with pre-query registers"
        );
    }

    {
        Fixture fixture;
        fixture.group_a_count = 0U;
        fixture.target_effect_value = 0x8000U << 16U;
        fixture.port.reply(Call::draw_text, {.edx = 0xBBBB5678U});
        const auto result =
            openswd3::battle::draw_legacy_battle_guard_panel_frame(
                fixture.bindings(), fixture.port, request()
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleGuardPanelFrameStatus::
                        group_a_actor_typed_stop &&
                result.font_width_calls == 1U,
            "guard panel treats high-bit counts as nonzero after zero extension rather than signed i16"
        );
    }
}
