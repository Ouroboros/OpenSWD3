#include "openswd3/battle/legacy_battle_frame_effect.hpp"
#include "openswd3/rendering/legacy_image_command_stream.hpp"
#include "test.hpp"

#include <array>
#include <bit>
#include <cstring>
#include <functional>
#include <limits>
#include <vector>

namespace {

using openswd3::battle::LegacyBattleActionRotationUpdateSnapshot;
using openswd3::battle::LegacyBattleFrameEffectPort;
using openswd3::battle::LegacyBattleFrameEffectSource;
using openswd3::battle::LegacyBattleFrameEffectSurfaceReply;
using openswd3::battle::LegacyBattleFrameEffectSurfaceRequest;
using openswd3::compat::u8;
using openswd3::compat::u16;
using openswd3::compat::u32;

class EffectPort final : public LegacyBattleFrameEffectPort {
public:
    [[nodiscard]] LegacyBattleActionRotationUpdateSnapshot
    update_action(openswd3::asset_runtime::LegacyActionRecord&) override {
        ++action_updates;
        return {
            .eax = action_eax,
            .edx = action_edx,
            .domain_token = action_domain,
            .typed_stop = action_typed_stop,
        };
    }

    [[nodiscard]] LegacyBattleFrameEffectSurfaceReply surface_operation(
        const LegacyBattleFrameEffectSurfaceRequest& request
    ) override {
        surface_requests.push_back(request);
        if (on_surface_operation) {
            on_surface_operation();
        }

        return {
            .return_value = surface_return,
            .callee_returned = surface_returned,
        };
    }

    u32 action_updates{};
    u32 action_eax{};
    u32 action_edx{};
    std::uint64_t action_domain{1U};
    bool action_typed_stop{};
    u32 surface_return{0xABCDEF01U};
    bool surface_returned{true};
    std::function<void()> on_surface_operation;
    std::vector<LegacyBattleFrameEffectSurfaceRequest> surface_requests;
};

struct Fixture {
    openswd3::rendering::LegacyFramebuffer framebuffer;
    openswd3::rendering::LegacyRasterGeometryState raster{};
    openswd3::rendering::LegacyBlitRequest request{};
    openswd3::rendering::LegacyBlitEffectState effects{};
    openswd3::rendering::LegacyRleRowJitterState jitter{};
    std::vector<u8> source_bytes;
    openswd3::compat::i32 pending_rotation{};
    openswd3::battle::LegacyBattleScreenFlashState flash{};
    openswd3::battle::LegacyBattleFrameRefreshState refresh{};

    Fixture() {
        static_cast<void>(
            openswd3::rendering::initialize_legacy_raster_geometry(
                raster, framebuffer.geometry().surface
            )
        );
        std::array<u16, 6> pixels{
            0x001FU,
            0x03E0U,
            0x7C00U,
            0x4210U,
            0x1234U,
            0x2AAAU,
        };
        const std::span<const u8> raw{
            reinterpret_cast<const u8*>(pixels.data()),
            pixels.size() * sizeof(u16),
        };
        source_bytes = openswd3::rendering::encode_legacy_image_command_stream(
                           raw, 3U, 2U, 16U
        )
                           .bytes;
    }

    [[nodiscard]] openswd3::battle::LegacyBattleFrameEffectContext context() {
        return {
            .framebuffer = framebuffer,
            .raster = raster,
            .shared_request = request,
            .shared_effects = effects,
            .jitter = jitter,
            .pending_rotation = pending_rotation,
            .flash = flash,
            .refresh = refresh,
        };
    }

    [[nodiscard]] LegacyBattleFrameEffectSource source() {
        return {
            .token = 0xA100U,
            .bytes = source_bytes,
            .width = 3U,
            .height = 2U,
        };
    }
};

[[nodiscard]] std::vector<u16> decode_words(const std::vector<u8>& bytes) {
    const auto decoded =
        openswd3::rendering::decode_legacy_image_command_stream(bytes);
    std::vector<u16> words(decoded.bytes.size() / sizeof(u16));
    std::memcpy(words.data(), decoded.bytes.data(), decoded.bytes.size());
    return words;
}

}  // namespace

