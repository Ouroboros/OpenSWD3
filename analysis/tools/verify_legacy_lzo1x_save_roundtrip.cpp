#include "openswd3/resource_io/legacy_lzo1x.hpp"

#include <algorithm>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

using openswd3::compat::u8;
using openswd3::compat::u32;
using openswd3::resource_io::LegacyLzo1xResult;
using openswd3::resource_io::LegacyLzo1xStatus;

constexpr std::size_t kFixedPrefixSize = 0x962CU;
constexpr std::size_t kRawAfterPrimaryState = 0x1CU;
constexpr std::size_t kExtensionSize = 0x84U + 0x180U;
constexpr std::size_t kRawAfterFame = 0x84U;
constexpr std::size_t kExpectedSaveCount = 40U;
constexpr std::size_t kExpectedNormalBlockCount = 160U;
constexpr std::size_t kExpectedFameBlockCount = 40U;

enum class CompressorKind {
    dictionary_14,
    dictionary_15,
};

struct Block {
    std::size_t payload_offset{};
    u32 compressed_size{};
    u32 decompressed_size{};
    CompressorKind compressor{};
};

struct Totals {
    std::size_t save_count{};
    std::size_t normal_blocks{};
    std::size_t fame_blocks{};
    std::size_t exact_normal_blocks{};
    std::size_t exact_fame_blocks{};
    std::uint64_t original_compressed_bytes{};
    std::uint64_t recompressed_bytes{};
    std::uint64_t decompressed_bytes{};
    std::uint64_t original_dictionary_14_hash{0xCBF29CE484222325ULL};
    std::uint64_t original_dictionary_15_hash{0xCBF29CE484222325ULL};
    std::uint64_t dictionary_14_hash{0xCBF29CE484222325ULL};
    std::uint64_t dictionary_15_hash{0xCBF29CE484222325ULL};
};

int report_error(const std::string_view message) {
    std::cerr << "verify_legacy_lzo1x_save_roundtrip: " << message << '\n';
    return 1;
}

[[nodiscard]] bool
read_file(const std::filesystem::path& path, std::vector<u8>& bytes) {
    std::ifstream input(path, std::ios::binary | std::ios::ate);
    if (!input) {
        return false;
    }

    const std::streampos end = input.tellg();
    if (end < 0) {
        return false;
    }

    bytes.resize(static_cast<std::size_t>(end));
    input.seekg(0);
    input.read(
        reinterpret_cast<char*>(bytes.data()),
        static_cast<std::streamsize>(bytes.size())
    );
    return input.gcount() == static_cast<std::streamsize>(bytes.size());
}

[[nodiscard]] bool read_u32(
    const std::span<const u8> bytes, const std::size_t offset, u32& value
) noexcept {
    if (offset > bytes.size() || bytes.size() - offset < 4U) {
        return false;
    }

    value = static_cast<u32>(bytes[offset]) |
        (static_cast<u32>(bytes[offset + 1U]) << 8U) |
        (static_cast<u32>(bytes[offset + 2U]) << 16U) |
        (static_cast<u32>(bytes[offset + 3U]) << 24U);
    return true;
}

[[nodiscard]] bool append_standard_block(
    const std::span<const u8> bytes,
    const std::size_t header_offset,
    std::vector<Block>& blocks,
    std::size_t& block_end
) {
    u32 compressed_size{};
    u32 decompressed_size{};
    if (!read_u32(bytes, header_offset, compressed_size) ||
        !read_u32(bytes, header_offset + 4U, decompressed_size)) {
        return false;
    }

    const std::size_t payload_offset = header_offset + 8U;
    if (payload_offset > bytes.size() ||
        compressed_size > bytes.size() - payload_offset) {
        return false;
    }

    block_end = payload_offset + compressed_size;
    blocks.push_back(
        Block{
            payload_offset,
            compressed_size,
            decompressed_size,
            CompressorKind::dictionary_14,
        }
    );
    return true;
}

