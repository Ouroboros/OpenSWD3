#include "openswd3/battle/legacy_battle_surface_blend.hpp"

#include <vector>

#include "test.hpp"

namespace {

using openswd3::battle::LegacyBattleSurfaceBlendOperation;
using openswd3::compat::i32;
using openswd3::compat::u32;

class BlendPort final : public openswd3::battle::LegacyBattleSurfaceBlendPort {
public:
    [[nodiscard]] i32 query_system_metric(const i32 index) override {
        metric_indices.push_back(index);
        return index == 1 ? screen_height : screen_width;
    }

    [[nodiscard]] u32 create_screen_surface(
        const u32 owner_token, const i32 width, const i32 height
    ) override {
        screen_create_owner = owner_token;
        screen_create_width = width;
        screen_create_height = height;
        ++screen_create_calls;
        return screen_surface_token;
    }

    [[nodiscard]] u32
    create_temporary_surface(const u32 owner_token, const u32 format) override {
        temporary_owners.push_back(owner_token);
        temporary_formats.push_back(format);
        const u32 call_index = temporary_create_calls++;
        if (call_index == failed_temporary_index) {
            return 0U;
        }
        return 0x60000000U + call_index;
    }

    [[nodiscard]] u32 random_below(const u32 bound) override {
        random_bounds.push_back(bound);
        return random_return;
    }

    [[nodiscard]] u32 operate_surface(
        const LegacyBattleSurfaceBlendOperation& operation
    ) override {
        operations.push_back(operation);
        return operation_return;
    }

    [[nodiscard]] u32 release_surface(const u32 surface_token) override {
        released_tokens.push_back(surface_token);
        return release_return;
    }

    std::vector<i32> metric_indices;
    std::vector<u32> random_bounds;
    std::vector<LegacyBattleSurfaceBlendOperation> operations;
    std::vector<u32> temporary_owners;
    std::vector<u32> temporary_formats;
    std::vector<u32> released_tokens;
    i32 screen_width{1920};
    i32 screen_height{1080};
    i32 screen_create_width{};
    i32 screen_create_height{};
    u32 screen_create_owner{};
    u32 screen_create_calls{};
    u32 screen_surface_token{0x50000000U};
    u32 temporary_create_calls{};
    u32 failed_temporary_index{0xFFFFFFFFU};
    u32 random_return{19U};
    u32 operation_return{0x11223344U};
    u32 release_return{0xCAFEBABEU};
};

}  // namespace

