#include "pic/rle.h"

#include <cstring>

#include "crt/crt.h"
#include "data/globals.h"
#include "pic/pixel_format.h"

namespace swd3 {
namespace {

std::uint32_t magic() { return static_cast<std::uint32_t>(G.rleMagic); }

}  // namespace

// swd32.exe 0x004014F0
void* RleEncode(const void* pixels, std::uint32_t* size, std::uint32_t width, std::uint32_t height, int format) {
    *size = 0;
    if (format == 0x10) {
        auto* src = static_cast<const std::uint16_t*>(pixels);
        auto* base = static_cast<std::uint16_t*>(crt::malloc(width * height * 2 + 0x2000));
        base[1] = static_cast<std::uint16_t>(width);
        base[0] = static_cast<std::uint16_t>(G.rleMagic);
        base[2] = static_cast<std::uint16_t>(height);
        base[3] = 0x10;
        std::uint16_t* out = base + 4;
        *size += 4;
        if (height != 0) {
            for (std::uint32_t rows = height; rows != 0; --rows) {
                std::uint16_t* lineHeader = out;
                ++out;
                ++*size;
                std::uint32_t x = 1;
                std::uint32_t lineBytes = 2;
                std::uint16_t flag = 0x8000;
                const auto fill = static_cast<std::uint16_t>(G.rleFillColor16);
                const auto skip = static_cast<std::uint16_t>(G.rleSkipColor16);
                if (*src == fill || *src == skip) {
                    flag = 0;
                }
                do {
                    std::uint16_t c = *src;
                    std::uint32_t n;
                    std::uint16_t code;
                    if (c == static_cast<std::uint16_t>(G.rleFillColor16)) {
                        if (c != static_cast<std::uint16_t>(G.rleSkipColor16)) {
                            if (c != static_cast<std::uint16_t>(G.rleFillColor16)) {
                                continue;
                            }
                            n = 1;
                            if (x + 1 <= width) {
                                std::uint32_t j = x + 1;
                                const std::uint16_t* q = src + 1;
                                while (*q == static_cast<std::uint16_t>(G.rleFillColor16)) {
                                    ++n;
                                    ++q;
                                    ++j;
                                    if (j > width) {
                                        break;
                                    }
                                }
                            }
                            code = static_cast<std::uint16_t>(n) | 0xC000;
                            goto emitRun16;
                        }
                    } else if (c != static_cast<std::uint16_t>(G.rleSkipColor16)) {
                        std::uint16_t* countSlot = out;
                        std::uint32_t j = x + 1;
                        std::uint16_t first = *src;
                        ++out;
                        ++src;
                        n = 1;
                        *out = first;
                        *size += 2;
                        ++out;
                        lineBytes += 4;
                        while (j <= width) {
                            std::uint16_t v = *src;
                            if (v == static_cast<std::uint16_t>(G.rleFillColor16) ||
                                v == static_cast<std::uint16_t>(G.rleSkipColor16)) {
                                break;
                            }
                            *out = v;
                            ++*size;
                            ++n;
                            ++j;
                            ++out;
                            ++src;
                            lineBytes += 2;
                        }
                        x += n;
                        *countSlot = static_cast<std::uint16_t>(n);
                        continue;
                    }
                    n = 1;
                    if (x + 1 <= width) {
                        std::uint32_t j = x + 1;
                        const std::uint16_t* q = src + 1;
                        while (*q == static_cast<std::uint16_t>(G.rleSkipColor16)) {
                            ++n;
                            ++q;
                            ++j;
                            if (j > width) {
                                break;
                            }
                        }
                    }
                    code = static_cast<std::uint16_t>(n) | 0x8000;
                emitRun16:
                    *out = code;
                    lineBytes += 2;
                    ++out;
                    ++*size;
                    src += n;
                    x += n;
                } while (x <= width);
                *out = 0;
                ++out;
                ++*size;
                *lineHeader = static_cast<std::uint16_t>((lineBytes + 2) | flag);
            }
        }
        *out = 0;
        *size = *size * 2 + 2;
        void* result = crt::malloc(*size);
        std::memcpy(result, base, *size);
        crt::free(base);
        return result;
    }

    auto* src = static_cast<const std::uint8_t*>(pixels);
    auto* base = static_cast<std::uint16_t*>(crt::malloc(width * height + 0x2000));
    base[0] = static_cast<std::uint16_t>(G.rleMagic);
    base[1] = static_cast<std::uint16_t>(width);
    base[2] = static_cast<std::uint16_t>(height);
    base[3] = static_cast<std::uint16_t>(format);
    auto* out = reinterpret_cast<std::uint8_t*>(base + 4);
    *size += 8;
    for (std::uint32_t rows = height; rows != 0; --rows) {
        auto* lineHeader = out;
        *size += 2;
        out += 2;
        std::uint32_t x = 1;
        std::uint32_t lineBytes = 2;
        std::uint16_t flag = 0x8000;
        if (*src == static_cast<std::uint8_t>(G.rleFillColor8)) {
            flag = 0;
        }
        do {
            std::uint8_t c = *src;
            std::uint32_t n;
            std::uint16_t code;
            if (c == static_cast<std::uint8_t>(G.rleFillColor8)) {
                if (c != static_cast<std::uint8_t>(G.rleSkipColor8)) {
                    if (c != static_cast<std::uint8_t>(G.rleFillColor8)) {
                        continue;
                    }
                    n = 1;
                    for (std::uint32_t j = x + 1; j <= width; ++j) {
                        if (static_cast<std::uint32_t>(src[j - x]) != static_cast<std::uint32_t>(G.rleFillColor8)) {
                            break;
                        }
                        ++n;
                    }
                    code = static_cast<std::uint16_t>(n) | 0xC000;
                    goto emitRun8;
                }
            } else if (c != static_cast<std::uint8_t>(G.rleSkipColor8)) {
                std::uint8_t* countSlot = out;
                out += 2;
                std::uint32_t j = x + 1;
                n = 1;
                *out = *src;
                *size += 3;
                ++out;
                lineBytes += 3;
                ++src;
                while (j <= width) {
                    std::uint8_t v = *src;
                    if (v == static_cast<std::uint8_t>(G.rleFillColor8) || v == static_cast<std::uint8_t>(G.rleSkipColor8)) {
                        break;
                    }
                    ++n;
                    *out = v;
                    ++j;
                    ++out;
                    ++src;
                    ++*size;
                    ++lineBytes;
                }
                std::uint16_t count = static_cast<std::uint16_t>(n);
                std::memcpy(countSlot, &count, 2);
                x += n;
                continue;
            }
            n = 1;
            for (std::uint32_t j = x + 1; j <= width; ++j) {
                if (static_cast<std::uint32_t>(src[j - x]) != static_cast<std::uint32_t>(G.rleSkipColor8)) {
                    break;
                }
                ++n;
            }
            code = static_cast<std::uint16_t>(n) | 0x8000;
        emitRun8:
            std::memcpy(out, &code, 2);
            *size += 2;
            out += 2;
            lineBytes += 2;
            src += n;
            x += n;
        } while (x <= width);
        std::uint16_t zero = 0;
        std::memcpy(out, &zero, 2);
        *size += 2;
        std::uint16_t header = static_cast<std::uint16_t>((lineBytes + 2) | flag);
        std::memcpy(lineHeader, &header, 2);
        out += 2;
    }
    std::uint16_t zero = 0;
    std::memcpy(out, &zero, 2);
    *size += 2;
    void* result = crt::malloc(*size);
    std::memcpy(result, base, *size);
    return result;
}

// swd32.exe 0x004019A0
void* RleDecode(const std::uint16_t* rle, std::uint32_t* width, std::uint32_t* height, std::uint32_t* format) {
    if (static_cast<std::uint32_t>(rle[0]) != magic()) {
        return nullptr;
    }
    *width = rle[1];
    *height = rle[2];
    std::uint32_t fmt = rle[3] & 0x3FFF;
    *format = fmt;
    if (fmt == 0x10) {
        const std::uint16_t* src = rle + 4;
        auto* result = static_cast<std::uint16_t*>(crt::malloc(*width * *height * 2));
        std::uint16_t* out = result;
        while (*src != 0) {
            std::uint16_t code = src[1];
            src += 2;
            while (code != 0) {
                std::uint16_t n = code & 0x3FFF;
                std::uint16_t i = 0;
                switch (code & 0xC000) {
                case 0x0000:
                    do {
                        *out++ = *src++;
                    } while (++i != n);
                    break;
                case 0x8000:
                    do {
                        *out++ = static_cast<std::uint16_t>(G.rleSkipColor16);
                    } while (++i != n);
                    break;
                case 0xC000:
                    do {
                        *out++ = static_cast<std::uint16_t>(G.rleFillColor16);
                    } while (++i != n);
                    break;
                default:
                    break;
                }
                code = *src++;
            }
        }
        return result;
    }
    if (fmt == 8) {
        const std::uint16_t* src = rle + 4;
        auto* result = static_cast<std::uint8_t*>(crt::malloc(*width * *height));
        std::uint8_t* out = result;
        while (*src != 0) {
            std::uint16_t code = src[1];
            auto* bytes = reinterpret_cast<const std::uint8_t*>(src + 2);
            while (code != 0) {
                std::uint16_t n = code & 0x3FFF;
                std::uint16_t i = 0;
                switch (code & 0xC000) {
                case 0x0000:
                    do {
                        *out++ = *bytes++;
                    } while (++i != n);
                    break;
                case 0x8000:
                    do {
                        *out++ = static_cast<std::uint8_t>(G.rleSkipColor8);
                    } while (++i != n);
                    break;
                case 0xC000:
                    do {
                        *out++ = static_cast<std::uint8_t>(G.rleFillColor8);
                    } while (++i != n);
                    break;
                default:
                    break;
                }
                std::memcpy(&code, bytes, 2);
                bytes += 2;
            }
            src = reinterpret_cast<const std::uint16_t*>(bytes);
        }
        return result;
    }
    return nullptr;
}

// swd32.exe 0x00401B70
int RleCheck16(std::uint16_t* rle, std::uint16_t* width, std::uint16_t* height, std::uint16_t* format) {
    if (static_cast<std::uint32_t>(rle[0]) != magic()) {
        return 0;
    }
    if (width) {
        *width = rle[1];
    }
    if (height) {
        *height = rle[2];
    }
    if (format) {
        *format = rle[3] & 0x7FFF;
    }
    if ((rle[3] & 0x7FFF) != 0x10) {
        return 0;
    }
    std::uint16_t* p = rle + 4;
    while (*p != 0) {
        std::uint16_t code = p[1];
        p += 2;
        while (code != 0) {
            if ((code & 0xC000) == 0) {
                std::uint16_t n = code & 0x3FFF;
                PixelConvert(p, n);
                p += n;
            }
            code = *p++;
        }
    }
    return 1;
}

// swd32.exe 0x00401C70
void RleConvert8To16(Picture* picture) {
    std::uint16_t* src = picture->pixels;
    if (static_cast<std::uint32_t>(src[0]) != magic()) {
        return;
    }
    if ((src[3] & 0x7FFF) != 8) {
        std::uint16_t format = 0x10;
        RleCheck16(src, &picture->width, &picture->height, &format);
        return;
    }
    const std::uint16_t* palette = picture->palette;
    auto* out = static_cast<std::uint16_t*>(crt::malloc(static_cast<std::uint32_t>(src[2]) * src[1] * 4 + 0x800));
    out[1] = src[1];
    out[2] = src[2];
    out[0] = src[0];
    out[3] = (src[3] & 0x8010) | 0x10;
    std::uint16_t* dst = out + 4;
    src += 4;
    std::uint32_t total = 8;
    for (;;) {
        std::uint16_t* lineHeader = dst;
        ++dst;
        total += 2;
        if (*src == 0) {
            break;
        }
        *dst = src[1];
        ++src;
        std::uint32_t code = *src;
        ++dst;
        total += 2;
        ++src;
        std::uint32_t lineBytes = 4;
        while (code != 0) {
            if ((code & 0xC000) == 0) {
                std::uint32_t n = code & 0x3FFF;
                std::uint16_t* runStart = dst;
                std::uint32_t i = 0;
                auto* bytes = reinterpret_cast<const std::uint8_t*>(src);
                do {
                    std::uint8_t index = *bytes++;
                    ++dst;
                    total += 2;
                    dst[-1] = palette[index];
                    lineBytes += 2;
                    ++i;
                } while (i != n);
                src = reinterpret_cast<std::uint16_t*>(const_cast<std::uint8_t*>(bytes));
                PixelConvert(runStart, static_cast<int>(n));
            }
            std::uint16_t next;
            std::memcpy(&next, src, 2);
            *dst = next;
            lineBytes += 2;
            code = next;
            ++dst;
            total += 2;
            src = reinterpret_cast<std::uint16_t*>(reinterpret_cast<std::uint8_t*>(src) + 2);
        }
        *lineHeader = static_cast<std::uint16_t>(lineBytes);
    }
    dst[-1] = 0;
    total += 2;
    crt::free(picture->palette);
    crt::free(picture->pixels);
    auto* result = static_cast<std::uint16_t*>(crt::malloc(total));
    std::memcpy(result, out, total);
    crt::free(out);
    picture->size = total;
    picture->pixels = result;
    picture->palette = nullptr;
    return;
}

// swd32.exe 0x00401E50
void RleConvertTile8To16(std::uint16_t** data, std::uint32_t* size) {
    std::uint16_t* base = *data;
    if (static_cast<std::uint32_t>(base[0x100]) != magic()) {
        return;
    }
    if ((base[0x103] & 0x7FFF) != 8) {
        return;
    }
    auto* out = static_cast<std::uint16_t*>(crt::malloc(static_cast<std::uint32_t>(base[0x102]) * base[0x101] * 2 + 0x400));
    const std::uint8_t* src = reinterpret_cast<const std::uint8_t*>(base + 0x104);
    out[1] = base[0x101];
    out[2] = base[0x102];
    out[0] = base[0x100];
    out[3] = (base[0x103] & 0x8010) | 0x10;
    std::uint16_t* lineHeader = out + 4;
    std::uint16_t* dst = out + 5;
    std::uint32_t total = 0xA;
    auto readWord = [](const std::uint8_t* p) {
        std::uint16_t v;
        std::memcpy(&v, p, 2);
        return v;
    };
    std::uint16_t word = readWord(src);
    while (word != 0) {
        *dst = readWord(src + 2);
        src += 2;
        ++dst;
        std::uint16_t code = readWord(src);
        total += 2;
        src += 2;
        std::uint32_t lineBytes = 4;
        while (code != 0) {
            if ((code & 0xC000) == 0) {
                std::uint16_t* runStart = dst;
                std::uint16_t i = 0;
                do {
                    std::uint8_t index = *src++;
                    ++dst;
                    total += 2;
                    dst[-1] = base[index];
                    lineBytes += 2;
                    ++i;
                } while (i != code);
                PixelConvert(runStart, code);
            }
            std::uint16_t next = readWord(src);
            *dst = next;
            lineBytes += 2;
            code = next;
            ++dst;
            total += 2;
            src += 2;
        }
        *lineHeader = static_cast<std::uint16_t>(lineBytes);
        lineHeader = dst;
        ++dst;
        word = readWord(src);
        total += 2;
    }
    dst[-1] = 0;
    total += 2;
    crt::free(*data);
    total &= 0xFFFF;
    auto* result = static_cast<std::uint16_t*>(crt::malloc(total));
    std::memcpy(result, out, total);
    crt::free(out);
    *data = result;
    *size = total;
}

}  // namespace swd3
