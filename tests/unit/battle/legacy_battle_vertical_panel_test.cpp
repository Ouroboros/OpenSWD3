#include "test.hpp"

#include "openswd3/battle/legacy_battle_vertical_panel.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <vector>

namespace {

using openswd3::compat::u8;
using openswd3::compat::u16;
using openswd3::compat::u32;

class BattlePanelActionStreamProvider final
    : public openswd3::asset_runtime::LegacyActionStreamProvider {
public:
    [[nodiscard]] openswd3::asset_runtime::LegacyActionStreamLoadResult
    load_action_stream(
        const u32 action_id, const u32 variant_index, const bool cached
    ) override {
        action_ids.push_back(action_id);
        variants.push_back(variant_index);
        cached_flags.push_back(cached);
        if (calls++ == failed_call || variant_index >= streams.size() ||
            streams[variant_index].empty()) {
            return {};
        }
        return {
            openswd3::asset_runtime::LegacyActionStreamStatus::ready,
            streams[variant_index],
            false,
        };
    }

    void
    set_phase(const u32 variant, const u16 resource_id, const u16 frame_index) {
        const std::array<u16, 8> words{
            0x5246U,
            resource_id,
            0x5041U,
            frame_index,
            0x5859U,
            0U,
            0U,
            0x4544U,
        };
        auto& bytes = streams[variant];
        bytes.clear();
        for (const u16 word : words) {
            bytes.push_back(static_cast<u8>(word));
            bytes.push_back(static_cast<u8>(word >> 8U));
        }
    }

    std::array<std::vector<u8>, 0x20> streams;
    std::vector<u32> action_ids;
    std::vector<u32> variants;
    std::vector<bool> cached_flags;
    u32 calls{};
    u32 failed_call{std::numeric_limits<u32>::max()};
};

class BattlePanelFrameProvider final
    : public openswd3::rendering::LegacyFramePieceProvider {
public:
    BattlePanelFrameProvider() {
        widths = {32U, 28U, 20U, 32U};
        heights = {2U, 3U, 2U, 4U};
        for (std::size_t phase = 0; phase < storage.size(); ++phase) {
            storage[phase].resize(
                static_cast<std::size_t>(widths[phase]) * heights[phase] * 2U
            );
            const u16 color = static_cast<u16>(0x4100U + phase);
            for (std::size_t offset = 0; offset < storage[phase].size();
                 offset += 2U) {
                storage[phase][offset] = static_cast<u8>(color);
                storage[phase][offset + 1U] = static_cast<u8>(color >> 8U);
            }
        }
    }

    [[nodiscard]] bool load_frame_piece(
        const u32 resource_id,
        const u32 frame_index,
        openswd3::rendering::LegacyFramePiece& piece
    ) noexcept override {
        const std::size_t phase = resource_ids.size();
        resource_ids.push_back(resource_id);
        frame_indices.push_back(frame_index);
        if (phase == failed_call || phase >= storage.size()) {
            piece = {};
            return false;
        }
        piece = {
            .source =
                {
                    .bytes = storage[phase],
                    .layout = layouts[phase],
                    .palette = palettes[phase],
                },
            .width = widths[phase],
            .height = heights[phase],
        };
        return true;
    }

    std::array<u16, 4> widths{};
    std::array<u16, 4> heights{};
    std::array<std::vector<u8>, 4> storage;
    std::array<openswd3::rendering::LegacyBlitSourceLayout, 4> layouts{
        openswd3::rendering::LegacyBlitSourceLayout::direct_16,
        openswd3::rendering::LegacyBlitSourceLayout::direct_16,
        openswd3::rendering::LegacyBlitSourceLayout::direct_16,
        openswd3::rendering::LegacyBlitSourceLayout::direct_16,
    };
    std::array<std::span<const u16>, 4> palettes{};
    std::vector<u32> resource_ids;
    std::vector<u32> frame_indices;
    std::size_t failed_call{std::numeric_limits<std::size_t>::max()};
};

void prime_streams(BattlePanelActionStreamProvider& provider) {
    provider.set_phase(0x18U, 0x0061U, 1U);
    provider.set_phase(0x19U, 0x0062U, 2U);
    provider.set_phase(0x1AU, 0x007AU, 0U);
    provider.set_phase(0x1BU, 0x0063U, 3U);
    provider.set_phase(0x1EU, 0x007EU, 0U);
    provider.set_phase(0x1FU, 0x006FU, 3U);
}

[[nodiscard]] constexpr std::
    array<openswd3::battle::LegacyBattleActionUpdateRegisterSnapshot, 4>
    registers() noexcept {
    return {
        openswd3::battle::LegacyBattleActionUpdateRegisterSnapshot{
            .eax = 0xA1A10000U,
            .ecx = 0xB2B20000U,
        },
        openswd3::battle::LegacyBattleActionUpdateRegisterSnapshot{
            .eax = 0xC3C30000U,
            .edx = 0xD4D40000U,
        },
        openswd3::battle::LegacyBattleActionUpdateRegisterSnapshot{
            .eax = 0xE5E50000U,
            .ecx = 0xF6F60000U,
        },
        openswd3::battle::LegacyBattleActionUpdateRegisterSnapshot{
            .eax = 0x17170000U,
            .ecx = 0x28280000U,
        },
    };
}

[[nodiscard]] constexpr openswd3::rendering::LegacySurfaceGeometry
panel_surface() noexcept {
    return {
        .pitch_bytes = 1280,
        .width = 640,
        .height = 480,
    };
}

}  // namespace

