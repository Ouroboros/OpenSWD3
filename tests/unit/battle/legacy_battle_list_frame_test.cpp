#include "openswd3/battle/legacy_battle_list_frame.hpp"

#include <algorithm>
#include <array>
#include <ranges>
#include <span>
#include <vector>

#include "test.hpp"

namespace {

using openswd3::compat::u8;
using openswd3::compat::u16;
using openswd3::compat::u32;

class ActionStreamProvider final
    : public openswd3::asset_runtime::LegacyActionStreamProvider {
public:
    ActionStreamProvider() {
        set_resource(0x0066U);
    }

    [[nodiscard]] openswd3::asset_runtime::LegacyActionStreamLoadResult
    load_action_stream(
        const u32 action_id, const u32 variant_index, const bool
    ) override {
        action_ids.push_back(action_id);
        variants.push_back(variant_index);
        if (action_id == failing_action_id) {
            return {};
        }
        set_resource(action_id == 0x233BU ? 0x0077U : 0x0066U);
        return {
            openswd3::asset_runtime::LegacyActionStreamStatus::ready,
            bytes,
            false,
        };
    }

    void set_resource(const u16 resource) {
        constexpr std::array<u16, 8> kTemplate{
            0x5246U, 0U, 0x5041U, 0U, 0x5859U, 2U, 3U, 0x4544U
        };
        auto words = kTemplate;
        words[1U] = resource;
        bytes.clear();
        for (const u16 word : words) {
            bytes.push_back(static_cast<u8>(word));
            bytes.push_back(static_cast<u8>(word >> 8U));
        }
    }

    std::vector<u8> bytes;
    std::vector<u32> action_ids;
    std::vector<u32> variants;
    u32 failing_action_id{0xFFFFFFFFU};
};

class FrameProvider final
    : public openswd3::rendering::LegacyFramePieceProvider {
public:
    FrameProvider() {
        for (std::size_t index = 0U; index < storage.size(); ++index) {
            const u16 color = static_cast<u16>(0x1000U + index);
            storage[index] = {
                static_cast<u8>(color),
                static_cast<u8>(color >> 8U),
            };
        }
    }

    [[nodiscard]] bool load_frame_piece(
        const u32 resource_id,
        const u32 piece_index,
        openswd3::rendering::LegacyFramePiece& piece
    ) noexcept override {
        resource_ids.push_back(resource_id);
        piece_indices.push_back(piece_index);
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
    std::vector<u32> piece_indices;
    u32 failing_resource{0xFFFFFFFFU};
};

class ListPort final : public openswd3::battle::LegacyBattleListFramePort {
public:
    [[nodiscard]] openswd3::battle::LegacyBattleListFrameCallReply
    invoke_list_frame(
        const openswd3::battle::LegacyBattleListFrameCallRequest& request
    ) override {
        calls.push_back(request);
        const std::size_t index = calls.size() - 1U;
        return index < replies.size()
            ? replies[index]
            : openswd3::battle::LegacyBattleListFrameCallReply{};
    }

    std::vector<openswd3::battle::LegacyBattleListFrameCallRequest> calls;
    std::vector<openswd3::battle::LegacyBattleListFrameCallReply> replies;
};

struct Fixture {
    Fixture() : action_updater(action_streams), raster(framebuffer.geometry()) {
        port.battle_offset_action_frame_draw_state().result_latch = 1U;
    }

    [[nodiscard]] openswd3::battle::LegacyBattleListFrameBindings bindings() {
        return {
            .input = input,
            .panel_action_record = panel_action_record,
            .framebuffer = framebuffer,
            .clip = clip,
            .raster = raster,
            .shared_request = request,
            .shared_effects = effects,
            .jitter = jitter,
            .action_updater = action_updater,
            .frame_provider = frame_provider,
        };
    }

    openswd3::battle::LegacyBattleInputDispatchState input;
    openswd3::asset_runtime::LegacyActionRecord panel_action_record;
    openswd3::rendering::LegacyFramebuffer framebuffer{{
        .pitch_bytes = 1280,
        .width = 640,
        .height = 480,
    }};
    openswd3::rendering::LegacyBlitClipRectangle clip{0, 0, 640, 480};
    ActionStreamProvider action_streams;
    openswd3::asset_runtime::LegacyActionUpdater action_updater;
    openswd3::rendering::LegacyRasterGeometryState raster;
    openswd3::rendering::LegacyBlitRequest request;
    openswd3::rendering::LegacyBlitEffectState effects;
    openswd3::rendering::LegacyRleRowJitterState jitter;
    FrameProvider frame_provider;
    ListPort port;
};

[[nodiscard]] openswd3::battle::LegacyBattleListFrameRequest request() {
    openswd3::battle::LegacyBattleListFrameRequest value{
        .origin_x = 224U,
        .origin_y = 126U,
        .entry_eax = 0x11111111U,
        .entry_ecx = 0x22222222U,
        .entry_edx = 0x33333333U,
        .rectangle_return_registers =
            {.eax = 0x44444444U, .ecx = 0xABCD1234U, .edx = 0x55555555U},
        .tiled_frame_return_registers = {
            .eax = 0x66666666U, .ecx = 0xDEAD1111U, .edx = 0xBEEF2222U
        },
    };
    for (std::size_t index = 0U;
         index < value.action_frame_return_registers.size();
         ++index) {
        value.action_update_edx_snapshots[index] = static_cast<u32>(index);
        value.action_frame_return_registers[index] = {
            .eax = 0xB0000000U + static_cast<u32>(index),
            .ecx = 0xC0000000U + static_cast<u32>(index),
            .edx = 0xD0000000U + static_cast<u32>(index),
        };
    }
    return value;
}

}  // namespace

void test_battle_list_frame(openswd3::test::Context& test) {
    {
        Fixture fixture;
        fixture.input.action_category_index = 1U;
        fixture.input.selection_animation_frame_b = 0xFFFFFFFFU;
        fixture.panel_action_record.action_id = 0xAAAAAAAAU;
        fixture.panel_action_record.mode_flags = 0xBBBBBBBBU;
        std::ranges::fill(
            fixture.framebuffer.physical_pixels(), static_cast<u16>(0xFFFFU)
        );
        fixture.port.replies = {
            {.eax = 0x10101010U, .ecx = 0x20202020U, .edx = 0x30303030U},
            {.eax = 0x40404040U, .ecx = 0x50505050U, .edx = 0x60606060U},
        };
        const auto result = openswd3::battle::draw_legacy_battle_list_frame(
            fixture.bindings(), fixture.port, request()
        );
        const std::array<u32, 5> expected_actions{
            0x2394U, 0x2394U, 0x2394U, 0x2394U, 0x233BU
        };
        const std::array<u32, 5> expected_variants{2U, 1U, 0U, 5U, 0U};
        test.expect_true(
            result.status ==
                openswd3::battle::LegacyBattleListFrameStatus::completed,
            "list frame completes all action, rectangle, and tiled stages"
        );
        test.expect_true(
            std::ranges::all_of(
                result.action_frames,
                [](const auto& frame) {
                    return frame.status ==
                        openswd3::battle::
                            LegacyBattleOffsetActionFrameDrawStatus::completed;
                }
            ),
            "all four offset action frames complete"
        );
        test.expect_true(
            result.action_frame_calls == 4U && result.font_style_calls == 2U &&
                result.port_calls == 2U &&
                result.panel_action_update_calls == 1U &&
                result.rectangle_calls == 1U &&
                result.tiled_frame_calls == 1U &&
                fixture.action_streams.action_ids ==
                    std::vector<u32>(
                        expected_actions.begin(), expected_actions.end()
                    ) &&
                fixture.action_streams.variants ==
                    std::vector<u32>(
                        expected_variants.begin(), expected_variants.end()
                    ) &&
                result.action_frames[0U].draw_x == 306 &&
                result.action_frames[1U].draw_x == 264 &&
                result.action_frames[2U].draw_x == 222 &&
                result.action_frames[3U].draw_x == 264 &&
                result.action_frames[3U].draw_y == 155 &&
                fixture.port.calls.size() == 2U &&
                fixture.port.calls[0U].arguments[0U] == 0xF000U &&
                fixture.port.calls[0U].eax == 0xB0000003U &&
                fixture.port.calls[0U].ecx == 0x004C9A28U &&
                fixture.port.calls[0U].edx == 0xD0000003U &&
                fixture.port.calls[1U].arguments[0U] == 0xFFFEU &&
                fixture.port.calls[1U].eax == 0x10101010U &&
                fixture.panel_action_record.action_id == 0x233BU &&
                fixture.panel_action_record.base_variant == 0U &&
                fixture.panel_action_record.field_4a == 0x0077U &&
                fixture.input.selection_animation_frame_a == 10U &&
                fixture.input.selection_animation_frame_b == 1U &&
                result.return_eax == 1U && result.return_ecx == 0xDEAD1111U &&
                result.return_edx == 0xBEEF2222U &&
                fixture.framebuffer.row_pixels(200U)[300U] != 0xFFFFU &&
                fixture.framebuffer.row_pixels(100U)[100U] == 0xFFFFU &&
                std::ranges::all_of(
                    fixture.frame_provider.resource_ids | std::views::drop(4),
                    [](const u32 resource) { return resource == 0xABCD0077U; }
                ),
            "list frame preserves draw order, shared records, stale resource high word, and signed frame wrap"
        );
    }

    {
        Fixture fixture;
        fixture.action_streams.failing_action_id = 0x233BU;
        const auto result = openswd3::battle::draw_legacy_battle_list_frame(
            fixture.bindings(), fixture.port, request()
        );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleListFrameStatus::completed &&
                result.panel_action_update.return_value == 0U &&
                result.rectangle_calls == 1U &&
                result.tiled_frame_calls == 1U &&
                fixture.panel_action_record.field_4a == 0U &&
                std::ranges::all_of(
                    fixture.frame_provider.resource_ids | std::views::drop(4),
                    [](const u32 resource) { return resource == 0xABCD0000U; }
                ) &&
                fixture.input.selection_animation_frame_b == 2U,
            "panel action update failure remains non-branching and feeds the cleared resource low word"
        );
    }

