#include "openswd3/asset_runtime/legacy_ani_role_particle_effect.hpp"

#include "openswd3/input_time_rng/legacy_secondary_rng.hpp"

#include <bit>
#include <limits>

namespace openswd3::asset_runtime {
namespace {

using compat::i16;
using compat::i32;
using compat::u16;
using compat::u32;

constexpr i32 kCullMargin = 64;
constexpr i16 kSpawnFieldLimit = 24;

[[nodiscard]] constexpr i32
wrapping_add(const i32 left, const i32 right) noexcept {
    return std::bit_cast<i32>(
        std::bit_cast<u32>(left) + std::bit_cast<u32>(right)
    );
}

[[nodiscard]] constexpr i32
wrapping_subtract(const i32 left, const i32 right) noexcept {
    return std::bit_cast<i32>(
        std::bit_cast<u32>(left) - std::bit_cast<u32>(right)
    );
}

[[nodiscard]] constexpr i16
wrapping_add(const i16 left, const i16 right) noexcept {
    return std::bit_cast<i16>(static_cast<u16>(
        static_cast<u32>(std::bit_cast<u16>(left)) +
        static_cast<u32>(std::bit_cast<u16>(right))
    ));
}

[[nodiscard]] constexpr i16 from_low_word(const u32 value) noexcept {
    return std::bit_cast<i16>(static_cast<u16>(value));
}

[[nodiscard]] constexpr i32 field_as_i32(const u32 value) noexcept {
    return std::bit_cast<i32>(value);
}

[[nodiscard]] constexpr bool accepted_blit_status(
    const rendering::LegacyBlitExecutionStatus status
) noexcept {
    return status == rendering::LegacyBlitExecutionStatus::completed ||
        status == rendering::LegacyBlitExecutionStatus::clipped_out ||
        status == rendering::LegacyBlitExecutionStatus::opacity_disabled;
}

[[nodiscard]] rendering::LegacyBlitClipRectangle
current_clip(const rendering::LegacyRasterGeometryState& raster) noexcept {
    return rendering::LegacyBlitClipRectangle{
        .left = raster.clip_left,
        .top = raster.clip_top,
        .width = raster.clip_width,
        .height = raster.clip_height,
    };
}

[[nodiscard]] bool outside_cull_rectangle(
    const i32 world_x,
    const i32 world_y,
    const LegacyAniRoleParticleViewport& viewport
) noexcept {
    return world_x <= wrapping_subtract(viewport.left, kCullMargin) ||
        world_x >= wrapping_add(viewport.right, kCullMargin) ||
        world_y <= wrapping_subtract(viewport.top, kCullMargin) ||
        world_y >= wrapping_add(viewport.bottom, kCullMargin);
}

[[nodiscard]] u32 random_bounded(
    input_time_rng::LegacySecondaryRng& random,
    const u32 upper_bound,
    LegacyAniRoleParticleResult& result
) noexcept {
    ++result.random_call_count;
    return random.next_bounded(upper_bound);
}

void set_flag_bit_zero(
    LegacyAniRoleParticleEmitter& emitter, const bool value
) noexcept {
    u16 flags = std::bit_cast<u16>(emitter.flags);
    flags &= 0xFFFEU;
    if (value) {
        flags |= 1U;
    }
    emitter.flags = std::bit_cast<i16>(flags);
}

[[nodiscard]] bool
flag_bit_zero(const LegacyAniRoleParticleEmitter& emitter) noexcept {
    return (std::bit_cast<u16>(emitter.flags) & 1U) != 0U;
}

void color_offsets(
    const i16 lifetime,
    const i32 legacy_map_id,
    i32& red_offset,
    i32& green_offset,
    i32& blue_offset
) noexcept {
    const i32 life = lifetime;
    if (legacy_map_id == kLegacyAniRoleParticleSpecialMapId) {
        red_offset = life - 30;
        green_offset = life - 38;
        blue_offset = life - 50;
        return;
    }

    const i32 common = (life >> 1) - 30;
    red_offset = common;
    green_offset = common;
    blue_offset = common;
}

}  // namespace

u32 LegacyAniRoleParticleNodePool::allocate_zeroed() {
    for (std::size_t index = 0U; index < slots_.size(); ++index) {
        Slot& slot = slots_[index];
        if (!slot.active) {
            slot.node = LegacyAniRoleParticleNode{};
            slot.active = true;
            ++active_count_;
            return static_cast<u32>(index + 1U);
        }
    }

    if (slots_.size() >= std::numeric_limits<u32>::max()) {
        return 0U;
    }
    slots_.push_back(Slot{.active = true});
    ++active_count_;
    return static_cast<u32>(slots_.size());
}

void LegacyAniRoleParticleNodePool::release(const u32 token) noexcept {
    if (token == 0U || token > slots_.size()) {
        return;
    }
    Slot& slot = slots_[static_cast<std::size_t>(token - 1U)];
    if (!slot.active) {
        return;
    }
    slot.node = LegacyAniRoleParticleNode{};
    slot.active = false;
    --active_count_;
}

void LegacyAniRoleParticleNodePool::clear() noexcept {
    std::vector<Slot>{}.swap(slots_);
    active_count_ = 0U;
}

LegacyAniRoleParticleNode*
LegacyAniRoleParticleNodePool::node(const u32 token) noexcept {
    if (token == 0U || token > slots_.size()) {
        return nullptr;
    }
    Slot& slot = slots_[static_cast<std::size_t>(token - 1U)];
    return slot.active ? &slot.node : nullptr;
}

const LegacyAniRoleParticleNode*
LegacyAniRoleParticleNodePool::node(const u32 token) const noexcept {
    if (token == 0U || token > slots_.size()) {
        return nullptr;
    }
    const Slot& slot = slots_[static_cast<std::size_t>(token - 1U)];
    return slot.active ? &slot.node : nullptr;
}

std::size_t LegacyAniRoleParticleNodePool::active_count() const noexcept {
    return active_count_;
}

LegacyAniRoleParticleRuntimePorts::LegacyAniRoleParticleRuntimePorts(
    LegacyActionUpdater& action_updater,
    LegacyTswRuntime& tsw_runtime,
    rendering::LegacyFramebuffer& framebuffer,
    rendering::LegacyRasterGeometryState& raster,
    rendering::LegacyBlitEffectState& effects,
    rendering::LegacyRleRowJitterState& jitter
) noexcept
    : action_updater_(action_updater), tsw_runtime_(tsw_runtime),
      framebuffer_(framebuffer), raster_(raster), effects_(effects),
      jitter_(jitter) {}

LegacyActionUpdateStatus
LegacyAniRoleParticleRuntimePorts::update_action_record(
    LegacyActionRecord& record
) {
    return action_updater_.update(record).status;
}

bool LegacyAniRoleParticleRuntimePorts::load_frame_piece(
    const u16 resource_id,
    const u16 frame_index,
    rendering::LegacyFramePiece& piece
) {
    const LegacyTswQueryResult loaded =
        tsw_runtime_.query_cached(resource_id, frame_index);
    if (loaded.status != LegacyTswRuntimeStatus::ready) {
        piece = rendering::LegacyFramePiece{};
        return false;
    }

    piece = rendering::LegacyFramePiece{
        .source =
            rendering::LegacyBlitSource{
                .bytes = loaded.frame.primary_stream,
                .layout = rendering::LegacyBlitSourceLayout::direct_16,
                .palette = {},
            },
        .width = loaded.frame.width,
        .height = loaded.frame.height,
    };
    return true;
}

rendering::LegacyBlitExecutionStatus
LegacyAniRoleParticleRuntimePorts::draw_frame_piece(
    const rendering::LegacyFramePiece& piece,
    const i32 destination_x,
    const i32 destination_y,
    const u32 flags,
    const i32 red_offset,
    const i32 green_offset,
    const i32 blue_offset
) noexcept {
    effects_.red_offset = red_offset;
    effects_.green_offset = green_offset;
    effects_.blue_offset = blue_offset;
    return rendering::blit_legacy_copy_paths(
               framebuffer_,
               current_clip(raster_),
               piece.source,
               rendering::LegacyBlitRequest{
                   .destination_x = destination_x,
                   .destination_y = destination_y,
                   .source_width = piece.width,
                   .source_height = piece.height,
                   .target_height = piece.height,
                   .flags = flags,
               },
               effects_,
               jitter_
    )
        .status;
}

LegacyAniRoleParticleReleaseResult
LegacyAniRoleParticleEffect::release() noexcept {
    LegacyAniRoleParticleReleaseResult result;
    for (std::size_t emitter_index = 0U; emitter_index < emitters_.size();
         ++emitter_index) {
        LegacyAniRoleParticleEmitter& emitter = emitters_[emitter_index];
        while (emitter.head_token != 0U) {
            const u32 token = emitter.head_token;
            const LegacyAniRoleParticleNode* const node = nodes_.node(token);
            if (node == nullptr) {
                ++result.corrupt_link_count;
                break;
            }
            emitter.head_token = node->next_token;
            nodes_.release(token);
            ++result.released_per_emitter[emitter_index];
            ++result.released_node_count;
        }
    }
    result.orphaned_node_count = static_cast<u32>(nodes_.active_count());
    nodes_.clear();
    emitters_.fill(LegacyAniRoleParticleEmitter{});
    return result;
}

void LegacyAniRoleParticleEffect::reset() noexcept {
    static_cast<void>(release());
}

LegacyAniRoleParticleResult LegacyAniRoleParticleEffect::update(
    const i32 role_world_x,
    const i32 role_world_y,
    const u16 role_selector,
    const i32 legacy_map_id,
    const LegacyAniRoleParticleViewport& viewport,
    input_time_rng::LegacySecondaryRng& random,
    LegacyAniRoleParticlePositionPort& positions,
    LegacyActionRecord& shared_action_record,
    LegacyAniRoleParticlePorts& ports
) {
    LegacyAniRoleParticleResult result;
    if (outside_cull_rectangle(role_world_x, role_world_y, viewport)) {
        result.status = LegacyAniRoleParticleStatus::culled;
        return result;
    }

    const i16 frame_horizontal_drift = from_low_word(
        random_bounded(random, 5U, result) - static_cast<u32>(3U)
    );

    shared_action_record.action_id = kLegacyAniRoleParticleActionId;
    shared_action_record.base_variant = kLegacyAniRoleParticleVariant;
    if (ports.update_action_record(shared_action_record) !=
        LegacyActionUpdateStatus::completed) {
        result.status = LegacyAniRoleParticleStatus::action_update_failed;
        return result;
    }

    rendering::LegacyFramePiece piece;
    if (!ports.load_frame_piece(
            shared_action_record.field_4a, shared_action_record.field_4c, piece
        )) {
        result.status = LegacyAniRoleParticleStatus::frame_load_failed;
        return result;
    }

    for (LegacyAniRoleParticleEmitter& emitter : emitters_) {
        if (static_cast<i32>(emitter.role_selector) !=
            static_cast<i32>(role_selector)) {
            continue;
        }
        ++result.matching_emitter_count;

        i16 current_role_x{};
        i16 current_role_y{};
        ++result.role_query_count;
        if (!positions.resolve_role_position(
                role_selector, current_role_x, current_role_y
            )) {
            result.status = LegacyAniRoleParticleStatus::role_lookup_failed;
            return result;
        }
        set_flag_bit_zero(emitter, false);
        emitter.world_x = current_role_x;
        emitter.world_y = current_role_y;

        if (random_bounded(random, 1000U, result) > 250U) {
            set_flag_bit_zero(emitter, true);
        }

        if (emitter.field_08 < kSpawnFieldLimit && flag_bit_zero(emitter)) {
            const u32 token = nodes_.allocate_zeroed();
            if (token == 0U) {
                result.status = LegacyAniRoleParticleStatus::allocation_failed;
                return result;
            }
            LegacyAniRoleParticleNode* const spawned = nodes_.node(token);
            spawned->next_token = emitter.head_token;
            emitter.head_token = token;
            spawned->world_y = emitter.world_y;

            const u32 spawn_x = random_bounded(random, 16U, result) +
                static_cast<u32>(std::bit_cast<u16>(emitter.world_x)) - 8U;
            spawned->fixed_x_1_16 = from_low_word(spawn_x << 4U);
            if (legacy_map_id == kLegacyAniRoleParticleSpecialMapId) {
                spawned->lifetime =
                    static_cast<i16>(random_bounded(random, 16U, result) + 16U);
            } else {
                spawned->lifetime =
                    static_cast<i16>(random_bounded(random, 30U, result) + 30U);
            }
            spawned->horizontal_step_1_16 = from_low_word(
                random_bounded(random, 16U, result) - static_cast<u32>(8U)
            );
            spawned->vertical_step = from_low_word(
                std::numeric_limits<u32>::max() -
                random_bounded(random, 1U, result)
            );
            ++result.spawned_node_count;
        }

        u32* link = &emitter.head_token;
        u32 token = *link;
        while (token != 0U) {
            LegacyAniRoleParticleNode* node = nodes_.node(token);
            if (node == nullptr) {
                result.status = LegacyAniRoleParticleStatus::corrupt_node_link;
                return result;
            }

            const u32 lifetime_roll = random_bounded(random, 1000U, result);
            const i16 horizontal_step = wrapping_add(
                node->horizontal_step_1_16, frame_horizontal_drift
            );
            node->world_y = wrapping_add(node->world_y, node->vertical_step);
            node->fixed_x_1_16 =
                wrapping_add(node->fixed_x_1_16, horizontal_step);
            if (lifetime_roll > 50U) {
                node->lifetime = wrapping_add(node->lifetime, i16{-1});
            }
            ++result.updated_node_count;

            if (node->lifetime <= 0) {
                const u32 successor_token = node->next_token;
                if (successor_token != 0U) {
                    const LegacyAniRoleParticleNode* const successor =
                        nodes_.node(successor_token);
                    if (successor == nullptr) {
                        result.status =
                            LegacyAniRoleParticleStatus::corrupt_node_link;
                        return result;
                    }
                    *node = *successor;
                    *link = token;
                    nodes_.release(successor_token);
                    ++result.removed_node_count;
                    ++result.copied_successor_count;
                    continue;
                }

                *link = 0U;
                nodes_.release(token);
                ++result.removed_node_count;
                break;
            }

            i32 red_offset{};
            i32 green_offset{};
            i32 blue_offset{};
            color_offsets(
                node->lifetime,
                legacy_map_id,
                red_offset,
                green_offset,
                blue_offset
            );
            const i32 destination_y = wrapping_subtract(
                wrapping_subtract(
                    static_cast<i32>(node->world_y),
                    field_as_i32(shared_action_record.draw_offset_y)
                ),
                viewport.top
            );
            const i32 destination_x = wrapping_subtract(
                wrapping_subtract(
                    static_cast<i32>(node->fixed_x_1_16) / 16,
                    field_as_i32(shared_action_record.draw_offset_x)
                ),
                viewport.left
            );
            result.last_blit_status = ports.draw_frame_piece(
                piece,
                destination_x,
                destination_y,
                shared_action_record.mode_flags,
                red_offset,
                green_offset,
                blue_offset
            );
            ++result.draw_count;
            if (!accepted_blit_status(result.last_blit_status)) {
                ++result.blit_failure_count;
            }

            link = &node->next_token;
            token = node->next_token;
        }
    }

    return result;
}

std::array<LegacyAniRoleParticleEmitter, kLegacyAniRoleParticleEmitterCount>&
LegacyAniRoleParticleEffect::emitters() noexcept {
    return emitters_;
}

const std::
    array<LegacyAniRoleParticleEmitter, kLegacyAniRoleParticleEmitterCount>&
    LegacyAniRoleParticleEffect::emitters() const noexcept {
    return emitters_;
}

LegacyAniRoleParticleNodePool& LegacyAniRoleParticleEffect::nodes() noexcept {
    return nodes_;
}

const LegacyAniRoleParticleNodePool&
LegacyAniRoleParticleEffect::nodes() const noexcept {
    return nodes_;
}

}  // namespace openswd3::asset_runtime
