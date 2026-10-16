#pragma once

#include <cstddef>
#include <cstdint>

namespace w32 {

using BYTE = std::uint8_t;
using WORD = std::uint16_t;
using DWORD = std::uint32_t;
using UINT = std::uint32_t;
using LONG = std::int32_t;
using BOOL = std::int32_t;
using CHAR = char;
using SIZE_T = std::size_t;
using HANDLE = void*;
using LPVOID = void*;
using LPCVOID = const void*;
using LPSTR = char*;
using LPCSTR = const char*;
using LPDWORD = DWORD*;
using PLONG = LONG*;
using HWND = void*;

inline HANDLE const INVALID_HANDLE_VALUE = reinterpret_cast<HANDLE>(static_cast<std::intptr_t>(-1));

constexpr DWORD GENERIC_READ = 0x80000000u;
constexpr DWORD GENERIC_WRITE = 0x40000000u;
constexpr DWORD CREATE_NEW = 1;
constexpr DWORD CREATE_ALWAYS = 2;
constexpr DWORD OPEN_EXISTING = 3;
constexpr DWORD OPEN_ALWAYS = 4;
constexpr DWORD TRUNCATE_EXISTING = 5;
constexpr DWORD FILE_ATTRIBUTE_NORMAL = 0x80;
constexpr DWORD FILE_FLAG_RANDOM_ACCESS = 0x10000000u;
constexpr DWORD FILE_BEGIN = 0;
constexpr DWORD FILE_CURRENT = 1;
constexpr DWORD FILE_END = 2;
constexpr DWORD PAGE_READONLY = 0x02;
constexpr DWORD PAGE_READWRITE = 0x04;
constexpr DWORD FILE_MAP_READ = 0x04;
constexpr DWORD FORMAT_MESSAGE_FROM_SYSTEM = 0x1000;
constexpr DWORD ERROR_SUCCESS = 0;
constexpr DWORD ERROR_FILE_NOT_FOUND = 2;
constexpr DWORD ERROR_PATH_NOT_FOUND = 3;
constexpr DWORD ERROR_ACCESS_DENIED = 5;
constexpr DWORD ERROR_INVALID_HANDLE = 6;
constexpr DWORD ERROR_NOT_ENOUGH_MEMORY = 8;
constexpr DWORD ERROR_INVALID_PARAMETER = 87;
constexpr DWORD ERROR_NEGATIVE_SEEK = 131;
constexpr DWORD ERROR_FILE_EXISTS = 80;
constexpr DWORD ERROR_ALREADY_EXISTS = 183;

struct FILETIME {
    DWORD dwLowDateTime;
    DWORD dwHighDateTime;
};
using LPFILETIME = FILETIME*;

struct SYSTEMTIME {
    WORD wYear;
    WORD wMonth;
    WORD wDayOfWeek;
    WORD wDay;
    WORD wHour;
    WORD wMinute;
    WORD wSecond;
    WORD wMilliseconds;
};

DWORD GetLastError();
void SetLastError(DWORD code);
BOOL CloseHandle(HANDLE handle);
HANDLE CreateEventA(void* attributes, BOOL manualReset, BOOL initialState, LPCSTR name);
HANDLE CreateFileA(LPCSTR fileName, DWORD desiredAccess, DWORD shareMode, void* securityAttributes,
                   DWORD creationDisposition, DWORD flagsAndAttributes, HANDLE templateFile);
BOOL SetFileAttributesA(LPCSTR fileName, DWORD attributes);
BOOL ReadFile(HANDLE file, LPVOID buffer, DWORD toRead, LPDWORD read, void* overlapped);
BOOL WriteFile(HANDLE file, LPCVOID buffer, DWORD toWrite, LPDWORD written, void* overlapped);
DWORD SetFilePointer(HANDLE file, LONG distance, PLONG distanceHigh, DWORD moveMethod);
DWORD GetFileSize(HANDLE file, LPDWORD sizeHigh);
BOOL SetEndOfFile(HANDLE file);
BOOL GetFileTime(HANDLE file, LPFILETIME creation, LPFILETIME lastAccess, LPFILETIME lastWrite);
HANDLE CreateFileMappingA(HANDLE file, void* attributes, DWORD protect, DWORD sizeHigh, DWORD sizeLow, LPCSTR name);
LPVOID MapViewOfFile(HANDLE mapping, DWORD access, DWORD offsetHigh, DWORD offsetLow, SIZE_T bytes);
BOOL UnmapViewOfFile(LPCVOID base);
DWORD FormatMessageA(DWORD flags, LPCVOID source, DWORD messageId, DWORD languageId, LPSTR buffer, DWORD size,
                     void* arguments);

constexpr UINT MB_OK = 0x0;
constexpr UINT MB_ICONHAND = 0x10;
constexpr int IDOK = 1;

struct SYSTEM_INFO {
    WORD wProcessorArchitecture;
    WORD wReserved;
    DWORD dwPageSize;
    LPVOID lpMinimumApplicationAddress;
    LPVOID lpMaximumApplicationAddress;
    DWORD dwActiveProcessorMask;
    DWORD dwNumberOfProcessors;
    DWORD dwProcessorType;
    DWORD dwAllocationGranularity;
    WORD wProcessorLevel;
    WORD wProcessorRevision;
};

void GetSystemInfo(SYSTEM_INFO* info);
HANDLE HeapCreate(DWORD options, SIZE_T initialSize, SIZE_T maximumSize);
BOOL HeapDestroy(HANDLE heap);
int MessageBoxA(HWND owner, LPCSTR text, LPCSTR caption, UINT type);
BOOL MessageBeep(UINT type);
// The port loads no external DLLs: LoadLibraryA always fails.
HANDLE LoadLibraryA(LPCSTR fileName);
int LoadStringA(HANDLE instance, UINT id, LPSTR buffer, int size);
BOOL FreeLibrary(HANDLE module);

using COLORREF = DWORD;
constexpr int SM_CXSCREEN = 0;
constexpr int SM_CYSCREEN = 1;
int GetSystemMetrics(int index);
int wsprintfA(LPSTR buffer, LPCSTR format, ...);

// DBCS navigation with the Big5 (code page 950) lead bytes 0x81-0xFE, independent of the host code page.
LPSTR CharNextA(LPCSTR current);
LPSTR CharPrevA(LPCSTR start, LPCSTR current);

int lstrlenA(LPCSTR string);
LPSTR lstrcpyA(LPSTR dest, LPCSTR src);
LPSTR lstrcatA(LPSTR dest, LPCSTR src);
int lstrcmpA(LPCSTR a, LPCSTR b);

}  // namespace w32

namespace swd3 {
using namespace w32;
}
