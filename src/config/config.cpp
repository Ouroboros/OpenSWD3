#include "config/config.h"
#include "core/format.h"

#include <algorithm>
#include <climits>
#include <cstring>
#include <vector>

#include "file/file.h"
#include "input/controls.h"
#include "input/input.h"

namespace swd3
{

namespace
{

constexpr std::string_view kEnvFile = "Env.dat";
constexpr u8 kDebugBit = 1;
constexpr u8 kEnvBattleSpeed = 0xA;  // the battle speed a converted Env.dat gets (a fresh install takes 0xB)

constexpr int Slot(Button button)
{
    return static_cast<int>(button);
}

u32 ReadDword(const std::vector<u8>& data, std::size_t offset)
{
    u32 value = 0;
    std::memcpy(&value, data.data() + offset, sizeof(value));
    return value;
}

void WriteDword(std::vector<u8>& data, std::size_t offset, u32 value)
{
    std::memcpy(data.data() + offset, &value, sizeof(value));
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
        std::memmove(data.data() + sizeof(u32), data.data(), Config::EnvBytes - sizeof(u32));
        oldFormat = true;
    }

    WriteDword(data, 0, Config::CurrentFormat);

    for (int i = 0; i < Config::EnvKeys; ++i)
    {
        data[Config::KeysAt + static_cast<std::size_t>(i)] =
            static_cast<u8>(this->keys[static_cast<std::size_t>(Config::EnvKeyOrder[static_cast<std::size_t>(i)])]);
    }

    WriteDword(data, Config::Unknown14At, static_cast<u32>(settings.unknown14));
    data[Config::SettingsAt + Config::SoundVolumeAt] = settings.soundVolume;
    data[Config::SettingsAt + Config::MusicVolumeAt] = settings.musicVolume;
    data[Config::SettingsAt + Config::CacheSizeAt] = settings.cacheSize;
    data[Config::SettingsAt + Config::EffectsOffAt] = settings.effectsOff;
    data[Config::SettingsAt + Config::TextSpeedAt] = settings.textSpeed;
    data[Config::SettingsAt + Config::BattleSpeedAt] = settings.battleSpeed;

    if (oldFormat)
    {
        std::fill(data.begin() + Config::MasksAt, data.begin() + Config::StringsAt, u8{0});
    }

    std::string tail(installDirectory);
    tail.push_back('\0');
    tail.append(cdRoot);
    tail.push_back('\0');
    tail.push_back(static_cast<char>(this->installType));
    tail.push_back('\0');
    const std::size_t copied = std::min(tail.size(), data.size() - Config::StringsAt);
    std::memcpy(data.data() + Config::StringsAt, tail.data(), copied);
    u32 bytes = static_cast<u32>(Config::StringsAt + copied);
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
        const std::size_t firstEnd = StringEnd(data, Config::OldStringsAt);
        const std::size_t secondEnd = StringEnd(data, firstEnd + 1);
        this->installType = data[secondEnd + 1];
        this->SetDefaultKeys();
        const std::string directory = StringAt(data, Config::OldStringsAt);
        Settings defaults;
        defaults.soundVolume = Config::DefaultVolume;
        defaults.musicVolume = Config::DefaultVolume;
        defaults.cacheSize = Config::DefaultCacheMegabytes;
        defaults.unknown14 = Config::DefaultUnknown14;
        defaults.effectsOff = 1;
        defaults.textSpeed = Config::DefaultTextSpeed;
        defaults.battleSpeed = kEnvBattleSpeed;
        this->Update(directory, StringAt(data, firstEnd + 1), defaults);
        current = false;
        file.Open(Concat({directory, kEnvFile}), File::Disposition::OpenExisting, File::Access::Read);
        ReadInto(file, data);
    }

    for (int i = 0; i < Config::EnvKeys; ++i)
    {
        SetLowByte(
            this->keys[static_cast<std::size_t>(Config::EnvKeyOrder[static_cast<std::size_t>(i)])],
            data[Config::KeysAt + static_cast<std::size_t>(i)]
        );
    }

    unknown14 = static_cast<i32>(ReadDword(data, Config::Unknown14At));
    SetLowByte(this->soundVolume, data[Config::SettingsAt + Config::SoundVolumeAt]);
    SetLowByte(this->musicVolume, data[Config::SettingsAt + Config::MusicVolumeAt]);
    SetLowByte(this->cacheSize, data[Config::SettingsAt + Config::CacheSizeAt]);
    effectsOff = data[Config::SettingsAt + Config::EffectsOffAt];
    SetLowByte(this->textSpeed, data[Config::SettingsAt + Config::TextSpeedAt]);
    SetLowByte(this->battleSpeed, data[Config::SettingsAt + Config::BattleSpeedAt]);

    for (std::size_t i = 0; i < this->channelMasks.size(); ++i)
    {
        this->channelMasks[i] = static_cast<u16>(
            data[Config::MasksAt + sizeof(u16) * i] | (data[Config::MasksAt + sizeof(u16) * i + 1] << CHAR_BIT)
        );
    }

    this->debugMode = data[Config::DebugAt] & kDebugBit;
    this->installDirectory = StringAt(data, Config::StringsAt);
    const std::size_t rootStart = StringEnd(data, Config::StringsAt) + 1;
    this->cdRoot = StringAt(data, rootStart);
    this->installType = data[StringEnd(data, rootStart) + 1];
    return current;
}

// swd32.exe 0x00424390
void Config::SetDefaultKeys()
{
    constexpr std::array<std::pair<int, i32>, Config::EnvKeys> kDefaults = {{
        {Slot(Button::Escape), Input::Escape},
        {Slot(Button::Space), Input::Space},
        {Slot(Button::RightShift), Input::RightShift},
        {Slot(Button::Left), Input::Left},
        {Slot(Button::Up), Input::Up},
        {Slot(Button::Right), Input::Right},
        {Slot(Button::Down), Input::Down},
        {Slot(Button::PageUp), Input::PageUp},
        {Slot(Button::PageDown), Input::PageDown},
        {Slot(Button::RightControl), Input::RightControl},
        {Slot(Button::End), Input::End},
        {Slot(Button::Enter), Input::Enter},
        {Slot(Button::KeyR), Input::R},
        {Slot(Button::KeyA), Input::A},
        {Slot(Button::KeyG), Input::G},
        {Slot(Button::F1), Input::F1},
    }};

    for (const auto& [slot, key] : kDefaults)
    {
        this->keys[static_cast<std::size_t>(slot)] = key;
    }
}

}  // namespace swd3
