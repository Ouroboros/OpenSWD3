#include "openswd3/resource_io/legacy_lzo1x.hpp"

#include <algorithm>
#include <cstddef>
#include <limits>

namespace openswd3::resource_io {

namespace {

enum class DecoderState {
    literal,
    match,
    match_done,
};

enum class MatchResult {
    copied,
    finished,
    failed,
};

class LegacyLzo1xDecoder {
public:
    LegacyLzo1xDecoder(
        const std::span<const compat::u8> source,
        const std::span<compat::u8> destination
    ) noexcept
        : source_(source), destination_(destination) {}

    [[nodiscard]] LegacyLzo1xResult run() noexcept {
        constexpr std::size_t kMaximumLegacySize =
            std::numeric_limits<compat::u32>::max();
        if (source_.size() > kMaximumLegacySize ||
            destination_.size() > kMaximumLegacySize) {
            return make_result(LegacyLzo1xStatus::size_overflow);
        }

        if (source_.empty()) {
            return make_result(LegacyLzo1xStatus::source_exhausted);
        }

        compat::u32 token = source_[0];
        DecoderState state = DecoderState::literal;
        if (token > 0x11U) {
            input_position_ = 1U;
            const compat::u32 literal_count = token - 0x11U;
            if (!copy_literals(literal_count) || !read_byte(token)) {
                return failure_result();
            }

            if (literal_count >= 4U && token < 0x10U) {
                if (!copy_short_match_after_literal(token)) {
                    return failure_result();
                }

                state = DecoderState::match_done;
            } else {
                state = DecoderState::match;
            }
        }

        while (true) {
            if (state == DecoderState::literal) {
                if (!decode_literal_state(token, state)) {
                    return failure_result();
                }

                continue;
            }

            if (state == DecoderState::match) {
                const MatchResult match_result = decode_match(token);
                if (match_result == MatchResult::finished) {
                    const LegacyLzo1xStatus status =
                        input_position_ == source_.size()
                        ? LegacyLzo1xStatus::success
                        : LegacyLzo1xStatus::input_not_consumed;
                    return make_result(status);
                }

                if (match_result == MatchResult::failed) {
                    return failure_result();
                }

                state = DecoderState::match_done;
                continue;
            }

            if (!decode_match_done_state(token, state)) {
                return failure_result();
            }
        }
    }

private:
    [[nodiscard]] LegacyLzo1xResult
    make_result(const LegacyLzo1xStatus status) const noexcept {
        return LegacyLzo1xResult{
            status,
            static_cast<compat::u32>(output_position_),
        };
    }

    [[nodiscard]] LegacyLzo1xResult failure_result() const noexcept {
        return make_result(failure_status_);
    }

    bool fail(const LegacyLzo1xStatus status) noexcept {
        failure_status_ = status;
        return false;
    }

    bool read_byte(compat::u32& value) noexcept {
        if (input_position_ >= source_.size()) {
            return fail(LegacyLzo1xStatus::source_exhausted);
        }

        value = source_[input_position_];
        ++input_position_;
        return true;
    }

    bool read_u16(compat::u32& value) noexcept {
        if (source_.size() - input_position_ < 2U) {
            return fail(LegacyLzo1xStatus::source_exhausted);
        }

        value = static_cast<compat::u32>(source_[input_position_]) |
            (static_cast<compat::u32>(source_[input_position_ + 1U]) << 8U);
        input_position_ += 2U;
        return true;
    }

    bool checked_add(
        const compat::u32 left, const compat::u32 right, compat::u32& result
    ) noexcept {
        if (left > std::numeric_limits<compat::u32>::max() - right) {
            return fail(LegacyLzo1xStatus::size_overflow);
        }

        result = left + right;
        return true;
    }

    bool
    read_extended_count(const compat::u32 base, compat::u32& count) noexcept {
        count = 0U;
        while (true) {
            compat::u32 value{};
            if (!read_byte(value)) {
                return false;
            }

            if (value != 0U) {
                compat::u32 value_with_base{};
                return checked_add(value, base, value_with_base) &&
                    checked_add(count, value_with_base, count);
            }

            if (!checked_add(count, 0xFFU, count)) {
                return false;
            }
        }
    }

    bool copy_literals(const compat::u32 count) noexcept {
        const std::size_t copy_count = count;
        if (copy_count > source_.size() - input_position_) {
            return fail(LegacyLzo1xStatus::source_exhausted);
        }

        if (copy_count > destination_.size() - output_position_) {
            return fail(LegacyLzo1xStatus::destination_exhausted);
        }

        std::copy_n(
            source_.begin() + static_cast<std::ptrdiff_t>(input_position_),
            copy_count,
            destination_.begin() + static_cast<std::ptrdiff_t>(output_position_)
        );
        input_position_ += copy_count;
        output_position_ += copy_count;
        return true;
    }

