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

const char* kName = "file_test.bin";

void cleanup()
{
    std::error_code ignored;
    std::filesystem::remove(kName, ignored);
}

}  // namespace

TEST_CASE("file: open always creates, writes and reads back")
{
    cleanup();
    {
        File file;
        CHECK(!file.Open(kName, File::Disposition::OpenExisting, File::Access::Read));
        CHECK(file.Open(kName, File::Disposition::OpenAlways, File::Access::ReadWrite));
        CHECK(file.WriteDword(0x12345678u));
        CHECK(file.WriteByte(0xAB));
        CHECK_EQ(file.Size(), 5u);
        CHECK_EQ(file.SeekBegin(0), 1u);
        u32 value = 0;
        CHECK(file.ReadDword(value));
        CHECK_EQ(value, 0x12345678u);
        u32 position = 0;
        CHECK(file.Tell(position));
        CHECK_EQ(position, 4u);
        u8 byte = 0;
        CHECK(file.ReadByte(byte));
        CHECK_EQ(byte, 0xABu);
        CHECK(file.Close());
    }

    cleanup();
}

TEST_CASE("file: empty read fails, short read succeeds")
{
    cleanup();
    File file;
    CHECK(file.Open(kName, File::Disposition::OpenAlways, File::Access::ReadWrite));
    CHECK(file.WriteByte(7));
    file.SeekBegin(0);
    char buffer[8] = {};
    u32 bytes = 0;
    CHECK(!file.Read(buffer, bytes));
    CHECK(file.ReadExact(buffer, 8));
    file.Close();
    cleanup();
}

TEST_CASE("file: mapping views the whole file; only the current view can be released")
{
    cleanup();
    File file;
    CHECK(file.Open(kName, File::Disposition::OpenAlways, File::Access::ReadWrite));
    file.WriteExact("abcdef", 6);
    CHECK(file.MapView(0, 0) == nullptr);
    CHECK(file.CreateMapping());
    const u8* view = file.MapView(0, 0);
    CHECK(view != nullptr);
    CHECK(std::memcmp(view, "abcdef", 6) == 0);
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
    CHECK_EQ(file.SeekCurrent(-5), 0u);
    File closed;
    CHECK_EQ(closed.Seek(File::Origin::Begin, 0), 0u);
    file.Close();
    cleanup();
}
