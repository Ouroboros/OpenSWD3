#include "map/field_view.h"

#include <algorithm>
#include <climits>
#include <cmath>
#include <cstring>
#include <span>

#include "core/byte_reader.h"
#include "core/debug_log.h"
#include "core/words.h"
#include "crt/msvc.h"
#include "game/game.h"
#include "platform/display.h"

namespace swd3
{

namespace
{

constexpr int kScreenColumns = Display::Width / MapHeader::TileSize;
constexpr int kScreenRows = Display::Height / MapHeader::TileSize;
constexpr int kRoleMarginX = Display::Width / 2;  // roles this far beside the screen are still drawn
constexpr int kReflectionRowsAbove = 5;           // the reflections are drawn from this many rows above the camera
constexpr u32 kNoFrame = 0xFFFF;                  // the frame of a role without an act
constexpr int kPerMille = 1000;                   // random.Below(kPerMille): the chances are in thousandths
constexpr int kSoundRange = 0x200;                // the distance (pixels) a role is heard from
constexpr int kSoundScale = 0x80;                 // the volume and the distance kept for the sounds
constexpr int kPanScale = 0x40;                   // the pan at kSoundRange to the side

constexpr int kSpriteMargin = 0x48;   // sprites this far off the screen are still drawn
constexpr int kSpriteArrival = 0x20;  // a sprite this near its target is freed

// The script lines (0x00414E50): each column moves kLineStep pixels a frame plus twice a random jitter.
constexpr int kLineStep = 0x30;
constexpr int kFallJitter = 6;
constexpr int kRiseJitter = 0x18;
constexpr int kOpenJitter = 0x30;  // opening and closing

// The weather (0x00415B70).
constexpr std::size_t kWeatherParticles = 2;
constexpr int kWeatherMarginX = Display::Width;  // a particle this far off the map is placed again
constexpr int kWeatherMarginY = 0x140;
constexpr int kWeatherReturnRoll = 0x3DE;  // a particle off the map returns on a roll (per mille) from this
constexpr int kWeatherSpeedStep = 0xC8;    // the returning roll gives the speed: roll / kWeatherSpeedStep + 1
constexpr int kWeatherBelow = 0x76;        // a particle rising again starts this far below the map
constexpr int kWeatherAbove = 0x12C;       // a falling one this far above it
constexpr int kWeatherDrift = 2;           // the steps: Below(kWeatherDrift), less kWeatherDrift for the left
constexpr int kWalkQuarter = 2;            // the particles follow a quarter of the walk
constexpr int kWeatherSpeedChance = 0x19;  // per mille: the speed target changes to 1..kWeatherSpeeds
constexpr int kWeatherSpeeds = 5;
constexpr int kWeatherTintChance = 5;  // per mille: the tint target changes
constexpr int kWeatherTints = 4;       // to -kWeatherDarkest + Below(kWeatherTints)
constexpr int kWeatherDarkest = 5;

// The leaves (0x004161C0).
constexpr int kLeafMargin = 0x78;  // a leaf this far off the map is placed again
constexpr int kLeafEntry = 0x40;   // beside the map, where a leaf enters
constexpr int kLeafStartDown = 2;  // the first vertical step: -kLeafStartDown..kLeafStartDown
constexpr int kLeafStartSide = 3;  // the first sideways step: -kLeafStartSide..kLeafStartSide
constexpr int kLeafZeroSide = 2;   // what a first sideways step of zero becomes
constexpr int kLeafMaxDown = 3;
constexpr int kLeafMaxSide = 5;

// The heat haze (0x004163C0): strips of kHeatStrip x kHeatStrip pixels, changed every kHeatPeriod frames.
constexpr int kHeatStrip = 0x50;
constexpr int kHeatPeriod = 4;
constexpr int kHeatLengthMin = 0x4C;  // bytes copied per line: kHeatLengthMin + 2 * Below(kHeatLengthJitter)
constexpr int kHeatLengthJitter = 0x26;
constexpr int kHeatLines = 0x4E;  // lines moved: 1 + Below(kHeatLines)

// Lightning (0x00416590) and fireflies (0x004167B0): streaks down the screen, their numbers changing on a
// tenth of the frames.
constexpr int kMaxBolts = 8;
constexpr int kMaxFireflies = 1;
constexpr int kSlopes = 3;           // a streak slants 0..2 subpixels a line, away from the nearer side
constexpr int kBoltSpeedMin = 0x10;  // lines a frame: kBoltSpeedMin + Below(kBoltSpeedJitter)
constexpr int kBoltSpeedJitter = 0x1E;
constexpr int kBoltLengthMin = 8;  // lines: kBoltLengthMin + Below(kBoltLengthJitter)
constexpr int kBoltLengthJitter = 0x10;
constexpr int kShortLife = 0x10;    // frames of a slower bolt (none is)
constexpr int kBrightLife = 0x20;   // frames of a bolt; a streak with this life brightens twice as fast
constexpr int kFireflySpeeds = 3;   // lines a frame: 1..kFireflySpeeds
constexpr int kFireflyLife = 0xA0;  // frames: kFireflyLife x speed less Below(kFireflyLife)
constexpr int kFireflyGlow = 0x1F;  // the brightness at full life, less one per kFireflyFade frames lived
constexpr int kFireflyFade = 0x18;
constexpr u32 kFireflyDark = 0x3E8;  // a brightness above this (a negative one) is 0
constexpr int kSwayFrames = 4;       // frames per step of kFireflySway

constexpr int kSpotlightRimAction = 0x4E;  // of the effect act
constexpr int kSpotlightDarkAction = 0x4F;
constexpr int kFramesPerSecond = 0x1E;  // the timers count frames
constexpr int kSecondsPerMinute = 0x3C;
constexpr int kTensBase = 0xA;    // the timer digits are decimal
constexpr int kTimerColon = 0xA;  // the action of the timer act after the ten digits

// The debug text: lines of kDebugLine pixels from kDebugLeft.
constexpr int kDebugLeft = 8;
constexpr int kDebugLine = 0x10;
constexpr u32 kMillisecondsPerSecond = 1000;

constexpr int DebugRow(int row)
{
    return row * kDebugLine;
}

// The bubbles (0x00415EE0).
constexpr int kBubbleMargin = 0x40;     // drawn while the role is this near the camera
constexpr int kBubbleAction = 0x3B;     // the bubble of the effect act
constexpr int kBubbleMarkLimit = 0x18;  // a mark starts bubbles while its count is below this
constexpr int kBubbleSpread = 0x10;     // the start and the drift of a bubble are spread over this many units
constexpr int kWarmLife = 0x10;         // frames: kWarmLife + Below(kWarmLife) on Field::MonasteryOutside
constexpr int kColdLife = 0x1E;         // frames: kColdLife + Below(kColdLife) elsewhere
constexpr int kBubbleAgeChance = 0x32;  // a bubble ages unless the roll (per mille) is at most this
constexpr int kWarmRed = 0x1E;          // the tints subtracted from the life of a warm bubble
constexpr int kWarmGreen = 0x26;
constexpr int kWarmBlue = 0x32;
constexpr int kColdTint = 0x1E;  // subtracted from half the life of the other bubbles
constexpr int kSwayRange = 5;    // the sway of a frame: Below(kSwayRange) - kSwayShift
constexpr int kSwayShift = 3;

// Particle (bubble, lightning, firefly) coordinates are kept in 1/16 pixels.
constexpr int kSubpixels = 0x10;

// swd32.exe 0x004995D4: the tint of a role's shadow by bits 20..23 of its flags.
constexpr std::array<i32, 16> kShadowTint = {-8, -7, -6, -5, -4, -3, -2, 0, 0, 0, -14, -13, -12, -11, -10, -9};
// swd32.exe 0x00499594: the tint added to a role by bits 20..23 of its flags.
constexpr std::array<i32, 16> kRoleTint = {0, 2, 4, 6, 8, 10, 12, 0, 0, 0, -12, -10, -8, -6, -4, -2};
// The action of a leaf by its vertical speed plus 3 (a table on the stack of 0x004161C0).
constexpr std::array<char, 8> kLeafActions = {'4', '4', '4', '6', '8', '8', '8', '6'};
// The sideways sway of a firefly by its phase / 4 (a table on the stack of 0x004167B0).
constexpr std::array<i8, 8> kFireflySway = {0, 1, 1, 1, 0, -1, -1, -1};

// Scene::unk4B7EB4 (the weather kind): kinds 0 and 1 send returning particles up from below the map, 2 and 3 down
// from above it; roles cast shadows under the kinds below kShadowWeathers.
constexpr int kWeatherKindMask = 7;
constexpr int kShadowWeathers = 4;
constexpr int kWeatherFromBelow = 0;
constexpr int kWeatherFromBelowDrifting = 1;
constexpr int kWeatherFromAbove = 2;
constexpr int kWeatherFromAboveDrifting = 3;

constexpr int kAttachedDrop = 0x1C;    // an attached act is drawn this far below its role
constexpr int kLabelGlyphWidth = 0xB;  // a role label is centred by this width per byte
constexpr int kLabelShift = 0x10;
constexpr int kLabelStyle = TextFont::BoldShadow;

// DrawShadow `kind`: bit 0 leans the shadow right, bit 3 squashes it to a quarter; kinds from kFlippedShadow flip it.
constexpr int kShadowLeansRight = 1;
constexpr int kQuarterShadow = 8;
constexpr int kShadowKindMask = 7;
constexpr int kFlippedShadow = 2;
constexpr int kShadowStep = 4;
constexpr int kShadowDrop = 8;
constexpr int kShearShift = 2;  // the shear is a quarter of the height

constexpr int kReflectionAlpha = 4;
constexpr int kReflectionDrop = 8;
constexpr u32 kQuarterChance = kPerMille / 4;
constexpr u32 kTenthChance = kPerMille * 9 / 10;
constexpr int kHeadAcceleration = 3;  // a leaving head triples its speed every frame
constexpr int kSlideShare = 2;        // an arriving head covers two thirds of the way every frame
constexpr int kSlideParts = 3;
constexpr int kPixelsPerPair = 2;  // the blends work on dwords of two pixels
constexpr int kJitterStep = 2;     // random steps come in even sizes
constexpr i16 kMinLineLength = 2;
constexpr u32 kTurnChoices = 3;  // a random turn of -1, 0 or 1
constexpr u32 kHeatSway = 3;

// The heat effect renews one part each period.
constexpr int kHeatNewLengths = 0;
constexpr int kHeatNewOffsets = 1;
constexpr int kHeatNewCounts = 2;

constexpr int kMarkBorder = 2;  // the debug marks leave out the first and last row and column
constexpr i32 kShakeEntry = 2;  // the shake table holds x, y pairs
constexpr u16 kUnkLeader20Limit = 8;
constexpr std::size_t kWhiteColor = 0;  // TextColors::primary
constexpr std::size_t kRoleLineColor = 3;
constexpr int kOverlapColumns = 3;  // roles this near the pointer are counted as overlapping it
constexpr int kOverlapRows = 4;
constexpr int kOverlapRoles = 2;
constexpr int kFastFactor = 2;
constexpr int kTimerX = 0x190;
constexpr int kTimerY = 8;
constexpr int kLayerSwing = 2;
constexpr std::size_t kRowTargetAt = sizeof(u32);  // a movie row: its length and target, then its pixels
constexpr u32 kRowHeaderBytes = 2 * sizeof(u32);
constexpr int kDarkenStep = 2;
constexpr int kCloseRowsPerFrame = 2;

// ScriptObject: two pages of heights, a border cell around the grid.
constexpr int kDefaultDamping = 4;
constexpr std::size_t kPages = 2;
constexpr int kGridBorder = 2;
constexpr int kRefractShift = 3;
constexpr int kAverageShift = 2;

// The insets of the debug cell marks.
constexpr int kRoleCellInset = 2;
constexpr int kRegionInset = 4;
constexpr int kSolidRoleInset = 6;
constexpr int kUnk800000Inset = 8;

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

// The fields of a movie header, little-endian.
MovieHeader MovieHeader::Read(std::span<const u8, MovieHeader::Bytes> bytes)
{
    ByteReader reader(bytes);
    MovieHeader header;
    header.unk00 = reader.Word32();
    header.lastFrame = reader.Word32();
    header.bitsPerPixel = reader.Word16();
    header.unk0A = reader.Word16();
    header.unk0C = reader.Word16();
    header.active = reader.Word16();
    header.height = reader.Word16();
    header.unk12 = reader.Word16();
    header.paletteCount = reader.Word16();
    header.unk16 = reader.Word16();
    header.nextUnk0C = reader.Word32();
    header.nextRows = reader.Word32();
    header.nextSize = reader.Word32();
    return header;
}

u16 MapMovie::Rgb555(u8 red, u8 green, u8 blue)
{
    constexpr int kChannelBits = 5;
    constexpr int kDropped = CHAR_BIT - kChannelBits;
    return static_cast<u16>(
        (red >> kDropped) << (2 * kChannelBits) | (green >> kDropped) << kChannelBits | blue >> kDropped
    );
}

FieldView::FieldView(Game& game)
    : game(game)
{
}

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
    this->DrawTileGrid(TileSet{true});
}

// swd32.exe 0x00413370: 0x00412D30 for the 8-bit tiles; on the grid it falls back to the 16-bit 0x00412BE0
void FieldView::DrawTiles8Scrolled(int)
{
    const Field& field = this->game.field;

    if ((field.cameraLeft & MapHeader::TileFraction) == 0 && (field.cameraTop & MapHeader::TileFraction) == 0)
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
        auto row = static_cast<u32>(this->game.field.cameraTop / MapHeader::TileSize - Field::RowMargin);

        for (int count = 0; count < kScreenRows + 2 * Field::RowMargin; ++count, ++row)
        {
            const std::size_t index = row + Field::RowMargin;

            if (row >= static_cast<u32>(state.map.height + Field::RowMargin) || index >= rows.size())
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
    if ((role.flags & (Role::Visible | Role::Hidden)) != Role::Visible)
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

    if (x - cameraLeft < -kRoleMarginX || x - cameraLeft > Display::Width + kRoleMarginX)
    {
        return 1;
    }

    u32 frame = kNoFrame;

    if (role.act.id != 0)
    {
        frame = role.act.frame;

        if (this->game.state.GameFlag(GameState::HideActRoles))
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

    if ((role.flags & Role::CellEffects) != 0 && (view.unk4B7EB4 & kWeatherKindMask) < kShadowWeathers)
    {
        this->DrawShadow(
            role,
            *picture,
            x - role.act.x - cameraLeft,
            y - role.act.y - cameraTop,
            view.unk4B7EB4,
            kShadowTint[(role.flags & Role::CellShades) >> Role::CellShadeShift]
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

    if ((role.flags & Role::CellEffects) != 0)
    {
        red = view.unk4AB97C;
        green = view.unk4AB994;
        blue = view.unk4ACB98;
    }

    if ((role.flags & Role::CellShades) != 0)
    {
        const int tint = kRoleTint[(role.flags & Role::CellShades) >> Role::CellShadeShift];
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
            (mode & (Painter::RleSource | Painter::Mirror | Painter::Flip)) | Painter::TintUnder,
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
                role.unk2A - attached.y - role.act.y - cameraTop + y + kAttachedDrop,
                extra->width,
                extra->height,
                attached.flags
            );
        }
    }

    role.act.unknown89 = static_cast<u8>(painter.WavePhase());

    if ((role.flags & Role::Bubbles) != 0 && !this->game.state.GameFlag(GameState::PlainScreen))
    {
        this->DrawBubbles(x, y, role.id);
    }

    if (this->game.state.leader.id == Role::NoId && role.label.has_value())
    {
        const auto length = static_cast<int>(role.label->size());
        const auto color = static_cast<Pixel>(this->game.textColors.primary[static_cast<std::size_t>(role.labelColor)]);
        this->game.largeFont.DrawText(
            this->game.drawSurface,
            role.x - kLabelGlyphWidth * length / 2 - cameraLeft + kLabelShift,
            role.y - role.act.y - cameraTop,
            *role.label,
            color,
            kLabelStyle
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
    // 0x00413CA0: the differences and the sum of their squares wrap in 32 bits (sub, imul, add).
    const auto dx = static_cast<u32>(role.x) - static_cast<u32>(player.x);
    const auto dy = static_cast<u32>(role.y) - static_cast<u32>(player.y);
    const auto squared = static_cast<i32>(dx * dx + dy * dy);
    const auto distance = static_cast<int>(crt::Ftol(std::sqrt(static_cast<double>(squared))));
    bool due = true;
    u32 timer = role.soundTimer & Role::SoundPeriodMask;

    if (timer != Role::SoundPlaysOnce)
    {
        const u32 left = (role.soundTimer & Role::SoundCountMask) - 1;

        if (left != 0)
        {
            timer += left;
            due = false;
            role.soundTimer = timer;
        }
    }

    if (distance > kSoundRange || (role.flags & Role::Visible) == 0)
    {
        if ((role.flags & Role::SoundOnce) != 0)
        {
            effects.Stop(role.sound);
            role.flags &= ~Role::SoundOnce;
        }

        return;
    }

    u32 index = 0;
    state.FindRole(role.id, index);

    if ((role.flags & Role::SoundOnce) == 0 && due)
    {
        const u32 reloaded = (role.soundTimer >> Role::SoundPeriodShift) + (role.soundTimer & Role::SoundPeriodMask);
        role.soundTimer = reloaded;

        if ((reloaded & Role::SoundPeriodMask) == Role::SoundPlaysOnce)
        {
            role.flags |= Role::SoundOnce;
        }

        effects.Play(role.sound, 0, 0, 1);

        if (index < view.soundDistance.size())
        {
            view.soundDistance[index] = static_cast<i16>(distance * kSoundScale / kSoundRange);
            const auto rise =
                static_cast<i16>(static_cast<u16>(static_cast<u16>(role.y) - static_cast<u16>(player.y)) * kPanScale);
            view.soundRise[index] = static_cast<i16>(rise / kSoundRange);
        }
    }

    effects.SetVolume(
        role.sound,
        this->game.config.soundVolume * (kSoundScale - distance * kSoundScale / kSoundRange) / Config::MaxVolume
    );
    effects.SetPan(role.sound, (role.x - player.x) * kPanScale / kSoundRange);
}

// swd32.exe 0x004145F0: the shadow of a role: its picture squashed to half its height (a quarter with bit 3 of
// `kind`), sheared to the side of bit 0 and darkened by `tint`; kinds from 2 on flip it over
void FieldView::DrawShadow(const Role& role, const Picture& picture, int x, int y, int kind, int tint)
{
    Painter& painter = this->game.painter;
    DrawOptions& options = painter.Options();
    u32 mode = (role.act.flags & (Painter::RleSource | Painter::Mirror | Painter::Flip)) | Painter::Shadow;
    painter.SetSource(picture.data.data());
    const int half = picture.height >> 1;
    options.scaleHeight = half;
    options.shear = -(picture.height >> kShearShift);

    if ((kind & kShadowLeansRight) != 0)
    {
        options.shear = picture.height >> kShearShift;
    }

    options.tintBlue = tint;
    options.tintGreen = tint;
    options.tintRed = tint;
    int extra = 0;
    int lift = 0;

    if ((kind & kQuarterShadow) != 0)
    {
        options.scaleHeight = half / 2;
        lift = role.act.y / 2;
        extra = kShadowStep;
    }

    int offsetY = 0;

    if ((kind & kShadowKindMask) < kFlippedShadow)
    {
        offsetY = (role.act.y + lift + kShadowDrop) / 2 + extra;
    }
    else
    {
        mode |= Painter::Flip;
        offsetY = extra + role.act.y + kShadowStep;
    }

    const int offsetX = (kind & kShadowLeansRight) == 0 ? kShadowStep : -kShadowStep;
    painter.DrawPicture(x + offsetX, y + offsetY, picture.width, picture.height, mode);
}

// swd32.exe 0x00413EA0: the reflections of the roles of layer 0 standing on reflecting cells (flag 0x20000000), 40
// rows from 5 above the camera
void FieldView::DrawReflections()
{
    const std::vector<Role*>& rows = this->game.field.rows[0];
    int row = this->game.field.cameraTop / MapHeader::TileSize - kReflectionRowsAbove;

    for (int count = -2 * kReflectionRowsAbove; count < kScreenRows; ++count, ++row)
    {
        if (row >= this->game.state.map.height)
        {
            break;
        }

        const auto index = static_cast<std::size_t>(row + Field::RowMargin);

        if (row < 0 || index >= rows.size())
        {
            continue;
        }

        for (const Role* role = rows[index]; role != nullptr; role = role->next)
        {
            if ((role->flags & Role::Reflects) != 0)
            {
                this->DrawReflection(*role);
            }
        }
    }
}

// swd32.exe 0x00413F00: a role's picture flipped below it at translucency 4
int FieldView::DrawReflection(const Role& role)
{
    if ((role.flags & (Role::Visible | Role::Hidden)) != Role::Visible)
    {
        return 1;
    }

    const Field& field = this->game.field;
    const int x = role.x;

    if (x - field.cameraLeft <= -kRoleMarginX || x - field.cameraLeft >= Display::Width + kRoleMarginX)
    {
        return 1;
    }

    const u32 frame = role.act.id != 0 ? role.act.frame : kNoFrame;
    const Picture* picture = this->game.archive.GetPicture(frame, role.act.picture);

    if (picture == nullptr)
    {
        return 1;
    }

    Painter& painter = this->game.painter;
    painter.Options().alpha = kReflectionAlpha;
    painter.SetSource(picture->data.data());
    painter.DrawPicture(
        x + role.unk28 - role.act.x - field.cameraLeft,
        role.unk2A - field.cameraTop + role.y + kReflectionDrop,
        picture->width,
        picture->height,
        (role.act.flags & (Painter::RleSource | Painter::Mirror)) | Painter::Translucent | Painter::Flip
    );
    return 1;
}

// swd32.exe 0x00415EE0: the bubbles of role `role` near the camera: each mark of the role follows it, starts a new
// bubble now and then (at most while its count is below 0x18), and its bubbles rise, drift with the frame's sway and
// fade (warmer on map 0x32); spent ones are dropped by moving the next one into their place
void FieldView::DrawBubbles(int x, int y, u16 role)
{
    const Field& field = this->game.field;

    if (x <= field.cameraLeft - kBubbleMargin || x >= field.cameraRight + kBubbleMargin ||
        y <= field.cameraTop - kBubbleMargin || y >= field.cameraBottom + kBubbleMargin)
    {
        return;
    }

    Game& owner = this->game;
    Random& random = owner.random;
    Scene& view = owner.scene;
    Painter& painter = owner.painter;
    const int sway = static_cast<int>(random.Below(kSwayRange)) - kSwayShift;
    view.unk4ACAF8.id = Scene::EffectAct;
    view.unk4ACAF8.action = kBubbleAction;
    owner.acts.Run(view.unk4ACAF8);
    const Picture* picture = owner.archive.GetPicture(view.unk4ACAF8.frame, view.unk4ACAF8.picture);
    const bool warm = owner.currentMap == Field::MonasteryOutside;

    for (RoleMark& mark : view.marks)
    {
        // 0x00415F7D: the mark's role is sign-extended (movsx) against the zero-extended id, so ids from 0x8000 on
        // never match.
        if (static_cast<i16>(mark.role) != static_cast<int>(role))
        {
            continue;
        }

        u32 index = 0;
        owner.state.FindRoleOrPlayer(role, index);
        mark.flags &= ~RoleMark::Emitting;
        const Role& source = owner.state.roles[std::min<std::size_t>(index, GameState::Roles - 1)];
        mark.x = static_cast<u16>(source.x);
        mark.y = static_cast<u16>(source.y);

        if (random.Below(kPerMille) > kQuarterChance)
        {
            mark.flags |= RoleMark::Emitting;
        }

        if (mark.unk08 < kBubbleMarkLimit && (mark.flags & RoleMark::Emitting) != 0)
        {
            auto bubble = std::make_unique<RoleMarkNode>();
            bubble->next = std::move(mark.nodes);
            bubble->y = static_cast<i16>(mark.y);
            bubble->x = static_cast<i16>(
                kSubpixels * (mark.x - kBubbleSpread / 2 + static_cast<int>(random.Below(kBubbleSpread)))
            );
            bubble->life =
                static_cast<i16>(warm ? random.Below(kWarmLife) + kWarmLife : random.Below(kColdLife) + kColdLife);
            bubble->stepX = static_cast<i16>(static_cast<int>(random.Below(kBubbleSpread)) - kBubbleSpread / 2);
            bubble->stepY = static_cast<i16>(-1 - static_cast<int>(random.Below(1)));
            mark.nodes = std::move(bubble);
        }

        std::unique_ptr<RoleMarkNode>* link = &mark.nodes;

        while (*link != nullptr)
        {
            RoleMarkNode& bubble = **link;
            const u32 roll = random.Below(kPerMille);
            bubble.y = static_cast<i16>(bubble.y + bubble.stepY);
            bubble.x = static_cast<i16>(bubble.x + sway + bubble.stepX);

            if (roll > kBubbleAgeChance)
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
                        options.tintRed = bubble.life - kWarmRed;
                        options.tintGreen = bubble.life - kWarmGreen;
                        options.tintBlue = bubble.life - kWarmBlue;
                    }
                    else
                    {
                        options.tintBlue = (bubble.life >> 1) - kColdTint;
                        options.tintGreen = (bubble.life >> 1) - kColdTint;
                        options.tintRed = (bubble.life >> 1) - kColdTint;
                    }

                    painter.SetSource(picture->data.data());
                    painter.DrawPicture(
                        bubble.x / kSubpixels - view.unk4ACAF8.x - field.cameraLeft,
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
    this->game.painter.TintPixels(std::span<u16>(this->game.drawSurface, Display::Pixels), red, green, blue);
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
        // 0x0041486B, 0x00414898: the place words are zero-extended (xor, mov cx, [esi]).
        const int x = static_cast<u16>(animation.x);
        const int y = static_cast<u16>(animation.y);

        if (owner.acts.Run(animation.act) == 0)
        {
            DebugLog(
                u8"Act Err(PicPaint:PlayAct) 設定錯誤 \nact[%d] qq[%d] dir[%d]",
                animation.act.id,
                animation.act.action,
                animation.act.direction
            );
        }

        const Picture* picture = owner.archive.GetPicture(animation.act.frame, animation.act.picture);

        if (picture != nullptr)
        {
            painter.Options().alpha = animation.act.signal;
            painter.SetSource(picture->data.data());
            painter.DrawPicture(
                x - animation.act.x,
                y - animation.act.y,
                picture->width,
                picture->height,
                animation.act.flags
            );
        }

        if (animation.act.view != 0)
        {
            owner.PlaySoundAt(animation.act.view, owner.field.cameraLeft + x, owner.field.cameraTop + y);
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

        if (owner.acts.Run(sprite.act) == 0)
        {
            DebugLog(
                u8"Act Err(PicPaint:DrawTObj) 設定錯誤 \nact[%d] qq[%d] dir[%d]",
                sprite.act.id,
                sprite.act.action,
                sprite.act.direction
            );
        }

        if (screenX > -kSpriteMargin && screenX < Display::Width + kSpriteMargin && screenY > -kSpriteMargin &&
            screenY < Display::Height + kSpriteMargin)
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

        if (movedX <= sprite.targetX - kSpriteArrival || movedX >= sprite.targetX + kSpriteArrival ||
            movedY <= sprite.targetY - kSpriteArrival || movedY >= sprite.targetY + kSpriteArrival)
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

        if (owner.acts.Run(head.act) == 0)
        {
            DebugLog(
                u8"Act Err(PicPaint:drawRoleHead) 設定錯誤 \nact[%d] qq[%d] dir[%d]",
                head.act.id,
                head.act.action,
                head.act.direction
            );
        }

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

        if ((head.headSpeed & ScriptSprite::HeadSpeedMask) != 0)
        {
            head.headX = static_cast<i16>(head.headX + head.headSpeed);
            head.headSpeed = static_cast<i16>(kHeadAcceleration * head.headSpeed);

            if (head.headX <= -ScriptSprite::HeadMargin || head.headX >= Display::Width + ScriptSprite::HeadMargin)
            {
                *link = std::move(head.next);
                continue;
            }

            link = &head.next;
            continue;
        }

        const int slide = kSlideShare * (head.targetX - head.headX) / kSlideParts;
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
    const std::span<u16> screen(owner.drawSurface, static_cast<std::size_t>(pitch) * Display::Height);

    // 0x00417DE0 is given the pixels of the run and blends half as many pairs (none below one pair, deviations.md).
    auto blend = [&painter, screen](int at, u32 color, int pixels)
    {
        const u32 pairs = static_cast<u32>(pixels) >> 1;

        for (u32 pair = 0; pair < pairs; ++pair)
        {
            const i64 place = at + static_cast<i64>(kPixelsPerPair) * pair;
            const auto first = static_cast<std::size_t>(place);

            if (place < 0)
            {
                continue;
            }

            if (first + 1 >= screen.size())
            {
                break;
            }

            u32 value = MakeWords(screen[first + 1], screen[first]);
            painter.BlendPairsWithColor(std::span<u32>(&value, 1), color);
            screen[first] = LowWord(value);
            screen[first + 1] = HighWord(value);
        }
    };

    std::unique_ptr<ScriptEffect>* link = &owner.scene.effects;

    while (*link != nullptr)
    {
        ScriptEffect& line = **link;
        const u32 color = owner.textColors.primary[static_cast<std::size_t>(line.color) & (TextColors::Colors - 1)];

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

        const int kind = line.mode & ScriptEffect::KindMask;

        if (kind == ScriptEffect::Still)
        {
            for (int row = 0; row < line.count; ++row)
            {
                blend(line.x + pitch * (line.y + row), color, line.limit);
            }

            link = &line.next;
            continue;
        }

        if (kind == ScriptEffect::Falling)
        {
            int done = 0;

            for (int index = line.count - 1; index >= 0; --index)
            {
                i16& length = line.lengths[column(index)];
                length =
                    static_cast<i16>(length + kJitterStep * static_cast<int>(random.Below(kFallJitter)) + kLineStep);

                if (length >= line.limit)
                {
                    ++done;
                    length = line.limit;
                }

                blend(at(index), color, length);
            }

            if (done == line.count)
            {
                line.mode = static_cast<u16>((line.mode & ScriptEffect::NumberMask) | ScriptEffect::Still);
            }
        }

        if ((line.mode & ScriptEffect::KindMask) == ScriptEffect::Rising)
        {
            int done = 0;

            for (int index = line.count - 1; index >= 0; --index)
            {
                i16& offset = line.offsets[column(index)];
                offset =
                    static_cast<i16>(offset - kLineStep - kJitterStep * static_cast<int>(random.Below(kRiseJitter)));

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
                line.mode = static_cast<u16>((line.mode & ScriptEffect::NumberMask) | ScriptEffect::Still);
            }
        }

        bool finished = false;

        if ((line.mode & ScriptEffect::KindMask) == ScriptEffect::Opening)
        {
            int done = 0;

            for (int index = line.count - 1; index >= 0; --index)
            {
                i16& offset = line.offsets[column(index)];
                offset =
                    static_cast<i16>(offset + kJitterStep * static_cast<int>(random.Below(kOpenJitter)) + kLineStep);

                if (offset >= line.limit)
                {
                    ++done;
                    offset = line.limit;
                }

                i16& length = line.lengths[column(index)];
                length = static_cast<i16>(line.limit - offset);

                if (length < kMinLineLength)
                {
                    length = kMinLineLength;
                }

                blend(at(index), color, length);
            }

            finished = done == line.count;
        }

        if (!finished && (line.mode & ScriptEffect::KindMask) == ScriptEffect::Closing)
        {
            int done = 0;

            for (int index = line.count - 1; index >= 0; --index)
            {
                i16& length = line.lengths[column(index)];
                length =
                    static_cast<i16>(length - kLineStep - kJitterStep * static_cast<int>(random.Below(kOpenJitter)));

                if (length <= kMinLineLength)
                {
                    ++done;
                    length = kMinLineLength;
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
            painter.SetClipClamped(0, 0, Display::Width, Display::Height);
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

    if (!state.GameFlag(GameState::Weather))
    {
        return;
    }

    Scene& view = owner.scene;
    Random& random = owner.random;
    Painter& painter = owner.painter;
    const Field& field = owner.field;

    for (std::size_t particle = 0; particle < kWeatherParticles; ++particle)
    {
        const auto roll = static_cast<int>(random.Below(kPerMille));
        const int width = state.map.width;
        MapParticle& drop = view.weather[particle];
        WeatherShade& shade = view.weatherShades[particle];
        WeatherMotion& motion = view.weatherMotion[particle];

        if (drop.x <= -kWeatherMarginX || drop.x >= MapHeader::TileSize * width + kWeatherMarginX ||
            drop.y <= -kWeatherMarginY || drop.y >= MapHeader::TileSize * state.map.height + kWeatherMarginY)
        {
            if (roll < kWeatherReturnRoll)
            {
                continue;
            }

            shade.tintTarget = 0;
            motion.speedTarget = roll / kWeatherSpeedStep + 1;
            motion.action = view.weatherAction + roll * view.weatherActions / kPerMille;

            switch (view.unk4B7EB4)
            {
                case kWeatherFromBelow:
                    drop.x = static_cast<i32>(random.Below(static_cast<u32>(MapHeader::TileSize * width)));
                    drop.y = MapHeader::TileSize * state.map.height + kWeatherBelow;
                    drop.stepX = static_cast<i32>(random.Below(1)) - kWeatherDrift;
                    drop.stepY = static_cast<i32>(random.Below(1)) - kWeatherDrift;
                    break;

                case kWeatherFromBelowDrifting:
                    drop.x = static_cast<i32>(random.Below(static_cast<u32>(MapHeader::TileSize * width)));
                    drop.y = MapHeader::TileSize * state.map.height + kWeatherBelow;
                    drop.stepX = static_cast<i32>(random.Below(kWeatherDrift));
                    drop.stepY = static_cast<i32>(random.Below(kWeatherDrift));
                    break;

                case kWeatherFromAbove:
                    drop.x = static_cast<i32>(random.Below(static_cast<u32>(MapHeader::TileSize * width)));
                    drop.y = -kWeatherAbove;
                    drop.stepX = static_cast<i32>(random.Below(kWeatherDrift)) - kWeatherDrift;
                    drop.stepY = static_cast<i32>(random.Below(kWeatherDrift)) - kWeatherDrift;
                    break;

                case kWeatherFromAboveDrifting:
                    drop.x = static_cast<i32>(random.Below(static_cast<u32>(MapHeader::TileSize * width)));
                    drop.y = -kWeatherAbove;
                    drop.stepX = static_cast<i32>(random.Below(kWeatherDrift));
                    drop.stepY = static_cast<i32>(random.Below(kWeatherDrift));
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
            drop.x += drop.stepX + ((owner.unk4B7518 * owner.state.walkSpeed) >> kWalkQuarter);
            drop.y += drop.stepY + ((owner.state.walkSpeed * owner.unk4B7920) >> kWalkQuarter);

            if (roll < kWeatherSpeedChance)
            {
                motion.speedTarget = static_cast<i32>(random.Below(kWeatherSpeeds)) + 1;
            }

            if (motion.speed != motion.speedTarget)
            {
                motion.speed += motion.speed <= motion.speedTarget ? 1 : -1;
            }

            if (roll < kWeatherTintChance)
            {
                shade.tintTarget = static_cast<i32>(random.Below(kWeatherTints)) - kWeatherDarkest;
            }

            if (shade.tint != shade.tintTarget)
            {
                shade.tint += shade.tint <= shade.tintTarget ? 1 : -1;
            }
        }

        view.unk4ACAF8.id = Scene::EffectAct;
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
            view.weatherActions == Scene::GlowWeatherActions ? Painter::Additive : Painter::Subtractive
        );
    }
}

// swd32.exe 0x004161C0: the leaves (story flag 6): four leaves blow across the map, entering again from a side once
// they leave it and now and then changing speed; the action follows the vertical speed
void FieldView::DrawLeaves()
{
    Game& owner = this->game;
    GameState& state = owner.state;

    if (!state.GameFlag(GameState::Leaves))
    {
        return;
    }

    Scene& view = owner.scene;
    Random& random = owner.random;
    Painter& painter = owner.painter;
    const Field& field = owner.field;
    const int width = MapHeader::TileSize * state.map.width;
    const int height = MapHeader::TileSize * state.map.height;

    for (std::size_t index = 0; index < view.leaves.size(); ++index)
    {
        MapParticle& leaf = view.leaves[index];
        ActState& act = view.unk4AD0D8[index];

        if (leaf.x <= -kLeafMargin || leaf.x >= width + kLeafMargin || leaf.y <= -kLeafMargin ||
            leaf.y >= height + kLeafMargin)
        {
            leaf.x = MapParticle::Unplaced;
        }

        if (leaf.x == MapParticle::Unplaced)
        {
            leaf.x = -kLeafEntry;
            leaf.y = static_cast<i32>(random.Below(static_cast<u32>(height)));
            leaf.stepY = static_cast<i32>(random.Below(2 * kLeafStartDown + 1)) - kLeafStartDown;
            leaf.stepX = static_cast<i32>(random.Below(2 * kLeafStartSide + 1)) - kLeafStartSide;

            if (leaf.stepX == 0)
            {
                leaf.stepX = kLeafZeroSide;
            }

            if (leaf.stepX < 0)
            {
                leaf.x = width + kLeafEntry;
            }
        }
        else if (random.Below(kPerMille) > kQuarterChance)
        {
            const int turn = static_cast<int>(random.Below(kTurnChoices)) - 1;
            leaf.stepY =
                std::clamp(static_cast<int>(random.Below(kTurnChoices)) - 1 + leaf.stepY, -kLeafMaxDown, kLeafMaxDown);
            leaf.stepX = std::clamp(turn + leaf.stepX, -kLeafMaxSide, kLeafMaxSide);

            if (leaf.stepX == 0)
            {
                leaf.stepX = -turn;
            }
        }

        act.action = kLeafActions[static_cast<std::size_t>(
            std::clamp(leaf.stepY + kLeafMaxDown, 0, static_cast<int>(kLeafActions.size()) - 1)
        )];

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

    if (!owner.state.GameFlag(GameState::HeatHaze))
    {
        return;
    }

    Scene& view = owner.scene;
    Random& random = owner.random;

    auto newLength = [&random]()
    {
        return static_cast<i16>(kJitterStep * random.Below(kHeatLengthJitter) + kHeatLengthMin);
    };

    auto newOffset = [&random](int column, int line)
    {
        return line + static_cast<i32>(random.Below(kHeatSway)) + column + Display::Width - 1;
    };

    if (view.heatFrame != 0)
    {
        if (view.heatFrame % kHeatPeriod == 0)
        {
            switch (view.heatFrame / kHeatPeriod % kHeatPeriod)
            {
                case kHeatNewLengths:
                    for (i16& length : view.heatLengths)
                    {
                        length = newLength();
                    }

                    break;

                case kHeatNewOffsets:
                {
                    std::size_t index = 0;

                    for (int column = 0; column < Display::Width; column += kHeatStrip)
                    {
                        for (int line = 0; line < Display::Pixels; line += kHeatStrip * Display::Width)
                        {
                            view.heatOffsets[index++] = newOffset(column, line);
                        }
                    }

                    break;
                }

                case kHeatNewCounts:
                    for (i16& count : view.heatCounts)
                    {
                        count = static_cast<i16>(random.Below(kHeatLines) + 1);
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

        for (int column = 0; column < Display::Width; column += kHeatStrip)
        {
            for (int line = 0; line < Display::Pixels; line += kHeatStrip * Display::Width)
            {
                view.heatLengths[index] = newLength();
                view.heatOffsets[index] = newOffset(column, line);
                view.heatCounts[index] = static_cast<i16>(random.Below(kHeatLines) + 1);
                ++index;
            }
        }
    }

    ++view.heatFrame;
    auto* bytes = reinterpret_cast<u8*>(owner.drawSurface);
    constexpr std::size_t kScreenBytes = sizeof(Pixel) * Display::Pixels;
    constexpr int kLineBytes = static_cast<int>(sizeof(Pixel)) * Display::Width;

    for (std::size_t index = 0; index < view.heatOffsets.size(); ++index)
    {
        const int count = view.heatCounts[index];
        const auto length = static_cast<u16>(view.heatLengths[index]);
        int at = static_cast<int>(sizeof(Pixel)) * view.heatOffsets[index];

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

    if (random.Below(kPerMille) > kTenthChance)
    {
        if (owner.state.GameFlag(GameState::Lightning))
        {
            if (++view.lightningCount > kMaxBolts)
            {
                view.lightningCount = kMaxBolts;
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
    // 0x00416610: the end of the screen is fixed at 0x96000 bytes, whatever the pitch.
    constexpr auto pixels = static_cast<std::ptrdiff_t>(Display::Pixels);
    view.lightningShown = 0;
    int started = 0;

    for (Streak& bolt : view.bolts)
    {
        if ((bolt.active & 1) == 0)
        {
            if (started < view.lightningCount)
            {
                bolt.x = static_cast<i16>(kSubpixels * random.Below(Display::Width));
                bolt.y = 0;
                const auto slope = static_cast<i16>(random.Below(kSlopes));
                bolt.slope = bolt.x <= Display::Width / 2 * kSubpixels ? slope : static_cast<i16>(-slope);
                bolt.speed = static_cast<i16>(random.Below(kBoltSpeedJitter) + kBoltSpeedMin);
                bolt.length = static_cast<i16>(random.Below(kBoltLengthJitter) + kBoltLengthMin);
                bolt.life = bolt.speed < kBoltSpeedMin ? kShortLife : kBrightLife;
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
                    const std::ptrdiff_t at = row + x / kSubpixels;

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

            if (bolt.life == kBrightLife)
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

    if (random.Below(kPerMille) > kTenthChance)
    {
        if (owner.state.GameFlag(GameState::Fireflies))
        {
            if (++view.fireflyCount > kMaxFireflies)
            {
                view.fireflyCount = kMaxFireflies;
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
    const auto pixels = static_cast<std::ptrdiff_t>(pitch) * Display::Height;
    const std::ptrdiff_t firstLine = Display::Width;
    const std::ptrdiff_t lastLine = Display::Pixels - Display::Width;
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
                fly.x = static_cast<i16>(kSubpixels * random.Below(Display::Width));
                fly.y = 0;
                const auto slope = static_cast<i16>(random.Below(kSlopes));
                fly.slope = fly.x <= Display::Width / 2 * kSubpixels ? slope : static_cast<i16>(-slope);
                fly.speed = static_cast<i16>(random.Below(kFireflySpeeds) + 1);
                fly.length = 1;
                fly.life = static_cast<i16>(kFireflyLife * fly.speed - static_cast<int>(random.Below(kFireflyLife)));
                fly.phase = 0;
                fly.active = 1;
                ++started;
            }

            continue;
        }

        auto level = static_cast<u32>(kFireflyGlow - (kFireflySpeeds * kFireflyLife - fly.life) / kFireflyFade);

        if (level > kFireflyDark)
        {
            level = 0;
        }

        int x = fly.x;
        const int sway = kFireflySway[static_cast<std::size_t>(fly.phase / kSwayFrames) % kFireflySway.size()];
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
                    const std::ptrdiff_t center = row + x / kSubpixels + sway;
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

            if (fly.life == kBrightLife)
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

        if (++fly.phase > kSwayFrames * (static_cast<int>(kFireflySway.size()) - 1))
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

    if (!owner.state.GameFlag(GameState::Spotlight))
    {
        return;
    }

    Scene& view = owner.scene;
    Painter& painter = owner.painter;
    ActState& act = view.unk4AD3F0;
    act.id = Scene::EffectAct;
    act.action = kSpotlightRimAction;
    owner.acts.Run(act);
    const Picture* rim = owner.archive.GetPicture(act.frame, act.picture);

    if (rim != nullptr)
    {
        painter.SetClipClamped(
            view.spotlightX - (rim->width >> 1),
            view.spotlightY - (rim->width >> 1),
            view.spotlightX + (rim->height >> 1),
            (rim->height >> 1) + view.spotlightY
        );
        painter.SetSource(rim->data.data());
        painter.DrawPicture(
            view.spotlightX - (rim->width >> 1),
            view.spotlightY - (rim->height >> 1),
            rim->width,
            rim->height,
            0
        );
    }

    act.action = kSpotlightDarkAction;
    owner.acts.Run(act);
    const Picture* dark = owner.archive.GetPicture(act.frame, act.picture);
    painter.SetClipClamped(
        view.spotlightX - Scene::SpotlightRadius,
        view.spotlightY - Scene::SpotlightRadius,
        view.spotlightX + Scene::SpotlightRadius,
        view.spotlightY + Scene::SpotlightRadius
    );

    if (dark != nullptr)
    {
        painter.SetSource(dark->data.data());
        painter.DrawPicture(
            view.spotlightX - (dark->width >> 1),
            view.spotlightY - (dark->height >> 1),
            dark->width,
            dark->height,
            Painter::Subtractive
        );
    }

    if (view.spotlightX != view.spotlightTargetX || view.spotlightY != view.spotlightTargetY)
    {
        view.spotlightX += view.spotlightStepX;
        view.spotlightY += view.spotlightStepY;

        if (view.spotlightX == view.spotlightTargetX)
        {
            view.spotlightStepX = 0;
        }

        if (view.spotlightY == view.spotlightTargetY)
        {
            view.spotlightStepY = 0;
        }
    }
}

// swd32.exe 0x00416CC0: the ripples: each copies the screen and refracts it through its waves; settled ones are freed
void FieldView::DrawRipples()
{
    Game& owner = this->game;
    const std::span<u16> screen(
        owner.drawSurface,
        static_cast<std::size_t>(owner.painter.ScreenPitch()) * Display::Height
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

    if (which == 0 && !state.GameFlag(GameState::Timer))
    {
        return;
    }

    if (which == 1 && !state.GameFlag(GameState::SecondTimer))
    {
        return;
    }

    if (state.GameFlag(GameState::HideTimer))
    {
        return;
    }

    Scene& view = owner.scene;
    Painter& painter = owner.painter;
    int seconds = (which != 0 ? view.unk4A9918 : view.unk4CAD20) / kFramesPerSecond;

    if (seconds < 0)
    {
        seconds = 0;
    }

    ActState& digits = view.unk4A9A30;
    digits.id = Scene::TimerAct;
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
            painter.DrawPicture(at, y, picture->width, picture->height, Painter::Copy);
            at += picture->width;
        }
    };

    if (seconds / (kTensBase * kSecondsPerMinute) != 0)
    {
        draw(seconds / (kTensBase * kSecondsPerMinute));
    }

    draw(seconds / kSecondsPerMinute % kTensBase);
    draw(kTimerColon);
    const int rest = seconds % kSecondsPerMinute;
    draw(rest / kTensBase);
    draw(rest % kTensBase);
}

// swd32.exe 0x00430180: the outline of a cell mark: the top and bottom lines as `width / 2` pixel pairs, the sides
// `height` pixels long
void FieldView::FillCellMark(int x, int y, int width, int height, int color)
{
    Game& owner = this->game;
    const int pitch = owner.painter.ScreenPitch();
    const auto pixels = static_cast<std::ptrdiff_t>(pitch) * Display::Height;
    const auto value = static_cast<u16>(color);
    // The lines are written as pixel pairs of (colour << 16) + colour (0x0043019B..0x0043019E): the whole colour is
    // added, so bits above 16 change the second pixel.
    const u32 pairValue = (static_cast<u32>(value) << (CHAR_BIT * sizeof(u16))) + static_cast<u32>(color);
    const auto second = static_cast<u16>(pairValue >> (CHAR_BIT * sizeof(u16)));

    auto put = [&owner, pixels](std::ptrdiff_t at, u16 pixel)
    {
        if (at >= 0 && at < pixels)
        {
            owner.drawSurface[at] = pixel;
        }
    };

    const int pairs = width >> 1;  // sar (0x004301CF)

    for (int pair = 0; pair < pairs; ++pair)
    {
        const std::ptrdiff_t top = x + static_cast<std::ptrdiff_t>(pitch) * y + kPixelsPerPair * pair;
        const std::ptrdiff_t bottom = x + static_cast<std::ptrdiff_t>(pitch) * (y + height) + kPixelsPerPair * pair;
        put(top, value);
        put(top + 1, second);
        put(bottom, value);
        put(bottom + 1, second);
    }

    for (int line = 0; line < height; ++line)
    {
        const std::ptrdiff_t left = x + static_cast<std::ptrdiff_t>(pitch) * (y + line);
        put(left, value);
        put(left + kPixelsPerPair * pairs, value);
    }
}

// swd32.exe 0x00430230: the debug marks of the cells under the camera holding any bit of `mask` (38 x 28 cells from
// one past the camera), inset by `inset` pixels
void FieldView::MarkCells(u32 mask, int color, int offsetX, int offsetY, int inset)
{
    const Field& field = this->game.field;
    const int width = this->game.state.map.width;
    const int shiftY = offsetY & MapHeader::TileFraction;
    const int shiftX = -(offsetX & MapHeader::TileFraction);
    int cell = ((field.cameraLeft + MapHeader::TileSize) >> MapHeader::TileShift) +
               width * ((field.cameraTop + MapHeader::TileSize) >> MapHeader::TileShift);
    int y = inset - shiftY + MapHeader::TileSize;

    for (int row = 0; row < kScreenRows - kMarkBorder; ++row)
    {
        for (int x = MapHeader::TileSize; x < Display::Width - MapHeader::TileSize - 1; x += MapHeader::TileSize)
        {
            const std::vector<u32>& blocks = this->game.state.map.blocks;
            const u32 value = cell >= 0 && static_cast<std::size_t>(cell) < blocks.size()
                                  ? blocks[static_cast<std::size_t>(cell)]
                                  : 0;

            if ((mask & value) != 0)
            {
                this->FillCellMark(
                    inset + x + shiftX,
                    y,
                    MapHeader::TileFraction - inset,
                    MapHeader::TileFraction - inset,
                    color
                );
            }

            ++cell;
        }

        y += MapHeader::TileSize;
        cell += width - (kScreenColumns - kMarkBorder);
    }
}

// swd32.exe 0x00414570: the camera pans by the step of the pan still to go (0x004A99F0 across, 0x004A98C0 down); a step
// is cleared once its distance is covered
void FieldView::PanCamera()
{
    Field& field = this->game.field;
    Scene& view = this->game.scene;

    if (view.panAcross == 0 && view.panDown == 0)
    {
        return;
    }

    field.cameraLeft += view.panStepX;
    field.cameraRight += view.panStepX;
    field.cameraTop += view.panStepY;
    const int remaining = view.panDown - view.panStepY;
    field.cameraBottom += view.panStepY;
    view.panAcross -= view.panStepX;
    view.panDown -= view.panStepY;

    if (view.panAcross == 0)
    {
        view.panStepX = 0;
    }

    if (remaining == 0)
    {
        view.panStepY = 0;
    }
}

// swd32.exe 0x004148F0: the quake: the camera is offset by the pairs of the shake table (0x004ACE70, ended by 0xCFCF)
// in turn, each held for 0x004AD0D0 frames (not on map 0x16); the camera before it is kept to be restored
void FieldView::ShakeCamera()
{
    Field& field = this->game.field;
    Scene& view = this->game.scene;

    if (view.shake[0] == Scene::ShakeEnd || this->game.currentMap == Field::DemonRefiningPot)
    {
        return;
    }

    auto at = static_cast<std::size_t>(view.shakeIndex);

    if (at >= view.shake.size() || view.shake[at] == Scene::ShakeEnd)
    {
        at = 0;
        view.shakeIndex = 0;
    }

    const int dx = view.shake[at];
    const int dy = at + 1 < view.shake.size() ? view.shake[at + 1] : 0;

    if (--view.shakeFrames <= 0)
    {
        view.shakeFrames = view.shakeHold;
        view.shakeIndex = static_cast<i32>(at) + kShakeEntry;
    }

    view.unk4A93E4 = field.cameraLeft;
    field.cameraLeft += dx;
    view.unk4A992C = field.cameraTop;
    field.cameraTop += dy;
    field.cameraRight += dx;
    field.cameraBottom += dy;
}

// swd32.exe 0x004157D0: the rows of the movie frame, centred vertically: 8-bit rows through the palette, 16-bit rows
// converted to the display in the frame itself (so a frame drawn again is converted again) and copied
void FieldView::DrawMovieFrame()
{
    Game& owner = this->game;
    MapMovie& film = this->movie;
    const int pitch = owner.painter.ScreenPitch();
    auto* screen = reinterpret_cast<u8*>(owner.drawSurface);
    const std::size_t screenBytes = sizeof(Pixel) * static_cast<std::size_t>(pitch) * Display::Height;
    const std::ptrdiff_t base =
        static_cast<std::ptrdiff_t>(sizeof(Pixel)) * pitch * ((Display::Height - film.height) >> 1);
    const u32 rows = film.current != nullptr ? film.current->rows : 0;
    std::size_t at = 0;

    auto dword = [&film](std::size_t offset) -> u32
    {
        return ByteReader::Word32At(film.frame, offset);
    };

    for (u32 row = 0; row < rows && static_cast<i32>(row) >= 0; ++row)
    {
        const u32 length = dword(at);
        const std::ptrdiff_t target = base + static_cast<std::ptrdiff_t>(dword(at + kRowTargetAt));

        if (length < kRowHeaderBytes || at + length > film.frame.size())
        {
            break;
        }

        const std::size_t count = length - kRowHeaderBytes;
        u8* pixels = film.frame.data() + at + kRowHeaderBytes;

        if (!film.palette.empty())
        {
            for (std::size_t index = 0; index < count; ++index)
            {
                const std::ptrdiff_t out = target + static_cast<std::ptrdiff_t>(sizeof(Pixel) * index);

                if (out >= 0 && static_cast<std::size_t>(out) + sizeof(Pixel) <= screenBytes)
                {
                    const u16 color = film.palette[pixels[index]];
                    std::memcpy(screen + out, &color, sizeof(Pixel));
                }
            }
        }
        else
        {
            std::vector<u16> converted(count / sizeof(Pixel));
            std::memcpy(converted.data(), pixels, converted.size() * sizeof(Pixel));
            owner.painter.ConvertPixels(converted);
            std::memcpy(pixels, converted.data(), converted.size() * sizeof(Pixel));

            if (target >= 0 && static_cast<std::size_t>(target) + count <= screenBytes)
            {
                std::memcpy(screen + target, pixels, count);
            }
        }

        at += length;
    }
}

// swd32.exe 0x004158C0: the chunk of the current frame: from the chunks read before, or read on from the last one
// (keeping every chunk; a palette due at this frame read from the file's end); past the last frame the screen is kept
// as the frame and 1 returned
int FieldView::ReadMovieFrame()
{
    Game& owner = this->game;
    MapMovie& film = this->movie;

    auto unpack = [&film](u32 bytes)
    {
        film.buffer.resize(std::max<std::size_t>(film.buffer.size(), bytes));
        MapCache::Unpack(std::span<const u8>(film.buffer.data(), bytes), film.frame);
    };

    MovieChunk* previous = &film.chunks;

    for (MovieChunk* chunk = film.chunks.next.get(); chunk != nullptr; chunk = chunk->next.get())
    {
        if (chunk->frame == film.frameIndex)
        {
            const u32 bytes = chunk->size;
            film.file.SeekBegin(static_cast<i32>(chunk->offset + MapMovie::FileHeaderBytes));

            if (bytes != 0)
            {
                owner.audioDevice.Serve();
                u32 read = bytes;
                film.buffer.resize(std::max<std::size_t>(film.buffer.size(), bytes));
                film.file.Read(film.buffer.data(), read);
                unpack(bytes);
            }

            film.current = chunk;
            return 0;
        }

        previous = chunk;
    }

    u32 offset = previous->offset + previous->size;
    i32 frame = previous->frame + 1;
    film.file.SeekBegin(static_cast<i32>(offset + MapMovie::FileHeaderBytes));
    MovieChunk* chunk = nullptr;

    for (;;)
    {
        if (frame > film.lastFrame)
        {
            const std::size_t bytes = std::min<std::size_t>(film.frame.size(), sizeof(Pixel) * Display::Pixels);
            std::memcpy(film.frame.data(), owner.drawSurface, bytes);
            return 1;
        }

        auto fresh = std::make_unique<MovieChunk>();
        fresh->offset = offset;
        fresh->unk0C = previous->nextUnk0C;
        fresh->rows = previous->nextRows;
        fresh->size = previous->nextSize;
        fresh->frame = frame;
        owner.audioDevice.Serve();
        u32 read = fresh->size;
        film.buffer.resize(std::max<std::size_t>(film.buffer.size(), fresh->size));
        film.file.Read(film.buffer.data(), read);

        if (film.paletteCount != 0 && film.paletteIndex < film.paletteFrames.size() &&
            film.frameIndex == film.paletteFrames[film.paletteIndex])
        {
            owner.audioDevice.Serve();
            const auto back = static_cast<i32>(
                MapMovie::PaletteBytes * (film.paletteCount - film.paletteIndex++) + MapMovie::PaletteTailBytes
            );
            std::array<u8, MapMovie::PaletteBytes> colors = {};
            film.file.SeekEnd(-back);
            u32 bytes = MapMovie::PaletteBytes;
            film.file.Read(colors.data(), bytes);
            film.palette.assign(MapMovie::PaletteColors, 0);

            for (std::size_t index = 0; index < MapMovie::PaletteColors; ++index)
            {
                const u8* color = colors.data() + MapMovie::PaletteBytes / MapMovie::PaletteColors * index;
                film.palette[index] =
                    MapMovie::Rgb555(color[MapMovie::RedAt], color[MapMovie::GreenAt], color[MapMovie::BlueAt]);
            }

            owner.painter.ConvertPixels(film.palette);
        }

        auto tail = [&film, &fresh](u32 back) -> u32
        {
            const std::size_t at = static_cast<std::size_t>(fresh->size) - back;
            return fresh->size >= back ? ByteReader::Word32At(film.buffer, at) : 0;
        };

        fresh->nextUnk0C = tail(MovieChunk::TrailerBytes);
        fresh->nextRows = tail(MovieChunk::TrailerBytes - sizeof(u32));
        fresh->nextSize = tail(sizeof(u32));
        offset += fresh->size;
        chunk = fresh.get();
        previous->next = std::move(fresh);
        previous = chunk;

        if (frame == film.frameIndex)
        {
            break;
        }

        ++frame;
    }

    const u32 packed = chunk->size - MovieChunk::TrailerBytes;

    if (packed != 0)
    {
        unpack(packed);
    }

    film.current = chunk;
    return 0;
}

// swd32.exe 0x004154A0: the movie over the map: while talk windows, lines or role heads show (or it is paused) the
// screen is kept as it was; it fades in with black bars closing, plays a frame a call, and fades out (darkening, or the
// map coming back between closing bars); at the end everything is freed and the frame rate restored. 1 while fading
// out.
int FieldView::UpdateMovie()
{
    Game& owner = this->game;
    MapMovie& film = this->movie;
    const i32 active = film.active;

    if (active == 0)
    {
        return 0;
    }

    auto* screen = reinterpret_cast<u8*>(owner.drawSurface);
    constexpr std::size_t kScreenBytes = sizeof(Pixel) * Display::Pixels;

    if (owner.scene.talkWindows != nullptr || owner.scene.effects != nullptr || owner.scene.spritesB != nullptr ||
        (film.options & MapMovie::Paused) != 0)
    {
        film.buffer.resize(std::max(film.buffer.size(), kScreenBytes));

        if (film.saved == 0)
        {
            std::memcpy(film.buffer.data(), screen, kScreenBytes);
            film.saved = 1;
            return 0;
        }

        std::memcpy(screen, film.buffer.data(), kScreenBytes);
        return 0;
    }

    if (film.saved == 1)
    {
        std::memcpy(screen, film.buffer.data(), std::min(film.buffer.size(), kScreenBytes));
        film.saved = 0;
        return 0;
    }

    const int pitch = owner.painter.ScreenPitch();

    if (film.frameIndex <= 0)
    {
        this->DrawMovieFrame();
        const int bar = MapMovie::BarStep * (film.frameIndex - MapMovie::FadeInStart);
        const auto bytes = static_cast<std::size_t>(std::max(bar, 0)) * sizeof(Pixel) * static_cast<std::size_t>(pitch);
        std::memset(screen, 0, std::min(bytes, kScreenBytes));

        if (bytes <= kScreenBytes)
        {
            std::memset(
                screen +
                    sizeof(Pixel) * static_cast<std::size_t>(pitch) * static_cast<std::size_t>(Display::Height - bar),
                0,
                bytes
            );
        }

        ++film.frameIndex;
        return 0;
    }

    if (film.frameIndex >= MapMovie::FadeOutStart)
    {
        if ((film.options & MapMovie::DarkenOut) != 0)
        {
            const int level = kDarkenStep * (MapMovie::FadeOutStart - film.frameIndex);
            owner.painter.TintPixels(std::span<u16>(owner.drawSurface, Display::Pixels), level, level, level);
        }
        else
        {
            film.active = 0;
            owner.scene.screenFlags &= ~Scene::BlackScreen;
            this->Paint();
            owner.scene.screenFlags |= Scene::BlackScreen;
            film.active = active;
            const auto bytes = static_cast<std::size_t>(std::max(
                0,
                kCloseRowsPerFrame * static_cast<int>(sizeof(Pixel)) * pitch *
                    (MapMovie::FadeOutStart + MapMovie::CloseFrames - film.frameIndex)
            ));
            std::memset(screen, 0, std::min(bytes, kScreenBytes));

            if (bytes <= kScreenBytes)
            {
                std::memset(screen + kScreenBytes - bytes, 0, bytes);
            }
        }

        ++film.frameIndex;
        const bool ended = (film.options & MapMovie::DarkenOut) != 0
                               ? film.frameIndex > MapMovie::FadeOutStart + MapMovie::DarkenFrames
                               : film.frameIndex > MapMovie::FadeOutStart + MapMovie::CloseFrames;

        if (ended)
        {
            film.chunks.next.reset();
            film.frame.clear();
            film.buffer.clear();
            film.palette.clear();
            film.paletteFrames.clear();
            film.chunks.offset = 0;
            film.chunks.size = 0;
            film.chunks.nextSize = 0;
            film.chunks.frame = 0;
            film.current = &film.chunks;
            film.height = 0;
            film.active = 0;
            film.frameIndex = 0;
            film.file.Close();
            owner.systemFlags &= ~Game::MovieOpened;
            owner.state.SetFrameInterval(Game::FrameInterval);
        }

        return 1;
    }

    if (this->ReadMovieFrame() != 0)
    {
        film.frameIndex = MapMovie::FadeOutStart;
    }
    else
    {
        this->DrawMovieFrame();
        ++film.frameIndex;
    }

    return 0;
}

// swd32.exe 0x00412930: the frame of the map: the movie, or the overlays, the tiles (8-bit maps through their palette),
// reflections, roles, animations, sprites and the screen effects (all but the spotlight left out with story flag
// 0x48); then the lines, the talk windows, the role heads, the gold, the pointer, the fade and the notices
void FieldView::Paint()
{
    Game& owner = this->game;
    GameState& state = owner.state;
    Painter& painter = owner.painter;
    Scene& view = owner.scene;
    const Field& field = owner.field;

    auto lightClip = [&painter, &view]()
    {
        painter.SetClipClamped(
            view.spotlightX - Scene::SpotlightRadius,
            view.spotlightY - Scene::SpotlightRadius,
            view.spotlightX + Scene::SpotlightRadius,
            view.spotlightY + Scene::SpotlightRadius
        );
    };

    painter.SetClipClamped(0, 0, Display::Width, Display::Height);

    if ((view.screenFlags & Scene::SpotlightClip) != 0)
    {
        lightClip();
    }

    if (this->movie.active != 0)
    {
        this->UpdateMovie();
    }
    else
    {
        bool later = true;

        if (state.GameFlag(GameState::OverlaysFirst) || state.GameFlag(GameState::Spotlight))
        {
            std::memset(owner.drawSurface, 0, sizeof(Pixel) * Display::Pixels);
        }

        if ((view.screenFlags & Scene::BlackScreen) != 0)
        {
            std::memset(owner.drawSurface, 0, sizeof(Pixel) * Display::Pixels);
        }
        else
        {
            this->DrawOverlays();

            if (!state.GameFlag(GameState::PlainScreen) && state.GameFlag(GameState::Spotlight))
            {
                lightClip();
            }

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

            if (!state.GameFlag(GameState::HideActRoles))
            {
                this->DrawReflections();
            }

            this->DrawRoles();
            this->DrawAnimations(view.animationsA);
            this->DrawSprites();

            if (!state.GameFlag(GameState::PlainScreen) && (view.screenFlags & Scene::BlackScreen) == 0)
            {
                this->DrawLeaves();
                this->DrawLightning();
                this->DrawFireflies();
                this->DrawWeather();
                this->DrawHeat();
                this->DrawRipples();
            }

            if (!state.GameFlag(GameState::PlainScreen))
            {
                this->DrawSpotlight();
                this->DrawAnimations(view.animationsB);
                later = false;
            }
        }

        if (later)
        {
            this->DrawAnimations(view.animationsB);
        }
    }

    painter.SetClipClamped(0, 0, Display::Width, Display::Height);
    this->DrawLines();
    owner.talk.Draw();
    this->DrawRoleHeads();

    if ((state.leader.id == Role::NoId || state.leader.unk20 < kUnkLeader20Limit) &&
        !state.GameFlag(GameState::HideHud))
    {
        owner.actDrawer.DrawCounter(
            ActDrawer::GoldX,
            ActDrawer::GoldY,
            static_cast<u32>(owner.party.variables[Party::Gold]),
            owner.counterAlpha
        );
    }

    if ((!state.GameFlag(GameState::NoPointer) && !state.GameFlag(GameState::NoChoicePointer) &&
         !state.GameFlag(GameState::HideHud)) ||
        owner.input.IsKeyDown(Input::C) != 0)
    {
        owner.RefreshCursor();
    }

    this->FadeColors(1);
    owner.notices.Draw(owner.smallFont, painter, state, owner.textColors);
}

void FieldView::DrawSmallText(int x, int y, u32 color, const std::string& text)
{
    this->game.smallFont.DrawText(this->game.drawSurface, x, y, text, static_cast<Pixel>(color), TextFont::BoldOutline);
}

// swd32.exe 0x00413FE0: the debug display: the cell marks of the walls (F2) and the lines of the state, the region
// and the roles under the pointer (F1)
void FieldView::DrawDebug(int cameraLeft, int cameraTop)
{
    constexpr std::u8string_view kMusicLine = u8"[%d/%d][%d/%d][playing/%d][mode/%s],場景%s,劇情%s";  // 0x004A04F0
    constexpr std::u8string_view kStory = u8"劇情";                                                   // 0x004A052C
    constexpr std::u8string_view kScene = u8"場景";                                                   // 0x004A0524
    constexpr std::u8string_view kBlocked = u8"不可通過";                                             // 0x004A04C8
    constexpr std::u8string_view kPassable = u8"可通過";                                              // 0x004A04C0
    constexpr std::u8string_view kClickable = u8"可點選";                                             // 0x004A04B8
    constexpr std::u8string_view kNotClickable = u8"不可點選";                                        // 0x004A04AC
    constexpr std::u8string_view kRegionLine = u8"劇情號碼[%d] [%d,%s] [%d,%s]";                      // 0x004A048C
    constexpr std::u8string_view kBadRegion = u8"錯誤的地圖劇情號碼 (%d)";                            // 0x004A04D8
    constexpr std::u8string_view kOverlap = u8"角色重疊";                                             // 0x004A0480
    constexpr std::u8string_view kRoleLine = u8"角色 [GUID %d] [act %d] [qq %d] [dir %d]";            // 0x004A0454
    Game& owner = this->game;
    GameState& state = owner.state;
    const Field& field = owner.field;
    const Scene& view = owner.scene;
    owner.smallFont.SetBackground(TextFont::NoBackground);
    owner.smallFont.SetShadow(0);
    const std::array<u16, Config::Channels>& masks = owner.config.channelMasks;
    const u32 red = masks[Config::RedChannel];
    const u32 green = masks[Config::GreenChannel];
    const u32 blue = masks[Config::BlueChannel];

    if (owner.debugCells == 1)
    {
        this->MarkCells(MapHeader::Wall, static_cast<int>(red + green + blue), cameraLeft, cameraTop, 0);
        this->MarkCells(MapHeader::RoleCell, static_cast<int>(green + blue), cameraLeft, cameraTop, kRoleCellInset);
        this->MarkCells(MapHeader::RegionMask, static_cast<int>(red + green), cameraLeft, cameraTop, kRegionInset);
        this->MarkCells(MapHeader::SolidRole, static_cast<int>(red), cameraLeft, cameraTop, kSolidRoleInset);
        this->MarkCells(MapHeader::Unk800000, static_cast<int>(red + blue), cameraLeft, cameraTop, kUnk800000Inset);
    }

    if (owner.debugText != 1)
    {
        return;
    }

    const u32 white = owner.textColors.primary[kWhiteColor];
    const std::vector<u32>& blocks = state.map.blocks;
    const std::size_t pointerCell = (static_cast<u32>(field.cameraLeft + state.pointerX) >> MapHeader::TileShift) +
                                    static_cast<std::size_t>(state.map.width) *
                                        (static_cast<u32>(state.pointerY + field.cameraTop) >> MapHeader::TileShift);
    const u32 region = pointerCell < blocks.size() ? blocks[pointerCell] & MapHeader::RegionMask : 0;
    const Role& player = state.roles[static_cast<std::size_t>(state.playerRole)];
    this->DrawDebugText(
        kDebugLeft,
        DebugRow(0),
        white,
        u8"MAct[%4.1f/%4.1f] LTCor[%4d/%4d]",
        static_cast<double>(static_cast<u32>(player.x)) / MapHeader::TileSize,
        static_cast<double>(static_cast<u32>(player.y)) / MapHeader::TileSize,
        field.cameraLeft >> MapHeader::TileShift,
        field.cameraTop >> MapHeader::TileShift
    );
    this->DrawDebugText(
        kDebugLeft,
        DebugRow(1),
        white,
        u8"Mouse :OnMap[%4d/%4d] OnScr[%4d/%4d]",
        (static_cast<u32>(state.pointerX) >> MapHeader::TileShift) +
            static_cast<u32>(field.cameraLeft >> MapHeader::TileShift),
        (static_cast<u32>(state.pointerY) >> MapHeader::TileShift) +
            static_cast<u32>(field.cameraTop >> MapHeader::TileShift),
        state.pointerX,
        state.pointerY
    );
    const ButtonState& click = owner.controls.At(Button::MouseLeft);
    this->DrawDebugText(
        kDebugLeft,
        DebugRow(2),
        white,
        u8"FPS[%2d] FUsed(%d),kr.d[%1d] ,kr.p[%4d] _IPA[%4d]",
        owner.frameTime != 0 ? kMillisecondsPerSecond / static_cast<u32>(owner.frameTime) : 0u,
        owner.unk4C8448,
        click.press,
        click.heldFrames,
        owner.controls.mouseSuppressed
    );
    this->DrawDebugText(
        kDebugLeft,
        DebugRow(3),
        white,
        u8"MapID[%d] MapCyc[%d] %d",
        owner.currentMap,
        view.unk49E0BC,
        owner.state.walkSpeed
    );
    this->DrawDebugText(
        kDebugLeft,
        DebugRow(4),
        white,
        u8"ULck[%x] UIpt[%x] gTm[%d] Fsc[%d] ,BtlM[%d] ,BtnIp[%d]",
        view.playerHold,
        owner.talk.advance,
        owner.party.clock,
        view.encounterSteps & Scene::EncounterStepMask,
        view.unk4B896C,
        owner.talk.choice
    );
    this->DrawDebugText(
        kDebugLeft,
        DebugRow(5),
        white,
        u8"T.Gd[%d] T.Tid[%d] dbg1[%d] dbg2[%d]",
        static_cast<u32>(state.leader.id),
        static_cast<u32>(state.leader.event),
        view.unk4C9A08,
        view.unk4ACE64
    );

    auto mode = [&view](u32 used, u32 repeat) -> std::string
    {
        if ((view.musicCue & used) == 0)
        {
            return "Nil";
        }

        return (view.musicCue & repeat) != 0 ? "Rep1" : "Cyc2";
    };

    this->DrawDebugText(
        kDebugLeft,
        DebugRow(6),
        white,
        kMusicLine,
        view.musicTunes[Scene::SceneFirst],
        view.musicTunes[Scene::SceneSecond],
        view.musicTunes[Scene::StoryFirst],
        view.musicTunes[Scene::StorySecond],
        view.musicCue & Scene::TrackByte,
        (view.musicCue & Scene::StoryMusic) != 0 ? kStory : kScene,
        mode(Scene::SceneLoops, Scene::SceneRepeatFirst),
        mode(Scene::StoryLoops, Scene::StoryRepeatFirst)
    );

    if (region != 0)
    {
        const MapRegion* found = owner.field.FindRegion(static_cast<int>(region));

        if (found != nullptr)
        {
            const std::string_view name(found->name.data(), found->name.size());
            this->DrawDebugText(
                kDebugLeft,
                DebugRow(kScreenRows - 2),
                white,
                u8"%s",
                std::string(name.substr(0, name.find('\0')))
            );
            const auto flag = static_cast<u32>(found->flag);
            const std::u8string_view passable =
                state.GameFlag(static_cast<int>(flag >> MapRegion::BlockFlagShift)) ? kBlocked : kPassable;
            const std::u8string_view clickable =
                state.GameFlag(static_cast<int>(flag & MapRegion::ClickFlagMask)) ? kClickable : kNotClickable;
            this->DrawDebugText(
                kDebugLeft,
                DebugRow(kScreenRows - 1),
                white,
                kRegionLine,
                found->event & MapRegion::EventMask,
                flag & MapRegion::ClickFlagMask,
                clickable,
                flag >> MapRegion::BlockFlagShift,
                passable
            );
        }
        else
        {
            this->DrawDebugText(kDebugLeft, DebugRow(kScreenRows - 3), white, kBadRegion, region);
        }
    }

    const auto mapX = static_cast<u32>(state.pointerX + field.cameraLeft);
    const auto mapY = static_cast<u32>(state.pointerY + field.cameraTop);
    int overlapping = 0;

    for (u32 index = 1; index < static_cast<u32>(state.roleCount) && index < GameState::Roles; ++index)
    {
        const Role& role = state.roles[index];

        if (role.act.id == 0)
        {
            continue;
        }

        if (mapX > static_cast<u32>(role.x - MapHeader::TileSize) &&
            mapX < static_cast<u32>(role.x + kOverlapColumns * MapHeader::TileSize) &&
            mapY > static_cast<u32>(role.y - kOverlapRows * MapHeader::TileSize) &&
            mapY < static_cast<u32>(role.y + MapHeader::TileSize))
        {
            ++overlapping;

            if (overlapping >= kOverlapRoles)
            {
                this->DrawDebugText(kDebugLeft, DebugRow(kScreenRows - 5), white, kOverlap);
            }

            this->DrawDebugText(
                kDebugLeft,
                DebugRow(kScreenRows - 4),
                owner.textColors.primary[kRoleLineColor],
                kRoleLine,
                static_cast<u32>(role.id),
                role.act.id,
                role.act.action,
                role.act.direction
            );
            this->DrawDebugText(
                kDebugLeft,
                DebugRow(kScreenRows - 3),
                white,
                u8"[GUID %d] [Talk %d] [Path %d] [Argu %4x] [ArrIdx %d]",
                static_cast<u32>(role.id),
                static_cast<u32>(role.event),
                static_cast<u32>(role.unk1C),
                role.flags,
                index
            );
        }
    }
}

// The flags a role takes from its cell on entering it (inline in the original): a reflecting cell (bit 11) sets flag
// 29, the shade bits 12..15 go to flags 20..23
void FieldView::UpdateRoleCellFlags(Role& role)
{
    if ((role.flags & Role::CellEffects) == 0)
    {
        return;
    }

    role.flags &= ~(Role::Reflects | Role::CellShades);
    const u32 cell = role.cell != nullptr ? *role.cell : 0;

    if ((cell & MapHeader::Reflecting) != 0)
    {
        role.flags |= Role::Reflects;
    }

    if ((cell & MapHeader::ShadeMask) != 0)
    {
        role.flags |= (cell & MapHeader::ShadeMask) << MapHeader::ShadeShift;
    }
}

// swd32.exe 0x004120B0: a frame of the map: the screen acts step, the camera and the controlled role move by their
// step, every role on a move steps along it (twice as fast without flag 0xD4 or with flag 0x4000000; at the end it
// takes the move's act, resumes a held move and, for role 1, stops the keys and recentres), touch roles start their
// event, the followers step, the camera pans and shakes; then the frame is painted with the timer and the debug display
// and shown; the controlled role's cells and trail follow its step, the tile layers cycle, and a shaken camera is put
// back
void FieldView::Render()
{
    // swd32.exe 0x00499574: the act direction of each path direction; 0x00499534 and 0x00499554: its cell step.
    static constexpr std::array<i32, 8> kCellX = {1, 0, -1, -1, -1, 0, 1, 1};
    static constexpr std::array<i32, 8> kCellY = {1, 1, 1, 0, -1, -1, -1, 0};
    Game& owner = this->game;
    GameState& state = owner.state;
    Scene& view = owner.scene;
    Field& field = owner.field;
    const i32 leaderId = owner.unk4B7BC4;
    const int width = state.map.width;

    for (std::size_t act = view.unk4B9F68.size(); act-- > 0;)
    {
        ActState& slot = view.unk4B9F68[act];

        if (slot.id != 0 && owner.acts.Run(slot) == 0)
        {
            DebugLog(
                u8"Act Err(picPaint:HeadSgn) 頭定符號設定錯誤 \nact[%d] qq[%d] dir[%d]",
                slot.id,
                slot.action,
                slot.direction
            );
        }
    }

    Role& player = state.roles[static_cast<std::size_t>(state.playerRole)];
    const int scroll = owner.state.walkSpeed;
    player.x += scroll * (owner.unk4B7518 + owner.unk4A9488);
    player.y += scroll * (owner.unk4B7920 + owner.unk4A948C);
    field.cameraLeft += owner.unk4B7518 * scroll;
    field.cameraTop += owner.unk4B7920 * scroll;
    field.cameraRight += owner.unk4B7518 * scroll;
    field.cameraBottom += owner.unk4B7920 * scroll;

    auto moveCell = [&state, width](Role& role, int direction)
    {
        if (role.cell == nullptr || direction < 0 || direction >= RolePath::Directions)
        {
            return;
        }

        const std::ptrdiff_t at = role.cell - state.map.blocks.data() + kCellX[static_cast<std::size_t>(direction)] +
                                  static_cast<std::ptrdiff_t>(width) * kCellY[static_cast<std::size_t>(direction)];
        role.cell = at >= 0 && static_cast<std::size_t>(at) < state.map.blocks.size()
                        ? &state.map.blocks[static_cast<std::size_t>(at)]
                        : nullptr;
    };

    for (MoveRecord& move : state.moves)
    {
        if (move.role == MoveRecord::NoRole || move.step >= MoveRecord::StepMask)
        {
            continue;
        }

        const u16 index = move.role;

        if (index >= GameState::Roles)
        {
            continue;
        }

        Role& role = state.roles[index];

        if ((role.flags & Role::Visible) == 0)
        {
            continue;
        }

        const u16 id = role.id;

        if ((id == state.leader.id && move.Kind() == MoveKind::Path) ||
            ((role.flags & Role::FacesTalker) != 0 && role.talkState == Role::TalkWaiting))
        {
            continue;
        }

        if (role.act.delay == 0)
        {
            int stepX = move.stepX;
            int stepY = move.stepY;

            if (role.unkD4 == 0)
            {
                stepX *= kFastFactor;
                stepY *= kFastFactor;
            }

            if ((role.flags & Role::FastMove) != 0)
            {
                stepX *= kFastFactor;
                stepY *= kFastFactor;
            }

            const u8 direction = move.directions[static_cast<std::size_t>(move.step) % move.directions.size()];

            if (direction != MoveRecord::EndOfPath && direction < RolePath::Directions)
            {
                role.act.direction = RolePath::Facing[direction];
            }

            role.x += stepX;
            role.y += stepY;

            if (((role.x | role.y) & MapHeader::TileFraction) == 0)
            {
                field.RemoveRole(
                    id,
                    static_cast<int>(role.flags & Role::LayerMask),
                    (static_cast<u32>(role.y) >> MapHeader::TileShift) - 1,
                    0
                );

                if (role.x == move.x && role.y == move.y)
                {
                    role.act.speed = 0;
                    role.flags = (role.flags & ~(Role::Stepping | Role::FastMove | Role::Arrived)) | Role::Arrived;

                    if (move.act != -1)
                    {
                        role.act.id = move.act;
                    }

                    if (move.action != -1)
                    {
                        role.act.action = move.action;
                    }

                    if (move.direction != -1)
                    {
                        role.act.direction = move.direction;
                    }

                    if ((move.mode & MoveRecord::HoldAtEnd) == 0)
                    {
                        owner.rolePath.ResumeMove(index);
                        role.act.delay = 0;
                        role.flags &= ~Role::Held;
                    }

                    if (role.id == 1)
                    {
                        owner.unk4B7920 = 0;
                        owner.controls.buttons = {};
                        owner.unk4B7518 = 0;
                        owner.unk4A948C = 0;
                        owner.unk4A9488 = 0;

                        if ((view.screenFlags & Scene::CameraHeld) == 0)
                        {
                            field.CenterCamera();
                        }
                    }

                    role.flags &= ~Role::PassWalls;
                }

                field.ClearRoleCells(role, leaderId);

                if (direction != MoveRecord::EndOfPath)
                {
                    moveCell(role, direction);
                }

                field.MarkRoleCells(role, leaderId);
                move.step = static_cast<u16>((move.step + 1) | MoveRecord::Done);
                this->UpdateRoleCellFlags(role);
            }

            if (state.leader.id == Role::NoId && (role.flags & Role::TouchEvent) != 0)
            {
                const std::ptrdiff_t cell = role.cell != nullptr ? role.cell - state.map.blocks.data() : 0;

                if (static_cast<u8>(field.ProbeCells(cell, role.act.width, role.act.height, MapHeader::LeaderCell)) !=
                    0)
                {
                    state.leader.unk14 = role.unk14;
                    state.leader.unk20 = role.unk20;
                    state.leader.event = role.event;
                    state.leader.id = role.id;
                    state.leader.flags = role.flags;
                }
            }
        }

        if (owner.acts.Run(role.act) == 0)
        {
            DebugLog(
                u8"Act Err(picPaint:PathMove) 設定錯誤 GUID[%d]\nact[%d] qq[%d] dir[%d]",
                role.id,
                role.act.id,
                role.act.action,
                role.act.direction
            );
        }

        if (index == state.playerRole && (view.screenFlags & Scene::CameraHeld) == 0)
        {
            field.CenterCamera();
        }
    }

    for (u32 follower = 1; follower < static_cast<u32>(view.unk4BABA0) && follower < view.followerMoves.size();
         ++follower)
    {
        MoveRecord& move = view.followerMoves[follower];

        if (move.role == MoveRecord::NoRole || move.role >= GameState::Roles)
        {
            continue;
        }

        Role& role = state.roles[move.role];

        if (move.step >= MoveRecord::StepMask)
        {
            owner.acts.Run(role.act);
            continue;
        }

        if (role.act.delay == 0)
        {
            const u8 direction = move.directions[static_cast<std::size_t>(move.step) % move.directions.size()];

            if (direction < RolePath::Directions)
            {
                role.act.direction = RolePath::Facing[direction];
            }

            role.x += move.stepX;
            role.y += move.stepY;

            if (((role.x | role.y) & MapHeader::TileFraction) == 0)
            {
                field.RemoveRole(role.id, static_cast<int>(role.flags & Role::LayerMask), 0, 0);
                field.ClearRoleCells(role, leaderId);
                moveCell(role, direction);
                field.MarkRoleCells(role, leaderId);
                move.step = static_cast<u16>((move.step + 1) | MoveRecord::Done);
                this->UpdateRoleCellFlags(role);
            }
        }

        if (owner.acts.Run(role.act) == 0)
        {
            DebugLog(
                u8"Act Err(picPaint:CompanyMove) 設定錯誤 GUID[%d]\nact[%d] qq[%d] dir[%d]",
                role.id,
                role.act.id,
                role.act.action,
                role.act.direction
            );
        }
    }

    this->PanCamera();
    this->ShakeCamera();
    owner.audioDevice.Serve();
    owner.drawSurface = owner.painter.LockSurface(*owner.offscreen);
    owner.painter.SetTarget(owner.drawSurface);
    owner.painter.UnlockSurface(*owner.offscreen);
    this->Paint();
    this->DrawTimer(kTimerX, kTimerY, 0);

    if (owner.config.debugMode == 1)
    {
        this->DrawDebug(field.cameraLeft, field.cameraTop);
    }

    owner.audioDevice.Serve();
    owner.ShowFrame();

    if ((player.x & MapHeader::TileFraction) == 0 && (player.y & MapHeader::TileFraction) == 0)
    {
        field.RemoveRole(
            player.id,
            static_cast<int>(player.flags & Role::LayerMask),
            (static_cast<u32>(player.y) >> MapHeader::TileShift) - 1,
            0
        );
        field.ClearRoleCells(player, leaderId);

        if (player.cell != nullptr)
        {
            const std::ptrdiff_t at = player.cell - state.map.blocks.data() + owner.unk4B7518 + owner.unk4A9488 +
                                      static_cast<std::ptrdiff_t>(width) * (owner.unk4B7920 + owner.unk4A948C);
            player.cell = at >= 0 && static_cast<std::size_t>(at) < state.map.blocks.size()
                              ? &state.map.blocks[static_cast<std::size_t>(at)]
                              : nullptr;
        }

        field.MarkRoleCells(player, leaderId);
        owner.unk4B7920 = 0;
        owner.unk4A948C = 0;
        owner.unk4B7518 = 0;
        owner.unk4A9488 = 0;

        if (view.trailX[0] != player.x || view.trailY[0] != player.y)
        {
            std::copy_backward(view.trailX.begin(), view.trailX.end() - 1, view.trailX.end());
            std::copy_backward(view.trailY.begin(), view.trailY.end() - 1, view.trailY.end());
            std::copy_backward(view.trailDirection.begin(), view.trailDirection.end() - 1, view.trailDirection.end());
            view.trailX[0] = player.x;
            view.trailY[0] = player.y;
            view.trailDirection[0] = player.act.direction;
        }

        this->UpdateRoleCellFlags(player);
    }

    if ((player.flags & Role::Stepping) == 0 && (player.flags & Role::StepsAct) != 0 && owner.acts.Run(player.act) == 0)
    {
        DebugLog(
            u8"Act Err(PicPaint:Render:MRl) 設定錯誤 GUID[%d]\nact[%d] qq[%d] dir[%d]",
            player.id,
            player.act.id,
            player.act.action,
            player.act.direction
        );
    }

    if (++view.unk4CC2DC >= view.unk49E0BC && state.map.layers != 1)
    {
        int layer = view.unk49E0C0 + view.unk4B7944;
        view.unk4CC2DC = 0;
        view.unk4B7944 += view.unk49E0C0;

        if (static_cast<u32>(view.unk4B7944) >= static_cast<u32>(state.map.layers))
        {
            view.unk49E0C0 = view.unk49E0C0 != 1 ? 1 : -1;
            layer += kLayerSwing * view.unk49E0C0;
            view.unk4B7944 = layer;
        }

        view.unk4B873C = layer * view.unk4B7A98;
    }

    if (view.shake[0] != Scene::ShakeEnd && owner.currentMap != Field::DemonRefiningPot)
    {
        field.cameraLeft = view.unk4A93E4;
        field.cameraTop = view.unk4A992C;
        field.cameraRight = view.unk4A93E4 + Display::Width;
        field.cameraBottom = view.unk4A992C + Display::Height;
    }
}

// swd32.exe 0x00430C60: a ripple over `lines` lines of `pitch` pixels at (left, top) with a gridWidth x gridHeight grid
ScriptObject::ScriptObject(int pitch, int lines, int left, int top, int gridWidth, int gridHeight)
    : pitch(pitch),
      lines(lines),
      left(left),
      top(top),
      gridWidth(gridWidth),
      gridHeight(gridHeight),
      damping(kDefaultDamping),
      heights(
          kPages * static_cast<std::size_t>(std::max(gridWidth, 0)) * static_cast<std::size_t>(std::max(gridHeight, 0)),
          0
      ),
      screen(static_cast<std::size_t>(std::max(pitch, 0)) * static_cast<std::size_t>(std::max(lines, 0)), 0)
{
}

// swd32.exe 0x00430D10: the ripple moved to (x, y), kept one cell inside the right and bottom of the screen copy
void ScriptObject::MoveTo(int x, int y)
{
    this->left = x;
    this->top = y;

    if (this->gridWidth + x > this->pitch)
    {
        this->left = this->pitch - this->gridWidth - 1;
    }

    if (this->gridHeight + y > this->lines)
    {
        this->top = this->lines - this->gridHeight - 1;
    }
}

// swd32.exe 0x00430FF0: a drop of `radius` cells into the current page, deepest (radius x depth) at its centre;
// a negative place is chosen at random inside the grid
void ScriptObject::Splash(int x, int y, int radius, int depth, crt::Random& random)
{
    if (x < 0)
    {
        x = random.Next() % (this->gridWidth - 2 * radius - 1) + radius + 1;
    }

    if (y < 0)
    {
        y = random.Next() % (this->gridHeight - 2 * radius - 1) + radius + 1;
    }

    int fromX = -radius;
    int fromY = -radius;
    int toX = radius;
    int toY = radius;

    if (x - radius < 1)
    {
        fromX = 1 - x;
    }

    if (y - radius < 1)
    {
        fromY = 1 - y;
    }

    if (x + radius > this->gridWidth - 1)
    {
        toX = this->gridWidth - x - 1;
    }

    if (radius + y > this->gridHeight - 1)
    {
        toY = this->gridHeight - y - 1;
    }

    const int page = this->gridWidth * this->gridHeight * this->page;

    for (int dy = fromY; dy < toY; ++dy)
    {
        for (int dx = fromX; dx < toX; ++dx)
        {
            if (dx * dx + dy * dy < radius * radius)
            {
                const int at = page + x + dx + this->gridWidth * (dy + y);
                const auto drop = static_cast<i16>(
                    crt::Ftol((static_cast<double>(radius) - std::sqrt(static_cast<double>(dx * dx + dy * dy))) * depth)
                );

                if (at >= 0 && static_cast<std::size_t>(at) < this->heights.size())
                {
                    this->heights[static_cast<std::size_t>(at)] =
                        static_cast<i16>(this->heights[static_cast<std::size_t>(at)] + drop);
                }
            }
        }
    }
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

    for (int cell = width + 1; cell < last; at += this->pitch - width + kGridBorder)
    {
        const int end = width + cell - kGridBorder;

        while (cell < end)
        {
            for (int pair = 0; pair < kPixelsPerPair; ++pair)
            {
                const int here = height(cell);
                const int source = this->pitch * ((here - height(cell + width)) >> kRefractShift) + at +
                                   ((here - height(cell + 1)) >> kRefractShift);

                if (at >= 0 && static_cast<std::size_t>(at) < pixels.size())
                {
                    copy(source, pixels[static_cast<std::size_t>(at)]);
                }

                ++cell;
                ++at;
            }
        }

        cell += kGridBorder;
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
    int sum = read(2 * width + kGridBorder) + read(kGridBorder);
    const int last = width * (this->gridHeight - 1);

    while (cell < last)
    {
        int before = sum;
        sum = read(width + cell) + read(cell - width);
        const int end = width + cell - kGridBorder;

        while (cell < end)
        {
            const int current = sum;
            sum = read(cell + width + 1) + read(cell - width + 1);
            const auto at = static_cast<std::size_t>(target + cell);

            if (at < this->heights.size())
            {
                const int change =
                    ((before + current + read(cell - 1) + sum + read(cell + 1)) >> kAverageShift) - this->heights[at];
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

        cell += kGridBorder;
    }

    return settled;
}

}  // namespace swd3
