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

// The resources of swd32.exe (the dialogs, bitmaps and sounds of the launcher, the icon and cursor of the window): the
// Win32 resource calls of the original find them in its own image, the rebuild in the files extracted from it, one
// standard file each named by its resource id.
class ExeResources
{
public:
    static constexpr std::u8string_view Subdirectory = u8"launcher/";  // of the asset directory

    void Open(std::u8string directory);
    // <id>.bmp converted to the display format; false when it is missing or not a bitmap DecodeBitmap reads.
    bool LoadBitmap(u32 id, Surface& out) const;
    // The member of <id>.ico (<id>.cur) with the most bits per pixel, decoded; false when it is missing or not a
    // bitmap of 1, 4, 8, 24 or 32 bits.
    bool LoadIcon(u32 id, IconImage& out) const;
    bool LoadCursor(u32 id, IconImage& out) const;
    // <id>.wav and the dialog template <id>.dlg as stored; empty when missing.
    std::vector<u8> ReadSound(u32 id) const;
    std::vector<u8> ReadDialog(u32 id) const;

private:
    std::vector<u8> Read(u32 id, std::u8string_view extension) const;
    bool LoadGroup(u32 id, std::u8string_view extension, IconImage& out) const;

    std::u8string directory;
};

// A device-independent bitmap of 8 (with its palette) or 24 bits per pixel converted to the display format; false for
// other formats.
bool DecodeBitmap(std::span<const u8> bitmap, Surface& out);

}  // namespace swd3
