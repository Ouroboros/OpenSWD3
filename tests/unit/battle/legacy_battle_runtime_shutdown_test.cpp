#include "openswd3/battle/legacy_battle_runtime_shutdown.hpp"
#include "test.hpp"

#include <algorithm>
#include <array>
#include <memory>
#include <vector>

namespace {

using namespace openswd3::battle;
using openswd3::compat::u32;

class ShutdownPort final : public LegacyBattleRenderAuxiliaryBufferReleaser,
                           public LegacyBattleGroupAResourceReleasePort,
                           public LegacyBattleGroupBResourceReleasePort {
public:
    void release(const u32 token) noexcept override {
        released_tokens.push_back(token);
        release_order.push_back(token);
    }

    LegacyBattleGroupAResourceReleaseCallReply release_group_a_resource(
        const LegacyBattleGroupAResourceReleaseCallRequest& request
    ) override {
        party_releases.push_back(request);
        release_order.push_back(request.resource_token);
        return {
            .eax = 0xA0000000U | request.actor_index,
            .ecx = request.actor_token,
            .edx = request.resource_offset,
        };
    }

    LegacyBattleGroupBResourceReleaseCallReply release_group_b_resource(
        const LegacyBattleGroupBResourceReleaseCallRequest& request
    ) override {
        enemy_releases.push_back(request);
        release_order.push_back(request.resource_token);
        return {
            .eax = 0xB0000000U | request.actor_index,
            .ecx = request.actor_token,
            .edx = request.actor_index + 0x100U,
        };
    }

    std::vector<u32> released_tokens;
    std::vector<u32> release_order;
    std::vector<LegacyBattleGroupAResourceReleaseCallRequest> party_releases;
    std::vector<LegacyBattleGroupBResourceReleaseCallRequest> enemy_releases;
};

}  // namespace

