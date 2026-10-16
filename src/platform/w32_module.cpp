#include "swd3/w32.h"

namespace w32 {

HANDLE LoadLibraryA(LPCSTR) {
    SetLastError(ERROR_FILE_NOT_FOUND);
    return nullptr;
}

int LoadStringA(HANDLE, UINT, LPSTR buffer, int size) {
    if (buffer != nullptr && size > 0) {
        buffer[0] = 0;
    }
    return 0;
}

BOOL FreeLibrary(HANDLE) { return 0; }

}  // namespace w32
