#include "test.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>

namespace {

struct ExpectedBitmap {
    const char* filename{};
    std::uintmax_t file_size{};
    std::uint32_t width{};
    std::uint32_t height{};
    std::uint16_t bits_per_pixel{};
};

constexpr std::array<ExpectedBitmap, 6> kExpectedBitmaps{
    ExpectedBitmap{"bitmap-0113-background.bmp", 308278U, 640U, 480U, 8U},
    ExpectedBitmap{"bitmap-0114-hover-0.bmp", 8358U, 40U, 182U, 8U},
    ExpectedBitmap{"bitmap-0115-hover-1.bmp", 8358U, 40U, 182U, 8U},
    ExpectedBitmap{"bitmap-0116-hover-2.bmp", 8358U, 40U, 182U, 8U},
    ExpectedBitmap{"bitmap-0117-hover-3.bmp", 8358U, 40U, 182U, 8U},
    ExpectedBitmap{"bitmap-0118-hover-4.bmp", 21894U, 40U, 182U, 24U},
};

[[nodiscard]] std::uint16_t read_u16(
    const std::array<unsigned char, 54>& header, const std::size_t offset
) noexcept {
    return static_cast<std::uint16_t>(header[offset]) |
        static_cast<std::uint16_t>(
               static_cast<std::uint16_t>(header[offset + 1U]) << 8U
        );
}

[[nodiscard]] std::uint32_t read_u32(
    const std::array<unsigned char, 54>& header, const std::size_t offset
) noexcept {
    return static_cast<std::uint32_t>(header[offset]) |
        (static_cast<std::uint32_t>(header[offset + 1U]) << 8U) |
        (static_cast<std::uint32_t>(header[offset + 2U]) << 16U) |
        (static_cast<std::uint32_t>(header[offset + 3U]) << 24U);
}

void test_assets(
    openswd3::test::Context& test, const std::filesystem::path& directory
) {
    for (const ExpectedBitmap& expected : kExpectedBitmaps) {
        const std::filesystem::path path = directory / expected.filename;
        std::error_code error;
        const std::uintmax_t size = std::filesystem::file_size(path, error);
        test.expect_false(
            static_cast<bool>(error),
            "startup bitmap exists and has a readable size"
        );
        if (error) {
            continue;
        }
        test.expect_equal(size, expected.file_size, "startup bitmap file size");

        std::ifstream input(path, std::ios::binary);
        std::array<unsigned char, 54> header{};
        input.read(
            reinterpret_cast<char*>(header.data()),
            static_cast<std::streamsize>(header.size())
        );
        test.expect_equal(
            input.gcount(),
            static_cast<std::streamsize>(header.size()),
            "startup bitmap has a complete BMP header"
        );
        if (input.gcount() != static_cast<std::streamsize>(header.size())) {
            continue;
        }
        test.expect_equal(
            header[0], static_cast<unsigned char>('B'), "BMP B magic"
        );
        test.expect_equal(
            header[1], static_cast<unsigned char>('M'), "BMP M magic"
        );
        test.expect_equal(read_u32(header, 18U), expected.width, "BMP width");
        test.expect_equal(read_u32(header, 22U), expected.height, "BMP height");
        test.expect_equal(
            read_u16(header, 28U), expected.bits_per_pixel, "BMP bit depth"
        );
    }
}

}  // namespace

int main(const int argument_count, char** arguments) {
    openswd3::test::Context test;
    test.expect_equal(argument_count, 2, "asset test receives one directory");
    if (argument_count == 2 && arguments != nullptr &&
        arguments[1] != nullptr) {
        test_assets(test, arguments[1]);
    }
    return test.exit_code();
}
