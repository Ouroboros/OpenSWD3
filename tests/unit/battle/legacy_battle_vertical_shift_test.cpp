#include "openswd3/battle/legacy_battle_vertical_shift.hpp"

#include <algorithm>
#include <array>
#include <optional>
#include <vector>

#include "test.hpp"

namespace {

using openswd3::compat::u16;
using openswd3::compat::u32;

class ShiftPort final : public openswd3::battle::LegacyBattleVerticalShiftPort {
public:
    [[nodiscard]] u32 resolve_vertical_shift_surface(
        const u32 owner_token, const u32 selector
    ) override {
        resolve_requests.push_back({owner_token, selector});
        if (resolve_index < resolve_returns.size()) {
            return resolve_returns[resolve_index++];
        }
        ++resolve_index;
        return 0x70000000U + static_cast<u32>(resolve_index);
    }

    [[nodiscard]] u32 blit_vertical_shift(
        const openswd3::battle::LegacyBattleSurfaceBlendOperation& operation
    ) override {
        operations.push_back(operation);
        if (operations.size() == 1U && phase_after_first_blit.has_value()) {
            battle_vertical_shift_state().phase_index = *phase_after_first_blit;
        }
        if (operations.size() == 2U && mutate_after_second_blit) {
            battle_vertical_shift_state().phase_index = second_phase;
            battle_vertical_shift_state().tick_counter = second_tick;
            battle_vertical_shift_state().tick_limit = second_limit;
            *battle_mode_flags = second_mode_flags;
        }
        return blit_return;
    }

    std::vector<std::array<u32, 2>> resolve_requests;
    std::vector<u32> resolve_returns;
    std::size_t resolve_index{};
    std::vector<openswd3::battle::LegacyBattleSurfaceBlendOperation> operations;
    std::optional<u32> phase_after_first_blit;
    bool mutate_after_second_blit{};
    u32 second_phase{};
    u32 second_tick{};
    u32 second_limit{};
    u32 second_mode_flags{};
    u32* battle_mode_flags{};
    u32 blit_return{0xDEADBEEFU};
};

struct FramebufferFixture {
    openswd3::rendering::LegacyFramebuffer framebuffer{
        {.pitch_bytes = 1280, .width = 640, .height = 480}
    };

    FramebufferFixture() {
        auto pixels = framebuffer.physical_pixels();
        std::fill(pixels.begin(), pixels.end(), u16{0x7FFFU});
    }
};

[[nodiscard]] bool rectangle_is(
    const std::optional<openswd3::battle::LegacyBattleSurfaceBlendRectangle>&
        rectangle,
    const openswd3::battle::LegacyBattleSurfaceBlendRectangle& expected
) {
    return rectangle.has_value() && *rectangle == expected;
}

}  // namespace

