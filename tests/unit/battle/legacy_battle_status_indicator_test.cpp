#include "test.hpp"

#include "openswd3/battle/legacy_battle_status_indicator.hpp"

#include <array>
#include <cstdint>
#include <span>
#include <vector>

namespace {

using openswd3::compat::u8;
using openswd3::compat::u16;
using openswd3::compat::u32;

class IndicatorActionStreamProvider final
    : public openswd3::asset_runtime::LegacyActionStreamProvider {
public:
    IndicatorActionStreamProvider() {
        const std::array<u16, 5> words{0x5246U, 0x0066U, 0x5041U, 0U, 0x4544U};
        for (const u16 word : words) {
            bytes.push_back(static_cast<u8>(word));
            bytes.push_back(static_cast<u8>(word >> 8U));
        }
    }

    [[nodiscard]] openswd3::asset_runtime::LegacyActionStreamLoadResult
    load_action_stream(
        const u32 action_id, const u32 variant_index, const bool cached
    ) override {
        ++calls;
        action_ids.push_back(action_id);
        variants.push_back(variant_index);
        cached_flags.push_back(cached);
        if (fail) {
            return {};
        }
        return {
            openswd3::asset_runtime::LegacyActionStreamStatus::ready,
            bytes,
            false,
        };
    }

    std::vector<u8> bytes;
    std::vector<u32> action_ids;
    std::vector<u32> variants;
    std::vector<bool> cached_flags;
    u32 calls{};
    bool fail{};
};

class IndicatorFrameProvider final
    : public openswd3::rendering::LegacyFramePieceProvider {
public:
    [[nodiscard]] bool load_frame_piece(
        const u32 resource_id,
        const u32 frame_index,
        openswd3::rendering::LegacyFramePiece& piece
    ) noexcept override {
        ++calls;
        resource_ids.push_back(resource_id);
        frame_indices.push_back(frame_index);
        if (fail) {
            piece = {};
            return false;
        }
        piece = {
            .source =
                {
                    .bytes = bytes,
                    .layout = layout,
                    .palette = palette,
                },
            .width = 1U,
            .height = 1U,
        };
        return true;
    }

    std::array<u8, 2> bytes{0x34U, 0x12U};
    std::span<const u16> palette{};
    openswd3::rendering::LegacyBlitSourceLayout layout{
        openswd3::rendering::LegacyBlitSourceLayout::direct_16
    };
    std::vector<u32> resource_ids;
    std::vector<u32> frame_indices;
    u32 calls{};
    bool fail{};
};

class IndicatorRandomPort final
    : public openswd3::battle::LegacyBattleBoundedRandomPort {
public:
    [[nodiscard]] u32 random_bounded(const u32 bound) override {
        ++calls;
        bounds.push_back(bound);
        return value;
    }

    u32 value{};
    u32 calls{};
    std::vector<u32> bounds;
};

class IndicatorSoundPort final
    : public openswd3::battle::LegacyBattleIndicatorSoundPort {
public:
    void play_indicator_sound(const u16 sound_id, const u16 level) override {
        ++calls;
        ids.push_back(sound_id);
        levels.push_back(level);
    }

    u32 calls{};
    std::vector<u16> ids;
    std::vector<u16> levels;
};

struct IndicatorFixture {
    openswd3::rendering::LegacyFramebuffer framebuffer{{
        .pitch_bytes = 1280,
        .width = 640,
        .height = 480,
    }};
    openswd3::rendering::LegacyBlitClipRectangle clip{
        .left = 0,
        .top = 0,
        .width = 640,
        .height = 480,
    };
    IndicatorActionStreamProvider action_provider;
    openswd3::asset_runtime::LegacyActionUpdater action_updater{
        action_provider
    };
    IndicatorFrameProvider frame_provider;
    IndicatorRandomPort random;
    IndicatorSoundPort sound;
    openswd3::rendering::LegacyBlitRequest request;
    openswd3::rendering::LegacyBlitEffectState effects;
    openswd3::rendering::LegacyRleRowJitterState jitter;
    openswd3::battle::LegacyBattleStatusIndicatorState state;

