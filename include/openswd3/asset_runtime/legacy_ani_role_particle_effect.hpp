#pragma once

#include "openswd3/asset_runtime/legacy_action_record.hpp"
#include "openswd3/asset_runtime/legacy_tsw_runtime.hpp"
#include "openswd3/compat/types.hpp"
#include "openswd3/rendering/legacy_blitter.hpp"
#include "openswd3/rendering/legacy_framebuffer.hpp"
#include "openswd3/rendering/legacy_tiled_frame.hpp"

#include <array>
#include <cstddef>
#include <vector>

namespace openswd3::input_time_rng {
class LegacySecondaryRng;
}

namespace openswd3::asset_runtime {

inline constexpr compat::u32 kLegacyAniRoleParticleActionId = 0x232BU;
inline constexpr compat::u32 kLegacyAniRoleParticleVariant = 0x3BU;
inline constexpr std::size_t kLegacyAniRoleParticleEmitterCount = 4U;
inline constexpr compat::i32 kLegacyAniRoleParticleSpecialMapId = 0x32;

// The original 32-bit allocation is exactly 0x10 bytes. Host-independent
// one-based tokens preserve its link field without widening it to a host
// pointer on 64-bit builds.
struct LegacyAniRoleParticleNode {
    compat::u32 next_token{};
    compat::i16 fixed_x_1_16{};
    compat::i16 world_y{};
    compat::i16 horizontal_step_1_16{};
    compat::i16 vertical_step{};
    compat::i16 lifetime{};
    compat::i16 reserved{};
};

static_assert(sizeof(LegacyAniRoleParticleNode) == 0x10U);
static_assert(offsetof(LegacyAniRoleParticleNode, fixed_x_1_16) == 0x04U);
static_assert(offsetof(LegacyAniRoleParticleNode, world_y) == 0x06U);
static_assert(offsetof(LegacyAniRoleParticleNode, lifetime) == 0x0CU);

struct LegacyAniRoleParticleEmitter {
    compat::u32 head_token{};
    compat::i16 world_x{};
    compat::i16 world_y{};
    compat::i16 field_08{};
    compat::i16 flags{};
    compat::i16 role_selector{};
    compat::i16 reserved{};
};

static_assert(sizeof(LegacyAniRoleParticleEmitter) == 0x10U);
static_assert(offsetof(LegacyAniRoleParticleEmitter, world_x) == 0x04U);
static_assert(offsetof(LegacyAniRoleParticleEmitter, flags) == 0x0AU);
static_assert(offsetof(LegacyAniRoleParticleEmitter, role_selector) == 0x0CU);

class LegacyAniRoleParticleNodePool final {
public:
    [[nodiscard]] compat::u32 allocate_zeroed();
    void release(compat::u32 token) noexcept;
    void clear() noexcept;

    [[nodiscard]] LegacyAniRoleParticleNode* node(compat::u32 token) noexcept;
    [[nodiscard]] const LegacyAniRoleParticleNode*
    node(compat::u32 token) const noexcept;
    [[nodiscard]] std::size_t active_count() const noexcept;

private:
    struct Slot {
        LegacyAniRoleParticleNode node{};
        bool active{};
    };

    std::vector<Slot> slots_;
    std::size_t active_count_{};
};

struct LegacyAniRoleParticleViewport {
    compat::i32 left{};
    compat::i32 top{};
    compat::i32 right{};
    compat::i32 bottom{};
};

class LegacyAniRoleParticlePositionPort {
public:
    virtual ~LegacyAniRoleParticlePositionPort() = default;

    [[nodiscard]] virtual bool resolve_role_position(
        compat::u16 role_selector, compat::i16& world_x, compat::i16& world_y
    ) = 0;
};

class LegacyAniRoleParticlePorts {
public:
    virtual ~LegacyAniRoleParticlePorts() = default;

