#pragma once

#include "openswd3/compat/types.hpp"

#include <cstddef>
#include <memory>
#include <span>
#include <vector>

namespace openswd3::input_time_rng {
class LegacyCrtRng;
}

namespace openswd3::asset_runtime {

inline constexpr compat::u32 kLegacyDeformationDampingShift = 4U;

struct LegacyDeformationConfiguration {
    compat::u32 framebuffer_width{};
    compat::u32 framebuffer_height{};
    compat::i32 origin_x{};
    compat::i32 origin_y{};
    compat::u32 field_width{};
    compat::u32 field_height{};
};

struct LegacyDeformationState {
    compat::u32 framebuffer_width{};
    compat::u32 framebuffer_height{};
    compat::i32 origin_x{};
    compat::i32 origin_y{};
    compat::u32 field_width{};
    compat::u32 field_height{};
    compat::u32 damping_shift{kLegacyDeformationDampingShift};
    compat::u32 active_field_index{};
};

enum class LegacyDeformationStatus {
    ready,
    invalid_geometry,
    framebuffer_too_small,
    source_sample_out_of_bounds,
    random_range_invalid,
};

struct LegacyDeformationAdvanceResult {
    LegacyDeformationStatus status{LegacyDeformationStatus::ready};
    bool complete{};
};

struct LegacyDeformationInjectionResult {
    LegacyDeformationStatus status{LegacyDeformationStatus::ready};
    compat::i32 resolved_x{};
    compat::i32 resolved_y{};
};

struct LegacyDeformationListUpdateResult {
    LegacyDeformationStatus status{LegacyDeformationStatus::ready};
    std::size_t processed{};
    std::size_t removed{};
};

class LegacyDeformationNode final {
public:
    explicit LegacyDeformationNode(
        const LegacyDeformationConfiguration& configuration
    );

    LegacyDeformationNode(const LegacyDeformationNode&) = delete;
    LegacyDeformationNode& operator=(const LegacyDeformationNode&) = delete;
    LegacyDeformationNode(LegacyDeformationNode&&) = delete;
    LegacyDeformationNode& operator=(LegacyDeformationNode&&) = delete;

    [[nodiscard]] compat::i32 set_origin(
        compat::i32 origin_x,
        compat::i32 origin_y
    ) noexcept;
    [[nodiscard]] LegacyDeformationStatus capture(
        std::span<const compat::u16> framebuffer
    ) noexcept;
    [[nodiscard]] LegacyDeformationStatus apply(
        std::span<compat::u16> framebuffer
    ) const noexcept;
    [[nodiscard]] LegacyDeformationAdvanceResult advance() noexcept;
    [[nodiscard]] LegacyDeformationInjectionResult inject(
        compat::i32 x,
        compat::i32 y,
        compat::i32 radius,
        compat::i32 strength,
        input_time_rng::LegacyCrtRng& random
    ) noexcept;

    [[nodiscard]] LegacyDeformationState& state() noexcept;
    [[nodiscard]] const LegacyDeformationState& state() const noexcept;
    [[nodiscard]] std::span<compat::i16> field(
        compat::u32 index
    ) noexcept;
    [[nodiscard]] std::span<const compat::i16> field(
        compat::u32 index
    ) const noexcept;
    [[nodiscard]] std::span<const compat::u16> source_snapshot()
        const noexcept;

private:
    friend class LegacyDeformationList;

    [[nodiscard]] bool geometry_is_usable() const noexcept;

    LegacyDeformationState state_;
    std::vector<compat::u16> source_snapshot_;
    std::vector<compat::i16> fields_;
    std::unique_ptr<LegacyDeformationNode> next_;
    bool storage_valid_{};
};

class LegacyDeformationList final {
public:
    LegacyDeformationList();
    ~LegacyDeformationList();

    LegacyDeformationList(const LegacyDeformationList&) = delete;
    LegacyDeformationList& operator=(const LegacyDeformationList&) = delete;
    LegacyDeformationList(LegacyDeformationList&&) = delete;
    LegacyDeformationList& operator=(LegacyDeformationList&&) = delete;

    void push_front(std::unique_ptr<LegacyDeformationNode> node) noexcept;
    void clear() noexcept;
    [[nodiscard]] LegacyDeformationListUpdateResult update(
        std::span<compat::u16> framebuffer
    ) noexcept;

    [[nodiscard]] LegacyDeformationNode* front() noexcept;
    [[nodiscard]] const LegacyDeformationNode* front() const noexcept;
    [[nodiscard]] std::size_t size() const noexcept;
    [[nodiscard]] bool empty() const noexcept;

private:
    LegacyDeformationNode sentinel_;
};

}  // namespace openswd3::asset_runtime
