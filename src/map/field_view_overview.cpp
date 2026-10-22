#include "map/field_view_internal.h"

namespace swd3
{

// The map is drawn a canvas at a time, the camera on each part in turn, and the parts laid side by side; the spotlight
// and a black screen are set aside meanwhile. What the parts change is put back after each (PaintKept), the camera
// and the flags at the end.
void FieldView::PaintOverview(Surface& out)
{
    Game& owner = this->game;
    GameState& state = owner.state;
    Field& field = owner.field;
    Scene& view = owner.scene;
    Painter& painter = owner.painter;
    const int mapWidth = MapHeader::TileSize * state.map.width;
    const int mapHeight = MapHeader::TileSize * state.map.height;
    const int width = field.viewWidth;
    const int height = field.viewHeight;

    if (mapWidth <= 0 || mapHeight <= 0 || width <= 0 || height <= 0)
    {
        out.Resize(1, 1);
        return;
    }

    out.Resize(mapWidth, mapHeight);
    const CameraRect camera{field.cameraLeft, field.cameraTop, field.cameraRight, field.cameraBottom};
    const bool spotlight = state.GameFlag(GameState::Spotlight);
    const i32 screenFlags = view.screenFlags;
    owner.SetDrawSize(width, height);
    const PaintKept kept(*this);
    Surface part(width, height);
    state.ClearGameFlag(GameState::Spotlight);
    view.screenFlags &= ~Scene::BlackScreen;
    this->replaying = true;
    this->overview = true;

    for (int top = 0; top < mapHeight; top += height)
    {
        for (int left = 0; left < mapWidth; left += width)
        {
            field.cameraLeft = left;
            field.cameraTop = top;
            field.cameraRight = left + width;
            field.cameraBottom = top + height;
            owner.drawSurface = painter.LockSurface(part);
            painter.SetTarget(owner.drawSurface);
            painter.UnlockSurface(part);
            painter.SetClipClamped(0, 0, width, height);
            this->PaintOverviewPart();
            const int columns = std::min(width, mapWidth - left);

            for (int row = 0; row < height && top + row < mapHeight; ++row)
            {
                std::copy_n(part.Row(row), columns, out.Row(top + row) + left);
            }

            kept.PutBack();
        }
    }

    this->overview = false;
    this->replaying = false;
    this->paintedValid = false;
    view.screenFlags = screenFlags;

    if (spotlight)
    {
        state.SetGameFlag(GameState::Spotlight);
    }

    field.cameraLeft = camera.left;
    field.cameraTop = camera.top;
    field.cameraRight = camera.right;
    field.cameraBottom = camera.bottom;
    owner.SetDrawSize(Display::Width, Display::Height);
}

// A part of the overview: PaintWorld's overlays, tiles and roles (DrawRoles keeps to the map file's), on black.
void FieldView::PaintOverviewPart()
{
    Game& owner = this->game;
    const GameState& state = owner.state;
    Painter& painter = owner.painter;
    const Field& field = owner.field;
    const std::span<Pixel> canvas(
        owner.drawSurface,
        static_cast<std::size_t>(field.viewWidth) * static_cast<std::size_t>(field.viewHeight)
    );
    std::fill(canvas.begin(), canvas.end(), Pixel{0});
    this->DrawOverlays();
    const bool scrolled =
        (field.cameraLeft & MapHeader::TileFraction) != 0 || (field.cameraTop & MapHeader::TileFraction) != 0;

    if (state.map.bitsPerPixel == MapHeader::TrueColorBits)
    {
        scrolled ? this->DrawTiles16Scrolled(0) : this->DrawTiles16(0);
    }
    else
    {
        painter.SetPalette(field.palette.data());
        scrolled ? this->DrawTiles8Scrolled(0) : this->DrawTiles8(0);
    }

    painter.SetPalette(nullptr);
    this->DrawRoles();
}

// The regions are found by their cells (a cell's low byte is its region); each whose event changes the map to
// another map is an exit, at the middle of its cells.
std::vector<FieldView::MapExit> FieldView::MapExits()
{
    struct Cells
    {
        i64 x = 0;
        i64 y = 0;
        i64 count = 0;
    };

    Game& owner = this->game;
    const GameState& state = owner.state;
    std::array<Cells, MapHeader::RegionMask + 1> regions = {};

    for (int row = 0; row < state.map.height; ++row)
    {
        for (int column = 0; column < state.map.width; ++column)
        {
            const std::size_t at = static_cast<std::size_t>(row) * static_cast<std::size_t>(state.map.width) +
                                   static_cast<std::size_t>(column);

            if (at >= state.map.blocks.size())
            {
                continue;
            }

            Cells& cells = regions[state.map.blocks[at] & MapHeader::RegionMask];
            cells.x += MapHeader::TileSize * column + MapHeader::TileSize / 2;
            cells.y += MapHeader::TileSize * row + MapHeader::TileSize / 2;
            ++cells.count;
        }
    }

    std::vector<MapExit> exits;

    for (const MapRegion* region = owner.field.regions.next.get(); region != nullptr; region = region->next.get())
    {
        const i32 event = region->event & MapRegion::EventMask;

        if (region->id <= 0 || region->id > static_cast<i32>(MapHeader::RegionMask) || event == 0)
        {
            continue;
        }

        const Cells& cells = regions[static_cast<std::size_t>(region->id)];

        if (cells.count == 0)
        {
            continue;
        }

        MapExit exit;

        for (const u16 map : this->eventScan.EnteredMaps(owner.config.installDirectory, static_cast<u32>(event)))
        {
            if (map != owner.currentMap)
            {
                exit.maps.push_back(map);
            }
        }

        if (exit.maps.empty())
        {
            continue;
        }

        exit.at = {static_cast<i32>(cells.x / cells.count), static_cast<i32>(cells.y / cells.count)};
        exits.push_back(std::move(exit));
    }

    return exits;
}

}  // namespace swd3
