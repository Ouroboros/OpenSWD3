#pragma once

#include "openswd3/compat/types.hpp"
#include "openswd3/input_time_rng/legacy_crt_rng.hpp"

#include <array>
#include <cstddef>
#include <optional>
#include <span>
#include <vector>

namespace openswd3::battle {

struct LegacyBattleImageParticleNode {
    std::array<compat::u16, 4> saved_pixels{};
    compat::i32 random_lifetime{};
    compat::i32 distance_offset{};
    compat::i32 source_x{};
    compat::i32 source_y{};
    compat::i32 target_x{};
    compat::i32 target_y{};
    compat::i32 current_x{};
    compat::i32 current_y{};
    compat::i32 reserved_28{};
    compat::i32 reserved_2c{};
    compat::u32 previous_token{};
    compat::u32 next_token{};
};
static_assert(sizeof(LegacyBattleImageParticleNode) == 0x38U);
static_assert(
    offsetof(LegacyBattleImageParticleNode, random_lifetime) == 0x08U
);
static_assert(offsetof(LegacyBattleImageParticleNode, source_x) == 0x10U);
static_assert(offsetof(LegacyBattleImageParticleNode, target_x) == 0x18U);
static_assert(offsetof(LegacyBattleImageParticleNode, current_x) == 0x20U);
static_assert(offsetof(LegacyBattleImageParticleNode, previous_token) == 0x30U);
static_assert(offsetof(LegacyBattleImageParticleNode, next_token) == 0x34U);

class LegacyBattleImageParticleNodePool final {
public:
    [[nodiscard]] compat::u32 allocate_zeroed();
    [[nodiscard]] bool release(compat::u32 token) noexcept;
    [[nodiscard]] LegacyBattleImageParticleNode*
    node(compat::u32 token) noexcept;
    [[nodiscard]] const LegacyBattleImageParticleNode*
    node(compat::u32 token) const noexcept;
    [[nodiscard]] std::size_t active_count() const noexcept;
    void clear() noexcept;

    void fail_after_successful_allocations(std::size_t count) noexcept;
    void disable_allocation_failure() noexcept;

private:
    struct Slot {
        LegacyBattleImageParticleNode node{};
        bool active{};
    };

    std::vector<Slot> slots_;
    std::optional<std::size_t> successful_allocations_before_failure_;
};

struct LegacyBattleImageParticleEmitter {
    std::span<compat::u16> source_pixels{};
    compat::u16 source_width{};
    compat::u16 source_height{};
    compat::i32 source_origin_x{};
    compat::i32 source_origin_y{};
    compat::i32 target_origin_x{};
    compat::i32 target_width{};
    compat::i32 target_origin_y{};
    compat::i32 target_height{};
    compat::u16 distance_offset_base{};
    compat::u16 lifetime_divisor{};
    compat::u16 remaining_batches{};
    compat::u16 spawn_divisor{};
    compat::u16 flags{};
    compat::i32 source_pixel_count{};
    compat::i32 spawned_count{};
    compat::i32 target_particle_count{};
    compat::i32 nontransparent_pixel_count{};
    compat::i32 shared_modulus_increment{};
    compat::u32 head_token{};
    compat::u32 tail_token{};
};

struct LegacyBattleImageParticleSharedState {
    compat::i32 random_modulus{};
    compat::u16 first_transparent_color{0x319FU};
    compat::u16 second_transparent_color{0x026BU};
};

struct LegacyBattleImageParticleStackSnapshot {
    compat::i32 stale_source_y{};
};

struct LegacyBattleImageParticleDiagnostics {
    compat::u32 initial_allocation_failures{};
    compat::u32 successor_allocation_failures{};
};

enum class LegacyBattleImageParticleSpawnStatus : compat::u8 {
    completed,
    initial_allocation_failed,
    successor_allocation_failed,
    current_node_out_of_range,
    shared_modulus_zero,
    remaining_batch_divisor_zero,
    per_batch_modulus_zero,
    source_pixel_count_zero,
    source_pixel_out_of_range,
    source_width_zero,
    target_width_zero,
    target_height_zero,
    lifetime_divisor_zero,
};

struct LegacyBattleImageParticleSpawnResult {
    LegacyBattleImageParticleSpawnStatus status{
        LegacyBattleImageParticleSpawnStatus::completed
    };
    compat::u32 attempts_completed{};
    compat::u32 transparent_skips{};
    compat::u32 particles_initialized{};
};

// sub_434DD0.
[[nodiscard]] LegacyBattleImageParticleSpawnResult
spawn_legacy_battle_image_particles(
    LegacyBattleImageParticleEmitter& emitter,
    compat::i32 attempt_count,
    const LegacyBattleImageParticleStackSnapshot& stack_snapshot,
    LegacyBattleImageParticleNodePool& nodes,
    input_time_rng::LegacyCrtRng& rng,
    LegacyBattleImageParticleSharedState& shared,
    LegacyBattleImageParticleDiagnostics& diagnostics
) noexcept;

}  // namespace openswd3::battle
