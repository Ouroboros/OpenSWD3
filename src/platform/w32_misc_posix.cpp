#include <unistd.h>

#include <cstdarg>
#include <cstdio>

#include "swd3/w32.h"

namespace w32 {
namespace {

int g_heapToken;

}  // namespace

void GetSystemInfo(SYSTEM_INFO* info) {
    *info = SYSTEM_INFO{};
    info->dwPageSize = static_cast<DWORD>(::sysconf(_SC_PAGESIZE));
    info->dwAllocationGranularity = 0x10000;
    info->dwNumberOfProcessors = static_cast<DWORD>(::sysconf(_SC_NPROCESSORS_ONLN));
    info->dwProcessorType = 586;
    info->dwActiveProcessorMask = 1;
}

HANDLE HeapCreate(DWORD, SIZE_T, SIZE_T) { return &g_heapToken; }

BOOL HeapDestroy(HANDLE heap) { return heap == &g_heapToken; }

int MessageBoxA(HWND, LPCSTR text, LPCSTR caption, UINT) {
    std::fprintf(stderr, "[MessageBox] %s: %s\n", caption ? caption : "", text ? text : "");
    return IDOK;
}

// wsprintfA writes at most 1024 characters including the terminator and has no floating-point support;
// the integer/string conversions used by the game format identically through vsnprintf.
int wsprintfA(LPSTR buffer, LPCSTR format, ...) {
    va_list args;
    va_start(args, format);
    int n = std::vsnprintf(buffer, 1024, format, args);
    va_end(args);
    return n > 1023 ? 1023 : n;
}

}  // namespace w32
