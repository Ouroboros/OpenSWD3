#include "openswd3/battle/legacy_battle_runtime_shutdown.hpp"
#include "openswd3/battle/legacy_battle_group_b_storage.hpp"
#include "test.hpp"

#include <algorithm>
#include <array>
#include <memory>
#include <vector>

namespace {

using namespace openswd3::battle;
using openswd3::compat::u32;

class ShutdownPort final : public LegacyBattleRenderAuxiliaryBufferReleaser,
                           public LegacyBattleGroupAResourceReleasePort {
public:
    explicit ShutdownPort(LegacyBattleGroupBStorage& enemies)
        : enemies_(enemies) {}

    void release(const u32 token) noexcept override {
        release_order.push_back(token);
    }

    LegacyBattleGroupAResourceReleaseCallReply release_group_a_resource(
        const LegacyBattleGroupAResourceReleaseCallRequest& request
    ) override {
        for (const auto& actor : *enemies_.actors()) {
            enemies_live_during_party_release =
                enemies_live_during_party_release &&
                actor.resource_token != 0U &&
                !enemies_.resource_bytes(actor.resource_token).empty();
        }

        party_releases.push_back(request);
        release_order.push_back(request.resource_token);
        return {
            .eax = 0xA0000000U | request.actor_index,
            .ecx = request.actor_token,
            .edx = request.resource_offset
        };
    }

    LegacyBattleGroupBStorage& enemies_;
    std::vector<u32> release_order;
    std::vector<LegacyBattleGroupAResourceReleaseCallRequest> party_releases;
    bool enemies_live_during_party_release{true};
};

}  // namespace

void test_battle_runtime_shutdown(openswd3::test::Context& test) {
    using namespace openswd3::battle;

    {
        LegacyBattleStartupState startup;
        LegacyBattleGroupBStorage enemies;
        test.expect_true(
            enemies.construct(),
            "construct actual enemy allocations for shutdown"
        );
        startup.group_b_lifecycle = enemies.actors();
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

        std::array<u32, 8> enemy_tokens{};
        for (std::size_t index = 0U; index < enemy_tokens.size(); ++index) {
            auto& actor = (*enemies.actors())[index];
            enemy_tokens[index] = actor.resource_token;
            actor.resource_bytes.fill(0xA5U);
        }

        ShutdownPort port{enemies};
        const auto result =
            shutdown_legacy_battle_runtime(startup, port, port, &enemies);
        std::vector<u32> expected_release_order{0x12345678U};
        bool party_cleared = true;
        for (u32 index = 0U; index < kLegacyBattleGroupAObjectCount; ++index) {
            expected_release_order.push_back(0xA2000000U + index);
            expected_release_order.push_back(0xA1000000U + index);
            party_cleared = party_cleared &&
                startup.party[index].configuration.actor_record_token == 0U &&
                startup.party[index].secondary_resource_token == 0U;
        }

        bool enemies_cleared = true;
        for (std::size_t index = 0U; index < enemy_tokens.size(); ++index) {
            const auto& actor = (*enemies.actors())[index];
            enemies_cleared = enemies_cleared &&
                result.group_b_resource_cleanups[index].resource_released &&
                actor.resource_token == 0U &&
                enemies.resource_bytes(enemy_tokens[index]).empty() &&
                std::ranges::all_of(actor.resource_bytes, [](const auto value) {
                                  return value == 0U;
                              });
        }

        test.expect_true(
            result.status == LegacyBattleRuntimeShutdownStatus::completed &&
                result.render_cleanup.auxiliary_buffer_released &&
                result.render_cleanup.surface_row_offsets_released &&
                result.render_cleanup.primary_row_offsets_released &&
                startup.render_geometry.auxiliary_buffer_token == 0U &&
                startup.render_geometry.surface_row_offsets == nullptr &&
                startup.render_geometry.primary_row_offsets == nullptr &&
                port.party_releases.size() == 20U &&
                port.release_order == expected_release_order &&
                port.enemies_live_during_party_release && party_cleared &&
                enemies_cleared,
            "shutdown releases render and party resources before retiring all eight actual enemy records"
        );
    }

    {
        LegacyBattleStartupState startup;
        LegacyBattleGroupBStorage enemies;
        ShutdownPort port{enemies};
        const auto result =
            shutdown_legacy_battle_runtime(startup, port, port, nullptr);
        test.expect_true(
            result.status ==
                    LegacyBattleRuntimeShutdownStatus::
                        group_b_resource_typed_stop &&
                result.stopped_group_b_index == 0U &&
                port.release_order.empty() &&
                !result.render_cleanup.auxiliary_buffer_released &&
                !result.group_b_resource_cleanups[0U].resource_released,
            "shutdown before enemy initialization stops at the first missing actor without borrowing storage"
        );
    }

    {
        LegacyBattleStartupState startup;
        LegacyBattleGroupBStorage enemies;
        startup.group_b_lifecycle = enemies.actors();
        for (auto& actor : *startup.group_b_lifecycle) {
            actor.resource_bytes.fill(0x5AU);
        }

        ShutdownPort port{enemies};
        const auto result =
            shutdown_legacy_battle_runtime(startup, port, port, nullptr);
        test.expect_true(
            result.status == LegacyBattleRuntimeShutdownStatus::completed &&
                port.release_order.empty() &&
                std::ranges::all_of(
                    result.group_b_resource_cleanups,
                    [](const auto& cleanup) {
                        return cleanup.status ==
                            LegacyBattleGroupBResourceCleanupStatus::
                                completed &&
                            !cleanup.resource_released;
                    }
                ) &&
                std::ranges::all_of(
                    *startup.group_b_lifecycle,
                    [](const auto& actor) {
                        return actor.resource_token == 0U &&
                            std::ranges::all_of(
                                   actor.resource_bytes, [](const auto value) {
                                       return value == 0x5AU;
                                   }
                            );
                    }
                ),
            "eight zero enemy pointers preserve stale bytes and never require a storage release"
        );
    }
}
