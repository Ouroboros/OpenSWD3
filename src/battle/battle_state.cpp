#include "battle/battle_state.h"

#include "core/words.h"

#include <algorithm>

#include "crt/msvc.h"

namespace swd3
{

namespace
{

constexpr std::size_t kAfterKindsWord = 4;  // MenuKindWord: the menu command whose word follows commandKinds
constexpr int kSecondKind = 2;
constexpr u16 kAllOnes16 = 0xFFFF;  // a free record has every byte 0xFF

}  // namespace

// swd32.exe 0x004694E0: takes the first non-zero entry of the 4 x 10 queue unless one is pending
void BattleState::TakePendingAction()
{
    if (this->pendingAction != 0)
    {
        return;
    }

    for (int row = 0; row < BattleState::QueueRows; ++row)
    {
        for (int column = 0; column < BattleState::QueueColumns; ++column)
        {
            i16& entry = this->actionQueue[static_cast<std::size_t>(row * BattleState::QueueColumns + column)];

            if (entry != 0)
            {
                this->pendingActionRow = static_cast<i16>(row);
                this->pendingAction = entry;
                entry = 0;
                return;
            }
        }
    }
}

// swd32.exe 0x0045B190: the units from the top down: each pass takes the first unit with a y not yet taken, then the
// lowest y among the enemies after it and the roles (a role at the same y as an enemy comes after it); a unit with y 0
// is never the first pick. With none left the first pick is unit 18, whose y is the dword after unitY (darkFrames).
void BattleState::SortDrawOrder()
{
    const u32 units = this->enemyCount + this->roleCount;

    for (u32 pass = 0; pass < units; ++pass)
    {
        u32 first = 0;

        while (first < static_cast<u32>(BattleState::Units) && (this->unitY[first] == 0 || this->drawTaken[first] == 1))
        {
            ++first;
        }

        i32 lowest = first < static_cast<u32>(BattleState::Units) ? this->unitY[first] : this->darkFrames;
        u32 chosen = first;

        for (u32 enemy = first + 1; enemy < this->enemyCount; ++enemy)
        {
            if (this->drawTaken[enemy] != 1 && this->unitY[enemy] < lowest)
            {
                lowest = this->unitY[enemy];
                chosen = enemy;
            }
        }

        for (u32 unit = BattleState::FirstRoleUnit; unit < this->roleCount + BattleState::FirstRoleUnit; ++unit)
        {
            if (this->drawTaken[unit] != 1 && this->unitY[unit] < lowest)
            {
                lowest = this->unitY[unit];
                chosen = unit;
            }
        }

        if (chosen < static_cast<u32>(BattleState::Units))
        {
            this->drawTaken[chosen] = 1;
        }

        this->drawOrder[pass] = static_cast<i32>(chosen);
    }

    this->drawTaken.fill(0);
}

// swd32.exe 0x0045B5A0: the enemies in drawing order
void BattleState::ListEnemyOrder()
{
    u32 listed = 0;

    for (const i32 unit : this->drawOrder)
    {
        if (unit < BattleState::FirstRoleUnit)
        {
            this->enemyOrder[listed] = unit;
            ++listed;

            if (listed == this->enemyCount)
            {
                break;
            }
        }
    }
}

// swd32.exe 0x0045EDF0: `unit` with kind 1 or 2 put in the first free record (none when all are used); other kinds
// add nothing
void BattleState::AddSlotRecord(int kind, i32 unit)
{
    if (kind != 1 && kind != kSecondKind)
    {
        return;
    }

    for (BattleSlotRecord& record : this->slotRecords)
    {
        if (record.unit == -1)
        {
            record.unit = unit;
            record.kind = static_cast<u16>(kind);
            return;
        }
    }
}

// swd32.exe 0x0045EFB0: the record of `unit` removed, the later ones moving up and the last made free with every byte
// 0xFF (the original first copies the bytes after the records into the last one)
void BattleState::RemoveSlotRecord(i32 unit)
{
    auto& records = this->slotRecords;
    auto found = std::find_if(
        records.begin(),
        records.end(),
        [unit](const BattleSlotRecord& record)
        {
            return record.unit == unit;
        }
    );

    if (found == records.end())
    {
        return;
    }

    std::move(found + 1, records.end(), found);
    records.back() = BattleSlotRecord{-1, -1, kAllOnes16, kAllOnes16, -1, -1, -1, -1};
}

// swd32.exe 0x0045D3E0: the field fades from (red, green, blue) to (toRed, toGreen, toBlue) over `frames` frames; the
// steps are taken from the levels truncated to integers
void BattleState::StartFieldFade(int red, int green, int blue, int toRed, int toGreen, int toBlue, int frames)
{
    ColorFade& fade = this->fieldFade;
    fade.frames = frames;
    fade.red = static_cast<float>(red);
    fade.green = static_cast<float>(green);
    fade.blue = static_cast<float>(blue);
    fade.startRed = static_cast<float>(toRed);
    fade.startGreen = static_cast<float>(toGreen);
    fade.startBlue = static_cast<float>(toBlue);
    const auto count = static_cast<double>(frames);
    fade.stepRed = static_cast<float>(static_cast<double>(toRed - static_cast<i32>(crt::Ftol(fade.red))) / count);
    fade.stepGreen = static_cast<float>(static_cast<double>(toGreen - static_cast<i32>(crt::Ftol(fade.green))) / count);
    fade.stepBlue = static_cast<float>(static_cast<double>(toBlue - static_cast<i32>(crt::Ftol(fade.blue))) / count);
}

// swd32.exe 0x0045EE70: `unit` put at place `at` among the records (the records from there to the first free one move
// down a place). Kind 1 (a party unit) at -1 goes to the first free record, and takes its pending order (cleared, as
// are unk53BF80 and unk53BFD0). The original counts from 0 when no record is free, puts a kind 1 unit placed past the
// first free record at place 0, and moves the last record past the array when only it was free.
void BattleState::InsertSlotRecord(int kind, i32 unit, int at)
{
    auto& records = this->slotRecords;
    int count = 0;

    for (std::size_t place = 0; place < records.size(); ++place)
    {
        if (records[place].unit == -1)
        {
            count = static_cast<int>(place);
            break;
        }
    }

    auto shiftFrom = [&records, count](int first)
    {
        for (int place = count; place >= first; --place)
        {
            if (place + 1 < static_cast<int>(records.size()))
            {
                records[static_cast<std::size_t>(place + 1)] = records[static_cast<std::size_t>(place)];
            }
        }
    };

    if (kind != 1)
    {
        if (count >= at)
        {
            shiftFrom(at);
        }

        BattleSlotRecord& record = records[static_cast<std::size_t>(at)];
        record.unit = unit;
        record.kind = static_cast<u16>(kind);
        return;
    }

    int place = 0;

    if (at == -1)
    {
        place = count;
    }
    else if (count >= at)
    {
        place = at;
        shiftFrom(at);
    }

    BattleSlotRecord& record = records[static_cast<std::size_t>(place)];
    record.unit = unit;
    record.kind = 1;

    // A unit before the party units reads before the orders in the original.
    if (unit >= BattleState::FirstRoleUnit)
    {
        PendingOrder& order = this->pendingOrders[static_cast<std::size_t>(unit - BattleState::FirstRoleUnit)];
        record.unk14 = order.unk04;
        record.unk0C = order.unk00;
        record.unk18 = order.unk08;
        record.unk0A = order.unk0C;
        record.targetCount = order.targetCount;
        order = PendingOrder{};
    }

    this->unk53BF80 = 0;
    this->unk53BFD0 = 0;
}

// The slot records filled with 0xFF bytes (inline in the original, by 0x0045ADF0, 0x0045AA00 and others).
void BattleState::WipeSlotRecords()
{
    for (BattleSlotRecord& record : this->slotRecords)
    {
        record = BattleSlotRecord{-1, -1, 0xFFFF, 0xFFFF, -1, -1, -1, -1};
    }
}

// The order of 0x0045B280 (inline in the original): first the units of the acting unit's side above it (lower y, not
// it or `target`), sorted by an insertion that also copies the last placed unit one further; then each pass takes the
// first unit not taken that is off that side or above the acting unit, and the lowest enemy (unless a role acts) or
// role (unless an enemy acts) not taken below it; `target` brings the acting unit right after it. With no unit left
// the first pick reads the dword after unitY (darkFrames) as its y and places unit 18, and the order is then made
// again by SortDrawOrder.
void BattleState::SortDrawOrderAround(i32 acting, i32 target)
{
    const bool roleActs = acting >= FirstRoleUnit;
    const int sideFirst = roleActs ? FirstRoleUnit : 0;
    const int sideEnd = roleActs ? Units : FirstRoleUnit;
    i32 bound = this->unitY[static_cast<std::size_t>(acting)];
    int placed = 0;
    int at = 0;

    for (int unit = sideFirst; unit < sideEnd; ++unit)
    {
        const i32 y = this->unitY[static_cast<std::size_t>(unit)];

        if (y == 0 || unit == acting || unit == target || y >= bound)
        {
            continue;
        }

        if (placed > 0)
        {
            int position = 0;
            bool found = true;

            while (y >= this->unitY[static_cast<std::size_t>(this->drawOrder[static_cast<std::size_t>(position)])])
            {
                if (++position >= placed)
                {
                    found = false;
                    break;
                }
            }

            if (found)
            {
                at = position;

                for (int move = placed + 1; move > position; --move)
                {
                    this->drawOrder[static_cast<std::size_t>(move)] =
                        this->drawOrder[static_cast<std::size_t>(move - 1)];
                }
            }
        }

        this->drawOrder[static_cast<std::size_t>(at)] = unit;
        this->drawTaken[static_cast<std::size_t>(unit)] = 1;
        ++placed;
        at = placed;
    }

    const auto total = static_cast<int>(this->roleCount + this->enemyCount);
    i32 actingLeft = acting;
    i32 targetLeft = target;

    // The original runs on past the order (writing the globals after it) when the pair is placed at its end.
    while (placed != total && at < Units)
    {
        if (actingLeft != -1)
        {
            bound = this->unitY[static_cast<std::size_t>(actingLeft)];
        }

        int first = 0;

        for (; first < Units; ++first)
        {
            const i32 y = this->unitY[static_cast<std::size_t>(first)];
            const bool allowed = actingLeft == -1 || first < sideFirst || first >= sideEnd || y < bound;

            if (y != 0 && allowed && this->drawTaken[static_cast<std::size_t>(first)] != 1)
            {
                break;
            }
        }

        i32 lowest = first < Units ? this->unitY[static_cast<std::size_t>(first)] : this->darkFrames;
        int chosen = first;

        for (u32 enemy = 0; enemy < this->enemyCount; ++enemy)
        {
            const auto unit = static_cast<std::size_t>(enemy);
            const bool allowed = actingLeft >= FirstRoleUnit || actingLeft == -1;

            if (this->drawTaken[unit] != 1 && this->unitY[unit] < lowest && static_cast<i32>(enemy) != actingLeft &&
                allowed)
            {
                lowest = this->unitY[unit];
                chosen = static_cast<int>(enemy);
            }
        }

        for (u32 role = FirstRoleUnit; role < this->roleCount + FirstRoleUnit; ++role)
        {
            const auto unit = static_cast<std::size_t>(role);
            const bool allowed = actingLeft < FirstRoleUnit || actingLeft == -1;

            if (this->drawTaken[unit] != 1 && this->unitY[unit] < lowest && static_cast<i32>(role) != actingLeft &&
                allowed)
            {
                lowest = this->unitY[unit];
                chosen = static_cast<int>(role);
            }
        }

        if (chosen == targetLeft && at + 1 < Units)
        {
            this->drawOrder[static_cast<std::size_t>(at)] = targetLeft;
            this->drawTaken[static_cast<std::size_t>(targetLeft)] = 1;
            this->drawTaken[static_cast<std::size_t>(actingLeft)] = 1;
            this->drawOrder[static_cast<std::size_t>(at + 1)] = actingLeft;
            at += 2;
            placed += 2;
            actingLeft = -1;
            targetLeft = -1;
            continue;
        }

        this->drawOrder[static_cast<std::size_t>(at)] = chosen;

        // The original marks unit 18 past the table (over 0x00520DA4).
        if (chosen < Units)
        {
            this->drawTaken[static_cast<std::size_t>(chosen)] = 1;
        }

        ++placed;
        ++at;
    }

    this->drawTaken.fill(0);
    this->orderSorted = 1;

    if (std::find(this->drawOrder.begin(), this->drawOrder.end(), Units) != this->drawOrder.end())
    {
        this->SortDrawOrder();
    }
}

// swd32.exe 0x0045EC60: object `id` held back as returned object `index` (1 or 2; given back by ReturnBattleItems)
void BattleState::SetReturnedItem(int index, u16 id)
{
    this->givenBack[static_cast<std::size_t>(index)] = id;
}

// The removal of 0x0045F020 (inline in the original): record `at` copied into `taken`; a used one is removed, the
// later records moving up, and from the first free record on (from `at` when none is free) the records are cleared.
// The original reads the table after the records for `at` 18; a free record is taken then.
void BattleState::TakeSlotRecordAt(std::size_t at, BattleSlotRecord& taken)
{
    auto& records = this->slotRecords;

    if (at >= records.size())
    {
        taken = BattleSlotRecord{};
        return;
    }

    taken = records[at];

    if (taken.unit == -1)
    {
        return;
    }

    for (std::size_t move = at; move + 1 < records.size(); ++move)
    {
        records[move] = records[move + 1];
    }

    std::size_t clearFrom = at;

    for (std::size_t place = 0; place < records.size(); ++place)
    {
        if (records[place].unit == -1)
        {
            clearFrom = place;
            break;
        }
    }

    for (std::size_t place = clearFrom; place < records.size(); ++place)
    {
        records[place] = BattleSlotRecord{};
    }
}

// The seven dwords from 0x0053AE70 cleared (a memset in the original: the acting unit and its hit)
void BattleState::ClearActing()
{
    this->actingUnit = 0;
    this->sweepTotal = 0;
    this->unk53AE78 = 0;
    this->slideBackPicture = 0;
    this->unk53AE7C = 0;
    this->unk53AE80 = 0;
    this->unk53AE84 = 0;
    this->unk53AE88 = 0;
}

// The word at 0x004FE5CA + 2 * `index` that 0x0045FC60 reads as the accessory kind of menu command `index` (4..7):
// the words after commandKinds (the word after it and the halves of unk4FE5D4), not the kinds of the commands.
u16 BattleState::MenuKindWord(std::size_t index) const
{
    switch (index)
    {
        case kAfterKindsWord:
            return this->unk4FE5D2;

        case kAfterKindsWord + 1:
            return LowWord(static_cast<u32>(this->unk4FE5D4[0]));

        case kAfterKindsWord + 2:
            return HighWord(static_cast<u32>(this->unk4FE5D4[0]));

        default:
            return LowWord(static_cast<u32>(this->unk4FE5D4[1]));
    }
}
}  // namespace swd3