    {
        Fixture fixture;
        fixture.action_streams.failing_action_id = 0x2394U;
        fixture.input.selection_animation_frame_a = 3U;
        fixture.input.selection_animation_frame_b = 4U;
        fixture.panel_action_record.action_id = 0xAAAAAAAAU;
        auto call = request();
        call.action_frame_return_registers[0U] = {
            .eax = 0x11112222U,
            .ecx = 0x33334444U,
            .edx = 0x55556666U,
        };
        const auto result = openswd3::battle::draw_legacy_battle_list_frame(
            fixture.bindings(), fixture.port, call
        );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleListFrameStatus::
                        action_frame_typed_stop &&
                result.action_frame_calls == 1U &&
                result.action_frames[0U].status ==
                    openswd3::battle::LegacyBattleOffsetActionFrameDrawStatus::
                        action_update_failed &&
                result.font_style_calls == 0U &&
                result.panel_action_update_calls == 0U &&
                result.rectangle_calls == 0U &&
                result.tiled_frame_calls == 0U &&
                fixture.input.selection_animation_frame_a == 3U &&
                fixture.input.selection_animation_frame_b == 4U &&
                fixture.panel_action_record.action_id == 0xAAAAAAAAU &&
                result.return_eax == 0x11112222U &&
                result.return_ecx == 0x33334444U &&
                result.return_edx == 0x55556666U,
            "first action-frame stop preserves its prefix and blocks all panel side effects"
        );
    }

    {
        Fixture fixture;
        fixture.input.selection_animation_frame_b = 3U;
        fixture.raster.surface.pitch_bytes = 1;
        const auto result = openswd3::battle::draw_legacy_battle_list_frame(
            fixture.bindings(), fixture.port, request()
        );
        test.expect_true(
            result.status ==
                openswd3::battle::LegacyBattleListFrameStatus::
                    rectangle_typed_stop,
            "invalid rectangle geometry stops before tiled drawing"
        );
        test.expect_true(
            result.action_frame_calls == 4U && result.font_style_calls == 2U &&
                result.panel_action_update_calls == 1U &&
                result.rectangle_status ==
                    openswd3::rendering::LegacyRectangleEffectStatus::
                        invalid_geometry &&
                result.tiled_frame_calls == 0U &&
                fixture.input.selection_animation_frame_a == 10U &&
                fixture.input.selection_animation_frame_b == 3U &&
                result.return_eax == 0x44444444U &&
                result.return_ecx == 0xABCD1234U &&
                result.return_edx == 0x55555555U,
            "rectangle stop preserves action and record prefixes before frame advancement"
        );
    }

    {
        Fixture fixture;
        fixture.input.selection_animation_frame_b = 4U;
        fixture.frame_provider.failing_resource = 0xABCD0077U;
        const auto result = openswd3::battle::draw_legacy_battle_list_frame(
            fixture.bindings(), fixture.port, request()
        );
        test.expect_true(
            result.status ==
                openswd3::battle::LegacyBattleListFrameStatus::
                    tiled_frame_typed_stop,
            "missing tiled resource stops before animation advancement"
        );
        test.expect_true(
            result.action_frame_calls == 4U &&
                result.panel_action_update_calls == 1U &&
                result.rectangle_calls == 1U &&
                result.tiled_frame_calls == 1U &&
                fixture.input.selection_animation_frame_a == 10U &&
                fixture.input.selection_animation_frame_b == 4U &&
                result.return_eax == 0x66666666U &&
                result.return_ecx == 0xDEAD1111U &&
                result.return_edx == 0xBEEF2222U,
            "tiled-frame stop keeps the completed rectangle and blocks animation advancement"
        );
    }
}