    [[nodiscard]] openswd3::battle::LegacyBattleStatusIndicatorResult
    step(const u32 eax_snapshot = 0xABCD0001U) {
        return openswd3::battle::advance_legacy_battle_status_indicator(
            state,
            framebuffer,
            clip,
            request,
            effects,
            jitter,
            action_updater,
            frame_provider,
            random,
            sound,
            eax_snapshot
        );
    }
};

}  // namespace

void test_battle_status_indicator(openswd3::test::Context& test) {
    {
        IndicatorFixture fixture;
        fixture.random.value = 0U;
        fixture.state.intensity = 5U;
        fixture.state.intensity_countdown = 2U;
        fixture.state.action_record.action_id = 0x2329U;
        fixture.state.action_record.cached_action_id = 0x2329U;
        fixture.state.action_record.base_variant = 2U;
        fixture.state.action_record.cached_base_variant = 2U;
        fixture.state.action_record.field_4a = 0x0077U;
        fixture.state.action_record.wait_remaining = 1U;
        fixture.state.action_record.field_94 = 0xDEADBEEFU;
        const auto result = fixture.step();
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleStatusIndicatorStatus::
                        completed &&
                result.random_calls == 1U && result.action_update_calls == 1U &&
                result.frame_load_calls == 1U &&
                result.frame_draw_calls == 1U && result.sound_calls == 0U &&
                result.draw_x == 260 && result.draw_y == 200 &&
                result.return_value == 0U && !result.tick_multiple_of_25 &&
                !result.state_toggled && !result.action_record_cleared &&
                fixture.random.bounds == std::vector<u32>{2U} &&
                fixture.action_provider.variants == std::vector<u32>{2U} &&
                fixture.frame_provider.resource_ids ==
                    std::vector<u32>{0xABCD0077U} &&
                fixture.frame_provider.frame_indices == std::vector<u32>{0U} &&
                fixture.state.tick_counter == 1U &&
                fixture.state.side_state == 0U &&
                fixture.state.intensity == 5U &&
                fixture.state.intensity_countdown == 1U &&
                fixture.state.action_record.field_94 == 0xDEADBEEFU &&
                fixture.framebuffer.row_pixels(200U)[260U] == 0x1234U,
            "idle random zero draws left indicator without clearing stale record"
        );
    }

    {
        IndicatorFixture fixture;
        fixture.random.value = 1U;
        fixture.state.intensity = 5U;
        fixture.state.intensity_countdown = 1U;
        const auto result = fixture.step();
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleStatusIndicatorStatus::
                        completed &&
                result.draw_x == 310 && result.state_toggled &&
                result.sound_calls == 1U && result.return_value == 0U &&
                fixture.action_provider.variants == std::vector<u32>{3U} &&
                fixture.state.side_state == 0U &&
                fixture.state.intensity_countdown == 5U &&
                fixture.sound.ids == std::vector<u16>{0x2EU} &&
                fixture.sound.levels == std::vector<u16>{1U} &&
                fixture.framebuffer.row_pixels(200U)[310U] == 0x1234U,
            "countdown zero reloads intensity toggles side and plays fixed sound"
        );
    }

    {
        IndicatorFixture fixture;
        fixture.state.side_state = 1U;
        fixture.state.completed_hold_count = 3U;
        fixture.state.intensity = 1U;
        fixture.state.intensity_countdown = 2U;
        const auto result = fixture.step();
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleStatusIndicatorStatus::
                        completed &&
                result.random_calls == 0U && fixture.random.calls == 0U &&
                fixture.state.side_state == 0U &&
                fixture.state.completed_hold_count == 0U &&
                fixture.action_provider.variants == std::vector<u32>{2U} &&
                result.draw_x == 260,
            "idle completed hold clears both state words instead of consuming random"
        );
    }

    {
        IndicatorFixture fixture;
        fixture.state.tick_counter = 24U;
        fixture.state.side_state = 1U;
        fixture.state.intensity = 32U;
        fixture.state.intensity_countdown = 9U;
        fixture.state.action_record.action_id = 0x2329U;
        fixture.state.action_record.cached_action_id = 0x2329U;
        fixture.state.action_record.base_variant = 3U;
        fixture.state.action_record.cached_base_variant = 3U;
        fixture.state.action_record.field_4a = 0x0077U;
        fixture.state.action_record.wait_remaining = 1U;
        fixture.state.action_record.field_94 = 0xCAFEBABEU;
        const auto result = fixture.step(0x13570001U);
        const auto* record_bytes =
            reinterpret_cast<const u8*>(&fixture.state.action_record);
        bool all_zero = true;
        for (std::size_t index = 0;
             index < openswd3::asset_runtime::kLegacyActionRecordSize;
             ++index) {
            all_zero = all_zero && record_bytes[index] == 0U;
        }
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleStatusIndicatorStatus::
                        completed &&
                result.tick_multiple_of_25 && result.action_record_cleared &&
                result.return_value == 1U && result.sound_calls == 0U &&
                fixture.state.tick_counter == 0U &&
                fixture.state.intensity == 1U &&
                fixture.state.intensity_countdown == 1U &&
                fixture.state.side_state == 1U &&
                fixture.state.completed_hold_count == 1U && all_zero &&
                fixture.frame_provider.resource_ids ==
                    std::vector<u32>{0x13570077U},
            "twenty-fifth tick reaches threshold resets brightness and clears record"
        );
    }

    {
        IndicatorFixture fixture;
        fixture.state.tick_counter = 24U;
        fixture.state.intensity = 31U;
        fixture.state.intensity_countdown = 0x9999U;
        const auto result = fixture.step();
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleStatusIndicatorStatus::
                        completed &&
                result.tick_multiple_of_25 && result.return_value == 0U &&
                !result.state_toggled && !result.action_record_cleared &&
                fixture.state.tick_counter == 25U &&
                fixture.state.intensity == 62U &&
                fixture.state.intensity_countdown == 61U,
            "subthreshold twenty-fifth tick copies doubled low word then decrements high"
        );
    }

    {
        IndicatorFixture fixture;
        fixture.random.value = 1U;
        fixture.action_provider.fail = true;
        fixture.state.intensity = 9U;
        fixture.state.intensity_countdown = 8U;
        const auto result = fixture.step();
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleStatusIndicatorStatus::
                        action_update_failed &&
                result.random_calls == 1U && result.action_update_calls == 1U &&
                result.frame_load_calls == 0U &&
                fixture.state.side_state == 1U &&
                fixture.state.action_record.action_id == 0x2329U &&
                fixture.state.action_record.base_variant == 3U &&
                fixture.state.tick_counter == 0U &&
                fixture.state.intensity == 9U &&
                fixture.state.intensity_countdown == 8U,
            "update failure preserves random and action prefix without advancing tick"
        );
    }

    {
        IndicatorFixture fixture;
        fixture.state.tick_counter = 5U;
        fixture.state.side_state = 1U;
        fixture.frame_provider.fail = true;
        const auto result = fixture.step();
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleStatusIndicatorStatus::
                        frame_unavailable &&
                result.random_calls == 0U && result.frame_load_calls == 1U &&
                result.frame_draw_calls == 0U &&
                fixture.state.tick_counter == 5U &&
                !fixture.state.source_published,
            "frame failure stops before source publication draw and tick advance"
        );
    }

    {
        IndicatorFixture fixture;
        fixture.state.tick_counter = 5U;
        fixture.state.intensity = 2U;
        fixture.state.intensity_countdown = 2U;
        fixture.frame_provider.layout =
            openswd3::rendering::LegacyBlitSourceLayout::indexed_8;
        fixture.frame_provider.bytes = {2U, 0U};
        fixture.request.target_height = 1;
        fixture.effects.blue_offset = 7;
        const auto result = fixture.step();
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleStatusIndicatorStatus::
                        blit_typed_stop &&
                result.blit_status ==
                    openswd3::rendering::LegacyBlitExecutionStatus::
                        palette_out_of_bounds &&
                result.frame_draw_calls == 1U &&
                fixture.state.tick_counter == 5U &&
                fixture.state.intensity_countdown == 2U &&
                fixture.request.target_height == 1 &&
                fixture.effects.blue_offset == 7 && fixture.sound.calls == 0U,
            "indexed frame fixed empty tail stops before common suffix and counters"
        );
    }
}