    [[nodiscard]] virtual LegacyActionUpdateStatus
    update_action_record(LegacyActionRecord& record) = 0;
    [[nodiscard]] virtual bool load_frame_piece(
        compat::u16 resource_id,
        compat::u16 frame_index,
        rendering::LegacyFramePiece& piece
    ) = 0;
    [[nodiscard]] virtual rendering::LegacyBlitExecutionStatus draw_frame_piece(
        const rendering::LegacyFramePiece& piece,
        compat::i32 destination_x,
        compat::i32 destination_y,
        compat::u32 flags,
        compat::i32 red_offset,
        compat::i32 green_offset,
        compat::i32 blue_offset
    ) noexcept = 0;
};

class LegacyAniRoleParticleRuntimePorts final
    : public LegacyAniRoleParticlePorts {
public:
    LegacyAniRoleParticleRuntimePorts(
        LegacyActionUpdater& action_updater,
        LegacyTswRuntime& tsw_runtime,
        rendering::LegacyFramebuffer& framebuffer,
        rendering::LegacyRasterGeometryState& raster,
        rendering::LegacyBlitEffectState& effects,
        rendering::LegacyRleRowJitterState& jitter
    ) noexcept;

    [[nodiscard]] LegacyActionUpdateStatus
    update_action_record(LegacyActionRecord& record) override;
    [[nodiscard]] bool load_frame_piece(
        compat::u16 resource_id,
        compat::u16 frame_index,
        rendering::LegacyFramePiece& piece
    ) override;
    [[nodiscard]] rendering::LegacyBlitExecutionStatus draw_frame_piece(
        const rendering::LegacyFramePiece& piece,
        compat::i32 destination_x,
        compat::i32 destination_y,
        compat::u32 flags,
        compat::i32 red_offset,
        compat::i32 green_offset,
        compat::i32 blue_offset
    ) noexcept override;

private:
    LegacyActionUpdater& action_updater_;
    LegacyTswRuntime& tsw_runtime_;
    rendering::LegacyFramebuffer& framebuffer_;
    rendering::LegacyRasterGeometryState& raster_;
    rendering::LegacyBlitEffectState& effects_;
    rendering::LegacyRleRowJitterState& jitter_;
};

enum class LegacyAniRoleParticleStatus {
    ready,
    culled,
    action_update_failed,
    frame_load_failed,
    role_lookup_failed,
    allocation_failed,
    corrupt_node_link,
};

struct LegacyAniRoleParticleResult {
    LegacyAniRoleParticleStatus status{LegacyAniRoleParticleStatus::ready};
    compat::u32 random_call_count{};
    compat::u32 matching_emitter_count{};
    compat::u32 role_query_count{};
    compat::u32 spawned_node_count{};
    compat::u32 updated_node_count{};
    compat::u32 draw_count{};
    compat::u32 removed_node_count{};
    compat::u32 copied_successor_count{};
    compat::u32 blit_failure_count{};
    rendering::LegacyBlitExecutionStatus last_blit_status{
        rendering::LegacyBlitExecutionStatus::completed
    };
};

class LegacyAniRoleParticleEffect final {
public:
    // 0x0040F630 frees all four lists, then zeros the full 0x40-byte emitter
    // block.
    void reset() noexcept;

    [[nodiscard]] LegacyAniRoleParticleResult update(
        compat::i32 role_world_x,
        compat::i32 role_world_y,
        compat::u16 role_selector,
        compat::i32 legacy_map_id,
        const LegacyAniRoleParticleViewport& viewport,
        input_time_rng::LegacySecondaryRng& random,
        LegacyAniRoleParticlePositionPort& positions,
        LegacyActionRecord& shared_action_record,
        LegacyAniRoleParticlePorts& ports
    );

    [[nodiscard]] std::
        array<LegacyAniRoleParticleEmitter, kLegacyAniRoleParticleEmitterCount>&
        emitters() noexcept;
    [[nodiscard]] const std::
        array<LegacyAniRoleParticleEmitter, kLegacyAniRoleParticleEmitterCount>&
        emitters() const noexcept;
    [[nodiscard]] LegacyAniRoleParticleNodePool& nodes() noexcept;
    [[nodiscard]] const LegacyAniRoleParticleNodePool& nodes() const noexcept;

private:
    std::array<LegacyAniRoleParticleEmitter, kLegacyAniRoleParticleEmitterCount>
        emitters_{};
    LegacyAniRoleParticleNodePool nodes_;
};

}  // namespace openswd3::asset_runtime
