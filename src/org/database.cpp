#include "org/database.h"

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <vector>

#include "core/byte_reader.h"
#include "core/format.h"
#include "core/words.h"

namespace swd3
{

namespace
{

constexpr u32 kIndexBase = 0x204;
constexpr u32 kDataBase = 0x200;
constexpr u32 kLevelIndex = 0x70;
constexpr int kLevelsPerGroup = 0x64;
constexpr int kTextSearch = 0xFF;
constexpr u16 kEnd = static_cast<u16>(ObjectTag::End);
constexpr std::size_t kActBlockBytes = 0x4D;
constexpr std::size_t kTextEndBytes = 2;   // the "$$" after a text
constexpr std::size_t kResistUnk9BAt = 9;  // tag 0x7D0: nine resistances, two bytes and two words
constexpr std::size_t kResistUnk9CAt = 10;
constexpr std::size_t kResistLevelAt = 11;
constexpr std::size_t kResistUnk52At = 13;
constexpr std::size_t kResistBytes = 15;
constexpr u16 kLevelStats = 0;  // level.dat opcodes: the stats (kLevelStatBytes), a skipped word
constexpr u16 kLevelSkip = 1;
constexpr std::size_t kLevelStatBytes = 0x1A;
constexpr std::size_t kExperienceAt = 0x16;

// Reads the script words; past the 0x400-byte buffer (where the original reads on) the script ends.
class ScriptReader
{
public:
    explicit ScriptReader(const std::vector<u8>& script)
        : script(script)
    {
    }

    // The next opcode; past the buffer the script ends.
    u16 Word()
    {
        const u16 value = this->Ended() ? kEnd : this->WordAt(this->position);
        this->position += sizeof(u16);
        return value;
    }

    u16 WordAt(std::size_t at) const
    {
        return ByteReader::Word16At(this->script, at);
    }

    u32 DwordAt(std::size_t at) const
    {
        return MakeWords(this->WordAt(at + sizeof(u16)), this->WordAt(at));
    }

    u8 ByteAt(std::size_t at) const
    {
        return at < this->script.size() ? this->script[at] : 0;
    }

    // The length of the text up to "$$", searching at most 0xFF bytes (0xFF when there is none).
    int TextLength() const
    {
        int length = 0;

        while (this->ByteAt(this->position + length) != '$' || this->ByteAt(this->position + length + 1) != '$')
        {
            if (++length >= kTextSearch)
            {
                return length;
            }
        }

        return length;
    }

    void Copy(void* target, std::size_t bytes) const
    {
        auto* out = static_cast<u8*>(target);

        for (std::size_t i = 0; i < bytes; ++i)
        {
            out[i] = this->ByteAt(this->position + i);
        }
    }

    bool Ended() const
    {
        return this->position + 1 >= this->script.size();
    }

