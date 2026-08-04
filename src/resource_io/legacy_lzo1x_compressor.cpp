#include "openswd3/resource_io/legacy_lzo1x.hpp"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <span>
#include <vector>

namespace openswd3::resource_io {
namespace {

constexpr std::size_t kHelperThreshold = 13U;
constexpr std::size_t kInitialSearchPosition = 4U;
constexpr std::size_t kSearchLookahead = 13U;
constexpr std::size_t kM2MaximumDistance = 0x0800U;
constexpr std::size_t kM3MaximumDistance = 0x4000U;
constexpr std::size_t kMaximumMatchDistance = 0xBFFFU;
constexpr compat::u32 kInvalidDictionaryPosition =
    std::numeric_limits<compat::u32>::max();

class CompressionOutput {
public:
    explicit CompressionOutput(
        const std::span<compat::u8> destination
    ) noexcept
        : destination_{destination} {}

    [[nodiscard]] bool write_byte(const compat::u8 value) noexcept {
        if (position_ >= destination_.size()) {
            return false;
        }

        destination_[position_] = value;
        ++position_;
        return true;
    }

    [[nodiscard]] bool write_zeroes(const std::size_t count) noexcept {
        if (count > destination_.size() - position_) {
            return false;
        }

        std::fill_n(
            destination_.begin() + static_cast<std::ptrdiff_t>(position_),
            count,
            compat::u8{0}
        );
        position_ += count;
        return true;
    }

    [[nodiscard]] bool write_bytes(
        const std::span<const compat::u8> source,
        const std::size_t source_position,
        const std::size_t count
    ) noexcept {
        if (count > destination_.size() - position_) {
            return false;
        }

        std::copy_n(
            source.begin() + static_cast<std::ptrdiff_t>(source_position),
            count,
            destination_.begin() + static_cast<std::ptrdiff_t>(position_)
        );
        position_ += count;
        return true;
    }

    [[nodiscard]] bool add_trailing_literal_count(
        const compat::u8 count
    ) noexcept {
        if (position_ < 2U) {
            return false;
        }

        destination_[position_ - 2U] |= count;
        return true;
    }

    [[nodiscard]] std::size_t size() const noexcept {
        return position_;
    }