void test_battle_runtime_shutdown(openswd3::test::Context& test) {
    using openswd3::battle::kLegacyBattleGroupAObjectBaseToken;
    using openswd3::battle::kLegacyBattleGroupAObjectCount;
    using openswd3::battle::kLegacyBattleGroupAObjectStride;
    using openswd3::battle::kLegacyBattleGroupBObjectBaseToken;
    using openswd3::battle::kLegacyBattleGroupBObjectCount;
    using openswd3::battle::kLegacyBattleGroupBObjectStride;
    using openswd3::battle::LegacyBattleActorGroupBElementState;
    using openswd3::battle::LegacyBattleRuntimeShutdownStatus;
    using openswd3::battle::shutdown_legacy_battle_runtime;

    {
        openswd3::battle::LegacyBattleStartupState startup;
        startup.render_geometry.auxiliary_buffer_token = 0x12345678U;
        startup.render_geometry.primary_row_offsets =
            std::make_unique<u32[]>(2U);
        startup.render_geometry.surface_row_offsets =
            std::make_unique<u32[]>(3U);
        for (u32 index = 0U; index < kLegacyBattleGroupAObjectCount; ++index) {
            startup.party[index].configuration.actor_record_token =
                0xA1000000U + index;
            startup.party[index].secondary_resource_token = 0xA2000000U + index;
        }
        startup.group_b_lifecycle = std::make_shared<std::array<
            LegacyBattleActorGroupBElementState,
            kLegacyBattleGroupBObjectCount>>();
        for (u32 index = 0U; index < kLegacyBattleGroupBObjectCount; ++index) {
            auto& actor = (*startup.group_b_lifecycle)[index];
            actor.object_token = kLegacyBattleGroupBObjectBaseToken +
                index * kLegacyBattleGroupBObjectStride;
            actor.resource_token = 0xB1000000U + index;
            actor.resource_bytes.fill(0xA5U);
        }
        ShutdownPort port;

        const auto result =
            shutdown_legacy_battle_runtime(startup, port, port, port);

        std::vector<u32> expected_release_order{0x12345678U};
        bool group_a_tokens_match = true;
        bool group_b_resources_match = true;
        for (u32 index = 0U; index < kLegacyBattleGroupAObjectCount; ++index) {
            const auto& secondary = port.party_releases[index * 2U];
            const auto& primary = port.party_releases[index * 2U + 1U];
            expected_release_order.push_back(0xA2000000U + index);
            expected_release_order.push_back(0xA1000000U + index);
            const u32 object_token = kLegacyBattleGroupAObjectBaseToken +
                index * kLegacyBattleGroupAObjectStride;
            group_a_tokens_match = group_a_tokens_match &&
                secondary.actor_index == index &&
                primary.actor_index == index &&
                secondary.actor_token == object_token &&
                primary.actor_token == object_token &&
                secondary.resource_token == 0xA2000000U + index &&
                secondary.resource_offset == 0x2BC4U &&
                primary.resource_token == 0xA1000000U + index &&
                primary.resource_offset == 0U &&
                startup.party[index].configuration.actor_record_token == 0U &&
                startup.party[index].secondary_resource_token == 0U;
        }
        for (u32 index = 0U; index < kLegacyBattleGroupBObjectCount; ++index) {
            const auto& call = port.enemy_releases[index];
            expected_release_order.push_back(0xB1000000U + index);
            const auto& actor = (*startup.group_b_lifecycle)[index];
            const u32 expected_edx = index == 0U ? 0U : index + 0xFFU;
            group_b_resources_match = group_b_resources_match &&
                call.actor_index == index &&
                call.actor_token ==
                    kLegacyBattleGroupBObjectBaseToken +
                        index * kLegacyBattleGroupBObjectStride &&
                call.resource_token == 0xB1000000U + index &&
                call.resource_offset == 0x0CU &&
                call.eax == 0xB1000000U + index &&
                call.ecx == call.actor_token && call.edx == expected_edx &&
                result.group_b_resource_cleanups[index].return_eax ==
                    (0xB0000000U | index) &&
                result.group_b_resource_cleanups[index].resource_released &&
                actor.resource_token == 0U &&
                std::ranges::all_of(actor.resource_bytes, [](const auto value) {
                                          return value == 0U;
                                      });
        }

        test.expect_true(
            result.status == LegacyBattleRuntimeShutdownStatus::completed &&
                result.render_cleanup_calls == 1U &&
                result.render_cleanup.auxiliary_buffer_released &&
                result.render_cleanup.surface_row_offsets_released &&
                result.render_cleanup.primary_row_offsets_released &&
                startup.render_geometry.auxiliary_buffer_token == 0U &&
                startup.render_geometry.surface_row_offsets == nullptr &&
                startup.render_geometry.primary_row_offsets == nullptr &&
                port.released_tokens == std::vector<u32>{0x12345678U} &&
                result.group_a_calls == 10U &&
                result.group_a_resource_calls == 20U &&
                result.group_b_calls == 8U &&
                result.group_b_resource_calls == 8U &&
                port.party_releases.size() == 20U &&
                port.enemy_releases.size() == 8U &&
                port.release_order == expected_release_order &&
                group_a_tokens_match && group_b_resources_match,
            "runtime shutdown releases typed render group-A and group-B resources in fixed order"
        );
        test.expect_true(
            result.return_value == 0xB0000007U &&
                result.final_ecx ==
                    kLegacyBattleGroupBObjectBaseToken +
                        7U * kLegacyBattleGroupBObjectStride &&
                result.final_edx == 0x107U,
            "runtime shutdown returns the complete release reply from the eighth group-B actor"
        );
    }

    {
        openswd3::battle::LegacyBattleStartupState startup;
        ShutdownPort port;

        const auto result =
            shutdown_legacy_battle_runtime(startup, port, port, port);

        test.expect_true(
            result.status ==
                    LegacyBattleRuntimeShutdownStatus::
                        group_b_resource_typed_stop &&
                !result.render_cleanup.auxiliary_buffer_released &&
                !result.render_cleanup.surface_row_offsets_released &&
                !result.render_cleanup.primary_row_offsets_released &&
                port.released_tokens.empty() && port.party_releases.empty() &&
                port.enemy_releases.empty() && result.group_a_calls == 10U &&
                result.group_a_resource_calls == 0U &&
                result.group_b_calls == 1U &&
                result.group_b_resource_calls == 0U &&
                result.stopped_group_b_index == 0U &&
                result.return_value == 0U &&
                result.final_ecx == kLegacyBattleGroupBObjectBaseToken &&
                result.final_edx == 0U,
            "missing group-B lifecycle state stops runtime shutdown at its first resource field access"
        );
    }

    {
        openswd3::battle::LegacyBattleStartupState startup;
        startup.group_b_lifecycle = std::make_shared<std::array<
            LegacyBattleActorGroupBElementState,
            kLegacyBattleGroupBObjectCount>>();
        for (auto& actor : *startup.group_b_lifecycle) {
            actor.resource_bytes.fill(0x5AU);
        }
        ShutdownPort port;

        const auto result =
            shutdown_legacy_battle_runtime(startup, port, port, port);

        const bool bytes_unchanged = std::ranges::all_of(
            *startup.group_b_lifecycle, [](const auto& actor) {
                return actor.resource_token == 0U &&
                    std::ranges::all_of(
                           actor.resource_bytes,
                           [](const auto value) { return value == 0x5AU; }
                    );
            }
        );
        test.expect_true(
            result.status == LegacyBattleRuntimeShutdownStatus::completed &&
                result.group_a_calls == 10U &&
                result.group_a_resource_calls == 0U &&
                result.group_b_calls == 8U &&
                result.group_b_resource_calls == 0U &&
                port.party_releases.empty() && port.enemy_releases.empty() &&
                bytes_unchanged && result.return_value == 0U &&
                result.final_ecx ==
                    kLegacyBattleGroupBObjectBaseToken +
                        7U * kLegacyBattleGroupBObjectStride &&
                result.final_edx == 0U,
            "eight zero group-B tokens skip every release while preserving stale resource bytes"
        );
    }
}
