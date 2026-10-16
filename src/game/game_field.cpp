#include <algorithm>
#include <cmath>
#include <cstring>
#include <span>
#include <string>

#include <SDL3/SDL.h>

#include "core/debug_log.h"
#include "core/format.h"
#include "crt/msvc.h"
#include "game/game.h"
#include "platform/system_info.h"

namespace swd3
{

namespace
{

constexpr int kRecordWords = 0xB;
constexpr u16 kTableEnd = 0xFFFF;
constexpr u16 kNameEnd = 0x5125;
constexpr int kLoadingAct = 0x232A;
constexpr int kLoadingAction = 0x4F;
constexpr int kLoadingBarWidth = 0x18A;
constexpr std::size_t kScreenBytes = 0x96000;
constexpr u16 kPartyLeader = 1;
constexpr u16 kSpecialItem = 0x192;
constexpr std::string_view kCdData = "swd3_dvd.dat";  // swd32.exe 0x0049FC9C (the same name for every disc)

// swd32.exe 0x0049FBA8: a nibble as a signed step.
constexpr std::array<i32, 16> kSignedNibble = {0, 1, 2, 3, 4, 5, 6, 7, 8, -7, -6, -5, -4, -3, -2, -1};
// swd32.exe 0x004994AC and 0x004994CC: the step of a move by direction.
constexpr std::array<i32, 8> kStepX = {4, 0, -4, -4, -4, 0, 4, 4};
constexpr std::array<i32, 8> kStepY = {4, 4, 4, 0, -4, -4, -4, 0};

}  // namespace

// swd32.exe 0x0040ED60: the loading screen: a bar of `percent` (its colour shading along it) under the picture of act
// 0x232A, a random runner (1 or 2) at its end; -1 starts it, 100 ends it. Nothing while flag 0x46 is set.
void Game::ShowLoading(int percent)
{
    Scene& view = this->scene;
    int shown = percent;

    if (percent == -1)
    {
        this->loadingPercent = 0;
        this->loadingUnk4CC2C0 = 0;
        shown = 0;
        this->loadingRunner = static_cast<i32>(this->random.Below(2)) + 1;
        this->loadingRed = 0x1F;
        this->loadingGreen = 0x73;
        this->loadingBlue = 0xFF;
        this->loadingUnk4CC2C4 = 0;
        ResetPlayback(view.unk4A93E8);
        ResetPlayback(this->loadingAct);
    }

    if (!this->state.GameFlag(0x46))
    {
        this->drawSurface = this->painter.LockSurface(*this->offscreen);
        this->painter.SetTarget(this->drawSurface);
        this->painter.UnlockSurface(*this->offscreen);
        std::memset(this->drawSurface, 0, kScreenBytes);
        const int length = kLoadingBarWidth * shown / 100;
        const int pitch = this->offscreen->Width();
        int red = 0;
        int green = 0;
        int blue = 0;

        for (int column = 0; column <= length; ++column)
        {
            this->loadingBlue = blue / 0x400 + 0xFF;
            this->loadingGreen = green / 0x400 + 0x73;
            this->loadingRed = red / 0x400 + 0x1F;
            const auto color = static_cast<u16>(this->painter.MakeColor(
                static_cast<u32>(this->loadingRed >> 3),
                static_cast<u32>(this->loadingGreen >> 3),
                static_cast<u32>(this->loadingBlue >> 3)
            ));
            u16* pixel = this->drawSurface + pitch * 0x1BD + column + 0x7B;

            for (int row = 0; row < 0x1E; ++row)
            {
                *pixel = color;
                pixel += pitch;
            }

            red += 0x246;
            green += 0xDA;
            blue -= 0x296;
        }

        this->loadingPercent = shown;
        view.unk4A93E8.id = kLoadingAct;
        view.unk4A93E8.action = kLoadingAction;
        view.unk4A93E8.direction = 0;
        view.unk4A93E8.x = 0;
        view.unk4A93E8.y = 0;
        this->actDrawer.Draw(view.unk4A93E8, 0x78, 0x1BA);
        this->loadingAct.id = this->loadingRunner;
        this->loadingAct.action = 0x10;
        this->loadingAct.direction = 3;
        this->actDrawer.Draw(this->loadingAct, length + 0x6B, 0x1CC);
        this->ShowFrame();
    }

    if (percent == 100)
    {
        this->state.ClearGameFlag(0x46);
    }
}

// swd32.exe 0x0040CF40: the music of map `map` (maps.dat's music table): a new pair of tunes fades the playing one out
// unless the music is held; bits 13..15 of the entry set the cue marks
void Game::MusicZone(int map)
{
    Scene& view = this->scene;
    const std::vector<u8>& data = this->databases.mapsData;
    const u32 lists = MapsTable(data, 2);
    u32 at = MapsDword(data, MapsDword(data, lists + 4));

    for (;; at += 8)
    {
        const u16 entry = MapsWord(data, at);

        if (entry == 0 || at + 1 >= data.size())
        {
            view.unk4B7C74[1] = 0;
            view.unk4B7C74[2] = 0;
            view.unk4ACDBC &= static_cast<i32>(0xFFF3FFFFu);
            return;
        }

        if (entry == static_cast<u16>(map))
        {
            break;
        }
    }

    const i32 first = MapsWord(data, at + 2);
    const i32 second = MapsWord(data, at + 4);

    if (first != view.unk4B7C74[1] || second != view.unk4B7C74[2])
    {
        view.unk4B7C74[1] = first;
        view.unk4B7C74[2] = second;

        if ((view.unk4ACDBC & 0x800000) == 0)
        {
            view.unk4ACDBC = 0;

            if (!this->audio.IsMusicStopped())
            {
                this->audio.SetMusicFade(2, 0xF);
                this->audio.UpdateMusicFade();
            }

            DebugLog("StreamStartOut");
        }

        view.unk4ACDBC &= 0x8FFFFF;
        const u16 marks = MapsWord(data, at + 6);
        view.unk4ACDBC &= static_cast<i32>(0xFFF3FFFFu);

        if ((marks & 0x8000) == 0)
        {
            if ((marks & 0x4000) != 0)
            {
                view.unk4ACDBC |= 0xC0000;
            }

            if ((marks & 0x2000) != 0)
            {
                view.unk4ACDBC |= 0x80000;
            }
        }
    }
}

// swd32.exe 0x0040D060: the sound of a role and its period (in both halves of the timer) from maps.dat's sound table;
// false when it has none
bool Game::LoadRoleSound(Role& role)
{
    const std::vector<u8>& data = this->databases.mapsData;

    for (u32 at = MapsTable(data, 0x15); at + 1 < data.size(); at += 6)
    {
        const u16 id = MapsWord(data, at);

        if (id == 0)
        {
            return false;
        }

        if (id == role.id)
        {
            role.sound = MapsWord(data, at + 2);
            const u32 period = MapsWord(data, at + 4);
            role.soundTimer = period << 16 | period;
            return true;
        }
    }

    return false;
}

// swd32.exe 0x0040D9E0: the encounter areas of the current map (maps.dat's table 7) put in front of the list
void Game::BuildTriggers(MapTrigger& head)
{
    const std::vector<u8>& data = this->databases.mapsData;

    for (u32 at = MapsTable(data, 7); MapsWord(data, at) != kTableEnd; at += 0xE)
    {
        if (MapsWord(data, at) == static_cast<u16>(this->unk4A9A10))
        {
            auto node = std::make_unique<MapTrigger>();
            node->left = MapsWord(data, at + 2);
            node->top = MapsWord(data, at + 4);
            node->right = MapsWord(data, at + 6);
            node->bottom = MapsWord(data, at + 8);
            node->table = MapsDword(data, at + 0xA);
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
    this->acts.Run(role.act);
    role.flags &= 0xDF0FFFFF;
    const auto column = static_cast<u32>(role.x) >> 4;
    const i32 row = role.y >> 4;
    const std::size_t at = column + static_cast<std::size_t>(this->state.map.width) * static_cast<std::size_t>(row);
    std::vector<u32>& blocks = this->state.map.blocks;
    role.cell = at < blocks.size() ? &blocks[at] : nullptr;

    if ((role.flags & 0x100) != 0 && role.cell != nullptr)
    {
        const u32 cell = *role.cell;

        if ((cell & 0x800) != 0)
        {
            role.flags |= 0x20000000;
        }

        if ((cell & 0xF000) != 0)
        {
            role.flags = (role.flags & 0xFF0FFFFF) | (cell & 0xF000) << 8;
        }
    }

    if (role.act.id != 0 && (role.flags & 0x8400) == 0x8000)
    {
        this->field.MarkRoleCells(role, static_cast<u16>(this->unk4B7BC4));
    }
}

// swd32.exe 0x0040EFD0: the name of map `map` (maps.dat's name table, Big5 ending with 0x5125); false when it has none
bool Game::MapName(int map, std::string& name) const
{
    const std::vector<u8>& data = this->databases.mapsData;
    std::size_t at = MapsTable(data, 0x14);
    u16 code = MapsWord(data, at);

    if (code == kTableEnd)
    {
        return false;
    }

    while (code != static_cast<u16>(map))
    {
        if (code != kNameEnd)
        {
            do
            {
                ++at;
            } while (at + 1 < data.size() && MapsWord(data, at) != kNameEnd);
        }

        code = MapsWord(data, at + 2);
        at += 2;

        if (code == kTableEnd || at + 1 >= data.size())
        {
            return false;
        }
    }

    name.clear();

    for (std::size_t character = at + 2; character + 1 < data.size() && MapsWord(data, character) != kNameEnd;
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

    for (std::size_t at = this->scene.unk4ACE40;; at += 2 * kRecordWords)
    {
        if (at + 2 * kRecordWords > data.size() || MapsWord(data, at) == kTableEnd)
        {
            DebugLog("role record missing", id);
            return false;
        }

        if (MapsWord(data, at + 2) == id)
        {
            auto put = [&data, at](int word, i16 value)
            {
                const std::size_t offset = at + 2 * static_cast<std::size_t>(word);
                data[offset] = static_cast<u8>(value);
                data[offset + 1] = static_cast<u8>(static_cast<u16>(value) >> 8);
            };

            auto get = [&data, at](int word)
            {
                return static_cast<i16>(MapsWord(data, at + 2 * static_cast<std::size_t>(word)));
            };

            if (act != -1)
            {
                put(2, act);
            }

            if (action != -1)
            {
                put(3, action);
            }

            if (direction != -1)
            {
                put(4, direction);
            }

            if (x != -1)
            {
                put(5, x);
            }

            if (y != -1)
            {
                put(6, y);
            }

            if (event != -1)
            {
                put(7, event);
            }

            if (talk != -1)
            {
                put(8, talk);
                put(9, 0);
            }

            if (clearBits != -1)
            {
                put(0xA, static_cast<i16>(get(0xA) & clearBits));
            }

            if (setBits != -1)
            {
                put(0xA, static_cast<i16>(get(0xA) | setBits));
            }

            if (map != -1)
            {
                put(0, map);
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

        if ((copy.flags & 0x18000000) != 0 || copy.id == 0xFFFF || index == this->state.playerRole)
        {
            continue;
        }

        if (static_cast<i8>(copy.flags) >= 0)
        {
            if (copy.unk1C != 0)
            {
                const u8* paths = this->databases.pathData;
                const std::size_t pathBytes = this->databases.paths.Size();

                auto pathWord = [paths, pathBytes](std::size_t at)
                {
                    return paths != nullptr && at + 2 <= pathBytes ? static_cast<u16>(paths[at] | paths[at + 1] << 8)
                                                                   : u16{0};
                };

                const u32 path = pathWord(0x200 + 4 * static_cast<std::size_t>(copy.unk1C)) |
                                 static_cast<u32>(pathWord(0x202 + 4 * static_cast<std::size_t>(copy.unk1C))) << 16;
                copy.x &= static_cast<i32>(0xFFFFFFF0u);
                copy.y &= static_cast<i32>(0xFFFFFFF0u);

                if (pathWord(0x200 + path + 2 * static_cast<std::size_t>(static_cast<u32>(copy.unk18))) == 8)
                {
                    for (const MoveRecord& move : this->state.moves)
                    {
                        if (move.role == index)
                        {
                            copy.x = move.x;
                            copy.y = move.y;
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
                static_cast<i16>(this->unk4A9A10)
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

            if ((role.x & 0xF) != 0 || (role.y & 0xF) != 0)
            {
                this->field.ClearRoleCells(role, static_cast<u16>(this->unk4B7BC4));
                const u8 direction =
                    move.directions[static_cast<std::size_t>(move.step & 0x7FFF) % move.directions.size()];

                while ((role.x & 0xF) != 0 && direction < kStepX.size())
                {
                    role.x -= kStepX[direction];
                }

                while ((role.y & 0xF) != 0 && direction < kStepY.size())
                {
                    role.y -= kStepY[direction];
                }

                this->field.RemoveRole(role.id, static_cast<int>(role.flags & 3), (role.y >> 4) - 1, 0);
            }

            this->UpdateRoleRecord(role.id, -1, -1, -1, -1, -1, -1, -1, 0x80, -1, -1);
            move.Clear();
            break;
        }
    }

    Scene& view = this->scene;
    const auto follower = static_cast<std::size_t>(view.unk4BABA0);

    if (follower < view.unk4B7C8C.size())
    {
        view.unk4B7C8C[follower] = index;
        view.followerMoves[follower].Clear();
    }

    role.event = 0;
    role.flags = (role.flags & ~0x4000u) | 0x80;
    ++view.unk4BABA0;
}

// swd32.exe 0x004118B0: the minimal installation looks for its data on the CD (story flag 0x53 picks the disc file);
// Q gives up (and quits). 1 with the drive letter in `drive`, 2 when given up.
int Game::CheckCd(const std::string& root, int& drive)
{
    drive = 0;
    this->systemFlags |= 0x10;
    int drives = 0;

    while (drives < static_cast<int>(this->cdDrives.size()) && this->cdDrives[static_cast<std::size_t>(drives)] != 0xFF)
    {
        ++drives;
    }

    const u32 wait = drives > 1 ? 0x3E8 : 0xBB8;

    for (;;)
    {
        const u32 now = SystemUptime();

        if (now > wait)
        {
            std::string path = WinFormat("%sswd3\\%s", {root, kCdData});
            path[0] = static_cast<char>(this->cdDrives[0]);
            Game::ProbeFile(path);
            drive = static_cast<u8>(path[0]);
            this->systemFlags &= ~0x10u;
            return 1;
        }

        this->input.PollKeyboard();

        if (this->input.IsKeyDown(0x10) != 0)
        {
            break;
        }
    }

    this->systemFlags |= 4;
    return 2;
}

// swd32.exe 0x0040C130: enters map `map` with the leader at (x, y) tiles in act `act`, action and direction. The map's
// settings (table 3: its unit of HUGE.lmf, the story flags of its effects, its speeds), the music, the old map's roles
// written back (option bit 0), every scene list and role cleared, the map loaded (8-bit maps take their palette from
// the unit), its name, the weather, the encounter areas, the roles of maps.dat's role table on this map (the leader
// moved to the entrance, followers, marks), the camera. A map missing from the tables ends the game.
void Game::LoadCity(int map, int x, int y, i16 act, i16 action, i16 direction, int options)
{
    Scene& view = this->scene;
    GameState& game = this->state;
    const int previous = this->unk4A9A10;
    game.ClearFrameInterval();

    if (this->drawSurface != nullptr)
    {
        view.unk4A93E8.id = kLoadingAct;
        view.unk4A93E8.action = kLoadingAction;
        view.unk4A93E8.direction = 0;
        view.unk4A93E8.x = 0;
        view.unk4A93E8.y = 0;
        this->actDrawer.Draw(view.unk4A93E8, 0x78, 0xCA);
        this->ShowLoading(-1);
    }

    view.unk4C9A18 = 0;
    this->AudioPump();
    game.ClearGameFlag(5);
    game.ClearGameFlag(6);

    view.weather.fill({0x0F0F0F0F, 0x0F0F0F0F, 0x0F0F0F0F, 0x0F0F0F0F});

    ResetPlayback(view.unk4ACAF8);
    view.ResetLeaves();
    game.ClearGameFlag(7);
    game.ClearGameFlag(8);
    std::fill_n(view.bolts.begin(), 0x30, Streak{});
    view.lightningShown = 0;
    view.lightningCount = 0;
    game.ClearGameFlag(0x16);
    std::fill_n(view.bolts.begin(), 0x30, Streak{});
    view.firefliesShown = 0;
    view.unk4B9EF8 = 0x140;
    view.unk4A94A8 = 0x140;
    view.fireflyCount = 0;
    view.unk4B9EFC = 0xF0;
    view.unk4CAE80 = 0xF0;
    view.unk4ACE58 = 0;
    view.unk4ACE5C = 0;
    this->acts.CloseAll();
    this->archive.CloseAll();
    this->audio.StopAllSounds();
    this->AudioPump();
    const std::vector<u8>& data = this->databases.mapsData;
    u16 unit = kTableEnd;

    for (u32 at = MapsTable(data, 3); MapsWord(data, at) != kTableEnd; at += 0xE)
    {
        if (MapsWord(data, at) != static_cast<u16>(map))
        {
            continue;
        }

        unit = MapsWord(data, at + 2);
        const u8 kind = data[at + 4];
        const u8 effects = data[at + 5];
        const u16 speeds = MapsWord(data, at + 6);
        view.unk4B7EB4 = kind & 0xF;
        view.unk4C8BD8 = data[at + 6] & 0xF;
        view.unk49E0BC = MapsWord(data, at + 8);
        view.unk4B896C = MapsWord(data, at + 0xA);
        view.unk4B88C0 = MapsWord(data, at + 0xC);
        view.unk4ACDB4 = (kind & 0x10) == 0 ? 1 : 0;
        game.ClearGameFlag(0xF);

        if (static_cast<i8>(effects) < 0)
        {
            game.SetGameFlag(0xF);
        }

        view.unk4ACEF0 = 0x40;
        view.unk4C97F6 = 4;
        game.ClearGameFlag(5);

        if ((effects & 0x40) != 0)
        {
            game.SetGameFlag(5);
        }

        if ((effects & 0x20) != 0)
        {
            game.SetGameFlag(5);
            view.unk4ACEF0 = 0x40;
            view.unk4C97F6 = 8;
        }

        constexpr std::array<std::pair<u8, int>, 5> kEffectFlags = {{{0x10, 6}, {8, 7}, {4, 8}, {2, 0x13}, {1, 0x16}}};

        for (const auto& [bit, flag] : kEffectFlags)
        {
            game.ClearGameFlag(flag);

            if ((effects & bit) != 0)
            {
                game.SetGameFlag(flag);
            }
        }

        view.unk4AB97C = kSignedNibble[speeds >> 0xC];
        view.unk4AB994 = kSignedNibble[(speeds >> 8) & 0xF];
        view.unk4ACB98 = kSignedNibble[data[at + 6] >> 4];

        if (view.unk4C8BD8 != 1 && view.unk4C8BD8 != 2 && view.unk4C8BD8 != 4 && view.unk4C8BD8 != 8 &&
            view.unk4C8BD8 != 0x10)
        {
            view.unk4C8BD8 = 4;
        }

        if (view.unk49E0BC < 1)
        {
            view.unk49E0BC = 1;
        }

        if (!view.unk4CAE78.empty() && static_cast<i16>(view.unk4B896C) > static_cast<i16>(view.unk4CAE78[0]))
        {
            view.unk4B896C = 1;
        }

        break;
    }

    if (this->config.installType == 1)
    {
        int drive = 0;

        if (this->CheckCd(this->config.cdRoot, drive) == 2)
        {
            return;
        }

        if (!this->config.cdRoot.empty())
        {
            this->config.cdRoot[0] = static_cast<char>(drive);
        }
    }

    this->MusicZone(map);

    if (unit == kTableEnd)
    {
        this->Destroy();
        return;
    }

    this->ShowLoading(2);
    this->AudioPump();
    this->unk4A9A10 = map;
    view.unk49E0C0 = 1;
    game.SetGameFlag(map + 0x1388);

    if ((options & 1) == 1)
    {
        this->SaveRoleStates(static_cast<i16>(x), static_cast<i16>(y));
    }

    view.unk4C9A20 = 0;
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

    view.unk4BABA0 = 0;
    view.unk4B7C8C.fill(-1);

    for (MoveRecord& move : view.followerMoves)
    {
        move.Clear();
    }

    this->AudioPump();
    this->field.overlays.reset();
    this->notices.head.next.reset();
    this->ShowLoading(0xA);
    this->AudioPump();
    game.roleCount = 1;
    this->field.palette.clear();

    if (!this->field.LoadMap(unit, this->systemFlags))
    {
        this->Destroy();
        return;
    }

    MapHeader& header = game.map;

    if (header.bitsPerPixel == 8 && header.pixels != nullptr)
    {
        this->field.palette.assign(0x100, 0);
        std::memcpy(this->field.palette.data(), header.pixels, 0x200);
        this->painter.ConvertPixels(this->field.palette);
    }

    view.unk4CC2DC = 1;
    view.unk4B7944 = 0;
    view.unk4B873C = 0;
    view.unk4B7A98 = header.width * header.height;
    std::string name;

    if (!this->MapName(this->unk4A9A10, name))
    {
        name = Big5(Game::kUnknownPlace);
    }

    header.name = {};
    std::copy_n(name.begin(), std::min(name.size(), header.name.size() - 1), header.name.begin());
    this->ShowLoading(0x55);
    this->AudioPump();

    if (game.GameFlag(5))
    {
        for (std::size_t particle = 0; particle < 4; ++particle)
        {
            view.weatherMotion[particle].speedTarget = static_cast<i32>(this->random.Below(3)) + 1;
            const u32 spread = this->random.Below(view.unk4C97F6);
            view.weatherShades[particle].tintTarget = 0;
            view.weatherMotion[particle].action = view.unk4ACEF0 + static_cast<i32>(spread);
            view.weather[particle].x = static_cast<i32>(this->random.Below(static_cast<u32>(0x10 * header.width)));
            view.weather[particle].y = static_cast<i32>(this->random.Below(static_cast<u32>(0x10 * header.height)));
            view.weather[particle].stepX = static_cast<i32>(this->random.Below(2)) - 2;
            view.weather[particle].stepY = static_cast<i32>(this->random.Below(2)) - 2;
        }
    }

    view.unk4ACE4C = 0;
    view.unk4ACE48 = 0;
    view.unk4ACE50 = 0x10 * header.width;
    view.unk4ACE54 = 0x10 * header.height;
    this->BuildTriggers(view.triggers);
    view.unk4BABA0 = 1;
    this->PlaceRoles(map, x, y, act, action, direction, previous);
    this->ShowLoading(0x5A);
    this->AudioPump();

    for (int index = 1; index < game.roleCount; ++index)
    {
        this->InitRole(game.roles[static_cast<std::size_t>(index)]);
    }

    this->field.CenterCamera();
    DebugLog(
        "\n"
        "MapName[%s] id[%d]",
        std::string_view(header.name.data(), header.name.size()),
        this->unk4A9A10
    );
    DebugLog(
        "\n"
        "TotalRole[%d]",
        game.roleCount
    );
    this->ShowLoading(0x5F);
    this->ShowLoading(100);
    game.SetFrameInterval(Game::kFrameInterval);
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
    const std::size_t table = view.unk4ACE40;
    const MapHeader& header = game.map;

    auto word = [&data, table](std::size_t index)
    {
        return MapsWord(data, table + 2 * index);
    };

    auto put = [&data, table](std::size_t index, u16 value)
    {
        const std::size_t at = table + 2 * index;

        if (at + 1 < data.size())
        {
            data[at] = static_cast<u8>(value);
            data[at + 1] = static_cast<u8>(value >> 8);
        }
    };

    std::size_t record = 0;
    int placed = 0;
    std::size_t mark = 0;

    while (word(record) != kTableEnd)
    {
        for (std::size_t earlier = 0; earlier < record / kRecordWords; ++earlier)
        {
            if (word(record + 1) == word(earlier * kRecordWords + 1))
            {
                record += kRecordWords;
            }
        }

        const u16 id = word(record + 1);

        if (id == 0 || id == 0x2710 || id == 0x2711)
        {
            put(record, static_cast<u16>(map));
        }

        if (id == static_cast<u16>(this->unk4B7BC4))
        {
            this->AudioPump();
            put(record, static_cast<u16>(map));
            put(record + 2, static_cast<u16>(act));
            put(record + 3, static_cast<u16>(action));
            put(record + 4, static_cast<u16>(direction));
            put(record + 5, static_cast<u16>(x));
            put(record + 6, static_cast<u16>(y));
            put(record + 7, 0);
            put(record + 8, 0);
            put(record + 9, 0);
            put(record + 0xA, 0xD100);
            game.playerRole = placed + game.roleCount;
            view.unk4B7C8C[0] = placed + game.roleCount;
            view.trailY.fill(0x10 * y);
            view.trailX.fill(0x10 * x);
        }

        if (word(record) != static_cast<u16>(map))
        {
            record += kRecordWords;
            continue;
        }

        const u16 column = word(record + 5);
        const u16 row = word(record + 6);
        const int slot = placed + game.roleCount;

        if (column > static_cast<u32>(header.width) || row > static_cast<u32>(header.height) ||
            slot >= GameState::kRoles)
        {
            put(record + 5, static_cast<u16>(header.width - 1));
            put(record + 6, static_cast<u16>(header.height - 1));
            record += kRecordWords;
            continue;
        }

        Role& role = game.roles[static_cast<std::size_t>(slot)];
        role.id = word(record + 1);
        role.act.id = word(record + 2);
        role.act.action = word(record + 3);
        role.act.direction = word(record + 4);
        role.act.wait = 0;
        role.act.drawLevel = 0;
        role.act.signal = 0;
        role.x = 0x10 * column;
        role.y = 0x10 * row;
        role.event = word(record + 7);
        role.unk1C = word(record + 8);
        role.unk18 = static_cast<i16>(word(record + 9));
        role.flags = word(record + 0xA);
        role.unk26 = 0;
        role.unk14 = 0;
        role.unk20 = 0;
        this->LoadRoleSound(role);
        this->field.InsertRole(role, static_cast<int>(role.flags & 3));

        if (role.id == kPartyLeader)
        {
            if (previous == 0x16 && this->unk4A9A10 != 0x16 && view.unk4C8BE0 != 0)
            {
                role.act.id = view.unk4C8BE0;
            }

            if ((view.unk4B88C0 & 0x8000) != 0)
            {
                role.act.id = 0x60;

                if (this->unk4A9A10 == 6 || this->unk4A9A10 == 8 || this->unk4A9A10 == 0xC8)
                {
                    for (const ItemNode* item = this->party.inventory.get(); item != nullptr; item = item->next.get())
                    {
                        if (item->id == kSpecialItem)
                        {
                            role.act.id = 0x5F;
                            break;
                        }
                    }
                }
            }
        }

        if (static_cast<i8>(role.flags) < 0)
        {
            u32 follow = 1;
            const u32 story = static_cast<u32>(view.unk4B88C0) & 0x7FFF;

            for (std::size_t list = MapsTable(data, 0x19); MapsWord(data, list) != kTableEnd; list += 0x12)
            {
                if (MapsWord(data, list) == role.id)
                {
                    follow &= ~1u;

                    for (std::size_t value = 1; value <= 8; ++value)
                    {
                        if (story == MapsWord(data, list + 2 * value))
                        {
                            follow |= 2;
                        }
                    }
                }
            }

            if (follow != 0)
            {
                this->FollowLeader(slot);
                role.flags |= 0x8000;

                if (this->unk4A9A10 == 0x16 || (view.unk4B88C0 & 0x8000) != 0)
                {
                    role.flags &= ~0x8000u;
                }
            }
        }

        if ((role.flags & 0x200) != 0 && mark < view.marks.size())
        {
            RoleMark& entry = view.marks[mark++];
            entry.role = role.id;
            entry.x = static_cast<u16>(role.x);
            entry.y = static_cast<u16>(role.y);
            entry.unk08 = 0;
            entry.flags = 0;
            entry.unk0E = 0;
        }

        record += kRecordWords;
        ++placed;
    }

    game.roleCount += placed;
}

// swd32.exe 0x0040D3C0: a role written into its record of maps.dat's role table (its act, place in tiles, event, talk,
// step and flags). The original searches past the end of the table unless story flag 2 is set.
void Game::StoreRole(const Role& role)
{
    std::vector<u8>& data = this->databases.mapsData;

    for (std::size_t at = this->scene.unk4ACE40; at + 2 * kRecordWords <= data.size(); at += 2 * kRecordWords)
    {
        if (MapsWord(data, at) == kTableEnd)
        {
            return;
        }

        if (MapsWord(data, at + 2) != role.id)
        {
            continue;
        }

        auto put = [&data, at](int word, u32 value)
        {
            const std::size_t offset = at + 2 * static_cast<std::size_t>(word);
            data[offset] = static_cast<u8>(value);
            data[offset + 1] = static_cast<u8>(value >> 8);
        };

        put(2, static_cast<u32>(role.act.id));
        put(3, static_cast<u32>(role.act.action));
        put(4, static_cast<u32>(role.act.direction));
        put(5, static_cast<u16>(role.x) >> 4);
        put(6, static_cast<u16>(role.y) >> 4);
        put(7, role.event);
        put(8, role.unk1C);
        put(9, static_cast<u32>(role.unk18));
        put(0xA, role.flags);
        return;
    }
}

// swd32.exe 0x00412050: whether a file can be opened (the system's message is formatted on failure, never shown)
bool Game::ProbeFile(const std::string& path)
{
    File file;
    const bool opened = file.Open(path, File::Disposition::OpenExisting, File::Access::Read);
    file.Close();
    return opened;
}

// swd32.exe 0x00485750: sound `sound` from (x, y) of the map when within 512 pixels of the controlled role, quieter and
// panned by the distance
void Game::PlaySoundAt(u16 sound, int x, int y)
{
    const Role& player = this->state.roles[static_cast<std::size_t>(this->state.playerRole)];
    const int squared = (player.x - x) * (player.x - x) + (player.y - y) * (player.y - y);
    const auto distance = static_cast<int>(crt::Ftol(std::sqrt(static_cast<double>(squared))));

    if (distance >= 0x200)
    {
        return;
    }

    SoundEffects& effects = this->audio.Effects();
    effects.Play(sound, 0, 0, 1);
    effects.SetVolume(sound, this->config.soundVolume * (0x80 - (distance << 7) / 0x200) / 0xB);
    effects.SetPan(sound, ((x - player.x) << 6) / 0x200);
}

// swd32.exe 0x0040DA60: a random encounter after `steps` steps: the map's encounter set (from maps.dat's table 0x12)
// gives up to four step thresholds with their chances in percent; once one is passed and its roll succeeds (or with
// `force` 1) the encounter area under the controlled role picks one of its battles at random. 0 for none.
u32 Game::RollEncounter(int steps, int force)
{
    Scene& view = this->scene;

    if (view.unk4B896C == 0)
    {
        return 0;
    }

    const Role& player = this->state.roles[static_cast<std::size_t>(this->state.playerRole)];
    const auto roll = static_cast<int>(this->random.Below(0x64));
    const std::vector<u16>& sets = view.unk4CAE78;
    std::size_t at = 1 + 8 * static_cast<std::size_t>(view.unk4B896C - 1);
    int threshold = 0;
    int pair = 4;

    auto word = [&sets](std::size_t index) -> i16
    {
        return index < sets.size() ? static_cast<i16>(sets[index]) : i16{0};
    };

    for (;;)
    {
        threshold += word(at);

        if (steps < threshold || force == 1)
        {
            break;
        }

        at += 2;

        if (--pair <= 0)
        {
            return 0;
        }
    }

    if (word(at + 1) <= roll && force != 1)
    {
        return 0;
    }

    const int column = player.x >> 4;
    const int row = player.y >> 4;
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
    view.unk4C9A20 = 0;
    return MapsWord(data, area->table + 2 * (pick + 1));
}

}  // namespace swd3
