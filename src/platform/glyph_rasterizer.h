#pragma once

#include <memory>
#include <string_view>

#include "core/types.h"

namespace swd3
{

class Surface;

// Draws Big5 text the way the game draws it with GDI: TextOutA at (0, 0) with a Big5 font of `size` pixels (normal
// weight) in face `face`, the given colour on an opaque black background. The original creates the font with the
// default quality, which drew without smoothing on the systems of its time; the Windows rasterizer asks for
// non-antialiased output to keep that look. Elsewhere no glyphs are drawn.
class GlyphRasterizer
{
public:
    virtual ~GlyphRasterizer() = default;
    virtual void RenderText(Surface& surface, std::string_view face, int size, std::string_view text, u32 color) = 0;
};

// The rasterizer of the platform (GDI on Windows, none elsewhere).
std::unique_ptr<GlyphRasterizer> CreateGlyphRasterizer();

}  // namespace swd3