void test_battle_vertical_panel(openswd3::test::Context& test) {
    const auto update_registers = registers();

    {
        openswd3::rendering::LegacyFramebuffer framebuffer{panel_surface()};
        BattlePanelActionStreamProvider action_provider;
        prime_streams(action_provider);
        openswd3::asset_runtime::LegacyActionUpdater action_updater{
            action_provider
        };
        BattlePanelFrameProvider frame_provider;
        openswd3::battle::LegacyBattleVerticalPanelState state{
            .maximum_count = 10,
            .current_count = 2,
        };
        state.action_record.action_id = 0x232AU;
        state.action_record.cached_action_id = 0x232AU;
        state.action_record.base_variant = 0x1EU;
        state.action_record.cached_base_variant = 0x1EU;
        state.action_record.field_4a = 0x0060U;
        state.action_record.field_4c = 0U;
        state.action_record.wait_remaining = 1U;
        state.action_record.field_94 = 0xDEADBEEFU;
        openswd3::rendering::LegacyBlitRequest shared_request{
            .vertical_resample_enlarge_state = 1U,
        };
        openswd3::rendering::LegacyBlitEffectState shared_effects{
            .red_offset = 1,
            .green_offset = 2,
            .blue_offset = 3,
            .skip_every_third_row = true,
        };
        openswd3::rendering::LegacyRleRowJitterState jitter;
        const auto result = openswd3::battle::draw_legacy_battle_vertical_panel(
            state,
            framebuffer,
            shared_request,
            shared_effects,
            jitter,
            action_updater,
            frame_provider,
            0xABCD232AU,
            10,
            20,
            4,
            1,
            1U,
            update_registers,
            0xDEADBEEFU
        );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleVerticalPanelStatus::
                        completed &&
                result.action_update_calls == 4U &&
                result.frame_load_calls == 4U &&
                result.frame_draw_calls == 10U &&
                result.phase_draw_calls == std::array<u32, 4>{1U, 4U, 4U, 1U} &&
                result.middle_draw_count == 4U &&
                result.fill_draw_count == 4U &&
                result.repeated_middle_height == 12 &&
                result.scaled_fill_height == 8 && result.ratio_quotient == 1 &&
                result.bottom_draw_y == 34 && result.clip_set_calls == 2U &&
                result.return_value == 0xDEADBEEFU &&
                state.base_variants ==
                    std::array<u32, 4>{0x1EU, 0x18U, 0x19U, 0x1BU} &&
                state.frame_resource_ids ==
                    std::array<u32, 4>{
                        0xB2B20060U,
                        0xC3C30061U,
                        0xF6F60062U,
                        0x28280063U,
                    } &&
                state.frame_indices ==
                    std::array<u32, 4>{
                        0xA1A10000U,
                        0xD4D40001U,
                        0xE5E50002U,
                        0x17170003U,
                    } &&
                state.ratio_quotient == 1 && state.panel_content_top == 22 &&
                state.fill_start == 25 && state.fill_clip_bottom == 33 &&
                state.panel_content_bottom == 34 &&
                state.shared_clip.left == 0 && state.shared_clip.top == 0 &&
                state.shared_clip.width == 640 &&
                state.shared_clip.height == 480 &&
                state.action_record.action_id == 0x232AU &&
                state.action_record.base_variant == 0x1BU &&
                state.action_record.field_94 == 0U &&
                action_provider.action_ids ==
                    std::vector<u32>{0x232AU, 0x232AU, 0x232AU, 0x232AU} &&
                action_provider.variants ==
                    std::vector<u32>{0x1EU, 0x18U, 0x19U, 0x1BU} &&
                frame_provider.resource_ids ==
                    std::vector<u32>{
                        0xB2B20060U,
                        0xC3C30061U,
                        0xF6F60062U,
                        0x28280063U,
                    } &&
                framebuffer.row_pixels(20U)[10U] == 0x4100U &&
                framebuffer.row_pixels(25U)[12U] == 0x4101U &&
                framebuffer.row_pixels(25U)[15U] == 0x4102U &&
                framebuffer.row_pixels(34U)[10U] == 0x4103U &&
                shared_request.target_height == 0 &&
                shared_request.horizontal_resample_displacement == 0 &&
                shared_request.vertical_resample_enlarge_state == 1U &&
                shared_request.vertical_resample_phase_10_10 == 0U &&
                shared_request.opacity_step == 0 &&
                shared_effects.red_offset == 0 &&
                shared_effects.green_offset == 0 &&
                shared_effects.blue_offset == 0 &&
                !shared_effects.skip_every_third_row,
            "vertical panel preserves stale registers and draws all phases"
        );
    }

    {
        openswd3::rendering::LegacyFramebuffer framebuffer{panel_surface()};
        BattlePanelActionStreamProvider action_provider;
        prime_streams(action_provider);
        openswd3::asset_runtime::LegacyActionUpdater action_updater{
            action_provider
        };
        BattlePanelFrameProvider frame_provider;
        openswd3::battle::LegacyBattleVerticalPanelState state{
            .maximum_count = 10,
            .current_count = 3,
        };
        openswd3::rendering::LegacyBlitRequest shared_request;
        openswd3::rendering::LegacyBlitEffectState shared_effects;
        openswd3::rendering::LegacyRleRowJitterState jitter;
        const auto result = openswd3::battle::draw_legacy_battle_vertical_panel(
            state,
            framebuffer,
            shared_request,
            shared_effects,
            jitter,
            action_updater,
            frame_provider,
            0x232AU,
            10,
            20,
            1,
            0,
            2U,
            update_registers,
            1U
        );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleVerticalPanelStatus::
                        completed &&
                state.base_variants ==
                    std::array<u32, 4>{0x1AU, 0x18U, 0x19U, 0x1FU} &&
                action_provider.variants ==
                    std::vector<u32>{0x1AU, 0x18U, 0x19U, 0x1FU} &&
                result.middle_draw_count == 1U &&
                result.repeated_middle_height == 3 &&
                result.scaled_fill_height == 2 && result.ratio_quotient == 0 &&
                state.panel_content_top == 22 &&
                state.panel_content_bottom == 25 &&
                state.fill_clip_bottom == 25 && result.fill_draw_count == 1U &&
                result.bottom_draw_y == 25 && result.return_value == 1U,
            "selector two chooses alternate bottom and full threshold clip"
        );
    }

    {
        openswd3::rendering::LegacyFramebuffer framebuffer{panel_surface()};
        BattlePanelActionStreamProvider action_provider;
        prime_streams(action_provider);
        openswd3::asset_runtime::LegacyActionUpdater action_updater{
            action_provider
        };
        BattlePanelFrameProvider frame_provider;
        openswd3::battle::LegacyBattleVerticalPanelState state{
            .maximum_count = 7,
        };
        openswd3::rendering::LegacyBlitRequest shared_request;
        openswd3::rendering::LegacyBlitEffectState shared_effects;
        openswd3::rendering::LegacyRleRowJitterState jitter;
        const auto result = openswd3::battle::draw_legacy_battle_vertical_panel(
            state,
            framebuffer,
            shared_request,
            shared_effects,
            jitter,
            action_updater,
            frame_provider,
            0x232AU,
            10,
            20,
            1,
            0,
            0U,
            update_registers,
            0U
        );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleVerticalPanelStatus::
                        ratio_divide_by_zero &&
                result.stopped_phase == 2U &&
                result.action_update_calls == 3U &&
                result.frame_load_calls == 3U &&
                result.frame_draw_calls == 2U &&
                result.scaled_fill_height == 3 && result.clip_set_calls == 0U &&
                state.source_published && state.frame_available[2] &&
                state.shared_clip.width == 640 &&
                action_provider.variants ==
                    std::vector<u32>{0x1AU, 0x18U, 0x19U},
            "ratio divisor zero stops after fill frame publication"
        );
    }

    {
        openswd3::rendering::LegacyFramebuffer framebuffer{panel_surface()};
        BattlePanelActionStreamProvider action_provider;
        prime_streams(action_provider);
        action_provider.failed_call = 1U;
        openswd3::asset_runtime::LegacyActionUpdater action_updater{
            action_provider
        };
        BattlePanelFrameProvider frame_provider;
        openswd3::battle::LegacyBattleVerticalPanelState state{
            .maximum_count = 10,
        };
        openswd3::rendering::LegacyBlitRequest shared_request;
        openswd3::rendering::LegacyBlitEffectState shared_effects;
        openswd3::rendering::LegacyRleRowJitterState jitter;
        const auto result = openswd3::battle::draw_legacy_battle_vertical_panel(
            state,
            framebuffer,
            shared_request,
            shared_effects,
            jitter,
            action_updater,
            frame_provider,
            0x232AU,
            10,
            20,
            1,
            0,
            0U,
            update_registers,
            0U
        );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleVerticalPanelStatus::
                        action_update_failed &&
                result.stopped_phase == 1U &&
                result.action_update_calls == 2U &&
                result.frame_load_calls == 1U &&
                result.frame_draw_calls == 1U &&
                state.action_record.action_id == 0x232AU &&
                state.action_record.base_variant == 0x18U &&
                !state.frame_available[1] && state.source_published &&
                action_provider.variants == std::vector<u32>{0x1AU, 0x18U},
            "second update failure preserves cleared middle record prefix"
        );
    }

    {
        openswd3::rendering::LegacyFramebuffer framebuffer{panel_surface()};
        BattlePanelActionStreamProvider action_provider;
        prime_streams(action_provider);
        openswd3::asset_runtime::LegacyActionUpdater action_updater{
            action_provider
        };
        BattlePanelFrameProvider frame_provider;
        frame_provider.failed_call = 2U;
        openswd3::battle::LegacyBattleVerticalPanelState state{
            .maximum_count = 10,
        };
        openswd3::rendering::LegacyBlitRequest shared_request;
        openswd3::rendering::LegacyBlitEffectState shared_effects;
        openswd3::rendering::LegacyRleRowJitterState jitter;
        const auto result = openswd3::battle::draw_legacy_battle_vertical_panel(
            state,
            framebuffer,
            shared_request,
            shared_effects,
            jitter,
            action_updater,
            frame_provider,
            0x232AU,
            10,
            20,
            1,
            0,
            0U,
            update_registers,
            0U
        );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleVerticalPanelStatus::
                        frame_unavailable &&
                result.stopped_phase == 2U &&
                result.action_update_calls == 3U &&
                result.frame_load_calls == 3U &&
                result.frame_draw_calls == 2U && !state.frame_available[2] &&
                state.source_published &&
                frame_provider.resource_ids.size() == 3U,
            "fill frame failure retains prior source and stops before ratio"
        );
    }

    {
        openswd3::rendering::LegacyFramebuffer framebuffer{panel_surface()};
        BattlePanelActionStreamProvider action_provider;
        prime_streams(action_provider);
        openswd3::asset_runtime::LegacyActionUpdater action_updater{
            action_provider
        };
        BattlePanelFrameProvider frame_provider;
        frame_provider.layouts[0] =
            openswd3::rendering::LegacyBlitSourceLayout::indexed_8;
        frame_provider.storage[0].assign(
            static_cast<std::size_t>(frame_provider.widths[0]) *
                frame_provider.heights[0],
            2U
        );
        openswd3::battle::LegacyBattleVerticalPanelState state{
            .maximum_count = 10,
        };
        openswd3::rendering::LegacyBlitRequest shared_request{
            .target_height = 9,
        };
        openswd3::rendering::LegacyBlitEffectState shared_effects{
            .blue_offset = 6,
        };
        openswd3::rendering::LegacyRleRowJitterState jitter;
        const auto result = openswd3::battle::draw_legacy_battle_vertical_panel(
            state,
            framebuffer,
            shared_request,
            shared_effects,
            jitter,
            action_updater,
            frame_provider,
            0x232AU,
            10,
            20,
            1,
            0,
            0U,
            update_registers,
            0U
        );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleVerticalPanelStatus::
                        blit_typed_stop &&
                result.stopped_phase == 0U &&
                result.last_blit_status ==
                    openswd3::rendering::LegacyBlitExecutionStatus::
                        palette_out_of_bounds &&
                result.action_update_calls == 1U &&
                result.frame_load_calls == 1U &&
                result.frame_draw_calls == 1U &&
                state.action_record.base_variant == 0x1AU &&
                state.source_published && shared_request.target_height == 9 &&
                shared_effects.blue_offset == 6,
            "indexed top frame keeps fixed empty tail before first clear"
        );
    }
}