void test_battle_surface_blend(openswd3::test::Context& test) {
    {
        openswd3::battle::LegacyBattleSurfaceBlendState state;
        state.unused_random_table.fill(0xFFFFFFFFU);
        state.row_offsets.fill(1234);
        BlendPort port;

        const auto result = openswd3::battle::run_legacy_battle_surface_blend(
            state,
            port,
            openswd3::battle::LegacyBattleSurfaceBlendRequest{
                .primary_surface_token = 0x11111111U,
                .secondary_surface_token = 0x22222222U,
                .ignored_arguments = {
                    0x33333333U,
                    0x44444444U,
                    0x55555555U,
                    0xFFFFFFFFU,
                },
            }
        );

        const auto& first_capture = port.operations[0];
        const auto& first_row = port.operations[1];
        const auto& first_temporary = port.operations[481];
        const auto& second_capture = port.operations[482];
        const auto& last_row = port.operations[962];
        const auto& second_temporary = port.operations[963];
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleSurfaceBlendStatus::
                        completed &&
                result.return_value == 0xCAFEBABEU &&
                result.metric_queries == 2U && result.random_calls == 1440U &&
                result.unused_random_writes == 480U &&
                result.outer_passes == 2U &&
                result.secondary_capture_calls == 2U &&
                result.row_operation_calls == 960U &&
                result.rectangle_pairs == 960U &&
                result.temporary_surface_calls == 2U &&
                result.temporary_copy_calls == 2U &&
                result.release_calls == 1U &&
                state.screen_surface_token == 0x50000000U &&
                state.screen_width == 1920 && state.screen_height == 1080 &&
                state.completed_row_observations == 960U &&
                state.unused_random_table.front() == 34U &&
                state.unused_random_table.back() == 34U &&
                state.row_offsets.front() == 0 &&
                state.row_offsets.back() == 0 &&
                port.metric_indices == std::vector<i32>{1, 0} &&
                port.screen_create_calls == 1U &&
                port.screen_create_owner ==
                    openswd3::battle::kLegacyBattleSurfaceBlendOwnerToken &&
                port.screen_create_width == 1920 &&
                port.screen_create_height == 1080 &&
                port.random_bounds.size() == 1440U &&
                port.random_bounds.front() == 20U &&
                port.random_bounds.back() == 20U &&
                port.operations.size() == 964U &&
                first_capture.kind ==
                    openswd3::battle::LegacyBattleSurfaceBlendOperationKind::
                        capture_secondary &&
                first_capture.object_token == 0x50000000U &&
                first_capture.source_token == 0x22222222U &&
                !first_capture.destination_rectangle.has_value() &&
                !first_capture.source_rectangle.has_value() &&
                first_row.kind ==
                    openswd3::battle::LegacyBattleSurfaceBlendOperationKind::
                        blend_primary_row &&
                first_row.source_token == 0x11111111U &&
                first_row.destination_rectangle ==
                    openswd3::battle::LegacyBattleSurfaceBlendRectangle{
                        0, 479, 640, 480
                    } &&
                first_row.source_rectangle == first_row.destination_rectangle &&
                first_temporary.kind ==
                    openswd3::battle::LegacyBattleSurfaceBlendOperationKind::
                        copy_screen_to_temporary &&
                first_temporary.object_token == 0x60000000U &&
                first_temporary.source_token == 0x50000000U &&
                second_capture.kind == first_capture.kind &&
                last_row.destination_rectangle ==
                    openswd3::battle::LegacyBattleSurfaceBlendRectangle{
                        0, 0, 640, 1
                    } &&
                second_temporary.object_token == 0x60000001U &&
                port.temporary_owners ==
                    std::vector<u32>{
                        openswd3::battle::kLegacyBattleSurfaceBlendOwnerToken,
                        openswd3::battle::kLegacyBattleSurfaceBlendOwnerToken,
                    } &&
                port.temporary_formats == std::vector<u32>{0x2711U, 0x2711U} &&
                port.released_tokens == std::vector<u32>{0x50000000U},
            "surface blend preserves unused random table two reverse row passes rectangle pairing temporary copies and final release eax"
        );
    }

    {
        openswd3::battle::LegacyBattleSurfaceBlendState state;
        BlendPort port;
        port.screen_surface_token = 0U;

        const auto result = openswd3::battle::run_legacy_battle_surface_blend(
            state, port, openswd3::battle::LegacyBattleSurfaceBlendRequest{}
        );

        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleSurfaceBlendStatus::
                        screen_surface_typed_stop &&
                result.metric_queries == 2U && result.random_calls == 480U &&
                result.unused_random_writes == 480U &&
                result.outer_passes == 0U && port.operations.empty() &&
                port.temporary_create_calls == 0U &&
                port.released_tokens.empty(),
            "null screen surface stops only at first virtual access after metrics creation zeroing and unused random fill"
        );
    }

    {
        openswd3::battle::LegacyBattleSurfaceBlendState state;
        BlendPort port;
        port.failed_temporary_index = 0U;

        const auto result = openswd3::battle::run_legacy_battle_surface_blend(
            state,
            port,
            openswd3::battle::LegacyBattleSurfaceBlendRequest{
                .primary_surface_token = 7U,
                .secondary_surface_token = 8U,
            }
        );

        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleSurfaceBlendStatus::
                        temporary_surface_typed_stop &&
                result.random_calls == 960U && result.outer_passes == 1U &&
                result.secondary_capture_calls == 1U &&
                result.row_operation_calls == 480U &&
                state.completed_row_observations == 480U &&
                result.temporary_surface_calls == 1U &&
                result.temporary_copy_calls == 0U &&
                result.release_calls == 0U && port.operations.size() == 481U &&
                port.released_tokens.empty(),
            "null temporary surface stops after first complete reverse row pass without synthetic copy repeat or release"
        );
    }
}
