#include <array>
#include <cstdio>
#include <cstring>
#include <filesystem>

#include "core/types.h"
#include "file/file.h"
#include "test.h"

using swd3::u32;
using swd3::u8;

using swd3::File;

namespace
{

constexpr std::string_view kName = "file_test.bin";

void cleanup()
{
    std::error_code ignored;
    std::filesystem::remove(kName, ignored);
}

constexpr u32 kDword = 0x12345678;
constexpr u8 kByte = 0xAB;
constexpr std::size_t kBufferBytes = 8;  // more than the file holds
constexpr std::string_view kText = "abcdef";
constexpr swd3::i32 kBackwards = 5;

}  // namespace

TEST_CASE("file: open always creates, writes and reads back")
{
    cleanup();
    {
        File file;
        CHECK(!file.Open(kName, File::Disposition::OpenExisting, File::Access::Read));
        CHECK(file.Open(kName, File::Disposition::OpenAlways, File::Access::ReadWrite));
        CHECK(file.WriteDword(kDword));
        CHECK(file.WriteByte(kByte));
        CHECK_EQ(file.Size(), sizeof(kDword) + sizeof(kByte));
        CHECK_EQ(file.SeekBegin(0), 1u);
        u32 value = 0;
        CHECK(file.ReadDword(value));
        CHECK_EQ(value, kDword);
        u32 position = 0;
        CHECK(file.Tell(position));
        CHECK_EQ(position, sizeof(kDword));
        u8 byte = 0;
        CHECK(file.ReadByte(byte));
        CHECK_EQ(byte, kByte);
        CHECK(file.Close());
    }

    cleanup();
}

TEST_CASE("file: empty read fails, short read succeeds")
{
    cleanup();
    File file;
    CHECK(file.Open(kName, File::Disposition::OpenAlways, File::Access::ReadWrite));
    CHECK(file.WriteByte(kByte));
    file.SeekBegin(0);
    std::array<char, kBufferBytes> buffer = {};
    u32 bytes = 0;
    CHECK(!file.Read(buffer.data(), bytes));
    CHECK(file.ReadExact(buffer.data(), static_cast<int>(buffer.size())));
    file.Close();
    cleanup();
}

TEST_CASE("file: mapping views the whole file; only the current view can be released")
{
    cleanup();
    File file;
    CHECK(file.Open(kName, File::Disposition::OpenAlways, File::Access::ReadWrite));
    file.WriteExact(kText.data(), static_cast<int>(kText.size()));
    CHECK(file.MapView(0, 0) == nullptr);
    CHECK(file.CreateMapping());
    const u8* view = file.MapView(0, 0);
    CHECK(view != nullptr);
    CHECK(std::memcmp(view, kText.data(), kText.size()) == 0);
    u8 other = 0;
    CHECK(!file.UnmapView(&other));
    CHECK(file.UnmapView(view));
    CHECK(file.CloseMapping());
    file.Close();
    cleanup();
}

TEST_CASE("file: seek before the start fails")
{
    cleanup();
    File file;
    CHECK(file.Open(kName, File::Disposition::OpenAlways, File::Access::ReadWrite));
    CHECK_EQ(file.SeekCurrent(-kBackwards), 0u);
    File closed;
    CHECK_EQ(closed.Seek(File::Origin::Begin, 0), 0u);
    file.Close();
    cleanup();
}
