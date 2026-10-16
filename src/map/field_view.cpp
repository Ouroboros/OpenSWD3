#include "map/field_view.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <span>

#include "crt/msvc.h"
#include "game/game.h"

namespace swd3
{

namespace
{

constexpr int kScreenWidth = 0x280;
constexpr int kScreenHeight = 0x1E0;
constexpr u32 kHiddenTile = 0x08000000;
constexpr u32 kKeyedTile = 0x04000000;
constexpr u32 kTranslucentFlip = 0x16;
constexpr u32 kTinted = 0x10;
constexpr u32 kShadow = 0x0C;
constexpr int kEffectAct = 0x232B;
constexpr int kTimerAct = 0x232C;

// swd32.exe 0x004995D4: the tint of a role's shadow by bits 20..23 of its flags.
constexpr std::array<i32, 16> kShadowTint = {-8, -7, -6, -5, -4, -3, -2, 0, 0, 0, -14, -13, -12, -11, -10, -9};
// swd32.exe 0x00499594: the tint added to a role by bits 20..23 of its flags.
constexpr std::array<i32, 16> kRoleTint = {0, 2, 4, 6, 8, 10, 12, 0, 0, 0, -12, -10, -8, -6, -4, -2};
// The action of a leaf by its vertical speed plus 3 (a table on the stack of 0x004161C0).
constexpr std::array<char, 8> kLeafActions = {'4', '4', '4', '6', '8', '8', '8', '6'};
// The sideways sway of a firefly by its phase / 4 (a table on the stack of 0x004167B0).
constexpr std::array<i8, 8> kFireflySway = {0, 1, 1, 1, 0, -1, -1, -1};

// The sixth argument the original passes with a picture: the word at +4 of the picture record (its palette).
DrawArgument PictureArgument(const Picture& picture)
{
    DrawArgument argument;
    argument.alphaMap = reinterpret_cast<const u8*>(picture.palette.data());
    return argument;
}

}  // namespace

// The map's tile source: 16-bit tiles of 0x200 bytes, or 8-bit tiles of 0x100 bytes after the 0x200-byte palette.
struct FieldView::TileSet
{
    bool indexed = false;
};

FieldView::FieldView(Game& game) : game(game)
{
}

u32 FieldView::Block(int index) const
{
    const std::vector<u32>& blocks = this->game.state.map.blocks;

    if (index < 0 || static_cast<std::size_t>(index) >= blocks.size())
    {
        return kHiddenTile;
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

// The source of tile `index` of the grid in the cached unit; the original reads past the view for codes beyond it,
// here such a tile shows the start of the view.
void FieldView::SetTileSource(const TileSet& tiles, int index)
{
    const std::span<const u8> view = this->game.mapCache.unitFile.View();
    const std::size_t tile = this->Tile(index);
    const std::size_t offset = tiles.indexed ? 0x100 * tile + 0x200 : 0x200 * tile;
    const std::size_t bytes = tiles.indexed ? 0x100 : 0x200;
    this->game.painter.SetSource(offset + bytes <= view.size() ? view.data() + offset : view.data());
}

void FieldView::DrawTileGrid(const TileSet& tiles)
{
    Painter& painter = this->game.painter;
    const GameState& state = this->game.state;
    const Scene& view = this->game.scene;
    int left = 0;
    int firstColumn = 0;
    int right = kScreenWidth;
    int top = 0;
    int bottom = kScreenHeight;

    if (!state.GameFlag(0x48) && state.GameFlag(0x13))
    {
        const int lightX = (view.unk4B9EF8 + 0xF) & ~0xF;
        const int lightY = (view.unk4B9EFC + 0xF) & ~0xF;
        firstColumn = lightX - 0xC0;
        right = lightX + 0xC0;
        left = firstColumn;
        top = lightY - 0xC0;
        bottom = lightY + 0xC0;
    }

    const int width = state.map.width;
    int skip = width - 0x28;

    if (left != 0)
    {
        skip += (left - right + kScreenWidth) >> 4;
    }

    const int first = width * ((top + this->game.field.cameraTop) >> 4) + ((left + this->game.field.cameraLeft) >> 4);
    int tile = first + this->game.scene.unk4B873C;
    int block = first;

    for (int y = top; y < bottom; y += 0x10)
    {
        for (int x = firstColumn; x < right; x += 0x10)
        {
            const u32 cell = this->Block(block);

            if ((cell & kHiddenTile) == 0)
            {
                this->SetTileSource(tiles, tile);
                const bool keyed = (cell & kKeyedTile) != 0;

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
    const int fractionX = field.cameraLeft & 0xF;
    const int fractionY = field.cameraTop & 0xF;
    int row = field.cameraTop >> 4;
    int column = field.cameraLeft >> 4;
    int columns = 0x28;
    int rows = 0x1E;
    this->tileX = -fractionX;
    this->tileY = -fractionY;

    if (row < 0)
    {
        row = 0;
        rows = 0x1D;
        this->tileY = 0x10 - fractionY;
    }

    if (column < 0)
    {
        column = 0;
        columns = 0x27;
        this->tileX = 0x10 - fractionX;
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

    const int lightX = (this->game.scene.unk4B9EF8 + 0xF) & ~0xF;
    const int lightY = (this->game.scene.unk4B9EFC + 0xF) & ~0xF;
    int left = lightX - 0xC0;
    int right = lightX + 0xC0;
    int top = lightY - 0xC0;
    int bottom = lightY + 0xC0;
    const int layer = this->game.scene.unk4B873C;

    auto drawEdge = [this, &painter, &tiles](int block, int tile, int x, int y)
    {
        const u32 cell = this->Block(block);

        if ((cell & kHiddenTile) == 0)
        {
            this->SetTileSource(tiles, tile);
            painter.DrawPicture(x, y, 0x10, 0x10, (cell & kKeyedTile) != 0 ? 4 : 0);
        }
    };

    if (state.GameFlag(0x48) || !state.GameFlag(0x13))
    {
        top = 0;
        right = kScreenWidth;
        bottom = kScreenHeight;
        const int rowStart = row * width;
        const int lastRow = width * (row + rows - 1);
        const int end = this->tileX + 0x10 * columns;

        for (int x = this->tileX, at = 0; x < end; x += 0x10, ++at)
        {
            drawEdge(column + rowStart + at, column + rowStart + layer + at, x, this->tileY);
            drawEdge(column + lastRow + at, column + lastRow + layer + at, x, 0x10 * rows - 0x10 + this->tileY);
        }

        for (int y = 0, at = 0; y < 0x10 * rows; y += 0x10, at += width)
        {
            const int leftBlock = rowStart + column + at;
            drawEdge(leftBlock, leftBlock + layer, this->tileX, y + this->tileY);
            drawEdge(
                leftBlock + columns - 1,
                leftBlock + columns - 1 + layer,
                this->tileX + 0x10 * columns - 0x10,
                y + this->tileY
            );
        }

        left = 0;
    }

    const int base = (left >> 4) + width * ((top >> 4) + row + 1);
    int tile = column + base + layer + 1;
    int block = column + base + 1;
    this->tileY += 0x10;
    this->tileX += 0x10;
    int y = top + this->tileY;
    const int firstX = this->tileX + left;

    if (y >= bottom - 0x10)
    {
        return;
    }

    for (;;)
    {
        int rowTile = tile;
        int rowBlock = block;

        for (int x = firstX; x < right - 0x10; x += 0x10)
        {
            const u32 cell = this->Block(rowBlock);

            if ((cell & kHiddenTile) == 0)
            {
                this->SetTileSource(tiles, rowTile);
                const bool keyed = (cell & kKeyedTile) != 0;

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

        y += 0x10;
        tile += width;
        block += width;

        if (y >= bottom - 0x10)
        {
            break;
        }
    }
}

// swd32.exe 0x00412BE0: the 16-bit tiles of the map at a camera on the tile grid (with the spotlight only the 24 x 24
// tiles around it, the rows then advancing as if 40 columns were drawn when its left edge is at 0)
void FieldView::DrawTiles16(int)
{
    this->DrawTileGrid(TileSet{false});
}

// swd32.exe 0x00412D30: the 16-bit tiles at a camera off the grid: the edge tiles clipped, the inner ones whole
void FieldView::DrawTiles16Scrolled(int)
{
    const Field& field = this->game.field;

    if ((field.cameraLeft & 0xF) == 0 && (field.cameraTop & 0xF) == 0)
    {
        this->DrawTiles16(0);
        return;
    }

    this->DrawTileGridScrolled(TileSet{false});
}

// swd32.exe 0x00413220: 0x00412BE0 for the 8-bit tiles of the map's palette
void FieldView::DrawTiles8(int)
{
    this->DrawTileGrid(TileSet{true});
}

// swd32.exe 0x00413370: 0x00412D30 for the 8-bit tiles; on the grid it falls back to the 16-bit 0x00412BE0
void FieldView::DrawTiles8Scrolled(int)
{
    const Field& field = this->game.field;

    if ((field.cameraLeft & 0xF) == 0 && (field.cameraTop & 0xF) == 0)
    {
        this->DrawTiles16(0);
        return;
    }

    this->DrawTileGridScrolled(TileSet{true});
}

// swd32.exe 0x00413870: the roles of the three layers (2, 0, then 1), 70 rows from 20 above the camera, with their
// sounds
void FieldView::DrawRoles()
{
    constexpr std::array<int, 3> kLayerOrder = {2, 0, 1};
    const GameState& state = this->game.state;

    for (const int layer : kLayerOrder)
    {
        const std::vector<Role*>& rows = this->game.field.rows[static_cast<std::size_t>(layer)];
        auto row = static_cast<u32>(this->game.field.cameraTop / 0x10 - Field::kRowMargin);

        for (int count = 0; count < 0x46; ++count, ++row)
        {
            const std::size_t index = row + Field::kRowMargin;

            if (row >= static_cast<u32>(state.map.height + Field::kRowMargin) || index >= rows.size())
            {
                continue;
            }

            for (Role* role = rows[index]; role != nullptr; role = role->next)
            {
                this->DrawRole(*role);

                if (role->sound != 0)
                {
                    this->UpdateRoleSound(*role);
                }
            }
        }
    }
}

// swd32.exe 0x00413910: a visible role (flag 0x8000 without 0x400) near the camera: its sound cue, its shadow, its
// picture with its wave and translucency, a tinted copy (the map's tint and the tint of bits 20..23), the act attached
// to it, its bubbles and, while no event runs, its label. Act roles are skipped while story flag 0xB is set.
int FieldView::DrawRole(Role& role)
{
    if ((role.flags & 0x8400) != 0x8000)
    {
        return 0;
    }

    Painter& painter = this->game.painter;
    const Field& field = this->game.field;
    const Scene& view = this->game.scene;
    const int x = role.x;
    const int y = role.y;
    const int cameraLeft = field.cameraLeft;
    const int cameraTop = field.cameraTop;

    if (x - cameraLeft < -0x140 || x - cameraLeft > 0x3C0)
    {
        return 1;
    }

    u32 frame = 0xFFFF;

    if (role.act.id != 0)
    {
        frame = role.act.frame;

        if (this->game.state.GameFlag(0xB))
        {
            return 1;
        }
    }

    if (role.act.view != 0)
    {
        this->game.PlaySoundAt(role.act.view, x, y);
        role.act.view = 0;
    }

    const Picture* picture = this->game.archive.GetPicture(frame, role.act.picture);

    if (picture == nullptr)
    {
        return 1;
    }

    if ((role.flags & 0x100) != 0 && (view.unk4B7EB4 & 7) < 4)
    {
        this->DrawShadow(
            role,
            *picture,
            x - role.act.x - cameraLeft,
            y - role.act.y - cameraTop,
            view.unk4B7EB4,
            kShadowTint[(role.flags >> 20) & 0xF]
        );
    }

    const u32 mode = role.act.flags;
    painter.SetWave(role.act.wait);
    painter.SetWavePhase(role.act.unknown89);
    DrawOptions& options = painter.Options();
    options.unusedFlag = 0;
    options.alpha = role.act.signal;
    painter.SetSource(picture->data.data());
    const int drawX = x + role.unk28 - role.act.x - cameraLeft;
    const int drawY = y + role.unk2A - role.act.y - cameraTop;
    painter.DrawPicture(drawX, drawY, picture->width, picture->height, mode, PictureArgument(*picture));
    int red = 0;
    int green = 0;
    int blue = 0;

    if ((role.flags & 0x100) != 0)
    {
        red = view.unk4AB97C;
        green = view.unk4AB994;
        blue = view.unk4ACB98;
    }

    if ((role.flags & 0xF00000) != 0)
    {
        const int tint = kRoleTint[(role.flags >> 20) & 0xF];
        red += tint;
        green += tint;
        blue += tint;
    }

    options.tintRed = red;
    options.tintGreen = green;
    options.tintBlue = blue;

    if (red != 0 || green != 0 || blue != 0)
    {
        painter.SetSource(picture->data.data());
        painter.DrawPicture(
            drawX,
            drawY,
            picture->width,
            picture->height,
            (mode & 0x80000003) | kTinted,
            PictureArgument(*picture)
        );
    }

    if (role.attached != nullptr)
    {
        const ActState& attached = *role.attached;
        const Picture* extra = this->game.archive.GetPicture(attached.frame, attached.picture);

        if (extra != nullptr)
        {
            painter.SetSource(extra->data.data());
            painter.DrawPicture(
                x + role.unk28 - attached.x - cameraLeft,
                role.unk2A - attached.y - role.act.y - cameraTop + y + 0x1C,
                extra->width,
                extra->height,
                attached.flags
            );
        }
    }

    role.act.unknown89 = static_cast<u8>(painter.WavePhase());

    if ((role.flags & 0x200) != 0 && !this->game.state.GameFlag(0x48))
    {
        this->DrawBubbles(x, y, role.id);
    }

    if (this->game.state.leader.id == 0xFFFF && !role.label.empty())
    {
        const auto length = static_cast<int>(role.label.size());
        const auto color = static_cast<Pixel>(this->game.textColors.primary[static_cast<std::size_t>(role.labelColor)]);
        this->game.largeFont.DrawText(
            this->game.drawSurface,
            role.x - 0xB * length / 2 - cameraLeft + 0x10,
            role.y - role.act.y - cameraTop,
            role.label,
            color,
            4
        );
    }

    return 1;
}

// swd32.exe 0x00413CA0: the sound of a role near the controlled one (within 512 pixels while visible): it is played
// once its timer runs out (a period of 0xFFFF holds it, marked by flag 0x1000000) at a volume and pan by the distance;
// out of range a held sound is stopped
void FieldView::UpdateRoleSound(Role& role)
{
    GameState& state = this->game.state;
    Scene& view = this->game.scene;
    SoundEffects& effects = this->game.audio.Effects();
    const Role& player = state.roles[static_cast<std::size_t>(state.playerRole)];
    const int dx = role.x - player.x;
    const int squared = dx * dx + (role.y - player.y) * (role.y - player.y);
    const auto distance = static_cast<int>(crt::Ftol(std::sqrt(static_cast<double>(squared))));
    bool due = true;
    u32 timer = role.soundTimer & 0xFFFF0000;

    if (timer != 0xFFFF0000)
    {
        const u32 left = (role.soundTimer & 0xFFFF) - 1;

        if (left != 0)
        {
            timer += left;
            due = false;
            role.soundTimer = timer;
        }
    }

    if (distance > 0x200 || (role.flags & 0x8000) == 0)
    {
        if ((role.flags & 0x1000000) != 0)
        {
            effects.Stop(role.sound);
            role.flags &= ~0x1000000u;
        }

        return;
    }

    u32 index = 0;
    state.FindRole(role.id, index);

    if ((role.flags & 0x1000000) == 0 && due)
    {
        const u32 reloaded = (role.soundTimer >> 16) + (role.soundTimer & 0xFFFF0000);
        role.soundTimer = reloaded;

        if ((reloaded & 0xFFFF0000) == 0xFFFF0000)
        {
            role.flags |= 0x1000000;
        }

        effects.Play(role.sound, 0, 0, 1);

        if (index < view.soundDistance.size())
        {
            view.soundDistance[index] = static_cast<i16>((distance << 7) / 0x200);
            const auto rise =
                static_cast<i16>(static_cast<u16>(static_cast<u16>(role.y) - static_cast<u16>(player.y)) << 6);
            view.soundRise[index] = static_cast<i16>(rise / 0x200);
        }
    }

    effects.SetVolume(role.sound, this->game.config.soundVolume * (0x80 - (distance << 7) / 0x200) / 0xB);
    effects.SetPan(role.sound, ((role.x - player.x) << 6) / 0x200);
}

// swd32.exe 0x004145F0: the shadow of a role: its picture squashed to half its height (a quarter with bit 3 of
// `kind`), sheared to the side of bit 0 and darkened by `tint`; kinds from 2 on flip it over
void FieldView::DrawShadow(const Role& role, const Picture& picture, int x, int y, int kind, int tint)
{
    Painter& painter = this->game.painter;
    DrawOptions& options = painter.Options();
    u32 mode = (role.act.flags & 0x80000003) | kShadow;
    painter.SetSource(picture.data.data());
    const int half = picture.height >> 1;
    options.scaleHeight = half;
    options.shear = -(picture.height >> 2);

    if ((kind & 1) != 0)
    {
        options.shear = picture.height >> 2;
    }

    options.tintBlue = tint;
    options.tintGreen = tint;
    options.tintRed = tint;
    int extra = 0;
    int lift = 0;

    if ((kind & 8) != 0)
    {
        options.scaleHeight = half / 2;
        lift = role.act.y / 2;
        extra = 4;
    }

    int offsetY = 0;

    if ((kind & 7) < 2)
    {
        offsetY = (role.act.y + lift + 8) / 2 + extra;
    }
    else
    {
        mode |= 2;
        offsetY = extra + role.act.y + 4;
    }

    const int offsetX = (kind & 1) == 0 ? 4 : -4;
    painter.DrawPicture(x + offsetX, y + offsetY, picture.width, picture.height, mode);
}

// swd32.exe 0x00413EA0: the reflections of the roles of layer 0 standing on reflecting cells (flag 0x20000000), 40
// rows from 5 above the camera
void FieldView::DrawReflections()
{
    const std::vector<Role*>& rows = this->game.field.rows[0];
    int row = this->game.field.cameraTop / 0x10 - 5;

    for (int count = -10; count < 0x1E; ++count, ++row)
    {
        if (row >= this->game.state.map.height)
        {
            break;
        }

        const auto index = static_cast<std::size_t>(row + Field::kRowMargin);

        if (row < 0 || index >= rows.size())
        {
            continue;
        }

        for (const Role* role = rows[index]; role != nullptr; role = role->next)
        {
            if ((role->flags & 0x20000000) != 0)
            {
                this->DrawReflection(*role);
            }
        }
    }
}

// swd32.exe 0x00413F00: a role's picture flipped below it at translucency 4
int FieldView::DrawReflection(const Role& role)
{
    if ((role.flags & 0x8400) != 0x8000)
    {
        return 1;
    }

    const Field& field = this->game.field;
    const int x = role.x;

    if (x - field.cameraLeft <= -0x140 || x - field.cameraLeft >= 0x3C0)
    {
        return 1;
    }

    const u32 frame = role.act.id != 0 ? role.act.frame : 0xFFFF;
    const Picture* picture = this->game.archive.GetPicture(frame, role.act.picture);

    if (picture == nullptr)
    {
        return 1;
    }

    Painter& painter = this->game.painter;
    painter.Options().alpha = 4;
    painter.SetSource(picture->data.data());
    painter.DrawPicture(
        x + role.unk28 - role.act.x - field.cameraLeft,
        role.unk2A - field.cameraTop + role.y + 8,
        picture->width,
        picture->height,
        (role.act.flags & 0x80000001) | kTranslucentFlip
    );
    return 1;
}

// swd32.exe 0x00415EE0: the bubbles of role `role` near the camera: each mark of the role follows it, starts a new
// bubble now and then (at most while its count is below 0x18), and its bubbles rise, drift with the frame's sway and
// fade (warmer on map 0x32); spent ones are dropped by moving the next one into their place
void FieldView::DrawBubbles(int x, int y, u16 role)
{
    const Field& field = this->game.field;

    if (x <= field.cameraLeft - 0x40 || x >= field.cameraRight + 0x40 || y <= field.cameraTop - 0x40 ||
        y >= field.cameraBottom + 0x40)
    {
        return;
    }

    Game& owner = this->game;
    Random& random = owner.random;
    Scene& view = owner.scene;
    Painter& painter = owner.painter;
    const int sway = static_cast<int>(random.Below(5)) - 3;
    view.unk4ACAF8.id = kEffectAct;
    view.unk4ACAF8.action = 0x3B;
    owner.acts.Run(view.unk4ACAF8);
    const Picture* picture = owner.archive.GetPicture(view.unk4ACAF8.frame, view.unk4ACAF8.picture);
    const bool warm = owner.unk4A9A10 == 0x32;

    for (RoleMark& mark : view.marks)
    {
        if (mark.role != role)
        {
            continue;
        }

        u32 index = 0;
        owner.state.FindRoleOrPlayer(role, index);
        mark.flags &= ~RoleMark::kEmitting;
        const Role& source = owner.state.roles[std::min<std::size_t>(index, GameState::kRoles - 1)];
        mark.x = static_cast<u16>(source.x);
        mark.y = static_cast<u16>(source.y);

        if (random.Below(0x3E8) > 0xFA)
        {
            mark.flags |= RoleMark::kEmitting;
        }

        if (mark.unk08 < 0x18 && (mark.flags & RoleMark::kEmitting) != 0)
        {
            auto bubble = std::make_unique<RoleMarkNode>();
            bubble->next = std::move(mark.nodes);
            bubble->y = static_cast<i16>(mark.y);
            bubble->x = static_cast<i16>(0x10 * (mark.x - 8 + static_cast<int>(random.Below(0x10))));
            bubble->life = static_cast<i16>(warm ? random.Below(0x10) + 0x10 : random.Below(0x1E) + 0x1E);
            bubble->stepX = static_cast<i16>(static_cast<int>(random.Below(0x10)) - 8);
            bubble->stepY = static_cast<i16>(0xFFFF - random.Below(1));
            mark.nodes = std::move(bubble);
        }

        std::unique_ptr<RoleMarkNode>* link = &mark.nodes;

        while (*link != nullptr)
        {
            RoleMarkNode& bubble = **link;
            const u32 roll = random.Below(0x3E8);
            bubble.y = static_cast<i16>(bubble.y + bubble.stepY);
            bubble.x = static_cast<i16>(bubble.x + sway + bubble.stepX);

            if (roll > 0x32)
            {
                --bubble.life;
            }

            if (bubble.life > 0)
            {
                if (picture != nullptr)
                {
                    DrawOptions& options = painter.Options();

                    if (warm)
                    {
                        options.tintRed = bubble.life - 0x1E;
                        options.tintGreen = bubble.life - 0x26;
                        options.tintBlue = bubble.life - 0x32;
                    }
                    else
                    {
                        options.tintBlue = (bubble.life >> 1) - 0x1E;
                        options.tintGreen = (bubble.life >> 1) - 0x1E;
                        options.tintRed = (bubble.life >> 1) - 0x1E;
                    }

                    painter.SetSource(picture->data.data());
                    painter.DrawPicture(
                        bubble.x / 0x10 - view.unk4ACAF8.x - field.cameraLeft,
                        bubble.y - view.unk4ACAF8.y - field.cameraTop,
                        picture->width,
                        picture->height,
                        view.unk4ACAF8.flags
                    );
                }

                link = &bubble.next;
                continue;
            }

            if (bubble.next == nullptr)
            {
                link->reset();
                break;
            }

            std::unique_ptr<RoleMarkNode> following = std::move(bubble.next);
            bubble.x = following->x;
            bubble.y = following->y;
            bubble.stepX = following->stepX;
            bubble.stepY = following->stepY;
            bubble.life = following->life;
            bubble.next = std::move(following->next);
        }
    }
}

// swd32.exe 0x004146F0: while a fade step is set the screen is tinted by the levels reached, which grow by the steps
// for its frames (counted with `step`); once they run out the steps are restored from their start values
void FieldView::FadeColors(int step)
{
    ColorFade& fade = this->game.scene.fade;

    if (fade.stepRed == 0.0F && fade.stepGreen == 0.0F && fade.stepBlue == 0.0F)
    {
        return;
    }

    bool restore = false;

    if (step != 0)
    {
        if (fade.frames < 0)
        {
            restore = true;
        }
        else
        {
            --fade.frames;
        }
    }

    if (!restore && fade.frames >= 0)
    {
        fade.red = fade.red + fade.stepRed;
        fade.green = fade.green + fade.stepGreen;
        fade.blue = fade.blue + fade.stepBlue;
    }
    else
    {
        fade.stepRed = fade.startRed;
        fade.stepGreen = fade.startGreen;
        fade.stepBlue = fade.startBlue;
    }

    this->DrawScreenTint(
        static_cast<int>(crt::Ftol(fade.red)),
        static_cast<int>(crt::Ftol(fade.green)),
        static_cast<int>(crt::Ftol(fade.blue))
    );
}

void FieldView::DrawScreenTint(int red, int green, int blue)
{
    this->game.painter.TintPixels(std::span<u16>(this->game.drawSurface, 0x4B000), red, green, blue);
}

// swd32.exe 0x004147E0: the animations of a list stepped and drawn at their screen places with their translucency and
// sound cues; finished ones (the script reached its end) are freed
void FieldView::DrawAnimations(std::unique_ptr<ScriptAnimation>& list)
{
    Game& owner = this->game;
    Painter& painter = owner.painter;
    std::unique_ptr<ScriptAnimation>* link = &list;

    while (*link != nullptr)
    {
        ScriptAnimation& animation = **link;
        owner.acts.Run(animation.act);
        const Picture* picture = owner.archive.GetPicture(animation.act.frame, animation.act.picture);

        if (picture != nullptr)
        {
            painter.Options().alpha = animation.act.signal;
            painter.SetSource(picture->data.data());
            painter.DrawPicture(
                animation.x - animation.act.x,
                animation.y - animation.act.y,
                picture->width,
                picture->height,
                animation.act.flags
            );
        }

        if (animation.act.view != 0)
        {
            owner.PlaySoundAt(
                animation.act.view,
                owner.field.cameraLeft + animation.x,
                owner.field.cameraTop + animation.y
            );
            animation.act.view = 0;
        }

        if (animation.act.waiting == 1)
        {
            *link = std::move(animation.next);
        }
        else
        {
            link = &animation.next;
        }
    }
}

// swd32.exe 0x00414B60: the sprites over the map: stepped, drawn when near the screen, moved by their steps (unless
// the script waits) and freed once within 32 pixels of their target
void FieldView::DrawSprites()
{
    Game& owner = this->game;
    Painter& painter = owner.painter;
    const Field& field = owner.field;
    std::unique_ptr<ScriptSprite>* link = &owner.scene.spritesA;

    while (*link != nullptr)
    {
        ScriptSprite& sprite = **link;
        const auto screenX = static_cast<int>(crt::Ftol(sprite.x)) - field.cameraLeft;
        const auto screenY = static_cast<int>(crt::Ftol(sprite.y)) - field.cameraTop;
        owner.acts.Run(sprite.act);

        if (screenX > -0x48 && screenX < 0x2C8 && screenY > -0x48 && screenY < 0x228)
        {
            const Picture* picture = owner.archive.GetPicture(sprite.act.frame, sprite.act.picture);

            if (picture != nullptr)
            {
                painter.Options().alpha = sprite.act.signal;
                painter.SetSource(picture->data.data());
                painter.DrawPicture(
                    screenX - sprite.act.x,
                    screenY - sprite.act.y,
                    picture->width,
                    picture->height,
                    sprite.act.flags,
                    PictureArgument(*picture)
                );
            }
        }

        if (sprite.act.delay != 0)
        {
            link = &sprite.next;
            continue;
        }

        const double nextX = static_cast<double>(sprite.stepX) + static_cast<double>(sprite.x);
        sprite.x = static_cast<float>(nextX);
        const float nextY = sprite.stepY + sprite.y;
        sprite.y = nextY;
        const auto movedX = static_cast<int>(crt::Ftol(nextX));
        const auto movedY = static_cast<int>(crt::Ftol(nextY));

        if (movedX <= sprite.targetX - 0x20 || movedX >= sprite.targetX + 0x20 || movedY <= sprite.targetY - 0x20 ||
            movedY >= sprite.targetY + 0x20)
        {
            link = &sprite.next;
        }
        else
        {
            *link = std::move(sprite.next);
        }
    }
}

// swd32.exe 0x00414CE0: the role heads: drawn at (headX, targetY), a head with a speed slides out tripling it and is
// freed past the screen's sides, one without slides two thirds of the way to targetX each frame
void FieldView::DrawRoleHeads()
{
    Game& owner = this->game;
    Painter& painter = owner.painter;
    std::unique_ptr<ScriptSprite>* link = &owner.scene.spritesB;

    while (*link != nullptr)
    {
        ScriptSprite& head = **link;
        owner.acts.Run(head.act);
        const Picture* picture = owner.archive.GetPicture(head.act.frame, head.act.picture);

        if (picture != nullptr)
        {
            painter.Options().alpha = head.act.signal;
            painter.SetSource(picture->data.data());
            painter.DrawPicture(
                head.headX - head.act.x,
                head.targetY - head.act.y,
                picture->width,
                picture->height,
                head.act.flags
            );
        }

        if ((head.headSpeed & 0x7FFF) != 0)
        {
            head.headX = static_cast<i16>(head.headX + head.headSpeed);
            head.headSpeed = static_cast<i16>(3 * head.headSpeed);

            if (head.headX <= -0x78 || head.headX >= 0x2F8)
            {
                *link = std::move(head.next);
                continue;
            }

            link = &head.next;
            continue;
        }

        const int slide = 2 * (head.targetX - head.headX) / 3;
        head.headX = static_cast<i16>(head.headX + slide);

        if (slide <= 1 && slide >= -1)
        {
            head.headX = head.targetX;
        }

        link = &head.next;
    }
}

// swd32.exe 0x00414E50: the lines of the screen blended with their colour: still ones whole, the others growing or
// shrinking column by column at random speeds; a closed line becomes still, an opened one is freed
void FieldView::DrawLines()
{
    Game& owner = this->game;
    const Painter& painter = owner.painter;
    Random& random = owner.random;
    const int pitch = painter.ScreenPitch();
    const std::span<u16> screen(owner.drawSurface, static_cast<std::size_t>(pitch) * kScreenHeight);

    auto blend = [&painter, screen](int at, u32 color, int pairs)
    {
        for (int pair = 0; pair < pairs; ++pair)
        {
            const auto first = static_cast<std::size_t>(at + 2 * pair);

            if (at + 2 * pair < 0 || first + 1 >= screen.size())
            {
                continue;
            }

            u32 value = screen[first] | static_cast<u32>(screen[first + 1]) << 16;
            painter.BlendPairsWithColor(std::span<u32>(&value, 1), color);
            screen[first] = static_cast<u16>(value);
            screen[first + 1] = static_cast<u16>(value >> 16);
        }
    };

    std::unique_ptr<ScriptEffect>* link = &owner.scene.effects;

    while (*link != nullptr)
    {
        ScriptEffect& line = **link;
        const u32 color = owner.textColors.primary[static_cast<std::size_t>(line.color) & 0xF];

        const auto column = [](int index) -> std::size_t
        {
            return static_cast<std::size_t>(index);
        };

        auto at = [&line, pitch](int index)
        {
            return line.x + line.offsets[static_cast<std::size_t>(index)] + pitch * (index + line.y);
        };

        if (line.offsets.size() < static_cast<std::size_t>(std::max<int>(line.count, 0)) ||
            line.lengths.size() < static_cast<std::size_t>(std::max<int>(line.count, 0)))
        {
            link = &line.next;
            continue;
        }

        const int kind = line.mode & 0xFF00;

        if (kind == 0x800)
        {
            for (int row = 0; row < line.count; ++row)
            {
                blend(line.x + pitch * (line.y + row), color, line.limit);
            }

            link = &line.next;
            continue;
        }

        if (kind == 0x8000)
        {
            int done = 0;

            for (int index = line.count - 1; index >= 0; --index)
            {
                i16& length = line.lengths[column(index)];
                length = static_cast<i16>(length + 2 * static_cast<int>(random.Below(6)) + 0x30);

                if (length >= line.limit)
                {
                    ++done;
                    length = line.limit;
                }

                blend(at(index), color, length);
            }

            if (done == line.count)
            {
                line.mode = static_cast<u16>((line.mode & 0xFF) | 0x800);
            }
        }

        if ((line.mode & 0xFF00) == 0x4000)
        {
            int done = 0;

            for (int index = line.count - 1; index >= 0; --index)
            {
                i16& offset = line.offsets[column(index)];
                offset = static_cast<i16>(offset + 0xFFD0 - 2 * static_cast<int>(random.Below(0x18)));

                if (offset <= 0)
                {
                    ++done;
                    offset = 0;
                }

                line.lengths[column(index)] = static_cast<i16>(line.limit - offset);
                blend(at(index), color, line.lengths[column(index)]);
            }

            if (done == line.count)
            {
                line.mode = static_cast<u16>((line.mode & 0xFF) | 0x800);
            }
        }

        bool finished = false;

        if ((line.mode & 0xFF00) == 0x2000)
        {
            int done = 0;

            for (int index = line.count - 1; index >= 0; --index)
            {
                i16& offset = line.offsets[column(index)];
                offset = static_cast<i16>(offset + 2 * static_cast<int>(random.Below(0x30)) + 0x30);

                if (offset >= line.limit)
                {
                    ++done;
                    offset = line.limit;
                }

                i16& length = line.lengths[column(index)];
                length = static_cast<i16>(line.limit - offset);

                if (length <= 1)
                {
                    length = 2;
                }

                blend(at(index), color, length);
            }

            finished = done == line.count;
        }

        if (!finished && (line.mode & 0xFF00) == 0x1000)
        {
            int done = 0;

            for (int index = line.count - 1; index >= 0; --index)
            {
                i16& length = line.lengths[column(index)];
                length = static_cast<i16>(length + 0xFFD0 - 2 * static_cast<int>(random.Below(0x30)));

                if (length <= 2)
                {
                    ++done;
                    length = 2;
                }

                blend(at(index), color, length);
            }

            finished = done == line.count;
        }

        if (finished)
        {
            *link = std::move(line.next);
        }
        else
        {
            link = &line.next;
        }
    }
}

// swd32.exe 0x004151F0: for each drawing order 0..30 the first overlay of that order over the camera, clipped to the
// part of its area on screen and placed by its parallax (8: fixed to the map)
void FieldView::DrawOverlays()
{
    Game& owner = this->game;
    Painter& painter = owner.painter;
    const Field& field = owner.field;

    for (int order = 0; order < 0x1F; ++order)
    {
        for (const MapOverlay* overlay = field.overlays.get(); overlay != nullptr; overlay = overlay->next.get())
        {
            if (overlay->kind != order)
            {
                continue;
            }

            const int areaLeft = overlay->left;
            const int areaTop = overlay->top;
            const int areaRight = overlay->right + 0x10;
            const int areaBottom = overlay->bottom + 0x10;
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

            if (overlay->parallaxX == 8)
            {
                offsetX = field.cameraLeft >= areaLeft ? field.cameraLeft - areaLeft : 0;
            }
            else
            {
                offsetX = overlay->parallaxX * (field.cameraRight - areaLeft) / 0x10;

                if (field.cameraLeft < areaLeft)
                {
                    offsetX += clipLeft;
                }
            }

            int offsetY = 0;

            if (overlay->parallaxY == 8)
            {
                offsetY = field.cameraTop >= areaTop ? field.cameraTop - areaTop : 0;
            }
            else
            {
                offsetY = overlay->parallaxY * (field.cameraBottom - areaTop) / 0x10;

                if (field.cameraTop < areaTop)
                {
                    offsetY += clipTop;
                }
            }

            painter.SetSource(overlay->picture.data());
            painter.DrawPicture(clipLeft - offsetX, clipTop - offsetY, overlay->width, overlay->height, 0);
            painter.SetClipClamped(0, 0, kScreenWidth, kScreenHeight);
            break;
        }
    }
}

// swd32.exe 0x00415B70: the weather (story flag 5): the first two particles drift with the camera at their own pace and
// shade; one off the map is placed again at an edge by the map's weather kind, now and then
void FieldView::DrawWeather()
{
    Game& owner = this->game;
    GameState& state = owner.state;

    if (!state.GameFlag(5))
    {
        return;
    }

    Scene& view = owner.scene;
    Random& random = owner.random;
    Painter& painter = owner.painter;
    const Field& field = owner.field;

    for (std::size_t particle = 0; particle < 2; ++particle)
    {
        const auto roll = static_cast<int>(random.Below(0x3E8));
        const int width = state.map.width;
        Particle& drop = view.weather[particle];
        WeatherShade& shade = view.weatherShades[particle];
        WeatherMotion& motion = view.weatherMotion[particle];

        if (drop.x <= -0x280 || drop.x >= 0x10 * (width + 0x28) || drop.y <= -0x140 ||
            drop.y >= 0x10 * (state.map.height + 0x14))
        {
            if (roll < 0x3DE)
            {
                continue;
            }

            shade.tintTarget = 0;
            motion.speedTarget = roll / 0xC8 + 1;
            motion.action = view.unk4ACEF0 + roll * view.unk4C97F6 / 0x3E8;

            switch (view.unk4B7EB4)
            {
                case 0:
                    drop.x = static_cast<i32>(random.Below(static_cast<u32>(0x10 * width)));
                    drop.y = 0x10 * state.map.height + 0x76;
                    drop.stepX = static_cast<i32>(random.Below(1)) - 2;
                    drop.stepY = static_cast<i32>(random.Below(1)) - 2;
                    break;

                case 1:
                    drop.x = static_cast<i32>(random.Below(static_cast<u32>(0x10 * width)));
                    drop.y = 0x10 * state.map.height + 0x76;
                    drop.stepX = static_cast<i32>(random.Below(2));
                    drop.stepY = static_cast<i32>(random.Below(2));
                    break;

                case 2:
                    drop.x = static_cast<i32>(random.Below(static_cast<u32>(0x10 * width)));
                    drop.y = -0x12C;
                    drop.stepX = static_cast<i32>(random.Below(2)) - 2;
                    drop.stepY = static_cast<i32>(random.Below(2)) - 2;
                    break;

                case 3:
                    drop.x = static_cast<i32>(random.Below(static_cast<u32>(0x10 * width)));
                    drop.y = -0x12C;
                    drop.stepX = static_cast<i32>(random.Below(2));
                    drop.stepY = static_cast<i32>(random.Below(2));
                    break;

                default:
                    break;
            }

            continue;
        }

        const int speed = motion.speed;

        if (++motion.counter > speed)
        {
            motion.counter = 0;
            drop.x += drop.stepX + ((owner.unk4B7518 * view.unk4AB2D0) >> 2);
            drop.y += drop.stepY + ((view.unk4AB2D0 * owner.unk4B7920) >> 2);

            if (roll < 0x19)
            {
                motion.speedTarget = static_cast<i32>(random.Below(5)) + 1;
            }

            if (motion.speed != motion.speedTarget)
            {
                motion.speed += motion.speed <= motion.speedTarget ? 1 : -1;
            }

            if (roll < 5)
            {
                shade.tintTarget = static_cast<i32>(random.Below(4)) - 5;
            }

            if (shade.tint != shade.tintTarget)
            {
                shade.tint += shade.tint <= shade.tintTarget ? 1 : -1;
            }
        }

        view.unk4ACAF8.id = kEffectAct;
        view.unk4ACAF8.action = motion.action;
        owner.acts.Run(view.unk4ACAF8);
        const Picture* picture = owner.archive.GetPicture(view.unk4ACAF8.frame, view.unk4ACAF8.picture);

        if (picture == nullptr)
        {
            continue;
        }

        DrawOptions& options = painter.Options();
        options.tintBlue = shade.tint;
        options.tintGreen = shade.tint;
        options.tintRed = shade.tint;
        options.alpha = view.unk4ACAF8.signal;
        painter.SetSource(picture->data.data());
        painter.DrawPicture(
            drop.x - view.unk4ACAF8.x - field.cameraLeft,
            drop.y - view.unk4ACAF8.y - field.cameraTop,
            picture->width,
            picture->height,
            view.unk4C97F6 == 4 ? 4u : 0x2Cu
        );
    }
}

// swd32.exe 0x004161C0: the leaves (story flag 6): four leaves blow across the map, entering again from a side once
// they leave it and now and then changing speed; the action follows the vertical speed
void FieldView::DrawLeaves()
{
    Game& owner = this->game;
    GameState& state = owner.state;

    if (!state.GameFlag(6))
    {
        return;
    }

    Scene& view = owner.scene;
    Random& random = owner.random;
    Painter& painter = owner.painter;
    const Field& field = owner.field;
    const int width = 0x10 * state.map.width;
    const int height = 0x10 * state.map.height;

    for (std::size_t index = 0; index < view.leaves.size(); ++index)
    {
        Particle& leaf = view.leaves[index];
        ActState& act = view.unk4AD0D8[index];

        if (leaf.x <= -0x78 || leaf.x >= width + 0x78 || leaf.y <= -0x78 || leaf.y >= height + 0x78)
        {
            leaf.x = 0x7FFFFFFF;
        }

        if (leaf.x == 0x7FFFFFFF)
        {
            leaf.x = -0x40;
            leaf.y = static_cast<i32>(random.Below(static_cast<u32>(height)));
            leaf.stepY = static_cast<i32>(random.Below(5)) - 2;
            leaf.stepX = static_cast<i32>(random.Below(7)) - 3;

            if (leaf.stepX == 0)
            {
                leaf.stepX = 2;
            }

            if (leaf.stepX < 0)
            {
                leaf.x = width + 0x40;
            }
        }
        else if (random.Below(0x3E8) > 0xFA)
        {
            const int turn = static_cast<int>(random.Below(3)) - 1;
            leaf.stepY = std::clamp(static_cast<int>(random.Below(3)) - 1 + leaf.stepY, -3, 3);
            leaf.stepX = std::clamp(turn + leaf.stepX, -5, 5);

            if (leaf.stepX == 0)
            {
                leaf.stepX = -turn;
            }
        }

        act.action = kLeafActions[static_cast<std::size_t>(std::clamp(leaf.stepY + 3, 0, 7))];

        if (leaf.stepX > 0)
        {
            ++act.action;
        }

        owner.acts.Run(act);
        const Picture* picture = owner.archive.GetPicture(act.frame, act.picture);

        if (picture != nullptr)
        {
            painter.SetSource(picture->data.data());
            painter.DrawPicture(
                leaf.x - act.x - field.cameraLeft,
                leaf.y - act.y - field.cameraTop,
                picture->width,
                picture->height,
                act.flags
            );
        }

        leaf.x += leaf.stepX;
        leaf.y += leaf.stepY;
    }
}

// swd32.exe 0x004163C0: the heat haze (story flag 7): 48 strips of the screen copied up a line each frame; every
// fourth frame one of their lengths, places or line counts is chosen again (all three on the first frame)
void FieldView::DrawHeat()
{
    Game& owner = this->game;

    if (!owner.state.GameFlag(7))
    {
        return;
    }

    Scene& view = owner.scene;
    Random& random = owner.random;

    auto newLength = [&random]()
    {
        return static_cast<i16>(2 * random.Below(0x26) + 0x4C);
    };

    auto newOffset = [&random](int column, int line)
    {
        return line + static_cast<i32>(random.Below(3)) + column + 0x27F;
    };

    if (view.heatFrame != 0)
    {
        if ((view.heatFrame & 3) == 0)
        {
            switch ((view.heatFrame >> 2) & 3)
            {
                case 0:
                    for (i16& length : view.heatLengths)
                    {
                        length = newLength();
                    }

                    break;

                case 1:
                {
                    std::size_t index = 0;

                    for (int column = 0; column < kScreenWidth; column += 0x50)
                    {
                        for (int line = 0; line < 0x4B000; line += 0xC800)
                        {
                            view.heatOffsets[index++] = newOffset(column, line);
                        }
                    }

                    break;
                }

                case 2:
                    for (i16& count : view.heatCounts)
                    {
                        count = static_cast<i16>(random.Below(0x4E) + 1);
                    }

                    break;

                default:
                    break;
            }
        }
    }
    else
    {
        std::size_t index = 0;

        for (int column = 0; column < kScreenWidth; column += 0x50)
        {
            for (int line = 0; line < 0x4B000; line += 0xC800)
            {
                view.heatLengths[index] = newLength();
                view.heatOffsets[index] = newOffset(column, line);
                view.heatCounts[index] = static_cast<i16>(random.Below(0x4E) + 1);
                ++index;
            }
        }
    }

    ++view.heatFrame;
    auto* bytes = reinterpret_cast<u8*>(owner.drawSurface);
    constexpr std::size_t kScreenBytes = 0x96000;
    constexpr int kLineBytes = 0x500;

    for (std::size_t index = 0; index < view.heatOffsets.size(); ++index)
    {
        const int count = view.heatCounts[index];
        const auto length = static_cast<u16>(view.heatLengths[index]);
        int at = 2 * view.heatOffsets[index];

        for (int line = 0; line < count; ++line, at += kLineBytes)
        {
            if (at < 0 || static_cast<std::size_t>(at) + kLineBytes + length > kScreenBytes)
            {
                continue;
            }

            std::memmove(bytes + at, bytes + at + kLineBytes, length);
        }
    }
}

// swd32.exe 0x00416590: lightning (story flag 8): the bolts wanted grow to 8 while the flag is set and shrink without
// it, changing only on one frame in ten; each bolt runs down the screen as a slanted line of brightening pixels
void FieldView::DrawLightning()
{
    Game& owner = this->game;
    Scene& view = owner.scene;
    Random& random = owner.random;
    bool draw = false;

    if (random.Below(0x3E8) > 0x384)
    {
        if (owner.state.GameFlag(8))
        {
            if (++view.lightningCount > 8)
            {
                view.lightningCount = 8;
                draw = true;
            }
        }
        else if (--view.lightningCount < 0)
        {
            view.lightningCount = 0;
            draw = view.lightningShown != 0;

            if (!draw)
            {
                return;
            }
        }
    }

    if (!draw && view.lightningCount == 0 && view.lightningShown == 0)
    {
        return;
    }

    const int pitch = owner.painter.ScreenPitch();
    const auto pixels = static_cast<std::ptrdiff_t>(pitch) * kScreenHeight;
    view.lightningShown = 0;
    int started = 0;

    for (Streak& bolt : view.bolts)
    {
        if ((bolt.active & 1) == 0)
        {
            if (started < view.lightningCount)
            {
                bolt.x = static_cast<i16>(0x10 * random.Below(kScreenWidth));
                bolt.y = 0;
                const auto slope = static_cast<i16>(random.Below(3));
                bolt.slope = bolt.x <= 0x1400 ? slope : static_cast<i16>(-slope);
                bolt.speed = static_cast<i16>(random.Below(0x1E) + 0x10);
                bolt.length = static_cast<i16>(random.Below(0x10) + 8);
                bolt.life = bolt.speed <= 0xF ? 0x10 : 0x20;
                bolt.phase = 0;
                bolt.active = 1;
                ++started;
            }

            continue;
        }

        int level = 0;
        int x = bolt.x;
        std::ptrdiff_t row = static_cast<std::ptrdiff_t>(pitch) * bolt.y;

        for (int segment = 0; bolt.length >= 0;)
        {
            if (row > 0)
            {
                if (row >= pixels)
                {
                    bolt.active = 0;
                }
                else
                {
                    const std::ptrdiff_t at = row + x / 0x10;

                    if (at >= 0 && at < pixels)
                    {
                        owner.painter.TintPixels(std::span<u16>(owner.drawSurface + at, 1), level, level, level);
                    }
                }
            }
            else if (row >= pixels)
            {
                bolt.active = 0;
            }

            x += bolt.slope;
            row += pitch;
            ++level;

            if (bolt.life == 0x20)
            {
                ++level;
            }

            if (++segment > bolt.length)
            {
                break;
            }
        }

        bolt.x = static_cast<i16>(bolt.x + bolt.speed * bolt.slope);
        bolt.y = static_cast<i16>(bolt.y + bolt.speed);

        if (--bolt.life != 0)
        {
            ++view.lightningShown;
        }
        else
        {
            bolt.active = 0;
        }
    }
}

// swd32.exe 0x004167B0: fireflies (story flag 0x16): at most one at a time drifts down the screen, a glowing dot with
// a dimmer cross around it that sways with its phase and fades as its life runs out
void FieldView::DrawFireflies()
{
    Game& owner = this->game;
    Scene& view = owner.scene;
    Random& random = owner.random;
    bool draw = false;

    if (random.Below(0x3E8) > 0x384)
    {
        if (owner.state.GameFlag(0x16))
        {
            if (++view.fireflyCount > 1)
            {
                view.fireflyCount = 1;
                draw = true;
            }
        }
        else if (--view.fireflyCount < 0)
        {
            view.fireflyCount = 0;
            draw = view.firefliesShown != 0;

            if (!draw)
            {
                return;
            }
        }
    }

    if (!draw && view.fireflyCount == 0 && view.firefliesShown == 0)
    {
        return;
    }

    Painter& painter = owner.painter;
    const int pitch = painter.ScreenPitch();
    const auto pixels = static_cast<std::ptrdiff_t>(pitch) * kScreenHeight;
    const std::ptrdiff_t firstLine = 0x500 / 2;
    const std::ptrdiff_t lastLine = (0x95B00 / 2);
    view.firefliesShown = 0;
    int started = 0;

    auto tint = [&painter, &owner, pixels](std::ptrdiff_t at, int level)
    {
        if (at >= 0 && at < pixels)
        {
            painter.TintPixels(std::span<u16>(owner.drawSurface + at, 1), level, level, level);
        }
    };

    for (Streak& fly : view.fireflies)
    {
        if ((fly.active & 1) == 0)
        {
            if (started < view.fireflyCount)
            {
                fly.x = static_cast<i16>(0x10 * random.Below(kScreenWidth));
                fly.y = 0;
                const auto slope = static_cast<i16>(random.Below(3));
                fly.slope = fly.x <= 0x1400 ? slope : static_cast<i16>(-slope);
                fly.speed = static_cast<i16>(random.Below(3) + 1);
                fly.length = 1;
                fly.life = static_cast<i16>(0xA0 * fly.speed - static_cast<int>(random.Below(0xA0)));
                fly.phase = 0;
                fly.active = 1;
                ++started;
            }

            continue;
        }

        auto level = static_cast<u32>(0x1F - (0x1E0 - fly.life) / 0x18);

        if (level > 0x3E8)
        {
            level = 0;
        }

        int x = fly.x;
        const int sway = kFireflySway[static_cast<std::size_t>(fly.phase / 4) & 7];
        std::ptrdiff_t row = static_cast<std::ptrdiff_t>(pitch) * fly.y;

        for (int segment = 0; fly.length > 0;)
        {
            if (row > firstLine)
            {
                if (row >= lastLine)
                {
                    fly.active = 0;
                }
                else
                {
                    const auto full = static_cast<int>(level);
                    const int half = full >> 1;
                    const int quarter = full >> 2;
                    const std::ptrdiff_t center = row + x / 0x10 + sway;
                    tint(center, full);
                    tint(center + 1, half);
                    tint(center - 1, half);
                    tint(center + pitch, half);
                    tint(center - pitch, half);
                    tint(center + pitch + 1, quarter);
                    tint(center + pitch - 1, quarter);
                    tint(center - pitch + 1, quarter);
                    tint(center - pitch - 1, quarter);
                }
            }
            else if (row >= lastLine)
            {
                fly.active = 0;
            }

            x += fly.slope;
            row += pitch;
            ++level;

            if (fly.life == 0x20)
            {
                ++level;
            }

            if (++segment >= fly.length)
            {
                break;
            }
        }

        fly.x = static_cast<i16>(fly.x + fly.speed * fly.slope);
        fly.y = static_cast<i16>(fly.y + fly.speed);

        if (++fly.phase > 0x1C)
        {
            fly.phase = 0;
        }

        fly.life = static_cast<i16>(fly.life - fly.speed);

        if (fly.life > 0)
        {
            ++view.firefliesShown;
        }
        else
        {
            fly.active = 0;
        }
    }
}

// swd32.exe 0x00416B30: the spotlight (story flag 0x13): its rim (action 0x4E, clipped by mixing its width and height)
// and the dark around it (action 0x4F, subtracted within 192 pixels); the light then moves towards its target
void FieldView::DrawSpotlight()
{
    Game& owner = this->game;

    if (!owner.state.GameFlag(0x13))
    {
        return;
    }

    Scene& view = owner.scene;
    Painter& painter = owner.painter;
    ActState& act = view.unk4AD3F0;
    act.id = kEffectAct;
    act.action = 0x4E;
    owner.acts.Run(act);
    const Picture* rim = owner.archive.GetPicture(act.frame, act.picture);

    if (rim != nullptr)
    {
        painter.SetClipClamped(
            view.unk4B9EF8 - (rim->width >> 1),
            view.unk4B9EFC - (rim->width >> 1),
            view.unk4B9EF8 + (rim->height >> 1),
            (rim->height >> 1) + view.unk4B9EFC
        );
        painter.SetSource(rim->data.data());
        painter.DrawPicture(
            view.unk4B9EF8 - (rim->width >> 1),
            view.unk4B9EFC - (rim->height >> 1),
            rim->width,
            rim->height,
            0
        );
    }

    act.action = 0x4F;
    owner.acts.Run(act);
    const Picture* dark = owner.archive.GetPicture(act.frame, act.picture);
    painter.SetClipClamped(view.unk4B9EF8 - 0xC0, view.unk4B9EFC - 0xC0, view.unk4B9EF8 + 0xC0, view.unk4B9EFC + 0xC0);

    if (dark != nullptr)
    {
        painter.SetSource(dark->data.data());
        painter.DrawPicture(
            view.unk4B9EF8 - (dark->width >> 1),
            view.unk4B9EFC - (dark->height >> 1),
            dark->width,
            dark->height,
            0x2C
        );
    }

    if (view.unk4B9EF8 != view.unk4A94A8 || view.unk4B9EFC != view.unk4CAE80)
    {
        view.unk4B9EF8 += view.unk4ACE58;
        view.unk4B9EFC += view.unk4ACE5C;

        if (view.unk4B9EF8 == view.unk4A94A8)
        {
            view.unk4ACE58 = 0;
        }

        if (view.unk4B9EFC == view.unk4CAE80)
        {
            view.unk4ACE5C = 0;
        }
    }
}

// swd32.exe 0x00416CC0: the ripples: each copies the screen and refracts it through its waves; settled ones are freed
void FieldView::DrawRipples()
{
    Game& owner = this->game;
    const std::span<u16> screen(
        owner.drawSurface,
        static_cast<std::size_t>(owner.painter.ScreenPitch()) * kScreenHeight
    );
    std::unique_ptr<ScriptObject>* link = &owner.scene.objects;

    while (*link != nullptr)
    {
        ScriptObject& ripple = **link;
        ripple.CopyScreen(screen);
        ripple.Refract(screen);

        if (ripple.Step())
        {
            *link = std::move(ripple.next);
        }
        else
        {
            link = &ripple.next;
        }
    }
}

// swd32.exe 0x004308C0: the time left of a timed event (which 0: the timer 0x004CAD20 under flag 0x10, 1: 0x004A9918
// under flag 0x4A) in frames as [h]m:ss with the digits of act 0x232C, unless flag 0x4C hides it
void FieldView::DrawTimer(int x, int y, int which)
{
    Game& owner = this->game;
    const GameState& state = owner.state;

    if (which == 0 && !state.GameFlag(0x10))
    {
        return;
    }

    if (which == 1 && !state.GameFlag(0x4A))
    {
        return;
    }

    if (state.GameFlag(0x4C))
    {
        return;
    }

    Scene& view = owner.scene;
    Painter& painter = owner.painter;
    int seconds = (which != 0 ? view.unk4A9918 : view.unk4CAD20) / 0x1E;

    if (seconds < 0)
    {
        seconds = 0;
    }

    ActState& digits = view.unk4A9A30;
    digits.id = kTimerAct;
    digits.direction = 0;
    int at = x;

    auto draw = [&owner, &painter, &digits, &at, y](int action)
    {
        digits.action = action;
        owner.acts.Run(digits);
        const Picture* picture = owner.archive.GetPicture(digits.frame, digits.picture);

        if (picture != nullptr)
        {
            painter.SetSource(picture->data.data());
            painter.DrawPicture(at, y, picture->width, picture->height, 0);
            at += picture->width;
        }
    };

    if (seconds / 0x258 != 0)
    {
        draw(seconds / 0x258);
    }

    draw(seconds / 0x3C % 0xA);
    draw(0xA);
    const int rest = seconds % 0x3C;
    draw(rest / 0xA);
    draw(rest % 0xA);
}

// swd32.exe 0x00430180: the outline of a cell mark: the top and bottom lines as `width / 2` pixel pairs, the sides
// `height` pixels long
void FieldView::FillCellMark(int x, int y, int width, int height, int color)
{
    Game& owner = this->game;
    const int pitch = owner.painter.ScreenPitch();
    const auto pixels = static_cast<std::ptrdiff_t>(pitch) * kScreenHeight;
    const auto value = static_cast<u16>(color);

    auto put = [&owner, pixels, value](std::ptrdiff_t at)
    {
        if (at >= 0 && at < pixels)
        {
            owner.drawSurface[at] = value;
        }
    };

    const int pairs = width >> 1;

    for (int pair = 0; pair < pairs; ++pair)
    {
        const std::ptrdiff_t top = x + static_cast<std::ptrdiff_t>(pitch) * y + 2 * pair;
        const std::ptrdiff_t bottom = x + static_cast<std::ptrdiff_t>(pitch) * (y + height) + 2 * pair;
        put(top);
        put(top + 1);
        put(bottom);
        put(bottom + 1);
    }

    for (int line = 0; line < height; ++line)
    {
        const std::ptrdiff_t left = x + static_cast<std::ptrdiff_t>(pitch) * (y + line);
        put(left);
        put(left + 2 * pairs);
    }
}

// swd32.exe 0x00430230: the debug marks of the cells under the camera holding any bit of `mask` (38 x 28 cells from
// one past the camera), inset by `inset` pixels
void FieldView::MarkCells(u32 mask, int color, int offsetX, int offsetY, int inset)
{
    const Field& field = this->game.field;
    const int width = this->game.state.map.width;
    const int shiftY = offsetY & 0xF;
    const int shiftX = -(offsetX & 0xF);
    int cell = ((field.cameraLeft + 0x10) >> 4) + width * ((field.cameraTop + 0x10) >> 4);
    int y = inset - shiftY + 0x10;

    for (int row = 0; row < 0x1C; ++row)
    {
        for (int x = 0x10; x < 0x26F; x += 0x10)
        {
            const std::vector<u32>& blocks = this->game.state.map.blocks;
            const u32 value = cell >= 0 && static_cast<std::size_t>(cell) < blocks.size()
                                  ? blocks[static_cast<std::size_t>(cell)]
                                  : 0;

            if ((mask & value) != 0)
            {
                this->FillCellMark(inset + x + shiftX, y, 0xF - inset, 0xF - inset, color);
            }

            ++cell;
        }

        y += 0x10;
        cell += width - 0x26;
    }
}

// swd32.exe 0x00414570: the camera pans by the step of the pan still to go (0x004A99F0 across, 0x004A98C0 down); a step
// is cleared once its distance is covered
void FieldView::PanCamera()
{
    Field& field = this->game.field;
    Scene& view = this->game.scene;

    if (view.unk4A99F0 == 0 && view.unk4A98C0 == 0)
    {
        return;
    }

    field.cameraLeft += view.unk4A9480;
    field.cameraRight += view.unk4A9480;
    field.cameraTop += view.unk4A93D8;
    const int remaining = view.unk4A98C0 - view.unk4A93D8;
    field.cameraBottom += view.unk4A93D8;
    view.unk4A99F0 -= view.unk4A9480;
    view.unk4A98C0 -= view.unk4A93D8;

    if (view.unk4A99F0 == 0)
    {
        view.unk4A9480 = 0;
    }

    if (remaining == 0)
    {
        view.unk4A93D8 = 0;
    }
}

// swd32.exe 0x004148F0: the quake: the camera is offset by the pairs of the shake table (0x004ACE70, ended by 0xCFCF)
// in turn, each held for 0x004AD0D0 frames (not on map 0x16); the camera before it is kept to be restored
void FieldView::ShakeCamera()
{
    Field& field = this->game.field;
    Scene& view = this->game.scene;

    if (view.shake[0] == static_cast<i16>(0xCFCF) || this->game.unk4A9A10 == 0x16)
    {
        return;
    }

    auto at = static_cast<std::size_t>(view.shakeIndex);

    if (at >= view.shake.size() || view.shake[at] == static_cast<i16>(0xCFCF))
    {
        at = 0;
        view.shakeIndex = 0;
    }

    const int dx = view.shake[at];
    const int dy = at + 1 < view.shake.size() ? view.shake[at + 1] : 0;

    if (--view.shakeFrames <= 0)
    {
        view.shakeFrames = view.shakeHold;
        view.shakeIndex = static_cast<i32>(at) + 2;
    }

    view.unk4A93E4 = field.cameraLeft;
    field.cameraLeft += dx;
    view.unk4A992C = field.cameraTop;
    field.cameraTop += dy;
    field.cameraRight += dx;
    field.cameraBottom += dy;
}

// swd32.exe 0x00430CF0: its grid and screen copy freed
ScriptObject::~ScriptObject() = default;

// swd32.exe 0x00430D50: a copy of the screen
void ScriptObject::CopyScreen(std::span<const u16> pixels)
{
    const std::size_t count = std::min(pixels.size(), static_cast<std::size_t>(this->pitch) * this->lines);
    this->screen.assign(pixels.begin(), pixels.begin() + static_cast<std::ptrdiff_t>(count));
}

// swd32.exe 0x00430D80: the screen inside the grid redrawn from the copy, each pixel taken from the place its wave
// slopes point at (the differences to the cells right of and below it, divided by 8); only the first page is read
void ScriptObject::Refract(std::span<u16> pixels) const
{
    const int width = this->gridWidth;
    const int last = width * (this->gridHeight - 1);
    int at = this->left + this->pitch * (this->top + 1);

    auto height = [this](int index) -> int
    {
        return index >= 0 && static_cast<std::size_t>(index) < this->heights.size()
                   ? this->heights[static_cast<std::size_t>(index)]
                   : 0;
    };

    auto copy = [this](int index, u16& out)
    {
        if (index >= 0 && static_cast<std::size_t>(index) < this->screen.size())
        {
            out = this->screen[static_cast<std::size_t>(index)];
        }
    };

    for (int cell = width + 1; cell < last; at += this->pitch - width + 2)
    {
        const int end = width + cell - 2;

        while (cell < end)
        {
            for (int pair = 0; pair < 2; ++pair)
            {
                const int here = height(cell);
                const int source =
                    this->pitch * ((here - height(cell + width)) >> 3) + at + ((here - height(cell + 1)) >> 3);

                if (at >= 0 && static_cast<std::size_t>(at) < pixels.size())
                {
                    copy(source, pixels[static_cast<std::size_t>(at)]);
                }

                ++cell;
                ++at;
            }
        }

        cell += 2;
    }
}

// swd32.exe 0x00430EB0: one step of the waves into the other page: each cell becomes the average of its neighbours'
// sums less its old value, damped; true once every cell of the new page is 0
bool ScriptObject::Step()
{
    const int width = this->gridWidth;
    const int cells = width * this->gridHeight;
    this->page ^= 1;
    const int target = this->page * cells;
    const int source = (this->page ^ 1) * cells;
    bool settled = true;

    auto read = [this, source](int index) -> int
    {
        const int at = source + index;
        return at >= 0 && static_cast<std::size_t>(at) < this->heights.size()
                   ? this->heights[static_cast<std::size_t>(at)]
                   : 0;
    };

    int cell = width + 1;
    int sum = read(2 * width + 2) + read(2);
    const int last = width * (this->gridHeight - 1);

    while (cell < last)
    {
        int before = sum;
        sum = read(width + cell) + read(cell - width);
        const int end = width + cell - 2;

        while (cell < end)
        {
            const int current = sum;
            sum = read(cell + width + 1) + read(cell - width + 1);
            const auto at = static_cast<std::size_t>(target + cell);

            if (at < this->heights.size())
            {
                const int change =
                    ((before + current + read(cell - 1) + sum + read(cell + 1)) >> 2) - this->heights[at];
                const auto value = static_cast<i16>(change - (change >> this->damping));
                this->heights[at] = value;

                if (value != 0)
                {
                    settled = false;
                }
            }

            ++cell;
            before = current;
        }

        cell += 2;
    }

    return settled;
}

}  // namespace swd3
