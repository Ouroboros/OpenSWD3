#include <algorithm>
#include <cmath>
#include <cstring>

#include "core/debug_log.h"
#include "core/fault.h"
#include "core/format.h"
#include "game/direction.h"
#include "game/game.h"
#include "platform/display.h"
#include "platform/system_info.h"
#include "talk/script_text.h"
#include "talk/talk.h"
#include "talk/talk_event.h"

namespace swd3
{

namespace
{

constexpr u16 kListEnd = 0xFF00;
constexpr u16 kPlaceAtTarget = 0x8000;  // in the y of HeadIn: the head is placed at its target
constexpr u16 kEvenPixels = 0xFFFE;     // the line effects keep to even pixels

// The nodes of a list for which `match` holds are freed (`first`: only the first of them).
template <typename Node, typename Match>
bool EraseNodes(std::unique_ptr<Node>& head, Match match, bool first)
{
    bool erased = false;
    std::unique_ptr<Node>* link = &head;

    while (*link != nullptr)
    {
        if (match(**link))
        {
            std::unique_ptr<Node> gone = std::move(*link);
            *link = std::move(gone->next);
            erased = true;

            if (first)
            {
                return true;
            }

            continue;
        }

        link = &(*link)->next;
    }

    return erased;
}

}  // namespace

// Commands 0x32, 0x46 and 0x49: the camera pans by tiles (0x32), to a tile (0x46) or to a role (0x49) by steps of
// whole pixels that must divide the distance (otherwise 4), kept inside the map.
bool Talk::PanCamera(EventRun& run)
{
    Game& owner = this->game;
    GameState& state = owner.state;
    Scene& view = owner.scene;
    const Field& field = owner.field;
    i32 across = view.unk4A99F0;
    i32 down = view.unk4A98C0;
    std::size_t steps = 3;

    if ((across | down) != 0)
    {
        DebugLog(u8"pan while panning", across, down);
    }

    if (run.code == EventCommand::PanCamera)
    {
        across = static_cast<i16>(this->Word(run, 1));
        view.unk4A99F0 = across;
        down = static_cast<i16>(this->Word(run, 2));
        view.unk4A98C0 = down;
    }
    else if (run.code == EventCommand::PanCameraTo)
    {
        run.valueA = static_cast<i16>(this->Word(run, 1));
        across = run.valueA - (field.cameraLeft >> 4);
        view.unk4A99F0 = across;
        down = static_cast<i16>(this->Word(run, 2)) - (field.cameraTop >> 4);
        view.unk4A98C0 = down;
    }
    else
    {
        state.FindRoleOrPlayer(this->Word(run, 1), run.found);
        const Role* role = this->FoundRole(run);
        std::array<i32, 4> rect = {field.cameraLeft, field.cameraTop, field.cameraRight, field.cameraBottom};
        field.CameraAround(role != nullptr ? role->x : 0, role != nullptr ? role->y : 0, rect);
        across = (rect[0] - field.cameraLeft) >> 4;
        down = (rect[1] - field.cameraTop) >> 4;
        view.unk4A99F0 = across;
        view.unk4A98C0 = down;
        steps = 2;
    }

    run.valueB = static_cast<i32>(steps);
    i32 stepX = this->Word(run, steps);
    i32 stepY = this->Word(run, steps + 1);
    view.unk4A9480 = stepX;
    view.unk4A93D8 = stepY;

    if (across == 0)
    {
        stepX = 0;
        view.unk4A9480 = 0;
    }

    if (down == 0)
    {
        stepY = 0;
        view.unk4A93D8 = 0;
    }

    i32 pixelsDown = 0x10 * down;
    view.unk4A99F0 = 0x10 * across;
    view.unk4A98C0 = pixelsDown;

    if (view.unk4A99F0 != 0 && stepX == 0)
    {
        Fault(owner.errors, u8"Integer divide by zero (0x00427920 pan)");
    }

    if (view.unk4A99F0 != 0 && view.unk4A99F0 % stepX != 0)
    {
        DebugLog(u8"pan step across", stepX);
        pixelsDown = view.unk4A98C0;
        stepY = view.unk4A93D8;
        stepX = 4;
        view.unk4A9480 = 4;
    }

    if (pixelsDown != 0 && stepY == 0)
    {
        Fault(owner.errors, u8"Integer divide by zero (0x00427920 pan)");
    }

    if (pixelsDown != 0 && pixelsDown % stepY != 0)
    {
        DebugLog(u8"pan step down", stepY);
        pixelsDown = view.unk4A98C0;
        stepX = view.unk4A9480;
        stepY = 4;
        view.unk4A93D8 = 4;
    }

    i32 pixelsAcross = view.unk4A99F0;

    if (view.unk4A99F0 < 0)
    {
        view.unk4A9480 = -stepX;
    }

    if (pixelsDown < 0)
    {
        view.unk4A93D8 = -stepY;
    }

    bool clamped = false;

    if (view.unk4A99F0 + field.cameraLeft < 0)
    {
        clamped = true;
        pixelsAcross = -field.cameraLeft;
        view.unk4A99F0 = -field.cameraLeft;
    }

    if (field.cameraRight + pixelsAcross > 0x10 * state.map.width)
    {
        clamped = true;
        view.unk4A99F0 = 0x10 * state.map.width - field.cameraRight;
    }

    if (pixelsDown + field.cameraTop < 0)
    {
        clamped = true;
        pixelsDown = -field.cameraTop;
        view.unk4A98C0 = -field.cameraTop;
    }

    if (field.cameraBottom + pixelsDown > 0x10 * state.map.height)
    {
        view.unk4A98C0 = 0x10 * state.map.height - field.cameraBottom;
        clamped = true;
    }

    if (clamped)
    {
        DebugLog(
            u8"pan past the map",
            this->Word(run, 1),
            this->Word(run, 2),
            field.cameraLeft >> 4,
            field.cameraTop >> 4
        );
    }

    const int length = static_cast<int>(2 * steps + 4);
    run.valueB = length;
    this->Advance(run, length);
    return true;
}

// Command 0x3E: role `id` put on a map (its record's act, place, event and talk set; 0xFFFF keeps the map or takes
// the controlled role's tile). A role on the map leaves it first (its moves freed); a role put on the current map is
// loaded from maps.dat and placed (replacing a role of the same id), and a role with flag 0x200 takes the empty
// bubble marks.
bool Talk::PlaceRole(EventRun& run)
{
    Game& owner = this->game;
    GameState& state = owner.state;
    const Role& player = state.roles[static_cast<std::size_t>(state.playerRole)];
    const u16 id = this->EventRoleId(run, this->Word(run, 1));
    i16 setBits = 0;
    i16 event = -1;
    run.valueA = id;

    if (state.FindRoleOrPlayer(id, run.found))
    {
        for (MoveRecord& move : state.moves)
        {
            if (move.role == run.found)
            {
                move.Clear();
            }
        }

        Role& role = state.roles[run.found];
        setBits = static_cast<i16>(role.flags);
        role.flags &= ~(Role::Visible | Role::Solid);
        owner.field.ClearRoleCells(role, static_cast<u16>(owner.unk4B7BC4));
        role.flags |= Role::Removed;
        event = static_cast<i16>(role.event);
    }

    run.code = EventCommand::None;
    const i16 map =
        this->Word(run, 2) == 0xFFFF ? static_cast<i16>(owner.currentMap) : static_cast<i16>(this->Word(run, 2));
    const i16 tileY = this->Word(run, 5) == 0xFFFF ? static_cast<i16>(static_cast<u32>(player.y) >> 4)
                                                   : static_cast<i16>(this->Word(run, 5));
    const i16 tileX = this->Word(run, 4) == 0xFFFF ? static_cast<i16>(static_cast<u32>(player.x) >> 4)
                                                   : static_cast<i16>(this->Word(run, 4));
    run.valueA = map;

    if (!owner.UpdateRoleRecord(
            id,
            static_cast<i16>(this->Word(run, 6)),
            static_cast<i16>(this->Word(run, 7)),
            static_cast<i16>(this->Word(run, 8)),
            tileX,
            tileY,
            event,
            static_cast<i16>(this->Word(run, 3)),
            setBits,
            -1,
            map
        ))
    {
        DebugLog(u8"maps.dat role missing", this->Word(run, 1));
        this->Next(run);
        return true;
    }

    const u16 target = this->Word(run, 2) == 0xFFFF ? static_cast<u16>(owner.currentMap) : this->Word(run, 2);

    if (static_cast<u16>(owner.currentMap) == target)
    {
        Role loaded;
        owner.LoadRoleRecord(loaded, this->Word(run, 1));
        owner.InitRole(loaded);
        Role* placed = nullptr;
        int index = 1;

        while (index < state.roleCount && state.roles[static_cast<std::size_t>(index)].id != loaded.id)
        {
            ++index;
        }

        if (index >= state.roleCount)
        {
            if (state.roleCount < GameState::Roles)
            {
                placed = &state.roles[static_cast<std::size_t>(state.roleCount)];
                *placed = loaded;
                owner.field.InsertRole(*placed, static_cast<int>(placed->flags & Role::LayerMask));
                ++state.roleCount;
            }
        }
        else
        {
            placed = owner.field.RemoveRole(loaded.id, static_cast<int>(loaded.flags & Role::LayerMask), 0, 1);

            if (placed == nullptr)
            {
                placed = &state.roles[static_cast<std::size_t>(index)];
            }

            *placed = loaded;
            owner.field.InsertRole(*placed, static_cast<int>(placed->flags & Role::LayerMask));
        }

        if (placed != nullptr && (placed->flags & Role::Bubbles) != 0)
        {
            for (RoleMark& mark : owner.scene.marks)
            {
                if (mark.role == 0)
                {
                    mark.role = placed->id;
                    mark.x = static_cast<u16>(placed->x);
                    mark.y = static_cast<u16>(placed->y);
                    mark.unk08 = 0;
                    mark.flags = 0;
                    mark.unk0E = 0;
                }
            }
        }
    }

    this->Next(run);
    return true;
}

// Commands 0xA2 and 0x5B: the name plate of the next talk window from maps.dat's name table (0xA2: by script variable
// 0xB or 0xC, which must be set; 0x5B: by role), the default names of the first two roles replaced by theirs.
bool Talk::LoadName(EventRun& run)
{
    Game& owner = this->game;
    u32 index = 0;

    if (run.code == EventCommand::RoleNamePlate)
    {
        index = this->EventRoleId(run, this->Word(run, 1));
        run.found = index;
    }
    else
    {
        const u16 variable = this->Word(run, 1);
        run.found = variable;

        if (variable != 0xB && variable != 0xC)
        {
            DebugLog(u8"對話系統(Caption2)u8", u8"設定的變數(%d)和系統衝突", variable);
            this->Next(run);
            return true;
        }

        index = static_cast<u32>(owner.party.variables[variable]);
        run.found = index;

        if (index == 0)
        {
            DebugLog(u8"對話系統(Caption2)u8", u8"設定的變數(0)和系統衝突");
            this->Next(run);
            return true;
        }
    }

    const std::vector<u8>& data = owner.databases.mapsData;
    const u32 at = MapsDword(data, MapsTable(data, 8) + 4 * index);

    for (std::size_t offset = 0; offset < this->dialogName.size(); ++offset)
    {
        this->dialogName[offset] = at + offset < data.size() ? static_cast<char>(data[at + offset]) : '\0';
    }

    const std::size_t end = TextEndAt(
        std::span<const u8>(reinterpret_cast<const u8*>(this->dialogName.data()), this->dialogName.size()),
        0
    );

    this->dialogName[end] = '\0';
    std::string name(this->dialogName.data(), this->dialogName.size());
    const Party& party = owner.party;
    Talk::ReplacePrefix(
        name,
        0,
        Big5(Party::RoleNames[0]),
        std::string_view(party.names[0].data(), strnlen(party.names[0].data(), party.names[0].size())),
        0x20
    );
    Talk::ReplacePrefix(
        name,
        0,
        Big5(Party::RoleNames[1]),
        std::string_view(party.names[1].data(), strnlen(party.names[1].data(), party.names[1].size())),
        0x20
    );
    std::copy_n(name.begin(), this->dialogName.size(), this->dialogName.begin());
    this->Next(run);
    return true;
}

// Command 0x60: a map movie from Video\ (in the installation, else on the CD; a missing CD ends the game): "%" plays
// it without the fade in, "*" fades it out darkening. Its header gives the frames, the first chunk and the palettes
// (an 8-bit movie: the frames they change at and the palettes from the file's end); the first frame is read and the
// main loop slows to 70 ms a frame.
bool Talk::OpenMapMovie(EventRun& run)
{
    Game& owner = this->game;
    GameState& state = owner.state;
    MapMovie& film = owner.fieldView.movie;
    const std::vector<u8>& script = owner.databases.script;
    state.ClearFrameInterval();
    state.SetFrameInterval(0x46);
    film.options = 0;
    this->Advance(run, kOpcodeBytes);

    if (this->Byte(run, 0) == '%')
    {
        ++run.cursor;
        film.options |= MapMovie::NoFadeIn;
        ++run.context.unk20;
    }

    if (this->Byte(run, 0) == '*')
    {
        ++run.cursor;
        film.options |= MapMovie::DarkenOut;
        ++run.context.unk20;
    }

    std::array<u8, 0x400> copy = {};

    for (std::size_t index = 0; index < 0x20; ++index)
    {
        copy[index] = run.cursor + index < script.size() ? script[run.cursor + index] : u8{0};
    }

    std::size_t end = 0;

    while (end + 1 < copy.size() && TextWord(copy, end) != kTextEnd)
    {
        ++run.context.unk20;
        ++end;
        ++run.cursor;
    }

    run.cursor += 2;
    run.context.unk20 = static_cast<u16>(run.context.unk20 + 2);
    copy[end] = 0;
    const std::string name(reinterpret_cast<const char*>(copy.data()), end);
    ErrorLog(u8"Talk:ani_open:cd:%s:%s", owner.config.cdRoot, name);
    std::string path = owner.config.installDirectory + "Video\\" + name;
    owner.AudioPump();
    File probe;

    if (probe.Open(path, File::Disposition::OpenExisting, File::Access::Read))
    {
        probe.Close();
        path = owner.config.installDirectory + "Video\\";
    }
    else
    {
        probe.Close();
        int drive = 0;

        if (owner.CheckCd(owner.config.cdRoot, drive) == 2)
        {
            owner.systemFlags |= 4;
            run.stopped = true;
            return false;
        }

        path = owner.config.cdRoot + "swd3\\Video\\";
        path[0] = static_cast<char>(drive);
    }

    path += name;
    ErrorLog(u8"Talk:Ani_Open::%s", path);
    owner.AudioPump();

    if (!film.file.Open(path, File::Disposition::OpenExisting, File::Access::Read))
    {
        DebugLog(
            u8"MAni Open Failure!",
            u8"MAni File Error!\n"
            u8"%s",
            name
        );
        return false;
    }

    film.chunks.next.reset();
    owner.AudioPump();
    std::array<u8, 0x24> header = {};
    u32 bytes = static_cast<u32>(header.size());
    film.file.Read(header.data(), bytes);

    if (bytes != header.size())
    {
        ErrorLog(u8"Talk:Ani_Open Header read fail ,%s", film.file.ErrorText());
        SleepMilliseconds(0xFA);
    }

    auto word = [&header](std::size_t at)
    {
        return static_cast<u16>(header[at] | header[at + 1] << 8);
    };

    auto dword = [&word](std::size_t at)
    {
        return static_cast<u32>(word(at) | static_cast<u32>(word(at + 2)) << 16);
    };

    MovieChunk& head = film.chunks;
    head.offset = 0;
    head.unk0C = 0x18;
    head.rows = 1;
    head.size = 0x18;
    head.nextUnk0C = dword(0x18);
    head.nextRows = dword(0x1C);
    head.nextSize = dword(0x20);
    head.frame = 0;
    head.next.reset();
    film.lastFrame = static_cast<i32>(dword(4));
    film.active = word(0xE);
    film.height = word(0x10);
    film.frame.assign(0x9C400, 0);
    film.buffer.assign(0x9C400, 0);
    film.palette.clear();
    film.paletteFrames.clear();
    film.paletteCount = word(0x14);
    film.paletteIndex = 0;

    if (word(8) == 8)
    {
        if (film.paletteCount != 0)
        {
            owner.AudioPump();
            film.paletteFrames.assign(0x200, 0);
            u32 frames = 0x400;
            film.file.SeekEnd(-0x400);
            film.file.Read(film.paletteFrames.data(), frames);
        }

        i32 back = 0x300;

        if (film.paletteCount != 0)
        {
            back = 0x300 * (film.paletteCount - film.paletteIndex++) + 0x400;
        }

        owner.AudioPump();
        std::array<u8, 0x300> colors = {};
        film.file.SeekEnd(-back);
        u32 read = 0x300;
        film.file.Read(colors.data(), read);
        film.palette.assign(0x100, 0);

        for (std::size_t index = 0; index < 0x100; ++index)
        {
            const u8* color = colors.data() + 3 * index;
            const u32 high = (color[1] & 0xF8u) | 0x20u * (color[0] & 0xF8u);
            film.palette[index] = static_cast<u16>((color[2] >> 3) | 4 * high);
        }

        owner.painter.ConvertPixels(film.palette);
    }

    film.frameIndex = 1;
    owner.fieldView.ReadMovieFrame();
    film.frameIndex = MapMovie::FadeInStart;

    if ((film.options & MapMovie::NoFadeIn) != 0)
    {
        film.frameIndex = 1;
        owner.ShowFrame();
    }

    film.saved = 0;
    owner.systemFlags |= 2;
    return false;
}

// Commands 0x32..0x62: the camera, the colour fade, the roles' acts, layers and followers, the quake, the sprites and
// line effects over the screen, the movies and the battles.
bool Talk::RunSceneCommand(EventRun& run)
{
    Game& owner = this->game;
    GameState& state = owner.state;
    Scene& view = owner.scene;
    const Field& field = owner.field;
    const Role blank;

    switch (run.code)
    {
        case EventCommand::PanCamera:
        case EventCommand::PanCameraTo:
        case EventCommand::PanCameraToRole:
            return this->PanCamera(run);

        case EventCommand::WaitPan:
            if (view.unk4A99F0 == 0 && view.unk4A98C0 == 0 && view.unk4A9480 == 0 && view.unk4A93D8 == 0)
            {
                this->Next(run);
                return true;
            }

            return false;

        case EventCommand::FadeColors:
        {
            ColorFade& fade = view.fade;
            fade.red = static_cast<float>(static_cast<i16>(this->Word(run, 1)));
            fade.green = static_cast<float>(static_cast<i16>(this->Word(run, 2)));
            fade.blue = static_cast<float>(static_cast<i16>(this->Word(run, 3)));
            const i32 red = static_cast<i16>(this->Word(run, 4));
            const i32 green = static_cast<i16>(this->Word(run, 5));
            const i32 blue = static_cast<i16>(this->Word(run, 6));
            run.valueA = red;
            run.valueB = green;
            fade.startRed = static_cast<float>(red);
            fade.startGreen = static_cast<float>(green);
            fade.startBlue = static_cast<float>(blue);
            fade.frames = this->Word(run, 7);
            const auto frames = static_cast<double>(fade.frames);
            fade.stepRed =
                static_cast<float>(static_cast<double>(red - static_cast<i32>(crt::Ftol(fade.red))) / frames);
            fade.stepGreen =
                static_cast<float>(static_cast<double>(green - static_cast<i32>(crt::Ftol(fade.green))) / frames);
            fade.stepBlue =
                static_cast<float>(static_cast<double>(blue - static_cast<i32>(crt::Ftol(fade.blue))) / frames);
            this->Next(run);
            return true;
        }

        case EventCommand::WaitFade:
            if (view.fade.frames <= 0)
            {
                this->Next(run);
                return true;
            }

            return false;

        case EventCommand::SeekAct:
        {
            state.FindRoleOrPlayer(this->EventRoleId(run, this->Word(run, 1)), run.found);
            run.valueB = static_cast<i16>(this->Word(run, 2));
            run.valueA = 0;

            if (Role* role = this->FoundRole(run))
            {
                role->act.delay = 0;
                role->act.position = 0;

                if (owner.acts.Run(role->act) == 0)
                {
                    LogActError(u8"Act Err(Talk:SeekAct1)", *role);
                }

                for (int step = 0; step < run.valueB; ++step)
                {
                    role->act.delay = 0;

                    if (owner.acts.Run(role->act) == 0)
                    {
                        LogActError(u8"Act Err(Talk:SeekAct2)", *role);
                    }

                    role->act.view = 0;
                    run.valueA = step + 1;
                }
            }

            this->Next(run);
            return true;
        }

        case EventCommand::LayerFront:
        case EventCommand::LayerMiddle:
        case EventCommand::LayerBack:
        {
            state.FindRoleOrPlayer(this->EventRoleId(run, this->Word(run, 1)), run.found);

            if (Role* role = this->FoundRole(run))
            {
                const u32 layer = role->flags & Role::LayerMask;
                run.valueA = static_cast<i32>(layer);
                role->flags &= ~Role::LayerMask;

                if (run.code == EventCommand::LayerFront)
                {
                    role->flags |= 1;
                }

                if (run.code == EventCommand::LayerBack)
                {
                    role->flags |= 2;
                }

                owner.field.RemoveRole(
                    role->id,
                    static_cast<int>(layer),
                    static_cast<int>(static_cast<u32>(role->y) >> 4) - 1,
                    0
                );
            }

            this->Next(run);
            return false;
        }

        case EventCommand::ScreenAnimation:
        {
            auto animation = std::make_unique<ScriptAnimation>();
            ResetPlayback(animation->act);
            animation->x = static_cast<i16>(this->Word(run, 1));
            animation->y = static_cast<i16>(this->Word(run, 2));
            animation->act.id = this->Word(run, 3);
            animation->act.action = this->Word(run, 4);
            animation->next = std::move(view.animationsA);
            view.animationsA = std::move(animation);
            this->Next(run);
            return false;
        }

        case EventCommand::Sound:
            owner.audio.PlaySound(this->Word(run, 1), owner.config.soundVolume);
            this->Next(run);
            return false;

        case EventCommand::ShowScreen:
        case EventCommand::BlackScreen:
            view.unk4C9A18 &= ~1;

            if (run.code == EventCommand::BlackScreen)
            {
                if (owner.offscreen != nullptr)
                {
                    owner.offscreen->Fill(0);
                }

                view.unk4C9A18 |= 1;
            }

            this->Next(run);
            return false;

        case EventCommand::PlaceRoleOnMap:
            return this->PlaceRole(run);

        case EventCommand::Quake:
        {
            std::size_t count = 0;

            while (this->Word(run, 2 + count) != kListEnd &&
                   run.cursor + 2 * (2 + count) + 1 < owner.databases.script.size())
            {
                ++count;
            }

            run.valueA = static_cast<i32>(count);

            if (count > 0x38)
            {
                DebugLog(u8"quake list too long");
                return false;
            }

            view.shake.fill(Scene::ShakeEnd);

            for (std::size_t index = 0; index < count; ++index)
            {
                view.shake[index] = static_cast<i16>(this->Word(run, 2 + index));
            }

            const int length = static_cast<int>(2 * count + 6);
            run.valueA = length;
            view.shakeHold = this->Word(run, 1);
            view.shakeFrames = this->Word(run, 1);
            view.unk4A992C = field.cameraTop;
            view.unk4A93E4 = field.cameraLeft;
            this->Advance(run, length);
            return true;
        }

        case EventCommand::StopQuake:
            view.shake.fill(Scene::ShakeEnd);
            this->Next(run);
            return true;

        case EventCommand::JoinFollowers:
            if (state.FindRoleOrPlayer(this->Word(run, 1), run.found))
            {
                owner.FollowLeader(static_cast<int>(run.found));
            }

            this->Next(run);
            return false;

        case EventCommand::LeaveFollowers:
            owner.LeaveParty(
                this->Word(run, 1),
                this->Word(run, 2),
                this->Word(run, 3),
                this->Word(run, 4),
                this->Word(run, 5),
                this->Word(run, 6),
                this->Word(run, 7)
            );
            this->Next(run);
            return false;

        case EventCommand::Wait:
        {
            const u16 length = this->Word(run, 1);
            run.found = length;

            if ((length & 0x8000) == 0)
            {
                this->waitLength = length;
                this->SetWord(run, 1, static_cast<u16>(length | 0x8000));
                this->waitStart = owner.frameStart;
                return false;
            }

            if (owner.frameStart - this->waitStart <= this->waitLength)
            {
                return false;
            }

            this->SetWord(run, 1, static_cast<u16>(length & 0x7FFF));
            this->Next(run);
            return true;
        }

        case EventCommand::ClearRoleHidden:
        case EventCommand::SetRoleHidden:
        {
            const u16 id = this->EventRoleId(run, this->Word(run, 1));

            if (state.FindRoleOrPlayer(id, run.found))
            {
                Role& role = state.roles[run.found];
                role.flags =
                    run.code == EventCommand::ClearRoleHidden ? role.flags & ~Role::Hidden : role.flags | Role::Hidden;
            }
            else if (run.code == EventCommand::ClearRoleHidden)
            {
                owner.UpdateRoleRecord(id, -1, -1, -1, -1, -1, -1, -1, 0, static_cast<i16>(0xFBFF), -1);
            }
            else
            {
                owner.UpdateRoleRecord(id, -1, -1, -1, -1, -1, -1, -1, 0x400, -1, -1);
            }

            this->Next(run);
            return false;
        }

        case EventCommand::AttachAct:
            if (state.FindRoleOrPlayer(this->Word(run, 1), run.found))
            {
                const u16 act = this->Word(run, 2);
                state.roles[run.found].attached = act < view.unk4B9F68.size() ? &view.unk4B9F68[act] : nullptr;
            }

            this->Next(run);
            return false;

        case EventCommand::DetachAct:
            if (state.FindRoleOrPlayer(this->Word(run, 1), run.found))
            {
                state.roles[run.found].attached = nullptr;
            }

            this->Next(run);
            return false;

        case EventCommand::StopFade:
            view.fade.stepRed = 0;
            view.fade.stepGreen = 0;
            view.fade.stepBlue = 0;
            view.fade.frames = 0;
            this->Next(run);
            return true;

        case EventCommand::HoldRole:
            state.FindRoleOrPlayer(this->Word(run, 1), run.found);

            if (run.found < GameState::Roles)
            {
                owner.rolePath.HoldRole(static_cast<int>(run.found));
            }

            this->Next(run);
            return true;

        case EventCommand::TurnToFace:
        {
            const u16 id = this->EventRoleId(run, this->Word(run, 1));

            if (!state.FindRoleOrPlayer(id, run.found))
            {
                DebugLog(u8"Act Err(Talk:TurnFace[1])", id);
            }

            u32 other = 0;

            if (!state.FindRoleOrPlayer(this->Word(run, 2), other))
            {
                DebugLog(u8"Act Err(Talk:TurnFace[2])", this->Word(run, 2));
            }

            Role* role = this->FoundRole(run);
            const Role& turning = role != nullptr ? *role : blank;
            const Role& faced = other < GameState::Roles ? state.roles[other] : blank;
            i32 distance = 0;
            const int angle = AngleBetween(
                faced.x + 8 * faced.act.width,
                faced.y + 8 * faced.act.height,
                turning.x + 8 * turning.act.width,
                turning.y + 8 * turning.act.height,
                distance
            );
            const int direction = AngleToDirection(angle);
            run.valueA = direction;
            run.valueB = distance;

            if (role != nullptr)
            {
                role->act.action = 0;
                role->act.direction = 0;

                if (distance >= 4)
                {
                    role->act.direction = direction;
                }

                role->act.delay = 0;

                if (owner.acts.Run(role->act) == 0)
                {
                    LogActError(u8"Act Err(Talk:TurnFace[3])", *role);
                }

                owner.rolePath.HoldRole(static_cast<int>(run.found));
            }

            this->Next(run);
            return true;
        }

        case EventCommand::FixSpeed:
        case EventCommand::FreeSpeed:
        {
            const u16 id = this->EventRoleId(run, this->Word(run, 1));

            if (state.FindRoleOrPlayer(id, run.found))
            {
                Role& role = state.roles[run.found];

                if (run.code == EventCommand::FixSpeed)
                {
                    role.act.speed = static_cast<i16>(this->Word(run, 2) | 0x8000);
                    run.valueB = 6;
                }
                else
                {
                    run.valueB = 4;
                    role.act.speed = 0;
                }

                role.act.delay = 0;

                if (owner.acts.Run(role.act) == 0)
                {
                    LogActError(u8"Act Err(Talk:SetRoleSpd)", role);
                }
            }
            else
            {
                const std::u8string text = Utf8Format(u8"要設速度的角色(%d)不在地圖上%%Q", {id});
                this->ShowMessage(static_cast<u16>(owner.rolePath.RoleId(static_cast<u32>(state.playerRole))), text);
            }

            this->Advance(run, run.valueB);
            return true;
        }

        case EventCommand::FlyingSprite:
        {
            auto sprite = std::make_unique<ScriptSprite>();
            ResetPlayback(sprite->act);
            sprite->act.id = this->Word(run, 1);
            sprite->act.direction = 0;
            sprite->act.action = this->Word(run, 2);
            sprite->headX = static_cast<i16>(MapHeader::TileSize * this->Word(run, 3));
            sprite->headSpeed = static_cast<i16>(MapHeader::TileSize * this->Word(run, 4));
            sprite->targetX = static_cast<i16>(MapHeader::TileSize * this->Word(run, 5));
            sprite->targetY = static_cast<i16>(MapHeader::TileSize * this->Word(run, 6));
            const i32 fromX = sprite->headX;
            const i32 fromY = sprite->headSpeed;
            const auto speed = static_cast<double>(static_cast<i16>(this->Word(run, 7)));
            const i32 across = sprite->targetX - fromX;
            const i32 down = sprite->targetY - fromY;
            const double scale = speed / std::sqrt(static_cast<double>(down * down + across * across));
            sprite->stepX = static_cast<float>(static_cast<double>(across) * scale);
            sprite->stepY = static_cast<float>(static_cast<double>(down) * scale);
            sprite->x = static_cast<float>(fromX);
            sprite->y = static_cast<float>(fromY);
            sprite->next = std::move(view.spritesA);
            view.spritesA = std::move(sprite);
            this->Next(run);
            return true;
        }

        case EventCommand::DialogOverlay:
            this->dialogOptions &= ~Talk::Overlay;
            this->Next(run);
            return true;

        case EventCommand::HeadIn:
        {
            auto sprite = std::make_unique<ScriptSprite>();
            ResetPlayback(sprite->act);
            sprite->act.id = this->Word(run, 1);
            sprite->act.action = this->Word(run, 2);
            const u16 target = this->Word(run, 3);
            sprite->targetX = static_cast<i16>(target);
            sprite->headX = static_cast<i16>(-ScriptSprite::HeadMargin);
            sprite->targetY = static_cast<i16>(this->Word(run, 4) & (kPlaceAtTarget - 1));

            if (target > Display::Width / 2)
            {
                sprite->headX = static_cast<i16>(Display::Width + ScriptSprite::HeadMargin);
            }

            sprite->headSpeed = 0;

            if ((this->Word(run, 4) & kPlaceAtTarget) != 0)
            {
                sprite->headX = static_cast<i16>(target);
                sprite->headSpeed = ScriptSprite::HeadPlaced;
            }

            sprite->next = std::move(view.spritesB);
            view.spritesB = std::move(sprite);
            this->Next(run);
            return true;
        }

        case EventCommand::HeadOut:
        {
            for (ScriptSprite* sprite = view.spritesB.get(); sprite != nullptr; sprite = sprite->next.get())
            {
                if (sprite->act.id == this->Word(run, 1) && sprite->act.action == this->Word(run, 2))
                {
                    if (sprite->headSpeed >= 0)
                    {
                        sprite->headSpeed = static_cast<u16>(sprite->headX) <= Display::Width / 2 ? i16{-1} : i16{1};
                    }
                    else
                    {
                        sprite->headSpeed = ScriptSprite::HeadDropSpeed;
                    }

                    break;
                }
            }

            this->Next(run);
            return true;
        }

        case EventCommand::LineEffect:
        {
            const u16 mode = this->Word(run, 1);

            if (mode > ScriptEffect::NumberMask)
            {
                DebugLog(u8"effect number", run.context.event, mode);
                this->Next(run);
                return true;
            }

            run.valueA = 0;

            if (EraseNodes(
                    view.effects,
                    [mode](const ScriptEffect& effect)
                    {
                        return static_cast<u8>(effect.mode) == mode;
                    },
                    false
                ))
            {
                run.valueA = 1;
            }

            auto effect = std::make_unique<ScriptEffect>();
            effect->mode = mode;
            effect->color = static_cast<i16>(this->Word(run, 2));
            effect->x = static_cast<i16>(this->Word(run, 4) & kEvenPixels);
            effect->y = static_cast<i16>(this->Word(run, 5) & kEvenPixels);
            effect->limit = static_cast<i16>(this->Word(run, 6) & kEvenPixels);
            effect->count = static_cast<i16>(this->Word(run, 7) & kEvenPixels);

            if (effect->x < 0 || effect->y < 0 || effect->limit + effect->x > Display::Width ||
                effect->count + effect->y > Display::Height)
            {
                DebugLog(u8"effect off the screen", effect->x, effect->y, effect->limit, effect->count);
                this->Next(run);
                return true;
            }

            const auto columns = static_cast<std::size_t>(std::max<int>(effect->count, 0) + 4);
            effect->offsets.assign(columns, 0);
            effect->lengths.assign(columns, 0);
            run.valueB = 0;
            u16 kind = ScriptEffect::Falling;

            if (this->Word(run, 3) == 1)
            {
                run.valueB = effect->limit - 2;
                kind = ScriptEffect::Rising;
            }

            if (this->Word(run, 3) == 2)
            {
                kind = ScriptEffect::Still;
                run.valueB = 0;
            }

            effect->mode |= kind;

            for (int column = 0; column < effect->count; ++column)
            {
                effect->offsets[static_cast<std::size_t>(column)] = static_cast<i16>(run.valueB);
                effect->lengths[static_cast<std::size_t>(column)] = 2;
                run.valueA = column + 1;
            }

            effect->next = std::move(view.effects);
            view.effects = std::move(effect);
            this->Next(run);
            return true;
        }

        case EventCommand::LineEffectMode:
        {
            const u16 mode = this->Word(run, 1);

            if (mode > ScriptEffect::NumberMask)
            {
                DebugLog(u8"effect number", run.context.id, mode);
                this->Next(run);
                return true;
            }

            ScriptEffect* found = nullptr;

            for (ScriptEffect* effect = view.effects.get(); effect != nullptr; effect = effect->next.get())
            {
                if (static_cast<u8>(effect->mode) == mode)
                {
                    found = effect;
                    break;
                }
            }

            if (found == nullptr)
            {
                this->Next(run);
                return true;
            }

            const u16 change = this->Word(run, 2);

            if (change == 0)
            {
                run.valueA = ScriptEffect::Opening;
            }

            if (change == 1)
            {
                run.valueA = ScriptEffect::Closing;
            }

            if (change == 2)
            {
                EraseNodes(
                    view.effects,
                    [found](const ScriptEffect& effect)
                    {
                        return &effect == found;
                    },
                    true
                );
            }
            else
            {
                found->mode = static_cast<u16>(run.valueA | static_cast<u8>(found->mode));
            }

            this->Next(run);
            return true;
        }

        case EventCommand::PlayVideo:
            if (owner.offscreen != nullptr)
            {
                owner.offscreen->Fill(0);
            }

            owner.ShowFrame();
            owner.AudioPump();
            owner.PlayVideo(run.cursor, run.context.unk20);
            return false;

        case EventCommand::ChangeHead:
            for (ScriptSprite* sprite = view.spritesB.get(); sprite != nullptr; sprite = sprite->next.get())
            {
                if (sprite->act.id == this->Word(run, 1) && sprite->act.action == this->Word(run, 2))
                {
                    sprite->act.id = this->Word(run, 3);
                    sprite->act.action = this->Word(run, 4);
                    break;
                }
            }

            this->Next(run);
            return true;

        case EventCommand::RandomJump:
        {
            u32 count = 0;

            while (this->Dword(run, 2 + 4 * static_cast<std::size_t>(count)) != 0xFF00FF00 &&
                   run.cursor + 2 + 4 * static_cast<std::size_t>(count) < owner.databases.script.size())
            {
                ++count;
            }

            run.valueA = static_cast<i32>(count);

            if (count == 0)
            {
                Fault(owner.errors, u8"Integer divide by zero (0x00439070)");
            }

            run.valueB = static_cast<i32>(owner.random.Below(count));
            this->Jump(run, this->Dword(run, 2 + 4 * static_cast<std::size_t>(run.valueB)));
            return true;
        }

        case EventCommand::StartBattle:
            view.FreeEffects();
            view.FreeSpritesB();
            owner.unk4A94AC = static_cast<i16>(this->Word(run, 1)) | static_cast<i32>(0x80000000u);
            this->Next(run);
            return false;

        case EventCommand::RecordDialogWait:
        case EventCommand::RecordDialog:
            return this->OpenDialog(run);

        case EventCommand::RoleNamePlate:
            return this->LoadName(run);

        case EventCommand::SetCueFlag:
        case EventCommand::ClearCueFlag:
        {
            run.found = static_cast<u32>(this->Word(run, 1) - 1);

            if (run.found > 3)
            {
                DebugLog(u8"cue number", run.found);
            }

            this->SetStoryFlag(static_cast<int>(run.found + 0x1E), run.code == EventCommand::SetCueFlag);
            this->Next(run);
            return true;
        }

        case EventCommand::CameraHold:
            view.unk4C9A18 |= 2;
            this->Next(run);
            return false;

        case EventCommand::CameraFollow:
            view.unk4C9A18 &= ~2;
            this->Next(run);
            return false;

        case EventCommand::MapMovie:
            return this->OpenMapMovie(run);

        case EventCommand::WaitMapMovie:
            if (owner.fieldView.movie.active != 0)
            {
                return false;
            }

            this->Next(run);
            state.ClearFrameInterval();
            state.SetFrameInterval(0x23);
            return true;

        case EventCommand::Skip:
            this->Next(run);
            return false;

        default:
            return this->UnknownCommand(run);
    }
}

}  // namespace swd3