void test_battle_frame_effect(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleFrameEffectState;
    using openswd3::battle::LegacyBattleFrameEffectStatus;

    constexpr std::array<u32, 3> surfaces{0xB000U, 0xB100U, 0xB200U};

    for (const u32 active : {0U, 1U, 2U, 0xFFFFFFFFU}) {
        LegacyBattleFrameEffectState state;
        Fixture fixture;
        fixture.flash.active = active;
        EffectPort port;
        auto context = fixture.context();
        const auto result = openswd3::battle::update_legacy_battle_frame_effect(
            state, port, context, fixture.source(), surfaces, 0
        );
        test.expect_true(
            result.status == LegacyBattleFrameEffectStatus::completed &&
                result.color_adjustment_calls == (active == 1U ? 3U : 0U) &&
                fixture.flash.active == active &&
                fixture.flash.intensity == (active == 1U ? 12U : 16U),
            "flash accepts exactly DWORD one and starts at the original static intensity"
        );
    }

    {
        LegacyBattleFrameEffectState state;
        Fixture fixture;
        fixture.flash.active = 1U;
        EffectPort port;
        auto context = fixture.context();
        constexpr std::array<u8, 4> remaining{12U, 8U, 4U, 16U};
        constexpr std::array<u16, 4> pixels{0x4210U, 0x318CU, 0x2108U, 0x1084U};
        for (std::size_t frame = 0; frame < remaining.size(); ++frame) {
            fixture.framebuffer.physical_pixels()[100U] = 0U;
            const auto result =
                openswd3::battle::update_legacy_battle_frame_effect(
                    state, port, context, fixture.source(), surfaces, 0
                );
            test.expect_true(
                result.status == LegacyBattleFrameEffectStatus::completed &&
                    result.color_adjustment_calls == 3U &&
                    fixture.flash.intensity == remaining[frame] &&
                    fixture.flash.active == (frame == 3U ? 0U : 1U) &&
                    fixture.framebuffer.physical_pixels()[100U] == pixels[frame],
                "four flash frames consume sixteen twelve eight four and reset only after the fourth pixel update"
            );
        }
    }

    {
        LegacyBattleFrameEffectState state;
        Fixture fixture;
        openswd3::rendering::LegacyFramebuffer small_framebuffer(
            {.pitch_bytes = 8, .width = 4, .height = 2}
        );
        static_cast<void>(
            openswd3::rendering::initialize_legacy_raster_geometry(
                fixture.raster, small_framebuffer.geometry().surface
            )
        );
        fixture.flash.active = 1U;
        fixture.flash.intensity = 8U;
        fixture.pending_rotation = 17;
        auto context = openswd3::battle::LegacyBattleFrameEffectContext{
            .framebuffer = small_framebuffer,
            .raster = fixture.raster,
            .shared_request = fixture.request,
            .shared_effects = fixture.effects,
            .jitter = fixture.jitter,
            .pending_rotation = fixture.pending_rotation,
            .flash = fixture.flash,
            .refresh = fixture.refresh,
        };
        EffectPort port;
        const auto result = openswd3::battle::update_legacy_battle_frame_effect(
            state, port, context, fixture.source(), surfaces, 0
        );
        test.expect_true(
            result.status ==
                    LegacyBattleFrameEffectStatus::color_adjustment_typed_stop &&
                result.color_adjustment_calls == 1U &&
                fixture.pending_rotation == 0 &&
                fixture.flash.active == 1U && fixture.flash.intensity == 8U,
            "color failure preserves the flash after the preceding rotation clear"
        );
    }

    constexpr std::array<std::array<openswd3::compat::i32, 4>, 4> byte_cases{{
        {0, 0, 252, 0x4210},
        {1, 1, 253, 0x4631},
        {128, -128, 124, 0},
        {252, -4, 248, 0x318C},
    }};
    for (const auto& item : byte_cases) {
        LegacyBattleFrameEffectState state;
        Fixture fixture;
        fixture.flash.active = 1U;
        fixture.flash.intensity = static_cast<u8>(item[0]);
        fixture.framebuffer.physical_pixels()[100U] = 0x4210U;
        EffectPort port;
        auto context = fixture.context();
        const auto result = openswd3::battle::update_legacy_battle_frame_effect(
            state, port, context, fixture.source(), surfaces, 0
        );
        test.expect_true(
            result.status == LegacyBattleFrameEffectStatus::completed &&
                result.applied_red_delta == item[1] &&
                result.applied_green_delta == item[1] &&
                result.applied_blue_delta == item[1] &&
                fixture.flash.active == 1U &&
                fixture.flash.intensity == static_cast<u8>(item[2]) &&
                fixture.framebuffer.physical_pixels()[100U] ==
                    static_cast<u16>(item[3]),
            "flash sign extends the full BYTE domain and wraps decay without normalizing its input"
        );
    }

    for (const u32 suppression : {1U, 2U}) {
        LegacyBattleFrameEffectState state;
        state.primary_suppression = suppression;
        Fixture fixture;
        fixture.flash.active = 1U;
        fixture.flash.intensity = 8U;
        EffectPort port;
        auto context = fixture.context();
        const auto result = openswd3::battle::update_legacy_battle_frame_effect(
            state, port, context, fixture.source(), surfaces, 0
        );
        test.expect_true(
            result.status == LegacyBattleFrameEffectStatus::completed &&
                result.color_adjustment_calls == 0U &&
                fixture.flash.active == 1U && fixture.flash.intensity == 8U,
            "suppressed background preserves the shared flash for a later frame"
        );
    }

    for (const u32 token : {0U, 1U, 0x80000000U, 0xFFFFFFFFU}) {
        LegacyBattleFrameEffectState state;
        state.primary_suppression = 1U;
        state.current_encounter_id = 1;
        state.expected_encounter_id = 2;
        state.fade_active = 1U;
        Fixture fixture;
        fixture.refresh.refresh_pending = 1U;
        fixture.refresh.active_surface_token = token;
        EffectPort port;
        port.on_surface_operation = [&] {
            fixture.refresh.refresh_pending = 0x8000U;
        };
        auto context = fixture.context();
        const auto result = openswd3::battle::update_legacy_battle_frame_effect(
            state, port, context, fixture.source(), surfaces, 0
        );
        const bool copied = token != 0xFFFFFFFFU;
        test.expect_true(
            result.status == LegacyBattleFrameEffectStatus::completed &&
                result.surface_operation_calls == (copied ? 1U : 0U) &&
                result.source_blit_calls == (copied ? 0U : 1U) &&
                fixture.refresh.refresh_pending == (copied ? 0x8000U : 0U) &&
                fixture.refresh.active_surface_token == token &&
                result.reset_calls == 0U &&
                (!copied || port.surface_requests.back().source_token ==
                                surfaces[0]),
            "fade compares the DWORD token with minus one and returns without overwriting a callee stage change"
        );
    }

    for (const bool fading : {false, true}) {
        for (const bool returned : {false, true}) {
            for (const u32 hresult : {0U, 1U, 0x80004005U}) {
                LegacyBattleFrameEffectState state;
                state.primary_suppression = 1U;
                state.current_encounter_id = 9;
                state.expected_encounter_id = fading ? 10 : 9;
                state.cadence = 2;
                state.fade_active = 1U;
                Fixture fixture;
                fixture.refresh.refresh_pending = fading ? 2U : 1U;
                fixture.refresh.active_surface_token = 7U;
                fixture.pending_rotation = 77;
                EffectPort port;
                port.surface_return = hresult;
                port.surface_returned = returned;
                port.on_surface_operation = [&] {
                    fixture.refresh.refresh_pending = 0U;
                    fixture.framebuffer.physical_pixels()[0] = 0x1234U;
                };
                auto context = fixture.context();
                const auto result =
                    openswd3::battle::update_legacy_battle_frame_effect(
                        state, port, context, fixture.source(), surfaces, 0
                    );

                test.expect_true(
                    result.status ==
                            (returned ? LegacyBattleFrameEffectStatus::completed
                                      : LegacyBattleFrameEffectStatus::
                                            staged_surface_typed_stop) &&
                        result.surface_operation.return_value == hresult &&
                        result.surface_operation.callee_returned == returned &&
                        result.surface_operation_calls ==
                            (!fading && returned ? 2U : 1U) &&
                        port.surface_requests.front().source_token ==
                            surfaces[1] &&
                        port.surface_requests.front().effect_flags ==
                            (fading ? 0U : 0x01000000U) &&
                        fixture.refresh.refresh_pending == 0U &&
                        fixture.pending_rotation == 77 &&
                        state.cadence == (!fading && returned ? 1 : 2) &&
                        result.cadence_updates ==
                            (!fading && returned ? 1U : 0U) &&
                        result.reset_calls == 0U &&
                        result.source_blit_calls == 0U &&
                        state.primary_suppression == 1U &&
                        state.fade_active == 1U &&
                        fixture.framebuffer.physical_pixels()[0] == 0x1234U,
                    "both surface calls ignore returned HRESULT but stop unfinished calls with pixel and state prefixes intact"
                );
            }
        }
    }

    // 453808 returns to 4538B2: reload the WORD before cadence and fade.
    struct SurfaceReturnVector {
        openswd3::compat::i16 returned_stage;
        openswd3::compat::i32 returned_cadence;
        u32 fade_active;
        openswd3::compat::i16 final_stage;
        openswd3::compat::i32 final_cadence;
        u32 resets;
        u32 surface_calls;
    };
    constexpr std::array<SurfaceReturnVector, 6> surface_returns{{
        {0, 2, 0U, 1, 1, 0U, 1U},
        {0, 0, 1U, 0, 1, 1U, 1U},
        {0x7FFF, 2, 0U, -32768, 1, 0U, 1U},
        {-1, 2, 0U, 0, 1, 0U, 1U},
        {3, 0, 1U, 2, 1, 0U, 2U},
        {4, 2, 0U, 2, 1, 0U, 1U},
    }};
    for (const auto& vector : surface_returns) {
        LegacyBattleFrameEffectState state;
        state.primary_suppression = 1U;
        state.current_encounter_id = 9;
        state.expected_encounter_id = 9;
        Fixture fixture;
        fixture.refresh.refresh_pending = 1U;
        fixture.refresh.active_surface_token = 7U;
        EffectPort port;
        port.on_surface_operation = [&] {
            if (port.surface_requests.size() == 1U) {
                fixture.refresh.refresh_pending =
                    std::bit_cast<u16>(vector.returned_stage);
                state.cadence = vector.returned_cadence;
                state.fade_active = vector.fade_active;
            }
        };
        auto context = fixture.context();
        const auto result = openswd3::battle::update_legacy_battle_frame_effect(
            state, port, context, fixture.source(), surfaces, 0
        );

        test.expect_true(
            result.status == LegacyBattleFrameEffectStatus::completed &&
                fixture.refresh.refresh_pending ==
                    std::bit_cast<u16>(vector.final_stage) &&
                state.cadence == vector.final_cadence &&
                result.reset_calls == vector.resets &&
                result.cadence_updates == 1U &&
                result.surface_operation_calls == vector.surface_calls &&
                port.surface_requests.front().source_token == surfaces[1] &&
                port.surface_requests.front().effect_flags == 0x01000000U &&
                (vector.surface_calls != 2U ||
                 (port.surface_requests.back().source_token == surfaces[2] &&
                  port.surface_requests.back().effect_flags == 0U)),
            "surface return reloads stage and cadence before wrapping growth or fade"
        );
    }

    for (const auto branch : {0U, 1U, 2U}) {
        LegacyBattleFrameEffectState state;
        state.rotation_cache.stored_action_id = 1U;
        state.split_extent = 10U;
        state.cadence = 2;
        state.current_encounter_id = 9;
        state.expected_encounter_id = 9;
        if (branch == 2U) {
            state.primary_suppression = 1U;
            state.alternate_surface_mode = 1U;
        }

        Fixture fixture;
        fixture.flash.active = 1U;
        fixture.refresh.refresh_pending = 1U;
        fixture.pending_rotation = 77;
        EffectPort port;
        port.action_typed_stop = true;
        auto context = fixture.context();
        const auto result = openswd3::battle::update_legacy_battle_frame_effect(
            state, port, context, fixture.source(), surfaces,
            branch == 0U ? 0 : 1
        );
        test.expect_true(
            result.status == (branch == 0U
                ? LegacyBattleFrameEffectStatus::rotation_frame_typed_stop
                : LegacyBattleFrameEffectStatus::rotation_playback_typed_stop) &&
                port.action_updates == 1U &&
                result.source_blit_calls == (branch == 2U ? 0U : 1U) &&
                result.color_adjustment_calls == 0U &&
                result.cadence_updates == 0U && result.reset_calls == 0U &&
                fixture.pending_rotation == 77 && state.split_extent == 10U &&
                fixture.flash.active == 1U &&
                fixture.refresh.refresh_pending == 1U &&
                state.cadence == 2,
            "effect caller preserves its prefix and stops before later effects"
        );
    }

    for (const u32 split_gate : {0U, 1U, 2U, 0xFFFFFFFFU}) {
        LegacyBattleFrameEffectState state;
        state.split_suppression = split_gate;
        state.split_extent = 10U;
        Fixture fixture;
        fixture.pending_rotation = 77;
        EffectPort port;
        auto context = fixture.context();

        const auto result = openswd3::battle::update_legacy_battle_frame_effect(
            state, port, context, fixture.source(), surfaces, 0
        );

        test.expect_true(
            result.status == LegacyBattleFrameEffectStatus::completed &&
                result.clip_calls == (split_gate == 1U ? 4U : 2U) &&
                result.source_blit_calls == (split_gate == 1U ? 3U : 1U) &&
                result.rotation_frame_calls == 1U &&
                state.published_source_token == 0xA100U &&
                state.split_extent == (split_gate == 1U ? 20U : 10U) &&
                fixture.pending_rotation == 0 &&
                fixture.framebuffer.physical_pixels()[0] == 0x001FU &&
                fixture.raster.clip_left == 0 && fixture.raster.clip_top == 0 &&
                fixture.raster.clip_width == 640 &&
                fixture.raster.clip_height == 480,
            "zero rotation draws split bands only for the exact enabled DWORD and restores full clip"
        );
    }

    for (const u32 split_gate : {0U, 1U, 2U, 0xFFFFFFFFU}) {
        LegacyBattleFrameEffectState state;
        state.split_suppression = split_gate;
        state.split_extent = 10U;
        Fixture fixture;
        const std::vector<u16> white(384U, 0x7FFFU);
        fixture.source_bytes =
            openswd3::rendering::encode_legacy_image_command_stream(
                {reinterpret_cast<const u8*>(white.data()),
                 white.size() * sizeof(u16)},
                1U,
                384U,
                16U
            )
                .bytes;
        auto source = fixture.source();
        source.width = 1U;
        source.height = 384U;
        EffectPort port;
        auto context = fixture.context();
        const auto result = openswd3::battle::update_legacy_battle_frame_effect(
            state, port, context, source, surfaces, 0
        );
        bool pixels_match = true;
        for (std::size_t row = 0U; row < 384U; ++row) {
            const bool in_band = split_gate == 1U && row >= 172U && row < 212U;
            // 00421A38..00421A93: (31 + 31 + 31) >> 2 = 23 in RGB555.
            const u16 expected = in_band ? 0x5EF7U : 0x7FFFU;
            pixels_match = pixels_match &&
                fixture.framebuffer.physical_pixels()[row * 640U] == expected;
        }

        test.expect_true(
            result.status == LegacyBattleFrameEffectStatus::completed &&
                pixels_match,
            "exact-one split gate grays only rows 172 through 211 across both clip boundaries"
        );
    }

    {
        bool extents_match = true;
        constexpr std::array<u16, 6> input_extents{
            0U, 19U, 20U, 191U, 192U, 0xFFFFU
        };
        constexpr std::array<u16, 6> expected_extents{
            0U, 38U, 42U, 213U, 192U, 0xFFFFU
        };
        for (std::size_t index = 0U; index < input_extents.size(); ++index) {
            LegacyBattleFrameEffectState state;
            state.split_suppression = 1U;
            state.split_extent = input_extents[index];
            Fixture fixture;
            EffectPort port;
            auto context = fixture.context();
            const auto result =
                openswd3::battle::update_legacy_battle_frame_effect(
                    state, port, context, fixture.source(), surfaces, 0
                );
            extents_match = extents_match &&
                result.status == LegacyBattleFrameEffectStatus::completed &&
                state.split_extent == expected_extents[index] &&
                result.clip_calls == 4U && result.source_blit_calls == 3U;
        }

        test.expect_true(
            extents_match,
            "enabled split bands preserve unsigned WORD boundaries and both clipped calls at zero"
        );
    }

    {
        LegacyBattleFrameEffectState state;
        state.split_suppression = 0U;
        Fixture fixture;
        fixture.flash.active = 1U;
        fixture.flash.intensity = 4U;
        EffectPort port;
        auto context = fixture.context();

        const auto result = openswd3::battle::update_legacy_battle_frame_effect(
            state, port, context, fixture.source(), surfaces, 0
        );

        test.expect_true(
            result.status == LegacyBattleFrameEffectStatus::completed &&
                result.source_blit_calls == 1U &&
                result.color_adjustment_calls == 3U &&
                result.applied_red_delta == 4 &&
                result.applied_green_delta == 4 &&
                result.applied_blue_delta == 4 &&
                state.published_red_delta == 4 &&
                state.published_green_delta == 4 &&
                state.published_blue_delta == 4 &&
                fixture.flash.intensity == 0x10U &&
                fixture.flash.active == 0U,
            "color cycle publishes one signed byte to three channels then wraps zero back to sixteen"
        );
    }

    {
        LegacyBattleFrameEffectState state;
        state.primary_suppression = 1U;
        state.current_encounter_id = 9;
        state.expected_encounter_id = 9;
        state.cadence = 2;
        Fixture fixture;
        fixture.refresh.refresh_pending = 1U;
        EffectPort port;
        auto context = fixture.context();

        const auto result = openswd3::battle::update_legacy_battle_frame_effect(
            state, port, context, fixture.source(), surfaces, 0
        );

        test.expect_true(
            result.status == LegacyBattleFrameEffectStatus::completed &&
                result.source_blit_calls == 0U &&
                result.surface_operation_calls == 1U &&
                port.surface_requests[0].source_token == surfaces[1] &&
                port.surface_requests[0].effect_flags == 0x01000000U &&
                fixture.refresh.refresh_pending == 2U && state.cadence == 1 &&
                result.cadence_updates == 1U,
            "suppressed matching encounter presents current staged surface and advances cadence with signed stage clamp"
        );
    }

    {
        LegacyBattleFrameEffectState state;
        state.secondary_suppression = 1U;
        state.current_encounter_id = 4;
        state.expected_encounter_id = 4;
        state.alternate_surface_mode = 1U;
        state.red_factor = 2;
        state.green_factor = 4;
        state.blue_factor = 6;
        Fixture fixture;
        fixture.refresh.refresh_pending = 2U;
        EffectPort port;
        auto context = fixture.context();

        const auto result = openswd3::battle::update_legacy_battle_frame_effect(
            state, port, context, fixture.source(), surfaces, 0
        );

        test.expect_true(
            result.status == LegacyBattleFrameEffectStatus::completed &&
                result.source_blit_calls == 1U &&
                result.color_adjustment_calls == 1U &&
                result.applied_red_delta == 2 &&
                result.applied_green_delta == 4 &&
                result.applied_blue_delta == 6 && state.cadence == 1,
            "alternate matching stage draws source and applies three signed half-factor products"
        );
    }

    {
        LegacyBattleFrameEffectState state;
        state.primary_suppression = 1U;
        state.current_encounter_id = 5;
        state.expected_encounter_id = 5;
        state.alternate_surface_mode = 1U;
        state.cadence = 2;
        Fixture fixture;
        fixture.refresh.refresh_pending = 0x7FFFU;
        EffectPort port;
        auto context = fixture.context();

        const auto result = openswd3::battle::update_legacy_battle_frame_effect(
            state, port, context, fixture.source(), surfaces, 0
        );

        test.expect_true(
            result.status == LegacyBattleFrameEffectStatus::completed &&
                fixture.refresh.refresh_pending == 0x8000U &&
                state.cadence == 1,
            "signed stage increment wraps maximum to minimum and bypasses greater than two clamp"
        );
    }

    {
        LegacyBattleFrameEffectState state;
        state.split_suppression = 1U;
        state.current_encounter_id = 1;
        state.expected_encounter_id = 2;
        state.fade_active = 1U;
        Fixture fixture;
        fixture.refresh.refresh_pending = 2U;
        fixture.refresh.active_surface_token = 7U;
        EffectPort port;
        auto context = fixture.context();

        const auto result = openswd3::battle::update_legacy_battle_frame_effect(
            state, port, context, fixture.source(), surfaces, 0
        );

        test.expect_true(
            result.status == LegacyBattleFrameEffectStatus::completed &&
                fixture.refresh.refresh_pending == 1U &&
                result.surface_operation_calls == 1U &&
                port.surface_requests[0].source_token == surfaces[1] &&
                port.surface_requests[0].effect_flags == 0U &&
                result.source_blit_calls == 3U,
            "active fade decrements stage then presents decremented surface and returns without fallback blit"
        );
    }

    {
        LegacyBattleFrameEffectState state;
        state.primary_suppression = 1U;
        state.secondary_suppression = 1U;
        state.alternate_surface_mode = 1U;
        state.red_factor = 8;
        state.green_factor = 10;
        state.blue_factor = 12;
        state.current_encounter_id = 3;
        state.expected_encounter_id = 4;
        state.fade_active = 1U;
        Fixture fixture;
        EffectPort port;
        auto context = fixture.context();

        const auto result = openswd3::battle::update_legacy_battle_frame_effect(
            state, port, context, fixture.source(), surfaces, 0
        );

        test.expect_true(
            result.status == LegacyBattleFrameEffectStatus::completed &&
                result.reset_calls == 1U && state.red_factor == 0 &&
                state.green_factor == 0 && state.blue_factor == 0 &&
                fixture.refresh.refresh_pending == 0U &&
                state.current_encounter_id == -1 &&
                state.primary_suppression == 0U &&
                state.secondary_suppression == 0U &&
                state.alternate_surface_mode == 0U && state.fade_active == 0U,
            "active fade at stage zero clears the exact terminal state slots"
        );
    }

    {
        LegacyBattleFrameEffectState state;
        state.primary_suppression = 1U;
        state.current_encounter_id = 6;
        state.expected_encounter_id = 6;
        Fixture fixture;
        fixture.refresh.refresh_pending = 3U;
        EffectPort port;
        auto context = fixture.context();

        const auto result = openswd3::battle::update_legacy_battle_frame_effect(
            state, port, context, fixture.source(), surfaces, 0
        );

        test.expect_true(
            result.status ==
                    LegacyBattleFrameEffectStatus::staged_surface_typed_stop &&
                result.clip_calls == 2U &&
                result.surface_operation_calls == 0U &&
                fixture.refresh.refresh_pending == 3U &&
                state.cadence == 0,
            "out of range staged surface stops at first table read after full clip restoration"
        );
    }

    {
        LegacyBattleFrameEffectState state;
        state.split_suppression = 1U;
        Fixture fixture;
        const auto before = decode_words(fixture.source_bytes);
        EffectPort port;
        auto context = fixture.context();

        const auto result = openswd3::battle::update_legacy_battle_frame_effect(
            state, port, context, fixture.source(), surfaces, 1
        );
        const auto after = decode_words(fixture.source_bytes);

        test.expect_true(
            result.status == LegacyBattleFrameEffectStatus::completed &&
                result.source_rotation_calls == 1U &&
                result.source_blit_calls == 1U &&
                result.rotation_playback_calls == 1U &&
                after.size() == before.size() && after[0] == before[2] &&
                after[1] == before[0] && after[2] == before[1] &&
                fixture.pending_rotation == 0,
            "positive amount rotates literal row right before draw and still invokes empty cached playback"
        );
    }

    {
        LegacyBattleFrameEffectState state;
        state.split_suppression = 0U;
        Fixture fixture;
        fixture.flash.active = 1U;
        fixture.flash.intensity = 0xFCU;
        EffectPort port;
        auto context = fixture.context();

        const auto result = openswd3::battle::update_legacy_battle_frame_effect(
            state, port, context, fixture.source(), surfaces, 0
        );

        test.expect_true(
            result.status == LegacyBattleFrameEffectStatus::completed &&
                result.applied_red_delta == -4 &&
                result.applied_green_delta == -4 &&
                result.applied_blue_delta == -4 &&
                fixture.flash.intensity == 0xF8U &&
                fixture.flash.active == 1U,
            "color cycle sign extends the byte and preserves nonzero wrapping subtraction"
        );
    }

    {
        LegacyBattleFrameEffectState state;
        Fixture fixture;
        const auto before = fixture.source_bytes;
        EffectPort port;
        auto context = fixture.context();

        const auto result = openswd3::battle::update_legacy_battle_frame_effect(
            state,
            port,
            context,
            fixture.source(),
            surfaces,
            std::numeric_limits<openswd3::compat::i32>::min()
        );

        test.expect_true(
            result.status == LegacyBattleFrameEffectStatus::completed &&
                result.source_rotation_calls == 1U &&
                result.source_rotation.status ==
                    openswd3::battle::LegacyBattleImageRotationStatus::
                        shift_not_positive &&
                result.source_blit_calls == 1U &&
                result.rotation_playback_calls == 1U &&
                fixture.source_bytes == before,
            "minimum signed rotation keeps two complement negation and returns through nonpositive shift"
        );
    }

    {
        LegacyBattleFrameEffectState state;
        Fixture fixture;
        fixture.source_bytes.clear();
        EffectPort port;
        auto context = fixture.context();

        const auto result = openswd3::battle::update_legacy_battle_frame_effect(
            state, port, context, fixture.source(), surfaces, 0
        );

        test.expect_true(
            result.status ==
                    LegacyBattleFrameEffectStatus::source_blit_typed_stop &&
                result.clip_calls == 1U && result.source_blit_calls == 1U &&
                result.rotation_frame_calls == 0U,
            "missing source bytes stops at first full blit after initial full clip side effect"
        );
    }
}