    std::size_t position = 0;

private:
    const std::vector<u8>& script;
};

u32 ReadDword(File& file)
{
    u32 value = 0;
    u32 bytes = sizeof(value);
    file.Read(&value, bytes);
    return value;
}

}  // namespace

Database::Database(const std::u8string& directory)
    : directory(directory)
{
}

// mon.dat and level.dat are opened (OPEN_ALWAYS, read) on first use and kept open.
File* Database::OpenData(File& file, bool& open, std::string_view name)
{
    if (open)
    {
        return &file;
    }

    if (!file.Open(PathFormat("%s%s", {this->directory, name}), File::Disposition::OpenAlways, File::Access::Read))
    {
        return nullptr;
    }

    file.Preload();
    open = true;
    return &file;
}

void Database::ReadScript(File& file, u32 offset, std::vector<u8>& script)
{
    file.SeekBegin(static_cast<i32>(offset + kDataBase));
    script.assign(Database::ScriptBytes, 0);
    u32 bytes = Database::ScriptBytes;
    file.Read(script.data(), bytes);
}

// swd32.exe 0x00476A80: the script of skill `id` into `skill` (not cleared first): word opcodes with their operands
// until 5; the leading 0 word (required) is itself run as opcode 0, unknown opcodes take no operand. The offsets are
// 0 where the original keeps stack garbage after a failed read.
bool Database::LoadSkill(SkillScript& skill, u16 id)
{
    File* file = this->OpenData(this->monFile, this->monOpen, "mon.dat");

    if (file == nullptr)
    {
        return false;
    }

    file->SeekBegin(static_cast<i32>(kIndexBase));
    const u32 base = ReadDword(*file);
    file->SeekBegin(static_cast<i32>(base + 4u * id + kDataBase));
    const u32 offset = ReadDword(*file);
    std::vector<u8> script;
    this->ReadScript(*file, offset, script);
    ScriptReader code(script);

    if (code.WordAt(0) != 0)
    {
        return false;
    }

    while (!code.Ended())
    {
        const u16 opcode = code.Word();

        if (opcode == static_cast<u16>(SkillOpcode::End))
        {
            break;
        }

        const std::size_t at = code.position;

        switch (static_cast<SkillOpcode>(opcode))
        {
            case SkillOpcode::Values:
                skill.unk0C = code.DwordAt(at);
                skill.unk10 = code.DwordAt(at + sizeof(u32));
                code.position += 2 * sizeof(u32);
                break;

            case SkillOpcode::Turns:
                skill.turns = code.ByteAt(at);
                code.position += sizeof(u16);
                break;

            case SkillOpcode::SetUnkFlag1:
                skill.flags |= SkillScript::TargetsAll;
                break;

            case SkillOpcode::Unk16:
                skill.unk16 = code.WordAt(at);
                code.position += sizeof(u16);
                break;

            case SkillOpcode::NoAilment:
                skill.ailment = code.WordAt(at);
                code.position += sizeof(u16);
                skill.flags |= SkillScript::NoAilmentFlag;
                break;

            case SkillOpcode::Ailment:
                skill.ailment = code.WordAt(at);
                code.position += sizeof(u16);
                break;

            case SkillOpcode::Unk14:
                skill.unk14 = code.WordAt(at);
                code.position += sizeof(u16);
                break;

            case SkillOpcode::SetUnkFlag2:
                skill.flags |= SkillScript::AllAtOnce;
                break;

            case SkillOpcode::Unk1E:
                skill.unk1E = code.WordAt(at);
                code.position += sizeof(u16);
                skill.unk1E |= SkillScript::Unk1EMark;
                break;

            case SkillOpcode::SetUnkFlag4:
                skill.flags |= SkillScript::UnkFlag4;
                break;

            case SkillOpcode::StatusBits:
                skill.statusBits = code.DwordAt(at);
                code.position += sizeof(u32);
                break;

            case SkillOpcode::Unk1C:
                skill.unk1C = code.WordAt(at);
                code.position += sizeof(u16);
                break;

            case SkillOpcode::Element:
                skill.flags |= SkillScript::ElementBuff;
                skill.element = code.WordAt(at);
                code.position += sizeof(u16);
                break;

            case SkillOpcode::SetUnkFlag10:
                skill.flags |= SkillScript::UnkFlag10;
                break;

            case SkillOpcode::SetUnkFlag20:
                skill.flags |= SkillScript::UnkFlag20;
                break;

            case SkillOpcode::SetUnkFlag40:
                skill.flags |= SkillScript::UnkFlag40;
                break;

            case SkillOpcode::SetUnkFlag100:
                skill.flags |= SkillScript::UnkFlag100;
                break;

            case SkillOpcode::SetUnkFlag200:
                skill.flags |= SkillScript::UnkFlag200;
                break;

            case SkillOpcode::SetUnkFlag400:
                skill.flags |= SkillScript::UnkFlag400;
                break;

            case SkillOpcode::SetUnkFlag800:
                skill.flags |= SkillScript::UnkFlag800;
                break;

            case SkillOpcode::Unk20:
                skill.flags |= SkillScript::UnkFlag1000;
                skill.unk20 = code.WordAt(at);
                skill.turns = code.ByteAt(at + sizeof(u16));
                code.position += sizeof(u32);
                break;

            case SkillOpcode::Unk22:
                skill.unk22 = code.WordAt(at);
                code.position += sizeof(u16);
                break;

            case SkillOpcode::SetUnkFlag2000:
                skill.flags |= SkillScript::UnkFlag2000;
                break;

            case SkillOpcode::SetUnkFlag4000:
                skill.flags |= SkillScript::UnkFlag4000;
                break;

            default:
                break;
        }
    }

    return true;
}

// swd32.exe 0x00476DB0: the object `id` of mon.dat into `record` (cleared first; a previous description is freed and
// uncounted). The script must start with tag 0x3E8 (the name up to "$$"), then tags until 5: words, the 0x4D-byte
// block of tag 1, the resistances and words of 0x7D0, single bytes of 0x1C and 0x1D, the description of 0x1E. The
// original copies a name of up to 0xFE bytes over the record (and past it); here at most the fixed part is written.
bool Database::LoadObject(ObjectRecord& record, u16 id)
{
    if (record.description.has_value())
    {
        this->descriptionBytes -= static_cast<i32>(record.description->size() + 1);
        record.description.reset();
    }

    record.data = ObjectData{};
    File* file = this->OpenData(this->monFile, this->monOpen, "mon.dat");

    if (file == nullptr)
    {
        return false;
    }

    file->SeekBegin(static_cast<i32>(kIndexBase));
    ReadDword(*file);
    file->SeekCurrent(static_cast<i32>(sizeof(u32) * (id - 1)));
    const u32 offset = ReadDword(*file);
    std::vector<u8> script;
    this->ReadScript(*file, offset, script);
    ScriptReader code(script);

    if (code.WordAt(0) != Database::NameTag)
    {
        return false;
    }

    ObjectData& data = record.data;
    auto* bytes = reinterpret_cast<u8*>(&data);

    while (!code.Ended())
    {
        const u16 tag = code.Word();

        if (tag == static_cast<u16>(ObjectTag::End))
        {
            break;
        }

        const std::size_t at = code.position;

        auto word = [&](u16& field)
        {
            field = code.WordAt(at);
            code.position += sizeof(u16);
        };

        switch (static_cast<ObjectTag>(tag))
        {
            case ObjectTag::ActBlock:
                code.Copy(bytes + offsetof(ObjectData, act), kActBlockBytes);
                code.position += kActBlockBytes;
                break;

            case ObjectTag::CostBits:
                word(data.costBits);
                break;

            case ObjectTag::AttackBonusUp:
                word(data.attackBonusUp);
                break;

            case ObjectTag::DefenseBonusUp:
                word(data.defenseBonusUp);
                break;

            case ObjectTag::AgilityUp:
                word(data.agilityUp);
                break;

            case ObjectTag::EvasionUp:
                word(data.evasionUp);
                break;

            case ObjectTag::Price:
                word(data.price);
                break;

            case ObjectTag::MasteryValue:
                word(data.masteryValue);
                break;

            case ObjectTag::CounterMax:
                word(data.counterMax);
                break;

            case ObjectTag::Act:
                word(data.act);
                break;

            case ObjectTag::StrengthUp:
                word(data.strengthUp);
                break;

            case ObjectTag::EnduranceUp:
                word(data.enduranceUp);
                break;

            case ObjectTag::WisdomUp:
                word(data.wisdomUp);
                break;

            case ObjectTag::RapportUp:
                word(data.rapportUp);
                break;

            case ObjectTag::HitPointsUp:
                word(data.hitPointsUp);
                break;

            case ObjectTag::SpiritUp:
                word(data.spiritUp);
                break;

            case ObjectTag::StaminaUp:
                word(data.staminaUp);
                break;

            case ObjectTag::ActionKind:
                word(data.actionKind);
                break;

            case ObjectTag::Kind:
                data.kind = code.DwordAt(at + sizeof(u16));
                code.position += sizeof(u16) + sizeof(u32);
                break;

            case ObjectTag::UsableBy:
                word(data.usableBy);
                break;

            case ObjectTag::UseKind:
                word(data.useKind);
                break;

            case ObjectTag::Altar:
                data.altar = code.ByteAt(at);
                code.position += sizeof(u8);
                break;

            case ObjectTag::Unk9C:
                data.unk9C = code.ByteAt(at);
                code.position += sizeof(u8);
                break;

            case ObjectTag::Description:
            {
                const int length = code.TextLength();

                if (length >= kTextSearch)
                {
                    code.position += static_cast<std::size_t>(length);
                    break;
                }

                std::string text(static_cast<std::size_t>(length), '\0');
                code.Copy(text.data(), text.size());
                record.description = std::move(text);
                code.position += static_cast<std::size_t>(length) + kTextEndBytes;
                this->descriptionBytes += length + 1;
                break;
            }

            case ObjectTag::SkillScript:
                word(data.skillScript);
                break;

            case ObjectTag::Name:
            {
                const int length = code.TextLength();

                if (length < kTextSearch)
                {
                    code.Copy(bytes, std::min(static_cast<std::size_t>(length), sizeof(ObjectData)));
                }

                code.position += static_cast<std::size_t>(length) + kTextEndBytes;
                break;
            }

            case ObjectTag::Resistances:
                code.Copy(data.resistances.data(), data.resistances.size());
                data.altar = code.ByteAt(at + kResistUnk9BAt);
                data.unk9C = code.ByteAt(at + kResistUnk9CAt);
                data.level = code.WordAt(at + kResistLevelAt);
                data.category = code.WordAt(at + kResistUnk52At);
                code.position += kResistBytes;
                break;

            default:
                break;
        }
    }

    return true;
}

// swd32.exe 0x00477290: the experience of entry (`group`, `index`) of level.dat (the dword at byte 0x16 of the 13
// words of opcode 0; opcode 1 skips a word)
bool Database::LoadLevelEntry(int group, int index, u32& experience)
{
    std::vector<u8> script;

    if (!this->ReadLevelScript(group, index, script))
    {
        return false;
    }

    ScriptReader code(script);

    if (code.WordAt(0) != 0)
    {
        return false;
    }

    while (!code.Ended())
    {
        const u16 opcode = code.Word();

        if (opcode == kEnd)
        {
            break;
        }

        if (opcode == kLevelStats)
        {
            experience = code.DwordAt(code.position + kExperienceAt);
            code.position += kLevelStatBytes;
        }
        else if (opcode == kLevelSkip)
        {
            code.position += sizeof(u16);
        }
    }

    return true;
}

bool Database::ReadLevelScript(int group, int index, std::vector<u8>& script)
{
    File* file = this->OpenData(this->levelFile, this->levelOpen, "level.dat");

    if (file == nullptr)
    {
        return false;
    }

    file->SeekBegin(static_cast<i32>(sizeof(u32)) * (index + kLevelsPerGroup * group) + static_cast<i32>(kLevelIndex));
    const u32 offset = ReadDword(*file);
    this->ReadScript(*file, offset, script);
    return true;
}

// swd32.exe 0x004776A0
void Database::CloseFiles()
{
    this->monFile.Close();
    this->levelFile.Close();
    this->monOpen = false;
    this->levelOpen = false;
}

// swd32.exe 0x00478220
void Database::FreeDescription(ObjectRecord& record)
{
    record.description.reset();
}

}  // namespace swd3
