#include "test.hpp"

#include "openswd3/rendering/legacy_image_command_stream.hpp"
#include "openswd3/resource_io/legacy_lzo1x.hpp"
#include "openswd3/world_map/legacy_world_special_frame_loader.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <span>
#include <system_error>
#include <vector>

#ifndef OPENSWD3_TEST_ARTIFACT_ROOT
#error OPENSWD3_TEST_ARTIFACT_ROOT must name a build-tree directory
#endif

namespace {

using openswd3::compat::u16;
using openswd3::compat::u32;
using openswd3::compat::u8;
using openswd3::resource_io::LegacyLmfReferencedRecord;
using openswd3::world_map::LegacyWorldSpecialFrameLoader;
using openswd3::world_map::LegacyWorldSpecialFrameStatus;

class TestTree {
public:
    TestTree() {
        const auto unique =
            std::chrono::steady_clock::now().time_since_epoch().count();
        root_ = std::filesystem::path{OPENSWD3_TEST_ARTIFACT_ROOT} /
            ("legacy-world-special-frame-" + std::to_string(unique));
        std::filesystem::create_directories(root_);
    }

    ~TestTree() {
        std::error_code ignored;
        std::filesystem::remove_all(root_, ignored);
    }

    [[nodiscard]] const std::filesystem::path& root() const noexcept {
        return root_;
    }

private:
    std::filesystem::path root_;
};

void write_u16(
    const std::span<u8> bytes,
    const std::size_t offset,
    const u16 value
) {
    bytes[offset] = static_cast<u8>(value);
    bytes[offset + 1U] = static_cast<u8>(value >> 8U);
}

void write_u32(
    const std::span<u8> bytes,
    const std::size_t offset,
    const u32 value
) {
    bytes[offset] = static_cast<u8>(value);
    bytes[offset + 1U] = static_cast<u8>(value >> 8U);
    bytes[offset + 2U] = static_cast<u8>(value >> 16U);
    bytes[offset + 3U] = static_cast<u8>(value >> 24U);
}

[[nodiscard]] std::vector<u8> compress(const std::span<const u8> source) {
    std::vector<u8> compressed(source.size() * 2U + 128U);
    const auto result =
        openswd3::resource_io::compress_legacy_lzo1x_15(source, compressed);
    compressed.resize(result.bytes_written);
    return compressed;
}

[[nodiscard]] std::vector<u8> make_record(
    const std::span<const u8> payload,
    const u16 depth
) {
    const std::vector<u8> compressed = compress(payload);
    std::vector<u8> record(0x10U + compressed.size(), 0U);
    write_u16(record, 0x02U, 1U);
    write_u16(record, 0x04U, 1U);
    write_u16(record, 0x06U, depth);
    write_u32(record, 0x08U, static_cast<u32>(payload.size()));
    write_u32(record, 0x0CU, static_cast<u32>(compressed.size()));
    std::ranges::copy(compressed, record.begin() + 0x10);
    return record;
}

[[nodiscard]] openswd3::rendering::LegacyPixelConversionState
rgb565_conversion() {
    openswd3::rendering::LegacyPixelConversionState conversion;
    openswd3::rendering::select_legacy_pixel_conversion(
        conversion,
        {0xF800U, 0x07E0U, 0x001FU}
    );
    return conversion;
}

void test_direct_and_indexed_records(openswd3::test::Context& test) {
    const std::array<u8, 4U> direct_pixels{
        0x00U, 0x7CU,
        0xE0U, 0x03U,
    };
    const auto direct_stream =
        openswd3::rendering::encode_legacy_image_command_stream(
            direct_pixels,
            2U,
            1U,
            16U
        );
    test.expect_equal(
        direct_stream.status,
        openswd3::rendering::LegacyImageCommandStreamStatus::completed,
        "direct fixture encodes"
    );

    std::vector<u8> indexed_payload(512U, 0U);
    write_u16(indexed_payload, 4U, 0x7C00U);
    write_u16(indexed_payload, 8U, 0x03E0U);
    const std::array<u8, 2U> indexed_pixels{2U, 4U};
    const auto indexed_stream =
        openswd3::rendering::encode_legacy_image_command_stream(
            indexed_pixels,
            2U,
            1U,
            8U
        );
    test.expect_equal(
        indexed_stream.status,
        openswd3::rendering::LegacyImageCommandStreamStatus::completed,
        "indexed fixture encodes"
    );
    indexed_payload.insert(
        indexed_payload.end(),
        indexed_stream.bytes.begin(),
        indexed_stream.bytes.end()
    );

    const std::vector<u8> direct_record = make_record(
        direct_stream.bytes,
        16U
    );
    const std::vector<u8> indexed_record = make_record(indexed_payload, 8U);
    constexpr u32 map_offset = 0x20U;
    constexpr u32 direct_relative_offset = 0x20U;
    constexpr u32 indexed_relative_offset = 0x200U;
    std::vector<u8> archive(
        map_offset + indexed_relative_offset + indexed_record.size(),
        0U
    );
    std::ranges::copy(
        direct_record,
        archive.begin() + map_offset + direct_relative_offset
    );
    std::ranges::copy(
        indexed_record,
        archive.begin() + map_offset + indexed_relative_offset
    );

    TestTree tree;
    const auto archive_path = tree.root() / "huge.lmf";
    std::ofstream output(archive_path, std::ios::binary);
    output.write(
        reinterpret_cast<const char*>(archive.data()),
        static_cast<std::streamsize>(archive.size())
    );
    output.close();

    const std::array<LegacyLmfReferencedRecord, 2U> records{
        LegacyLmfReferencedRecord{
            .relative_offset = direct_relative_offset,
            .field_0c = static_cast<u32>(direct_record.size() - 0x10U),
        },
        LegacyLmfReferencedRecord{
            .relative_offset = indexed_relative_offset,
            .field_0c = static_cast<u32>(indexed_record.size() - 0x10U),
        },
    };
    LegacyWorldSpecialFrameLoader loader{
        archive_path,
        map_offset,
        records,
        rgb565_conversion(),
    };

    openswd3::asset_runtime::LegacyTswRuntimeFrame direct_frame;
    test.expect_true(
        loader.load_special_frame(0U, direct_frame),
        "sub_40AD10 direct-16 record loads"
    );
    const auto decoded_direct =
        openswd3::rendering::decode_legacy_image_command_stream(
            direct_frame.primary_stream
        );
    test.expect_true(
        loader.last_status() == LegacyWorldSpecialFrameStatus::ready &&
            direct_frame.width == 2U && direct_frame.height == 1U &&
            decoded_direct.status == openswd3::rendering::
                                         LegacyImageCommandStreamStatus::completed &&
            decoded_direct.bytes ==
                std::vector<u8>{0x00U, 0xF8U, 0xC0U, 0x07U},
        "direct literals use the active RGB555-to-RGB565 conversion"
    );

    openswd3::asset_runtime::LegacyTswRuntimeFrame indexed_frame;
    test.expect_true(
        loader.load_special_frame(1U, indexed_frame),
        "sub_40AD10 embedded-palette record loads"
    );
    const auto decoded_indexed =
        openswd3::rendering::decode_legacy_image_command_stream(
            indexed_frame.primary_stream
        );
    test.expect_true(
        indexed_frame.width == 2U && indexed_frame.height == 1U &&
            decoded_indexed.status == openswd3::rendering::
                                          LegacyImageCommandStreamStatus::completed &&
            decoded_indexed.bytes ==
                std::vector<u8>{0x00U, 0xF8U, 0xC0U, 0x07U},
        "embedded palette expands to the same converted word stream"
    );

    openswd3::asset_runtime::LegacyTswRuntimeFrame rejected_frame;
    test.expect_true(
        !loader.load_special_frame(2U, rejected_frame) &&
            loader.last_status() ==
                LegacyWorldSpecialFrameStatus::variant_out_of_range,
        "unsafe original count-equal access is bounded at the platform edge"
    );
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_direct_and_indexed_records(test);
    return test.exit_code();
}
