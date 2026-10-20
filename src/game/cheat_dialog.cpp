#include "game/cheat_dialog.h"

#include "core/format.h"
#include "game/game.h"
#include "org/fame.h"
#include "party/party.h"

namespace swd3
{

namespace
{

constexpr u32 kPendingKindBit = 0x8000;    // cleared from the kind bits compared with a slot's kind
constexpr std::size_t kWeaponSlot = 0;     // Party::SlotKinds: the kinds with a percent counter
constexpr std::size_t kMagicSlot = 7;      // with a proficiency
constexpr std::size_t kCountedSlot = 9;    // with a capped counter
constexpr u16 kLastCountedObject = 0x1F4;  // objects 1..0x1F4 also have a capped counter
constexpr i32 kNoValue = -1;

// Whether `kind` holds all of `slot`'s kind bits (bit 15 aside).
bool HasKind(u32 kind, u32 slot)
{
    return ((kind & slot) & ~kPendingKindBit) == slot;
}

}  // namespace

CheatDialog::CheatDialog(Game& game)
    : game(game),
      party(game.party),
      database(game.database),
      running(game.running),
      resources(game.resources)
{
}

ItemLink* CheatDialog::TabList() const
{
    if (this->tab == CheatDialog::ItemsTab)
    {
        return &this->party.inventory;
    }

    if (this->tab >= CheatDialog::FirstSkillsTab && this->tab < CheatDialog::FirstAttributesTab)
    {
        ItemLink& head = this->party.skills[static_cast<std::size_t>(this->tab - CheatDialog::FirstSkillsTab)];
        return head != nullptr ? &head->next : nullptr;
    }

    return nullptr;
}

SDL_Window* CheatDialog::Window() const
{
    return this->game.display.Window();
}

std::string CheatDialog::ValueText(i32 value, i32 maximum, const std::string& before)
{
    if (value == kNoValue)
    {
        return {};
    }

    if (maximum > 1)
    {
        return WinFormat("%d/%d", {value, maximum});
    }

    if (maximum == 1)
    {
        return before;
    }

    return WinFormat("%d", {value});
}

void CheatDialog::ObjectCounter(const ItemNode& node, i32& value, i32& maximum) const
{
    std::array<CounterList, Fame::Lists>& lists = this->party.fame.lists;
    const u32 kind = node.record.data.kind;
    value = kNoValue;
    maximum = kNoValue;

    if (HasKind(kind, Party::SlotKinds[kWeaponSlot]))
    {
        value = static_cast<u16>(FindCounterPercent(lists[Fame::UnkList0], static_cast<i16>(node.id)));
        maximum = 0;
    }

    if (HasKind(kind, Party::SlotKinds[kMagicSlot]))
    {
        u16 most = 0;
        u16 got = 0;
        GetProficiency(lists[Fame::ProficiencyList], node.id, most, got, this->database);
        value = got;
        maximum = most;
    }

    if (HasKind(kind, Party::SlotKinds[kCountedSlot]))
    {
        value = static_cast<u16>(FindCounterValue(lists[Fame::FameList], static_cast<i16>(node.id)));
        maximum = kNoValue;
    }

    if (node.id != 0 && node.id <= kLastCountedObject)
    {
        value = static_cast<u16>(FindCounterValue(lists[Fame::FameList], static_cast<i16>(node.id)));
        maximum = kNoValue;
    }
}

void CheatDialog::SetObjectCounter(const ItemNode& node, u16 id, i32 value)
{
    std::array<CounterList, Fame::Lists>& lists = this->party.fame.lists;
    const u32 kind = node.record.data.kind;

    if (HasKind(kind, Party::SlotKinds[kWeaponSlot]))
    {
        SetCounterProgress(
            lists[Fame::UnkList0],
            static_cast<i16>(id),
            node.record.data.counterMax,
            static_cast<u16>(value)
        );
    }

    if (HasKind(kind, Party::SlotKinds[kMagicSlot]))
    {
        SetProficiency(lists[Fame::ProficiencyList], id, static_cast<u16>(value), this->database);
    }

    if (HasKind(kind, Party::SlotKinds[kCountedSlot]))
    {
        SetCounterCapped(lists[Fame::FameList], static_cast<i16>(id), static_cast<u16>(value));
    }

    if (node.id != 0 && node.id <= kLastCountedObject)
    {
        SetCounterCapped(lists[Fame::FameList], static_cast<i16>(id), static_cast<u16>(value));
    }
}

}  // namespace swd3