    [[nodiscard]] std::size_t capacity() const noexcept {
        return destination_.size();
    }

private:
    std::span<compat::u8> destination_;
    std::size_t position_{};
};

[[nodiscard]] bool write_extended_length(
    CompressionOutput& output,
    const std::size_t value
) noexcept {
    const std::size_t zero_count = (value - 1U) / 0xFFU;
    const std::size_t remainder = value - zero_count * 0xFFU;
    return output.write_zeroes(zero_count) &&
           output.write_byte(static_cast<compat::u8>(remainder));
}

[[nodiscard]] bool write_noninitial_literal_header(
    CompressionOutput& output,
    const std::size_t count
) noexcept {
    if (count <= 3U) {
        return output.add_trailing_literal_count(
            static_cast<compat::u8>(count)
        );
    }

    if (count <= 0x12U) {
        return output.write_byte(static_cast<compat::u8>(count - 3U));
    }

    return output.write_byte(0U) &&
           write_extended_length(output, count - 0x12U);
}

[[nodiscard]] bool write_literal_run(
    CompressionOutput& output,
    const std::span<const compat::u8> source,
    const std::size_t source_position,
    const std::size_t count
) noexcept {
    return write_noninitial_literal_header(output, count) &&
           output.write_bytes(source, source_position, count);
}

[[nodiscard]] bool write_trailing_literals(
    CompressionOutput& output,
    const std::span<const compat::u8> source,
    const std::size_t count
) noexcept {
    if (count == 0U) {
        return true;
    }

    if (output.size() == 0U && count <= 0xEEU) {
        if (!output.write_byte(static_cast<compat::u8>(count + 0x11U))) {
            return false;
        }
    } else if (!write_noninitial_literal_header(output, count)) {
        return false;
    }

    return output.write_bytes(source, source.size() - count, count);
}

[[nodiscard]] bool write_offset(
    CompressionOutput& output,
    const std::size_t offset
) noexcept {
    return output.write_byte(static_cast<compat::u8>(offset << 2U)) &&
           output.write_byte(static_cast<compat::u8>(offset >> 6U));
}

[[nodiscard]] bool write_match(
    CompressionOutput& output,
    const std::size_t distance,
    const std::size_t length
) noexcept {
    if (length <= 8U && distance <= kM2MaximumDistance) {
        const std::size_t offset = distance - 1U;
        const compat::u8 token = static_cast<compat::u8>(
            ((length - 1U) << 5U) | ((offset & 7U) << 2U)
        );
        return output.write_byte(token) &&
               output.write_byte(static_cast<compat::u8>(offset >> 3U));
    }

    if (distance <= kM3MaximumDistance) {
        const std::size_t offset = distance - 1U;
        if (length <= 0x21U) {
            if (!output.write_byte(static_cast<compat::u8>(
                    0x20U | (length - 2U)
                ))) {
                return false;
            }
        } else if (!output.write_byte(0x20U) ||
                   !write_extended_length(output, length - 0x21U)) {
            return false;
        }

        return write_offset(output, offset);
    }

    const std::size_t offset = distance - kM3MaximumDistance;
    const compat::u8 distance_bit = static_cast<compat::u8>(
        (offset >> 11U) & 8U
    );
    if (length <= 9U) {
        if (!output.write_byte(static_cast<compat::u8>(
                0x10U | distance_bit | (length - 2U)
            ))) {
            return false;
        }
    } else if (!output.write_byte(static_cast<compat::u8>(
                   0x10U | distance_bit
               )) ||
               !write_extended_length(output, length - 9U)) {
        return false;
    }

    return write_offset(output, offset);
}

template <unsigned DictionaryBits>
[[nodiscard]] compat::u32 dictionary_index(
    const std::span<const compat::u8> source,
    const std::size_t position
) noexcept {
    static_assert(DictionaryBits == 14U || DictionaryBits == 15U);
    compat::u32 value = static_cast<compat::u32>(source[position + 3U]) << 6U;
    value = (value ^ source[position + 2U]) << 5U;
    value = (value ^ source[position + 1U]) << 5U;
    value ^= source[position];
    value *= 0x21U;
    return (value >> 5U) & ((1U << DictionaryBits) - 1U);
}

template <unsigned DictionaryBits>
class LegacyLzo1xCompressor {
public:
    LegacyLzo1xCompressor(
        const std::span<const compat::u8> source,
        const std::span<compat::u8> destination
    )
        : source_{source},
          output_{destination},
          dictionary_(1U << DictionaryBits, kInvalidDictionaryPosition) {}

    [[nodiscard]] LegacyLzo1xResult run() noexcept {
        constexpr std::size_t kMaximumLegacySize =
            std::numeric_limits<compat::u32>::max();
        if (source_.size() > kMaximumLegacySize ||
            output_.capacity() > kMaximumLegacySize) {
            return result(LegacyLzo1xStatus::size_overflow);
        }

        std::size_t trailing_literals = source_.size();
        if (source_.size() > kHelperThreshold &&
            !write_matches(trailing_literals)) {
            return result(LegacyLzo1xStatus::destination_exhausted);
        }

        if (!write_trailing_literals(output_, source_, trailing_literals) ||
            !output_.write_byte(0x11U) ||
            !output_.write_byte(0x00U) ||
            !output_.write_byte(0x00U)) {
            return result(LegacyLzo1xStatus::destination_exhausted);
        }

        return result(LegacyLzo1xStatus::success);
    }

private:
    [[nodiscard]] LegacyLzo1xResult result(
        const LegacyLzo1xStatus status
    ) const noexcept {
        return LegacyLzo1xResult{
            status,
            static_cast<compat::u32>(output_.size()),
        };
    }

    [[nodiscard]] static bool valid_candidate(
        const compat::u32 candidate,
        const std::size_t position,
        std::size_t& distance
    ) noexcept {
        if (candidate == kInvalidDictionaryPosition || candidate >= position) {
            return false;
        }

        distance = position - candidate;
        return distance <= kMaximumMatchDistance;
    }

