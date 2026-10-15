#include "codec/lzo.h"

#include <cstring>

namespace swd3 {
namespace {

inline void copy4(std::uint8_t* dst, const std::uint8_t* src) { std::memcpy(dst, src, 4); }

}  // namespace

// swd32.exe 0x004399E0
int Lzo1xDecompress(const std::uint8_t* in, std::uint32_t inLength, std::uint8_t* out, std::uint32_t* outLength) {
    const std::uint8_t* ip = in;
    const std::uint8_t* const ipEnd = in + inLength;
    std::uint8_t* op = out;
    const std::uint8_t* mPos;
    std::uint32_t t;

    *outLength = 0;
    if (*ip > 17) {
        t = static_cast<std::uint32_t>(*ip++) - 17;
        if (t < 4) {
            goto matchNext;
        }
        do {
            *op++ = *ip++;
        } while (--t > 0);
        goto firstLiteralRun;
    }

    for (;;) {
        t = *ip++;
        if (t >= 16) {
            goto match;
        }
        if (t == 0) {
            while (*ip == 0) {
                t += 255;
                ip++;
            }
            t += 15 + *ip++;
        }
        copy4(op, ip);
        op += 4;
        ip += 4;
        if (--t > 0) {
            if (t >= 4) {
                do {
                    copy4(op, ip);
                    op += 4;
                    ip += 4;
                    t -= 4;
                } while (t >= 4);
                if (t > 0) {
                    do {
                        *op++ = *ip++;
                    } while (--t > 0);
                }
            } else {
                do {
                    *op++ = *ip++;
                } while (--t > 0);
            }
        }

    firstLiteralRun:
        t = *ip++;
        if (t >= 16) {
            goto match;
        }
        mPos = op - (1 + 0x0800);
        mPos -= t >> 2;
        mPos -= static_cast<std::uint32_t>(*ip++) << 2;
        *op++ = *mPos++;
        *op++ = *mPos++;
        *op++ = *mPos;
        goto matchDone;

        for (;;) {
        match:
            if (t >= 64) {
                mPos = op - 1;
                mPos -= (t >> 2) & 7;
                mPos -= static_cast<std::uint32_t>(*ip++) << 3;
                t = (t >> 5) - 1;
                goto copyMatch;
            } else if (t >= 32) {
                t &= 31;
                if (t == 0) {
                    while (*ip == 0) {
                        t += 255;
                        ip++;
                    }
                    t += 31 + *ip++;
                }
                std::uint16_t offset;
                std::memcpy(&offset, ip, 2);
                mPos = op - 1;
                mPos -= offset >> 2;
                ip += 2;
            } else if (t >= 16) {
                mPos = op;
                mPos -= (t & 8) << 11;
                t &= 7;
                if (t == 0) {
                    while (*ip == 0) {
                        t += 255;
                        ip++;
                    }
                    t += 7 + *ip++;
                }
                std::uint16_t offset;
                std::memcpy(&offset, ip, 2);
                mPos -= offset >> 2;
                ip += 2;
                if (mPos == op) {
                    goto eofFound;
                }
                mPos -= 0x4000;
            } else {
                mPos = op - 1;
                mPos -= t >> 2;
                mPos -= static_cast<std::uint32_t>(*ip++) << 2;
                *op++ = *mPos++;
                *op++ = *mPos;
                goto matchDone;
            }

            if (t >= 2 * 4 - (3 - 1) && static_cast<std::int32_t>(op - mPos) >= 4) {
                copy4(op, mPos);
                op += 4;
                mPos += 4;
                t -= 4 - (3 - 1);
                do {
                    copy4(op, mPos);
                    op += 4;
                    mPos += 4;
                    t -= 4;
                } while (t >= 4);
                if (t > 0) {
                    do {
                        *op++ = *mPos++;
                    } while (--t > 0);
                }
            } else {
            copyMatch:
                *op++ = *mPos++;
                *op++ = *mPos++;
                do {
                    *op++ = *mPos++;
                } while (--t > 0);
            }

        matchDone:
            t = ip[-2] & 3;
            if (t == 0) {
                break;
            }

        matchNext:
            do {
                *op++ = *ip++;
            } while (--t > 0);
            t = *ip++;
        }
    }

eofFound:
    *outLength = static_cast<std::uint32_t>(op - out);
    if (ip == ipEnd) {
        return 0;
    }
    return ip < ipEnd ? -8 : -4;
}

}  // namespace swd3
