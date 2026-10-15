#include "file/file.h"

#include <cstring>

#include "core/debug_log.h"
#include "crt/crt.h"

namespace swd3 {

// swd32.exe 0x00438000
File::File() {
    m_file = INVALID_HANDLE_VALUE;
    m_mapping = nullptr;
    m_view = nullptr;
    m_name = nullptr;
    std::memset(m_error, 0, sizeof(m_error));
}

// swd32.exe 0x00438030
File::~File() {
    if (m_mapping) {
        if (m_view) {
            if (!w32::UnmapViewOfFile(m_view)) {
                SetError(w32::GetLastError());
                return;
            }
            m_view = nullptr;
        }
        if (!w32::CloseHandle(m_mapping)) {
            SetError(w32::GetLastError());
            return;
        }
        m_mapping = nullptr;
    }
    if (m_file != INVALID_HANDLE_VALUE) {
        w32::CloseHandle(m_file);
        if (m_name) {
            crt::operator_delete(m_name);
        }
    }
}

// swd32.exe 0x004380B0
int File::Open(LPCSTR fileName, DWORD creationDisposition, DWORD desiredAccess) {
    if (m_file != INVALID_HANDLE_VALUE) {
        w32::CloseHandle(m_file);
    }
    w32::SetFileAttributesA(fileName, FILE_ATTRIBUTE_NORMAL);
    HANDLE file = w32::CreateFileA(fileName, desiredAccess, 0, nullptr, creationDisposition,
                                   FILE_FLAG_RANDOM_ACCESS | FILE_ATTRIBUTE_NORMAL, nullptr);
    m_file = file;
    if (file == INVALID_HANDLE_VALUE) {
        m_file = file;
        SetError(w32::GetLastError());
        return 0;
    }
    m_name = static_cast<char*>(crt::operator_new(static_cast<std::uint32_t>(w32::lstrlenA(fileName) + 1)));
    w32::lstrcpyA(m_name, fileName);
    w32::SetFilePointer(m_file, 0, nullptr, FILE_BEGIN);
    return 1;
}

// swd32.exe 0x00438150
int File::Close() {
    if (m_mapping) {
        if (m_view) {
            if (!w32::UnmapViewOfFile(m_view)) {
                SetError(w32::GetLastError());
                return 0;
            }
            m_view = nullptr;
        }
        if (!w32::CloseHandle(m_mapping)) {
            SetError(w32::GetLastError());
            return 0;
        }
        m_mapping = nullptr;
    }
    if (m_file != INVALID_HANDLE_VALUE) {
        if (!w32::CloseHandle(m_file)) {
            SetError(w32::GetLastError());
            return 0;
        }
        if (m_name) {
            crt::operator_delete(m_name);
        }
        m_name = nullptr;
        std::memset(m_error, 0, sizeof(m_error));
        m_file = INVALID_HANDLE_VALUE;
    }
    return 1;
}

// swd32.exe 0x004381E0
int File::CreateMapping() {
    if (m_file == INVALID_HANDLE_VALUE) {
        return 0;
    }
    DWORD size = GetSize();
    m_mapping = w32::CreateFileMappingA(m_file, nullptr, PAGE_READONLY, 0, size, nullptr);
    if (m_mapping) {
        return 1;
    }
    SetError(w32::GetLastError());
    return 0;
}

// swd32.exe 0x00438230
int File::CloseMapping() {
    if (m_mapping) {
        if (m_view) {
            if (!w32::UnmapViewOfFile(m_view)) {
                SetError(w32::GetLastError());
                return 0;
            }
            m_view = nullptr;
        }
        if (!w32::CloseHandle(m_mapping)) {
            SetError(w32::GetLastError());
            return 0;
        }
        m_mapping = nullptr;
    }
    return 1;
}

// swd32.exe 0x00438290
LPVOID File::MapView(DWORD offset, SIZE_T bytes) {
    if (m_view && !w32::UnmapViewOfFile(m_view)) {
        SetError(w32::GetLastError());
        return nullptr;
    }
    m_view = w32::MapViewOfFile(m_mapping, FILE_MAP_READ, 0, offset, bytes);
    return m_view;
}

// swd32.exe 0x004382E0
int File::UnmapView(LPCVOID view) {
    if (m_view != view) {
        DebugLog("File ,CloseView Src/Dest difference ");
        return 0;
    }
    if (view && !w32::UnmapViewOfFile(view)) {
        SetError(w32::GetLastError());
        return 0;
    }
    m_view = nullptr;
    return 1;
}

// swd32.exe 0x00438340
DWORD File::GetSize() {
    if (m_file == INVALID_HANDLE_VALUE) {
        return 0xFFFFFFFFu;
    }
    return w32::GetFileSize(m_file, nullptr);
}

// swd32.exe 0x00438360
BOOL File::SetEnd() {
    if (m_file == INVALID_HANDLE_VALUE) {
        return 0;
    }
    return w32::SetEndOfFile(m_file);
}

// swd32.exe 0x00438380
int File::Read(LPVOID buffer, DWORD* bytes) {
    if (m_file != INVALID_HANDLE_VALUE && buffer && *bytes) {
        DWORD done;
        BOOL ok = w32::ReadFile(m_file, buffer, *bytes, &done, nullptr);
        if (ok) {
            *bytes = done;
            return 1;
        }
        *bytes = static_cast<DWORD>(ok);
    }
    SetError(w32::GetLastError());
    return 0;
}

// swd32.exe 0x004383E0
int File::Write(LPCVOID buffer, DWORD* bytes) {
    if (m_file != INVALID_HANDLE_VALUE && buffer && *bytes) {
        DWORD done;
        BOOL ok = w32::WriteFile(m_file, buffer, *bytes, &done, nullptr);
        if (ok) {
            *bytes = done;
            return 1;
        }
        *bytes = static_cast<DWORD>(ok);
    }
    SetError(w32::GetLastError());
    return 0;
}

// swd32.exe 0x00438440
DWORD File::Seek(DWORD moveMethod, LONG distance) {
    if (m_file == INVALID_HANDLE_VALUE) {
        SetError(w32::GetLastError());
        return 0;
    }
    LONG high = (distance >= 0) - 1;
    DWORD result = w32::SetFilePointer(m_file, distance, &high, moveMethod);
    if (result == 0xFFFFFFFFu) {
        SetError(w32::GetLastError());
    }
    return result;
}

// swd32.exe 0x004384A0
DWORD File::SeekCurrent(LONG distance) { return Seek(FILE_CURRENT, distance) + 1; }

// swd32.exe 0x004384B0
DWORD File::SeekBegin(LONG distance) { return Seek(FILE_BEGIN, distance) + 1; }

// swd32.exe 0x004384C0
DWORD File::SeekEnd(LONG distance) { return Seek(FILE_END, distance) + 1; }

// swd32.exe 0x004384D0
int File::Tell(DWORD* position) {
    LONG high = 0;
    if (m_file != INVALID_HANDLE_VALUE) {
        DWORD result = w32::SetFilePointer(m_file, 0, &high, FILE_CURRENT);
        if (result != 0xFFFFFFFFu) {
            *position = result;
            return 1;
        }
    }
    SetError(w32::GetLastError());
    return 0;
}

// swd32.exe 0x00438520
int File::WriteExact(LPCVOID buffer, int bytes) {
    DWORD done = static_cast<DWORD>(bytes);
    if (Write(buffer, &done) && done == static_cast<DWORD>(bytes)) {
        return 1;
    }
    SetError(w32::GetLastError());
    return 0;
}

// swd32.exe 0x00438570
BOOL File::GetTime(LPFILETIME creation, LPFILETIME lastAccess, LPFILETIME lastWrite) {
    if (m_file == INVALID_HANDLE_VALUE) {
        return 0;
    }
    return w32::GetFileTime(m_file, creation, lastAccess, lastWrite);
}

// swd32.exe 0x004385A0
int File::WriteByte(DWORD value) { return WriteExact(&value, 1); }

// swd32.exe 0x004385B0
int File::WriteDword(DWORD value) { return WriteExact(&value, 4); }

// swd32.exe 0x004385C0
int File::ReadExact(LPVOID buffer, int bytes) {
    DWORD done = static_cast<DWORD>(bytes);
    if (Read(buffer, &done)) {
        return 1;
    }
    SetError(w32::GetLastError());
    return 0;
}

// swd32.exe 0x00438600
int File::ReadByte(LPVOID buffer) { return ReadExact(buffer, 1); }

// swd32.exe 0x00438610
int File::ReadDword(LPVOID buffer) { return ReadExact(buffer, 4); }

// swd32.exe 0x00438620
DWORD File::SetError(DWORD messageId) {
    return w32::FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, messageId, 0, m_error, sizeof(m_error), nullptr);
}

// swd32.exe 0x00438640
char* File::ErrorText() { return m_error; }

}  // namespace swd3
