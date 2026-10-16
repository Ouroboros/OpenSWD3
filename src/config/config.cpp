#include "config/config.h"
#include "core/format.h"

#include <algorithm>
#include <cstring>
#include <vector>

#include "file/file.h"

namespace swd3
{

namespace
{

constexpr std::string_view kEnvFile = "Env.dat";
constexpr std::size_t kKeysOffset = 4;
constexpr std::size_t kUnknown14Offset = 0x14;
constexpr std::size_t kSettingsOffset = 0x18;
constexpr std::size_t kMasksOffset = 0x1E;
constexpr std::size_t kDebugOffset = 0x24;
constexpr std::size_t kOldStringsOffset = 0x2A;
constexpr std::size_t kStringsOffset = 0x2E;
constexpr std::size_t kClearedEnd = 0x2E;

u32 ReadDword(const std::vector<u8>& data, std::size_t offset)
{
    u32 value = 0;
    std::memcpy(&value, data.data() + offset, 4);
    return value;
}

void WriteDword(std::vector<u8>& data, std::size_t offset, u32 value)
{
    std::memcpy(data.data() + offset, &value, 4);
}

// Reads the whole file into the 0x1000-byte buffer (the original reads the file size into it unchecked).
void ReadInto(File& file, std::vector<u8>& data)
{
    u32 size = std::min(file.Size(), Config::EnvBytes);
    file.Read(data.data(), size);
}

std::string StringAt(const std::vector<u8>& data, std::size_t offset)
{
    std::string text;

    while (offset < data.size() && data[offset] != 0)
    {
        text.push_back(static_cast<char>(data[offset]));
        ++offset;
    }

    return text;
}

// The end of the NUL-terminated string at `offset` as the original scans it: the offset itself for an empty string.
std::size_t StringEnd(const std::vector<u8>& data, std::size_t offset)
{
    std::size_t end = offset;

    if (data[end] != 0)
    {
        do
        {
            ++end;
        } while (end < data.size() && data[end] != 0);
    }

    return end;
}

void SetLowByte(i32& value, u8 byte)
{
    value = static_cast<i32>((static_cast<u32>(value) & 0xFFFFFF00u) | byte);
}

}  // namespace

// swd32.exe 0x00423A10: creates Env.dat when missing, then updates it
// swd32.exe 0x00498110 (EH unwind: the cFile, closed by its destructor)
// swd32.exe 0x0049811B (EH handler of 0x00423A10)
void Config::Write(std::string_view installDirectory, std::string_view cdRoot, const Settings& settings)
{
    File file;

    if (file.Open(Concat({this->installDirectory, kEnvFile}), File::Disposition::OpenAlways, File::Access::Write))
    {
        file.Close();
        this->Update(installDirectory, cdRoot, settings);
    }
}

// swd32.exe 0x00423AF0: rewrites the head of Env.dat: the format mark, the keys, the settings and the directories
// with the installation type. A file of the old format (no mark) is moved up by 4 bytes first and its channel masks
// and debug byte cleared. The original writes long directories past its 0x1000-byte buffer; they are cut here.
// swd32.exe 0x00498130 (EH unwind: the cFile, closed by its destructor)
// swd32.exe 0x0049813B (EH handler of 0x00423AF0)
void Config::Update(std::string_view installDirectory, std::string_view cdRoot, const Settings& settings)
{
    File file;

    if (!file.Open(
            Concat({this->installDirectory, kEnvFile}),
            File::Disposition::OpenExisting,
            File::Access::ReadWrite
        ))
    {
        return;
    }

    std::vector<u8> data(Config::EnvBytes, 0);
    ReadInto(file, data);
    bool oldFormat = false;

    if (ReadDword(data, 0) != Config::CurrentFormat)
    {
        std::memmove(data.data() + 4, data.data(), Config::EnvBytes - 4);
        oldFormat = true;
    }

    WriteDword(data, 0, Config::CurrentFormat);

    for (int i = 0; i < Config::EnvKeys; ++i)
    {
        data[kKeysOffset + static_cast<std::size_t>(i)] =
            static_cast<u8>(this->keys[static_cast<std::size_t>(Config::EnvKeyOrder[static_cast<std::size_t>(i)])]);
    }

    WriteDword(data, kUnknown14Offset, static_cast<u32>(settings.unknown14));
    data[kSettingsOffset] = settings.soundVolume;
    data[kSettingsOffset + 1] = settings.musicVolume;
    data[kSettingsOffset + 2] = settings.cacheSize;
    data[kSettingsOffset + 3] = settings.effectsOff;
    data[kSettingsOffset + 4] = settings.textSpeed;
    data[kSettingsOffset + 5] = settings.battleSpeed;

    if (oldFormat)
    {
        std::fill(data.begin() + kMasksOffset, data.begin() + kClearedEnd, u8{0});
    }

    std::string tail(installDirectory);
    tail.push_back('\0');
    tail.append(cdRoot);
    tail.push_back('\0');
    tail.push_back(static_cast<char>(this->installType));
    tail.push_back('\0');
    const std::size_t copied = std::min(tail.size(), data.size() - kStringsOffset);
    std::memcpy(data.data() + kStringsOffset, tail.data(), copied);
    u32 bytes = static_cast<u32>(kStringsOffset + copied);
    file.SeekBegin(0);
    file.Write(data.data(), bytes);
}

// swd32.exe 0x00423E00: the 16 key bytes over the head of Env.dat (where the format mark was: the next start reads
// it as the old format)
// swd32.exe 0x00498150 (EH unwind: the cFile, closed by its destructor)
// swd32.exe 0x0049815B (EH handler of 0x00423E00)
void Config::SaveKeys()
{
    File file;

    if (!file.Open(Concat({this->installDirectory, kEnvFile}), File::Disposition::OpenExisting, File::Access::Write))
    {
        return;
    }

    std::array<u8, Config::EnvKeys> data = {};

    for (int i = 0; i < Config::EnvKeys; ++i)
    {
        data[static_cast<std::size_t>(i)] =
            static_cast<u8>(this->keys[static_cast<std::size_t>(Config::EnvKeyOrder[static_cast<std::size_t>(i)])]);
    }

    u32 bytes = Config::EnvKeys;
    file.Write(data.data(), bytes);
}

// swd32.exe 0x00423FB0: the settings of Env.dat; true when the file was of the current format. An old-format file
// gets the default keys and settings written back with its directories, and is read again from the directory it
// names. The volumes, cache size, speeds and effects byte replace only the low byte of their values.
// swd32.exe 0x00498170 (EH unwind: the cFile, closed by its destructor)
// swd32.exe 0x0049817B (EH handler of 0x00423FB0)
bool Config::Load(i32& unknown14, u8& effectsOff)
{
    File file;

    if (!file.Open(Concat({this->installDirectory, kEnvFile}), File::Disposition::OpenExisting, File::Access::Read))
    {
        return false;
    }

    this->keys.fill(0);
    bool current = true;
    std::vector<u8> data(Config::EnvBytes, 0);
    ReadInto(file, data);

    if (ReadDword(data, 0) != Config::CurrentFormat)
    {
        file.Close();
        const std::size_t firstEnd = StringEnd(data, kOldStringsOffset);
        const std::size_t secondEnd = StringEnd(data, firstEnd + 1);
        this->installType = data[secondEnd + 1];
        this->SetDefaultKeys();
        const std::string directory = StringAt(data, kOldStringsOffset);
        Settings defaults;
        defaults.soundVolume = 6;
        defaults.musicVolume = 6;
        defaults.cacheSize = 0x3C;
        defaults.unknown14 = 0x64;
        defaults.effectsOff = 1;
        defaults.textSpeed = 2;
        defaults.battleSpeed = 0xA;
        this->Update(directory, StringAt(data, firstEnd + 1), defaults);
        current = false;
        file.Open(Concat({directory, kEnvFile}), File::Disposition::OpenExisting, File::Access::Read);
        ReadInto(file, data);
    }

    for (int i = 0; i < Config::EnvKeys; ++i)
    {
        SetLowByte(
            this->keys[static_cast<std::size_t>(Config::EnvKeyOrder[static_cast<std::size_t>(i)])],
            data[kKeysOffset + static_cast<std::size_t>(i)]
        );
    }

    unknown14 = static_cast<i32>(ReadDword(data, kUnknown14Offset));
    SetLowByte(this->soundVolume, data[kSettingsOffset]);
    SetLowByte(this->musicVolume, data[kSettingsOffset + 1]);
    SetLowByte(this->cacheSize, data[kSettingsOffset + 2]);
    effectsOff = data[kSettingsOffset + 3];
    SetLowByte(this->textSpeed, data[kSettingsOffset + 4]);
    SetLowByte(this->battleSpeed, data[kSettingsOffset + 5]);

    for (std::size_t i = 0; i < this->channelMasks.size(); ++i)
    {
        this->channelMasks[i] = static_cast<u16>(data[kMasksOffset + 2 * i] | (data[kMasksOffset + 2 * i + 1] << 8));
    }

    this->debugMode = data[kDebugOffset] & 1;
    this->installDirectory = StringAt(data, kStringsOffset);
    const std::size_t rootStart = StringEnd(data, kStringsOffset) + 1;
    this->cdRoot = StringAt(data, rootStart);
    this->installType = data[StringEnd(data, rootStart) + 1];
    return current;
}

// swd32.exe 0x00424390
void Config::SetDefaultKeys()
{
    constexpr std::array<std::pair<int, i32>, Config::EnvKeys> kDefaults = {{
        {0, 0x01},
        {1, 0x39},
        {2, 0x36},
        {3, 0xCB},
        {4, 0xC8},
        {5, 0xCD},
        {6, 0xD0},
        {7, 0xC9},
        {8, 0xD1},
        {9, 0x9D},
        {10, 0xCF},
        {12, 0x1C},
        {16, 0x13},
        {17, 0x1E},
        {18, 0x22},
        {19, 0x3B},
    }};

    for (const auto& [slot, key] : kDefaults)
    {
        this->keys[static_cast<std::size_t>(slot)] = key;
    }
}

}  // namespace swd3