[[nodiscard]] bool
parse_blocks(const std::span<const u8> bytes, std::vector<Block>& blocks) {
    std::size_t block_end{};
    if (!append_standard_block(bytes, kFixedPrefixSize, blocks, block_end)) {
        return false;
    }

    if (!append_standard_block(bytes, block_end, blocks, block_end)) {
        return false;
    }

    if (!append_standard_block(
            bytes, block_end + kRawAfterPrimaryState, blocks, block_end
        )) {
        return false;
    }

    const std::size_t fame_outer_header = block_end + kExtensionSize;
    u32 fame_record_size{};
    if (!read_u32(bytes, fame_outer_header, fame_record_size)) {
        return false;
    }

    const std::size_t fame_record = fame_outer_header + 4U;
    u32 fame_decompressed_size{};
    u32 fame_compressed_size{};
    if (!read_u32(bytes, fame_record + 2U, fame_decompressed_size) ||
        !read_u32(bytes, fame_record + 6U, fame_compressed_size) ||
        fame_record_size != fame_compressed_size + 10U) {
        return false;
    }

    const std::size_t fame_payload = fame_record + 10U;
    if (fame_payload > bytes.size() ||
        fame_compressed_size > bytes.size() - fame_payload) {
        return false;
    }

    blocks.push_back(
        Block{
            fame_payload,
            fame_compressed_size,
            fame_decompressed_size,
            CompressorKind::dictionary_15,
        }
    );

    const std::size_t tail_header =
        fame_record + fame_record_size + kRawAfterFame;
    if (!append_standard_block(bytes, tail_header, blocks, block_end)) {
        return false;
    }

    return block_end == bytes.size() && blocks.size() == 5U;
}

[[nodiscard]] std::size_t
compression_capacity(const std::size_t source_size) noexcept {
    return source_size + source_size / 16U + 67U;
}

[[nodiscard]] std::size_t original_capacity(
    const std::size_t source_size, const CompressorKind kind
) noexcept {
    if (kind == CompressorKind::dictionary_14) {
        return source_size + 0x20U;
    }

    return source_size + source_size / 64U + 0x13U;
}

[[nodiscard]] LegacyLzo1xResult compress(
    const std::span<const u8> source,
    const CompressorKind kind,
    const std::span<u8> destination
) noexcept {
    if (kind == CompressorKind::dictionary_14) {
        return openswd3::resource_io::compress_legacy_lzo1x_14(
            source, destination
        );
    }

    return openswd3::resource_io::compress_legacy_lzo1x_15(source, destination);
}

void update_hash(
    std::uint64_t& hash, const std::span<const u8> bytes
) noexcept {
    for (const u8 byte : bytes) {
        hash ^= byte;
        hash *= 0x100000001B3ULL;
    }
}

[[nodiscard]] bool verify_block(
    const std::span<const u8> file_bytes, const Block& block, Totals& totals
) {
    const std::span<const u8> original_compressed =
        file_bytes.subspan(block.payload_offset, block.compressed_size);
    std::vector<u8> decompressed(block.decompressed_size);
    const LegacyLzo1xResult original_result =
        openswd3::resource_io::decompress_legacy_lzo1x(
            original_compressed, decompressed
        );
    if (original_result.status != LegacyLzo1xStatus::success ||
        original_result.bytes_written != block.decompressed_size) {
        return false;
    }

    std::vector<u8> recompressed(compression_capacity(decompressed.size()));
    const LegacyLzo1xResult compression_result =
        compress(decompressed, block.compressor, recompressed);
    if (compression_result.status != LegacyLzo1xStatus::success ||
        compression_result.bytes_written >
            original_capacity(decompressed.size(), block.compressor)) {
        return false;
    }

    recompressed.resize(compression_result.bytes_written);
    std::vector<u8> restored(decompressed.size());
    const LegacyLzo1xResult restored_result =
        openswd3::resource_io::decompress_legacy_lzo1x(recompressed, restored);
    if (restored_result.status != LegacyLzo1xStatus::success ||
        restored_result.bytes_written != decompressed.size() ||
        restored != decompressed) {
        return false;
    }

    ++(block.compressor == CompressorKind::dictionary_14 ? totals.normal_blocks
                                                         : totals.fame_blocks);
    totals.original_compressed_bytes += original_compressed.size();
    totals.recompressed_bytes += recompressed.size();
    totals.decompressed_bytes += decompressed.size();
    update_hash(
        block.compressor == CompressorKind::dictionary_14
            ? totals.original_dictionary_14_hash
            : totals.original_dictionary_15_hash,
        original_compressed
    );
    update_hash(
        block.compressor == CompressorKind::dictionary_14
            ? totals.dictionary_14_hash
            : totals.dictionary_15_hash,
        recompressed
    );
    if (std::ranges::equal(recompressed, original_compressed)) {
        ++(block.compressor == CompressorKind::dictionary_14
               ? totals.exact_normal_blocks
               : totals.exact_fame_blocks);
    }

    return true;
}

