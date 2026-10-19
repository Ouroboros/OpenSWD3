#pragma once

namespace swd3
{

// The rebuild's smooth battle (deviations.md): while a frame is drawn again between game frames, the active placer
// moves the place of each picture and text drawn (Painter::DrawPicture, DrawZoomed, TextFont::DrawText) along its way
// from the frame before.
class DrawPlacer
{
public:
    virtual ~DrawPlacer() = default;
    virtual void Place(int& x, int& y) = 0;

    static DrawPlacer* Active();

    // The placer active while it lives (none: the places stay).
    class Scope
    {
    public:
        explicit Scope(DrawPlacer* placer);
        ~Scope();
        Scope(const Scope&) = delete;
        Scope& operator=(const Scope&) = delete;

    private:
        DrawPlacer* outer = nullptr;
    };

private:
    static DrawPlacer* active;
};

}  // namespace swd3
