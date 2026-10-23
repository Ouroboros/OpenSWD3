#include "config/config.h"

#include <algorithm>
#include <cstring>
#include <functional>
#include <string>
#include <vector>

#include "core/log.h"
#include "file/file.h"
#include "input/controls.h"
#include "input/input.h"

namespace swd3
{

namespace
{

constexpr u8 kDebugBit = 1;

constexpr int Slot(Button button)
{
    return static_cast<int>(button);
}

// The key slots [game.keys] holds: its members and their slots, in the rows of the key setup (swd32.exe 0x004A7210).
struct KeySlot
{
    std::optional<i64> GameKeys::* member;
    Button button;
};

constexpr std::array<KeySlot, Config::EnvKeys> kKeySlots = {{
    {&GameKeys::up, Button::Up},
    {&GameKeys::down, Button::Down},
    {&GameKeys::left, Button::Left},
    {&GameKeys::right, Button::Right},
    {&GameKeys::menu, Button::Enter},
    {&GameKeys::confirm, Button::Space},
    {&GameKeys::cancel, Button::RightControl},
    {&GameKeys::giveUp, Button::Escape},
    {&GameKeys::switchOne, Button::End},
    {&GameKeys::switchTwo, Button::RightShift},
    {&GameKeys::help, Button::F1},
    {&GameKeys::pageUp, Button::PageUp},
    {&GameKeys::pageDown, Button::PageDown},
    {&GameKeys::run, Button::KeyR},
    {&GameKeys::autoAttack, Button::KeyA},
    {&GameKeys::flee, Button::KeyG},
}};

u32 ReadDword(std::span<const u8> data, std::size_t offset)
{
    u32 value = 0;
    std::memcpy(&value, data.data() + offset, sizeof(value));
    return value;
}

u16 ReadWord(std::span<const u8> data, std::size_t offset)
{
    u16 value = 0;
    std::memcpy(&value, data.data() + offset, sizeof(value));
    return value;
}

}  // namespace

// swd32.exe 0x00423FB0 (the settings of Env.dat) as the rebuild's one import of Env.dat into [game]: a file of the
// current format gives its keys, unknown14, settings bytes, channel masks, debug bit and session mark (its directories
// and installation type are not used any more). An old-format file gives nothing (the original turned it into the
// defaults). See deviations.md.
// swd32.exe 0x00498170 (EH unwind of 0x00423FB0: the new cFile's memory freed when its constructor throws)
// swd32.exe 0x0049817B (EH handler of 0x00423FB0)
std::optional<GameSettings> Config::ImportEnv(std::span<const u8> data)
{
    if (data.size() < Config::StringsAt || ReadDword(data, 0) != Config::CurrentFormat)
    {
        return std::nullopt;
    }

    GameSettings game;

    for (int index = 0; index < Config::EnvKeys; ++index)
    {
        const int slot = Config::EnvKeyOrder[static_cast<std::size_t>(index)];

        for (const auto& [member, button] : kKeySlots)
        {
            if (Slot(button) == slot)
            {
                game.keys.*member = data[Config::KeysAt + static_cast<std::size_t>(index)];
            }
        }
    }

    const std::span<const u8> settings = data.subspan(Config::SettingsAt);
    game.unk14 = ReadDword(data, Config::Unknown14At);
    game.soundVolume = settings[Config::SoundVolumeAt];
    game.musicVolume = settings[Config::MusicVolumeAt];
    game.cacheSize = settings[Config::CacheSizeAt];
    game.effectsOff = settings[Config::EffectsOffAt] != 0;
    game.textSpeed = settings[Config::TextSpeedAt];
    game.battleSpeed = settings[Config::BattleSpeedAt];
    game.redMask = ReadWord(data, Config::MasksAt + sizeof(u16) * Config::RedChannel);
    game.greenMask = ReadWord(data, Config::MasksAt + sizeof(u16) * Config::GreenChannel);
    game.blueMask = ReadWord(data, Config::MasksAt + sizeof(u16) * Config::BlueChannel);
    game.debug = (data[Config::DebugAt] & kDebugBit) != 0;
    game.sessionOpen = data.back() != 0;
    return game;
}

// swd32.exe 0x00423A10 (the installation's first Env.dat, written by the install command line): removed with the
// command line; [game] is imported from Env.dat once instead (see ImportEnvFile and deviations.md).
// swd32.exe 0x00498110 (EH unwind of 0x00423A10: the new cFile's memory freed when its constructor throws)
// swd32.exe 0x0049811B (EH handler of 0x00423A10)

// swd32.exe 0x00423AF0 (Env.dat rewritten with the keys, unknown14 and the settings bytes): the same values set in
// [game] of the program's settings file (its channel masks and debug bit kept). The rewritten file ended with a session
// mark of 0 (0x00423D8C), so saving the settings clears the mark. See deviations.md.
// swd32.exe 0x00498130 (EH unwind of 0x00423AF0: the new cFile's memory freed when its constructor throws)
// swd32.exe 0x0049813B (EH handler of 0x00423AF0)
void Config::Update(const Settings& settings)
{
    this->Store(
        [this, &settings](GameSettings& game)
        {
            this->StoreKeys(game);
            game.sessionOpen = false;
            game.unk14 = static_cast<u32>(settings.unknown14);
            game.soundVolume = settings.soundVolume;
            game.musicVolume = settings.musicVolume;
            game.cacheSize = settings.cacheSize;
            game.effectsOff = settings.effectsOff != 0;
            game.textSpeed = settings.textSpeed;
            game.battleSpeed = settings.battleSpeed;
        }
    );
}

// swd32.exe 0x00423E00 (the key setup's keys written over the head of Env.dat, where they spoilt the format mark so the
// next start reset every setting): the keys set in [game.keys]; nothing else changes. See deviations.md.
// swd32.exe 0x00498150 (EH unwind of 0x00423E00: the new cFile's memory freed when its constructor throws)
// swd32.exe 0x0049815B (EH handler of 0x00423E00)
void Config::SaveKeys()
{
    this->Store(
        [this](GameSettings& game)
        {
            this->StoreKeys(game);
        }
    );
}

// The game directory's Env.dat imported once into [game] of the program's settings `file` (the defaults for a missing
// or old-format Env.dat); Env.dat is not read or written again. See deviations.md.
void Config::ImportEnvFile(const std::u8string& directory, const std::filesystem::path& file)
{
    File env;
    std::vector<u8> data;

    if (env.Open(directory + u8"\\Env.dat", File::Disposition::OpenExisting, File::Access::Read))
    {
        data.assign(std::min(env.Size(), Config::EnvBytes), 0);
        u32 bytes = static_cast<u32>(data.size());
        env.Read(data.data(), bytes);
    }

    env.Close();
    const std::optional<GameSettings> imported = Config::ImportEnv(data);
    std::string failure;
    const bool saved = UpdateSettingsFile(
        file,
        [&imported](swd3::Settings& values)
        {
            values.game = imported.value_or(GameSettings{});
        },
        failure
    );

    if (!saved)
    {
        LogWarning(u8"settings: Env.dat cannot be imported: %s", AsUtf8(failure));
    }
    else if (imported.has_value())
    {
        LogInfo(u8"settings: Env.dat of %s imported into [game]", directory);
    }
    else
    {
        LogInfo(
            u8"settings: no current-format Env.dat in %s (%d bytes): [game] starts from the defaults",
            directory,
            static_cast<int>(data.size())
        );
    }
}

// The 16 key slots the settings file holds, from the keys.
void Config::StoreKeys(GameSettings& game) const
{
    for (const auto& [member, button] : kKeySlots)
    {
        game.keys.*member = static_cast<u8>(this->keys[static_cast<std::size_t>(Slot(button))]);
    }
}

void Config::StoreRunning(bool running) const
{
    this->Store(
        [running](GameSettings& game)
        {
            game.running = running;
        }
    );
}

void Config::StoreLauncher(bool shown) const
{
    this->Store(
        [shown](GameSettings& game)
        {
            game.launcher = shown;
        }
    );
}

void Config::StoreAutoCombo(bool on) const
{
    this->Store(
        [on](GameSettings& game)
        {
            game.autoCombo = on;
        }
    );
}

// [game] of the program's settings file changed by `change` (a failure is logged).
void Config::Store(const std::function<void(GameSettings&)>& change) const
{
    std::string error;

    if (!UpdateSettingsFile(
            this->settingsFile,
            [&change](swd3::Settings& settings)
            {
                change(settings.game);
            },
            error
        ))
    {
        LogWarning(u8"settings: [game] cannot be saved: %s", AsUtf8(error));
    }
}

// The values of [game] over the defaults (the keys the key setup holds, the settings, the masks, the debug bit); a
// value missing keeps the default.
void Config::Apply(const GameSettings& game, i32& unknown14, u8& effectsOff)
{
    for (const auto& [member, button] : kKeySlots)
    {
        if ((game.keys.*member).has_value())
        {
            this->keys[static_cast<std::size_t>(Slot(button))] = static_cast<i32>(*(game.keys.*member));
        }
    }

    auto assign = [](i32& value, const std::optional<i64>& setting)
    {
        if (setting.has_value())
        {
            value = static_cast<i32>(*setting);
        }
    };

    assign(this->soundVolume, game.soundVolume);
    assign(this->musicVolume, game.musicVolume);
    assign(this->cacheSize, game.cacheSize);
    assign(this->textSpeed, game.textSpeed);
    assign(this->battleSpeed, game.battleSpeed);
    assign(unknown14, game.unk14);

    if (game.effectsOff.has_value())
    {
        effectsOff = *game.effectsOff ? 1 : 0;
    }

    this->showLauncher = game.launcher.value_or(true);
    this->autoCombo = game.autoCombo.value_or(false);

    if (game.debug.has_value())
    {
        this->debugMode = *game.debug ? 1 : 0;
    }

    const std::array<const std::optional<i64>*, Channels> masks = {&game.redMask, &game.greenMask, &game.blueMask};

    for (std::size_t channel = 0; channel < masks.size(); ++channel)
    {
        if (masks[channel]->has_value())
        {
            this->channelMasks[channel] = static_cast<u16>(**masks[channel]);
        }
    }
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
