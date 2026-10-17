#include "game/cheat_dialog.h"

#include <windows.h>

#include <commctrl.h>

#include <SDL3/SDL.h>

#include <algorithm>
#include <cstring>
#include <string>

#include "core/debug_log.h"
#include "core/format.h"
#include "core/log.h"
#include "game/game.h"
#include "item/item_list.h"
#include "menu/menu_list.h"
#include "org/object_record.h"
#include "party/party.h"
#include "platform/wide_win.h"
#include "talk/talk.h"
#include "text/big5.h"

namespace swd3
{

namespace
{

// The dialogs of swd32.exe's resources.
constexpr WORD kCheatDialogId = 0x6A;
constexpr WORD kNameDialogId = 0x70;

// The controls of the cheat dialog.
constexpr int kCloseButton = 0x3E8;
constexpr int kTopUpButton = 0x3EB;
constexpr int kAddButton = 0x3EC;
constexpr int kIdField = 0x3ED;
constexpr int kDeleteButton = 0x3EE;
constexpr int kTabControl = 0x3F1;
constexpr int kValueField = 0x3F4;
constexpr int kCountField = 0x3F5;
constexpr int kSetButton = 0x3F6;
constexpr int kList = 0x3F9;
constexpr std::array<int, 4> kEntryButtons = {kValueField, kTopUpButton, kDeleteButton, kAddButton};
constexpr int kIdLimit = 4;
constexpr int kCountLimit = 8;
constexpr int kValueLimit = 4;
constexpr int kFieldBytes = 0xA;  // GetDlgItemTextA reads at most nine bytes
constexpr i16 kFullStock = 0x5A;  // the top up and the counts set fill to 90
constexpr i16 kDeleteAll = -0x400;
constexpr u16 kFirstSkill = 0x5DD;  // the skill tabs take ids 0x5DD..0x7CF
constexpr u16 kSkillsEnd = 0x7D0;
constexpr int kLastVariable = 0x3F;
constexpr int kLastAttribute = 0x10;

// The columns of the list: the name, the count, the id, the value (three on the attributes and variables).
constexpr std::array<std::u8string_view, 4> kColumns = {u8"物品", u8"數量", u8"號碼", u8"附加值"};  // 0x004A020C..
constexpr std::array<int, 4> kColumnWidths = {0x78, 0x40, 0x40, 0x60};
constexpr int kColumnTextSize = 4;
constexpr std::u8string_view kItemsTab = u8"物品";          // swd32.exe 0x004A020C
constexpr std::u8string_view kAttributesSuffix = u8"屬性";  // the end of "%s屬性" (swd32.exe 0x004A01FC)
constexpr std::u8string_view kVariablesTab = u8"變數";      // swd32.exe 0x004A01F4
constexpr std::u8string_view kGoldLabel = u8"金";           // the first variable (swd32.exe 0x004A0288)
constexpr std::string_view kCompanyA = "Cmpy1";             // variables 11 and 12
constexpr std::string_view kCompanyB = "Cmpy2";
constexpr std::size_t kCompanyAVariable = 11;
constexpr std::size_t kCompanyBVariable = 12;
constexpr std::u8string_view kInsertFailed = u8"物品插入發生錯誤";  // swd32.exe 0x004A0214
constexpr std::u8string_view kDeleteFailed = u8"物品刪除發生錯誤";  // swd32.exe 0x004A023C

// The name dialog.
constexpr int kNameField = 0x40A;
constexpr int kNameCancel = 0x40C;
constexpr int kNameBytes = 8;
constexpr UINT kNameUnknownMessage = 0x46A;  // sent to the field as the original does
constexpr WPARAM kNameUnknownWord = 1;
constexpr LPARAM kNameUnknownLong = 0x42;
constexpr std::u8string_view kNoName = u8"請輸入一個字以上";  // swd32.exe 0x004A10E4
constexpr std::u8string_view kNameTitle = u8"姓名";           // swd32.exe 0x004A10F8

std::wstring Wide(std::string_view big5)
{
    const std::string text = Utf8FromBig5(big5);
    const int length = MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
    std::wstring wide(static_cast<std::size_t>(length), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), wide.data(), length);
    return wide;
}

std::wstring Wide(std::u8string_view text)
{
    return Wide(Big5(text));
}

// GetDlgItemTextA(`control`, `bytes`): the text of a field in Big5, at most `bytes` - 1 bytes; false when empty.
bool FieldText(HWND dialog, int control, int bytes, std::string& text)
{
    std::array<wchar_t, 0x100> wide = {};
    const int length = GetDlgItemTextW(dialog, control, wide.data(), static_cast<int>(wide.size()));
    const int size = WideCharToMultiByte(CP_UTF8, 0, wide.data(), length, nullptr, 0, nullptr, nullptr);
    std::string utf8(static_cast<std::size_t>(size), '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide.data(), length, utf8.data(), size, nullptr, nullptr);
    text = Big5FromUtf8(utf8);
    text.resize(std::min(text.size(), static_cast<std::size_t>(bytes - 1)));
    return !text.empty();
}

int ParsedNumber(const std::string& text, int fallback)
{
    int value = fallback;
    Talk::ParseNumber(text, value);
    return value;
}

std::string_view NodeName(const ItemNode& node)
{
    const std::array<char, 0x20>& name = node.record.data.name;
    return std::string_view(name.data(), strnlen(name.data(), name.size()));
}

void SetItemText(HWND list, int row, int column, const std::string& big5)
{
    std::wstring text = Wide(big5);
    // mask, item, sub item, state, state mask, text (positional: the Win32 field names do not follow the style)
    LVITEMW item{0, 0, column, 0, 0, text.data()};
    SendMessageW(list, LVM_SETITEMTEXTW, static_cast<WPARAM>(row), reinterpret_cast<LPARAM>(&item));
}

INT_PTR CALLBACK CheatProcedure(HWND dialog, UINT message, WPARAM word, LPARAM data);
INT_PTR CALLBACK NameProcedure(HWND dialog, UINT message, WPARAM word, LPARAM data);

// The window of the game, the owner of the dialogs.
HWND GameWindow(const CheatDialog& cheat)
{
    SDL_Window* window = cheat.Window();

    if (window == nullptr)
    {
        return nullptr;
    }

    return static_cast<HWND>(
        SDL_GetPointerProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr)
    );
}

// The dialog `id` of swd32.exe's resources run with `procedure` and `context`; 0 when it cannot be.
int RunDialog(CheatDialog& cheat, WORD id, DLGPROC procedure)
{
    const std::wstring wide = Utf16(cheat.installDirectory + std::u8string(Game::Executable));
    HMODULE module = LoadLibraryExW(wide.c_str(), nullptr, LOAD_LIBRARY_AS_DATAFILE | LOAD_LIBRARY_AS_IMAGE_RESOURCE);

    if (module == nullptr)
    {
        LogWarning(
            u8"dialog 0x%X: swd32.exe's resources not readable (%u)",
            static_cast<i32>(id),
            static_cast<u32>(GetLastError())
        );
        return 0;
    }

    INITCOMMONCONTROLSEX controls{sizeof(INITCOMMONCONTROLSEX), ICC_TAB_CLASSES | ICC_LISTVIEW_CLASSES};
    InitCommonControlsEx(&controls);
    const INT_PTR result =
        DialogBoxParamW(module, MAKEINTRESOURCEW(id), GameWindow(cheat), procedure, reinterpret_cast<LPARAM>(&cheat));
    FreeLibrary(module);
    LogInfo(u8"dialog 0x%X result=%d", static_cast<i32>(id), static_cast<i32>(result));
    return result < 0 ? 0 : static_cast<int>(result);
}

}  // namespace

