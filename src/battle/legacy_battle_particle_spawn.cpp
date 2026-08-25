#include "openswd3/battle/legacy_battle_particle_spawn.hpp"

#include <bit>

namespace openswd3::battle {
namespace {

[[nodiscard]] compat::i32
wrapping_add(const compat::i32 left, const compat::i32 right) noexcept {
    return std::bit_cast<compat::i32>(
        std::bit_cast<compat::u32>(left) + std::bit_cast<compat::u32>(right)
    );
}

[[nodiscard]] compat::i32
wrapping_subtract(const compat::i32 left, const compat::i32 right) noexcept {
    return std::bit_cast<compat::i32>(
        std::bit_cast<compat::u32>(left) - std::bit_cast<compat::u32>(right)
    );
}

[[nodiscard]] compat::i32
wrapping_multiply(const compat::i32 left, const compat::i32 right) noexcept {
    return std::bit_cast<compat::i32>(
        std::bit_cast<compat::u32>(left) * std::bit_cast<compat::u32>(right)
    );
}

[[nodiscard]] compat::u32 integer_square_root(compat::u32 value) noexcept {
    compat::u32 root{};
    compat::u32 bit = 1U << 30U;
    while (bit > value) {
        bit >>= 2U;
    }
    while (bit != 0U) {
        if (value >= root + bit) {
            value -= root + bit;
            root = (root >> 1U) + bit;
        } else {
            root >>= 1U;
        }
        bit >>= 2U;
    }
    return root;
}

[[nodiscard]] bool source_pixel_at(
    const std::span<compat::u16> pixels,
    const compat::u32 index,
    compat::u16& value
) noexcept {
    if (index >= pixels.size()) {
        return false;
    }
    value = pixels[index];
    return true;
}

}  // namespace

compat::u32 LegacyBattleImageParticleNodePool::allocate_zeroed() {
    if (successful_allocations_before_failure_.has_value()) {
        if (*successful_allocations_before_failure_ == 0U) {
            return 0U;
        }
        --*successful_allocations_before_failure_;
    }
    for (std::size_t index = 0; index < slots_.size(); ++index) {
        Slot& slot = slots_[index];
        if (!slot.active) {
            slot.node = LegacyBattleImageParticleNode{};
            slot.active = true;
            return static_cast<compat::u32>(index + 1U);
        }
    }
    slots_.push_back(Slot{.active = true});
    return static_cast<compat::u32>(slots_.size());
}

bool LegacyBattleImageParticleNodePool::release(
    const compat::u32 token
) noexcept {
    if (token == 0U || token > slots_.size()) {
        return false;
    }
    Slot& slot = slots_[token - 1U];
    if (!slot.active) {
        return false;
    }
    slot.node = LegacyBattleImageParticleNode{};
    slot.active = false;
    return true;
}

LegacyBattleImageParticleNode*
LegacyBattleImageParticleNodePool::node(const compat::u32 token) noexcept {
    if (token == 0U || token > slots_.size()) {
        return nullptr;
    }
    Slot& slot = slots_[token - 1U];
    return slot.active ? &slot.node : nullptr;
}

const LegacyBattleImageParticleNode* LegacyBattleImageParticleNodePool::node(
    const compat::u32 token
) const noexcept {
    if (token == 0U || token > slots_.size()) {
        return nullptr;
    }
    const Slot& slot = slots_[token - 1U];
    return slot.active ? &slot.node : nullptr;
}

std::size_t LegacyBattleImageParticleNodePool::active_count() const noexcept {
    std::size_t count{};
    for (const Slot& slot : slots_) {
        if (slot.active) {
            ++count;
        }
    }
    return count;
}

void LegacyBattleImageParticleNodePool::clear() noexcept {
    slots_.clear();
}

void LegacyBattleImageParticleNodePool::fail_after_successful_allocations(
    const std::size_t count
) noexcept {
    successful_allocations_before_failure_ = count;
}

void LegacyBattleImageParticleNodePool::disable_allocation_failure() noexcept {
    successful_allocations_before_failure_.reset();
}

LegacyBattleImageParticleSpawnResult spawn_legacy_battle_image_particles(
    LegacyBattleImageParticleEmitter& emitter,
    const compat::i32 attempt_count,
    const LegacyBattleImageParticleStackSnapshot& stack_snapshot,
    LegacyBattleImageParticleNodePool& nodes,
    input_time_rng::LegacyCrtRng& rng,
    LegacyBattleImageParticleSharedState& shared,
    LegacyBattleImageParticleDiagnostics& diagnostics
) noexcept {
    LegacyBattleImageParticleSpawnResult result;

    if (shared.random_modulus == 0) {
        shared.random_modulus = emitter.shared_modulus_increment;
    }

    const std::span<compat::u16> source_pixels = emitter.source_pixels;
    const compat::i32 source_origin_x = emitter.source_origin_x;
    const compat::i32 source_origin_y = emitter.source_origin_y;

    compat::u32 current_token{};
    if (emitter.head_token == 0U) {
        current_token = nodes.allocate_zeroed();
        if (current_token == 0U) {
            ++diagnostics.initial_allocation_failures;
            result.status =
                LegacyBattleImageParticleSpawnStatus::initial_allocation_failed;
            return result;
        }
        emitter.head_token = current_token;
    } else {
        current_token = emitter.tail_token;
    }

    compat::i32 distance_source_x = attempt_count;
    compat::i32 distance_source_y = stack_snapshot.stale_source_y;

    if (attempt_count > 0) {
        compat::i32 attempt_index{};
        while (true) {
            const compat::i32 first_random =
                static_cast<compat::i32>(rng.next());
            const compat::i32 random_mod_28 = first_random % 28;
            const compat::i32 second_random =
                static_cast<compat::i32>(rng.next());
            const compat::i32 random_product =
                wrapping_multiply(second_random, random_mod_28);

            const compat::u8 flags = static_cast<compat::u8>(emitter.flags);
            compat::i32 source_index{};
            if ((flags & 0x40U) != 0U) {
                if (shared.random_modulus == 0) {
                    result.status = LegacyBattleImageParticleSpawnStatus::
                        shared_modulus_zero;
                    return result;
                }
                source_index = random_product % shared.random_modulus;
            } else if ((flags & 0x80U) != 0U) {
                if (emitter.remaining_batches == 0U) {
                    result.status = LegacyBattleImageParticleSpawnStatus::
                        remaining_batch_divisor_zero;
                    return result;
                }
                const compat::i32 per_batch_modulus =
                    emitter.source_pixel_count /
                    static_cast<compat::i32>(emitter.remaining_batches);
                if (per_batch_modulus == 0) {
                    result.status = LegacyBattleImageParticleSpawnStatus::
                        per_batch_modulus_zero;
                    return result;
                }
                source_index = random_product % per_batch_modulus;
            } else {
                if (emitter.source_pixel_count == 0) {
                    result.status = LegacyBattleImageParticleSpawnStatus::
                        source_pixel_count_zero;
                    return result;
                }
                source_index = random_product % emitter.source_pixel_count;
            }

            const compat::u32 source_index_bits =
                std::bit_cast<compat::u32>(source_index);
            compat::u16 selected_pixel{};
            if (!source_pixel_at(
                    source_pixels, source_index_bits, selected_pixel
                )) {
                result.status = LegacyBattleImageParticleSpawnStatus::
                    source_pixel_out_of_range;
                return result;
            }

            if (selected_pixel == shared.first_transparent_color ||
                selected_pixel == shared.second_transparent_color) {
                ++result.transparent_skips;
            } else {
                emitter.spawned_count = wrapping_add(emitter.spawned_count, 1);

                LegacyBattleImageParticleNode* current{};
                if ((flags & 0x01U) != 0U) {
                    if (emitter.source_width == 0U) {
                        result.status = LegacyBattleImageParticleSpawnStatus::
                            source_width_zero;
                        return result;
                    }
                    const compat::u32 source_column =
                        source_index_bits % emitter.source_width;
                    const compat::u32 source_row =
                        source_index_bits / emitter.source_width;
                    distance_source_x = wrapping_add(
                        source_origin_x,
                        static_cast<compat::i32>(
                            emitter.source_width - source_column
                        )
                    );
                    distance_source_y = wrapping_add(
                        source_origin_y, static_cast<compat::i32>(source_row)
                    );
                    current = nodes.node(current_token);
                    if (current == nullptr) {
                        result.status = LegacyBattleImageParticleSpawnStatus::
                            current_node_out_of_range;
                        return result;
                    }
                    current->source_x = distance_source_x;
                    current->source_y = distance_source_y;
                    current->current_x = distance_source_x;
                    current->current_y = distance_source_y;
                } else if ((flags & 0x80U) == 0U) {
                    if (emitter.source_width == 0U) {
                        result.status = LegacyBattleImageParticleSpawnStatus::
                            source_width_zero;
                        return result;
                    }
                    const compat::u32 source_column =
                        source_index_bits % emitter.source_width;
                    const compat::u32 source_row =
                        source_index_bits / emitter.source_width;
                    distance_source_x = wrapping_add(
                        source_origin_x, static_cast<compat::i32>(source_column)
                    );
                    distance_source_y = wrapping_add(
                        source_origin_y, static_cast<compat::i32>(source_row)
                    );
                    current = nodes.node(current_token);
                    if (current == nullptr) {
                        result.status = LegacyBattleImageParticleSpawnStatus::
                            current_node_out_of_range;
                        return result;
                    }
                    current->source_x = distance_source_x;
                    current->source_y = distance_source_y;
                    current->current_x = distance_source_x;
                    current->current_y = distance_source_y;
                }

                const compat::i32 target_x_random =
                    static_cast<compat::i32>(rng.next());
                if (emitter.target_width == 0) {
                    result.status =
                        LegacyBattleImageParticleSpawnStatus::target_width_zero;
                    return result;
                }
                const compat::i32 target_x = wrapping_add(
                    emitter.target_origin_x,
                    target_x_random % emitter.target_width
                );
                if (current == nullptr) {
                    current = nodes.node(current_token);
                    if (current == nullptr) {
                        result.status = LegacyBattleImageParticleSpawnStatus::
                            current_node_out_of_range;
                        return result;
                    }
                }
                current->target_x = target_x;

                const compat::i32 target_y_random =
                    static_cast<compat::i32>(rng.next());
                if (emitter.target_height == 0) {
                    result.status = LegacyBattleImageParticleSpawnStatus::
                        target_height_zero;
                    return result;
                }
                const compat::i32 target_y = wrapping_add(
                    emitter.target_origin_y,
                    target_y_random % emitter.target_height
                );
                current->target_y = target_y;

                const compat::i32 x_delta =
                    wrapping_subtract(target_x, distance_source_x);
                const compat::i32 y_delta =
                    wrapping_subtract(target_y, distance_source_y);
                compat::u32 squared_distance =
                    std::bit_cast<compat::u32>(wrapping_add(
                        wrapping_multiply(x_delta, x_delta),
                        wrapping_multiply(y_delta, y_delta)
                    ));
                if (squared_distance == 0U) {
                    squared_distance = 1U;
                }

                const compat::u32 distance_random = rng.next();
                const compat::u32 distance =
                    integer_square_root(squared_distance);
                current->distance_offset = std::bit_cast<compat::i32>(
                    distance_random % distance + emitter.distance_offset_base
                );

                const compat::i32 lifetime_random =
                    static_cast<compat::i32>(rng.next());
                if (emitter.lifetime_divisor == 0U) {
                    result.status = LegacyBattleImageParticleSpawnStatus::
                        lifetime_divisor_zero;
                    return result;
                }
                current->random_lifetime =
                    lifetime_random % emitter.lifetime_divisor + 1;

                const compat::u32 second_index = source_index_bits + 1U;
                const compat::u32 lower_index =
                    source_index_bits + emitter.source_width;
                const compat::u32 lower_second_index = lower_index + 1U;
                compat::u16 source_pixel{};
                if (!source_pixel_at(
                        source_pixels, source_index_bits, source_pixel
                    )) {
                    result.status = LegacyBattleImageParticleSpawnStatus::
                        source_pixel_out_of_range;
                    return result;
                }
                current->saved_pixels[0U] = source_pixel;
                if (!source_pixel_at(
                        source_pixels, second_index, source_pixel
                    )) {
                    result.status = LegacyBattleImageParticleSpawnStatus::
                        source_pixel_out_of_range;
                    return result;
                }
                current->saved_pixels[1U] = source_pixel;
                if (!source_pixel_at(
                        source_pixels, lower_index, source_pixel
                    )) {
                    result.status = LegacyBattleImageParticleSpawnStatus::
                        source_pixel_out_of_range;
                    return result;
                }
                current->saved_pixels[2U] = source_pixel;
                if (!source_pixel_at(
                        source_pixels, lower_second_index, source_pixel
                    )) {
                    result.status = LegacyBattleImageParticleSpawnStatus::
                        source_pixel_out_of_range;
                    return result;
                }
                current->saved_pixels[3U] = source_pixel;

                source_pixels[source_index_bits] =
                    shared.first_transparent_color;
                source_pixels[second_index] = shared.first_transparent_color;
                source_pixels[lower_index] = shared.first_transparent_color;
                source_pixels[lower_second_index] =
                    shared.first_transparent_color;
                ++result.particles_initialized;

                const compat::u32 successor_token = nodes.allocate_zeroed();
                current = nodes.node(current_token);
                if (current == nullptr) {
                    result.status = LegacyBattleImageParticleSpawnStatus::
                        current_node_out_of_range;
                    return result;
                }
                current->next_token = successor_token;
                if (successor_token == 0U) {
                    ++diagnostics.successor_allocation_failures;
                    result.status = LegacyBattleImageParticleSpawnStatus::
                        successor_allocation_failed;
                    return result;
                }
                LegacyBattleImageParticleNode* const successor =
                    nodes.node(successor_token);
                successor->previous_token = current_token;
                successor->next_token = 0U;
                current_token = successor_token;
                emitter.tail_token = successor->next_token;
            }

            ++attempt_index;
            ++result.attempts_completed;
            if (attempt_index >= attempt_count) {
                break;
            }
        }
    }

    emitter.remaining_batches =
        static_cast<compat::u16>(emitter.remaining_batches - 1U);
    shared.random_modulus =
        wrapping_add(shared.random_modulus, emitter.shared_modulus_increment);
    if (emitter.remaining_batches == 0U) {
        shared.random_modulus = 0;
    }
    return result;
}

}  // namespace openswd3::battle