[[nodiscard]] bool
parse_slot_number(const std::filesystem::path& path, unsigned& slot) {
    const std::string stem = path.stem().string();
    const char* const begin = stem.data();
    const char* const end = begin + stem.size();
    const auto result = std::from_chars(begin, end, slot);
    return result.ec == std::errc{} && result.ptr == end;
}

[[nodiscard]] std::vector<std::filesystem::path>
find_saves(const std::filesystem::path& workspace_root) {
    std::vector<std::pair<unsigned, std::filesystem::path>> ordered;
    unsigned directory_order = 0U;
    for (const std::string_view directory_name : {"Save", "Save1"}) {
        const std::filesystem::path directory = workspace_root / directory_name;
        if (!std::filesystem::is_directory(directory)) {
            return {};
        }

        for (const std::filesystem::directory_entry& entry :
             std::filesystem::directory_iterator(directory)) {
            if (!entry.is_regular_file() ||
                entry.path().extension() != ".sav") {
                continue;
            }

            unsigned slot{};
            if (!parse_slot_number(entry.path(), slot)) {
                continue;
            }

            ordered.emplace_back(directory_order * 1000U + slot, entry.path());
        }

        ++directory_order;
    }

    std::ranges::sort(
        ordered, {}, &std::pair<unsigned, std::filesystem::path>::first
    );
    std::vector<std::filesystem::path> paths;
    paths.reserve(ordered.size());
    for (auto& [key, path] : ordered) {
        static_cast<void>(key);
        paths.push_back(std::move(path));
    }

    return paths;
}

}  // namespace

int main(const int argument_count, char** arguments) {
    if (argument_count != 2) {
        return report_error("usage: <workspace-root>");
    }

    const std::filesystem::path workspace_root = arguments[1];
    const std::vector<std::filesystem::path> save_paths =
        find_saves(workspace_root);
    if (save_paths.size() != kExpectedSaveCount) {
        return report_error("unexpected save file count");
    }

    Totals totals{};
    for (const std::filesystem::path& save_path : save_paths) {
        std::vector<u8> file_bytes;
        if (!read_file(save_path, file_bytes)) {
            return report_error("cannot read a save file");
        }

        std::vector<Block> blocks;
        if (!parse_blocks(file_bytes, blocks)) {
            return report_error("invalid save container layout");
        }

        for (const Block& block : blocks) {
            if (!verify_block(file_bytes, block, totals)) {
                return report_error("save compression round-trip mismatch");
            }
        }

        ++totals.save_count;
    }

    if (totals.normal_blocks != kExpectedNormalBlockCount ||
        totals.fame_blocks != kExpectedFameBlockCount) {
        return report_error("unexpected compression block counts");
    }

    if (totals.exact_normal_blocks != kExpectedNormalBlockCount ||
        totals.exact_fame_blocks != kExpectedFameBlockCount) {
        return report_error("recompressed bytes differ from original streams");
    }

    std::cout << "saves=" << totals.save_count
              << " normal_blocks=" << totals.normal_blocks
              << " fame_blocks=" << totals.fame_blocks
              << " exact_normal=" << totals.exact_normal_blocks
              << " exact_fame=" << totals.exact_fame_blocks
              << " original_compressed=" << totals.original_compressed_bytes
              << " recompressed=" << totals.recompressed_bytes
              << " decompressed=" << totals.decompressed_bytes
              << " original_hash14=0x" << std::hex
              << totals.original_dictionary_14_hash << " recompressed_hash14=0x"
              << totals.dictionary_14_hash << " original_hash15=0x"
              << totals.original_dictionary_15_hash << " recompressed_hash15=0x"
              << totals.dictionary_15_hash << '\n';
    return 0;
}
