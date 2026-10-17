#include "game/screenshot.h"

#include <array>
#include <vector>

#include "draw/painter.h"
#include "file/file.h"

namespace swd3
{

namespace
{

constexpr std::size_t kFileHeaderBytes = 0xE;
constexpr std::size_t kInfoHeaderBytes = 0x28;
constexpr u16 kBitmapTag = 0x4D42;  // "BM"
constexpr u16 kPlanes = 1;
constexpr u16 kBitsPerPixel = 0x18;
constexpr std::size_t kInfoWidthAt = 4;
constexpr std::size_t kInfoHeightAt = 8;
constexpr std::size_t kInfoPlanesAt = 0xC;
constexpr std::size_t kInfoBitsAt = 0xE;
constexpr i32 kFileSizeAt = 2;
constexpr i32 kPixelsAtField = 0xA;
constexpr u32 kPixelsAt = kFileHeaderBytes + kInfoHeaderBytes;
constexpr int kBytesPerPixel = 3;
constexpr int kRowAlign = 4;
constexpr int kRowsPerPump = 0xF;  // the audio is served every 15 rows
constexpr int kBlueShift = 3;      // RGB555 to 8-bit channels
constexpr int kGreenShift = 2;
constexpr int kRedShift = 7;
constexpr u8 kChannelMask = 0xF8;

template <std::size_t Size>
void PutWord(std::array<u8, Size>& bytes, std::size_t at, u16 value)
{
    bytes[at] = static_cast<u8>(value);
    bytes[at + 1] = static_cast<u8>(value >> 8);
}

template <std::size_t Size>
void PutDword(std::array<u8, Size>& bytes, std::size_t at, u32 value)
{
    PutWord(bytes, at, static_cast<u16>(value));
    PutWord(bytes, at + 2, static_cast<u16>(value >> 16));
}

}  // namespace

// swd32.exe 0x004303D0: the `width` x `height` picture `pixels` saved as a 24-bit BMP named `fileName` (the pixels
// converted back to RGB555 in a copy, rows bottom up padded to 4 bytes, `serveAudio` (AIL_serve) every 15 rows and at
// the end); the file size and the pixel offset are written into the header last. 0 when the file does not open.
// swd32.exe 0x004982F0 (EH unwind: the cFile, closed by its destructor)
// swd32.exe 0x004982F8 (EH handler of 0x004303D0)
int SaveScreenshot(
    const Painter& painter,
    std::span<const u16> pixels,
    int width,
    int height,
    std::string_view fileName,
    const std::function<void()>& serveAudio
)
{
    const auto count = static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
    std::vector<u16> copy(pixels.begin(), pixels.begin() + static_cast<std::ptrdiff_t>(count));
    painter.ConvertPixelsBack(copy);
    File file;

    if (!file.Open(fileName, File::Disposition::OpenAlways, File::Access::Write))
    {
        return 0;
    }

    file.SeekBegin(0);
    file.Truncate();
    std::array<u8, kFileHeaderBytes> fileHeader = {};
    PutWord(fileHeader, 0, kBitmapTag);
    u32 bytes = kFileHeaderBytes;
    file.Write(fileHeader.data(), bytes);
    std::array<u8, kInfoHeaderBytes> infoHeader = {};
    PutDword(infoHeader, 0, kInfoHeaderBytes);
    PutDword(infoHeader, kInfoWidthAt, static_cast<u32>(width));
    PutDword(infoHeader, kInfoHeightAt, static_cast<u32>(height));
    PutWord(infoHeader, kInfoPlanesAt, kPlanes);
    PutWord(infoHeader, kInfoBitsAt, kBitsPerPixel);
    bytes = kInfoHeaderBytes;
    file.Write(infoHeader.data(), bytes);
    const auto stride = static_cast<u32>(kRowAlign * ((kBytesPerPixel * width + kRowAlign - 1) / kRowAlign));
    std::vector<u8> row(stride, 0);

    for (int line = 0; line < height; ++line)
    {
        const auto from = static_cast<std::size_t>(height - 1 - line) * static_cast<std::size_t>(width);

        for (int column = 0; column < width; ++column)
        {
            const u16 pixel = copy[from + static_cast<std::size_t>(column)];
            const auto at = static_cast<std::size_t>(kBytesPerPixel * column);
            row[at] = static_cast<u8>(pixel << kBlueShift);
            row[at + 1] = static_cast<u8>((pixel >> kGreenShift) & kChannelMask);
            row[at + 2] = static_cast<u8>((pixel >> kRedShift) & kChannelMask);
        }

        bytes = stride;
        file.Write(row.data(), bytes);

        if (line % kRowsPerPump == 0)
        {
            serveAudio();
        }
    }

    u32 size = static_cast<u32>(count) * 2;
    file.Tell(size);
    file.SeekBegin(kFileSizeAt);
    file.WriteDword(size);
    file.SeekBegin(kPixelsAtField);
    file.WriteDword(kPixelsAt);
    file.Close();
    serveAudio();
    return 1;
}

}  // namespace swd3
