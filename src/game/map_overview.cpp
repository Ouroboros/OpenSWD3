#include "game/map_overview.h"

#include <SDL3/SDL.h>

#include <algorithm>
#include <cmath>
#include <string>

#include "core/log.h"
#include "game/game.h"
#include "platform/display.h"
#include "platform/surface.h"
#include "text/big5.h"

namespace swd3
{

namespace
{

constexpr double kNanosecondsPerSecond = 1e9;
// The clear colour of the overview's text and pictures: next to black, so that their black edges smoothed into it
// show no fringe.
constexpr u16 kKey = 0x0001;
constexpr u16 kBlack = 0x0000;
constexpr u16 kWhite = 0xFFFF;
constexpr u16 kYellow = 0xFFE0;
constexpr int kTextRows = 32;  // a line of text is drawn in a strip this high, then cut to what it drew
constexpr int kTextInset = 4;
constexpr int kSmallestLevel = 256;    // the map's picture is halved until it is no larger than this
constexpr double kWheelStep = 1.25;    // the zoom of a turn of the wheel or of a key
constexpr double kZoomSeconds = 0.08;  // the zoom moves to the one wanted with this time constant
constexpr double kRepeatDelay = 0.3;   // a zoom key held this long zooms on by itself
constexpr double kHeldZoom = 3.0;      // the zoom of a second of a held zoom key
constexpr double kPanSpeed = 1200.0;   // output pixels a second the arrow keys move the map
constexpr double kMostSeconds = 0.1;   // a longer wait between presentations moves as far as this
constexpr double kOpenZoom = 0.5;      // opened at this part of the game screen's scale (or fitting the whole map)
constexpr double kMostZoom = 2.0;      // up to this many times the game screen's scale
constexpr double kFitShare = 0.95;     // the whole map fills this much of the output at the least zoom
// The text, and the controlled role and the exits at the zoom it opens at (they grow and shrink with the map), at
// this part of the game screen's scale.
constexpr double kTextShare = 0.6;
constexpr double kPlayerShare = 0.6;
constexpr double kMarkerSize = 7.0;   // the exit marker: a white dot with a black edge, this wide in game pixels
constexpr int kMarkerPixels = 32;     // drawn this wide, smoothed down
constexpr double kMarkerEdge = 0.18;  // the edge's part of the radius
constexpr u8 kShadeAlpha = 160;       // the boxes under the text
constexpr double kNameGap = 3.0;      // game pixels between an exit's marker and its name
constexpr double kTextMargin = 6.0;   // game pixels around the title and the hint
constexpr std::u8string_view kHint = u8"滾輪：縮放　拖曳或方向鍵：移動　Tab 或 Esc：關閉";
constexpr std::u8string_view kNameJoint = u8"／";

// `from` halved each way, each pixel the average of the up to four it covers.
Surface Halve(const Surface& from)
{
    const int width = (from.Width() + 1) / 2;
    const int height = (from.Height() + 1) / 2;
    Surface to(width, height);

    for (int y = 0; y < height; ++y)
    {
        u16* row = to.Row(y);

        for (int x = 0; x < width; ++x)
        {
            u32 red = 0;
            u32 green = 0;
            u32 blue = 0;
            u32 count = 0;

            for (int down = 0; down < 2 && 2 * y + down < from.Height(); ++down)
            {
                const u16* source = from.Row(2 * y + down);

                for (int across = 0; across < 2 && 2 * x + across < from.Width(); ++across)
                {
                    const u16 pixel = source[2 * x + across];
                    red += pixel & Surface::RedMask;
                    green += pixel & Surface::GreenMask;
                    blue += pixel & Surface::BlueMask;
                    ++count;
                }
            }

            row[x] = static_cast<u16>(
                ((red / count) & Surface::RedMask) | ((green / count) & Surface::GreenMask) |
                ((blue / count) & Surface::BlueMask)
            );
        }
    }

    return to;
}

}  // namespace

MapOverview::MapOverview(Game& game)
    : game(game)
{
}

MapOverview::~MapOverview()
{
    this->Release();
}

bool MapOverview::IsOpen() const
{
    return this->open;
}

bool MapOverview::TryOpen()
{
    Game& owner = this->game;
    const GameState& state = owner.state;

    if (this->open || owner.running == 0 || owner.active == 0 || owner.inactive != 0 ||
        (owner.systemFlags & Game::VideoPlaying) != 0 || !state.GameFlag(GameState::Running) || owner.inBattle != 0 ||
        owner.battleRequest != 0 || owner.menuState != Game::NoMenu || state.leader.id != Role::NoId ||
        state.GameFlag(GameState::TextBox) || owner.fieldView.movie.active != 0 || owner.currentMap == 0 ||
        state.map.width <= 0 || state.map.height <= 0)
    {
        return false;
    }

    this->MakePanorama();

    if (this->levels.empty())
    {
        this->Release();
        return false;
    }

    this->MakeExits();
    this->MakePlayer();
    this->title = this->MakeText(owner.largeFont, std::string(state.map.name.data()), kYellow);
    this->hint = this->MakeText(owner.mediumFont, Big5(kHint), kWhite);
    // Opened at the zoom it was closed at (within this map's least and most), the first time at kOpenZoom.
    this->zoom = this->keptZoom.has_value()
                     ? std::clamp(
                           *this->keptZoom * this->ScreenScale(),
                           std::min(this->FitZoom(), this->MostZoom()),
                           this->MostZoom()
                       )
                     : std::max(this->FitZoom(), std::min(kOpenZoom * this->ScreenScale(), this->MostZoom()));
    this->wantedZoom = this->zoom;
    this->anchored = false;
    this->dragging = false;
    this->centerX = this->playerAt.x + this->player.size.x / 2.0;
    this->centerY = this->playerAt.y + this->player.size.y / 2.0;
    this->Clamp();
    this->open = true;
    this->shownNs = SDL_GetTicksNS();
    owner.display.ShowSystemPointer(true);
    LogInfo(
        u8"map overview: map %d %dx%d, %d exits, %d levels",
        owner.currentMap,
        this->mapSize.x,
        this->mapSize.y,
        static_cast<i32>(this->exits.size()),
        static_cast<i32>(this->levels.size())
    );
    return true;
}

void MapOverview::Close()
{
    if (!this->open)
    {
        return;
    }

    this->open = false;
    this->dragging = false;
    this->keptZoom = this->wantedZoom / this->ScreenScale();
    this->Release();
    this->game.display.ShowSystemPointer(false);
    this->game.controls.SpendHeld();
}

void MapOverview::HandleEvent(const SDL_Event& event)
{
    Game& owner = this->game;
    SDL_Event output = event;
    owner.display.ToOutput(output);

    switch (event.type)
    {
        case SDL_EVENT_KEY_DOWN:
            if (event.key.repeat)
            {
                break;
            }

            if (event.key.scancode == SDL_SCANCODE_TAB || event.key.scancode == SDL_SCANCODE_ESCAPE)
            {
                this->Close();
            }
            else if (
                event.key.scancode == SDL_SCANCODE_EQUALS || event.key.scancode == SDL_SCANCODE_KP_PLUS ||
                event.key.scancode == SDL_SCANCODE_PAGEUP
            )
            {
                const Point size = owner.display.OutputSize();
                const Rows rows = this->MapRows();
                this->ZoomTo(this->wantedZoom * kWheelStep, size.x / 2.0, (rows.top + rows.bottom) / 2.0);
            }
            else if (
                event.key.scancode == SDL_SCANCODE_MINUS || event.key.scancode == SDL_SCANCODE_KP_MINUS ||
                event.key.scancode == SDL_SCANCODE_PAGEDOWN
            )
            {
                const Point size = owner.display.OutputSize();
                const Rows rows = this->MapRows();
                this->ZoomTo(this->wantedZoom / kWheelStep, size.x / 2.0, (rows.top + rows.bottom) / 2.0);
            }
            else if (event.key.scancode == SDL_SCANCODE_HOME)
            {
                this->anchored = false;
                this->centerX = this->playerAt.x + this->player.size.x / 2.0;
                this->centerY = this->playerAt.y + this->player.size.y / 2.0;
                this->Clamp();
            }

            break;

        case SDL_EVENT_MOUSE_WHEEL:
            if (output.wheel.y != 0.0F)
            {
                this->ZoomTo(
                    this->wantedZoom * std::pow(kWheelStep, static_cast<double>(output.wheel.y)),
                    output.wheel.mouse_x,
                    output.wheel.mouse_y
                );
            }

            break;

        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            if (event.button.button == SDL_BUTTON_LEFT || event.button.button == SDL_BUTTON_RIGHT)
            {
                this->dragging = true;
                this->dragX = output.button.x;
                this->dragY = output.button.y;
                owner.display.ShowSystemPointer(true, true);
            }

            break;

        case SDL_EVENT_MOUSE_BUTTON_UP:
            if (this->dragging)
            {
                this->dragging = false;
                owner.display.ShowSystemPointer(true);
            }

            break;

        case SDL_EVENT_MOUSE_MOTION:
            if (this->dragging)
            {
                this->anchored = false;
                this->centerX -= (output.motion.x - this->dragX) / this->zoom;
                this->centerY -= (output.motion.y - this->dragY) / this->zoom;
                this->dragX = output.motion.x;
                this->dragY = output.motion.y;
                this->Clamp();
            }

            break;

        default:
            break;
    }
}

void MapOverview::Present()
{
    Game& owner = this->game;
    Display& display = owner.display;
    const u64 now = SDL_GetTicksNS();
    const double seconds = std::min(static_cast<double>(now - this->shownNs) / kNanosecondsPerSecond, kMostSeconds);
    this->shownNs = now;
    this->Step(seconds);

    const Point output = display.OutputSize();
    const double scale = this->ScreenScale();
    const Rows rows = this->MapRows();
    const double originX = output.x / 2.0 - this->centerX * this->zoom;
    const double originY = (rows.top + rows.bottom) / 2.0 - this->centerY * this->zoom;
    // The level drawn is the smallest at least as large as shown.
    std::size_t level = 0;

    while (level + 1 < this->levels.size() && this->zoom * static_cast<double>(1 << (level + 1)) <= 1.0)
    {
        ++level;
    }

    display.BeginOverlays();
    Display::SetOverlaySmooth(this->levels[level].texture, this->zoom < kMostZoom);
    display.DrawOverlay(
        this->levels[level].texture,
        Viewport{
            static_cast<float>(originX),
            static_cast<float>(originY),
            static_cast<float>(this->mapSize.x * this->zoom),
            static_cast<float>(this->mapSize.y * this->zoom),
        }
    );

    // The controlled role stands on its feet; it and the exits grow and shrink with the map.
    const double grown = this->zoom / (kOpenZoom * scale);
    const double shown = kPlayerShare * scale * grown;
    const double feetX = originX + (this->playerAt.x + this->player.size.x / 2.0) * this->zoom;
    const double feetY = originY + (this->playerAt.y + this->player.size.y) * this->zoom;
    display.DrawOverlay(
        this->player.texture,
        Viewport{
            static_cast<float>(feetX - this->player.size.x * shown / 2.0),
            static_cast<float>(feetY - this->player.size.y * shown),
            static_cast<float>(this->player.size.x * shown),
            static_cast<float>(this->player.size.y * shown),
        }
    );

    // Each exit: its marker, its name above it on a shaded box; while the marker is shown, its name is kept on the
    // output below the band (below the marker when there is no room above it).
    const double text = kTextShare * scale;
    const double nameText = text * grown;
    const double band = rows.top;

    for (const Exit& exit : this->exits)
    {
        const double x = originX + exit.at.x * this->zoom;
        const double y = originY + exit.at.y * this->zoom;
        const double markerSize = kMarkerSize * scale * grown;
        display.DrawOverlay(
            this->marker.texture,
            Viewport{
                static_cast<float>(x - markerSize / 2.0),
                static_cast<float>(y - markerSize / 2.0),
                static_cast<float>(markerSize),
                static_cast<float>(markerSize),
            }
        );
        const double nameWidth = exit.name.size.x * nameText;
        const double nameHeight = exit.name.size.y * nameText;
        const double gap = markerSize / 2.0 + kNameGap * scale * grown;
        double left = x - nameWidth / 2.0;
        double top = y - gap - nameHeight;

        if (x >= 0.0 && x <= output.x && y >= band && y <= rows.bottom)
        {
            left = std::clamp(left, 0.0, std::max(0.0, output.x - nameWidth));

            if (top < band)
            {
                top = y + gap;
            }

            top = std::clamp(top, band, std::max(band, rows.bottom - nameHeight));
        }

        const Viewport name{
            static_cast<float>(left),
            static_cast<float>(top),
            static_cast<float>(nameWidth),
            static_cast<float>(nameHeight),
        };

        display.ShadeOverlay(name, kShadeAlpha);
        display.DrawOverlay(exit.name.texture, name);
    }

    // The map's name in the middle and the keys at the right of a shaded band at the top.
    const double margin = kTextMargin * scale;
    display.ShadeOverlay(Viewport{0.0F, 0.0F, static_cast<float>(output.x), static_cast<float>(band)}, kShadeAlpha);
    display.DrawOverlay(
        this->title.texture,
        Viewport{
            static_cast<float>((output.x - this->title.size.x * text) / 2.0),
            static_cast<float>((band - this->title.size.y * text) / 2.0),
            static_cast<float>(this->title.size.x * text),
            static_cast<float>(this->title.size.y * text),
        }
    );
    display.DrawOverlay(
        this->hint.texture,
        Viewport{
            static_cast<float>(output.x - margin - this->hint.size.x * text),
            static_cast<float>((band - this->hint.size.y * text) / 2.0),
            static_cast<float>(this->hint.size.x * text),
            static_cast<float>(this->hint.size.y * text),
        }
    );
    display.EndOverlays();
}

MapOverview::Image MapOverview::MakeImage(const Surface& pixels, bool smooth, bool keyed)
{
    return Image{this->game.display.CreateOverlay(pixels, smooth, keyed, kKey), {pixels.Width(), pixels.Height()}};
}

// The text drawn outlined in black on a strip of the clear colour, cut to what it drew.
MapOverview::Image MapOverview::MakeText(TextFont& font, std::string_view text, u16 color)
{
    Surface strip(Display::Width, kTextRows);
    strip.Fill(kKey);
    const TextFont::DrawState kept = font.SaveDrawState();
    font.SetTarget(Display::Width, kTextRows);
    font.SetClip(0, 0, Display::Width, kTextRows);
    font.SetBackground(TextFont::NoBackground);
    font.SetShadow(kBlack);
    font.DrawText(strip.Pixels(), kTextInset, kTextInset, text, color, TextFont::BoldOutline);
    font.RestoreDrawState(kept);
    int left = Display::Width;
    int right = -1;
    int top = kTextRows;
    int bottom = -1;

    for (int y = 0; y < kTextRows; ++y)
    {
        for (int x = 0; x < Display::Width; ++x)
        {
            if (strip.Row(y)[x] != kKey)
            {
                left = std::min(left, x);
                right = std::max(right, x);
                top = std::min(top, y);
                bottom = std::max(bottom, y);
            }
        }
    }

    if (right < left || bottom < top)
    {
        return Image{};
    }

    Surface cut(right - left + 1, bottom - top + 1);

    for (int y = top; y <= bottom; ++y)
    {
        std::copy_n(strip.Row(y) + left, cut.Width(), cut.Row(y - top));
    }

    return this->MakeImage(cut, true, true);
}

void MapOverview::MakePanorama()
{
    Surface whole;
    this->game.fieldView.PaintOverview(whole);
    this->mapSize = {whole.Width(), whole.Height()};
    this->levels.push_back(this->MakeImage(whole, true, false));

    while (std::max(whole.Width(), whole.Height()) > kSmallestLevel)
    {
        whole = Halve(whole);
        this->levels.push_back(this->MakeImage(whole, true, false));
    }

    if (std::any_of(
            this->levels.begin(),
            this->levels.end(),
            [](const Image& image)
            {
                return image.texture == nullptr;
            }
        ))
    {
        this->Release();
    }
}

// The exits of the map, named by the maps they lead to (joined when one leads to several); the marker drawn at each.
void MapOverview::MakeExits()
{
    Game& owner = this->game;

    for (const FieldView::MapExit& exit : owner.fieldView.MapExits())
    {
        std::string names;

        for (const u16 map : exit.maps)
        {
            std::string name;

            if (!owner.MapName(map, name))
            {
                continue;
            }

            names += names.empty() ? name : Big5(kNameJoint) + name;
        }

        if (!names.empty())
        {
            this->exits.push_back(Exit{this->MakeText(owner.mediumFont, names, kWhite), exit.at});
        }
    }

    Surface dot(kMarkerPixels, kMarkerPixels);
    const double radius = kMarkerPixels / 2.0;

    for (int y = 0; y < kMarkerPixels; ++y)
    {
        for (int x = 0; x < kMarkerPixels; ++x)
        {
            const double distance = std::hypot(x + 0.5 - radius, y + 0.5 - radius) / radius;
            dot.Row(y)[x] = distance > 1.0 ? kKey : distance > 1.0 - kMarkerEdge ? kBlack : kWhite;
        }
    }

    this->marker = this->MakeImage(dot, true, true);
}

// The controlled role's picture as the last game frame drew it, on the clear colour.
void MapOverview::MakePlayer()
{
    Game& owner = this->game;
    const Role& role = owner.state.Player();
    const swd3::Picture* picture = owner.archive.GetPicture(role.act.frame, role.act.picture);
    this->playerAt = {role.x, role.y};

    if (picture == nullptr || picture->width <= 0 || picture->height <= 0)
    {
        return;
    }

    Painter& painter = owner.painter;
    Surface sprite(picture->width, picture->height);
    sprite.Fill(kKey);
    const Painter::DrawState kept = painter.SaveDrawState();
    owner.SetDrawSize(picture->width, picture->height);
    painter.SetTarget(sprite.Pixels());
    painter.SetClipClamped(0, 0, picture->width, picture->height);
    painter.Options() = DrawOptions{};
    painter.SetSource(picture->data.data());
    painter.DrawPicture(0, 0, picture->width, picture->height, role.act.flags, PaletteArgument(*picture));
    owner.SetDrawSize(Display::Width, Display::Height);
    painter.RestoreDrawState(kept);
    this->playerAt = {role.x + role.drawOffsetX - role.act.x, role.y + role.drawOffsetY - role.act.y};
    this->player = this->MakeImage(sprite, false, true);
}

void MapOverview::Release()
{
    for (Image& level : this->levels)
    {
        Display::DestroyOverlay(level.texture);
    }

    for (Exit& exit : this->exits)
    {
        Display::DestroyOverlay(exit.name.texture);
    }

    this->levels.clear();
    this->exits.clear();
    Display::DestroyOverlay(this->marker.texture);
    Display::DestroyOverlay(this->title.texture);
    Display::DestroyOverlay(this->hint.texture);
    Display::DestroyOverlay(this->player.texture);
    this->player = Image{};
}

double MapOverview::FitZoom() const
{
    const Point output = this->game.display.OutputSize();

    if (this->mapSize.x <= 0 || this->mapSize.y <= 0)
    {
        return 1.0;
    }

    const Rows rows = this->MapRows();
    return kFitShare *
           std::min(static_cast<double>(output.x) / this->mapSize.x, (rows.bottom - rows.top) / this->mapSize.y);
}

MapOverview::Rows MapOverview::MapRows() const
{
    const Point output = this->game.display.OutputSize();
    const double scale = this->ScreenScale();
    const double text = kTextShare * scale;
    const double margins = 2.0 * kTextMargin * scale;
    const double top = std::max(this->title.size.y, this->hint.size.y) * text + margins;
    return top < output.y ? Rows{top, static_cast<double>(output.y)} : Rows{0.0, static_cast<double>(output.y)};
}

double MapOverview::MostZoom() const
{
    return std::max(kMostZoom * this->ScreenScale(), this->FitZoom());
}

double MapOverview::ScreenScale() const
{
    const Point output = this->game.display.OutputSize();
    return std::max(1.0, static_cast<double>(output.y) / Display::Height);
}

// The zoom wanted becomes `wanted` (within the least and the most), the map point at output (outputX, outputY) kept
// there while it moves.
void MapOverview::ZoomTo(double wanted, double outputX, double outputY)
{
    const Point output = this->game.display.OutputSize();
    this->wantedZoom = std::clamp(wanted, std::min(this->FitZoom(), this->MostZoom()), this->MostZoom());
    this->anchorX = outputX;
    this->anchorY = outputY;
    this->anchorMapX = this->centerX + (outputX - output.x / 2.0) / this->zoom;
    const Rows rows = this->MapRows();
    this->anchorMapY = this->centerY + (outputY - (rows.top + rows.bottom) / 2.0) / this->zoom;
    this->anchored = true;
}

// The zoom moved towards the one wanted and the map by the arrow keys held, over `seconds`.
void MapOverview::Step(double seconds)
{
    const Point output = this->game.display.OutputSize();

    if (this->zoom != this->wantedZoom)
    {
        const double share = 1.0 - std::exp(-seconds / kZoomSeconds);
        const double logZoom = std::log(this->zoom) + (std::log(this->wantedZoom) - std::log(this->zoom)) * share;
        this->zoom = std::abs(logZoom - std::log(this->wantedZoom)) < 1e-3 ? this->wantedZoom : std::exp(logZoom);

        if (this->anchored)
        {
            this->centerX = this->anchorMapX - (this->anchorX - output.x / 2.0) / this->zoom;
            const Rows rows = this->MapRows();
            this->centerY = this->anchorMapY - (this->anchorY - (rows.top + rows.bottom) / 2.0) / this->zoom;
        }

        if (this->zoom == this->wantedZoom)
        {
            this->anchored = false;
        }
    }

    const bool* keys = SDL_GetKeyboardState(nullptr);
    const int zoomKeys =
        (keys[SDL_SCANCODE_EQUALS] || keys[SDL_SCANCODE_KP_PLUS] || keys[SDL_SCANCODE_PAGEUP] ? 1 : 0) -
        (keys[SDL_SCANCODE_MINUS] || keys[SDL_SCANCODE_KP_MINUS] || keys[SDL_SCANCODE_PAGEDOWN] ? 1 : 0);
    this->zoomHeld = zoomKeys != 0 ? this->zoomHeld + seconds : 0.0;

    if (zoomKeys != 0 && this->zoomHeld > kRepeatDelay)
    {
        const Rows rows = this->MapRows();
        this->ZoomTo(
            this->wantedZoom * std::pow(kHeldZoom, zoomKeys * seconds),
            output.x / 2.0,
            (rows.top + rows.bottom) / 2.0
        );
    }

    double acrossKeys = 0.0;
    double downKeys = 0.0;
    acrossKeys += keys[SDL_SCANCODE_RIGHT] || keys[SDL_SCANCODE_D] ? 1.0 : 0.0;
    acrossKeys -= keys[SDL_SCANCODE_LEFT] || keys[SDL_SCANCODE_A] ? 1.0 : 0.0;
    downKeys += keys[SDL_SCANCODE_DOWN] || keys[SDL_SCANCODE_S] ? 1.0 : 0.0;
    downKeys -= keys[SDL_SCANCODE_UP] || keys[SDL_SCANCODE_W] ? 1.0 : 0.0;

    if (acrossKeys != 0.0 || downKeys != 0.0)
    {
        this->anchored = false;
        this->centerX += acrossKeys * kPanSpeed * seconds / this->zoom;
        this->centerY += downKeys * kPanSpeed * seconds / this->zoom;
    }

    this->Clamp();
}

// The map kept between the bands (MapRows): centred while it is smaller, its edges at theirs otherwise.
void MapOverview::Clamp()
{
    const Point output = this->game.display.OutputSize();
    const double halfWidth = output.x / 2.0 / this->zoom;
    const Rows rows = this->MapRows();
    const double halfHeight = (rows.bottom - rows.top) / 2.0 / this->zoom;

    this->centerX = 2.0 * halfWidth >= this->mapSize.x
                        ? this->mapSize.x / 2.0
                        : std::clamp(this->centerX, halfWidth, this->mapSize.x - halfWidth);
    this->centerY = 2.0 * halfHeight >= this->mapSize.y
                        ? this->mapSize.y / 2.0
                        : std::clamp(this->centerY, halfHeight, this->mapSize.y - halfHeight);
}

}  // namespace swd3