void test_battle_vertical_shift(openswd3::test::Context& test) {
    {
        FramebufferFixture fixture;
        auto& framebuffer = fixture.framebuffer;
        ShiftPort port;
        auto& state = port.battle_vertical_shift_state();
        state.phase_index = 0U;
        state.tick_counter = 0U;
        state.tick_limit = 5U;
        u32 completion_gate = 1U;
        u32 mode_flags = 0U;
        const auto result = openswd3::battle::run_legacy_battle_vertical_shift(
            port, completion_gate, mode_flags, framebuffer
        );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleVerticalShiftStatus::
                        completed &&
                result.signed_offsets == std::array{4, 4, 4} &&
                result.table_reads == 3U &&
                result.surface_resolve_calls == 2U &&
                result.surface_blit_calls == 2U &&
                result.cleared_bytes == 5120U &&
                result.cleared_pixels == 2560U && result.return_value == 0U &&
                state.phase_index == 0U && state.tick_counter == 1U &&
                completion_gate == 1U &&
                port.resolve_requests ==
                    std::vector<std::array<u32, 2>>{
                        {0x004AB870U, 0x2711U},
                        {0x004AB870U, 0x2711U},
                    } &&
                port.operations.size() == 2U &&
                port.operations[0].kind ==
                    openswd3::battle::LegacyBattleSurfaceBlendOperationKind::
                        vertical_shift_frame &&
                port.operations[0].object_token == 0x70000001U &&
                port.operations[0].source_token == 0x004ACBA0U &&
                port.operations[0].flags == 0x01000000U &&
                rectangle_is(
                    port.operations[0].destination_rectangle,
                    {.left = 0, .top = 0, .right = 640, .bottom = 476}
                ) &&
                rectangle_is(
                    port.operations[0].source_rectangle,
                    {.left = 0, .top = 0, .right = 640, .bottom = 476}
                ) &&
                rectangle_is(
                    port.operations[1].destination_rectangle,
                    {.left = 0, .top = 476, .right = 640, .bottom = 480}
                ) &&
                rectangle_is(
                    port.operations[1].source_rectangle,
                    {.left = 0, .top = 0, .right = 640, .bottom = 4}
                ) &&
                std::all_of(
                    framebuffer.physical_pixels().begin(),
                    framebuffer.physical_pixels().begin() + 2560,
                    [](const u16 pixel) { return pixel == 0U; }
                ) &&
                framebuffer.physical_pixels()[2560U] == 0x7FFFU,
            "vertical shift even phase moves the cropped frame upward then copies the cleared band to the bottom"
        );
    }

    {
        FramebufferFixture fixture;
        auto& framebuffer = fixture.framebuffer;
        ShiftPort port;
        auto& state = port.battle_vertical_shift_state();
        state.phase_index = 1U;
        state.tick_limit = 0U;
        u32 completion_gate = 1U;
        u32 mode_flags = 0U;
        const auto result = openswd3::battle::run_legacy_battle_vertical_shift(
            port, completion_gate, mode_flags, framebuffer
        );
        test.expect_true(
            result.signed_offsets == std::array{3, 3, 3} &&
                rectangle_is(
                    result.operations[0].destination_rectangle,
                    {.left = 0, .top = 3, .right = 640, .bottom = 480}
                ) &&
                rectangle_is(
                    result.operations[0].source_rectangle,
                    {.left = 0, .top = 0, .right = 640, .bottom = 477}
                ) &&
                rectangle_is(
                    result.operations[1].destination_rectangle,
                    {.left = 0, .top = 0, .right = 640, .bottom = 3}
                ) &&
                result.cleared_bytes == 3840U && state.phase_index == 2U &&
                state.tick_counter == 0U && result.return_value == 2U,
            "vertical shift odd phase moves the cropped frame downward and advances after the live tick limit"
        );
    }

    {
        constexpr std::array expected{
            4, 3, 4, 3, 4, 3, 2, 3, 2, 3, 2, 1, 2, 1, 2, 0
        };
        for (u32 phase = 0U; phase < expected.size(); ++phase) {
            FramebufferFixture fixture;
            auto& framebuffer = fixture.framebuffer;
            ShiftPort port;
            auto& state = port.battle_vertical_shift_state();
            state.phase_index = phase;
            state.tick_limit = 0x7FFFFFFFU;
            u32 completion_gate = 1U;
            u32 mode_flags = 0U;
            const auto result =
                openswd3::battle::run_legacy_battle_vertical_shift(
                    port, completion_gate, mode_flags, framebuffer
                );
            test.expect_true(
                result.status ==
                        openswd3::battle::LegacyBattleVerticalShiftStatus::
                            completed &&
                    result.signed_offsets ==
                        std::array{
                            expected[phase], expected[phase], expected[phase]
                        },
                "vertical shift preserves every physical signed offset table entry"
            );
        }
    }

    {
        FramebufferFixture fixture;
        auto& framebuffer = fixture.framebuffer;
        ShiftPort port;
        auto& state = port.battle_vertical_shift_state();
        state.phase_index = 9U;
        state.tick_limit = 0U;
        u32 completion_gate = 7U;
        u32 mode_flags = 0U;
        const auto result = openswd3::battle::run_legacy_battle_vertical_shift(
            port, completion_gate, mode_flags, framebuffer
        );
        test.expect_true(
            result.return_value == 10U && state.phase_index == 0U &&
                state.tick_counter == 0U && completion_gate == 0U,
            "vertical shift returns ten before clearing the completion gate and phase"
        );
    }

    {
        FramebufferFixture fixture;
        auto& framebuffer = fixture.framebuffer;
        ShiftPort port;
        auto& state = port.battle_vertical_shift_state();
        state.phase_index = 0U;
        state.tick_limit = 0U;
        u32 completion_gate = 1U;
        u32 mode_flags = 0x00000100U;
        const auto first = openswd3::battle::run_legacy_battle_vertical_shift(
            port, completion_gate, mode_flags, framebuffer
        );
        test.expect_true(
            first.return_value == 1U && state.phase_index == 1U,
            "vertical shift mode bit advances phase zero to one by signed remainder"
        );

        state.tick_limit = 0U;
        const auto second = openswd3::battle::run_legacy_battle_vertical_shift(
            port, completion_gate, mode_flags, framebuffer
        );
        test.expect_true(
            second.return_value == 0U && state.phase_index == 0U,
            "vertical shift mode bit advances phase one back to zero"
        );
    }

    {
        FramebufferFixture fixture;
        auto& framebuffer = fixture.framebuffer;
        ShiftPort port;
        auto& state = port.battle_vertical_shift_state();
        state.phase_index = 0U;
        state.tick_limit = 9U;
        port.phase_after_first_blit = 15U;
        u32 completion_gate = 1U;
        u32 mode_flags = 0U;
        const auto result = openswd3::battle::run_legacy_battle_vertical_shift(
            port, completion_gate, mode_flags, framebuffer
        );
        test.expect_true(
            result.signed_offsets == std::array{4, 0, 0} &&
                result.cleared_bytes == 0U &&
                rectangle_is(
                    result.operations[1].destination_rectangle,
                    {.left = 0, .top = 0, .right = 640, .bottom = 0}
                ),
            "vertical shift rereads the live phase after the first surface call"
        );
    }

    {
        FramebufferFixture fixture;
        auto& framebuffer = fixture.framebuffer;
        ShiftPort port;
        port.battle_vertical_shift_state().phase_index = 0U;
        port.phase_after_first_blit = 16U;
        u32 completion_gate = 1U;
        u32 mode_flags = 0U;
        const auto result = openswd3::battle::run_legacy_battle_vertical_shift(
            port, completion_gate, mode_flags, framebuffer
        );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleVerticalShiftStatus::
                        phase_table_typed_stop &&
                result.table_reads == 1U && result.surface_blit_calls == 1U &&
                result.cleared_bytes == 0U,
            "vertical shift stops at the second physical table read after preserving the first blit"
        );
    }

    {
        FramebufferFixture fixture;
        auto& framebuffer = fixture.framebuffer;
        ShiftPort port;
        port.battle_vertical_shift_state().phase_index = 16U;
        u32 completion_gate = 1U;
        u32 mode_flags = 0U;
        const auto result = openswd3::battle::run_legacy_battle_vertical_shift(
            port, completion_gate, mode_flags, framebuffer
        );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleVerticalShiftStatus::
                        phase_table_typed_stop &&
                result.surface_resolve_calls == 0U &&
                result.surface_blit_calls == 0U,
            "vertical shift stops at the first one-past table access"
        );
    }

    {
        FramebufferFixture fixture;
        auto& framebuffer = fixture.framebuffer;
        ShiftPort port;
        port.resolve_returns = {0U};
        u32 completion_gate = 1U;
        u32 mode_flags = 0U;
        const auto result = openswd3::battle::run_legacy_battle_vertical_shift(
            port, completion_gate, mode_flags, framebuffer
        );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleVerticalShiftStatus::
                        primary_surface_typed_stop &&
                result.surface_resolve_calls == 1U &&
                result.surface_blit_calls == 0U && result.cleared_bytes == 0U,
            "vertical shift stops at the first null surface dereference"
        );
    }

    {
        FramebufferFixture fixture;
        auto& framebuffer = fixture.framebuffer;
        ShiftPort port;
        port.resolve_returns = {0x7000U, 0U};
        u32 completion_gate = 1U;
        u32 mode_flags = 0U;
        const auto result = openswd3::battle::run_legacy_battle_vertical_shift(
            port, completion_gate, mode_flags, framebuffer
        );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleVerticalShiftStatus::
                        primary_surface_typed_stop &&
                result.surface_resolve_calls == 2U &&
                result.surface_blit_calls == 1U &&
                result.cleared_bytes == 5120U,
            "vertical shift preserves the first blit and framebuffer clear before the second null surface"
        );
    }

    {
        openswd3::rendering::LegacyFramebuffer framebuffer{
            {.pitch_bytes = 2, .width = 1, .height = 1}
        };
        framebuffer.physical_pixels()[0] = 0x7FFFU;
        ShiftPort port;
        u32 completion_gate = 1U;
        u32 mode_flags = 0U;
        const auto result = openswd3::battle::run_legacy_battle_vertical_shift(
            port, completion_gate, mode_flags, framebuffer
        );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleVerticalShiftStatus::
                        framebuffer_typed_stop &&
                result.surface_blit_calls == 1U && result.table_reads == 2U &&
                result.cleared_bytes == 0U &&
                framebuffer.physical_pixels()[0] == 0x7FFFU,
            "vertical shift stops before the first full dword clear when only one pixel is owned"
        );
    }

    {
        FramebufferFixture fixture;
        auto& framebuffer = fixture.framebuffer;
        ShiftPort port;
        auto& state = port.battle_vertical_shift_state();
        u32 completion_gate = 1U;
        u32 mode_flags = 0U;
        port.mutate_after_second_blit = true;
        port.second_phase = 0U;
        port.second_tick = 4U;
        port.second_limit = 4U;
        port.second_mode_flags = 0x00000100U;
        port.battle_mode_flags = &mode_flags;
        const auto result = openswd3::battle::run_legacy_battle_vertical_shift(
            port, completion_gate, mode_flags, framebuffer
        );
        test.expect_true(
            result.return_value == 1U && state.phase_index == 1U &&
                state.tick_counter == 0U,
            "vertical shift rereads phase tick limit and mode after the second surface call"
        );
    }
}