    [[nodiscard]] bool first_three_bytes_match(
        const std::size_t candidate,
        const std::size_t position
    ) const noexcept {
        return source_[candidate] == source_[position] &&
               source_[candidate + 1U] == source_[position + 1U] &&
               source_[candidate + 2U] == source_[position + 2U];
    }

    [[nodiscard]] bool write_matches(
        std::size_t& trailing_literals
    ) noexcept {
        std::size_t literal_position = 0U;
        std::size_t position = kInitialSearchPosition;
        const std::size_t search_end = source_.size() - kSearchLookahead;

        while (true) {
            const compat::u32 primary_index =
                dictionary_index<DictionaryBits>(source_, position);
            std::size_t slot = primary_index;
            compat::u32 candidate = dictionary_[slot];
            std::size_t distance{};
            bool candidate_valid = valid_candidate(
                candidate,
                position,
                distance
            );

            if (candidate_valid &&
                distance > kM2MaximumDistance &&
                source_[candidate + 3U] != source_[position + 3U]) {
                slot = (primary_index & 0x07FFU) ^
                       ((1U << (DictionaryBits - 1U)) | 0x001FU);
                candidate = dictionary_[slot];
                candidate_valid = valid_candidate(
                    candidate,
                    position,
                    distance
                );
                if (candidate_valid &&
                    distance > kM2MaximumDistance &&
                    source_[candidate + 3U] != source_[position + 3U]) {
                    candidate_valid = false;
                }
            }

            if (candidate_valid &&
                first_three_bytes_match(candidate, position)) {
                dictionary_[slot] = static_cast<compat::u32>(position);
                const std::size_t literal_count = position - literal_position;
                if (literal_count > 0U &&
                    !write_literal_run(
                        output_,
                        source_,
                        literal_position,
                        literal_count
                    )) {
                    return false;
                }

                const std::size_t match_position = position;
                std::size_t match_length = 3U;
                while (match_position + match_length < source_.size() &&
                       source_[candidate + match_length] ==
                           source_[match_position + match_length]) {
                    ++match_length;
                }

                position += match_length;
                if (!write_match(output_, distance, match_length)) {
                    return false;
                }

                literal_position = position;
                if (position >= search_end) {
                    break;
                }

                continue;
            }

            dictionary_[slot] = static_cast<compat::u32>(position);
            ++position;
            if (position >= search_end) {
                break;
            }
        }

        trailing_literals = source_.size() - literal_position;
        return true;
    }

    std::span<const compat::u8> source_;
    CompressionOutput output_;
    std::vector<compat::u32> dictionary_;
};

template <unsigned DictionaryBits>
[[nodiscard]] LegacyLzo1xResult compress_legacy_lzo1x_impl(
    const std::span<const compat::u8> source,
    const std::span<compat::u8> destination
) noexcept {
    return LegacyLzo1xCompressor<DictionaryBits>{source, destination}.run();
}

}  // namespace

LegacyLzo1xResult compress_legacy_lzo1x_14(
    const std::span<const compat::u8> source,
    const std::span<compat::u8> destination
) noexcept {
    return compress_legacy_lzo1x_impl<14U>(source, destination);
}

LegacyLzo1xResult compress_legacy_lzo1x_15(
    const std::span<const compat::u8> source,
    const std::span<compat::u8> destination
) noexcept {
    return compress_legacy_lzo1x_impl<15U>(source, destination);
}

LegacyLzo1xOwnedBlock compress_legacy_save_block(
    const std::span<const compat::u8> source
) {
    LegacyLzo1xOwnedBlock block;
    if (source.size() >
        static_cast<std::size_t>(
            std::numeric_limits<compat::u32>::max() - 0x20U
        )) {
        block.status = LegacyLzo1xStatus::size_overflow;
        return block;
    }

    block.storage.resize(source.size() + 0x20U);
    const LegacyLzo1xResult compressed = compress_legacy_lzo1x_14(
        source,
        block.storage
    );
    block.status = compressed.status;
    block.bytes_written = compressed.bytes_written;
    return block;
}

}  // namespace openswd3::resource_io
