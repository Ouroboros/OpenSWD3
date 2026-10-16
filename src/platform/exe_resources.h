#pragma once

#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "core/types.h"

namespace swd3
{

class Surface;

// The resources of swd32.exe (the dialogs, bitmaps and sounds of the launcher), read from the executable in the game
// directory: the Win32 resource calls of the original find them in its own image.
class ExeResources
{
public:
    static constexpr u32 kBitmap = 2;  // RT_BITMAP
    static constexpr u32 kDialog = 5;  // RT_DIALOG

    bool Open(const std::string& path);
    // A resource of a numbered type, or of a named type such as "WAVE"; empty when missing.
    std::span<const u8> Find(u32 type, u32 id) const;
    std::span<const u8> Find(std::string_view type, u32 id) const;

private:
    struct Entry
    {
        u32 type = 0;
        std::string typeName;
        u32 id = 0;
        std::size_t offset = 0;
        std::size_t size = 0;
    };

    bool Parse();

    std::vector<u8> image;
    std::vector<Entry> entries;
};

// A device-independent bitmap of 8 (with its palette) or 24 bits per pixel converted to the display format; false for
// other formats.
bool DecodeBitmap(std::span<const u8> bitmap, Surface& out);

}  // namespace swd3
