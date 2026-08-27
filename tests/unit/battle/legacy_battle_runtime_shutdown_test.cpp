#include "openswd3/battle/legacy_battle_runtime_shutdown.hpp"
#include "test.hpp"

#include <memory>
#include <vector>

namespace {

using openswd3::battle::LegacyBattleRuntimeShutdownCall;
using openswd3::battle::LegacyBattleRuntimeShutdownCallReply;
using openswd3::battle::LegacyBattleRuntimeShutdownCallRequest;
using openswd3::battle::LegacyBattleRuntimeShutdownPort;
using openswd3::compat::u32;

class ShutdownPort final : public LegacyBattleRuntimeShutdownPort {
public:
    void release(const u32 token) noexcept override {
        released_tokens.push_back(token);
    }

    [[nodiscard]] LegacyBattleRuntimeShutdownCallReply
    invoke_battle_runtime_shutdown(
        const LegacyBattleRuntimeShutdownCallRequest& request
    ) override {
        calls.push_back(request);
        const u32 prefix = request.call ==
                LegacyBattleRuntimeShutdownCall::release_group_a_object
            ? 0xA0000000U
            : 0xB0000000U;
        return {
            .eax = prefix | request.object_index,
            .ecx = request.object_token,
            .edx = request.object_index + 0x100U,
        };
    }

    std::vector<u32> released_tokens;
    std::vector<LegacyBattleRuntimeShutdownCallRequest> calls;
};

}  // namespace

void test_battle_runtime_shutdown(openswd3::test::Context& test) {
    using openswd3::battle::kLegacyBattleGroupAObjectBaseToken;
    using openswd3::battle::kLegacyBattleGroupAObjectCount;
    using openswd3::battle::kLegacyBattleGroupAObjectStride;
    using openswd3::battle::kLegacyBattleGroupBObjectBaseToken;
    using openswd3::battle::kLegacyBattleGroupBObjectCount;
    using openswd3::battle::kLegacyBattleGroupBObjectStride;
    using openswd3::battle::shutdown_legacy_battle_runtime;

    {
        openswd3::battle::LegacyBattleStartupState startup;
        startup.render_geometry.auxiliary_buffer_token = 0x12345678U;
        startup.render_geometry.primary_row_offsets =
            std::make_unique<u32[]>(2U);
        startup.render_geometry.surface_row_offsets =
            std::make_unique<u32[]>(3U);
        ShutdownPort port;

        const auto result = shutdown_legacy_battle_runtime(startup, port);

        bool group_a_tokens_match = true;
        bool group_b_tokens_match = true;
        for (u32 index = 0U; index < kLegacyBattleGroupAObjectCount; ++index) {
            const auto& call = port.calls[index];
            group_a_tokens_match = group_a_tokens_match &&
                call.call ==
                    LegacyBattleRuntimeShutdownCall::release_group_a_object &&
                call.object_index == index &&
                call.object_token ==
                    kLegacyBattleGroupAObjectBaseToken +
                        index * kLegacyBattleGroupAObjectStride;
        }
        for (u32 index = 0U; index < kLegacyBattleGroupBObjectCount; ++index) {
            const auto& call =
                port.calls[kLegacyBattleGroupAObjectCount + index];
            group_b_tokens_match = group_b_tokens_match &&
                call.call ==
                    LegacyBattleRuntimeShutdownCall::release_group_b_object &&
                call.object_index == index &&
                call.object_token ==
                    kLegacyBattleGroupBObjectBaseToken +
                        index * kLegacyBattleGroupBObjectStride;
        }

        test.expect_true(
            result.render_cleanup_calls == 1U &&
                result.render_cleanup.auxiliary_buffer_released &&
                result.render_cleanup.surface_row_offsets_released &&
                result.render_cleanup.primary_row_offsets_released &&
                startup.render_geometry.auxiliary_buffer_token == 0U &&
                startup.render_geometry.surface_row_offsets == nullptr &&
                startup.render_geometry.primary_row_offsets == nullptr &&
                port.released_tokens == std::vector<u32>{0x12345678U} &&
                result.group_a_calls == 10U && result.group_b_calls == 8U &&
                port.calls.size() == 18U && group_a_tokens_match &&
                group_b_tokens_match,
            "runtime shutdown releases typed render resources before fixed ten group-A and eight group-B object calls"
        );
        test.expect_true(
            result.return_value == 0xB0000007U &&
                result.final_ecx ==
                    kLegacyBattleGroupBObjectBaseToken +
                        7U * kLegacyBattleGroupBObjectStride &&
                result.final_edx == 0x107U,
            "runtime shutdown returns the complete register reply from the eighth group-B destructor"
        );
    }

    {
        openswd3::battle::LegacyBattleStartupState startup;
        ShutdownPort port;

        const auto result = shutdown_legacy_battle_runtime(startup, port);

        test.expect_true(
            !result.render_cleanup.auxiliary_buffer_released &&
                !result.render_cleanup.surface_row_offsets_released &&
                !result.render_cleanup.primary_row_offsets_released &&
                port.released_tokens.empty() && port.calls.size() == 18U &&
                port.calls.front().object_token ==
                    kLegacyBattleGroupAObjectBaseToken &&
                port.calls.back().object_token ==
                    kLegacyBattleGroupBObjectBaseToken +
                        7U * kLegacyBattleGroupBObjectStride,
            "empty render resources do not suppress either fixed object destructor loop"
        );
    }
}
