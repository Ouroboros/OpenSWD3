#include "test.hpp"

#include "openswd3/rendering/legacy_image_command_stream.hpp"
#include "openswd3/resource_io/legacy_lmf_archive.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <vector>

namespace {

using openswd3::compat::u8;
using openswd3::compat::u16;
using openswd3::rendering::LegacyImageCommandStreamStatus;
using openswd3::rendering::LegacyPixelConversionState;
using openswd3::rendering::LegacyPixelMasks;

const std::vector<u8> kRaw16{
    0x9FU, 0x31U, 0x9FU, 0x31U, 0x33U, 0x33U, 0x44U, 0x44U,
    0x6BU, 0x02U, 0x6BU, 0x02U, 0x55U, 0x55U, 0x9FU, 0x31U,
    0x6BU, 0x02U, 0x66U, 0x66U, 0x67U, 0x66U, 0x68U, 0x66U,
};

const std::vector<u8> kStream16{
    0xFFU, 0xFFU, 0x06U, 0x00U, 0x02U, 0x00U, 0x10U, 0x00U, 0x0EU, 0x00U, 0x02U,
    0x80U, 0x02U, 0x00U, 0x33U, 0x33U, 0x44U, 0x44U, 0x02U, 0xC0U, 0x00U, 0x00U,
    0x14U, 0x80U, 0x01U, 0x00U, 0x55U, 0x55U, 0x01U, 0x80U, 0x01U, 0xC0U, 0x03U,
    0x00U, 0x66U, 0x66U, 0x67U, 0x66U, 0x68U, 0x66U, 0x00U, 0x00U, 0x00U, 0x00U,
};

const std::vector<u8> kRaw8{
    3U,
    3U,
    2U,
    4U,
    1U,
    1U,
    5U,
    3U,
    1U,
    6U,
    7U,
    8U,
};

const std::vector<u8> kStream8{
    0xFFU, 0xFFU, 0x06U, 0x00U, 0x02U, 0x00U, 0x08U, 0x00U, 0x0CU, 0x80U,
    0x02U, 0x80U, 0x02U, 0x00U, 0x02U, 0x04U, 0x02U, 0xC0U, 0x00U, 0x00U,
    0x10U, 0x80U, 0x01U, 0x00U, 0x05U, 0x01U, 0x80U, 0x01U, 0xC0U, 0x03U,
    0x00U, 0x06U, 0x07U, 0x08U, 0x00U, 0x00U, 0x00U, 0x00U,
};

[[nodiscard]] LegacyPixelConversionState rgb565_state() {
    LegacyPixelConversionState state;
    openswd3::rendering::select_legacy_pixel_conversion(
        state, LegacyPixelMasks{0xF800U, 0x07E0U, 0x001FU}
    );
    return state;
}

[[nodiscard]] std::uint64_t fnv1a64(const std::span<const u8> bytes) {
    std::uint64_t value = 14695981039346656037ULL;
    for (const u8 byte : bytes) {
        value ^= byte;
        value *= 1099511628211ULL;
    }
    return value;
}

void test_exact_encoders_and_decoders(openswd3::test::Context& test) {
    const auto encoded16 =
        openswd3::rendering::encode_legacy_image_command_stream(
            kRaw16, 6U, 2U, 16U
        );
    test.expect_equal(
        encoded16.status,
        LegacyImageCommandStreamStatus::completed,
        "sub_4014F0 word encoder status"
    );
    test.expect_equal(
        encoded16.bytes, kStream16, "sub_4014F0 exact word command stream"
    );

    const auto decoded16 =
        openswd3::rendering::decode_legacy_image_command_stream(kStream16);
    test.expect_equal(
        decoded16.status,
        LegacyImageCommandStreamStatus::completed,
        "sub_4019A0 word decoder status"
    );
    test.expect_equal(
        decoded16.bytes, kRaw16, "sub_4019A0 word decoder output"
    );

    const auto encoded8 =
        openswd3::rendering::encode_legacy_image_command_stream(
            kRaw8, 6U, 2U, 8U
        );
    test.expect_equal(
        encoded8.status,
        LegacyImageCommandStreamStatus::completed,
        "sub_4014F0 byte encoder status"
    );
    test.expect_equal(
        encoded8.bytes,
        kStream8,
        "sub_4014F0 exact byte command stream including unaligned commands"
    );

    const auto decoded8 =
        openswd3::rendering::decode_legacy_image_command_stream(kStream8);
    test.expect_equal(
        decoded8.status,
        LegacyImageCommandStreamStatus::completed,
        "sub_4019A0 byte decoder status"
    );
    test.expect_equal(decoded8.bytes, kRaw8, "sub_4019A0 byte decoder output");
}

void test_row_flag_quirk(openswd3::test::Context& test) {
    constexpr std::array<u8, 2> kStartsWithMarkerOne{1U, 2U};
    const auto starts_with_one =
        openswd3::rendering::encode_legacy_image_command_stream(
            kStartsWithMarkerOne, 2U, 1U, 8U
        );
    test.expect_equal(
        starts_with_one.bytes[8U],
        static_cast<u8>(9U),
        "byte row size includes the odd literal payload"
    );
    test.expect_equal(
        starts_with_one.bytes[9U],
        static_cast<u8>(0U),
        "byte row high bit clears only when the first value is one"
    );

    constexpr std::array<u8, 2> kStartsWithMarkerThree{3U, 2U};
    const auto starts_with_three =
        openswd3::rendering::encode_legacy_image_command_stream(
            kStartsWithMarkerThree, 2U, 1U, 8U
        );
    test.expect_equal(
        starts_with_three.bytes[9U],
        static_cast<u8>(0x80U),
        "byte marker three still sets the original row high bit"
    );
}

void test_literal_pixel_conversion(openswd3::test::Context& test) {
    std::vector<u8> converted = kStream16;
    const LegacyPixelConversionState state = rgb565_state();
    openswd3::rendering::LegacyImageCommandStreamHeader decoded_header;
    test.expect_equal(
        openswd3::rendering::
            convert_legacy_image_command_stream_literals_in_place(
                converted, state, &decoded_header
            ),
        LegacyImageCommandStreamStatus::completed,
        "sub_401B70 status"
    );
    test.expect_equal(
        decoded_header.width,
        static_cast<u16>(6U),
        "sub_401B70 optional width output"
    );
    test.expect_equal(
        decoded_header.height,
        static_cast<u16>(2U),
        "sub_401B70 optional height output"
    );
    test.expect_equal(
        decoded_header.format,
        static_cast<u16>(16U),
        "sub_401B70 optional masked depth output"
    );

    const std::vector<u8> expected{
        0xFFU, 0xFFU, 0x06U, 0x00U, 0x02U, 0x00U, 0x10U, 0x00U, 0x0EU,
        0x00U, 0x02U, 0x80U, 0x02U, 0x00U, 0x53U, 0x66U, 0x84U, 0x88U,
        0x02U, 0xC0U, 0x00U, 0x00U, 0x14U, 0x80U, 0x01U, 0x00U, 0x95U,
        0xAAU, 0x01U, 0x80U, 0x01U, 0xC0U, 0x03U, 0x00U, 0xC6U, 0xCCU,
        0xC7U, 0xCCU, 0xC8U, 0xCCU, 0x00U, 0x00U, 0x00U, 0x00U,
    };
    test.expect_equal(
        converted,
        expected,
        "sub_401B70 converts literal words and preserves every command"
    );

    const auto wrapper =
        openswd3::rendering::convert_legacy_image_command_stream(
            kStream16, {}, state
        );
    test.expect_equal(
        wrapper.status,
        LegacyImageCommandStreamStatus::completed,
        "sub_401C70 word branch status"
    );
    test.expect_equal(
        wrapper.bytes,
        expected,
        "sub_401C70 word branch delegates to literal conversion"
    );
}

void test_indexed_palette_conversion(openswd3::test::Context& test) {
    std::array<u16, 256> palette{};
    palette[2U] = 0x1234U;
    palette[4U] = 0x7C1FU;
    palette[5U] = 0x001FU;
    palette[6U] = 0x03E0U;
    palette[7U] = 0x3333U;
    palette[8U] = 0x4444U;

    const LegacyPixelConversionState state = rgb565_state();
    const auto converted =
        openswd3::rendering::convert_legacy_image_command_stream(
            kStream8, palette, state
        );
    test.expect_equal(
        converted.status,
        LegacyImageCommandStreamStatus::completed,
        "sub_401C70 indexed branch status"
    );
    test.expect_equal(
        converted.header.format,
        static_cast<u16>(16U),
        "sub_401C70 changes the stream depth to sixteen"
    );
    test.expect_equal(
        converted.bytes.size(),
        static_cast<std::size_t>(44U),
        "sub_401C70 recomputes word-stream row sizes"
    );
    test.expect_equal(
        converted.bytes[8U],
        static_cast<u8>(0x0EU),
        "sub_401C70 removes the source row high bit"
    );
    test.expect_equal(
        converted.bytes[22U],
        static_cast<u8>(0x14U),
        "sub_401C70 writes the second expanded row size"
    );
    test.expect_equal(
        converted.bytes[23U],
        static_cast<u8>(0U),
        "sub_401C70 does not copy the second row high bit"
    );

    const auto decoded =
        openswd3::rendering::decode_legacy_image_command_stream(
            converted.bytes
        );
    const std::vector<u8> expected_pixels{
        0x9FU, 0x31U, 0x9FU, 0x31U, 0x54U, 0x24U, 0x1FU, 0xF8U,
        0x6BU, 0x02U, 0x6BU, 0x02U, 0x1FU, 0x00U, 0x9FU, 0x31U,
        0x6BU, 0x02U, 0xC0U, 0x07U, 0x53U, 0x66U, 0x84U, 0x88U,
    };
    test.expect_equal(
        decoded.status,
        LegacyImageCommandStreamStatus::completed,
        "expanded indexed stream decodes"
    );
    test.expect_equal(
        decoded.bytes,
        expected_pixels,
        "indexed literals use the palette and selected forward conversion"
    );

    std::vector<u8> embedded(512U, 0U);
    for (std::size_t index = 0U; index < palette.size(); ++index) {
        embedded[index * 2U] = static_cast<u8>(palette[index] & 0x00FFU);
        embedded[index * 2U + 1U] = static_cast<u8>(palette[index] >> 8U);
    }
    embedded.insert(embedded.end(), kStream8.begin(), kStream8.end());
    const auto embedded_result = openswd3::rendering::
        convert_legacy_embedded_palette_image_command_stream(embedded, state);
    test.expect_equal(
        embedded_result.status,
        LegacyImageCommandStreamStatus::completed,
        "sub_401E50 status"
    );
    test.expect_equal(
        embedded_result.bytes,
        converted.bytes,
        "sub_401E50 embedded palette output"
    );
}

void test_boundaries(openswd3::test::Context& test) {
    const std::array<u8, 8> invalid_magic{
        0U,
        0U,
        1U,
        0U,
        1U,
        0U,
        8U,
        0U,
    };
    test.expect_equal(
        openswd3::rendering::decode_legacy_image_command_stream(invalid_magic)
            .status,
        LegacyImageCommandStreamStatus::invalid_magic,
        "decoder rejects a non-FFFF signature"
    );

    std::vector<u8> unsupported = kStream8;
    unsupported[6U] = 7U;
    test.expect_equal(
        openswd3::rendering::decode_legacy_image_command_stream(unsupported)
            .status,
        LegacyImageCommandStreamStatus::unsupported_depth,
        "decoder accepts only masked depth eight or sixteen"
    );

    std::vector<u8> truncated = kStream16;
    truncated.resize(truncated.size() - 1U);
    test.expect_equal(
        openswd3::rendering::decode_legacy_image_command_stream(truncated)
            .status,
        LegacyImageCommandStreamStatus::source_exhausted,
        "decoder requires the final row sentinel"
    );

    std::vector<u8> wrong_count = kStream8;
    wrong_count[2U] = 7U;
    test.expect_equal(
        openswd3::rendering::decode_legacy_image_command_stream(wrong_count)
            .status,
        LegacyImageCommandStreamStatus::pixel_count_mismatch,
        "decoder isolates the original destination-overrun boundary"
    );

    test.expect_equal(
        openswd3::rendering::convert_legacy_image_command_stream(
            kStream8, std::array<u16, 4>{}, LegacyPixelConversionState{}
        )
            .status,
        LegacyImageCommandStreamStatus::palette_too_small,
        "indexed conversion requires every byte-addressable palette entry"
    );

    constexpr std::array<u8, 1> kZeroWidthSource{5U};
    const auto zero_width =
        openswd3::rendering::encode_legacy_image_command_stream(
            kZeroWidthSource, 0U, 1U, 8U
        );
    test.expect_equal(
        zero_width.status,
        LegacyImageCommandStreamStatus::completed,
        "zero width preserves the original do-while encoder read"
    );
    test.expect_equal(
        openswd3::rendering::decode_legacy_image_command_stream(
            zero_width.bytes
        )
            .status,
        LegacyImageCommandStreamStatus::pixel_count_mismatch,
        "safe decoder reports the zero-width overwrite boundary"
    );

    const auto zero_height =
        openswd3::rendering::encode_legacy_image_command_stream(
            {}, 6U, 0U, 16U
        );
    test.expect_equal(
        zero_height.status,
        LegacyImageCommandStreamStatus::completed,
        "zero height emits only the header and final sentinel"
    );
    test.expect_equal(
        zero_height.bytes.size(),
        static_cast<std::size_t>(10U),
        "zero-height stream byte count"
    );
}

void test_current_lmf_command_stream(
    openswd3::test::Context& test, const std::filesystem::path& archive_path
) {
    using openswd3::resource_io::LegacyLmfIndexedObjectDirectoryStatus;
    using openswd3::resource_io::LegacyLmfMapHeaderStatus;
    using openswd3::resource_io::LegacyLmfMapLookupStatus;

    const auto lookup =
        openswd3::resource_io::legacy_lmf_lookup_map(archive_path, 72U);
    test.expect_equal(
        lookup.status,
        LegacyLmfMapLookupStatus::ready,
        "current huge.lmf contains map 72"
    );
    if (lookup.status != LegacyLmfMapLookupStatus::ready) {
        return;
    }

    const auto map_header = openswd3::resource_io::legacy_lmf_read_map_header(
        archive_path, lookup.map_offset
    );
    test.expect_equal(
        map_header.status,
        LegacyLmfMapHeaderStatus::ready,
        "current map 72 header"
    );
    if (map_header.status != LegacyLmfMapHeaderStatus::ready) {
        return;
    }

    const auto directory =
        openswd3::resource_io::legacy_lmf_read_indexed_object_directory(
            archive_path, lookup.map_offset, map_header
        );
    test.expect_equal(
        directory.status,
        LegacyLmfIndexedObjectDirectoryStatus::ready,
        "current map 72 indexed-object directory"
    );
    test.expect_false(
        directory.objects.empty(),
        "current map 72 has the command-stream object"
    );
    if (directory.status != LegacyLmfIndexedObjectDirectoryStatus::ready ||
        directory.objects.empty()) {
        return;
    }

    std::vector<u8> stream = directory.objects.front().decompressed_payload;
    openswd3::rendering::LegacyImageCommandStreamHeader decoded_header;
    test.expect_equal(
        openswd3::rendering::
            convert_legacy_image_command_stream_literals_in_place(
                stream, rgb565_state(), &decoded_header
            ),
        LegacyImageCommandStreamStatus::completed,
        "current map command stream accepts sub_401B70 conversion"
    );
    test.expect_true(
        decoded_header.width == 1072U && decoded_header.height == 1024U &&
            decoded_header.format == 16U,
        "current map command-stream header"
    );
    test.expect_equal(
        stream.size(),
        static_cast<std::size_t>(1'790'338U),
        "current map packed-stream size"
    );
    test.expect_equal(
        fnv1a64(stream),
        0xA70AE50B232B53DEULL,
        "current map RGB565 command-stream hash"
    );

    const auto decoded =
        openswd3::rendering::decode_legacy_image_command_stream(stream);
    test.expect_equal(
        decoded.status,
        LegacyImageCommandStreamStatus::completed,
        "current map converted stream decodes"
    );
    test.expect_equal(
        decoded.bytes.size(),
        static_cast<std::size_t>(2'195'456U),
        "current map decoded pixel byte count"
    );
    test.expect_equal(
        fnv1a64(decoded.bytes),
        0x3C444615B499C161ULL,
        "current map decoded RGB565 pixel hash"
    );
}

}  // namespace

int main(const int argument_count, char** arguments) {
    openswd3::test::Context test;
    test_exact_encoders_and_decoders(test);
    test_row_flag_quirk(test);
    test_literal_pixel_conversion(test);
    test_indexed_palette_conversion(test);
    test_boundaries(test);
    test.expect_true(
        argument_count == 1 || argument_count == 2,
        "the optional argument names the current huge.lmf"
    );
    if (argument_count == 2 && arguments != nullptr &&
        arguments[1] != nullptr) {
        test_current_lmf_command_stream(test, arguments[1]);
    }
    return test.exit_code();
}
