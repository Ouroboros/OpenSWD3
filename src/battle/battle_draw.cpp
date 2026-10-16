#include "battle/battle_draw.h"

#include <cstring>

#include "audio/audio.h"
#include "core/mem.h"
#include "core/random.h"
#include "crt/crt.h"
#include "data/globals.h"
#include "draw/draw.h"
#include "pic/pic_ops.h"
#include "res/act_anim.h"
#include "res/tsw.h"

namespace swd3 {

namespace {

int argOf(const Picture* picture) { return static_cast<int>(reinterpret_cast<std::uintptr_t>(picture->reserved4)); }

Picture* select(int id, int frame) {
    Picture* picture = TswGetPicture(static_cast<std::uint32_t>(id), static_cast<std::uint32_t>(frame));
    G.uiPicture = picture;
    G.drawSource = reinterpret_cast<std::uint8_t*>(picture->pixels);
    return picture;
}

}  // namespace

// swd32.exe 0x00450270
int DrawPictureFrame0(int id, int x, int y) {
    Picture* picture = select(id, 0);
    return DrawPicture(x, y, picture->width, picture->height, 0, 0);
}

// swd32.exe 0x004502B0: portrait action `index` of act 0x2390, outlined in green when highlighted, darkened when
// `dim` is 1
int DrawPortrait(std::uint16_t index, int x, int y, int dim) {
    ActState state;
    std::memset(&state, 0, sizeof(state));
    state.id = 0x2390;
    state.action = index;
    const int result = ActRun(&state);
    if (result == 0) {
        return 0;
    }
    Picture* picture = select(state.fr, state.ap);
    if (G.portraitHighlight[index] == 1) {
        std::uint32_t outline = 0x07E007E0u;
        DrawOutline(x - state.x, y - state.y, picture->width, picture->height, 0,
                    static_cast<int>(reinterpret_cast<std::uintptr_t>(&outline)));
        picture = G.uiPicture;
    }
    DrawPicture(x - state.x, y - state.y, picture->width, picture->height, 0, argOf(picture));
    if (dim != 1) {
        return dim;
    }
    G.drawOption30C = -10;
    G.drawOption304 = -10;
    G.drawOption71C = 0;
    Picture* current = G.uiPicture;
    return DrawPicture(x - state.x, y - state.y, current->width, current->height, static_cast<int>(state.flags | 0x10),
                       argOf(current));
}

// swd32.exe 0x00450400: looping arrow (act 0x2392) in one of four slots
int DrawArrow(int x, int y, int slot) {
    ActState& state = G.arrowActs[slot];
    state.id = 0x2392;
    state.action = 0;
    const int result = ActRun(&state);
    if (result == 0) {
        return 0;
    }
    Picture* picture = select(state.fr, state.ap);
    return DrawPicture(x - state.x, y - state.y, picture->width, picture->height, static_cast<int>(state.flags),
                       argOf(picture));
}

// swd32.exe 0x00450490: only positive widths are drawn
int DrawBar(int id, int frame, int x, int y, int width) {
    Picture* picture = select(id, frame);
    if (width > 0) {
        return DrawPicture(x, y, width, picture->height, 0, argOf(picture));
    }
    return static_cast<int>(reinterpret_cast<std::uintptr_t>(picture));
}

// swd32.exe 0x004504E0
int DrawPictureFrame(int id, int frame, int x, int y) {
    Picture* picture = select(id, frame);
    return DrawPicture(x, y, picture->width, picture->height, 0, argOf(picture));
}

// swd32.exe 0x00450530: frame 0 as background, `fill` pixels of frame 1 over it
int DrawGauge(int id, int x, int y, int fill) {
    Picture* back = select(id, 0);
    DrawPicture(x, y, back->width, back->height, 0, argOf(back));
    Picture* front = select(id, 1);
    if (fill != 0) {
        return DrawPicture(x, y, fill, front->height, 0, argOf(front));
    }
    return static_cast<int>(reinterpret_cast<std::uintptr_t>(front));
}

// swd32.exe 0x004505B0: as DrawGauge with frame 2
int DrawGaugeAlt(int id, int x, int y, int fill) {
    Picture* back = select(id, 0);
    DrawPicture(x, y, back->width, back->height, 0, argOf(back));
    Picture* front = select(id, 2);
    if (fill != 0) {
        DrawPicture(x, y, fill, front->height, 0, argOf(front));
    }
    return 1;
}

// swd32.exe 0x00450630: frame 1 drawn fill + 2 pixels wide (also when fill is 0)
int DrawGaugeWide(int id, int x, int y, std::uint16_t fill) {
    Picture* back = select(id, 0);
    DrawPicture(x, y, back->width, back->height, 0, argOf(back));
    Picture* front = select(id, 1);
    return DrawPicture(x, y, fill + 2, front->height, 0, argOf(front));
}

// swd32.exe 0x004506B0: up to four decimal digits right aligned at x (frames of picture `font`)
int DrawSmallNumber(int font, int value, int x, int y) {
    int digits[4];
    int count = 0;
    int divisor = 1000;
    G.numberX = x;
    G.numberY = y;
    G.numberStarted = 0;
    for (int i = 0; i < 4; ++i) {
        G.numberValue = value;
        digits[i] = value / divisor;
        value %= divisor;
        if (digits[i] != 0 || G.numberStarted == 1) {
            G.numberStarted = 1;
            ++count;
        }
        divisor /= 10;
    }
    if (count == 0) {
        count = 1;
    }
    int result = 0;
    for (int i = 0; i < count; ++i) {
        Picture* picture = select(font, digits[3 - i]);
        result = DrawPicture(x, y, picture->width, picture->height, 0, argOf(picture));
        x -= G.uiPicture->width;
    }
    return result;
}

// swd32.exe 0x004507A0: ten digit number from x without leading zeros (the units digit always drawn)
int DrawNumber(std::int16_t font, int value, int x, int y) {
    G.numberFont = (G.numberFont & 0xFFFF) | (static_cast<std::uint32_t>(static_cast<std::uint16_t>(font)) << 16);
    G.numberValue = value;
    G.numberX = x;
    G.numberY = y;
    G.numberStarted = 0;
    G.numberX = DrawNumberDigit(1000000000u) + x;
    G.numberX += DrawNumberDigit(100000000u);
    G.numberX += DrawNumberDigit(10000000u);
    G.numberX += DrawNumberDigit(1000000u);
    G.numberX += DrawNumberDigit(100000u);
    G.numberX += DrawNumberDigit(10000u);
    G.numberX += DrawNumberDigit(1000u);
    G.numberX += DrawNumberDigit(100u);
    G.numberX += DrawNumberDigit(10u);
    G.numberStarted = 1;
    const int width = DrawNumberDigit(1u);
    G.numberX += width;
    return width;
}

// swd32.exe 0x00450900: draws the digit for `divisor` 16 pixels left of G.numberX; returns its width (0 for a
// leading zero)
std::uint16_t DrawNumberDigit(std::uint32_t divisor) {
    const std::uint32_t digit = static_cast<std::uint32_t>(G.numberValue) / divisor;
    if (static_cast<std::uint16_t>(digit) == 0 && G.numberStarted == 0) {
        return 0;
    }
    Picture* picture = select(static_cast<std::uint16_t>(G.numberFont >> 16), static_cast<int>(digit));
    DrawPicture(G.numberX - 0x10, G.numberY, picture->width, picture->height,
                G.digitMode == static_cast<std::uint16_t>(0x8000) ? 0x20 : 0, 0);
    G.numberStarted = 1;
    G.numberValue = static_cast<std::int32_t>(static_cast<std::uint32_t>(G.numberValue) -
                                              divisor * static_cast<std::uint16_t>(digit));
    return G.uiPicture->width;
}

// swd32.exe 0x004509D0: looping animation `id` in one of eight slots
int DrawSlotAnimation(int id, int slot, int x, int y) {
    ActState& state = G.slotActs[slot];
    state.id = id;
    state.action = 0;
    const int result = ActRun(&state);
    if (result == 0) {
        return 0;
    }
    Picture* picture = TswGetPicture(state.fr, state.ap);
    G.drawSource = reinterpret_cast<std::uint8_t*>(picture->pixels);
    return DrawPicture(x, y, picture->width, picture->height, static_cast<int>(state.flags), 0);
}

// swd32.exe 0x00450A50: the pushed colour argument itself is the picture (a raw 16-bit pixel), drawn in mode 8
int FillBox(int x, int y, int width, int height, std::uint32_t color) {
    std::uint32_t slot = color;
    G.drawSource = reinterpret_cast<std::uint8_t*>(&slot);
    return DrawPicture(x, y, width, height, 8, 0);
}

// swd32.exe 0x00450A80: one step of the shared effect animation; returns 1 once it reached its end
int DrawEffectAt(int id, int action, int x, int y, int mirror) {
    ActState& state = G.effectAct;
    state.id = id;
    state.action = action;
    const int result = ActRun(&state);
    if (result == 0) {
        return 0;
    }
    Picture* picture = TswGetPicture(state.fr, state.ap);
    std::uint32_t flags = state.flags;
    std::int16_t offset = static_cast<std::int16_t>(state.x);
    if (mirror == 1) {
        flags = (state.flags & 1) ? state.flags & ~1u : state.flags | 1;
        offset = static_cast<std::int16_t>(picture->width - state.x);
    }
    G.drawSource = reinterpret_cast<std::uint8_t*>(picture->pixels);
    DrawPicture(x - offset, y - state.y, picture->width, picture->height, static_cast<int>(flags), 0);
    return G.effectAct.waiting == 1;
}

// swd32.exe 0x00450B40
int ResetEffectAt() {
    std::memset(&G.effectAct, 0, sizeof(ActState));
    return 0;
}

// swd32.exe 0x00450B60
int DrawCursorAnimation(int id, int x, int y) {
    ActState& state = G.cursorAct;
    state.id = id;
    state.action = 0;
    const int result = ActRun(&state);
    if (result == 0) {
        return 0;
    }
    Picture* picture = TswGetPicture(state.fr, state.ap);
    G.drawSource = reinterpret_cast<std::uint8_t*>(picture->pixels);
    return DrawPicture(x, y, picture->width, picture->height, static_cast<int>(G.cursorAct.flags), 0);
}

// swd32.exe 0x00450BD0: digit of font 0 (picture 0x2359) or 1 (0x2358); other fonts redraw the last digit
std::uint16_t DrawDigit(int font, int digit, int x, int y) {
    Picture* picture;
    if (font == 0) {
        picture = TswGetPicture(0x2359, static_cast<std::uint32_t>(digit));
        G.uiPicture = picture;
        G.drawSource = reinterpret_cast<std::uint8_t*>(picture->pixels);
    } else if (font == 1) {
        picture = TswGetPicture(0x2358, static_cast<std::uint32_t>(digit));
        G.uiPicture = picture;
        G.drawSource = reinterpret_cast<std::uint8_t*>(picture->pixels);
    } else {
        picture = G.uiPicture;
    }
    DrawPicture(x, y, picture->width, picture->height, G.digitMode == 0x4000 ? 0x20 : 0, 0);
    return G.uiPicture->width;
}

namespace {

Picture* runScroll(std::uint16_t id, int action) {
    ActState& state = G.scrollAct;
    state.id = id;
    state.action = action;
    if (ActRun(&state) == 0) {
        return nullptr;
    }
    Picture* picture = TswGetPicture(state.fr, state.ap);
    G.drawSource = reinterpret_cast<std::uint8_t*>(picture->pixels);
    return picture;
}

}  // namespace

// swd32.exe 0x00450C50: scroll bar of act `id`: cap (action 0x1A / 0x1E), `rows` track pieces (0x18), thumb (0x19)
// sized 7 / G.scrollCount of the track and placed by G.scrollFirst, bottom cap (0x1B / 0x1F)
int DrawScrollBar(std::uint16_t id, int x, int y, int rows, int offset, int style) {
    ActState& state = G.scrollAct;
    Picture* cap = runScroll(id, style == 1 ? 0x1E : 0x1A);
    if (cap == nullptr) {
        return 0;
    }
    DrawPicture(x, y, cap->width, cap->height, static_cast<int>(state.flags), 0);
    const int capHeight = cap->height;
    const int trackTop = capHeight + y;
    std::memset(&state, 0, sizeof(state));
    Picture* piece = runScroll(id, 0x18);
    if (piece == nullptr) {
        return 0;
    }
    int row = 0;
    for (; row < rows; ++row) {
        DrawPicture(x + 2, y + capHeight + row * piece->height, piece->width, piece->height,
                    static_cast<int>(state.flags), 0);
    }
    const std::uint16_t pieceHeight = piece->height;
    const int bottomOffset = row * pieceHeight + capHeight;
    const int track = rows * pieceHeight;
    std::memset(&state, 0, sizeof(state));
    Picture* thumb = runScroll(id, 0x19);
    if (thumb == nullptr) {
        return 0;
    }
    const auto thumbLength =
        static_cast<std::int32_t>(crt::ftol(7.0 / static_cast<double>(G.scrollCount) * static_cast<double>(track)));
    G.scrollStep = (track - thumbLength) / (G.scrollCount - 7);
    const int thumbTop = offset + G.scrollStep * G.scrollFirst + trackTop;
    int thumbBottom = offset + trackTop + G.scrollStep * G.scrollFirst + thumbLength;
    G.scrollThumbY = thumbTop;
    if (thumbBottom - thumbTop < 5) {
        thumbBottom = thumbTop + 5;
    }
    if (G.scrollFirst + 7 >= G.scrollCount) {
        thumbBottom = trackTop + track;
    }
    G.scrollTrackTop = trackTop;
    G.scrollThumbTop = thumbTop;
    G.scrollThumbBottom = thumbBottom;
    G.scrollTrackBottom = trackTop + track;
    SetClipRectClamped(x, y, x + 0x20, thumbBottom);
    if (thumbLength > 0) {
        int drawn = 0;
        int height = thumb->height;
        do {
            DrawPicture(x + 5, drawn + G.scrollThumbY, thumb->width, height, static_cast<int>(state.flags), 0);
            height = thumb->height;
            drawn += height;
        } while (drawn < thumbLength);
    }
    SetClipRectClamped(0, 0, 0x280, 0x1E0);
    std::memset(&state, 0, sizeof(state));
    Picture* bottom = runScroll(id, style == 2 ? 0x1F : 0x1B);
    if (bottom == nullptr) {
        return 0;
    }
    return DrawPicture(x, y + bottomOffset, bottom->width, bottom->height, static_cast<int>(state.flags), 0);
}

// swd32.exe 0x00450F90: the coin of act 0x2329 flips between two positions, slowing down every 25 frames; returns 1
// once the delay reaches 64 (a coin that ended on side 1 counts in the high word of G.coinState)
int DrawCoinToss() {
    if (G.coinFrames == 0) {
        if (G.coinState >> 16) {
            G.coinState = 0;
        } else {
            G.coinState = (G.coinState & 0xFFFF0000u) | static_cast<std::uint16_t>(R250Below(2));
        }
    }
    ActState& state = G.scrollAct;
    state.id = 0x2329;
    state.action = 2;
    if (static_cast<std::uint16_t>(G.coinState) == 1) {
        state.action = 3;
    }
    if (ActRun(&state) == 0) {
        return 0;
    }
    Picture* picture = TswGetPicture(state.fr, 0);
    G.drawSource = reinterpret_cast<std::uint8_t*>(picture->pixels);
    DrawPicture(0x32 * static_cast<std::uint16_t>(G.coinState) + 0x104, 0xC8, picture->width, picture->height,
                static_cast<int>(state.flags), 0);
    std::uint16_t delay;
    if (++G.coinFrames % 0x19 != 0) {
        delay = static_cast<std::uint16_t>(G.coinDelay);
    } else {
        delay = static_cast<std::uint16_t>(2 * G.coinDelay);
        G.coinDelay = (static_cast<std::uint32_t>(delay) << 16) | delay;
    }
    if (delay < 0x40) {
        std::uint16_t countdown = static_cast<std::uint16_t>(G.coinDelay >> 16);
        if (--countdown == 0) {
            countdown = delay;
            G.coinState = (G.coinState & 0xFFFF0000u) | (static_cast<std::uint16_t>(G.coinState) == 0);
            PlaySoundAt(0x2E, 1);
        }
        G.coinDelay = (G.coinDelay & 0xFFFF) | (static_cast<std::uint32_t>(countdown) << 16);
        return 0;
    }
    G.coinDelay = 0x10001;
    G.coinFrames = 0;
    if (static_cast<std::uint16_t>(G.coinState) == 1) {
        G.coinState += 0x10000;
    }
    std::memset(&state, 0, sizeof(state));
    return 1;
}

// swd32.exe 0x00451100: timing bar (picture 0x234F) with three marks and a moving needle; G.timingHit is the mark
// (1-based) under the needle unless it equals the low word of G.numberFont, which is cleared otherwise. Returns 1
// when the needle reaches the end.
int DrawTimingBar(int x, int y) {
    Picture* back = TswGetPicture(0x234F, 0);
    G.uiPicture = back;
    G.drawSource = reinterpret_cast<std::uint8_t*>(back->pixels);
    DrawPicture(x, y, back->width, back->height, 0, argOf(back));
    G.timingHit = 0;
    for (int mark = 0; mark < 3; ++mark) {
        const std::uint16_t position = G.timingMarks[mark];
        SetClipRectClamped(x + position, y, x + position + 1, y + G.uiPicture->height);
        Picture* line = select(0x234F, 1);
        DrawPicture(x, y, line->width, line->height, 0, argOf(line));
        const int needle = (static_cast<std::uint16_t>(G.timingCounter) >> 1) + 1;
        if (needle >= position && needle <= position + 2) {
            G.timingHit = static_cast<std::uint16_t>(mark + 1);
            if (static_cast<std::uint16_t>(G.numberFont) == mark + 1) {
                G.timingHit = 0;
            } else {
                G.numberFont &= 0xFFFF0000u;
            }
        }
    }
    const int needle = static_cast<std::uint16_t>(G.timingCounter) >> 1;
    SetClipRectClamped(needle + x + 1, y, needle + x + 2, y + G.uiPicture->height);
    Picture* line = select(0x234F, 1);
    DrawPicture(x, y, line->width, line->height, 0, argOf(line));
    SetClipRectClamped(0, 0, 0x280, 0x1E0);
    G.timingCounter = (G.timingCounter & 0xFFFF0000) | static_cast<std::uint16_t>(G.timingCounter + 1);
    if ((static_cast<std::uint16_t>(G.timingCounter) >> 1) != 0x3D) {
        return 0;
    }
    G.timingCounter = (G.timingCounter & 0xFFFF0000) | 0x8000;
    return 1;
}

// swd32.exe 0x004512B0: vertical gauge of picture 0x241A showing `level` sixths of its body
int DrawTallGauge(int x, int y, int level) {
    Picture* body = TswGetPicture(0x241A, 2);
    G.uiPicture = body;
    const int shown = level * (body->height / 6);
    Picture* top = select(0x241A, 0);
    DrawPicture(x, y, top->width, top->height, 0, argOf(top));
    const int bodyTop = G.uiPicture->height + y;
    if (level < 6) {
        SetClipRectClamped(x, bodyTop, x + G.uiPicture->width, shown + bodyTop);
    }
    body = select(0x241A, 2);
    DrawPicture(x + 4, bodyTop, body->width, body->height, 0, argOf(body));
    Picture* shine = select(0x241A, 3);
    G.drawAlpha = 8;
    DrawPicture(x + 0xB, bodyTop + 0x1F, shine->width, shine->height, 0x14, argOf(shine));
    SetClipRectClamped(0, 0, 0x280, 0x1E0);
    Picture* bottom = select(0x241A, 1);
    return DrawPicture(x, bodyTop + shown, bottom->width, bottom->height, 0, argOf(bottom));
}

// swd32.exe 0x00451540: one step of the animation with the picture loaded for its frame; returns ActState.waiting
int BattleAnimationDraw(std::uint8_t* animation) {
    const std::uint16_t id = mem::u16(animation, 0xC0);
    if (id == 0) {
        return 0;
    }
    auto* state = reinterpret_cast<ActState*>(animation);
    state->id = id;
    state->action = 0;
    ActRun(state);
    Picture* picture = mem::ptr<Picture>(animation, 0x9C + 4 * state->ap);
    G.drawSource = reinterpret_cast<std::uint8_t*>(picture->pixels);
    G.waveMode = mem::s32(animation, 0xBC);
    DrawPicture(mem::s32(animation, 0xB4) - state->x, mem::s32(animation, 0xB8) - state->y, picture->width,
                picture->height, static_cast<int>(state->flags), 0);
    G.waveMode = 0;
    return state->waiting;
}

// swd32.exe 0x00451730
int BattleAnimationFree(std::uint8_t* animation) {
    for (int i = 0; i < 6; ++i) {
        const std::intptr_t slot = 0x9C + 4 * i;
        auto* picture = mem::ptr<Picture>(animation, slot);
        if (picture != nullptr) {
            if (picture->pixels != nullptr) {
                crt::free(picture->pixels);
                picture->pixels = nullptr;
            }
            crt::free(picture);
            mem::put32(animation, slot, 0);
        }
    }
    mem::put16(animation, 0xC0, 0);
    mem::put32(animation, 0xBC, 0);
    std::memset(animation, 0, sizeof(ActState));
    return 0;
}

// swd32.exe 0x00451420: loads every frame picture of act `id` (run to its end) into the animation, each shifted
// right by 640 / `divisor`; the wave mode of 'WT' is kept at +0xBC. `unused` is ignored. Always returns 0.
int BattleAnimationLoad(std::uint8_t* animation, int unused, int x, int y, std::uint16_t id, std::uint16_t divisor) {
    static_cast<void>(unused);
    std::uint16_t loaded[6] = {0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
    auto* state = reinterpret_cast<ActState*>(animation);
    mem::put16(animation, 0xC0, id);
    mem::put32(animation, 0xB4, static_cast<std::uint32_t>(x));
    mem::put32(animation, 0xB8, static_cast<std::uint32_t>(y));
    state->id = id;
    state->action = 0;
    if (ActRun(state) == 0) {
        return 0;
    }
    for (;;) {
        if (state->wt != 0) {
            mem::put32(animation, 0xBC, state->wt);
        }
        const std::uint16_t frame = state->ap;
        if (loaded[frame] == 0xFFFF) {
            Picture* picture = TswLoadPictureDirect(state->fr, state->ap);
            mem::putPtr(animation, 0x9C + 4 * state->ap, picture);
            loaded[state->ap] = state->ap;
            ShiftPicture(static_cast<std::uint16_t*>(mem::ptr<Picture>(animation, 0x9C + 4 * state->ap)->pixels),
                         kShiftRight, 0x280 / divisor);
        }
        if (state->pc == 0) {
            break;
        }
        state->action = 0;
        state->id = mem::u16(animation, 0xC0);
        if (ActRun(state) == 0) {
            return 0;
        }
    }
    std::memset(animation, 0, sizeof(ActState));
    return 0;
}

// swd32.exe 0x004515E0: plays the loaded animation to its end in one call, every new frame shifted by `amount`
// (right when positive, left when negative) and drawn; returns 1 when it ended, 0 without an animation. `unused`
// is ignored.
int BattleAnimationPlayShifted(std::uint8_t* animation, int unused, int amount) {
    static_cast<void>(unused);
    std::uint16_t shown[6] = {0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
    if (mem::u16(animation, 0xC0) == 0) {
        return 0;
    }
    auto* state = reinterpret_cast<ActState*>(animation);
    std::memset(animation, 0, sizeof(ActState));
    state->action = 0;
    state->id = mem::u16(animation, 0xC0);
    if (ActRun(state) == 0) {
        return 0;
    }
    for (;;) {
        const std::uint16_t frame = state->ap;
        if (shown[frame] == 0xFFFF) {
            shown[frame] = frame;
            if (amount != 0) {
                auto* pixels = static_cast<std::uint16_t*>(mem::ptr<Picture>(animation, 0x9C + 4 * frame)->pixels);
                if (amount > 0) {
                    ShiftPicture(pixels, kShiftRight, amount);
                } else {
                    ShiftPicture(pixels, kShiftLeft, -amount);
                }
            }
            Picture* picture = mem::ptr<Picture>(animation, 0x9C + 4 * state->ap);
            G.drawSource = reinterpret_cast<std::uint8_t*>(picture->pixels);
            Picture* current = mem::ptr<Picture>(animation, 0x9C + 4 * state->ap);
            DrawPicture(mem::s32(animation, 0xB4) - state->x, mem::s32(animation, 0xB8) - state->y, current->width,
                        current->height, static_cast<int>(state->flags), 0);
        }
        const bool ended = state->pc == 0;
        state->delay = 0;
        state->frameDelay = 0;
        if (ended) {
            break;
        }
        state->action = 0;
        state->id = mem::u16(animation, 0xC0);
        if (ActRun(state) == 0) {
            return 0;
        }
    }
    std::memset(animation, 0, sizeof(ActState));
    return 1;
}

}  // namespace swd3
