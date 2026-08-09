#include "openswd3/asset_runtime/legacy_frame_deformation.hpp"

#include "openswd3/input_time_rng/legacy_crt_rng.hpp"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdint>
#include <limits>
#include <utility>

namespace openswd3::asset_runtime {
namespace {

using compat::i16;
using compat::i32;
using compat::u16;
using compat::u32;

[[nodiscard]] constexpr u32 to_bits(const i32 value) noexcept {
    return std::bit_cast<u32>(value);
}

[[nodiscard]] constexpr i32 from_bits(const u32 value) noexcept {
    return std::bit_cast<i32>(value);
}

[[nodiscard]] constexpr i32 wrapping_add(
    const i32 left,
    const i32 right
) noexcept {
    return from_bits(to_bits(left) + to_bits(right));
}

[[nodiscard]] constexpr i32 wrapping_subtract(
    const i32 left,
    const i32 right
) noexcept {
    return from_bits(to_bits(left) - to_bits(right));
}

[[nodiscard]] constexpr i32 wrapping_multiply(
    const i32 left,
    const i32 right
) noexcept {
    return from_bits(to_bits(left) * to_bits(right));
}

[[nodiscard]] constexpr i32 arithmetic_shift_right(
    const i32 value,
    const u32 raw_count
) noexcept {
    const u32 count = raw_count & 31U;
    if (count == 0U) {
        return value;
    }
    const u32 bits = to_bits(value);
    const u32 sign_fill = (bits & 0x80000000U) == 0U
                              ? 0U
                              : ~(std::numeric_limits<u32>::max() >> count);
    return from_bits((bits >> count) | sign_fill);
}

[[nodiscard]] constexpr i16 low_signed_word(const i32 value) noexcept {
    return std::bit_cast<i16>(static_cast<u16>(to_bits(value)));
}

[[nodiscard]] constexpr i16 wrapping_word_add(
    const i16 left,
    const i32 right
) noexcept {
    const u16 bits = static_cast<u16>(
        std::bit_cast<u16>(left) + static_cast<u16>(to_bits(right))
    );
    return std::bit_cast<i16>(bits);
}

[[nodiscard]] bool checked_product(
    const u32 left,
    const u32 right,
    std::size_t& result
) noexcept {
    const std::uint64_t product =
        static_cast<std::uint64_t>(left) * right;
    if (product == 0U ||
        product > static_cast<std::uint64_t>(
                      std::numeric_limits<i32>::max()) ||
        product > std::numeric_limits<std::size_t>::max()) {
        return false;
    }
    result = static_cast<std::size_t>(product);
    return true;
}

[[nodiscard]] bool checked_index(
    const i32 index,
    const std::size_t size,
    std::size_t& converted
) noexcept {
    if (index < 0) {
        return false;
    }
    converted = static_cast<std::size_t>(index);
    return converted < size;
}

[[nodiscard]] i32 signed_dimension(const u32 value) noexcept {
    return from_bits(value);
}

}  // namespace

LegacyDeformationNode::LegacyDeformationNode(
    const LegacyDeformationConfiguration& configuration
)
    : state_{
          .framebuffer_width = configuration.framebuffer_width,
          .framebuffer_height = configuration.framebuffer_height,
          .origin_x = configuration.origin_x,
          .origin_y = configuration.origin_y,
          .field_width = configuration.field_width,
          .field_height = configuration.field_height,
          .damping_shift = kLegacyDeformationDampingShift,
          .active_field_index = 0U,
      } {
    std::size_t field_pixels{};
    std::size_t framebuffer_pixels{};
    if (!checked_product(state_.field_width, state_.field_height,
                         field_pixels) ||
        !checked_product(state_.framebuffer_width,
                         state_.framebuffer_height,
                         framebuffer_pixels) ||
        field_pixels > std::numeric_limits<std::size_t>::max() / 2U) {
        return;
    }

    fields_.assign(field_pixels * 2U, i16{});
    source_snapshot_.resize(framebuffer_pixels);
    storage_valid_ = true;
}

bool LegacyDeformationNode::geometry_is_usable() const noexcept {
    return storage_valid_ && state_.framebuffer_width != 0U &&
           state_.framebuffer_height != 0U && state_.field_width != 0U &&
           state_.field_height != 0U;
}

i32 LegacyDeformationNode::set_origin(
    const i32 origin_x,
    const i32 origin_y
) noexcept {
    const i32 framebuffer_width = signed_dimension(state_.framebuffer_width);
    const i32 framebuffer_height =
        signed_dimension(state_.framebuffer_height);
    const i32 field_width = signed_dimension(state_.field_width);
    const i32 field_height = signed_dimension(state_.field_height);

    state_.origin_x = origin_x;
    state_.origin_y = origin_y;
    if (wrapping_add(origin_x, field_width) > framebuffer_width) {
        state_.origin_x = wrapping_subtract(
            wrapping_subtract(framebuffer_width, field_width), 1
        );
    }

    i32 result = framebuffer_height;
    if (wrapping_add(origin_y, field_height) > framebuffer_height) {
        result = wrapping_subtract(
            wrapping_subtract(framebuffer_height, field_height), 1
        );
        state_.origin_y = result;
    }
    return result;
}

LegacyDeformationStatus LegacyDeformationNode::capture(
    const std::span<const u16> framebuffer
) noexcept {
    if (!geometry_is_usable()) {
        return LegacyDeformationStatus::invalid_geometry;
    }
    if (framebuffer.size() < source_snapshot_.size()) {
        return LegacyDeformationStatus::framebuffer_too_small;
    }
    std::ranges::copy(framebuffer.first(source_snapshot_.size()),
                      source_snapshot_.begin());
    return LegacyDeformationStatus::ready;
}

LegacyDeformationStatus LegacyDeformationNode::apply(
    const std::span<u16> framebuffer
) const noexcept {
    if (!geometry_is_usable()) {
        return LegacyDeformationStatus::invalid_geometry;
    }
    if (framebuffer.size() < source_snapshot_.size()) {
        return LegacyDeformationStatus::framebuffer_too_small;
    }

    const i32 framebuffer_width = signed_dimension(state_.framebuffer_width);
    const i32 field_width = signed_dimension(state_.field_width);
    const i32 field_height = signed_dimension(state_.field_height);
    const std::size_t field_pixels = fields_.size() / 2U;
    const i32 origin_y_plus_one = wrapping_add(state_.origin_y, 1);
    i32 framebuffer_cursor = wrapping_add(
        state_.origin_x,
        wrapping_multiply(framebuffer_width, origin_y_plus_one)
    );
    const i32 field_end = wrapping_multiply(
        field_width,
        wrapping_subtract(field_height, 1)
    );
    i32 field_cursor = wrapping_add(field_width, 1);

    while (field_cursor < field_end) {
        const i32 row_end = wrapping_subtract(
            wrapping_add(field_width, field_cursor), 2
        );
        if (field_cursor < row_end) {
            do {
                for (u32 pair_index = 0U; pair_index < 2U; ++pair_index) {
                    std::size_t center{};
                    std::size_t below{};
                    std::size_t right{};
                    std::size_t destination{};
                    if (!checked_index(field_cursor, field_pixels, center) ||
                        !checked_index(
                            wrapping_add(field_cursor, field_width),
                            field_pixels, below
                        ) ||
                        !checked_index(
                            wrapping_add(field_cursor, 1), field_pixels, right
                        ) ||
                        !checked_index(framebuffer_cursor, framebuffer.size(),
                                       destination)) {
                        return LegacyDeformationStatus::invalid_geometry;
                    }

                    const i32 center_value = fields_[center];
                    const i32 vertical = arithmetic_shift_right(
                        wrapping_subtract(center_value, fields_[below]), 3U
                    );
                    const i32 horizontal = arithmetic_shift_right(
                        wrapping_subtract(center_value, fields_[right]), 3U
                    );
                    const i32 source_index = wrapping_add(
                        wrapping_add(
                            framebuffer_cursor,
                            wrapping_multiply(framebuffer_width, vertical)
                        ),
                        horizontal
                    );
                    std::size_t source{};
                    if (!checked_index(source_index, source_snapshot_.size(),
                                       source)) {
                        return LegacyDeformationStatus::source_sample_out_of_bounds;
                    }
                    framebuffer[destination] = source_snapshot_[source];
                    field_cursor = wrapping_add(field_cursor, 1);
                    framebuffer_cursor = wrapping_add(framebuffer_cursor, 1);
                }
            } while (field_cursor < row_end);
        }

        field_cursor = wrapping_add(field_cursor, 2);
        framebuffer_cursor = wrapping_add(
            framebuffer_cursor,
            wrapping_add(
                wrapping_subtract(framebuffer_width, field_width), 2
            )
        );
    }
    return LegacyDeformationStatus::ready;
}

LegacyDeformationAdvanceResult LegacyDeformationNode::advance() noexcept {
    LegacyDeformationAdvanceResult result;
    state_.active_field_index ^= 1U;
    if (!geometry_is_usable()) {
        result.status = LegacyDeformationStatus::invalid_geometry;
        return result;
    }

    const std::size_t field_pixels = fields_.size() / 2U;
    const std::size_t destination_base =
        field_pixels * state_.active_field_index;
    const std::size_t source_base =
        field_pixels * (state_.active_field_index ^ 1U);
    const i32 width = signed_dimension(state_.field_width);
    const i32 height = signed_dimension(state_.field_height);
    i32 cursor = wrapping_add(width, 1);

    const auto source_value = [&](const i32 index, i32& value) noexcept {
        std::size_t converted{};
        if (!checked_index(index, field_pixels, converted)) {
            return false;
        }
        value = fields_[source_base + converted];
        return true;
    };

    i32 first{};
    i32 second{};
    if (!source_value(wrapping_add(wrapping_add(width, width), 2), first) ||
        !source_value(2, second)) {
        result.status = LegacyDeformationStatus::invalid_geometry;
        return result;
    }
    i32 carried_vertical_sum = wrapping_add(first, second);
    const i32 field_end = wrapping_multiply(
        width,
        wrapping_subtract(height, 1)
    );
    bool complete = true;

    while (cursor < field_end) {
        i32 previous_vertical_sum = carried_vertical_sum;
        if (!source_value(wrapping_add(cursor, width), first) ||
            !source_value(wrapping_subtract(cursor, width), second)) {
            result.status = LegacyDeformationStatus::invalid_geometry;
            return result;
        }
        carried_vertical_sum = wrapping_add(first, second);
        const i32 row_end = wrapping_subtract(
            wrapping_add(width, cursor), 2
        );
        if (cursor < row_end) {
            while (true) {
                const i32 current_vertical_sum = carried_vertical_sum;
                if (!source_value(
                        wrapping_add(wrapping_add(cursor, width), 1), first
                    ) ||
                    !source_value(
                        wrapping_add(wrapping_subtract(cursor, width), 1),
                        second
                    )) {
                    result.status = LegacyDeformationStatus::invalid_geometry;
                    return result;
                }
                carried_vertical_sum = wrapping_add(first, second);

                i32 left{};
                i32 right{};
                std::size_t destination{};
                if (!source_value(wrapping_subtract(cursor, 1), left) ||
                    !source_value(wrapping_add(cursor, 1), right) ||
                    !checked_index(cursor, field_pixels, destination)) {
                    result.status = LegacyDeformationStatus::invalid_geometry;
                    return result;
                }

                i32 sum = wrapping_add(previous_vertical_sum,
                                       current_vertical_sum);
                sum = wrapping_add(sum, left);
                sum = wrapping_add(sum, carried_vertical_sum);
                sum = wrapping_add(sum, right);
                i32 value = wrapping_subtract(
                    arithmetic_shift_right(sum, 2U),
                    fields_[destination_base + destination]
                );
                value = wrapping_subtract(
                    value,
                    arithmetic_shift_right(value, state_.damping_shift)
                );
                const i16 stored = low_signed_word(value);
                fields_[destination_base + destination] = stored;
                if (stored != 0) {
                    complete = false;
                }

                cursor = wrapping_add(cursor, 1);
                if (cursor >= row_end) {
                    break;
                }
                previous_vertical_sum = current_vertical_sum;
            }
        }
        cursor = wrapping_add(cursor, 2);
    }

    result.complete = complete;
    return result;
}

LegacyDeformationInjectionResult LegacyDeformationNode::inject(
    i32 x,
    i32 y,
    const i32 radius,
    const i32 strength,
    input_time_rng::LegacyCrtRng& random
) noexcept {
    LegacyDeformationInjectionResult result;
    if (!geometry_is_usable() || radius < 0) {
        result.status = LegacyDeformationStatus::invalid_geometry;
        return result;
    }

    const i32 width = signed_dimension(state_.field_width);
    const i32 height = signed_dimension(state_.field_height);
    const i32 doubled_radius = wrapping_add(radius, radius);
    if (x < 0) {
        const i32 divisor = wrapping_subtract(
            wrapping_subtract(width, doubled_radius), 1
        );
        if (divisor <= 0) {
            result.status = LegacyDeformationStatus::random_range_invalid;
            return result;
        }
        x = wrapping_add(
            static_cast<i32>(random.next() % static_cast<u32>(divisor)),
            wrapping_add(radius, 1)
        );
    }
    if (y < 0) {
        const i32 divisor = wrapping_subtract(
            wrapping_subtract(height, doubled_radius), 1
        );
        if (divisor <= 0) {
            result.status = LegacyDeformationStatus::random_range_invalid;
            return result;
        }
        y = wrapping_add(
            static_cast<i32>(random.next() % static_cast<u32>(divisor)),
            wrapping_add(radius, 1)
        );
    }
    result.resolved_x = x;
    result.resolved_y = y;

    i32 first_x = wrapping_subtract(0, radius);
    i32 first_y = wrapping_subtract(0, radius);
    i32 end_x = radius;
    i32 end_y = radius;
    if (wrapping_subtract(x, radius) < 1) {
        first_x = wrapping_subtract(1, x);
    }
    if (wrapping_subtract(y, radius) < 1) {
        first_y = wrapping_subtract(1, y);
    }
    if (wrapping_add(x, radius) > wrapping_subtract(width, 1)) {
        end_x = wrapping_subtract(wrapping_subtract(width, x), 1);
    }
    if (wrapping_add(y, radius) > wrapping_subtract(height, 1)) {
        end_y = wrapping_subtract(wrapping_subtract(height, y), 1);
    }

    const i32 radius_squared = wrapping_multiply(radius, radius);
    const std::size_t field_pixels = fields_.size() / 2U;
    const std::size_t active_base =
        field_pixels * state_.active_field_index;
    for (i32 offset_y = first_y; offset_y < end_y;
         offset_y = wrapping_add(offset_y, 1)) {
        const i32 y_squared = wrapping_multiply(offset_y, offset_y);
        for (i32 offset_x = first_x; offset_x < end_x;
             offset_x = wrapping_add(offset_x, 1)) {
            const i32 squared_distance = wrapping_add(
                wrapping_multiply(offset_x, offset_x), y_squared
            );
            if (squared_distance >= radius_squared) {
                continue;
            }

            const i32 index = wrapping_add(
                wrapping_add(x, offset_x),
                wrapping_multiply(width, wrapping_add(y, offset_y))
            );
            std::size_t converted{};
            if (!checked_index(index, field_pixels, converted)) {
                result.status = LegacyDeformationStatus::invalid_geometry;
                return result;
            }
            const long double delta =
                (static_cast<long double>(radius) -
                 std::sqrt(static_cast<long double>(squared_distance))) *
                static_cast<long double>(strength);
            const auto truncated = static_cast<std::int64_t>(delta);
            fields_[active_base + converted] = wrapping_word_add(
                fields_[active_base + converted],
                from_bits(static_cast<u32>(
                    static_cast<std::uint64_t>(truncated)
                ))
            );
        }
    }
    return result;
}

LegacyDeformationState& LegacyDeformationNode::state() noexcept {
    return state_;
}

const LegacyDeformationState& LegacyDeformationNode::state() const noexcept {
    return state_;
}

std::span<i16> LegacyDeformationNode::field(const u32 index) noexcept {
    if (!storage_valid_ || index > 1U) {
        return {};
    }
    const std::size_t field_pixels = fields_.size() / 2U;
    return std::span<i16>{fields_}.subspan(field_pixels * index,
                                           field_pixels);
}

std::span<const i16> LegacyDeformationNode::field(
    const u32 index
) const noexcept {
    if (!storage_valid_ || index > 1U) {
        return {};
    }
    const std::size_t field_pixels = fields_.size() / 2U;
    return std::span<const i16>{fields_}.subspan(field_pixels * index,
                                                 field_pixels);
}

std::span<const u16> LegacyDeformationNode::source_snapshot()
    const noexcept {
    return source_snapshot_;
}

LegacyDeformationList::LegacyDeformationList()
    : sentinel_(LegacyDeformationConfiguration{
          .framebuffer_width = 1U,
          .framebuffer_height = 1U,
          .origin_x = 0,
          .origin_y = 0,
          .field_width = 1U,
          .field_height = 1U,
      }) {}

LegacyDeformationList::~LegacyDeformationList() {
    clear();
}

void LegacyDeformationList::push_front(
    std::unique_ptr<LegacyDeformationNode> node
) noexcept {
    if (!node) {
        return;
    }
    node->next_ = std::move(sentinel_.next_);
    sentinel_.next_ = std::move(node);
}

void LegacyDeformationList::clear() noexcept {
    while (sentinel_.next_) {
        std::unique_ptr<LegacyDeformationNode> current =
            std::move(sentinel_.next_);
        sentinel_.next_ = std::move(current->next_);
    }
}

LegacyDeformationListUpdateResult LegacyDeformationList::update(
    const std::span<u16> framebuffer
) noexcept {
    LegacyDeformationListUpdateResult result;
    auto* link = &sentinel_.next_;
    while (*link) {
        LegacyDeformationNode& node = **link;
        result.status = node.capture(framebuffer);
        if (result.status != LegacyDeformationStatus::ready) {
            return result;
        }
        result.status = node.apply(framebuffer);
        if (result.status != LegacyDeformationStatus::ready) {
            return result;
        }
        const LegacyDeformationAdvanceResult advanced = node.advance();
        result.status = advanced.status;
        if (result.status != LegacyDeformationStatus::ready) {
            return result;
        }
        ++result.processed;
        if (advanced.complete) {
            std::unique_ptr<LegacyDeformationNode> removed = std::move(*link);
            *link = std::move(removed->next_);
            ++result.removed;
        } else {
            link = &node.next_;
        }
    }
    return result;
}

LegacyDeformationNode* LegacyDeformationList::front() noexcept {
    return sentinel_.next_.get();
}

const LegacyDeformationNode* LegacyDeformationList::front() const noexcept {
    return sentinel_.next_.get();
}

std::size_t LegacyDeformationList::size() const noexcept {
    std::size_t count{};
    for (const LegacyDeformationNode* node = front(); node != nullptr;
         node = node->next_.get()) {
        ++count;
    }
    return count;
}

bool LegacyDeformationList::empty() const noexcept {
    return sentinel_.next_ == nullptr;
}

}  // namespace openswd3::asset_runtime
