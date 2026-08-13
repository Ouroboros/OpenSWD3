#include "openswd3/resource_io/legacy_lzo1x.hpp"

#include <algorithm>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#if defined(_WIN32)
#include <fcntl.h>
#include <io.h>
#endif

namespace {

struct DescriptorColumns {
    std::size_t archive{};
    std::size_t payload_offset{};
    std::size_t compressed_size{};
    std::size_t decompressed_size{};
};

[[nodiscard]] std::vector<std::string_view>
split_tsv(const std::string_view line) {
    std::vector<std::string_view> fields;
    std::size_t start = 0U;
    while (start <= line.size()) {
        const std::size_t end = line.find('\t', start);
        if (end == std::string_view::npos) {
            fields.push_back(line.substr(start));
            break;
        }

        fields.push_back(line.substr(start, end - start));
        start = end + 1U;
    }

    return fields;
}

[[nodiscard]] bool find_column(
    const std::span<const std::string_view> fields,
    const std::string_view name,
    std::size_t& index
) {
    for (std::size_t candidate = 0U; candidate < fields.size(); ++candidate) {
        if (fields[candidate] == name) {
            index = candidate;
            return true;
        }
    }

    return false;
}

[[nodiscard]] bool parse_u32(
    const std::string_view text, const int base, openswd3::compat::u32& value
) {
    const char* const begin = text.data();
    const char* const end = begin + text.size();
    const auto result = std::from_chars(begin, end, value, base);
    return result.ec == std::errc{} && result.ptr == end;
}

[[nodiscard]] bool
read_columns(const std::string_view header, DescriptorColumns& columns) {
    const std::vector<std::string_view> fields = split_tsv(header);
    return find_column(fields, "archive", columns.archive) &&
        find_column(
               fields, "payload_absolute_offset_hex", columns.payload_offset
        ) &&
        find_column(fields, "compressed_size_bytes", columns.compressed_size) &&
        find_column(
               fields,
               "declared_decompressed_size_bytes",
               columns.decompressed_size
        );
}

[[nodiscard]] bool set_binary_stdout() {
#if defined(_WIN32)
    return _setmode(_fileno(stdout), _O_BINARY) != -1;
#else
    return true;
#endif
}

int report_error(const std::string_view message) {
    std::cerr << "verify_legacy_lzo1x_tsw: " << message << '\n';
    return 1;
}

}  // namespace

int main(const int argument_count, char** arguments) {
    if (argument_count != 3) {
        return report_error("usage: <workspace-root> <archive-name>");
    }

    const std::filesystem::path workspace_root = arguments[1];
    const std::string_view requested_archive = arguments[2];
    const std::filesystem::path descriptor_path = workspace_root /
        "OpenSWD3/analysis/04-reverse-engineering/inventory/" "tsw-frame-descriptors.tsv";
    const std::filesystem::path archive_path =
        workspace_root / requested_archive;

    std::ifstream descriptors(descriptor_path);
    if (!descriptors) {
        return report_error("cannot open the TSW descriptor inventory");
    }

    std::ifstream archive(archive_path, std::ios::binary);
    if (!archive) {
        return report_error("cannot open the requested TSW archive");
    }

    std::string line;
    if (!std::getline(descriptors, line)) {
        return report_error("empty TSW descriptor inventory");
    }

    DescriptorColumns columns{};
    if (!read_columns(line, columns)) {
        return report_error("required TSW descriptor columns are missing");
    }

    if (!set_binary_stdout()) {
        return report_error("cannot set stdout to binary mode");
    }

    std::size_t frame_count = 0U;
    std::uint64_t compressed_bytes = 0U;
    std::uint64_t decompressed_bytes = 0U;
    while (std::getline(descriptors, line)) {
        const std::vector<std::string_view> fields = split_tsv(line);
        const std::size_t maximum_column = std::max(
            std::max(columns.archive, columns.payload_offset),
            std::max(columns.compressed_size, columns.decompressed_size)
        );
        if (fields.size() <= maximum_column) {
            return report_error("malformed TSW descriptor row");
        }

        if (fields[columns.archive] != requested_archive) {
            continue;
        }

        openswd3::compat::u32 payload_offset{};
        openswd3::compat::u32 compressed_size{};
        openswd3::compat::u32 decompressed_size{};
        if (!parse_u32(fields[columns.payload_offset], 16, payload_offset) ||
            !parse_u32(fields[columns.compressed_size], 10, compressed_size) ||
            !parse_u32(
                fields[columns.decompressed_size], 10, decompressed_size
            )) {
            return report_error("invalid numeric TSW descriptor field");
        }

        archive.clear();
        archive.seekg(static_cast<std::streamoff>(payload_offset));
        std::vector<openswd3::compat::u8> compressed(compressed_size);
        archive.read(
            reinterpret_cast<char*>(compressed.data()),
            static_cast<std::streamsize>(compressed.size())
        );
        if (archive.gcount() !=
            static_cast<std::streamsize>(compressed.size())) {
            return report_error("short TSW payload read");
        }

        std::vector<openswd3::compat::u8> decompressed(decompressed_size);
        openswd3::compat::u32 actual_output_size{};
        const openswd3::resource_io::LegacyLzo1xStatus status =
            openswd3::resource_io::decompress_legacy_resource_block(
                compressed, decompressed, actual_output_size
            );
        if (status != openswd3::resource_io::LegacyLzo1xStatus::success ||
            actual_output_size != decompressed_size) {
            return report_error("C++ wrapper rejected a declared TSW frame");
        }

        std::cout.write(
            reinterpret_cast<const char*>(decompressed.data()),
            static_cast<std::streamsize>(decompressed.size())
        );
        if (!std::cout) {
            return report_error("failed to stream decompressed bytes");
        }

        ++frame_count;
        compressed_bytes += compressed_size;
        decompressed_bytes += decompressed_size;
    }

    if (frame_count == 0U) {
        return report_error("requested archive has no descriptor rows");
    }

    std::cerr << requested_archive << ": frames=" << frame_count
              << " compressed=" << compressed_bytes
              << " decompressed=" << decompressed_bytes << '\n';
    return 0;
}
