#include "openswd3/battle/legacy_battle_runtime_shutdown.hpp"
#include "openswd3/battle/legacy_battle_group_a_storage.hpp"
#include "openswd3/battle/legacy_battle_group_b_storage.hpp"
#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "test.hpp"

#include <algorithm>
#include <array>
#include <memory>
#include <vector>

namespace {

using namespace openswd3::battle;
using openswd3::compat::u32;

class ShutdownPort final : public LegacyBattleRenderAuxiliaryBufferReleaser {
public:
    ShutdownPort(
        LegacyBattleStartupState& startup,
        LegacyBattleGroupAStorage& party,
        LegacyBattleGroupBStorage& enemies
    )
        : startup_(startup), party_(party), enemies_(enemies) {}

    void release(const u32 token) noexcept override {
        release_order.push_back(token);
        actors_live_during_render_release =
            std::ranges::all_of(
                startup_.party,
                [&](const auto& actor) {
                    return !party_
                                .record_bytes(
                                    actor.configuration.actor_record_token
                                )
                                .empty() &&
                        !party_.record_bytes(actor.secondary_resource_token)
                             .empty();
                }

            ) &&
            std::ranges::all_of(*enemies_.actors(), [&](const auto& actor) {
                return !enemies_.resource_bytes(actor.resource_token).empty();
            });
    }

    LegacyBattleStartupState& startup_;
    LegacyBattleGroupAStorage& party_;
    LegacyBattleGroupBStorage& enemies_;
    std::vector<u32> release_order;
    bool actors_live_during_render_release{};
};

}  // namespace

void test_battle_runtime_shutdown(openswd3::test::Context& test) {
    {
        LegacyBattleStartupState startup;
        auto action = std::make_unique<LegacyBattleActionDispatchState>();
        LegacyBattleGroupAStorage party_resources{startup, *action};
        LegacyBattleGroupBStorage enemies;
        test.expect_true(
            party_resources.construct() && enemies.construct(),
            "construct actual party and enemy allocations for shutdown"
        );
        startup.group_b_lifecycle = enemies.actors();
        startup.render_geometry.auxiliary_buffer_token = 0x12345678U;
        startup.render_geometry.primary_row_offsets =
            std::make_unique<u32[]>(2U);
        startup.render_geometry.surface_row_offsets =
            std::make_unique<u32[]>(3U);
        std::array<u32, 10> primary_tokens{};
        std::array<u32, 10> secondary_tokens{};
        for (u32 index = 0U; index < kLegacyBattleGroupAObjectCount; ++index) {
            auto& actor = startup.party[index];
            primary_tokens[index] = actor.configuration.actor_record_token;
            secondary_tokens[index] = party_resources.allocate_profile();
            actor.configuration.profile_token = secondary_tokens[index];
            actor.secondary_resource_token = secondary_tokens[index];
        }

        std::array<u32, 8> enemy_tokens{};
        for (std::size_t index = 0U; index < enemy_tokens.size(); ++index) {
            auto& actor = (*enemies.actors())[index];
            enemy_tokens[index] = actor.resource_token;
            actor.resource_bytes.fill(0xA5U);
        }

        ShutdownPort port{startup, party_resources, enemies};
        const auto result = shutdown_legacy_battle_runtime(
            startup, port, &party_resources, &enemies
        );
        bool party_cleared = true;
        for (u32 index = 0U; index < kLegacyBattleGroupAObjectCount; ++index) {
            party_cleared = party_cleared &&
                result.group_a_resource_cleanups[index]
                    .primary_resource_released &&
                result.group_a_resource_cleanups[index]
                    .secondary_resource_released &&
                party_resources.record_bytes(primary_tokens[index]).empty() &&
                party_resources.record_bytes(secondary_tokens[index]).empty() &&
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
                port.release_order == std::vector<u32>{0x12345678U} &&
                port.actors_live_during_render_release && party_cleared &&
                enemies_cleared,
            "shutdown releases rendering before retiring actual party and enemy records"
        );
    }

    {
        LegacyBattleStartupState startup;
        LegacyBattleGroupBStorage enemies;
        auto action = std::make_unique<LegacyBattleActionDispatchState>();
        LegacyBattleGroupAStorage party_resources{startup, *action};
        ShutdownPort port{startup, party_resources, enemies};
        const auto result =
            shutdown_legacy_battle_runtime(startup, port, nullptr, nullptr);
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

        auto action = std::make_unique<LegacyBattleActionDispatchState>();
        LegacyBattleGroupAStorage party_resources{startup, *action};
        ShutdownPort port{startup, party_resources, enemies};
        const auto result =
            shutdown_legacy_battle_runtime(startup, port, nullptr, nullptr);
        test.expect_true(
            result.status == LegacyBattleRuntimeShutdownStatus::completed &&
                port.release_order.empty() &&
                std::ranges::all_of(
                    result.group_a_resource_cleanups,
                    [](const auto& cleanup) {
                        return cleanup.status ==
                            LegacyBattleGroupAResourceCleanupStatus::
                                completed &&
                            !cleanup.primary_resource_released &&
                            !cleanup.secondary_resource_released;
                    }

                ) &&
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
            "zero party and enemy pointers preserve stale bytes and never require storage releases"
        );
    }
}
