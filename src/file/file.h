#pragma once

#include <cstdint>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

namespace swd3
{

// "cFile" (its error text names it): a file handle with a single read-only view. The original wraps CreateFileA
// with FILE_FLAG_RANDOM_ACCESS and a read-only file mapping; here the view is a copy of the mapped range.
class File
{
public:
    // dwCreationDisposition values the game passes.
    enum class Disposition
    {
        OpenExisting,  // OPEN_EXISTING (3)
        OpenAlways,    // OPEN_ALWAYS (4)
    };

    // dwDesiredAccess values the game passes.
    enum class Access
    {
        Read,       // GENERIC_READ
        Write,      // GENERIC_WRITE
        ReadWrite,  // GENERIC_READ | GENERIC_WRITE
    };

    enum class Origin
    {
        Begin,
        Current,
        End
    };

    static constexpr std::uint32_t kInvalidPosition = 0xFFFFFFFFu;

    File();
    ~File();
    File(const File&) = delete;
    File& operator=(const File&) = delete;

    bool Open(std::string_view name, Disposition disposition, Access access);
    bool Close();

    bool IsOpen() const
    {
        return this->file != nullptr;
    }

    const std::string& Name() const
    {
        return this->name;
    }

    bool CreateMapping();
    bool CloseMapping();
    // The bytes [offset, offset + size) of the mapping (to the end of the file for size 0); nullptr on failure.
    const std::uint8_t* MapView(std::uint32_t offset, std::uint32_t size);
    bool UnmapView(const std::uint8_t* view);

    // kInvalidPosition when the file is not open.
    std::uint32_t Size();
    bool Truncate();
    // `bytes` is the request on entry and the count transferred on success (0 when the call failed).
    bool Read(void* buffer, std::uint32_t& bytes);
    bool Write(const void* buffer, std::uint32_t& bytes);
    // The new position, kInvalidPosition on failure (0 when the file is not open).
    std::uint32_t Seek(Origin origin, std::int32_t distance);
    // The new position plus one: 0 exactly when the seek failed.
    std::uint32_t SeekCurrent(std::int32_t distance);
    std::uint32_t SeekBegin(std::int32_t distance);
    std::uint32_t SeekEnd(std::int32_t distance);
    bool Tell(std::uint32_t& position);
    // Last write time as a Windows FILETIME (100 ns units since 1601-01-01 UTC).
    bool LastWriteTime(std::uint64_t& fileTime) const;

    bool WriteExact(const void* buffer, int bytes);
    bool WriteByte(std::uint8_t value);
    bool WriteDword(std::uint32_t value);
    // Succeeds whenever the read call succeeds, also when fewer bytes were available.
    bool ReadExact(void* buffer, int bytes);
    bool ReadByte(std::uint8_t& value);
    bool ReadDword(std::uint32_t& value);

    const std::string& ErrorText() const;

private:
    void SetError(const char* text);
    bool ReleaseView();

    std::FILE* file = nullptr;
    bool mapped = false;
    bool viewed = false;
    std::vector<std::uint8_t> view;
    std::string name;
    std::string error;
};

}  // namespace swd3
