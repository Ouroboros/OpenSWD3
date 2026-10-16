#pragma once

#include <cstdint>
#include <cstdio>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "core/types.h"

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

    static constexpr u32 kInvalidPosition = 0xFFFFFFFFu;

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
    const u8* MapView(u32 offset, u32 size);
    bool UnmapView(const u8* view);
    // The bytes of the current view (empty without one).
    std::span<const u8> View() const;

    // kInvalidPosition when the file is not open.
    u32 Size();
    bool Truncate();
    // `bytes` is the request on entry and the count transferred on success (0 when the call failed).
    bool Read(void* buffer, u32& bytes);
    bool Write(const void* buffer, u32& bytes);
    // The new position, kInvalidPosition on failure (0 when the file is not open).
    u32 Seek(Origin origin, i32 distance);
    // The new position plus one: 0 exactly when the seek failed.
    u32 SeekCurrent(i32 distance);
    u32 SeekBegin(i32 distance);
    u32 SeekEnd(i32 distance);
    bool Tell(u32& position);
    // Last write time as a Windows FILETIME (100 ns units since 1601-01-01 UTC).
    bool LastWriteTime(u64& fileTime) const;

    bool WriteExact(const void* buffer, int bytes);
    bool WriteByte(u8 value);
    bool WriteDword(u32 value);
    // Succeeds whenever the read call succeeds, also when fewer bytes were available.
    bool ReadExact(void* buffer, int bytes);
    bool ReadByte(u8& value);
    bool ReadDword(u32& value);

    const std::string& ErrorText() const;

private:
    void SetError(std::string_view text);
    bool ReleaseView();

    std::FILE* file = nullptr;
    bool mapped = false;
    bool viewed = false;
    std::vector<u8> view;
    std::string name;
    std::string error;
};

}  // namespace swd3
