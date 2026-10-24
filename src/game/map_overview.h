#pragma once

#include <optional>
#include <string_view>
#include <vector>

#include "core/point.h"
#include "core/types.h"

struct SDL_Texture;
union SDL_Event;

namespace swd3
{

class Game;
class Surface;
class TextFont;

// The rebuild's map overview (deviations.md): Tab on the map shows the whole map filling the window, the game paused
// as in the main menu: the map's picture without its people, its exits with the names of the maps they lead to, and
// the controlled role where it stands. The wheel zooms about the pointer, dragging or the arrow keys move the map, Tab
// or Escape close it. It is drawn on the window's output at every refresh (Display overlays), apart from the game
// frames.
class MapOverview
{
public:
    explicit MapOverview(Game& game);
    ~MapOverview();
    MapOverview(const MapOverview&) = delete;
    MapOverview& operator=(const MapOverview&) = delete;

    bool IsOpen() const;
    // Opens it when the map is shown with no menu, battle, event or movie (as the system button opens the main menu);
    // false otherwise.
    bool TryOpen();
    void Close();
    // An event while it is open (it takes them all).
    void HandleEvent(const SDL_Event& event);
    void Present();

private:
    // A picture of the overview and its size in its own pixels.
    struct Image
    {
        SDL_Texture* texture = nullptr;
        Point size;
    };

    // An exit: its name, where it lies on the map (pixels).
    struct Exit
    {
        Image name;
        Point at;
    };

    Image MakeImage(const Surface& pixels, bool smooth, bool keyed);
    Image MakeText(TextFont& font, std::string_view text, u16 color);
    void MakePanorama();
    void MakeExits();
    void MakePlayer();
    void Release();

    // The rows of the output the map is shown in: below the band of the map's name and the keys.
    struct Rows
    {
        double top = 0.0;
        double bottom = 0.0;
    };

    Rows MapRows() const;
    // The zoom fitting the whole map, the most zoom, and the scale of the game's own screen on the output.
    double FitZoom() const;
    double MostZoom() const;
    double ScreenScale() const;
    void ZoomTo(double zoom, double outputX, double outputY);
    void Step(double seconds);
    void Clamp();

    Game& game;
    bool open = false;
    std::vector<Image> levels;  // the map's picture, each level half the size of the one before
    Point mapSize;
    std::vector<Exit> exits;
    Image marker;       // drawn at each exit
    Image title;        // the map's name
    Image hint;         // the keys
    Image player;       // the controlled role's picture
    Point playerAt;     // where its picture lies on the map (its top left)
    double zoom = 1.0;  // output pixels per map pixel
    double wantedZoom = 1.0;
    // The zoom it was closed at, in parts of the game screen's scale, opened at again (kept while the game runs).
    std::optional<double> keptZoom;
    // The map point kept at an output point while the zoom moves (the pointer at a turn of the wheel).
    double anchorMapX = 0.0;
    double anchorMapY = 0.0;
    double anchorX = 0.0;
    double anchorY = 0.0;
    bool anchored = false;
    double zoomHeld = 0.0;  // seconds a zoom key has been held
    double centerX = 0.0;   // the map point at the middle of the output
    double centerY = 0.0;
    bool dragging = false;
    double dragX = 0.0;  // the pointer at the last move of a drag (output pixels)
    double dragY = 0.0;
    u64 shownNs = 0;  // when it was last presented
};

}  // namespace swd3
