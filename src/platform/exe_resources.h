#pragma once

#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "core/types.h"

namespace swd3
{

class Surface;

// An icon or cursor picture in 32-bit ARGB: transparent where its mask is set, and the hotspot of a cursor.
struct IconImage
{
    int width = 0;
    int height = 0;
    int hotX = 0;
    int hotY = 0;
    std::vector<u32> pixels;
};

// The resources of swd32.exe (the dialogs, bitmaps and sounds of the launcher, the icon and cursor of the window),
// read from the executable in the game directory: the Win32 resource calls of the original find them in its own image.
class ExeResources
{
public:
    static constexpr u32 Cursor = 1;        // RT_CURSOR
    static constexpr u32 Bitmap = 2;        // RT_BITMAP
    static constexpr u32 Icon = 3;          // RT_ICON
    static constexpr u32 Dialog = 5;        // RT_DIALOG
    static constexpr u32 GroupCursor = 12;  // RT_GROUP_CURSOR
    static constexpr u32 GroupIcon = 14;    // RT_GROUP_ICON

    bool Open(const std::u8string& path);
    // A resource of a numbered type, or of a named type such as "WAVE"; empty when missing.
    std::span<const u8> Find(u32 type, u32 id) const;
    std::span<const u8> Find(std::string_view type, u32 id) const;
    // The member of icon group `group` (GroupIcon) or cursor group `group` (GroupCursor) with the most bits per
    // pixel, decoded; false when it is missing or not a bitmap of 1, 4, 8, 24 or 32 bits.
    bool FindIcon(u32 groupType, u32 group, IconImage& out) const;

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
