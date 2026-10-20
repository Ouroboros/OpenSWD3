#include <algorithm>
#include <bit>
#include <climits>
#include <cmath>
#include <cstring>
#include <span>
#include <string>

#include <SDL3/SDL.h>

#include "core/debug_log.h"
#include "core/fault.h"
#include "core/format.h"
#include "core/words.h"
#include "crt/msvc.h"
#include "game/game.h"
#include "platform/system_info.h"
#include "role/path_script.h"
#include "role/role_path.h"
#include "talk/talk_event.h"

namespace swd3
{

namespace
{

// The effect bits of a map (maps.dat table 3, byte 5).
constexpr u8 kFireflies = 1;
constexpr u8 kSpotlight = 2;
constexpr u8 kLightning = 4;
constexpr u8 kHeatHaze = 8;
constexpr u8 kLeaves = 0x10;
constexpr u8 kShadeWeather = 0x20;
constexpr u8 kGlowWeather = 0x40;
constexpr u8 kOverlaysFirst = 0x80;

constexpr int kSceneFadeLength = 0xF;  // the fade of the scene music when the map changes it
constexpr int kLoadingAction = 0x4F;
constexpr int kLoadingBarWidth = 0x18A;
constexpr std::size_t kScreenBytes = sizeof(Pixel) * Display::Pixels;
constexpr u16 kPartyLeader = 1;
constexpr u16 kSpecialItem = 0x192;

// The loading screen: the bar shades from (kBarRed, kBarGreen, kBarBlue) by the steps per column in 1/kColorFraction
// (8-bit channels, kChannelDrop bits dropped for the display); the runner walks at its end.
constexpr int kLoadingStart = -1;
constexpr int kLoadingSettings = 2;  // the progress of LoadCity
constexpr int kLoadingCleared = 10;
constexpr int kLoadingNamed = 85;
constexpr int kLoadingPlaced = 90;
constexpr int kLoadingCentered = 95;
constexpr int kLoadingDone = 100;
constexpr u32 kRunners = 2;  // the runner is act 1 or 2
constexpr i32 kBarRed = 0x1F;
constexpr i32 kBarGreen = 0x73;
constexpr i32 kBarBlue = 0xFF;
constexpr int kBarRedStep = 0x246;
constexpr int kBarGreenStep = 0xDA;
constexpr int kBarBlueFall = 0x296;
constexpr int kColorFraction = 0x400;
constexpr int kChannelDrop = 3;
constexpr int kBarLeft = 0x7B;
constexpr int kBarTop = 0x1BD;
constexpr int kBarHeight = 0x1E;
constexpr int kPictureX = 0x78;
constexpr int kPictureY = 0x1BA;
constexpr int kStartPictureY = 0xCA;  // the picture before the screen is cleared
constexpr int kRunnerX = 0x6B;        // past the end of the bar
constexpr int kRunnerY = 0x1CC;

// Map settings (maps.dat table 3): the kind and speed bytes hold nibbles.
constexpr int kNibbleBits = 4;
constexpr u16 kNibbleMask = 0xF;
constexpr u8 kNoUnk4ACDB4 = 0x10;    // in the kind: townWarpAllowed is 0
constexpr u32 kMaxWalkSpeed = 0x10;  // the walk speed is a power of two up to this
constexpr i32 kDefaultWalkSpeed = 4;
constexpr u32 kStartSpeeds = 3;  // a weather particle starts at speed 1..kStartSpeeds
constexpr int kStartDrift = 2;

// The roles of maps.dat's role table.
constexpr u16 kEveryMapRole = 10000;  // records of these roles (and of none) belong to every map
constexpr u16 kEveryMapRole2 = 10001;
constexpr u16 kLeaderFlags = Role::Visible | Role::Solid | Role::StepsAct | Role::CellEffects;
constexpr u16 kStoryActMark = 0x8000;  // in mapStory: the party leader takes a story act
constexpr i32 kLeaderStoryAct = 0x60;
constexpr i32 kLeaderSpecialAct = 0x5F;  // with kSpecialItem on the maps of the journey
constexpr u32 kFollowUnlisted = 1;       // a flagged role not in the follower table follows
constexpr u32 kFollowStory = 2;          // a listed one follows for the story values it lists

constexpr int kSoundRange = 0x200;  // pixels a map sound is heard within
constexpr int kSoundScale = 0x80;
constexpr int kPanScale = 0x40;
constexpr int kPlayOnce = 1;

// The encounter sets (maps.dat table 0x12): their count, then MapsRecord::GroupWords words each: kSetPairs pairs of a
// step threshold and a chance in percent.
constexpr int kSetPairs = 4;
constexpr std::size_t kPairWords = 2;
constexpr u32 kPercent = 100;
constexpr int kForcedEncounter = 1;  // RollEncounter `force`: the thresholds and the chance are skipped
constexpr i32 kTileMask = 0xF;       // ReleaseRole: the pixels within a tile

}  // namespace

// swd32.exe 0x0040ED60: the loading screen: a bar of `percent` (its colour shading along it) under the picture of act
// 0x232A, a random runner (1 or 2) at its end; -1 starts it, 100 ends it. Nothing while flag 0x46 is set.
void Game::ShowLoading(int percent)
{
    Scene& view = this->scene;
    int shown = percent;

    if (percent == kLoadingStart)
    {
        this->loadingPercent = 0;
        this->loadingUnk4CC2C0 = 0;
        shown = 0;
        this->loadingRunner = static_cast<i32>(this->random.Below(kRunners)) + 1;
        this->loadingRed = kBarRed;
        this->loadingGreen = kBarGreen;
        this->loadingBlue = kBarBlue;
        this->loadingUnk4CC2C4 = 0;
        ResetPlayback(view.loadingPicture);
        ResetPlayback(this->loadingAct);
    }

    if (!this->state.GameFlag(GameState::NoLoadingScreen))
    {
        this->drawSurface = this->painter.LockSurface(*this->offscreen);
        this->painter.SetTarget(this->drawSurface);
        this->painter.UnlockSurface(*this->offscreen);
        std::memset(this->drawSurface, 0, kScreenBytes);
        const int length = kLoadingBarWidth * shown / kLoadingDone;
        const int pitch = this->offscreen->Width();
        int red = 0;
        int green = 0;
        int blue = 0;

        for (int column = 0; column <= length; ++column)
        {
            this->loadingBlue = blue / kColorFraction + kBarBlue;
            this->loadingGreen = green / kColorFraction + kBarGreen;
            this->loadingRed = red / kColorFraction + kBarRed;
            const auto color = static_cast<u16>(this->painter.MakeColor(
                static_cast<u32>(this->loadingRed >> kChannelDrop),
                static_cast<u32>(this->loadingGreen >> kChannelDrop),
                static_cast<u32>(this->loadingBlue >> kChannelDrop)
            ));
            u16* pixel = this->drawSurface + pitch * kBarTop + column + kBarLeft;

            for (int row = 0; row < kBarHeight; ++row)
            {
                *pixel = color;
                pixel += pitch;
            }

            red += kBarRedStep;
            green += kBarGreenStep;
            blue -= kBarBlueFall;
        }

        this->loadingPercent = shown;
        view.loadingPicture.id = Scene::InterfaceAct;
        view.loadingPicture.action = kLoadingAction;
        view.loadingPicture.direction = 0;
        view.loadingPicture.x = 0;
        view.loadingPicture.y = 0;
        this->actDrawer.Draw(view.loadingPicture, kPictureX, kPictureY);
        this->loadingAct.id = this->loadingRunner;
        this->loadingAct.action = Role::RunAction;
        this->loadingAct.direction = ActState::Right;
        this->actDrawer.Draw(this->loadingAct, length + kRunnerX, kRunnerY);
        this->ShowFrame();
    }

    if (percent == kLoadingDone)
    {
        this->state.ClearGameFlag(GameState::NoLoadingScreen);
    }
}

// swd32.exe 0x0040CF40: the music of map `map` (maps.dat's music table): a new pair of tunes fades the playing one out
// unless the music is held; bits 13..15 of the entry set the cue marks
void Game::MusicZone(int map)
{
    Scene& view = this->scene;
    const std::vector<u8>& data = this->databases.mapsData;
    const u32 lists = MapsTable(data, MapsDataTable::Music);
    u32 at = MapsDword(data, MapsDword(data, lists + sizeof(u32) * MapsRecord::SceneMusicItem));

    for (;; at += MapsRecord::MusicBytes)
    {
        const u16 entry = MapsWord(data, at);

        if (entry == 0 || at + 1 >= data.size())
        {
            view.musicTunes[Scene::SceneFirst] = 0;
            view.musicTunes[Scene::SceneSecond] = 0;
            view.musicCue &= ~(Scene::SceneLoops | Scene::SceneRepeatFirst);
            return;
        }

        if (static_cast<int>(entry) == map)
        {
            break;
        }
    }

    const i32 first = MapsWord(data, at + MapsRecord::MusicFirstAt);
    const i32 second = MapsWord(data, at + MapsRecord::MusicSecondAt);

    if (first != view.musicTunes[Scene::SceneFirst] || second != view.musicTunes[Scene::SceneSecond])
    {
        view.musicTunes[Scene::SceneFirst] = first;
        view.musicTunes[Scene::SceneSecond] = second;

        if ((view.musicCue & Scene::StoryMusic) == 0)
        {
            view.musicCue = 0;

            if (!this->audio.IsMusicStopped())
            {
                this->audio.SetMusicFade(AudioManager::FadeOverLength, kSceneFadeLength);
                this->audio.UpdateMusicFade();
            }

            DebugLog(u8"StreamStartOut");
        }

        view.musicCue &= Scene::StoryMusic | (Scene::UnkMusic100000 - 1);
        const u16 marks = MapsWord(data, at + MapsRecord::MusicMarksAt);
        view.musicCue &= ~(Scene::SceneLoops | Scene::SceneRepeatFirst);

        if ((marks & Scene::MarksNone) == 0)
        {
            if ((marks & Scene::MarkRepeatFirst) != 0)
            {
                view.musicCue |= Scene::SceneLoops | Scene::SceneRepeatFirst;
            }

            if ((marks & Scene::MarkLoops) != 0)
            {
                view.musicCue |= Scene::SceneLoops;
            }
        }
    }
}

// swd32.exe 0x0040D060: the sound of a role and its period (in both halves of the timer) from maps.dat's sound table;
// false when it has none
bool Game::LoadRoleSound(Role& role)
{
    const std::vector<u8>& data = this->databases.mapsData;

    for (u32 at = MapsTable(data, MapsDataTable::RoleSounds); at + 1 < data.size(); at += MapsRecord::SoundBytes)
    {
        const u16 id = MapsWord(data, at);

        if (id == 0)
        {
            return false;
        }

        if (id == role.id)
        {
            role.sound = MapsWord(data, at + MapsRecord::SoundAt);
            const u16 period = MapsWord(data, at + MapsRecord::SoundPeriodAt);
            role.soundTimer = MakeWords(period, period);
            return true;
        }
    }

    return false;
}

// swd32.exe 0x0040D9E0: the encounter areas of the current map (maps.dat's table 7) put in front of the list
void Game::BuildTriggers(MapTrigger& head)
{
    const std::vector<u8>& data = this->databases.mapsData;

    for (u32 at = MapsTable(data, MapsDataTable::EncounterAreas); MapsWord(data, at) != MapsRecord::TableEnd;
         at += MapsRecord::AreaBytes)
    {
        if (static_cast<i32>(MapsWord(data, at)) == this->currentMap)
        {
            auto node = std::make_unique<MapTrigger>();
            node->left = MapsWord(data, at + MapsRecord::AreaLeftAt);
            node->top = MapsWord(data, at + MapsRecord::AreaTopAt);
            node->right = MapsWord(data, at + MapsRecord::AreaRightAt);
            node->bottom = MapsWord(data, at + MapsRecord::AreaBottomAt);
            node->table = MapsDword(data, at + MapsRecord::AreaTableAt);
            node->next = std::move(head.next);
            head.next = std::move(node);
        }
    }
}

// swd32.exe 0x0040F280: a role entering the map: its action started, its cell of the grid and the marks of a mark
// cell (bit 11 sets flag 29, bits 12..15 go to flags 20..23); a solid role takes its cells
void Game::InitRole(Role& role)
{
    role.act.flags = 0;

    if (this->acts.Run(role.act) == 0)
    {
        LogActError(u8"Act Err(Misc:newRoleInit)", role);
    }

    role.flags &= ~(Role::Reflects | Role::CellShades);
    const auto column = static_cast<u32>(role.x) >> MapHeader::TileShift;
    const auto row = static_cast<u32>(role.y) >> MapHeader::TileShift;
    const std::size_t at = column + static_cast<std::size_t>(this->state.map.width) * static_cast<std::size_t>(row);
    std::vector<u32>& blocks = this->state.map.blocks;
    role.cell = at < blocks.size() ? &blocks[at] : nullptr;

    if ((role.flags & Role::CellEffects) != 0 && role.cell != nullptr)
    {
        const u32 cell = *role.cell;

        if ((cell & MapHeader::Reflecting) != 0)
        {
            role.flags |= Role::Reflects;
        }

        if ((cell & MapHeader::ShadeMask) != 0)
        {
            role.flags = (role.flags & ~Role::CellShades) | (cell & MapHeader::ShadeMask) << MapHeader::ShadeShift;
        }
    }

    if (role.act.id != 0 && (role.flags & (Role::Visible | Role::Hidden)) == Role::Visible)
    {
        this->field.MarkRoleCells(role, this->partyLeaderId);
    }
}

// swd32.exe 0x0040EFD0: the name of map `map` (maps.dat's name table, Big5 ending with 0x5125); false when it has none
bool Game::MapName(int map, std::string& name) const
{
    const std::vector<u8>& data = this->databases.mapsData;
    std::size_t at = MapsTable(data, MapsDataTable::PlaceNames);
    u16 code = MapsWord(data, at);

    if (code == MapsRecord::TableEnd)
    {
        return false;
    }

    while (static_cast<int>(code) != map)
    {
        if (code != MapsRecord::NameEnd)
        {
            do
            {
                ++at;
            } while (at + 1 < data.size() && MapsWord(data, at) != MapsRecord::NameEnd);
        }

        code = MapsWord(data, at + MapsRecord::PlaceNameAt);
        at += sizeof(u16);

        if (code == MapsRecord::TableEnd || at + 1 >= data.size())
        {
            return false;
        }
    }

    name.clear();

    for (std::size_t character = at + MapsRecord::PlaceNameAt;
         character + 1 < data.size() && MapsWord(data, character) != MapsRecord::NameEnd;
         ++character)
    {
        name.push_back(static_cast<char>(data[character]));
    }

    return true;
}

// swd32.exe 0x0040D460: changes the record of role `id` in maps.dat's role table (0xFFFF leaves a value; the act's
// new action clears its step); false when the table lacks it
bool Game::UpdateRoleRecord(
    u16 id,
    i16 act,
    i16 action,
    i16 direction,
    i16 x,
    i16 y,
    i16 event,
    i16 talk,
    i16 setBits,
    i16 clearBits,
    i16 map
)
{
    std::vector<u8>& data = this->databases.mapsData;

    for (std::size_t at = this->scene.rolesTable;; at += static_cast<std::size_t>(sizeof(u16) * MapsRecord::RoleWords))
    {
        if (at + static_cast<std::size_t>(sizeof(u16) * MapsRecord::RoleWords) > data.size() ||
            MapsWord(data, at) == MapsRecord::TableEnd)
        {
            DebugLog(u8"未定義角色[%d]", id);
            return false;
        }

        if (MapsWord(data, at + sizeof(u16) * MapsRecord::RoleId) == id)
        {
            auto put = [&data, at](int word, i16 value)
            {
                const std::size_t offset = at + sizeof(u16) * static_cast<std::size_t>(word);
                data[offset] = static_cast<u8>(value);
                data[offset + 1] = static_cast<u8>(static_cast<u16>(value) >> CHAR_BIT);
            };

            auto get = [&data, at](int word)
            {
                return static_cast<i16>(MapsWord(data, at + sizeof(u16) * static_cast<std::size_t>(word)));
            };

            if (act != -1)
            {
                put(MapsRecord::RoleAct, act);
            }

            if (action != -1)
            {
                put(MapsRecord::RoleAction, action);
            }

            if (direction != -1)
            {
                put(MapsRecord::RoleDirection, direction);
            }

            if (x != -1)
            {
                put(MapsRecord::RoleColumn, x);
            }

            if (y != -1)
            {
                put(MapsRecord::RoleRow, y);
            }

            if (event != -1)
            {
                put(MapsRecord::RoleEvent, event);
            }

            if (talk != -1)
            {
                put(MapsRecord::RolePath, talk);
                put(MapsRecord::RoleStep, 0);
            }

            if (clearBits != -1)
            {
                put(MapsRecord::RoleFlags, static_cast<i16>(get(MapsRecord::RoleFlags) & clearBits));
            }

            if (setBits != -1)
            {
                put(MapsRecord::RoleFlags, static_cast<i16>(get(MapsRecord::RoleFlags) | setBits));
            }

            if (map != -1)
            {
                put(MapsRecord::RoleMap, map);
            }

            return true;
        }
    }
}

// swd32.exe 0x0040D200: the roles of the map written back to maps.dat before it is left: the visible named roles not
// controlled (a role marked by bit 7 of its flags with its place, act and story values; the others go to their tile, a
// role on a path to the end of its move)
void Game::SaveRoleStates(i16 x, i16 y)
{
    for (int index = 1; index != this->state.roleCount; ++index)
    {
        Role copy = this->state.roles[static_cast<std::size_t>(index)];

        if ((copy.flags & (Role::Removed | Role::Unk8000000)) != 0 || copy.id == Role::NoId ||
            index == this->state.playerRole)
        {
            continue;
        }

        if ((copy.flags & Role::Follower) == 0)
        {
            if (copy.unk1C != 0)
            {
                const u8* paths = this->databases.pathData;
                const std::span<const u8> pathBytes =
                    paths != nullptr ? std::span<const u8>(paths, this->databases.paths.Size()) : std::span<const u8>();
                const u32 path = PathScript::PathStart(pathBytes, static_cast<std::size_t>(copy.unk1C));
                copy.x &= ~MapHeader::TileFraction;
                copy.y &= ~MapHeader::TileFraction;
                const std::size_t step = PathScript::WordBytes * static_cast<std::size_t>(static_cast<u32>(copy.unk18));

                if (PathScript::Word(pathBytes, PathScript::DataAt + path + step) ==
                    static_cast<u16>(PathCommand::Step))
                {
                    for (const MoveRecord& move : this->state.moves)
                    {
                        if (move.role == index)
                        {
                            copy.x = move.x;
                            copy.y = move.y;

                            if (static_cast<u32>(move.x) >
                                    static_cast<u32>(MapHeader::TileSize * this->state.map.width) ||
                                move.y > MapHeader::TileSize * this->state.map.height)
                            {
                                DebugLog(
                                    u8"角色回寫 角色(%d)\n終點位置(%d,%d)\n不在地圖(%d,%d)內",
                                    copy.id,
                                    move.x,
                                    move.y,
                                    MapHeader::TileSize * this->state.map.width,
                                    MapHeader::TileSize * this->state.map.height
                                );
                            }

                            break;
                        }
                    }

                    ++copy.unk18;
                }
            }

            this->StoreRole(copy);
        }
        else
        {
            this->UpdateRoleRecord(
                copy.id,
                static_cast<i16>(copy.act.id),
                static_cast<i16>(copy.act.action),
                static_cast<i16>(copy.act.direction),
                x,
                y,
                static_cast<i16>(copy.event),
                static_cast<i16>(copy.unk1C),
                static_cast<i16>(copy.flags),
                0,
                static_cast<i16>(this->currentMap)
            );
        }
    }
}

// swd32.exe 0x0040D610: role `index` joins the followers of the leader: a role on a path is moved back to the start of
// its tile by its last step and taken off its move; its record gets flag 0x80
void Game::FollowLeader(int index)
{
    Role& role = this->state.roles[static_cast<std::size_t>(index)];

    if (role.unk1C != 0)
    {
        for (MoveRecord& move : this->state.moves)
        {
            if (move.role != index)
            {
                continue;
            }

            if ((role.x & MapHeader::TileFraction) != 0 || (role.y & MapHeader::TileFraction) != 0)
            {
                this->field.ClearRoleCells(role, this->partyLeaderId);
                const u8 direction = move.CurrentDirection();

                if (direction < RolePath::Directions)
                {
                    role.x = RolePath::TileStart(role.x, RolePath::StepX[direction]);
                    role.y = RolePath::TileStart(role.y, RolePath::StepY[direction]);
                }

                this->field.RemoveRole(
                    role.id,
                    static_cast<int>(role.flags & Role::LayerMask),
                    (role.y >> MapHeader::TileShift) - 1,
                    0
                );
            }

            this->UpdateRoleRecord(
                role.id,
                Game::KeepValue,
                Game::KeepValue,
                Game::KeepValue,
                Game::KeepValue,
                Game::KeepValue,
                Game::KeepValue,
                Game::KeepValue,
                Role::Follower,
                Game::KeepValue,
                Game::KeepValue
            );
            move.Clear();
            break;
        }
    }

    Scene& view = this->scene;
    const auto follower = static_cast<std::size_t>(view.followerCount);

    if (follower < view.followers.size())
    {
        view.followers[follower] = index;
        view.followerMoves[follower].Clear();
    }

    role.event = 0;
    role.flags = (role.flags & ~Role::Solid) | Role::Follower;
    ++view.followerCount;
}

// swd32.exe 0x004118B0 (the CD check of the minimal installation, the videos and the map movies) and 0x00412050 (the
// file probe it used): removed; the rebuild never looks for a CD and reads everything from the installation (see
// deviations.md)

// swd32.exe 0x0040C130: enters map `map` with the leader at (x, y) tiles in act `act`, action and direction. The map's
// settings (table 3: its unit of HUGE.lmf, the story flags of its effects, its speeds), the music, the old map's roles
// written back (option bit 0), every scene list and role cleared, the map loaded (8-bit maps take their palette from
// the unit), its name, the weather, the encounter areas, the roles of maps.dat's role table on this map (the leader
// moved to the entrance, followers, marks), the camera. A map missing from the tables ends the game.
void Game::LoadCity(int map, int x, int y, i16 act, i16 action, i16 direction, int options)
{
    Scene& view = this->scene;
    GameState& game = this->state;
    const int previous = this->currentMap;
    LogInfo(u8"map %d at (%d, %d) from map %d", map, x, y, previous);
    game.ClearFrameInterval();

    if (this->drawSurface != nullptr)
    {
        view.loadingPicture.id = Scene::InterfaceAct;
        view.loadingPicture.action = kLoadingAction;
        view.loadingPicture.direction = 0;
        view.loadingPicture.x = 0;
        view.loadingPicture.y = 0;
        this->actDrawer.Draw(view.loadingPicture, kPictureX, kStartPictureY);
        this->ShowLoading(kLoadingStart);
    }

    view.screenFlags = 0;
    this->AudioPump();
    game.ClearGameFlag(GameState::Weather);
    game.ClearGameFlag(GameState::Leaves);

    view.weather.fill({MapParticle::Filled, MapParticle::Filled, MapParticle::Filled, MapParticle::Filled});

    ResetPlayback(view.effectAct);
    view.ResetLeaves();
    game.ClearGameFlag(GameState::HeatHaze);
    game.ClearGameFlag(GameState::Lightning);
    std::fill_n(view.bolts.begin(), Scene::ClearedBolts, Streak{});
    view.lightningShown = 0;
    view.lightningCount = 0;
    game.ClearGameFlag(GameState::Fireflies);
    std::fill_n(view.bolts.begin(), Scene::ClearedBolts, Streak{});
    view.firefliesShown = 0;
    view.spotlightX = Display::Width / 2;
    view.spotlightTargetX = Display::Width / 2;
    view.fireflyCount = 0;
    view.spotlightY = Display::Height / 2;
    view.spotlightTargetY = Display::Height / 2;
    view.spotlightStepX = 0;
    view.spotlightStepY = 0;
    this->acts.CloseAll();
    this->archive.CloseAll();
    this->audio.StopAllSounds();
    this->audioDevice.Serve();
    const std::vector<u8>& data = this->databases.mapsData;
    u16 unit = MapsRecord::TableEnd;

    for (u32 at = MapsTable(data, MapsDataTable::MapSettings); MapsWord(data, at) != MapsRecord::TableEnd;
         at += MapsRecord::SettingsBytes)
    {
        if (MapsWord(data, at) != static_cast<u16>(map))
        {
            continue;
        }

        unit = MapsWord(data, at + MapsRecord::SettingsUnitAt);
        const u8 kind = data[at + MapsRecord::SettingsKindAt];
        const u8 effects = data[at + MapsRecord::SettingsEffectsAt];
        const u16 speeds = MapsWord(data, at + MapsRecord::SettingsSpeedsAt);
        view.weatherKind = kind & kNibbleMask;
        view.mapWalkSpeed = data[at + MapsRecord::SettingsSpeedsAt] & kNibbleMask;
        view.layerFrames = MapsWord(data, at + MapsRecord::SettingsUnk49E0BCAt);
        view.encounterGroup = MapsWord(data, at + MapsRecord::SettingsUnk4B896CAt);
        view.mapStory = MapsWord(data, at + MapsRecord::SettingsUnk4B88C0At);
        view.townWarpAllowed = (kind & kNoUnk4ACDB4) == 0 ? 1 : 0;
        game.ClearGameFlag(GameState::OverlaysFirst);

        if ((effects & kOverlaysFirst) != 0)
        {
            game.SetGameFlag(GameState::OverlaysFirst);
        }

        view.weatherAction = Scene::WeatherFirstAction;
        view.weatherActions = Scene::GlowWeatherActions;
        game.ClearGameFlag(GameState::Weather);

        if ((effects & kGlowWeather) != 0)
        {
            game.SetGameFlag(GameState::Weather);
        }

        if ((effects & kShadeWeather) != 0)
        {
            game.SetGameFlag(GameState::Weather);
            view.weatherAction = Scene::WeatherFirstAction;
            view.weatherActions = Scene::ShadeWeatherActions;
        }

        constexpr std::array<std::pair<u8, int>, 5> kEffectFlags = {{
            {kLeaves, GameState::Leaves},
            {kHeatHaze, GameState::HeatHaze},
            {kLightning, GameState::Lightning},
            {kSpotlight, GameState::Spotlight},
            {kFireflies, GameState::Fireflies},
        }};

        for (const auto& [bit, flag] : kEffectFlags)
        {
            game.ClearGameFlag(flag);

            if ((effects & bit) != 0)
            {
                game.SetGameFlag(flag);
            }
        }

        view.cellTintRed = Game::SpeedNibbles[speeds >> (3 * kNibbleBits)];
        view.cellTintGreen = Game::SpeedNibbles[(speeds >> (2 * kNibbleBits)) & kNibbleMask];
        view.cellTintBlue = Game::SpeedNibbles[data[at + MapsRecord::SettingsSpeedsAt] >> kNibbleBits];

        if (!std::has_single_bit(static_cast<u32>(view.mapWalkSpeed)) ||
            static_cast<u32>(view.mapWalkSpeed) > kMaxWalkSpeed)
        {
            view.mapWalkSpeed = kDefaultWalkSpeed;
        }

        if (view.layerFrames < 1)
        {
            view.layerFrames = 1;
        }

        if (!view.encounterGroups.empty() &&
            static_cast<i16>(view.encounterGroup) > static_cast<i16>(view.encounterGroups.front()))
        {
            DebugLog(u8"載入地圖 錯誤的戰鬥機率組[%d]", view.encounterGroup);
            view.encounterGroup = 1;
        }

        break;
    }

    this->MusicZone(map);

    if (unit == MapsRecord::TableEnd)
    {
        DebugLog(u8"遊戲初始 找不到設定的起始地圖碼 (%d)", map);
        this->Destroy();
        return;
    }

    this->ShowLoading(kLoadingSettings);
    this->AudioPump();
    this->currentMap = map;
    view.layerDirection = 1;
    game.SetGameFlag(map + GameState::MapEntered);

    if ((options & Game::SaveRoles) != 0)
    {
        this->SaveRoleStates(static_cast<i16>(x), static_cast<i16>(y));
    }

    view.encounterSteps = 0;
    view.soundRise = {};
    view.triggers.next.reset();
    view.FreeAnimations();
    view.FreeEffects();
    view.FreeSpritesA();
    view.FreeSpritesB();
    game.clickRegions.Free();
    view.FreeNodes();
    view.FreeMarks();
    view.ResetLeaves();
    view.FreeObjects();
    this->controls.buttons = {};
    game.ClearRoles();

    for (MoveRecord& move : game.moves)
    {
        move.Clear();
    }

    view.followerCount = 0;
    view.followers.fill(-1);

    for (MoveRecord& move : view.followerMoves)
    {
        move.Clear();
    }

    this->audioDevice.Serve();
    this->field.overlays.reset();
    this->notices.head.next.reset();
    this->ShowLoading(kLoadingCleared);
    this->AudioPump();
    game.roleCount = 1;
    this->field.palette.clear();

    if (!this->field.LoadMap(unit, this->systemFlags))
    {
        DebugLog(u8"載入地圖 過程發生錯誤");
        this->Destroy();
        return;
    }

    MapHeader& header = game.map;

    if (header.bitsPerPixel == MapHeader::IndexedBits && header.pixels != nullptr)
    {
        this->field.palette.assign(MapHeader::PaletteBytes / sizeof(u16), 0);
        std::memcpy(this->field.palette.data(), header.pixels, MapHeader::PaletteBytes);
        this->painter.ConvertPixels(this->field.palette);
    }

    if (header.pixels == nullptr)
    {
        ErrorLog(u8"LoadCity ,gMap.Pixel==NULL");
    }

    view.layerTick = 1;
    view.layerIndex = 0;
    view.layerOffset = 0;
    view.layerTiles = header.width * header.height;
    std::string name;

    if (!this->MapName(this->currentMap, name))
    {
        DebugLog(u8"讀取地圖名稱 MapName.txt中找不到地圖[%d]名稱", unit);
        name = Big5(Game::UnknownPlace);
    }

    header.name = {};
    std::copy_n(name.begin(), std::min(name.size(), header.name.size() - 1), header.name.begin());
    this->ShowLoading(kLoadingNamed);
    this->AudioPump();

    if (game.GameFlag(GameState::Weather))
    {
        for (std::size_t particle = 0; particle < view.weather.size(); ++particle)
        {
            view.weatherMotion[particle].speedTarget = static_cast<i32>(this->random.Below(kStartSpeeds)) + 1;
            const u32 spread = this->random.Below(view.weatherActions);
            view.weatherShades[particle].tintTarget = 0;
            view.weatherMotion[particle].action = view.weatherAction + static_cast<i32>(spread);
            view.weather[particle].x =
                static_cast<i32>(this->random.Below(static_cast<u32>(MapHeader::TileSize * header.width)));
            view.weather[particle].y =
                static_cast<i32>(this->random.Below(static_cast<u32>(MapHeader::TileSize * header.height)));
            view.weather[particle].stepX = static_cast<i32>(this->random.Below(kStartDrift)) - kStartDrift;
            view.weather[particle].stepY = static_cast<i32>(this->random.Below(kStartDrift)) - kStartDrift;
        }
    }

    view.mapTop = 0;
    view.mapLeft = 0;
    view.mapRight = MapHeader::TileSize * header.width;
    view.mapBottom = MapHeader::TileSize * header.height;
    this->BuildTriggers(view.triggers);
    view.followerCount = 1;
    this->PlaceRoles(map, x, y, act, action, direction, previous);
    this->ShowLoading(kLoadingPlaced);
    this->AudioPump();

    for (int index = 1; index < game.roleCount; ++index)
    {
        this->InitRole(game.roles[static_cast<std::size_t>(index)]);
    }

    this->field.CenterCamera();
    DebugLog(
        u8"\n"
        u8"MapName[%s] id[%d]",
        std::string_view(header.name.data(), header.name.size()),
        this->currentMap
    );
    DebugLog(
        u8"\n"
        u8"TotalRole[%d]",
        game.roleCount
    );
    this->ShowLoading(kLoadingCentered);
    this->ShowLoading(kLoadingDone);
    game.SetFrameInterval(Game::FrameInterval);
}

// The roles of maps.dat's role table on map `map` (part of 0x0040C130): records repeating an earlier role are skipped,
// records of no role and of roles 10000 and 10001 belong to every map; the leader is moved to the entrance and becomes
// the controlled role with its trail; roles past the map's edge are moved inside and left out; the party leader takes
// its act from the story; flagged roles follow the leader unless the story lists them; roles with flag 9 get a mark.
void Game::PlaceRoles(int map, int x, int y, i16 act, i16 action, i16 direction, int previous)
{
    GameState& game = this->state;
    Scene& view = this->scene;
    std::vector<u8>& data = this->databases.mapsData;
    const std::size_t table = view.rolesTable;
    const MapHeader& header = game.map;

    auto word = [&data, table](std::size_t index)
    {
        return MapsWord(data, table + sizeof(u16) * index);
    };

    auto put = [&data, table](std::size_t index, u16 value)
    {
        const std::size_t at = table + sizeof(u16) * index;

        if (at + 1 < data.size())
        {
            data[at] = static_cast<u8>(value);
            data[at + 1] = static_cast<u8>(value >> CHAR_BIT);
        }
    };

    std::size_t record = 0;
    int placed = 0;
    std::size_t mark = 0;

    while (word(record) != MapsRecord::TableEnd)
    {
        for (std::size_t earlier = 0; earlier < record / MapsRecord::RoleWords; ++earlier)
        {
            if (word(record + MapsRecord::RoleId) == word(earlier * MapsRecord::RoleWords + MapsRecord::RoleId))
            {
                DebugLog(u8"讀取路人 重複定義角色(%d)", word(earlier * MapsRecord::RoleWords + MapsRecord::RoleId));
                record += MapsRecord::RoleWords;
            }
        }

        const u16 id = word(record + MapsRecord::RoleId);

        if (id == 0 || id == kEveryMapRole || id == kEveryMapRole2)
        {
            put(record + MapsRecord::RoleMap, static_cast<u16>(map));
        }

        if (static_cast<i32>(id) == this->partyLeaderId)
        {
            this->AudioPump();
            put(record + MapsRecord::RoleMap, static_cast<u16>(map));
            put(record + MapsRecord::RoleAct, static_cast<u16>(act));
            put(record + MapsRecord::RoleAction, static_cast<u16>(action));
            put(record + MapsRecord::RoleDirection, static_cast<u16>(direction));
            put(record + MapsRecord::RoleColumn, static_cast<u16>(x));
            put(record + MapsRecord::RoleRow, static_cast<u16>(y));
            put(record + MapsRecord::RoleEvent, 0);
            put(record + MapsRecord::RolePath, 0);
            put(record + MapsRecord::RoleStep, 0);
            put(record + MapsRecord::RoleFlags, kLeaderFlags);
            game.playerRole = placed + game.roleCount;
            view.followers[0] = placed + game.roleCount;
            view.trailY.fill(MapHeader::TileSize * y);
            view.trailX.fill(MapHeader::TileSize * x);
        }

        if (static_cast<int>(word(record)) != map)
        {
            record += MapsRecord::RoleWords;
            continue;
        }

        const u16 column = word(record + MapsRecord::RoleColumn);
        const u16 row = word(record + MapsRecord::RoleRow);
        const int slot = placed + game.roleCount;

        if (column > static_cast<u32>(header.width) || row > static_cast<u32>(header.height) ||
            slot >= GameState::Roles)
        {
            DebugLog(
                u8"讀取路人 地圖(%d)啟動對話(%d)\n路人(%d)位置超出地圖\n地圖(%d,%d)\n路人(%d,%d)",
                map,
                game.leader.event,
                word(record + MapsRecord::RoleId),
                header.width,
                header.height,
                column,
                row
            );
            put(record + MapsRecord::RoleColumn, static_cast<u16>(header.width - 1));
            put(record + MapsRecord::RoleRow, static_cast<u16>(header.height - 1));
            record += MapsRecord::RoleWords;
            continue;
        }

        Role& role = game.roles[static_cast<std::size_t>(slot)];
        role.id = word(record + MapsRecord::RoleId);
        role.act.id = word(record + MapsRecord::RoleAct);
        role.act.action = word(record + MapsRecord::RoleAction);
        role.act.direction = word(record + MapsRecord::RoleDirection);
        role.act.wait = 0;
        role.act.drawLevel = 0;
        role.act.signal = 0;
        role.x = MapHeader::TileSize * column;
        role.y = MapHeader::TileSize * row;
        role.event = word(record + MapsRecord::RoleEvent);
        role.unk1C = word(record + MapsRecord::RolePath);
        role.unk18 = static_cast<i16>(word(record + MapsRecord::RoleStep));
        role.flags = word(record + MapsRecord::RoleFlags);
        role.talkState = Role::NoTalk;
        role.unk14 = 0;
        role.unk20 = 0;
        this->LoadRoleSound(role);
        this->field.InsertRole(role, static_cast<int>(role.flags & Role::LayerMask));

        if (role.id == kPartyLeader)
        {
            if (previous == Field::DemonRefiningPot && this->currentMap != Field::DemonRefiningPot &&
                view.warpLeaderAct != 0)
            {
                role.act.id = view.warpLeaderAct;
            }

            if ((view.mapStory & kStoryActMark) != 0)
            {
                role.act.id = kLeaderStoryAct;

                if (this->currentMap == Field::TwoRiversPlain || this->currentMap == Field::WesternRegions ||
                    this->currentMap == Field::MediterraneanEastCoast)
                {
                    for (const ItemNode* item = this->party.inventory.get(); item != nullptr; item = item->next.get())
                    {
                        if (item->id == kSpecialItem)
                        {
                            role.act.id = kLeaderSpecialAct;
                            break;
                        }
                    }
                }
            }
        }

        if ((role.flags & Role::Follower) != 0)
        {
            u32 follow = kFollowUnlisted;
            const u32 story = static_cast<u32>(view.mapStory) & ~kStoryActMark;

            for (std::size_t list = MapsTable(data, MapsDataTable::Followers);
                 MapsWord(data, list) != MapsRecord::TableEnd;
                 list += MapsRecord::FollowerBytes)
            {
                if (MapsWord(data, list) == role.id)
                {
                    follow &= ~kFollowUnlisted;

                    for (std::size_t value = 1; value <= MapsRecord::FollowerStories; ++value)
                    {
                        if (story == MapsWord(data, list + sizeof(u16) * value))
                        {
                            follow |= kFollowStory;
                        }
                    }
                }
            }

            if (follow != 0)
            {
                this->FollowLeader(slot);
                role.flags |= Role::Visible;

                if (this->currentMap == Field::DemonRefiningPot || (view.mapStory & kStoryActMark) != 0)
                {
                    role.flags &= ~Role::Visible;
                }
            }
        }

        if ((role.flags & Role::Bubbles) != 0 && mark >= view.marks.size())
        {
            DebugLog(u8"讀取路人 地圖(%d)定義煙囪物件數量超過四個", map);
        }

        if ((role.flags & Role::Bubbles) != 0 && mark < view.marks.size())
        {
            RoleMark& entry = view.marks[mark++];
            entry.role = role.id;
            entry.x = static_cast<u16>(role.x);
            entry.y = static_cast<u16>(role.y);
            entry.unk08 = 0;
            entry.flags = 0;
            entry.unk0E = 0;
        }

        record += MapsRecord::RoleWords;
        ++placed;
    }

    game.roleCount += placed;
}

// swd32.exe 0x0040D3C0: a role written into its record of maps.dat's role table (its act, place in tiles, event, talk,
// step and flags). The original searches past the end of the table unless story flag 2 is set.
void Game::StoreRole(const Role& role)
{
    std::vector<u8>& data = this->databases.mapsData;

    for (std::size_t at = this->scene.rolesTable;
         at + static_cast<std::size_t>(sizeof(u16) * MapsRecord::RoleWords) <= data.size();
         at += static_cast<std::size_t>(sizeof(u16) * MapsRecord::RoleWords))
    {
        if (MapsWord(data, at) == MapsRecord::TableEnd)
        {
            if (this->state.GameFlag(GameState::DebugReports))
            {
                DebugLog(
                    u8"!!! 警告 !!! 角色編號(%d)\n資料歸鍵時\n找不到符合的位置\n可能是定義錯誤或是角色不存在了",
                    role.id
                );
            }

            return;
        }

        if (MapsWord(data, at + sizeof(u16) * MapsRecord::RoleId) != role.id)
        {
            continue;
        }

        auto put = [&data, at](int word, u32 value)
        {
            const std::size_t offset = at + sizeof(u16) * static_cast<std::size_t>(word);
            data[offset] = static_cast<u8>(value);
            data[offset + 1] = static_cast<u8>(value >> CHAR_BIT);
        };

        put(MapsRecord::RoleAct, static_cast<u32>(role.act.id));
        put(MapsRecord::RoleAction, static_cast<u32>(role.act.action));
        put(MapsRecord::RoleDirection, static_cast<u32>(role.act.direction));
        put(MapsRecord::RoleColumn, static_cast<u16>(role.x) >> MapHeader::TileShift);
        put(MapsRecord::RoleRow, static_cast<u16>(role.y) >> MapHeader::TileShift);
        put(MapsRecord::RoleEvent, role.event);
        put(MapsRecord::RolePath, role.unk1C);
        put(MapsRecord::RoleStep, static_cast<u32>(role.unk18));
        put(MapsRecord::RoleFlags, role.flags);
        return;
    }
}

// swd32.exe 0x00485750: sound `sound` from (x, y) of the map when within 512 pixels of the controlled role, quieter and
// panned by the distance
void Game::PlaySoundAt(u16 sound, int x, int y)
{
    const Role& player = this->state.roles[static_cast<std::size_t>(this->state.playerRole)];
    const int squared = (player.x - x) * (player.x - x) + (player.y - y) * (player.y - y);
    const auto distance = static_cast<int>(crt::Ftol(std::sqrt(static_cast<double>(squared))));

    if (distance >= kSoundRange)
    {
        return;
    }

    SoundEffects& effects = this->audio.Effects();
    effects.Play(sound, 0, 0, kPlayOnce);
    effects.SetVolume(
        sound,
        this->config.soundVolume * (kSoundScale - distance * kSoundScale / kSoundRange) / Config::MaxVolume
    );
    effects.SetPan(sound, ((x - player.x) * kPanScale) / kSoundRange);
}

// swd32.exe 0x0040DA60: a random encounter after `steps` steps: the map's encounter set (from maps.dat's table 0x12)
// gives up to four step thresholds with their chances in percent; once one is passed and its roll succeeds (or with
// `force` 1) the encounter area under the controlled role picks one of its battles at random. 0 for none.
u32 Game::RollEncounter(int steps, int force)
{
    Scene& view = this->scene;

    if (view.encounterGroup == 0)
    {
        return 0;
    }

    const Role& player = this->state.roles[static_cast<std::size_t>(this->state.playerRole)];
    const auto roll = static_cast<int>(this->random.Below(kPercent));
    const std::vector<u16>& sets = view.encounterGroups;
    std::size_t at = MapsRecord::GroupsAt + MapsRecord::GroupWords * static_cast<std::size_t>(view.encounterGroup - 1);
    int threshold = 0;
    int pair = kSetPairs;

    auto word = [&sets](std::size_t index) -> i16
    {
        return index < sets.size() ? static_cast<i16>(sets[index]) : i16{0};
    };

    for (;;)
    {
        threshold += word(at);

        if (steps < threshold || force == kForcedEncounter)
        {
            break;
        }

        at += kPairWords;

        if (--pair <= 0)
        {
            return 0;
        }
    }

    if (word(at + 1) <= roll && force != kForcedEncounter)
    {
        return 0;
    }

    const auto column = static_cast<i32>(static_cast<u32>(player.x) >> MapHeader::TileShift);
    const auto row = static_cast<i32>(static_cast<u32>(player.y) >> MapHeader::TileShift);
    const MapTrigger* area = view.triggers.next.get();

    while (area != nullptr && (column < area->left || column > area->right || row < area->top || row > area->bottom))
    {
        area = area->next.get();
    }

    if (area == nullptr)
    {
        return 0;
    }

    const std::vector<u8>& data = this->databases.mapsData;
    const u16 count = MapsWord(data, area->table);
    const u32 pick = this->random.Below(count);
    view.encounterSteps = 0;
    return MapsWord(data, area->table + sizeof(u16) * (pick + 1));
}

// swd32.exe 0x0040F6D0: role `index` let go by an event: while held, each scripted move of it ends: a role between
// tiles leaves its cells and (unless controlled) steps back along the move until it stands on a tile, its stepping
// and fast bits and its act's speed cleared; its path move resumes, its act returns to the action and direction it had
// before turning to the talker, and it is no longer held. The original steps forever where the step cannot reach a
// tile (or reads past the step tables at the end of the path); here that is Hang().
void Game::ReleaseRole(int index)
{
    Role& role = this->state.roles[static_cast<std::size_t>(index)];

    if ((role.flags & Role::Held) == 0)
    {
        return;
    }

    for (MoveRecord& move : this->state.moves)
    {
        if (static_cast<int>(move.role) != index)
        {
            continue;
        }

        if ((role.x & kTileMask) != 0 || (role.y & kTileMask) != 0)
        {
            this->field.ClearRoleCells(role, this->partyLeaderId);

            if (index != this->state.playerRole)
            {
                const u8 direction = move.CurrentDirection();

                if (direction >= RolePath::StepX.size())
                {
                    Hang();
                }

                role.x = RolePath::TileStart(role.x, RolePath::StepX[direction]);
                role.y = RolePath::TileStart(role.y, RolePath::StepY[direction]);
            }
        }

        role.flags &= ~(Role::Stepping | Role::FastMove);
        role.act.speed = 0;
    }

    this->rolePath.ResumeMove(index);

    if (index >= GameState::Roles)
    {
        DebugLog(u8"對話系統(TalkEnd) GUID(%d)\n陣列索引錯誤(crID %d) (manMax %d) ", index, this->state.roleCount);
    }

    if ((role.flags & Role::FacesTalker) != 0)
    {
        if (role.act.savedAction != ActState::NotSaved)
        {
            role.act.action = role.act.savedAction;
        }

        if (role.act.savedDirection != ActState::NotSaved)
        {
            role.act.direction = role.act.savedDirection;
        }
    }

    role.act.savedAction = ActState::NotSaved;
    role.act.savedDirection = ActState::NotSaved;

    if (this->acts.Run(role.act) == 0)
    {
        LogActError(u8"Act Err(Talk:Talkend)", role);
    }

    role.act.savedAction = ActState::NotSaved;
    role.act.savedDirection = ActState::NotSaved;
    role.flags &= ~Role::Held;
    role.act.delay = 0;
}
}  // namespace swd3
