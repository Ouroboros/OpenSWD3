#include "swd3/w32.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace w32 {

DWORD GetLastError() { return ::GetLastError(); }

void SetLastError(DWORD code) { ::SetLastError(code); }

BOOL CloseHandle(HANDLE handle) { return ::CloseHandle(handle); }

HANDLE CreateFileA(LPCSTR fileName, DWORD desiredAccess, DWORD shareMode, void* securityAttributes,
                   DWORD creationDisposition, DWORD flagsAndAttributes, HANDLE templateFile) {
    return ::CreateFileA(fileName, desiredAccess, shareMode, static_cast<LPSECURITY_ATTRIBUTES>(securityAttributes),
                         creationDisposition, flagsAndAttributes, templateFile);
}

BOOL SetFileAttributesA(LPCSTR fileName, DWORD attributes) { return ::SetFileAttributesA(fileName, attributes); }

BOOL ReadFile(HANDLE file, LPVOID buffer, DWORD toRead, LPDWORD read, void* overlapped) {
    return ::ReadFile(file, buffer, toRead, reinterpret_cast<::LPDWORD>(read), static_cast<LPOVERLAPPED>(overlapped));
}

BOOL WriteFile(HANDLE file, LPCVOID buffer, DWORD toWrite, LPDWORD written, void* overlapped) {
    return ::WriteFile(file, buffer, toWrite, reinterpret_cast<::LPDWORD>(written),
                       static_cast<LPOVERLAPPED>(overlapped));
}

DWORD SetFilePointer(HANDLE file, LONG distance, PLONG distanceHigh, DWORD moveMethod) {
    return ::SetFilePointer(file, distance, reinterpret_cast<::PLONG>(distanceHigh), moveMethod);
}

DWORD GetFileSize(HANDLE file, LPDWORD sizeHigh) { return ::GetFileSize(file, reinterpret_cast<::LPDWORD>(sizeHigh)); }

BOOL SetEndOfFile(HANDLE file) { return ::SetEndOfFile(file); }

BOOL GetFileTime(HANDLE file, LPFILETIME creation, LPFILETIME lastAccess, LPFILETIME lastWrite) {
    return ::GetFileTime(file, reinterpret_cast<::LPFILETIME>(creation), reinterpret_cast<::LPFILETIME>(lastAccess),
                         reinterpret_cast<::LPFILETIME>(lastWrite));
}

HANDLE CreateFileMappingA(HANDLE file, void* attributes, DWORD protect, DWORD sizeHigh, DWORD sizeLow, LPCSTR name) {
    return ::CreateFileMappingA(file, static_cast<LPSECURITY_ATTRIBUTES>(attributes), protect, sizeHigh, sizeLow, name);
}

LPVOID MapViewOfFile(HANDLE mapping, DWORD access, DWORD offsetHigh, DWORD offsetLow, SIZE_T bytes) {
    return ::MapViewOfFile(mapping, access, offsetHigh, offsetLow, bytes);
}

BOOL UnmapViewOfFile(LPCVOID base) { return ::UnmapViewOfFile(base); }

DWORD FormatMessageA(DWORD flags, LPCVOID source, DWORD messageId, DWORD languageId, LPSTR buffer, DWORD size,
                     void* arguments) {
    return ::FormatMessageA(flags, source, messageId, languageId, buffer, size, static_cast<va_list*>(arguments));
}

int lstrlenA(LPCSTR string) { return ::lstrlenA(string); }

LPSTR lstrcpyA(LPSTR dest, LPCSTR src) { return ::lstrcpyA(dest, src); }

LPSTR lstrcatA(LPSTR dest, LPCSTR src) { return ::lstrcatA(dest, src); }

int lstrcmpA(LPCSTR a, LPCSTR b) { return ::lstrcmpA(a, b); }

}  // namespace w32
