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
    // `site`: where the draw is called from (the same code draws the same thing frame after frame); `source`: the
    // picture drawn (none for a text).
    virtual void Place(int& x, int& y, const void* site, const void* source) = 0;
    // The place of what the next draws show (a unit's stand, a flight's point), which stays where a new pose's
    // picture is drawn elsewhere: those draws move with it; `who` tells it from the others (a unit, a flight).
    virtual void Hold(bool held, int x, int y, const void* who) = 0;

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

    // The draws made while it lives show `who`, standing at (x, y).
    class Holding
    {
    public:
        Holding(int x, int y, const void* who = nullptr);
        ~Holding();
        Holding(const Holding&) = delete;
        Holding& operator=(const Holding&) = delete;
    };

private:
    static DrawPlacer* active;
};

}  // namespace swd3
