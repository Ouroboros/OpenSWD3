#include "openswd3/resource_io/legacy_environment.hpp"

#include "openswd3/resource_io/legacy_file.hpp"

#include <algorithm>
#include <array>
#include <limits>
#include <string_view>
#include <utility>

namespace openswd3::resource_io {
namespace {

constexpr compat::u32 kMarkedLayoutMarker = 0xFFFFFFFFU;
constexpr std::size_t kMarkedFieldOffset = 4U;
constexpr std::size_t kUnmarkedFieldOffset = 0U;
constexpr std::size_t kBindingBytesSize = 16U;
constexpr std::size_t kIntegerParameterRelativeOffset = 0x10U;
constexpr std::size_t kOptionBytesRelativeOffset = 0x14U;
constexpr std::size_t kPreservedBytesRelativeOffset = 0x1AU;
constexpr std::size_t kDirectoryRelativeOffset = 0x2AU;
constexpr std::size_t kEncodedFixedSize = 0x32U;

// 0x00424390 initializes the runtime dwords. 0x00423AF0 serializes their
// low bytes in this file order.
constexpr std::array<compat::u8, kBindingBytesSize>
    kDefaultBindingBytesFileOrder{
        0xC8U, 0xD0U, 0xCBU, 0xCDU,
        0x39U, 0x1CU, 0x9DU, 0x01U,
        0xCFU, 0x36U, 0x13U, 0x1EU,
        0x22U, 0x3BU, 0xC9U, 0xD1U,
    };

using EnvironmentWindow =
    std::array<compat::u8, kLegacyEnvironmentWindowSize>;

struct FileReadResult {
    bool opened{};
    LegacyEnvironmentCodecStatus status{LegacyEnvironmentCodecStatus::ok};
};

[[nodiscard]] compat::u32 read_u32(
    const EnvironmentWindow& window,
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
    const EnvironmentWindow& window,
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

[[nodiscard]] LegacyEnvironmentDecodeResult decode_window(
    const EnvironmentWindow& window,
    const LegacyEnvironmentLayout layout
) {
    LegacyEnvironmentDecodeResult result;
    result.layout = layout;
    const std::size_t field_offset =
        layout == LegacyEnvironmentLayout::marked
        ? kMarkedFieldOffset
        : kUnmarkedFieldOffset;

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

[[nodiscard]] FileReadResult read_environment_window(
    const std::filesystem::path& path,
    EnvironmentWindow& window
) {
    LegacyFile file;
    if (!file.open(
            path,
            LegacyFileCreation::open_existing,
            LegacyFileAccess::read
        )) {
        return {};
    }

    FileReadResult result{.opened = true};
    const compat::u32 file_size = file.size();
    if (file_size == std::numeric_limits<compat::u32>::max() ||
        file_size > kLegacyEnvironmentWindowSize) {
        result.status = LegacyEnvironmentCodecStatus::input_too_large;
        static_cast<void>(file.close());
        return result;
    }

    compat::u32 requested = file_size;
    static_cast<void>(file.read(
        std::span<compat::u8>{window}.first(file_size),
        requested
    ));
    static_cast<void>(file.close());
    return result;
}

[[nodiscard]] bool write_environment_prefix(
    const std::filesystem::path& path,
    const std::span<const compat::u8> bytes
) {
    LegacyFile file;
    if (!file.open(
            path,
            LegacyFileCreation::open_existing,
            LegacyFileAccess::read_write
        )) {
        return false;
    }

    static_cast<void>(file.seek_begin_one_based(0));
    compat::u32 requested = static_cast<compat::u32>(bytes.size());
    const bool written = file.write(bytes, requested);
    static_cast<void>(file.close());
    return written;
}

}  // namespace

LegacyEnvironmentDecodeResult decode_legacy_environment(
    const std::span<const compat::u8> bytes
) {
    if (bytes.size() > kLegacyEnvironmentWindowSize) {
        LegacyEnvironmentDecodeResult result;
        result.status = LegacyEnvironmentCodecStatus::input_too_large;
        return result;
    }

    EnvironmentWindow window{};
    std::ranges::copy(bytes, window.begin());

    const bool has_marker = read_u32(window, 0U) == kMarkedLayoutMarker;
    return decode_window(
        window,
        has_marker
        ? LegacyEnvironmentLayout::marked
        : LegacyEnvironmentLayout::unmarked
    );
}

LegacyEnvironmentDecodeResult decode_legacy_environment_as(
    const std::span<const compat::u8> bytes,
    const LegacyEnvironmentLayout layout
) {
    if (bytes.size() > kLegacyEnvironmentWindowSize) {
        LegacyEnvironmentDecodeResult result;
        result.layout = layout;
        result.status = LegacyEnvironmentCodecStatus::input_too_large;
        return result;
    }

    EnvironmentWindow window{};
    std::ranges::copy(bytes, window.begin());
    return decode_window(window, layout);
}

LegacyEnvironmentRecord migrate_unmarked_environment(
    const LegacyEnvironmentRecord& unmarked_record
) {
    LegacyEnvironmentRecord migrated;
    migrated.binding_bytes = kDefaultBindingBytesFileOrder;
    migrated.integer_parameter = 100U;
    migrated.option_bytes = {6U, 6U, 0x3CU, 1U, 2U, 0x0AU};
    migrated.primary_directory = unmarked_record.primary_directory;
    migrated.secondary_directory = unmarked_record.secondary_directory;
    migrated.trailing_mode = unmarked_record.trailing_mode;
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
    write_u32(result.bytes, 0U, kMarkedLayoutMarker);
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

LegacyEnvironmentLoadResult load_legacy_environment(
    const std::filesystem::path& initial_file,
    const LegacyEnvironmentDirectoryResolver& resolve_stored_directory,
    LegacyEnvironmentRecord& in_out_record
) {
    LegacyEnvironmentLoadResult result;
    EnvironmentWindow window{};
    const FileReadResult initial_read = read_environment_window(
        initial_file,
        window
    );
    if (!initial_read.opened) {
        return result;
    }
    if (initial_read.status != LegacyEnvironmentCodecStatus::ok) {
        result.status = LegacyEnvironmentLoadStatus::unsafe_record;
        result.codec_status = initial_read.status;
        return result;
    }

    LegacyEnvironmentDecodeResult decoded = decode_legacy_environment(window);
    if (decoded.status != LegacyEnvironmentCodecStatus::ok) {
        result.status = LegacyEnvironmentLoadStatus::unsafe_record;
        result.codec_status = decoded.status;
        return result;
    }
    if (decoded.layout == LegacyEnvironmentLayout::marked) {
        in_out_record = std::move(decoded.record);
        result.status = LegacyEnvironmentLoadStatus::marked_layout_loaded;
        result.original_return_value = true;
        return result;
    }

    const LegacyEnvironmentRecord migrated = migrate_unmarked_environment(
        decoded.record
    );
    const LegacyEnvironmentEncodeResult encoded = encode_legacy_environment(
        migrated
    );
    if (encoded.status != LegacyEnvironmentCodecStatus::ok) {
        result.status = LegacyEnvironmentLoadStatus::unsafe_record;
        result.codec_status = encoded.status;
        return result;
    }
    result.migration_write_succeeded = write_environment_prefix(
        initial_file,
        encoded.bytes
    );

    const std::filesystem::path migrated_file =
        resolve_stored_directory(decoded.record.primary_directory) /
        "Env.dat";
    const FileReadResult migrated_read = read_environment_window(
        migrated_file,
        window
    );
    result.migrated_reopen_succeeded = migrated_read.opened;
    if (migrated_read.opened &&
        migrated_read.status != LegacyEnvironmentCodecStatus::ok) {
        result.status = LegacyEnvironmentLoadStatus::unsafe_record;
        result.codec_status = migrated_read.status;
        return result;
    }

    decoded = decode_window(window, LegacyEnvironmentLayout::marked);
    if (decoded.status != LegacyEnvironmentCodecStatus::ok) {
        result.status = LegacyEnvironmentLoadStatus::unsafe_record;
        result.codec_status = decoded.status;
        return result;
    }
    in_out_record = std::move(decoded.record);
    result.status = LegacyEnvironmentLoadStatus::unmarked_layout_migrated;
    return result;
}

bool write_legacy_environment_cache_session_marker(
    const std::filesystem::path& environment_file,
    const LegacyEnvironmentCacheSessionMarker marker
) {
    LegacyFile file;
    if (!file.open(
            environment_file,
            LegacyFileCreation::open_existing,
            LegacyFileAccess::write
        )) {
        return false;
    }

    static_cast<void>(file.seek_end_one_based(-1));
    const bool written = file.write_u8(static_cast<compat::u8>(marker));
    static_cast<void>(file.close());
    return written;
}

}  // namespace openswd3::resource_io
