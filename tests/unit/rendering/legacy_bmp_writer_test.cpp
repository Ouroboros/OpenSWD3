#include "test.hpp"

#include "openswd3/rendering/legacy_bmp_writer.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace {

using openswd3::compat::u8;
using openswd3::compat::u16;
using openswd3::compat::u32;
using openswd3::rendering::LegacyBmpWriteStatus;
using openswd3::rendering::LegacyPixelConversionState;

class MemoryBmpPorts final : public openswd3::rendering::LegacyBmpWriterPorts {
public:
    bool open_or_create_without_truncation(
        const std::string_view filename
    ) override {
        opened_filename = std::string{filename};
        position = 0U;
        opened = open_succeeds;
        return opened;
    }

    bool seek_absolute(const u32 offset) override {
        if (!opened || !io_succeeds) {
            return false;
        }
        position = offset;
        return true;
    }

    bool write_bytes(const std::span<const u8> bytes) override {
        if (!opened || !io_succeeds) {
            return false;
        }
        const std::size_t end =
            static_cast<std::size_t>(position) + bytes.size();
        if (file.size() < end) {
            file.resize(end);
        }
        std::ranges::copy(
            bytes, file.begin() + static_cast<std::ptrdiff_t>(position)
        );
        position = static_cast<u32>(end);
        return true;
    }

    std::optional<u32> current_position() override {
        if (!opened || !io_succeeds) {
            return std::nullopt;
        }
        return position;
    }

    void close() override {
        opened = false;
        ++close_calls;
    }

    void maintain_audio() override {
        ++audio_calls;
    }

    bool open_succeeds{true};
    bool io_succeeds{true};
    bool opened{};
    u32 position{};
    u32 close_calls{};
    u32 audio_calls{};
    std::string opened_filename;
    std::vector<u8> file;
};

[[nodiscard]] u32
read_u32_le(const std::span<const u8> bytes, const std::size_t offset) {
    return static_cast<u32>(bytes[offset]) |
        (static_cast<u32>(bytes[offset + 1U]) << 8U) |
        (static_cast<u32>(bytes[offset + 2U]) << 16U) |
        (static_cast<u32>(bytes[offset + 3U]) << 24U);
}

void test_exact_rgb555_bmp(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> pixels{
        0x7C00U,
        0x03E0U,
        0x001FU,
        0x7FFFU,
    };
    MemoryBmpPorts ports;
    const auto result = openswd3::rendering::write_legacy_16bit_framebuffer_bmp(
        pixels, 2, 2, "ScrnShot/00000.bmp", LegacyPixelConversionState{}, ports
    );

    test.expect_equal(
        result.status, LegacyBmpWriteStatus::completed, "RGB555 BMP completes"
    );
    test.expect_equal(result.row_stride, 8U, "24-bit rows are DWORD aligned");
    test.expect_equal(result.logical_file_size, 70U, "logical BMP size");
    test.expect_equal(ports.file.size(), std::size_t{70U}, "physical size");
    test.expect_equal(
        ports.opened_filename, std::string{"ScrnShot/00000.bmp"}, "filename"
    );
    test.expect_equal(ports.close_calls, 1U, "successful writer closes once");
    test.expect_equal(
        ports.audio_calls, 2U, "row zero and final cleanup service audio"
    );

    test.expect_equal(ports.file[0], u8{0x42U}, "BMP signature B");
    test.expect_equal(ports.file[1], u8{0x4DU}, "BMP signature M");
    test.expect_equal(read_u32_le(ports.file, 2U), 70U, "patched file size");
    test.expect_equal(
        read_u32_le(ports.file, 10U), 54U, "patched pixel offset"
    );
    test.expect_equal(
        read_u32_le(ports.file, 14U), 40U, "BITMAPINFOHEADER size"
    );
    test.expect_equal(read_u32_le(ports.file, 18U), 2U, "BMP width");
    test.expect_equal(read_u32_le(ports.file, 22U), 2U, "BMP height");
    test.expect_equal(ports.file[26U], u8{1U}, "one color plane");
    test.expect_equal(ports.file[28U], u8{24U}, "24 bits per pixel");
    test.expect_equal(
        read_u32_le(ports.file, 34U), 0U, "legacy image size remains zero"
    );

    constexpr std::array<u8, 16U> expected_pixels{
        0xF8U,
        0x00U,
        0x00U,
        0xF8U,
        0xF8U,
        0xF8U,
        0x00U,
        0x00U,
        0x00U,
        0x00U,
        0xF8U,
        0x00U,
        0xF8U,
        0x00U,
        0x00U,
        0x00U,
    };
    test.expect_true(
        std::ranges::equal(
            std::span<const u8>{ports.file}.subspan(54U), expected_pixels
        ),
        "pixels are bottom-up BGR with zero row padding"
    );
}

