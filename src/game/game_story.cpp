#include <cstring>
#include <span>
#include <string>

#include "core/debug_log.h"
#include "game/game.h"

namespace swd3
{

// swd32.exe 0x0040D160: the camera around (x, y): 320 x 240 pixels before it, held inside the map
void Field::CameraAround(int x, int y, std::array<i32, 4>& rect) const
{
    rect[0] = x - 0x140;

    if (x - 0x140 < 0)
    {
        rect[0] = 0;
    }

    rect[1] = y - 0xF0;

    if (y - 0xF0 < 0)
    {
        rect[1] = 0;
    }

    rect[2] = rect[0] + 0x280;

    if (rect[2] >= 0x10 * this->state.map.width)
    {
        rect[0] = 0x10 * (this->state.map.width - 0x28);
        rect[2] = 0x10 * this->state.map.width;
    }

    rect[3] = rect[1] + 0x1E0;

    if (rect[3] >= 0x10 * this->state.map.height)
    {
        rect[1] = 0x10 * (this->state.map.height - 0x1E);
        rect[3] = 0x10 * this->state.map.height;
    }
}

// swd32.exe 0x0040D560: role `id` loaded from its record of maps.dat's role table (act, place in tiles, event, path,
// flags); its map is returned, 0xFFFFFFFF when the table lacks it
u32 Game::LoadRoleRecord(Role& role, u16 id)
{
    const std::vector<u8>& data = this->databases.mapsData;

    for (std::size_t at = this->scene.unk4ACE40; MapsWord(data, at) != 0xFFFF; at += 0x16)
    {
        if (MapsWord(data, at + 2) != id)
        {
            continue;
        }

        role.id = id;
        role.act.id = MapsWord(data, at + 4);
        role.act.action = MapsWord(data, at + 6);
        role.act.direction = MapsWord(data, at + 8);
        role.x = 0x10 * MapsWord(data, at + 0xA);
        role.y = 0x10 * MapsWord(data, at + 0xC);
        role.event = MapsWord(data, at + 0xE);
        role.unk1C = MapsWord(data, at + 0x10);
        role.unk18 = 0;
        role.flags = MapsWord(data, at + 0x14);
        return MapsWord(data, at);
    }

    DebugLog(u8"role record missing", id);
    return 0xFFFFFFFF;
}

// swd32.exe 0x0040D790: role `id` leaves the followers: a role between tiles is moved on to the next one along its
// last step, its follow move freed; it takes the act, path, event and flags given (its record too), takes its cells,
// and the followers after it move up. A role not on the map only loses the follow flag of its record.
void Game::LeaveParty(u16 id, u16 path, u16 event, i32 act, i32 action, i32 direction, u16 flags)
{
    GameState& game = this->state;
    Scene& view = this->scene;
    u32 index = 0;

    if (!game.FindRoleOrPlayer(id, index))
    {
        std::vector<u8>& data = this->databases.mapsData;

        for (std::size_t at = view.unk4ACE40; MapsWord(data, at) != 0xFFFF; at += 0x16)
        {
            if (MapsWord(data, at + 2) == id)
            {
                if (at + 0x15 < data.size())
                {
                    data[at + 0x14] &= static_cast<u8>(~0x80);
                }

                return;
            }
        }

        DebugLog(u8"LeaveParty", id);
        return;
    }

    std::size_t follower = 0;

    while (follower < view.unk4B7C8C.size() && view.unk4B7C8C[follower] != static_cast<i32>(index))
    {
        ++follower;
    }

    if (follower == view.unk4B7C8C.size() || index >= GameState::Roles)
    {
        DebugLog(u8"LeaveParty", id);
        return;
    }

    Role& role = game.roles[index];
    MoveRecord& move = view.followerMoves[follower];
    const auto leader = static_cast<u16>(this->unk4B7BC4);

    if (move.step < MoveRecord::StepMask && ((role.x & 0xF) != 0 || (role.y & 0xF) != 0))
    {
        this->field.ClearRoleCells(role, leader);
        const u8 last = move.CurrentDirection();

        if (last != 0xFF && last < 8)
        {
            while ((role.x & 0xF) != 0)
            {
                role.x += RolePath::StepX[last];
            }

            while ((role.y & 0xF) != 0)
            {
                role.y += RolePath::StepY[last];
            }
        }

        this->field.RemoveRole(role.id, static_cast<int>(role.flags & Role::LayerMask), (role.y >> 4) - 1, 0);
    }

    move.Clear();
    role.act.id = act;
    role.act.action = action;
    role.act.direction = direction;
    role.event = event;
    role.unk1C = path;
    role.unk18 = 0;
    role.flags = flags;
    this->UpdateRoleRecord(
        role.id,
        -1,
        -1,
        -1,
        -1,
        -1,
        static_cast<i16>(event),
        static_cast<i16>(path),
        0,
        static_cast<i16>(0xFF7F),
        -1
    );
    this->field.MarkRoleCells(role, leader);
    std::move(
        view.unk4B7C8C.begin() + static_cast<std::ptrdiff_t>(follower) + 1,
        view.unk4B7C8C.end(),
        view.unk4B7C8C.begin() + static_cast<std::ptrdiff_t>(follower)
    );
    std::move(
        view.followerMoves.begin() + static_cast<std::ptrdiff_t>(follower) + 1,
        view.followerMoves.end(),
        view.followerMoves.begin() + static_cast<std::ptrdiff_t>(follower)
    );
    --view.unk4BABA0;
}

// swd32.exe 0x0042E790: a scripted change of map: the keys and steps stop, the map frame is hidden while LoadCity
// runs (Talk::Keep keeps the controlled role's act, action or direction)
bool Game::ChangeMap(int map, int x, int y, int act, int action, int direction, int options)
{
    const Role& player = this->state.roles[static_cast<std::size_t>(this->state.playerRole)];
    this->unk4B7920 = 0;
    this->controls.buttons = {};
    this->unk4B7518 = 0;
    this->unk4A948C = 0;
    this->unk4A9488 = 0;
    this->scene.unk4C9A18 |= 1;
    const i32 newAct = act == Talk::Keep ? player.act.id : act;
    const i32 newAction = action == Talk::Keep ? player.act.action : action;
    const i32 newDirection = direction == Talk::Keep ? player.act.direction : direction;
    this->LoadCity(
        map,
        x,
        y,
        static_cast<i16>(newAct),
        static_cast<i16>(newAction),
        static_cast<i16>(newDirection),
        options
    );
    this->scene.unk4C9A18 &= ~1;
    return true;
}

// swd32.exe 0x00430B60: a timed event in frames (30 a second): the second timer (`which` nonzero, flag 0x4A) or the
// first with its event (flags 0x10 and 0x12)
void Game::StartTimer(int minutes, int seconds, int event, int which)
{
    Scene& view = this->scene;

    if (which != 0)
    {
        view.unk4BAB78 = 0;
        view.unk4BAB7C = 0;
        view.unk4A9918 = 0x1E * (seconds + 0x3C * minutes);
        this->state.SetGameFlag(GameState::SecondTimer);
        return;
    }

    view.unk4C97E8 = 0;
    view.unk4C97EC = 0;
    view.unk4CAD20 = 0x1E * (seconds + 0x3C * minutes);
    view.unk4A93D4 = event;
    this->state.SetGameFlag(GameState::Timer);
    this->state.SetGameFlag(GameState::TimerEvent);
}

// swd32.exe 0x00484730: the movie named in the script at `cursor` (up to "%Q", the event offset `pc` moving along;
// .avi and .mpg become .bik) from the video folder, at the music volume; with no CD where one is needed the game
// ends. With no drive letter the name is written over the start of the folder path, its rest kept.
bool Game::PlayVideo(std::size_t& cursor, u16& pc)
{
    const std::span<const u8> script(this->databases.script);
    int drive = 0;

    if (this->CheckCd(this->config.cdRoot, drive) == 2)
    {
        this->systemFlags |= 4;
        return false;
    }

    std::string path = this->config.installDirectory + "video\\";
    std::string name;

    auto word = [&script](std::size_t at)
    {
        return at + 1 < script.size() ? static_cast<u16>(script[at] | script[at + 1] << 8) : u16{0};
    };

    pc = static_cast<u16>(pc + 2);
    cursor += 2;

    while (word(cursor) != 0x5125 && cursor < script.size())
    {
        ++pc;
        name.push_back(static_cast<char>(script[cursor]));
        ++cursor;
    }

    pc = static_cast<u16>(pc + 2);
    cursor += 2;

    if (static_cast<u8>(drive) != 0)
    {
        path += name;
    }
    else
    {
        path.replace(0, std::min(name.size(), path.size()), name);
    }

    for (const std::string_view extension : {std::string_view(".avi"), std::string_view(".mpg")})
    {
        const std::size_t found = path.find(extension);

        if (found != std::string::npos)
        {
            path.replace(found + 1, 3, "bik");
        }
    }

    this->movie.Close();
    this->movie.Open(path, (this->config.musicVolume << 0xF) / 0xB);
    this->systemFlags |= 0x20;
    return true;
}

// swd32.exe 0x00484DA0: the frame of the playing movie (-1 without one)
int Game::VideoPosition() const
{
    return this->movie.Position();
}

// swd32.exe 0x00484920: the movie closed and its flag cleared
void Game::StopVideo()
{
    this->movie.Close();
    this->systemFlags &= ~0x20u;
}

// swd32.exe 0x00484950: the next frame of the movie (unless it waits for its time) centred on the frame drawn and
// shown; after its last frame the movie stops. (The original reports every failed blit of the frame in a message box;
// the emulated blit cannot fail.) A movie that did not open stops at once.
void Game::VideoFrame()
{
    if (!this->movie.IsOpen())
    {
        this->StopVideo();
        return;
    }

    if (this->movie.Waiting() || this->offscreen == nullptr)
    {
        return;
    }

    const int y = (0x1E0 - this->movie.Height()) / 2;
    const int x = (0x280 - this->movie.Width()) / 2;
    const int left = this->movie.Frame(*this->offscreen, x, y, this->errors);
    this->ShowFrame();

    if (left <= 0)
    {
        this->StopVideo();
    }
}

}  // namespace swd3
