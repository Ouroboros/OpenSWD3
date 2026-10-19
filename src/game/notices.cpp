#include "game/notices.h"

#include <cstring>

#include "draw/painter.h"
#include "font/text_font.h"
#include "game/game_state.h"
#include "talk/text_colors.h"

namespace swd3
{

namespace
{

constexpr int kRight = 0x280;
constexpr int kAdvance = 0xB;
constexpr int kLineHeight = 0x18;
constexpr int kTop = 8;
constexpr int kStyle = 0x10;
constexpr std::size_t kNoticeColor = 3;

}  // namespace

// swd32.exe 0x004153D0: right aligned in the small font, one line every 24 pixels (unless flag 0xE hides them); each
// counts its frames down and is freed after its last
void Notices::Draw(TextFont& font, Painter& painter, const GameState& state, const TextColors& colors, bool step)
{
    font.SetBackground(TextFont::NoBackground);
    font.SetShadow(0);
    font.SetBackground(TextFont::NoBackground);
    font.SetShadow(0);
    Notice* previous = &this->head;
    int y = kTop;

    while (previous->next != nullptr)
    {
        Notice& notice = *previous->next;

        if (!state.GameFlag(Notices::HiddenFlag))
        {
            const auto length = static_cast<int>(strnlen(notice.text.data(), notice.text.size()));
            font.DrawText(
                painter.Target(),
                kRight - kAdvance * length,
                y,
                notice.text.data(),
                static_cast<Pixel>(colors.primary[kNoticeColor]),
                kStyle
            );
        }

        y += kLineHeight;

        if (!step)
        {
            previous = &notice;
        }
        else if (--notice.frames == 0)
        {
            previous->next = std::move(notice.next);
        }
        else
        {
            previous = &notice;
        }
    }
}

}  // namespace swd3
