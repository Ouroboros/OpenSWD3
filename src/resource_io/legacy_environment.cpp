#include "openswd3/resource_io/legacy_environment.hpp"

#include <algorithm>
#include <array>
#include <string_view>

namespace openswd3::resource_io {
namespace {

constexpr compat::u32 kCurrentLayoutMarker = 0xFFFFFFFFU;
constexpr std::size_t kCurrentFieldOffset = 4U;
constexpr std::size_t kLegacyFieldOffset = 0U;
constexpr std::size_t kBindingBytesSize = 16U;
constexpr std::size_t kIntegerParameterRelativeOffset = 0x10U;
constexpr std::size_t kOptionBytesRelativeOffset = 0x14U;
constexpr std::size_t kPreservedBytesRelativeOffset = 0x1AU;
constexpr std::size_t kDirectoryRelativeOffset = 0x2AU;
constexpr std::size_t kEncodedFixedSize = 0x32U;

[[nodiscard]] compat::u32 read_u32(
    const std::array<compat::u8, kLegacyEnvironmentWindowSize>& window,
    const std::size_t offset
) noexcept {
    return static_cast<compat::u32>(window[offset]) |
        (static_cast<compat::u32>(window[offset + 1U]) << 8U) |
        (static_cast<compat::u32>(window[offset + 2U]) << 16U) |
        (static_cast<compat::u32>(window[offset + 3U]) << 24U);
}

void write_u32(
    std::vector<compat::u8>& bytes,
    const std::size_t offset,
    const compat::u32 value
) noexcept {
    bytes[offset] = static_cast<compat::u8>(value);
    bytes[offset + 1U] = static_cast<compat::u8>(value >> 8U);
    bytes[offset + 2U] = static_cast<compat::u8>(value >> 16U);
    bytes[offset + 3U] = static_cast<compat::u8>(value >> 24U);
}

[[nodiscard]] std::size_t find_terminator(
    const std::array<compat::u8, kLegacyEnvironmentWindowSize>& window,
    const std::size_t begin
) noexcept {
    const auto found = std::find(
        window.begin() + static_cast<std::ptrdiff_t>(begin),
        window.end(),
        compat::u8{}
    );
    return static_cast<std::size_t>(found - window.begin());
}

[[nodiscard]] std::string_view legacy_string_prefix(
    const std::string& value
) noexcept {
    const std::size_t terminator = value.find('\0');
    return std::string_view{value}.substr(0U, terminator);
}

}  // namespace

LegacyEnvironmentDecodeResult decode_legacy_environment(
    const std::span<const compat::u8> bytes
) {
    LegacyEnvironmentDecodeResult result;
    if (bytes.size() > kLegacyEnvironmentWindowSize) {
        result.status = LegacyEnvironmentCodecStatus::input_too_large;
        return result;
    }

    std::array<compat::u8, kLegacyEnvironmentWindowSize> window{};
    std::ranges::copy(bytes, window.begin());

    const bool current_layout = read_u32(window, 0U) == kCurrentLayoutMarker;
    result.layout = current_layout
        ? LegacyEnvironmentLayout::current
        : LegacyEnvironmentLayout::legacy_without_marker;
    const std::size_t field_offset = current_layout
        ? kCurrentFieldOffset
        : kLegacyFieldOffset;

    std::ranges::copy_n(
        window.begin() + static_cast<std::ptrdiff_t>(field_offset),
        kBindingBytesSize,
        result.record.binding_bytes.begin()
    );
    result.record.integer_parameter = read_u32(
        window,
        field_offset + kIntegerParameterRelativeOffset
    );
    std::ranges::copy_n(
        window.begin() + static_cast<std::ptrdiff_t>(
            field_offset + kOptionBytesRelativeOffset
        ),
        result.record.option_bytes.size(),
        result.record.option_bytes.begin()
    );
    std::ranges::copy_n(
        window.begin() + static_cast<std::ptrdiff_t>(
            field_offset + kPreservedBytesRelativeOffset
        ),
        result.record.preserved_bytes.size(),
        result.record.preserved_bytes.begin()
    );

    const std::size_t primary_begin = field_offset + kDirectoryRelativeOffset;
    const std::size_t primary_end = find_terminator(window, primary_begin);
    if (primary_end == window.size()) {
        result.status =
            LegacyEnvironmentCodecStatus::unterminated_primary_directory;
        return result;
    }
    result.record.primary_directory.assign(
        reinterpret_cast<const char*>(window.data() + primary_begin),
        primary_end - primary_begin
    );

    const std::size_t secondary_begin = primary_end + 1U;
    const std::size_t secondary_end = find_terminator(window, secondary_begin);
    if (secondary_end == window.size()) {
        result.status =
            LegacyEnvironmentCodecStatus::unterminated_secondary_directory;
        return result;
    }
    result.record.secondary_directory.assign(
        reinterpret_cast<const char*>(window.data() + secondary_begin),
        secondary_end - secondary_begin
    );

    if (secondary_end + 1U >= window.size()) {
        result.status = LegacyEnvironmentCodecStatus::missing_trailing_mode;
        return result;
    }
    result.record.trailing_mode = window[secondary_end + 1U];
    return result;
}

LegacyEnvironmentRecord migrate_legacy_environment(
    const LegacyEnvironmentRecord& legacy_record
) {
    LegacyEnvironmentRecord migrated;
    migrated.integer_parameter = 100U;
    migrated.option_bytes = {6U, 6U, 0x3CU, 1U, 2U, 0x0AU};
    migrated.primary_directory = legacy_record.primary_directory;
    migrated.secondary_directory = legacy_record.secondary_directory;
    migrated.trailing_mode = legacy_record.trailing_mode;
    return migrated;
}

LegacyEnvironmentEncodeResult encode_legacy_environment(
    const LegacyEnvironmentRecord& record
) {
    LegacyEnvironmentEncodeResult result;
    const std::string_view primary = legacy_string_prefix(
        record.primary_directory
    );
    const std::string_view secondary = legacy_string_prefix(
        record.secondary_directory
    );
    if (primary.size() + secondary.size() >
        kLegacyEnvironmentWindowSize - kEncodedFixedSize) {
        result.status = LegacyEnvironmentCodecStatus::output_too_large;
        return result;
    }

    result.bytes.resize(kEncodedFixedSize + primary.size() + secondary.size());
    write_u32(result.bytes, 0U, kCurrentLayoutMarker);
    std::ranges::copy(record.binding_bytes, result.bytes.begin() + 4);
    write_u32(result.bytes, 0x14U, record.integer_parameter);
    std::ranges::copy(record.option_bytes, result.bytes.begin() + 0x18);
    std::ranges::copy(record.preserved_bytes, result.bytes.begin() + 0x1E);

    auto output = result.bytes.begin() + 0x2E;
    output = std::ranges::copy(primary, output).out;
    *output++ = 0U;
    output = std::ranges::copy(secondary, output).out;
    *output++ = 0U;
    *output++ = record.trailing_mode;
    *output = 0U;
    return result;
}

}  // namespace openswd3::resource_io
