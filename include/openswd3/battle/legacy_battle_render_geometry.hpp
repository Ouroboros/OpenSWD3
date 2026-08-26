#pragma once

#include "openswd3/compat/types.hpp"
#include "openswd3/rendering/legacy_framebuffer.hpp"

#include <array>
#include <memory>

namespace openswd3::battle {

inline constexpr std::size_t kLegacyBattleDirectionCount = 360U;
inline constexpr compat::u32 kLegacyBattleRenderGeometryOwnerToken =
    0x0053B0B8U;
inline constexpr compat::u32 kLegacyBattleRenderGeometryExitCleanupToken =
    0x004518D0U;
inline constexpr compat::u32 kLegacyBattleRenderGeometryBindingObjectToken =
    0x004FF5B8U;

struct LegacyBattleDirectionVectors {
    std::array<compat::i32, kLegacyBattleDirectionCount> horizontal{};
    std::array<compat::i32, kLegacyBattleDirectionCount> vertical{};
};

struct LegacyBattleDirectionRaster {
    compat::i32 direction_index{};
    compat::i32 value_04{};
    compat::i32 value_08{};
    compat::i32 current_x{};
    compat::i32 current_y{};
    compat::i32 x_error{};
    compat::i32 y_error{};
};

enum class LegacyBattleDirectionStepStatus : compat::u8 {
    completed,
    direction_index_out_of_range,
};

struct LegacyBattleLineRaster {
    compat::i32 start_x{};
    compat::i32 start_y{};
    compat::i32 end_x{};
    compat::i32 end_y{};
    compat::i32 current_x{};
    compat::i32 current_y{};
    compat::i32 x_error{};
    compat::i32 y_error{};
};

struct LegacyBattleRenderGeometry {
    LegacyBattleDirectionVectors direction_vectors{};
    std::unique_ptr<compat::u32[]> primary_row_offsets{};
    std::unique_ptr<compat::u32[]> surface_row_offsets{};
    compat::i32 primary_row_stride{};
    compat::i32 primary_row_count{};
    compat::i32 surface_width{};
    compat::i32 surface_height{};
    compat::i32 left{};
    compat::i32 top{};
    compat::i32 right{};
    compat::i32 bottom{};
    compat::u32 auxiliary_buffer_token{};
};

struct LegacyBattleRowOffsetAllocation {
    std::unique_ptr<compat::u32[]> words{};
    compat::u32 word_capacity{};
};

class LegacyBattleRenderAuxiliaryBufferReleaser {
public:
    virtual ~LegacyBattleRenderAuxiliaryBufferReleaser() = default;
    virtual void release(compat::u32 token) noexcept = 0;
};

class LegacyBattleRenderGeometryBindingObjectInitializationPort {
public:
    virtual ~LegacyBattleRenderGeometryBindingObjectInitializationPort() =
        default;

    [[nodiscard]] virtual compat::u32 initialize_binding_object(
        compat::u32 binding_object_token,
        compat::u32 render_geometry_owner_token
    ) = 0;
};

class LegacyBattleRenderGeometryExitRegistrationPort {
public:
    virtual ~LegacyBattleRenderGeometryExitRegistrationPort() = default;

