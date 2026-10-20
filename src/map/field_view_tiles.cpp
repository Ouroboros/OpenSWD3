#include "map/field_view_internal.h"

namespace swd3
{

u32 FieldView::Block(int index) const
{
    const std::vector<u32>& blocks = this->game.state.map.blocks;

    if (index < 0 || static_cast<std::size_t>(index) >= blocks.size())
    {
        return MapHeader::HiddenTile;
    }

    return blocks[static_cast<std::size_t>(index)];
}

u16 FieldView::Tile(int index) const
{
    const std::vector<u16>& tiles = this->game.state.map.tiles;

    if (index < 0 || static_cast<std::size_t>(index) >= tiles.size())
    {
        return 0;
    }

    return tiles[static_cast<std::size_t>(index)];
}

// The source of tile `index` of the grid in the cached unit (`wideRead`: drawn as 16-bit pixels, 0x200 bytes, as the
// original draws the edges of 8-bit maps); the original reads past the view for codes beyond it,
// here such a tile shows the start of the view.
void FieldView::SetTileSource(const TileSet& tiles, int index, bool wideRead)
{
    const std::span<const u8> view = this->game.mapCache.unitFile.View();
    const std::size_t tile = this->Tile(index);
    const std::size_t offset =
        tiles.indexed ? MapHeader::IndexedTileBytes * tile + MapHeader::PaletteBytes : MapHeader::TileBytes * tile;
    const std::size_t bytes = tiles.indexed && !wideRead ? MapHeader::IndexedTileBytes : MapHeader::TileBytes;
    this->game.painter.SetSource(offset + bytes <= view.size() ? view.data() + offset : view.data());
}

void FieldView::DrawTileGrid(const TileSet& tiles)
{
    Painter& painter = this->game.painter;
    const GameState& state = this->game.state;
    const Scene& view = this->game.scene;
    int left = 0;
    int firstColumn = 0;
    int right = Display::Width;
    int top = 0;
    int bottom = Display::Height;

    if (!state.GameFlag(GameState::PlainScreen) && state.GameFlag(GameState::Spotlight))
    {
        const int lightX = (view.spotlightX + MapHeader::TileFraction) & ~MapHeader::TileFraction;
        const int lightY = (view.spotlightY + MapHeader::TileFraction) & ~MapHeader::TileFraction;
        firstColumn = lightX - Scene::SpotlightRadius;
        right = lightX + Scene::SpotlightRadius;
        left = firstColumn;
        top = lightY - Scene::SpotlightRadius;
        bottom = lightY + Scene::SpotlightRadius;
    }

    const int width = state.map.width;
    int skip = width - kScreenColumns;

    if (left != 0)
    {
        skip += (left - right + Display::Width) >> MapHeader::TileShift;
    }

    const int first = width * ((top + this->game.field.cameraTop) >> MapHeader::TileShift) +
                      ((left + this->game.field.cameraLeft) >> MapHeader::TileShift);
    int tile = first + this->game.scene.unk4B873C;
    int block = first;

    for (int y = top; y < bottom; y += MapHeader::TileSize)
    {
        for (int x = firstColumn; x < right; x += MapHeader::TileSize)
        {
            const u32 cell = this->Block(block);

            if ((cell & MapHeader::HiddenTile) == 0)
            {
                this->SetTileSource(tiles, tile);
                const bool keyed = (cell & MapHeader::KeyedTile) != 0;

                if (tiles.indexed)
                {
                    keyed ? painter.DrawTile8Keyed(x, y) : painter.DrawTile8(x, y);
                }
                else
                {
                    keyed ? painter.DrawTile16Keyed(x, y) : painter.DrawTile16(x, y);
                }
            }

            ++block;
            ++tile;
        }

        tile += skip;
        block += skip;
    }
}

void FieldView::DrawTileGridScrolled(const TileSet& tiles)
{
    Painter& painter = this->game.painter;
    const GameState& state = this->game.state;
    const Field& field = this->game.field;
    const int fractionX = field.cameraLeft & MapHeader::TileFraction;
    const int fractionY = field.cameraTop & MapHeader::TileFraction;
    int row = field.cameraTop >> MapHeader::TileShift;
    int column = field.cameraLeft >> MapHeader::TileShift;
    int columns = kScreenColumns;
    int rows = kScreenRows;
    this->tileX = -fractionX;
    this->tileY = -fractionY;

    if (row < 0)
    {
        row = 0;
        rows = kScreenRows - 1;
        this->tileY = MapHeader::TileSize - fractionY;
    }

    if (column < 0)
    {
        column = 0;
        columns = kScreenColumns - 1;
        this->tileX = MapHeader::TileSize - fractionX;
    }

    const int width = state.map.width;

    if (fractionX != 0)
    {
        ++columns;

        if (column + columns > width)
        {
            columns = width - column;
        }
    }

    if (fractionY != 0)
    {
        ++rows;

        if (row + rows > state.map.height)
        {
            rows = state.map.height - row;
        }
    }

    const int lightX = (this->game.scene.spotlightX + MapHeader::TileFraction) & ~MapHeader::TileFraction;
    const int lightY = (this->game.scene.spotlightY + MapHeader::TileFraction) & ~MapHeader::TileFraction;
    int left = lightX - Scene::SpotlightRadius;
    int right = lightX + Scene::SpotlightRadius;
    int top = lightY - Scene::SpotlightRadius;
    int bottom = lightY + Scene::SpotlightRadius;
    const int layer = this->game.scene.unk4B873C;

    auto drawEdge = [this, &painter, &tiles](int block, int tile, int x, int y)
    {
        const u32 cell = this->Block(block);

        if ((cell & MapHeader::HiddenTile) == 0)
        {
            this->SetTileSource(tiles, tile, true);
            painter.DrawPicture(
                x,
                y,
                MapHeader::TileSize,
                MapHeader::TileSize,
                (cell & MapHeader::KeyedTile) != 0 ? Painter::RawKeyed : Painter::RawCopy
            );
        }
    };

    if (state.GameFlag(GameState::PlainScreen) || !state.GameFlag(GameState::Spotlight))
    {
        top = 0;
        right = Display::Width;
        bottom = Display::Height;
        const int rowStart = row * width;
        const int lastRow = width * (row + rows - 1);
        const int end = this->tileX + MapHeader::TileSize * columns;

        for (int x = this->tileX, at = 0; x < end; x += MapHeader::TileSize, ++at)
        {
            drawEdge(column + rowStart + at, column + rowStart + layer + at, x, this->tileY);
            drawEdge(
                column + lastRow + at,
                column + lastRow + layer + at,
                x,
                MapHeader::TileSize * rows - MapHeader::TileSize + this->tileY
            );
        }

        for (int y = 0, at = 0; y < MapHeader::TileSize * rows; y += MapHeader::TileSize, at += width)
        {
            const int leftBlock = rowStart + column + at;
            drawEdge(leftBlock, leftBlock + layer, this->tileX, y + this->tileY);
            drawEdge(
                leftBlock + columns - 1,
                leftBlock + columns - 1 + layer,
                this->tileX + MapHeader::TileSize * columns - MapHeader::TileSize,
                y + this->tileY
            );
        }

        left = 0;
    }

    const int base = (left >> MapHeader::TileShift) + width * ((top >> MapHeader::TileShift) + row + 1);
    int tile = column + base + layer + 1;
    int block = column + base + 1;
    this->tileY += MapHeader::TileSize;
    this->tileX += MapHeader::TileSize;
    int y = top + this->tileY;
    const int firstX = this->tileX + left;

    if (y >= bottom - MapHeader::TileSize)
    {
        return;
    }

    for (;;)
    {
        int rowTile = tile;
        int rowBlock = block;

        for (int x = firstX; x < right - MapHeader::TileSize; x += MapHeader::TileSize)
        {
            const u32 cell = this->Block(rowBlock);

            if ((cell & MapHeader::HiddenTile) == 0)
            {
                this->SetTileSource(tiles, rowTile);
                const bool keyed = (cell & MapHeader::KeyedTile) != 0;

                if (tiles.indexed)
                {
                    keyed ? painter.DrawTile8Keyed(x, y) : painter.DrawTile8(x, y);
                }
                else
                {
                    keyed ? painter.DrawTile16Keyed(x, y) : painter.DrawTile16(x, y);
                }
            }

            ++rowTile;
            ++rowBlock;
        }

        y += MapHeader::TileSize;
        tile += width;
        block += width;

        if (y >= bottom - MapHeader::TileSize)
        {
            break;
        }
    }
}

// The rebuild's map filling the window (deviations.md): the tiles under the camera on a canvas of any size and a
// camera anywhere (off the tile grid, past the map's edges, where nothing is drawn); with the spotlight only those
// around it. A tile wholly inside is drawn as the grid draws it, one cut by an edge clipped as the scrolled grid draws
// its edge tiles.
void FieldView::DrawTileCanvas(const TileSet& tiles)
{
    Painter& painter = this->game.painter;
    const GameState& state = this->game.state;
    const Field& field = this->game.field;
    const Scene& view = this->game.scene;
    int left = 0;
    int top = 0;
    int right = field.viewWidth;
    int bottom = field.viewHeight;
    const auto [clipX, clipY, clipWidth, clipHeight] = painter.ClipRect();
    const bool spotlight = !state.GameFlag(GameState::PlainScreen) && state.GameFlag(GameState::Spotlight);

    if (spotlight)
    {
        const int lightX = view.spotlightX + field.screenX;
        const int lightY = view.spotlightY + field.screenY;
        left = std::max(lightX - Scene::SpotlightRadius, 0);
        top = std::max(lightY - Scene::SpotlightRadius, 0);
        right = std::min(lightX + Scene::SpotlightRadius, right);
        bottom = std::min(lightY + Scene::SpotlightRadius, bottom);

        if (left >= right || top >= bottom)
        {
            return;
        }

        painter.SetClip(left, top, right, bottom);
    }

    const int width = state.map.width;
    const int firstColumn = std::max(FloorDivide(field.cameraLeft + left, MapHeader::TileSize), 0);
    const int lastColumn = std::min(FloorDivide(field.cameraLeft + right - 1, MapHeader::TileSize), width - 1);
    const int firstRow = std::max(FloorDivide(field.cameraTop + top, MapHeader::TileSize), 0);
    const int lastRow = std::min(FloorDivide(field.cameraTop + bottom - 1, MapHeader::TileSize), state.map.height - 1);
    const int layer = view.unk4B873C;

    for (int row = firstRow; row <= lastRow; ++row)
    {
        const int y = MapHeader::TileSize * row - field.cameraTop;

        for (int column = firstColumn; column <= lastColumn; ++column)
        {
            const int x = MapHeader::TileSize * column - field.cameraLeft;
            const int block = width * row + column;
            const u32 cell = this->Block(block);

            if ((cell & MapHeader::HiddenTile) != 0)
            {
                continue;
            }

            const bool keyed = (cell & MapHeader::KeyedTile) != 0;

            if (x >= left && y >= top && x + MapHeader::TileSize <= right && y + MapHeader::TileSize <= bottom)
            {
                this->SetTileSource(tiles, block + layer);

                if (tiles.indexed)
                {
                    keyed ? painter.DrawTile8Keyed(x, y) : painter.DrawTile8(x, y);
                }
                else
                {
                    keyed ? painter.DrawTile16Keyed(x, y) : painter.DrawTile16(x, y);
                }

                continue;
            }

            this->SetTileSource(tiles, block + layer, true);
            painter.DrawPicture(
                x,
                y,
                MapHeader::TileSize,
                MapHeader::TileSize,
                keyed ? Painter::RawKeyed : Painter::RawCopy
            );
        }
    }

    if (spotlight)
    {
        painter.SetClip(clipX, clipY, clipX + clipWidth, clipY + clipHeight);
    }
}

// swd32.exe 0x00412BE0: the 16-bit tiles of the map at a camera on the tile grid (with the spotlight only the 24 x 24
// tiles around it, the rows then advancing as if 40 columns were drawn when its left edge is at 0)
void FieldView::DrawTiles16(int)
{
    if (this->FillsWindow())
    {
        this->DrawTileCanvas(TileSet{false});
        return;
    }

    this->DrawTileGrid(TileSet{false});
}

// swd32.exe 0x00412D30: the 16-bit tiles at a camera off the grid: the edge tiles clipped, the inner ones whole
void FieldView::DrawTiles16Scrolled(int)
{
    const Field& field = this->game.field;

    if (this->FillsWindow())
    {
        this->DrawTileCanvas(TileSet{false});
        return;
    }

    if ((field.cameraLeft & MapHeader::TileFraction) == 0 && (field.cameraTop & MapHeader::TileFraction) == 0)
    {
        this->DrawTiles16(0);
        return;
    }

    this->DrawTileGridScrolled(TileSet{false});
}

// swd32.exe 0x00413220: 0x00412BE0 for the 8-bit tiles of the map's palette
void FieldView::DrawTiles8(int)
{
    if (this->FillsWindow())
    {
        this->DrawTileCanvas(TileSet{true});
        return;
    }

    this->DrawTileGrid(TileSet{true});
}

// swd32.exe 0x00413370: 0x00412D30 for the 8-bit tiles; on the grid it falls back to the 16-bit 0x00412BE0
void FieldView::DrawTiles8Scrolled(int)
{
    const Field& field = this->game.field;

    if (this->FillsWindow())
    {
        this->DrawTileCanvas(TileSet{true});
        return;
    }

    if ((field.cameraLeft & MapHeader::TileFraction) == 0 && (field.cameraTop & MapHeader::TileFraction) == 0)
    {
        this->DrawTiles16(0);
        return;
    }

    this->DrawTileGridScrolled(TileSet{true});
}

// swd32.exe 0x004151F0: for each drawing order 0..30 the first overlay of that order over the camera, clipped to the
// part of its area on screen and placed by its parallax (8: fixed to the map)
void FieldView::DrawOverlays()
{
    Game& owner = this->game;
    Painter& painter = owner.painter;
    const Field& field = owner.field;

    for (int order = 0; order < MapOverlay::Orders; ++order)
    {
        for (const MapOverlay* overlay = field.overlays.get(); overlay != nullptr; overlay = overlay->next.get())
        {
            if (overlay->kind != order)
            {
                continue;
            }

            const int areaLeft = overlay->left;
            const int areaTop = overlay->top;
            const int areaRight = overlay->right + MapHeader::TileSize;
            const int areaBottom = overlay->bottom + MapHeader::TileSize;
            const int left = std::max(field.cameraLeft, areaLeft);
            const int top = std::max(field.cameraTop, areaTop);
            const int right = std::min(field.cameraRight, areaRight);
            const int bottom = std::min(field.cameraBottom, areaBottom);

            if (left >= right || top >= bottom)
            {
                continue;
            }

            const int clipLeft = left - field.cameraLeft;
            const int clipTop = top - field.cameraTop;
            painter.SetClip(clipLeft, clipTop, right - field.cameraLeft, bottom - field.cameraTop);
            int offsetX = 0;

            if (overlay->parallaxX == MapOverlay::FixedToMap)
            {
                offsetX = field.cameraLeft >= areaLeft ? field.cameraLeft - areaLeft : 0;
            }
            else
            {
                offsetX = overlay->parallaxX * (field.cameraRight - areaLeft) / MapOverlay::ParallaxUnit;

                if (field.cameraLeft < areaLeft)
                {
                    offsetX += clipLeft;
                }
            }

            int offsetY = 0;

            if (overlay->parallaxY == MapOverlay::FixedToMap)
            {
                offsetY = field.cameraTop >= areaTop ? field.cameraTop - areaTop : 0;
            }
            else
            {
                offsetY = overlay->parallaxY * (field.cameraBottom - areaTop) / MapOverlay::ParallaxUnit;

                if (field.cameraTop < areaTop)
                {
                    offsetY += clipTop;
                }
            }

            painter.SetSource(overlay->picture.data());
            painter.DrawPicture(clipLeft - offsetX, clipTop - offsetY, overlay->width, overlay->height, Painter::Copy);
            painter.SetClipClamped(0, 0, field.viewWidth, field.viewHeight);
            break;
        }
    }
}

}  // namespace swd3