int CheatDialog::Run()
{
    return RunDialog(*this, kCheatDialogId, &CheatProcedure);
}

int CheatDialog::RunNameDialog()
{
    return RunDialog(*this, kNameDialogId, &NameProcedure);
}

namespace
{

// swd32.exe 0x004103C0: the columns of the list (the value column only on the lists of objects)
void AddColumns(const CheatDialog& cheat, HWND list)
{
    const int columns = cheat.tab >= CheatDialog::FirstAttributesTab ? 3 : 4;

    for (int column = 0; column < columns; ++column)
    {
        std::wstring text = Wide(kColumns[static_cast<std::size_t>(column)]);
        // mask, format, width, text, text size, sub item
        LVCOLUMNW entry{
            LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM,
            LVCFMT_LEFT,
            kColumnWidths[static_cast<std::size_t>(column)],
            text.data(),
            kColumnTextSize,
            column
        };

        SendMessageW(list, LVM_INSERTCOLUMNW, static_cast<WPARAM>(column), reinterpret_cast<LPARAM>(&entry));
    }
}

// swd32.exe 0x00410600: row `row` written: the name, the count, the id and the value (CheatDialog::ValueText)
void SetRow(HWND list, int row, std::string_view name, int count, int id, i32 value, i32 maximum)
{
    SetItemText(list, row, 0, std::string(name));
    SetItemText(list, row, 1, WinFormat("%d", {count}));
    const std::string idText = WinFormat("%d", {id});
    SetItemText(list, row, 2, idText);
    SetItemText(list, row, 3, CheatDialog::ValueText(value, maximum, idText));
}

// swd32.exe 0x00410490: row `row` inserted, then written as SetRow does
void InsertRow(HWND list, int row, std::string_view name, int count, int id, i32 value, i32 maximum)
{
    LVITEMW item{};
    item.mask = LVIF_TEXT | LVIF_STATE | LVIF_PARAM;
    item.iItem = row;
    SendMessageW(list, LVM_INSERTITEMW, 0, reinterpret_cast<LPARAM>(&item));
    SetRow(list, row, name, count, id, value, maximum);
}

// swd32.exe 0x00410730: the list of the tab: the objects (with their counters on the items tab) or skills, the
// seventeen attributes of the role, or the 64 variables
void Fill(CheatDialog& cheat, HWND list)
{
    SendMessageW(list, LVM_DELETEALLITEMS, 0, 0);

    if (cheat.tab == CheatDialog::VariablesTab)
    {
        for (std::size_t variable = 0; variable < cheat.party.variables.size(); ++variable)
        {
            std::string label;

            if (variable == 0)
            {
                label = Big5(kGoldLabel);
            }
            else if (variable == kCompanyAVariable)
            {
                label = std::string(kCompanyA);
            }
            else if (variable == kCompanyBVariable)
            {
                label = std::string(kCompanyB);
            }

            const auto index = static_cast<int>(variable);
            InsertRow(list, index, label, cheat.party.variables[variable], index, -1, 0);
        }

        return;
    }

    if (cheat.tab >= CheatDialog::FirstAttributesTab)
    {
        const int role = cheat.tab - CheatDialog::FirstAttributesTab;

        for (std::size_t field = 0; field < CheatDialog::AttributeNames.size(); ++field)
        {
            const auto index = static_cast<int>(field);
            InsertRow(
                list,
                index,
                Big5(CheatDialog::AttributeNames[field]),
                cheat.party.RoleStat(index, role),
                index,
                0,
                0
            );
        }

        return;
    }

    ItemLink* link = cheat.TabList();
    int row = 0;

    for (const ItemNode* node = link != nullptr ? link->get() : nullptr; node != nullptr;
         node = node->next.get(), ++row)
    {
        i32 value = -1;
        i32 maximum = -1;

        if (cheat.tab == CheatDialog::ItemsTab)
        {
            cheat.ObjectCounter(*node, value, maximum);
        }

        InsertRow(list, row, NodeName(*node), node->count + node->pending, node->id, value, maximum);
    }
}

// The fields cleared after an entry changed (part of 0x0040F890)
void ClearFields(HWND dialog)
{
    for (const int field : {kIdField, kCountField, kValueField})
    {
        SetDlgItemTextW(dialog, field, L"");
    }
}

// swd32.exe 0x0040F890: the cheat dialog. A tab chosen shows its list (the buttons off on the attributes and
// variables); the buttons: close (the game runs again), top up the row chosen to 90 (items only), add the id of the
// field with its count and value (a skill tab takes skill ids, one, no value), delete the row chosen, set the count and
// value of the id (always looked up in the items), an attribute or a variable.
INT_PTR CALLBACK CheatProcedure(HWND dialog, UINT message, WPARAM word, LPARAM data)
{
    if (message == WM_INITDIALOG)
    {
        SetWindowLongPtrW(dialog, DWLP_USER, data);
    }

    auto* cheat = reinterpret_cast<CheatDialog*>(GetWindowLongPtrW(dialog, DWLP_USER));

    if (cheat == nullptr)
    {
        return FALSE;
    }

    HWND list = GetDlgItem(dialog, kList);

    switch (message)
    {
        case WM_NOTIFY:
        {
            const auto* header = reinterpret_cast<const NMHDR*>(data);

            if (header->code != static_cast<UINT>(TCN_SELCHANGE))
            {
                return FALSE;
            }

            cheat->tab = static_cast<int>(SendMessageW(header->hwndFrom, TCM_GETCURSEL, 0, 0));

            for (int column = 3; column >= 0; --column)
            {
                SendMessageW(list, LVM_DELETECOLUMN, static_cast<WPARAM>(column), 0);
            }

            AddColumns(*cheat, list);
            Fill(*cheat, list);

            for (const int button : kEntryButtons)
            {
                EnableWindow(GetDlgItem(dialog, button), cheat->tab < CheatDialog::FirstAttributesTab ? TRUE : FALSE);
            }

            return FALSE;
        }

        case WM_INITDIALOG:
        {
            HWND tabs = GetDlgItem(dialog, kTabControl);
            std::array<std::string, CheatDialog::Tabs> labels;
            labels[CheatDialog::ItemsTab] = Big5(kItemsTab);

            for (int role = 0; role < Party::Roles; ++role)
            {
                const Party::Name& name = cheat->party.names[static_cast<std::size_t>(role)];
                const std::string text(name.data(), strnlen(name.data(), name.size()));
                labels[static_cast<std::size_t>(CheatDialog::FirstSkillsTab + role)] = text;
                labels[static_cast<std::size_t>(CheatDialog::FirstAttributesTab + role)] =
                    text + Big5(kAttributesSuffix);
            }

            labels[CheatDialog::VariablesTab] = Big5(kVariablesTab);

            for (std::size_t index = 0; index < labels.size(); ++index)
            {
                std::wstring text = Wide(labels[index]);
                // mask, state, state mask, text
                TCITEMW item{TCIF_TEXT, 0, 0, text.data()};
                SendMessageW(tabs, TCM_INSERTITEMW, index, reinterpret_cast<LPARAM>(&item));
            }

            cheat->tab = CheatDialog::ItemsTab;
            AddColumns(*cheat, list);
            SendMessageW(list, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);
            Fill(*cheat, list);
            SendMessageW(GetDlgItem(dialog, kIdField), EM_LIMITTEXT, kIdLimit, 0);
            SendMessageW(GetDlgItem(dialog, kCountField), EM_LIMITTEXT, kCountLimit, 0);
            SendMessageW(GetDlgItem(dialog, kValueField), EM_LIMITTEXT, kValueLimit, 0);
            return TRUE;
        }

        case WM_COMMAND:
            break;

        default:
            return FALSE;
    }

    std::string text;

    switch (LOWORD(word))
    {
        case kCloseButton:
            cheat->running = 1;
            EndDialog(dialog, 1);
            SDL_HideCursor();
            return FALSE;

        case kTopUpButton:
        {
            if (cheat->tab >= CheatDialog::FirstAttributesTab)
            {
                return FALSE;
            }

            const auto row = static_cast<int>(SendMessageW(list, LVM_GETSELECTIONMARK, 0, 0));

            if (row < 0 || cheat->tab != CheatDialog::ItemsTab)
            {
                return FALSE;
            }

            ItemNode* node = nullptr;
            SkipItems(row, cheat->party.inventory, node);

            if (node == nullptr)
            {
                return FALSE;
            }

            if (node->pending + node->count < kFullStock)
            {
                node->pending = kFullStock;

                if (node->pending + node->count > kFullStock)
                {
                    node->count = static_cast<i16>(kFullStock - node->pending);
                }
            }

            SetRow(list, row, NodeName(*node), node->count + node->pending, node->id, 0, 0);
            return FALSE;
        }

        case kAddButton:
        {
            if (cheat->tab >= CheatDialog::FirstAttributesTab || !FieldText(dialog, kIdField, kFieldBytes, text))
            {
                return FALSE;
            }

            const int id = ParsedNumber(text, 0);
            ItemLink* link = cheat->TabList();
            const bool skill = cheat->tab != CheatDialog::ItemsTab;

            if (link == nullptr || (skill && (id < kFirstSkill || id >= kSkillsEnd)))
            {
                return FALSE;
            }

            if (!FieldText(dialog, kCountField, kFieldBytes, text) || skill)
            {
                text = "1";
            }

            const int count = ParsedNumber(text, 0);

            if (!FieldText(dialog, kValueField, kFieldBytes, text) || skill)
            {
                text = "0";
            }

            const int value = ParsedNumber(text, 0);
            const ItemNode* node =
                AddItem(*link, static_cast<u16>(id), static_cast<i16>(count), ItemCountMode::Pending, cheat->database);

            if (node == nullptr)
            {
                DebugLog(kInsertFailed);
            }
            else if (!skill)
            {
                cheat->SetObjectCounter(*node, static_cast<u16>(id), value);
            }

            Fill(*cheat, list);
            ClearFields(dialog);
            return FALSE;
        }

        case kDeleteButton:
        {
            if (cheat->tab >= CheatDialog::FirstAttributesTab)
            {
                return FALSE;
            }

            const auto row = static_cast<int>(SendMessageW(list, LVM_GETSELECTIONMARK, 0, 0));
            ItemLink* link = cheat->TabList();

            if (row < 0 || link == nullptr)
            {
                return FALSE;
            }

            ItemNode* node = nullptr;
            SkipItems(row, *link, node);

            if (node != nullptr &&
                AddItem(*link, node->id, kDeleteAll, ItemCountMode::Pending, cheat->database) != nullptr)
            {
                DebugLog(kDeleteFailed);
            }

            Fill(*cheat, list);
            return FALSE;
        }

        case kSetButton:
        {
            if (!FieldText(dialog, kIdField, kFieldBytes, text))
            {
                return FALSE;
            }

            const int id = ParsedNumber(text, 0);
            // The original keeps stack garbage for a count or value field left empty; 0 here.
            int count = 0;
            int value = 0;

            if (cheat->tab == CheatDialog::VariablesTab)
            {
                if (id < 0 || id > kLastVariable)
                {
                    return FALSE;
                }

                if (FieldText(dialog, kCountField, kFieldBytes, text))
                {
                    count = ParsedNumber(text, 0);
                }

                cheat->party.variables[static_cast<std::size_t>(id)] = count;
            }
            else if (cheat->tab < CheatDialog::FirstAttributesTab)
            {
                ItemNode* node = FindItemOfKind(cheat->party.inventory, static_cast<i16>(id));

                if (node == nullptr)
                {
                    return FALSE;
                }

                if (FieldText(dialog, kCountField, kFieldBytes, text))
                {
                    count = ParsedNumber(text, 0);
                    node->count = 0;
                    node->pending = static_cast<i16>(std::min<int>(count, kFullStock));
                }

                if (FieldText(dialog, kValueField, kFieldBytes, text))
                {
                    value = ParsedNumber(text, 0);
                }

                if (cheat->tab == CheatDialog::ItemsTab)
                {
                    cheat->SetObjectCounter(*node, static_cast<u16>(id), value);
                }
            }
            else
            {
                if (id > kLastAttribute)
                {
                    return FALSE;
                }

                if (FieldText(dialog, kCountField, kFieldBytes, text))
                {
                    count = ParsedNumber(text, 0);
                }

                cheat->party.SetRoleStat(id, count, cheat->tab - CheatDialog::FirstAttributesTab, cheat->database);
            }

            Fill(*cheat, list);
            ClearFields(dialog);
            return FALSE;
        }

        default:
            return FALSE;
    }
}

// swd32.exe 0x00425470: the name dialog: the field (eight bytes) starts with the first role's default name; OK takes
// a name of at least one byte as the first role's name, Cancel and closing give up
INT_PTR CALLBACK NameProcedure(HWND dialog, UINT message, WPARAM word, LPARAM data)
{
    if (message == WM_INITDIALOG)
    {
        SetWindowLongPtrW(dialog, DWLP_USER, data);
    }

    auto* cheat = reinterpret_cast<CheatDialog*>(GetWindowLongPtrW(dialog, DWLP_USER));

    switch (message)
    {
        case WM_CLOSE:
            EndDialog(dialog, 0);
            return TRUE;

        case WM_INITDIALOG:
        {
            HWND field = GetDlgItem(dialog, kNameField);
            SendMessageW(field, EM_LIMITTEXT, kNameBytes, 0);
            SendMessageW(field, kNameUnknownMessage, kNameUnknownWord, kNameUnknownLong);
            const Party::Name name = Party::DefaultNames()[0];
            SetDlgItemTextW(
                dialog,
                kNameField,
                Wide(std::string_view(name.data(), strnlen(name.data(), name.size()))).c_str()
            );
            return TRUE;
        }

        case WM_COMMAND:
        {
            if (LOWORD(word) == IDOK && cheat != nullptr)
            {
                std::string text;

                if (!FieldText(dialog, kNameField, kNameBytes, text))
                {
                    MessageBoxW(dialog, Wide(kNoName).c_str(), Wide(kNameTitle).c_str(), MB_OK);
                    return FALSE;
                }

                Party::Name& name = cheat->party.names[0];
                name.fill('\0');
                std::copy_n(text.begin(), std::min(text.size(), name.size() - 1), name.begin());
                EndDialog(dialog, 1);
            }
            else if (LOWORD(word) == kNameCancel)
            {
                EndDialog(dialog, 0);
            }

            return FALSE;
        }

        default:
            return FALSE;
    }
}

}  // namespace

}  // namespace swd3
