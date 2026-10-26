#include "battle/battle_hud.h"

#include "audio/audio_manager.h"
#include "battle/battle_hud_constants.h"
#include "battle/battle_services.h"
#include "battle/smooth/smooth_hud.h"
#include "core/words.h"
#include "crt/msvc.h"
#include "draw/painter.h"
#include "pic/picture.h"
#include "pic/picture_ops.h"
#include "platform/display.h"
#include "random/random.h"
#include "res/act_archive.h"
#include "res/tsw_archive.h"

namespace swd3
{

namespace
{

}  // namespace

BattleHud::BattleHud(Painter& painter, TswArchive& archive, ActArchive& acts, Random& random, AudioManager& audio)
    : painter(painter),
      archive(archive),
      acts(acts),
      random(random),
      audio(audio)
{
}

void BattleHud::Attach(BattleServices& battle)
{
    this->services = &battle;
}

// Picture `frame` of `id` as the source and the last picture.
const Picture* BattleHud::Select(int id, int frame)
{
    const Picture* picture = this->archive.GetPicture(static_cast<u32>(id), static_cast<u32>(frame));
    this->lastPicture = picture;

    if (picture != nullptr)
    {
        this->painter.SetSource(picture->data.data());
    }

    return picture;
}

void BattleHud::Draw(const Picture* picture, int x, int y, int width, int height, u32 mode)
{
    smooth::Scene scene;
    smooth::HudFrame(*this).DrawAdvance(picture, x, y, width, height, mode, scene);
    scene.Show(*this->services);
}

// swd32.exe 0x00450270
int BattleHud::DrawPictureFrame0(int id, int x, int y)
{
    smooth::Scene scene;
    const auto result = smooth::HudFrame(*this).DrawPictureFrame0Advance(id, x, y, scene);
    scene.Show(*this->services);
    return result;
}

// swd32.exe 0x004502B0: portrait action `index` of act 0x2390, outlined in green when highlighted, darkened when `dim`
// is 1
int BattleHud::DrawPortrait(u16 index, int x, int y, int dim)
{
    smooth::Scene scene;
    const auto result = smooth::HudFrame(*this).DrawPortraitAdvance(index, x, y, dim, scene);
    scene.Show(*this->services);
    return result;
}

// swd32.exe 0x00450400: the looping arrow (act 0x2392) of one of four slots
int BattleHud::DrawArrow(int x, int y, int slot)
{
    smooth::Scene scene;
    const auto result = smooth::HudFrame(*this).DrawArrowAdvance(x, y, slot, scene);
    scene.Show(*this->services);
    return result;
}

// swd32.exe 0x00450490: only positive widths are drawn
int BattleHud::DrawBar(int id, int frame, int x, int y, int width)
{
    smooth::Scene scene;
    const auto result = smooth::HudFrame(*this).DrawBarAdvance(id, frame, x, y, width, scene);
    scene.Show(*this->services);
    return result;
}

// swd32.exe 0x004504E0
int BattleHud::DrawPictureFrame(int id, int frame, int x, int y)
{
    smooth::Scene scene;
    const auto result = smooth::HudFrame(*this).DrawPictureFrameAdvance(id, frame, x, y, scene);
    scene.Show(*this->services);
    return result;
}

// swd32.exe 0x00450530: frame 0 as the background, `fill` pixels of frame 1 over it
int BattleHud::DrawGauge(int id, int x, int y, int fill)
{
    smooth::Scene scene;
    const auto result = smooth::HudFrame(*this).DrawGaugeAdvance(id, x, y, fill, scene);
    scene.Show(*this->services);
    return result;
}

// swd32.exe 0x004505B0: as DrawGauge with frame 2
int BattleHud::DrawGaugeAlt(int id, int x, int y, int fill)
{
    smooth::Scene scene;
    const auto result = smooth::HudFrame(*this).DrawGaugeAltAdvance(id, x, y, fill, scene);
    scene.Show(*this->services);
    return result;
}

// swd32.exe 0x00450630: frame 1 drawn fill + 2 pixels wide (also when fill is 0)
int BattleHud::DrawGaugeWide(int id, int x, int y, u16 fill)
{
    smooth::Scene scene;
    const auto result = smooth::HudFrame(*this).DrawGaugeWideAdvance(id, x, y, fill, scene);
    scene.Show(*this->services);
    return result;
}

// swd32.exe 0x004506B0: decimal digits right aligned at x (frames of picture `font`); the original splits four digits
// and asks a frame past 9 for values above 9999, here every digit is drawn
int BattleHud::DrawSmallNumber(int font, int value, int x, int y)
{
    smooth::Scene scene;
    const auto result = smooth::HudFrame(*this).DrawSmallNumberAdvance(font, value, x, y, scene);
    scene.Show(*this->services);
    return result;
}

// swd32.exe 0x004507A0: a ten-digit number from x without leading zeros (the units digit always drawn)
int BattleHud::DrawNumber(i16 font, int value, int x, int y)
{
    smooth::Scene scene;
    const auto result = smooth::HudFrame(*this).DrawNumberAdvance(font, value, x, y, scene);
    scene.Show(*this->services);
    return result;
}

// swd32.exe 0x00450900: the digit for `divisor` 16 pixels left of numberX; its width (0 for a leading zero)
u16 BattleHud::DrawNumberDigit(u32 divisor)
{
    smooth::Scene scene;
    const auto result = smooth::HudFrame(*this).DrawNumberDigitAdvance(divisor, scene);
    scene.Show(*this->services);
    return result;
}

// swd32.exe 0x004509D0: the looping animation `id` of one of eight slots
int BattleHud::DrawSlotAnimation(int id, int slot, int x, int y)
{
    smooth::Scene scene;
    const auto result = smooth::HudFrame(*this).DrawSlotAnimationAdvance(id, slot, x, y, scene);
    scene.Show(*this->services);
    return result;
}

// swd32.exe 0x00450A50: the colour argument itself is the source picture: its first pixel blended in by the raw mode 8
int BattleHud::FillColor(int x, int y, int width, int height, u32 color)
{
    u32 slot = color;
    this->painter.SetSource(&slot);
    const int result = this->painter.DrawPicture(x, y, width, height, Painter::RawGradient);
    this->painter.SetSource(nullptr);
    return result;
}

// swd32.exe 0x00450A80: one step of the shared effect animation (mirrored when `mirror` is 1); 1 once it reached its
// end
int BattleHud::DrawEffectAt(int id, int action, int x, int y, int mirror)
{
    smooth::Scene scene;
    const auto result = smooth::HudFrame(*this).DrawEffectAtAdvance(id, action, x, y, mirror, scene);
    scene.Show(*this->services);
    return result;
}

// swd32.exe 0x00450B40
int BattleHud::ResetEffectAt()
{
    this->effectAct = ActState{};
    return 0;
}

// swd32.exe 0x00450B60
int BattleHud::DrawCursorAnimation(int id, int x, int y)
{
    smooth::Scene scene;
    const auto result = smooth::HudFrame(*this).DrawCursorAnimationAdvance(id, x, y, scene);
    scene.Show(*this->services);
    return result;
}

// swd32.exe 0x00450BD0: a digit of font 0 (picture 0x2359) or 1 (0x2358); other fonts redraw the last picture
u16 BattleHud::DrawDigit(int font, int digit, int x, int y)
{
    smooth::Scene scene;
    const auto result = smooth::HudFrame(*this).DrawDigitAdvance(font, digit, x, y, scene);
    scene.Show(*this->services);
    return result;
}

const Picture* BattleHud::RunScroll(u16 id, int action)
{
    ActState& state = this->scrollAct;
    state.id = id;
    state.action = action;

    if (this->acts.Run(state) == 0)
    {
        return nullptr;
    }

    const Picture* picture = this->archive.GetPicture(state.frame, state.picture);

    if (picture != nullptr)
    {
        this->painter.SetSource(picture->data.data());
    }

    return picture;
}

// swd32.exe 0x00450C50: the scroll bar of act `id`: the top cap (action 0x1A, 0x1E with style 1), `rows` track pieces
// (0x18), the thumb (0x19) sized 7 / scrollCount of the track and placed by scrollFirst, the bottom cap (0x1B, 0x1F
// with style 2)
int BattleHud::DrawScrollBar(u16 id, int x, int y, int rows, int offset, int style)
{
    smooth::Scene scene;
    const auto result = smooth::HudFrame(*this).DrawScrollBarAdvance(id, x, y, rows, offset, style, scene);
    scene.Show(*this->services);
    return result;
}

// swd32.exe 0x00450F90: the coin of act 0x2329 flips between two positions, slowing down every 25 frames; 1 once the
// delay reaches 64 (a coin ending on side 1 counts in the high word of coinState)
int BattleHud::DrawCoinToss()
{
    smooth::Scene scene;
    const auto result = smooth::HudFrame(*this).DrawCoinTossAdvance(scene);
    scene.Show(*this->services);
    return result;
}

// swd32.exe 0x00451100: the timing bar (picture 0x234F) with three marks and a moving needle; timingHit is the mark
// (1-based) under the needle unless it equals the low word of numberFont, which is cleared otherwise. 1 when the needle
// reaches the end.
int BattleHud::DrawTimingBar(int x, int y)
{
    smooth::Scene scene;
    const auto result = smooth::HudFrame(*this).DrawTimingBarAdvance(x, y, scene);
    scene.Show(*this->services);
    return result;
}

// swd32.exe 0x004512B0: the vertical gauge of picture 0x241A showing `level` sixths of its body
int BattleHud::DrawTallGauge(int x, int y, int level)
{
    smooth::Scene scene;
    const auto result = smooth::HudFrame(*this).DrawTallGaugeAdvance(x, y, level, scene);
    scene.Show(*this->services);
    return result;
}

// swd32.exe 0x00451540: one step of the animation with the picture loaded for its frame; its waiting flag
int BattleHud::DrawAnimation(BattleAnimation& animation)
{
    smooth::Scene scene;
    const auto result = smooth::HudFrame(*this).DrawAnimationAdvance(animation, scene);
    scene.Show(*this->services);
    return result;
}

// swd32.exe 0x00451730
int BattleHud::FreeAnimation(BattleAnimation& animation)
{
    for (std::unique_ptr<Picture>& picture : animation.pictures)
    {
        picture.reset();
    }

    animation.id = 0;
    animation.wave = 0;
    animation.act = ActState{};
    return 0;
}

// swd32.exe 0x00451420: loads every frame picture of act `id` (run to its end) into the animation, each shifted right
// by 640 / `divisor`; the wave mode of 'WT' is kept. `unused` is ignored. Always 0.
int BattleHud::LoadAnimation(BattleAnimation& animation, int, int x, int y, u16 id, u16 divisor)
{
    std::array<u16, BattleAnimation::Pictures> loaded = {};
    loaded.fill(kNotLoaded);
    ActState& state = animation.act;
    animation.id = id;
    animation.x = x;
    animation.y = y;
    state.id = id;
    state.action = 0;

    if (this->acts.Run(state) == 0)
    {
        return 0;
    }

    for (;;)
    {
        if (state.wait != 0)
        {
            animation.wave = state.wait;
        }

        const u16 frame = state.picture;

        if (frame < BattleAnimation::Pictures && loaded[frame] == kNotLoaded)
        {
            animation.pictures[frame] = this->archive.LoadPictureDirect(state.frame, state.picture);
            loaded[frame] = frame;

            if (animation.pictures[frame] != nullptr)
            {
                ShiftPicture(animation.pictures[frame]->data, ShiftMode::Right, Display::Width / divisor);
            }
        }

        if (state.position == 0)
        {
            break;
        }

        state.action = 0;
        state.id = animation.id;

        if (this->acts.Run(state) == 0)
        {
            return 0;
        }
    }

    state = ActState{};
    return 0;
}

// swd32.exe 0x004515E0: plays the loaded animation to its end in one call, every new frame shifted by `amount` (right
// when positive, left when negative) and drawn; 1 when it ended, 0 without an animation. `unused` is ignored.
int BattleHud::PlayAnimationShifted(BattleAnimation& animation, int unused, int amount)
{
    smooth::Scene scene;
    const auto result = smooth::HudFrame(*this).PlayAnimationShiftedAdvance(animation, unused, amount, scene);
    scene.Show(*this->services);
    return result;
}

// swd32.exe 0x0044FFE0: a box of `columns` x `rows` tiles of picture `id` around a filled area (`fill`, from frame 4,
// 16 pixels a tile with a border tile each side): frames 0..2 the top (corner, edge repeated, corner), 3 and 5 the
// sides of each row, 6..8 the bottom. A missing frame is read through a null pointer in the original; it is not drawn
// and takes no room here.
void BattleHud::DrawTileBox(int id, int x, int y, int columns, int rows, i8 fill)
{
    smooth::Scene scene;
    smooth::HudFrame(*this).DrawTileBoxAdvance(id, x, y, columns, rows, fill, scene);
    scene.Show(*this->services);
}
}  // namespace swd3