void test_reverse_conversion(openswd3::test::Context& test) {
    LegacyPixelConversionState conversion;
    openswd3::rendering::select_legacy_pixel_conversion(
        conversion, {0xF800U, 0x07E0U, 0x001FU}
    );
    constexpr std::array<u16, 1U> rgb565_green{0x07E0U};
    MemoryBmpPorts ports;
    const auto result = openswd3::rendering::write_legacy_16bit_framebuffer_bmp(
        rgb565_green, 1, 1, "green.bmp", conversion, ports
    );

    test.expect_equal(
        result.status,
        LegacyBmpWriteStatus::completed,
        "RGB565 writer completes"
    );
    test.expect_equal(ports.file[54U], u8{0U}, "green blue byte");
    test.expect_equal(ports.file[55U], u8{0xF8U}, "green green byte");
    test.expect_equal(ports.file[56U], u8{0U}, "green red byte");
}

void test_audio_service_cadence(openswd3::test::Context& test) {
    std::vector<u16> pixels(16U);
    MemoryBmpPorts ports;
    const auto result = openswd3::rendering::write_legacy_16bit_framebuffer_bmp(
        pixels, 1, 16, "cadence.bmp", LegacyPixelConversionState{}, ports
    );

    test.expect_equal(
        result.status,
        LegacyBmpWriteStatus::completed,
        "16-row writer completes"
    );
    test.expect_equal(
        ports.audio_calls,
        3U,
        "audio is serviced after rows 0 and 15 and after close"
    );
}

void test_open_always_preserves_tail(openswd3::test::Context& test) {
    constexpr std::array<u16, 1U> pixels{0U};
    MemoryBmpPorts ports;
    ports.file.assign(80U, 0xAAU);
    const auto result = openswd3::rendering::write_legacy_16bit_framebuffer_bmp(
        pixels, 1, 1, "existing.bmp", LegacyPixelConversionState{}, ports
    );

    test.expect_equal(
        result.logical_file_size, 58U, "header records overwritten logical end"
    );
    test.expect_equal(
        ports.file.size(),
        std::size_t{80U},
        "OPEN_ALWAYS does not truncate old tail"
    );
    test.expect_true(
        std::ranges::all_of(
            std::span<const u8>{ports.file}.subspan(58U),
            [](const u8 value) { return value == 0xAAU; }
        ),
        "bytes beyond the logical end remain untouched"
    );
}

void test_rejections(openswd3::test::Context& test) {
    constexpr std::array<u16, 1U> pixels{0U};
    MemoryBmpPorts invalid_ports;
    const auto invalid =
        openswd3::rendering::write_legacy_16bit_framebuffer_bmp(
            pixels,
            2,
            1,
            "short.bmp",
            LegacyPixelConversionState{},
            invalid_ports
        );
    test.expect_equal(
        invalid.status,
        LegacyBmpWriteStatus::invalid_request,
        "short source rejected"
    );
    test.expect_false(
        invalid_ports.opened, "invalid request never opens output"
    );

    MemoryBmpPorts failed_open_ports;
    failed_open_ports.open_succeeds = false;
    const auto failed_open =
        openswd3::rendering::write_legacy_16bit_framebuffer_bmp(
            pixels,
            1,
            1,
            "missing-parent/out.bmp",
            LegacyPixelConversionState{},
            failed_open_ports
        );
    test.expect_equal(
        failed_open.status,
        LegacyBmpWriteStatus::open_failed,
        "open failure reported"
    );
    test.expect_equal(
        failed_open_ports.close_calls, 0U, "failed open is not closed"
    );
    test.expect_equal(
        failed_open_ports.audio_calls,
        0U,
        "failed open has no writer audio service"
    );
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_exact_rgb555_bmp(test);
    test_reverse_conversion(test);
    test_audio_service_cadence(test);
    test_open_always_preserves_tail(test);
    test_rejections(test);
    return test.exit_code();
}
