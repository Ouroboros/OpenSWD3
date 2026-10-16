#include <algorithm>
#include <climits>
#include <cstring>

#include "core/debug_log.h"
#include "core/format.h"
#include "game/game.h"
#include "talk/script_text.h"
#include "talk/talk.h"

namespace swd3
{

namespace
{

constexpr std::size_t kTextBytes = 0x200;
constexpr int kExpandLimit = 0x1FE;     // the bytes a replacement may grow the text to
constexpr std::size_t kItemDigits = 5;  // "%T" with this many digits or more fails
constexpr int kSkippedCodeBytes = 4;    // an object name code that fails is passed over by this much

// The script records of the talk windows: the speaker and the frame act, then by layout the place (Sized), the size
// in cells (Sized, Record), then the text.
constexpr std::size_t kSpeechWords = 2;
constexpr std::size_t kRecordWords = 4;
constexpr std::size_t kSizedWords = 6;
constexpr std::size_t kPlaceWord = 2;
constexpr std::size_t kSizedWidthWord = 4;
constexpr std::size_t kRecordWidthWord = 2;

// The window sizes in cells by the lines of the text.
constexpr u32 kMediumLines = 4;
constexpr int kSmallColumns = 0x10;
constexpr int kSmallRows = 6;
constexpr int kMediumColumns = 0x12;
constexpr int kMediumRows = 8;
constexpr int kLargeColumns = 0x14;
constexpr int kLargeRows = 0xA;

constexpr u16 kTextColor = 4;  // of the text tables
constexpr u16 kOverlayColor = 5;
constexpr int kOverlayLiftDown = 0x20;  // an overlay text is moved off its speaker
constexpr int kOverlayLiftUp = 0x30;
constexpr int kNarratorX = 0x40;  // role 0 speaks from here on the screen
constexpr int kNarratorY = 0x10;
constexpr u16 kLineMargin = 0x10;  // the screen margin of a single line
constexpr int kAboveSpeaker = 0x68;

// The bounds a window is kept in, and where it may go when it would cover its speaker.
constexpr int kWindowLeft = 0x1E;
constexpr int kWindowTop = 0x28;
constexpr int kWindowRight = 0x240;
constexpr int kWindowBottom = 0x1C8;
constexpr int kSpeakerLeft = 0x10;
constexpr int kSpeakerRight = 0x30;
constexpr u32 kSpeakerAbove = 0x10;
constexpr u32 kSpeakerBelow = 0x40;
constexpr u16 kMovedLeft = 0x18;
constexpr u16 kMovedTop = 0x20;

u16 ReadWord(std::span<const u8> bytes, std::size_t at)
{
    const u16 low = at < bytes.size() ? bytes[at] : u16{0};
    const u16 high = at + 1 < bytes.size() ? bytes[at + 1] : u16{0};
    return static_cast<u16>(low | high << CHAR_BIT);
}

u16 ReadWord(const std::string& text, std::size_t at)
{
    return ReadWord(std::span<const u8>(reinterpret_cast<const u8*>(text.data()), text.size()), at);
}

// The text up to its NUL from `at`.
std::string_view CString(const std::string& text, std::size_t at)
{
    if (at >= text.size())
    {
        return {};
    }

    const std::string_view rest = std::string_view(text).substr(at);
    return rest.substr(0, rest.find('\0'));
}

}  // namespace

// swd32.exe 0x0040BAA0: when the text at `at` starts with `pattern`, the pattern is
// replaced by `replacement`, moving `capacity` bytes less the replacement's length after it (past the buffer's end
// the original reads the heap; here those bytes are 0)
bool Talk::ReplacePrefix(
    std::string& text,
    std::size_t at,
    std::string_view pattern,
    std::string_view replacement,
    int capacity
)
{
    if (pattern.empty())
    {
        return false;
    }

    for (std::size_t index = 0; index < pattern.size(); ++index)
    {
        if (at + index >= text.size() || text[at + index] != pattern[index])
        {
            return false;
        }
    }

    const std::size_t length = replacement.size();
    const auto moved = static_cast<std::ptrdiff_t>(capacity) - static_cast<std::ptrdiff_t>(length);

    if (moved > 0)
    {
        std::string tail(static_cast<std::size_t>(moved), '\0');

        for (std::size_t index = 0; index < tail.size(); ++index)
        {
            const std::size_t from = at + pattern.size() + index;
            tail[index] = from < text.size() ? text[from] : '\0';
        }

        for (std::size_t index = 0; index < tail.size() && at + length + index < text.size(); ++index)
        {
            text[at + length + index] = tail[index];
        }
    }

    for (std::size_t index = 0; index < length && at + index < text.size(); ++index)
    {
        text[at + index] = replacement[index];
    }

    return true;
}

// swd32.exe 0x0040B7F0: the names in a talk text: "%T<object>." becomes the object's name (numbers of 5 digits and
// more fail), the default names of the first two roles become their names (both advancing by the first name's
// length); then the text is measured: the characters and the lines (20 characters a line; %N, %L, %K and %P end a
// line; %B and %A skip only their code) as lines << 16 | characters
u32 Talk::ExpandText(std::string& text)
{
    Game& owner = this->game;
    std::size_t at = 0;
    int consumed = 0;
    const std::string firstDefault = Big5(Party::RoleNames[0]);
    const std::string secondDefault = Big5(Party::RoleNames[1]);
    const std::string_view firstName(owner.party.names[0].data(), std::strlen(owner.party.names[0].data()));
    const std::string_view secondName(owner.party.names[1].data(), std::strlen(owner.party.names[1].data()));

    while (ReadWord(text, at) != kTextEnd)
    {
        if (at >= text.size() || text[at] == '\0')
        {
            break;
        }

        if (ReadWord(text, at) == kItemName)
        {
            std::string code = "%T";
            std::size_t digit = at + kCodeBytes;

            while (digit < text.size() && text[digit] != '.')
            {
                code.push_back(text[digit]);
                ++digit;
            }

            std::string number = code.substr(kCodeBytes);

            if (code.size() >= kCodeBytes + kItemDigits)
            {
                number.push_back('.');
                DebugLog(u8"物品名稱代換", code);
            }

            int id = static_cast<int>(code.size());

            if (Talk::ParseNumber(number, id))
            {
                ObjectRecord record;

                if (owner.database.LoadObject(record, static_cast<u16>(id)))
                {
                    const auto* raw = reinterpret_cast<const char*>(&record.data);
                    const std::string_view name(raw, strnlen(raw, sizeof(record.data)));
                    Talk::ReplacePrefix(text, at, code + ".", name, kExpandLimit - consumed);
                    at += name.size();
                    consumed += static_cast<int>(name.size());
                }
                else
                {
                    DebugLog(u8"物品名稱代換", id);
                    at += kSkippedCodeBytes;
                    consumed += kSkippedCodeBytes;
                }
            }
            else
            {
                DebugLog(u8"物品名稱代換", code);
                at += kSkippedCodeBytes;
                consumed += kSkippedCodeBytes;
            }

            continue;
        }

        int skip = 0;

        if (Talk::ReplacePrefix(text, at, firstDefault, firstName, kExpandLimit - consumed) ||
            Talk::ReplacePrefix(text, at, secondDefault, secondName, kExpandLimit - consumed))
        {
            skip = static_cast<int>(firstName.size()) - 1;
        }

        at += static_cast<std::size_t>(skip + 1);
        consumed += skip + 1;
    }

    u32 characters = 0;
    u32 column = 0;
    u32 lines = 1;
    std::size_t cursor = 0;

    if (ReadWord(text, 0) == kTextEnd)
    {
        return characters + (lines << Talk::LinesShift);
    }

    for (;;)
    {
        bool lineEnds = false;

        switch (ReadWord(text, cursor))
        {
            case kNewLine:
            case kLine:
            case kKey:
            case kPage:
                cursor += kCodeBytes;
                lineEnds = true;
                break;

            case kSpeed:
            case kColor:
            case kShadeCode:
                cursor += kCodeDigitBytes;
                break;

            case kChoice:
            case kSound:
                cursor += kCodeBytes;
                break;

            default:
                ++cursor;
                ++characters;
                lineEnds = ++column > Talk::LineCharacters;
                break;
        }

        if (lineEnds)
        {
            column = 0;
            ++lines;
        }

        if (cursor >= text.size() || text[cursor] == '\0' || ReadWord(text, cursor) == kTextEnd)
        {
            return characters + (lines << Talk::LinesShift);
        }
    }
}

// swd32.exe 0x0040BB20: a talk window put at the end of the list
void Talk::AppendWindow(std::unique_ptr<TalkWindow> window)
{
    std::unique_ptr<TalkWindow>* link = &this->game.scene.talkWindows;

    while (*link != nullptr)
    {
        link = &(*link)->next;
    }

    window->next.reset();
    *link = std::move(window);
}

// swd32.exe 0x0040AF70: a message in a plain window (frame act 0x232D, 40 x 10 cells, no name) over role `role`
void Talk::ShowMessage(u16 role, std::u8string_view message)
{
    constexpr u16 kFrameAct = 0x232D;
    constexpr u16 kCellX = 4;
    constexpr u16 kCellY = 3;
    constexpr u16 kColumns = 0x28;
    constexpr u16 kRows = 0xA;
    constexpr std::size_t kTextAt = kWordBytes * kSizedWords;  // the text after the header words
    constexpr std::size_t kRecordBytes = 0x100;
    const std::string text = Big5(message);
    std::array<u8, kRecordBytes> record = {};
    const std::array<u16, kSizedWords> header = {role, kFrameAct, kCellX, kCellY, kColumns, kRows};

    for (std::size_t index = 0; index < header.size(); ++index)
    {
        record[kWordBytes * index] = static_cast<u8>(header[index]);
        record[kWordBytes * index + 1] = static_cast<u8>(header[index] >> CHAR_BIT);
    }

    std::copy_n(
        text.begin(),
        std::min<std::size_t>(text.size(), record.size() - kTextAt - 1),
        record.begin() + kTextAt
    );
    auto window = std::make_unique<TalkWindow>();
    this->SetupDialog(*window, record, "", Talk::SizedLayout, 0, 0);
    this->AppendWindow(std::move(window));
}

// swd32.exe 0x0040AFF0: a talk window from a script record (role, frame act, place, text ending with "%Q"): the text is
// copied into a 0x200-byte buffer, its names put in and measured; the size follows the text (16 x 6, 18 x 8 or 20 x 10
// cells), a single line (option 0x10000), or the record (options 1 and 2); the window grows out of its speaker and is
// placed beside it by the way it faces, kept on the screen and away from the speaker. Option 0x20000 keeps the text
// in the talk log, 0x40000 centres it across, 0x8000 places it by (offsetX, offsetY). False for an unknown speaker.
bool Talk::SetupDialog(
    TalkWindow& window,
    std::span<const u8> record,
    std::string_view name,
    u32 options,
    i16 offsetX,
    i16 offsetY
)
{
    Game& owner = this->game;
    GameState& state = owner.state;
    const Field& field = owner.field;
    const int unit = this->unit;
    const bool hasOrigin = window.fromX != 0 || window.fromY != 0;
    const bool fixed = (window.flags & TalkWindow::FixedPlace) != 0;
    const u32 mode = options & Talk::LayoutMask;
    const bool singleLine = (options & Talk::OneLine) != 0;
    std::size_t textAt = kWordBytes * kSpeechWords;

    auto word = [&record](std::size_t index)
    {
        return ReadWord(record, kWordBytes * index);
    };

    if (mode == Talk::SizedLayout)
    {
        textAt = kWordBytes * kSizedWords;
    }

    if (mode == Talk::RecordLayout)
    {
        textAt = kWordBytes * kRecordWords;
    }

    if (singleLine && fixed)
    {
        textAt = kWordBytes * kSpeechWords;
    }

    std::size_t end = textAt;

    while (end + 1 < record.size() && ReadWord(record, end) != kTextEnd)
    {
        ++end;
    }

    window.text.assign(kTextBytes, '\0');
    std::copy_n(
        record.begin() + static_cast<std::ptrdiff_t>(textAt),
        std::min(end - textAt + kWordBytes, kTextBytes),
        window.text.begin()
    );
    window.page = 0;
    window.typed = 0;
    const u32 measured = this->ExpandText(window.text);
    const u32 lines = measured >> Talk::LinesShift;
    const u32 characters = measured & ((1u << Talk::LinesShift) - 1);

    if ((options & Talk::LogText) != 0)
    {
        this->history.emplace_back(CString(window.text, 0));
    }

    bool sized = true;

    if (singleLine)
    {
        window.width = static_cast<u16>(characters * static_cast<u32>(unit));
        window.height = static_cast<u16>(Talk::LineCells * unit);
    }
    else if (mode == Talk::SizedLayout)
    {
        window.width = static_cast<u16>(unit * word(kSizedWidthWord));
        window.height = static_cast<u16>(unit * word(kSizedWidthWord + 1));
    }
    else if (mode == Talk::RecordLayout)
    {
        window.width = static_cast<u16>(unit * word(kRecordWidthWord));
        window.height = static_cast<u16>(unit * word(kRecordWidthWord + 1));
    }
    else
    {
        sized = false;
    }

    if (!sized)
    {
        if (lines > kMediumLines)
        {
            window.width = static_cast<u16>(kLargeColumns * unit);
            window.height = static_cast<u16>(kLargeRows * unit);
        }
        else if (lines == kMediumLines)
        {
            window.width = static_cast<u16>(kMediumColumns * unit);
            window.height = static_cast<u16>(kMediumRows * unit);
        }
        else
        {
            window.width = static_cast<u16>(kSmallColumns * unit);
            window.height = static_cast<u16>(kSmallRows * unit);
        }
    }

    u32 speaker = TalkWindow::NoRole;

    if (word(0) != TalkWindow::NoRole && !fixed && !state.FindRoleOrPlayer(word(0), speaker))
    {
        DebugLog(u8"設定對話窗(SetupDialog)", word(0));
        window = TalkWindow{};
        window.page = TalkWindow::Closed;
        return false;
    }

    std::size_t frame = 0;

    while (frame < owner.scene.unk4CAA78.size() && owner.scene.unk4CAA78[frame].id != word(1))
    {
        ++frame;
    }

    if (frame == owner.scene.unk4CAA78.size())
    {
        DebugLog(u8"SetupDialog", word(1));
        frame = 0;
    }

    window.frameAct = &owner.scene.unk4CAA78[frame];
    owner.acts.Run(owner.scene.unk4CAA78[frame]);

    if (!name.empty())
    {
        window.nameAct = &owner.scene.unk4A9120[frame];
        window.name = std::string(name.substr(0, this->dialogName.size() - 1));
        owner.acts.Run(owner.scene.unk4A9120[frame]);
    }

    window.role = static_cast<u16>(speaker);
    window.openStep = 0;
    window.unk26 = 0;
    window.delay = 0;
    window.speed = static_cast<u16>(Talk::SpeedScale * owner.config.textSpeed);
    window.color = kTextColor;
    window.shade = kTextColor;
    window.style = TextFont::BoldShadow;
    window.next.reset();
    int lift = 0;

    if ((window.flags & TalkWindow::Overlay) != 0)
    {
        window.color = kOverlayColor;
        window.shade = kOverlayColor;

        if (window.role != TalkWindow::NoRole && window.role < GameState::Roles)
        {
            const auto facing =
                static_cast<std::size_t>(state.roles[window.role].act.direction) % Talk::WindowShiftY.size();
            lift = Talk::WindowShiftY[facing] <= 0 ? kOverlayLiftDown : -kOverlayLiftUp;
        }
    }

    if (window.role == 0)
    {
        state.roles[0].x = field.cameraLeft + kNarratorX;
        state.roles[0].y = field.cameraTop + kNarratorY;
    }

    window.x = word(kPlaceWord);
    window.y = word(kPlaceWord + 1);

    if ((options & Talk::Centred) != 0)
    {
        window.x = static_cast<u16>(window.x - (window.width >> 1));
    }

    if (singleLine && fixed)
    {
        const u16 height = window.height;
        // The original tests the far edges the wrong way round, pushing short lines to the right and bottom.
        window.x = static_cast<u16>(offsetY - height - kLineMargin);

        if (window.x < kLineMargin)
        {
            window.x = kLineMargin;
        }

        if (window.y < kLineMargin)
        {
            window.y = kLineMargin;
        }

        if (window.width + window.x < Display::Width - 2 * kLineMargin)
        {
            window.x = static_cast<u16>(Display::Width - 2 * kLineMargin - window.width);
        }

        if (window.y + height < Display::Height - kLineMargin)
        {
            window.y = static_cast<u16>(Display::Height - kLineMargin - height);
        }

        return true;
    }

    if (mode != Talk::SpeechLayout && mode != Talk::RecordLayout)
    {
        return true;
    }

    if (!hasOrigin)
    {
        if (window.role == TalkWindow::NoRole)
        {
            window.fromX = static_cast<u16>(state.leader.x);
            window.fromY = static_cast<u16>(state.leader.y);
            window.x = static_cast<u16>(window.fromX - (window.width >> 1) - field.cameraLeft);
            window.y = static_cast<u16>(window.fromY - (window.height >> 1) - field.cameraTop);
        }
        else if (window.role != 0 && window.role < GameState::Roles)
        {
            const Role& role = state.roles[window.role];
            window.x = static_cast<u16>(static_cast<u16>(role.x) - (window.width >> 1) - field.cameraLeft);
            window.y = static_cast<u16>(static_cast<u16>(role.y) - (window.height >> 1) - field.cameraTop);
        }
    }

    const std::size_t facing =
        window.role < GameState::Roles
            ? static_cast<std::size_t>(state.roles[window.role].act.direction) % Talk::WindowShiftX.size()
            : 0;

    if (fixed || window.role == TalkWindow::NoRole)
    {
        window.y = static_cast<u16>(window.y - kAboveSpeaker);
    }
    else if ((options & Talk::UseOffset) == 0)
    {
        window.x = static_cast<u16>(window.x + Talk::WindowShiftX[facing]);
        window.y = static_cast<u16>(window.y + lift + Talk::WindowShiftY[facing]);
    }
    else
    {
        window.x = static_cast<u16>(window.x + offsetX);
        window.y = static_cast<u16>(window.y + offsetY);
    }

    auto clamp = [&window](int left, int top)
    {
        if (static_cast<i16>(window.x) < left)
        {
            window.x = static_cast<u16>(left);
        }

        if (window.width + window.x >= kWindowRight)
        {
            window.x = static_cast<u16>(kWindowRight - window.width);
        }

        if (static_cast<i16>(window.y) < top)
        {
            window.y = static_cast<u16>(top);
        }

        if (window.height + window.y >= kWindowBottom)
        {
            window.y = static_cast<u16>(kWindowBottom - window.height);
        }
    };

    clamp(kWindowLeft, kWindowTop);

    if ((options & Talk::UseOffset) == 0 && (window.flags & TalkWindow::Overlay) == 0 &&
        window.role != TalkWindow::NoRole && window.role < GameState::Roles)
    {
        const Role& role = state.roles[window.role];
        const auto screenX = static_cast<u32>(role.x - field.cameraLeft);
        const int left = window.x;

        if (screenX > static_cast<u32>(left - kSpeakerLeft) &&
            screenX < static_cast<u32>(left + window.width + kSpeakerRight))
        {
            const u16 top = window.y;
            const auto screenY = static_cast<u32>(role.y - field.cameraTop);

            if (screenY > static_cast<u32>(top) - kSpeakerAbove &&
                screenY < static_cast<u32>(top) + window.height + kSpeakerBelow)
            {
                window.x = static_cast<u16>(window.x + Talk::WindowShiftX[facing]);
                window.y = static_cast<u16>(top + Talk::WindowShiftY[facing]);

                if (static_cast<i16>(window.x) < kMovedLeft)
                {
                    window.x = kMovedLeft;
                }

                if (window.width + window.x >= kWindowRight)
                {
                    window.x = static_cast<u16>(kWindowRight - window.width);
                }

                if (static_cast<i16>(window.y) < kMovedTop)
                {
                    window.y = kMovedTop;
                }

                if (window.height + window.y >= kWindowBottom)
                {
                    window.y = static_cast<u16>(kWindowBottom - window.height);
                }
            }
        }
    }

    return true;
}

// swd32.exe 0x0042E430: the script continues at `offset` of the open talk file (0x8000 bytes read from it)
bool Talk::SeekScript(Role& context, u32 offset)
{
    this->game.AudioPump();
    context.unk14 = static_cast<i32>(offset);
    context.unk20 = 0;
    this->game.databases.story.SeekBegin(static_cast<i32>(offset + Talk::FileHeaderBytes));
    u32 bytes = static_cast<u32>(this->game.databases.script.size());
    this->game.databases.story.Read(this->game.databases.script.data(), bytes);
    return true;
}

// swd32.exe 0x0042E480: script `index` of Talk<index / 2000 + 1>.dat (its offset from the file's table); a file that
// cannot be opened ends the game
bool Talk::OpenScript(Role& context, u32 index)
{
    Game& owner = this->game;
    this->game.databases.story.Close();
    const std::string path =
        WinFormat("%sTalk%d.dat", {owner.config.installDirectory, index / Talk::EventsPerFile + 1});
    owner.AudioPump();

    if (!this->game.databases.story.Open(path, File::Disposition::OpenExisting, File::Access::Read))
    {
        DebugLog(u8"檔案開啟", u8"劇情資料(1)無法讀取/%s");
        owner.Destroy();
        return false;
    }

    this->game.databases.story.SeekBegin(
        static_cast<i32>(sizeof(u32) * (index % Talk::EventsPerFile) + Talk::FileHeaderBytes)
    );
    u32 offset = 0;
    this->game.databases.story.ReadDword(offset);
    context.unk14 = static_cast<i32>(offset);
    context.unk20 = 0;
    owner.AudioPump();
    this->game.databases.story.SeekBegin(static_cast<i32>(offset + Talk::FileHeaderBytes));
    u32 bytes = static_cast<u32>(this->game.databases.script.size());
    this->game.databases.story.Read(this->game.databases.script.data(), bytes);
    owner.AudioPump();
    return true;
}

// swd32.exe 0x0042E740: whether a command (0xA, 0xB or 0x2D) names role `role`
bool Talk::CommandsRole(std::span<const u8> command, u32 role)
{
    const auto code = static_cast<EventCommand>(ReadWord(command, 0));

    if (code != EventCommand::SetAction && code != EventCommand::SetDirection && code != EventCommand::SetAct)
    {
        return false;
    }

    u32 found = 0;
    return this->game.state.FindRoleOrPlayer(ReadWord(command, kOpcodeBytes), found) && found == role;
}

}  // namespace swd3
