#include <cstdint>
#include <cstdio>
#include <cstring>

#include "file/file.h"
#include "test.h"

using namespace swd3;

namespace {

const char* kPath = "file_test.bin";

}  // namespace

TEST_CASE("file.write seek read round trip") {
    std::remove(kPath);
    File file;
    CHECK_EQ(file.GetSize(), 0xFFFFFFFFu);
    CHECK_EQ(file.SetEnd(), 0);
    CHECK_EQ(file.Open(kPath, CREATE_ALWAYS, GENERIC_READ | GENERIC_WRITE), 1);
    CHECK(std::strcmp(file.m_name, kPath) == 0);
    CHECK_EQ(file.WriteDword(0x11223344u), 1);
    CHECK_EQ(file.WriteByte(0xAABBCC55u), 1);
    CHECK_EQ(file.GetSize(), 5u);
    DWORD pos = 0;
    CHECK_EQ(file.Tell(&pos), 1);
    CHECK_EQ(pos, 5u);
    CHECK_EQ(file.SeekBegin(1), 2u);
    CHECK_EQ(file.SeekCurrent(1), 3u);
    CHECK_EQ(file.SeekEnd(-5), 1u);
    DWORD value = 0;
    CHECK_EQ(file.ReadDword(&value), 1);
    CHECK_EQ(value, 0x11223344u);
    std::uint8_t byte = 0;
    CHECK_EQ(file.ReadByte(&byte), 1);
    CHECK_EQ(byte, 0x55);
    CHECK_EQ(file.ReadExact(&value, 4), 1);
    DWORD zero = 0;
    CHECK_EQ(file.Read(&value, &zero), 0);
    CHECK_EQ(file.Close(), 1);
    CHECK(file.m_file == INVALID_HANDLE_VALUE);
    CHECK(file.m_name == nullptr);
    std::remove(kPath);
}

TEST_CASE("file.mapping view") {
    std::remove(kPath);
    {
        File writer;
        CHECK_EQ(writer.Open(kPath, CREATE_ALWAYS, GENERIC_WRITE), 1);
        const char text[] = "swd3 mapping";
        CHECK_EQ(writer.WriteExact(text, sizeof(text)), 1);
        CHECK_EQ(writer.Close(), 1);
    }
    File file;
    CHECK_EQ(file.CreateMapping(), 0);
    CHECK_EQ(file.Open(kPath, OPEN_EXISTING, GENERIC_READ), 1);
    CHECK_EQ(file.CreateMapping(), 1);
    auto* view = static_cast<const char*>(file.MapView(0, 0));
    CHECK(view != nullptr);
    CHECK(std::strcmp(view, "swd3 mapping") == 0);
    CHECK_EQ(file.UnmapView(reinterpret_cast<const void*>(0x1234)), 0);
    CHECK_EQ(file.UnmapView(view), 1);
    CHECK(file.m_view == nullptr);
    CHECK_EQ(file.CloseMapping(), 1);
    CHECK_EQ(file.Close(), 1);
    std::remove(kPath);
}

TEST_CASE("file.open failure keeps invalid handle and sets error text") {
    File file;
    CHECK_EQ(file.Open("no_such_dir/no_such_file.bin", OPEN_EXISTING, GENERIC_READ), 0);
    CHECK(file.m_file == INVALID_HANDLE_VALUE);
    CHECK(file.ErrorText()[0] != '\0');
    CHECK_EQ(file.Seek(FILE_BEGIN, 0), 0u);
}