    bool
    copy_match(const std::size_t distance, const compat::u32 count) noexcept {
        if (distance == 0U || distance > output_position_) {
            return fail(LegacyLzo1xStatus::invalid_lookbehind);
        }

        const std::size_t copy_count = count;
        if (copy_count > destination_.size() - output_position_) {
            return fail(LegacyLzo1xStatus::destination_exhausted);
        }

        for (std::size_t index = 0U; index < copy_count; ++index) {
            destination_[output_position_] =
                destination_[output_position_ - distance];
            ++output_position_;
        }

        return true;
    }

    bool copy_short_match_after_literal(const compat::u32 token) noexcept {
        compat::u32 offset{};
        if (!read_byte(offset)) {
            return false;
        }

        const std::size_t distance =
            0x801U + 4U * static_cast<std::size_t>(offset) + (token >> 2U);
        return copy_match(distance, 3U);
    }

    bool
    decode_literal_state(compat::u32& token, DecoderState& state) noexcept {
        if (!read_byte(token)) {
            return false;
        }

        if (token < 0x10U) {
            compat::u32 literal_count = token;
            if (literal_count == 0U &&
                !read_extended_count(0x0FU, literal_count)) {
                return false;
            }

            if (!checked_add(literal_count, 3U, literal_count) ||
                !copy_literals(literal_count) || !read_byte(token)) {
                return false;
            }

            if (token < 0x10U) {
                if (!copy_short_match_after_literal(token)) {
                    return false;
                }

                state = DecoderState::match_done;
                return true;
            }
        }

        state = DecoderState::match;
        return true;
    }

    MatchResult decode_match(const compat::u32 token) noexcept {
        if (token >= 0x40U) {
            compat::u32 offset{};
            if (!read_byte(offset)) {
                return MatchResult::failed;
            }

            const std::size_t distance = 8U * static_cast<std::size_t>(offset) +
                1U + ((token >> 2U) & 7U);
            const compat::u32 match_count = (token >> 5U) + 1U;
            return copy_match(distance, match_count) ? MatchResult::copied
                                                     : MatchResult::failed;
        }

        if (token >= 0x20U) {
            compat::u32 match_count = token & 0x1FU;
            if (match_count == 0U && !read_extended_count(0x1FU, match_count)) {
                return MatchResult::failed;
            }

            compat::u32 offset{};
            if (!read_u16(offset) ||
                !checked_add(match_count, 2U, match_count)) {
                return MatchResult::failed;
            }

            const std::size_t distance = (offset >> 2U) + 1U;
            return copy_match(distance, match_count) ? MatchResult::copied
                                                     : MatchResult::failed;
        }

        if (token < 0x10U) {
            compat::u32 offset{};
            if (!read_byte(offset)) {
                return MatchResult::failed;
            }

            const std::size_t distance =
                4U * static_cast<std::size_t>(offset) + (token >> 2U) + 1U;
            return copy_match(distance, 2U) ? MatchResult::copied
                                            : MatchResult::failed;
        }

        compat::u32 match_count = token & 7U;
        if (match_count == 0U && !read_extended_count(7U, match_count)) {
            return MatchResult::failed;
        }

        compat::u32 offset{};
        if (!read_u16(offset)) {
            return MatchResult::failed;
        }

        const std::size_t first_distance =
            ((token & 8U) != 0U ? 0x4000U : 0U) + (offset >> 2U);
        if (first_distance == 0U) {
            return MatchResult::finished;
        }

        if (!checked_add(match_count, 2U, match_count)) {
            return MatchResult::failed;
        }

        return copy_match(first_distance + 0x4000U, match_count)
            ? MatchResult::copied
            : MatchResult::failed;
    }

    bool
    decode_match_done_state(compat::u32& token, DecoderState& state) noexcept {
        if (input_position_ < 2U) {
            return fail(LegacyLzo1xStatus::source_exhausted);
        }

        const compat::u32 trailing_literals =
            source_[input_position_ - 2U] & 3U;
        if (trailing_literals == 0U) {
            state = DecoderState::literal;
            return true;
        }

        if (!copy_literals(trailing_literals) || !read_byte(token)) {
            return false;
        }

        state = DecoderState::match;
        return true;
    }

    std::span<const compat::u8> source_;
    std::span<compat::u8> destination_;
    std::size_t input_position_{};
    std::size_t output_position_{};
    LegacyLzo1xStatus failure_status_{LegacyLzo1xStatus::source_exhausted};
};

}  // namespace

LegacyLzo1xResult decompress_legacy_lzo1x(
    const std::span<const compat::u8> source,
    const std::span<compat::u8> destination
) noexcept {
    return LegacyLzo1xDecoder{source, destination}.run();
}

LegacyLzo1xStatus decompress_legacy_resource_block(
    const std::span<const compat::u8> source,
    const std::span<compat::u8> destination,
    compat::u32& actual_output_size
) noexcept {
    actual_output_size = 0U;
    const LegacyLzo1xResult result =
        decompress_legacy_lzo1x(source, destination);
    if (result.status == LegacyLzo1xStatus::success ||
        result.status == LegacyLzo1xStatus::input_not_consumed) {
        actual_output_size = result.bytes_written;
    }
    return result.status;
}

}  // namespace openswd3::resource_io
