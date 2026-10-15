#include "swd3/w32.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <cstdarg>

namespace w32 {

void GetSystemInfo(SYSTEM_INFO* info) {
    static_assert(sizeof(SYSTEM_INFO) == sizeof(::SYSTEM_INFO));
    ::GetSystemInfo(reinterpret_cast<::LPSYSTEM_INFO>(info));
}

HANDLE HeapCreate(DWORD options, SIZE_T initialSize, SIZE_T maximumSize) {
    return ::HeapCreate(options, initialSize, maximumSize);
}

BOOL HeapDestroy(HANDLE heap) { return ::HeapDestroy(heap); }

int MessageBoxA(HWND owner, LPCSTR text, LPCSTR caption, UINT type) {
    return ::MessageBoxA(static_cast<::HWND>(owner), text, caption, type);
}

int wsprintfA(LPSTR buffer, LPCSTR format, ...) {
    va_list args;
    va_start(args, format);
    int n = ::wvsprintfA(buffer, format, args);
    va_end(args);
    return n;
}

}  // namespace w32
