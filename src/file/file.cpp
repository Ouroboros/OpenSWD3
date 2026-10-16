#include "file/file.h"

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <system_error>

#include "core/debug_log.h"
#include "platform/file_system.h"

namespace swd3
{

namespace
{

const char* openMode(File::Access access, bool create)
{
    switch (access)
    {
        case File::Access::Read:
            return "rb";
        case File::Access::Write:
        case File::Access::ReadWrite:
            return create ? "w+b" : "r+b";
    }

    return "rb";
}

int seekOrigin(File::Origin origin)
{
    switch (origin)
    {
        case File::Origin::Begin:
            return SEEK_SET;
        case File::Origin::Current:
            return SEEK_CUR;
        case File::Origin::End:
            return SEEK_END;
    }

    return SEEK_SET;
}

}  // namespace

// swd32.exe 0x00438000: no file, mapping or view
File::File() = default;

// swd32.exe 0x00438030: a failure to release the view or the mapping leaves the file open (as in the original)
File::~File()
{
    if (this->mapped)
    {
        if (this->viewed && !this->ReleaseView())
        {
            return;
        }

        this->mapped = false;
    }

    if (this->file != nullptr)
    {
        std::fclose(this->file);
    }
}

// swd32.exe 0x004380B0: clears the read-only attribute first; the position starts at the beginning
bool File::Open(std::string_view name, Disposition disposition, Access access)
{
    if (this->file != nullptr)
    {
        std::fclose(this->file);
        this->file = nullptr;
    }

    const std::string path = ResolvePath(name);
    std::error_code ignored;
    std::filesystem::permissions(
        path,
        std::filesystem::perms::owner_write,
        std::filesystem::perm_options::add,
        ignored
    );
    this->file = std::fopen(path.c_str(), openMode(access, false));

    if (this->file == nullptr && disposition == Disposition::OpenAlways)
    {
        this->file = std::fopen(path.c_str(), openMode(access, true));
    }

    if (this->file == nullptr)
    {
        this->SetError(std::strerror(errno));
        return false;
    }

    this->name.assign(name);
    std::fseek(this->file, 0, SEEK_SET);
    return true;
}

// swd32.exe 0x00438150
bool File::Close()
{
    if (!this->CloseMapping())
    {
        return false;
    }

    if (this->file != nullptr)
    {
        if (std::fclose(this->file) != 0)
        {
            this->file = nullptr;
            this->SetError(std::strerror(errno));
            return false;
        }

        this->file = nullptr;
        this->name.clear();
        this->error.clear();
    }

    return true;
}

// swd32.exe 0x004381E0: a read-only mapping of the whole file
bool File::CreateMapping()
{
    if (this->file == nullptr)
    {
        return false;
    }

    this->mapped = true;
    return true;
}

// swd32.exe 0x00438230
bool File::CloseMapping()
{
    if (this->mapped)
    {
        if (this->viewed && !this->ReleaseView())
        {
            return false;
        }

        this->mapped = false;
    }

    return true;
}

// swd32.exe 0x00438290: a previous view is released first
const u8* File::MapView(u32 offset, u32 size)
{
    if (this->viewed && !this->ReleaseView())
    {
        return nullptr;
    }

    this->viewed = false;

    if (!this->mapped)
    {
        return nullptr;
    }

    const u32 fileSize = this->Size();

    if (offset > fileSize || (size != 0 && size > fileSize - offset))
    {
        return nullptr;
    }

    const u32 length = size != 0 ? size : fileSize - offset;
    const long position = std::ftell(this->file);
    this->view.assign(length, 0);
    std::fseek(this->file, static_cast<long>(offset), SEEK_SET);
    const std::size_t done = std::fread(this->view.data(), 1, length, this->file);
    std::fseek(this->file, position, SEEK_SET);

    if (done != length)
    {
        return nullptr;
    }

    this->viewed = true;
    return this->view.data();
}

// swd32.exe 0x004382E0: only the current view can be released
bool File::UnmapView(const u8* view)
{
    const u8* current = this->viewed ? this->view.data() : nullptr;

    if (current != view)
    {
        DebugLog("cFile ,CloseView Src/Dest difference ");
        return false;
    }

    if (view != nullptr && !this->ReleaseView())
    {
        return false;
    }

    this->viewed = false;
    return true;
}

bool File::ReleaseView()
{
    this->view.clear();
    this->view.shrink_to_fit();
    this->viewed = false;
    return true;
}

// swd32.exe 0x00438340
u32 File::Size()
{
    if (this->file == nullptr)
    {
        return kInvalidPosition;
    }

    const long position = std::ftell(this->file);
    std::fseek(this->file, 0, SEEK_END);
    const long size = std::ftell(this->file);
    std::fseek(this->file, position, SEEK_SET);
    return static_cast<u32>(size);
}

// swd32.exe 0x00438360: SetEndOfFile at the current position
bool File::Truncate()
{
    if (this->file == nullptr)
    {
        return false;
    }

    std::fflush(this->file);
    const long position = std::ftell(this->file);
    std::error_code failure;
    std::filesystem::resize_file(ResolvePath(this->name), static_cast<std::uintmax_t>(position), failure);
    return !failure;
}

// swd32.exe 0x00438380: an empty request fails
bool File::Read(void* buffer, u32& bytes)
{
    if (this->file != nullptr && buffer != nullptr && bytes != 0)
    {
        const std::size_t done = std::fread(buffer, 1, bytes, this->file);

        if (done == bytes || !std::ferror(this->file))
        {
            bytes = static_cast<u32>(done);
            return true;
        }

        std::clearerr(this->file);
        bytes = 0;
    }

    this->SetError(std::strerror(errno));
    return false;
}

// swd32.exe 0x004383E0: an empty request fails
bool File::Write(const void* buffer, u32& bytes)
{
    if (this->file != nullptr && buffer != nullptr && bytes != 0)
    {
        const std::size_t done = std::fwrite(buffer, 1, bytes, this->file);

        if (done == bytes)
        {
            bytes = static_cast<u32>(done);
            return true;
        }

        std::clearerr(this->file);
        bytes = 0;
    }

    this->SetError(std::strerror(errno));
    return false;
}

// swd32.exe 0x00438440
u32 File::Seek(Origin origin, i32 distance)
{
    if (this->file == nullptr)
    {
        this->SetError("file not open");
        return 0;
    }

    if (std::fseek(this->file, distance, seekOrigin(origin)) != 0)
    {
        this->SetError(std::strerror(errno));
        return kInvalidPosition;
    }

    return static_cast<u32>(std::ftell(this->file));
}

// swd32.exe 0x004384A0
u32 File::SeekCurrent(i32 distance)
{
    return this->Seek(Origin::Current, distance) + 1;
}

// swd32.exe 0x004384B0
u32 File::SeekBegin(i32 distance)
{
    return this->Seek(Origin::Begin, distance) + 1;
}

// swd32.exe 0x004384C0
u32 File::SeekEnd(i32 distance)
{
    return this->Seek(Origin::End, distance) + 1;
}

// swd32.exe 0x004384D0
bool File::Tell(u32& position)
{
    if (this->file != nullptr)
    {
        const long current = std::ftell(this->file);

        if (current >= 0)
        {
            position = static_cast<u32>(current);
            return true;
        }
    }

    this->SetError("tell failed");
    return false;
}

// swd32.exe 0x00438520
bool File::WriteExact(const void* buffer, int bytes)
{
    auto done = static_cast<u32>(bytes);

    if (this->Write(buffer, done) && done == static_cast<u32>(bytes))
    {
        return true;
    }

    this->SetError("short write");
    return false;
}

// swd32.exe 0x00438570: GetFileTime (the game only uses the last write time)
bool File::LastWriteTime(u64& fileTime) const
{
    if (this->file == nullptr)
    {
        return false;
    }

    std::error_code failure;
    const auto written = std::filesystem::last_write_time(ResolvePath(this->name), failure);

    if (failure)
    {
        return false;
    }

    using FileTimeTicks = std::chrono::duration<i64, std::ratio<1, 10000000>>;
    const auto sinceEpoch = std::chrono::clock_cast<std::chrono::system_clock>(written).time_since_epoch();
    const auto since1970 = std::chrono::duration_cast<FileTimeTicks>(sinceEpoch);
    constexpr i64 kFrom1601To1970 = 116444736000000000;
    fileTime = static_cast<u64>(since1970.count() + kFrom1601To1970);
    return true;
}

// swd32.exe 0x004385A0
bool File::WriteByte(u8 value)
{
    return this->WriteExact(&value, 1);
}

// swd32.exe 0x004385B0
bool File::WriteDword(u32 value)
{
    return this->WriteExact(&value, 4);
}

// swd32.exe 0x004385C0: succeeds whenever the read call does, also on a short read
bool File::ReadExact(void* buffer, int bytes)
{
    auto done = static_cast<u32>(bytes);

    if (this->Read(buffer, done))
    {
        return true;
    }

    this->SetError("read failed");
    return false;
}

// swd32.exe 0x00438600
bool File::ReadByte(u8& value)
{
    return this->ReadExact(&value, 1);
}

// swd32.exe 0x00438610
bool File::ReadDword(u32& value)
{
    return this->ReadExact(&value, 4);
}

// swd32.exe 0x00438620: FormatMessageA of the system error into the 0x40 byte buffer
void File::SetError(const char* text)
{
    this->error.assign(text, std::min<std::size_t>(std::strlen(text), 0x3F));
}

// swd32.exe 0x00438640
const std::string& File::ErrorText() const
{
    return this->error;
}

}  // namespace swd3