    [[nodiscard]] virtual compat::u32
    register_exit_cleanup(compat::u32 cleanup_token) = 0;
};

class LegacyBattleRowOffsetAllocator {
public:
    virtual ~LegacyBattleRowOffsetAllocator() = default;
    [[nodiscard]] virtual LegacyBattleRowOffsetAllocation
    allocate(compat::u32 requested_bytes) noexcept = 0;
};

enum class LegacyBattleRowOffsetStatus : compat::u8 {
    completed,
    allocation_failed,
    write_out_of_range,
};

struct LegacyBattleRowOffsetResult {
    LegacyBattleRowOffsetStatus status{LegacyBattleRowOffsetStatus::completed};
    compat::u32 requested_bytes{};
    compat::u32 legacy_return_value{};
};

struct LegacyBattleHostSurfaceResult {
    LegacyBattleRowOffsetResult row_offsets{};
    bool rectangle_published{};
    compat::i32 legacy_return_value{};
};

enum class LegacyBattleRenderInitializationStatus : compat::u8 {
    completed,
    primary_row_offsets_write_out_of_range,
    surface_row_offsets_write_out_of_range,
};

struct LegacyBattleRenderCleanupResult {
    bool auxiliary_buffer_released{};
    bool surface_row_offsets_released{};
    bool primary_row_offsets_released{};
};

enum class LegacyBattleRenderSurfaceRebuildStatus : compat::u8 {
    completed,
    surface_row_offsets_write_out_of_range,
    primary_row_offsets_write_out_of_range,
};

struct LegacyBattleRenderSurfaceRebuildResult {
    LegacyBattleRenderSurfaceRebuildStatus status{
        LegacyBattleRenderSurfaceRebuildStatus::completed
    };
    rendering::LegacySurfacePitchAndHeight source{};
    LegacyBattleRowOffsetResult surface_row_offsets{};
    bool rectangle_published{};
    LegacyBattleRowOffsetResult primary_row_offsets{};
};

struct LegacyBattleRenderInitializationResult {
    LegacyBattleRenderInitializationStatus status{
        LegacyBattleRenderInitializationStatus::completed
    };
    LegacyBattleRowOffsetResult primary_row_offsets{};
    LegacyBattleRowOffsetResult surface_row_offsets{};
    bool rectangle_published{};
    bool direction_vectors_published{};
    LegacyBattleRenderGeometry* legacy_return_value{};
};

struct LegacyBattleRenderGeometryBindingInitializationResult {
    compat::u32 binding_object_token{};
    compat::u32 render_geometry_owner_token{};
    compat::u32 initialization_calls{};
    compat::u32 return_value{};
};

struct LegacyBattleRenderGeometryStaticInitializationResult {
    compat::u32 owner_token{};
    LegacyBattleRenderInitializationResult initialization{};
    compat::u32 initialization_calls{};
    compat::u32 exit_registration_calls{};
    compat::u32 return_value{};
};

struct LegacyBattleRenderGeometryStaticCleanupResult {
    compat::u32 owner_token{};
    LegacyBattleRenderCleanupResult cleanup{};
    compat::u32 cleanup_calls{};
};

// sub_434350.
[[nodiscard]] bool
advance_legacy_battle_line_raster(LegacyBattleLineRaster& raster) noexcept;

// sub_434420.
[[nodiscard]] LegacyBattleDirectionStepStatus
advance_legacy_battle_direction_raster(
    const LegacyBattleDirectionVectors& vectors,
    LegacyBattleDirectionRaster& raster
) noexcept;

// sub_433F00.
[[nodiscard]] bool release_legacy_battle_render_auxiliary_buffer(
    LegacyBattleRenderGeometry& geometry,
    LegacyBattleRenderAuxiliaryBufferReleaser& releaser
) noexcept;

// sub_433D70.
[[nodiscard]] LegacyBattleRenderCleanupResult
release_legacy_battle_render_resources(
    LegacyBattleRenderGeometry& geometry,
    LegacyBattleRenderAuxiliaryBufferReleaser& releaser
) noexcept;

// sub_433DC0.
[[nodiscard]] LegacyBattleRenderSurfaceRebuildResult
rebuild_legacy_battle_render_surface(
    LegacyBattleRenderGeometry& geometry,
    const rendering::LegacySurfaceGeometry& source,
    LegacyBattleRowOffsetAllocator& allocator
) noexcept;

[[nodiscard]] LegacyBattleRenderSurfaceRebuildResult
rebuild_legacy_battle_render_surface(
    LegacyBattleRenderGeometry& geometry,
    const rendering::LegacySurfaceGeometry& source
) noexcept;

// sub_433C40.
[[nodiscard]] LegacyBattleRenderInitializationResult
initialize_legacy_battle_render_geometry(
    LegacyBattleRenderGeometry& geometry,
    LegacyBattleRowOffsetAllocator& allocator
) noexcept;

[[nodiscard]] LegacyBattleRenderInitializationResult
initialize_legacy_battle_render_geometry(
    LegacyBattleRenderGeometry& geometry
) noexcept;

// sub_4518F0: call the pending binding-object initializer with fixed tokens.
[[nodiscard]] LegacyBattleRenderGeometryBindingInitializationResult
initialize_legacy_battle_render_geometry_binding(
    LegacyBattleRenderGeometryBindingObjectInitializationPort&
        object_initialization_port
);

// sub_4518E0: tail-forward to the adjacent typed initialization helper.
[[nodiscard]] LegacyBattleRenderGeometryBindingInitializationResult
forward_legacy_battle_render_geometry_binding_static_initialization(
    LegacyBattleRenderGeometryBindingObjectInitializationPort&
        object_initialization_port
);

// sub_4518A0 with loc_4518C0 and attached constructor sub_4518B0.
[[nodiscard]] LegacyBattleRenderGeometryStaticInitializationResult
initialize_legacy_battle_render_geometry_static_lifecycle(
    LegacyBattleRenderGeometry& geometry,
    LegacyBattleRenderGeometryExitRegistrationPort& exit_registration_port
) noexcept;

// attached exit wrapper sub_4518D0.
[[nodiscard]] LegacyBattleRenderGeometryStaticCleanupResult
release_legacy_battle_render_geometry_static_lifecycle(
    LegacyBattleRenderGeometry& geometry,
    LegacyBattleRenderAuxiliaryBufferReleaser& releaser
) noexcept;

// sub_433E20.
[[nodiscard]] LegacyBattleRowOffsetResult
rebuild_legacy_battle_primary_row_offsets(
    LegacyBattleRenderGeometry& geometry,
    compat::i32 row_stride,
    compat::i32 row_count,
    LegacyBattleRowOffsetAllocator& allocator
) noexcept;

[[nodiscard]] LegacyBattleRowOffsetResult
rebuild_legacy_battle_primary_row_offsets(
    LegacyBattleRenderGeometry& geometry,
    compat::i32 row_stride,
    compat::i32 row_count
) noexcept;

// sub_433E90.
[[nodiscard]] LegacyBattleRowOffsetResult
rebuild_legacy_battle_surface_row_offsets(
    LegacyBattleRenderGeometry& geometry,
    compat::i32 row_stride,
    compat::i32 row_count,
    LegacyBattleRowOffsetAllocator& allocator
) noexcept;

[[nodiscard]] LegacyBattleRowOffsetResult
rebuild_legacy_battle_surface_row_offsets(
    LegacyBattleRenderGeometry& geometry,
    compat::i32 row_stride,
    compat::i32 row_count
) noexcept;

// sub_433F30.
[[nodiscard]] LegacyBattleHostSurfaceResult set_legacy_battle_host_surface(
    LegacyBattleRenderGeometry& geometry,
    compat::i32 surface_width,
    compat::i32 surface_height,
    LegacyBattleRowOffsetAllocator& allocator
) noexcept;

[[nodiscard]] LegacyBattleHostSurfaceResult set_legacy_battle_host_surface(
    LegacyBattleRenderGeometry& geometry,
    compat::i32 surface_width,
    compat::i32 surface_height
) noexcept;

// sub_4342E0. The final two parameters are dimensions, not absolute edges.
compat::i32 set_legacy_battle_render_rectangle(
    LegacyBattleRenderGeometry& geometry,
    compat::i32 left,
    compat::i32 top,
    compat::i32 width,
    compat::i32 height
) noexcept;

}  // namespace openswd3::battle
