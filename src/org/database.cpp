#include "org/database.h"

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <vector>

namespace swd3
{

namespace
{

constexpr u32 kIndexBase = 0x204;
constexpr u32 kDataBase = 0x200;
constexpr u32 kLevelIndex = 0x70;
constexpr int kLevelsPerGroup = 0x64;
constexpr int kTextSearch = 0xFF;
constexpr u16 kEnd = 5;

// Reads the script words; past the 0x400-byte buffer (where the original reads on) the script ends.
class ScriptReader
{
public:
    explicit ScriptReader(const std::vector<u8>& script) : script(script)
    {
    }

    // The next opcode; past the buffer the script ends.
    u16 Word()
    {
        const u16 value = this->Ended() ? kEnd : this->WordAt(this->position);
        this->position += 2;
        return value;
    }

    u16 WordAt(std::size_t at) const
    {
        if (at + 1 >= this->script.size())
        {
            return 0;
        }

        return static_cast<u16>(this->script[at] | (this->script[at + 1] << 8));
    }

    u32 DwordAt(std::size_t at) const
    {
        return this->WordAt(at) | (static_cast<u32>(this->WordAt(at + 2)) << 16);
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
    u32 bytes = 4;
    file.Read(&value, bytes);
    return value;
}

}  // namespace

Database::Database(const std::string& directory) : directory(directory)
{
}

// mon.dat and level.dat are opened (OPEN_ALWAYS, read) on first use and kept open.
File* Database::OpenData(File& file, bool& open, const char* name)
{
    if (open)
    {
        return &file;
    }

    if (!file.Open(this->directory + name, File::Disposition::OpenAlways, File::Access::Read))
    {
        return nullptr;
    }

    open = true;
    return &file;
}

void Database::ReadScript(File& file, u32 offset, std::vector<u8>& script)
{
    file.SeekBegin(static_cast<i32>(offset + kDataBase));
    script.assign(Database::kScriptBytes, 0);
    u32 bytes = Database::kScriptBytes;
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

        if (opcode == kEnd)
        {
            break;
        }

        const std::size_t at = code.position;

        switch (opcode)
        {
            case 0:
                skill.unk0C = code.DwordAt(at);
                skill.unk10 = code.DwordAt(at + 4);
                code.position += 8;
                break;

            case 2:
                skill.unk24 = code.ByteAt(at);
                code.position += 2;
                break;

            case 3:
                skill.flags |= 1;
                break;

            case 4:
                skill.unk16 = code.WordAt(at);
                code.position += 2;
                break;

            case 6:
                skill.unk18 = code.WordAt(at);
                code.position += 2;
                skill.flags |= 0x80;
                break;

            case 7:
                skill.unk18 = code.WordAt(at);
                code.position += 2;
                break;

            case 8:
                skill.unk14 = code.WordAt(at);
                code.position += 2;
                break;

            case 9:
                skill.flags |= 2;
                break;

            case 0xA:
                skill.unk1E = code.WordAt(at);
                code.position += 2;
                skill.unk1E |= 0x8000;
                break;

            case 0xB:
                skill.flags |= 4;
                break;

            case 0xC:
                skill.unk08 = code.DwordAt(at);
                code.position += 4;
                break;

            case 0xD:
                skill.unk1C = code.WordAt(at);
                code.position += 2;
                break;

            case 0xE:
                skill.flags |= 8;
                skill.unk1A = code.WordAt(at);
                code.position += 2;
                break;

            case 0xF:
                skill.flags |= 0x10;
                break;

            case 0x10:
                skill.flags |= 0x20;
                break;

            case 0x11:
                skill.flags |= 0x40;
                break;

            case 0x12:
                skill.flags |= 0x100;
                break;

            case 0x13:
                skill.flags |= 0x200;
                break;

            case 0x14:
                skill.flags |= 0x400;
                break;

            case 0x15:
                skill.flags |= 0x800;
                break;

            case 0x16:
                skill.flags |= 0x1000;
                skill.unk20 = code.WordAt(at);
                skill.unk24 = code.ByteAt(at + 2);
                code.position += 4;
                break;

            case 0x17:
                skill.unk22 = code.WordAt(at);
                code.position += 2;
                break;

            case 0x18:
                skill.flags |= 0x2000;
                break;

            case 0x19:
                skill.flags |= 0x4000;
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
    file->SeekCurrent(static_cast<i32>(4 * id - 4));
    const u32 offset = ReadDword(*file);
    std::vector<u8> script;
    this->ReadScript(*file, offset, script);
    ScriptReader code(script);

    if (code.WordAt(0) != Database::kNameTag)
    {
        return false;
    }

    ObjectData& data = record.data;
    auto* bytes = reinterpret_cast<u8*>(&data);

    while (!code.Ended())
    {
        const u16 tag = code.Word();

        if (tag == kEnd)
        {
            break;
        }

        const std::size_t at = code.position;

        auto word = [&](u16& field)
        {
            field = code.WordAt(at);
            code.position += 2;
        };

        switch (tag)
        {
            case 1:
                code.Copy(bytes + offsetof(ObjectData, unk50), 0x4D);
                code.position += 0x4D;
                break;

            case 6:
                word(data.unk40);
                break;

            case 7:
                word(data.unk24);
                break;

            case 8:
                word(data.unk26);
                break;

            case 9:
                word(data.unk2C);
                break;

            case 0xA:
                word(data.unk32);
                break;

            case 0xB:
                word(data.unk46);
                break;

            case 0xC:
                word(data.unk42);
                break;

            case 0xD:
                word(data.unk44);
                break;

            case 0xE:
                word(data.unk50);
                break;

            case 0xF:
                word(data.unk28);
                break;

            case 0x10:
                word(data.unk2A);
                break;

            case 0x11:
                word(data.unk2E);
                break;

            case 0x12:
                word(data.unk30);
                break;

            case 0x13:
                word(data.unk34);
                break;

            case 0x14:
                word(data.unk36);
                break;

            case 0x15:
                word(data.unk38);
                break;

            case 0x16:
                word(data.unk48);
                break;

            case 0x19:
                data.kind = code.DwordAt(at + 2);
                code.position += 6;
                break;

            case 0x1A:
                word(data.unk3A);
                break;

            case 0x1B:
                word(data.unk3C);
                break;

            case 0x1C:
                data.unk9B = code.ByteAt(at);
                code.position += 1;
                break;

            case 0x1D:
                data.unk9C = code.ByteAt(at);
                code.position += 1;
                break;

            case 0x1E:
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
                code.position += static_cast<std::size_t>(length) + 2;
                this->descriptionBytes += length + 1;
                break;
            }

            case 0x64:
                word(data.unk3E);
                break;

            case Database::kNameTag:
            {
                const int length = code.TextLength();

                if (length < kTextSearch)
                {
                    code.Copy(bytes, std::min(static_cast<std::size_t>(length), sizeof(ObjectData)));
                }

                code.position += static_cast<std::size_t>(length) + 2;
                break;
            }

            case 0x7D0:
                code.Copy(data.resistances.data(), data.resistances.size());
                data.unk9B = code.ByteAt(at + 9);
                data.unk9C = code.ByteAt(at + 10);
                data.level = code.WordAt(at + 11);
                data.unk52 = code.WordAt(at + 13);
                code.position += 15;
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

        if (opcode == 0)
        {
            experience = code.DwordAt(code.position + 0x16);
            code.position += 0x1A;
        }
        else if (opcode == 1)
        {
            code.position += 2;
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

    file->SeekBegin(4 * (index + kLevelsPerGroup * group) + static_cast<i32>(kLevelIndex));
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

}  // namespace swd3
