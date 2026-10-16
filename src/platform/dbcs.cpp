#include "swd3/w32.h"

namespace w32 {
namespace {

bool isLeadByte(char c) {
    const auto b = static_cast<unsigned char>(c);
    return b >= 0x81 && b <= 0xFE;
}

}  // namespace

LPSTR CharNextA(LPCSTR current) {
    if (*current != 0) {
        if (isLeadByte(*current) && current[1] != 0) {
            ++current;
        }
        ++current;
    }
    return const_cast<LPSTR>(current);
}

LPSTR CharPrevA(LPCSTR start, LPCSTR current) {
    LPCSTR previous = start;
    for (LPCSTR p = start; p < current;) {
        previous = p;
        LPCSTR next = CharNextA(p);
        if (next == p) {
            break;
        }
        p = next;
    }
    return const_cast<LPSTR>(previous);
}

}  // namespace w32
