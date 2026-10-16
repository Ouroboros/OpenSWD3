#include "platform/glyph_rasterizer.h"

namespace swd3
{

namespace
{

class NoGlyphRasterizer : public GlyphRasterizer
{
public:
    void RenderText(Surface&, std::string_view, int, std::string_view, u32) override
    {
    }
};

}  // namespace

std::unique_ptr<GlyphRasterizer> CreateGlyphRasterizer()
{
    return std::make_unique<NoGlyphRasterizer>();
}

}  // namespace swd3
