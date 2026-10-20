#include "map/field_view_internal.h"

namespace swd3
{

void FieldView::DrawSmallText(int x, int y, u32 color, const std::string& text)
{
    this->game.smallFont.DrawText(this->game.drawSurface, x, y, text, static_cast<Pixel>(color), TextFont::BoldOutline);
}

// swd32.exe 0x00413FE0: the debug display: the cell marks of the walls (F2) and the lines of the state, the region
// and the roles under the pointer (F1)
void FieldView::DrawDebug(int cameraLeft, int cameraTop)
{
    constexpr std::u8string_view kMusicLine = u8"[%d/%d][%d/%d][playing/%d][mode/%s],場景%s,劇情%s";  // 0x004A04F0
    constexpr std::u8string_view kStory = u8"劇情";                                                   // 0x004A052C
    constexpr std::u8string_view kScene = u8"場景";                                                   // 0x004A0524
    constexpr std::u8string_view kBlocked = u8"不可通過";                                             // 0x004A04C8
    constexpr std::u8string_view kPassable = u8"可通過";                                              // 0x004A04C0
    constexpr std::u8string_view kClickable = u8"可點選";                                             // 0x004A04B8
    constexpr std::u8string_view kNotClickable = u8"不可點選";                                        // 0x004A04AC
    constexpr std::u8string_view kRegionLine = u8"劇情號碼[%d] [%d,%s] [%d,%s]";                      // 0x004A048C
    constexpr std::u8string_view kBadRegion = u8"錯誤的地圖劇情號碼 (%d)";                            // 0x004A04D8
    constexpr std::u8string_view kOverlap = u8"角色重疊";                                             // 0x004A0480
    constexpr std::u8string_view kRoleLine = u8"角色 [GUID %d] [act %d] [qq %d] [dir %d]";            // 0x004A0454
    Game& owner = this->game;
    GameState& state = owner.state;
    const Field& field = owner.field;
    const Scene& view = owner.scene;
    owner.smallFont.SetBackground(TextFont::NoBackground);
    owner.smallFont.SetShadow(0);
    const std::array<u16, Config::Channels>& masks = owner.config.channelMasks;
    const u32 red = masks[Config::RedChannel];
    const u32 green = masks[Config::GreenChannel];
    const u32 blue = masks[Config::BlueChannel];

    if (owner.debugCells == 1)
    {
        this->MarkCells(MapHeader::Wall, static_cast<int>(red + green + blue), cameraLeft, cameraTop, 0);
        this->MarkCells(MapHeader::RoleCell, static_cast<int>(green + blue), cameraLeft, cameraTop, kRoleCellInset);
        this->MarkCells(MapHeader::RegionMask, static_cast<int>(red + green), cameraLeft, cameraTop, kRegionInset);
        this->MarkCells(MapHeader::SolidRole, static_cast<int>(red), cameraLeft, cameraTop, kSolidRoleInset);
        this->MarkCells(MapHeader::Unk800000, static_cast<int>(red + blue), cameraLeft, cameraTop, kUnk800000Inset);
    }

    if (owner.debugText != 1)
    {
        return;
    }

    const u32 white = owner.textColors.primary[kWhiteColor];
    const std::vector<u32>& blocks = state.map.blocks;
    const std::size_t pointerCell = (static_cast<u32>(field.cameraLeft + state.pointerX) >> MapHeader::TileShift) +
                                    static_cast<std::size_t>(state.map.width) *
                                        (static_cast<u32>(state.pointerY + field.cameraTop) >> MapHeader::TileShift);
    const u32 region = pointerCell < blocks.size() ? blocks[pointerCell] & MapHeader::RegionMask : 0;
    const Role& player = state.roles[static_cast<std::size_t>(state.playerRole)];
    this->DrawDebugText(
        kDebugLeft,
        DebugRow(0),
        white,
        u8"MAct[%4.1f/%4.1f] LTCor[%4d/%4d]",
        static_cast<double>(static_cast<u32>(player.x)) / MapHeader::TileSize,
        static_cast<double>(static_cast<u32>(player.y)) / MapHeader::TileSize,
        field.cameraLeft >> MapHeader::TileShift,
        field.cameraTop >> MapHeader::TileShift
    );
    this->DrawDebugText(
        kDebugLeft,
        DebugRow(1),
        white,
        u8"Mouse :OnMap[%4d/%4d] OnScr[%4d/%4d]",
        (static_cast<u32>(state.pointerX) >> MapHeader::TileShift) +
            static_cast<u32>(field.cameraLeft >> MapHeader::TileShift),
        (static_cast<u32>(state.pointerY) >> MapHeader::TileShift) +
            static_cast<u32>(field.cameraTop >> MapHeader::TileShift),
        state.pointerX,
        state.pointerY
    );
    const ButtonState& click = owner.controls.At(Button::MouseLeft);
    this->DrawDebugText(
        kDebugLeft,
        DebugRow(2),
        white,
        u8"FPS[%2d] FUsed(%d),kr.d[%1d] ,kr.p[%4d] _IPA[%4d]",
        owner.frameTime != 0 ? kMillisecondsPerSecond / static_cast<u32>(owner.frameTime) : 0u,
        owner.mainFrameTime,
        click.press,
        click.heldFrames,
        owner.controls.mouseSuppressed
    );
    this->DrawDebugText(
        kDebugLeft,
        DebugRow(3),
        white,
        u8"MapID[%d] MapCyc[%d] %d",
        owner.currentMap,
        view.layerFrames,
        owner.state.walkSpeed
    );
    this->DrawDebugText(
        kDebugLeft,
        DebugRow(4),
        white,
        u8"ULck[%x] UIpt[%x] gTm[%d] Fsc[%d] ,BtlM[%d] ,BtnIp[%d]",
        view.playerHold,
        owner.talk.advance,
        owner.party.clock,
        view.encounterSteps & Scene::EncounterStepMask,
        view.encounterGroup,
        owner.talk.choice
    );
    this->DrawDebugText(
        kDebugLeft,
        DebugRow(5),
        white,
        u8"T.Gd[%d] T.Tid[%d] dbg1[%d] dbg2[%d]",
        static_cast<u32>(state.leader.id),
        static_cast<u32>(state.leader.event),
        view.unk4C9A08,
        view.unk4ACE64
    );

    auto mode = [&view](u32 used, u32 repeat) -> std::string
    {
        if ((view.musicCue & used) == 0)
        {
            return "Nil";
        }

        return (view.musicCue & repeat) != 0 ? "Rep1" : "Cyc2";
    };

    this->DrawDebugText(
        kDebugLeft,
        DebugRow(6),
        white,
        kMusicLine,
        view.musicTunes[Scene::SceneFirst],
        view.musicTunes[Scene::SceneSecond],
        view.musicTunes[Scene::StoryFirst],
        view.musicTunes[Scene::StorySecond],
        view.musicCue & Scene::TrackByte,
        (view.musicCue & Scene::StoryMusic) != 0 ? kStory : kScene,
        mode(Scene::SceneLoops, Scene::SceneRepeatFirst),
        mode(Scene::StoryLoops, Scene::StoryRepeatFirst)
    );

    if (region != 0)
    {
        const MapRegion* found = owner.field.FindRegion(static_cast<int>(region));

        if (found != nullptr)
        {
            const std::string_view name(found->name.data(), found->name.size());
            this->DrawDebugText(
                kDebugLeft,
                DebugRow(kScreenRows - 2),
                white,
                u8"%s",
                std::string(name.substr(0, name.find('\0')))
            );
            const auto flag = static_cast<u32>(found->flag);
            const std::u8string_view passable =
                state.GameFlag(static_cast<int>(flag >> MapRegion::BlockFlagShift)) ? kBlocked : kPassable;
            const std::u8string_view clickable =
                state.GameFlag(static_cast<int>(flag & MapRegion::ClickFlagMask)) ? kClickable : kNotClickable;
            this->DrawDebugText(
                kDebugLeft,
                DebugRow(kScreenRows - 1),
                white,
                kRegionLine,
                found->event & MapRegion::EventMask,
                flag & MapRegion::ClickFlagMask,
                clickable,
                flag >> MapRegion::BlockFlagShift,
                passable
            );
        }
        else
        {
            this->DrawDebugText(kDebugLeft, DebugRow(kScreenRows - 3), white, kBadRegion, region);
        }
    }

    const auto mapX = static_cast<u32>(state.pointerX + field.cameraLeft);
    const auto mapY = static_cast<u32>(state.pointerY + field.cameraTop);
    int overlapping = 0;

    for (u32 index = 1; index < static_cast<u32>(state.roleCount) && index < GameState::Roles; ++index)
    {
        const Role& role = state.roles[index];

        if (role.act.id == 0)
        {
            continue;
        }

        if (mapX > static_cast<u32>(role.x - MapHeader::TileSize) &&
            mapX < static_cast<u32>(role.x + kOverlapColumns * MapHeader::TileSize) &&
            mapY > static_cast<u32>(role.y - kOverlapRows * MapHeader::TileSize) &&
            mapY < static_cast<u32>(role.y + MapHeader::TileSize))
        {
            ++overlapping;

            if (overlapping >= kOverlapRoles)
            {
                this->DrawDebugText(kDebugLeft, DebugRow(kScreenRows - 5), white, kOverlap);
            }

            this->DrawDebugText(
                kDebugLeft,
                DebugRow(kScreenRows - 4),
                owner.textColors.primary[kRoleLineColor],
                kRoleLine,
                static_cast<u32>(role.id),
                role.act.id,
                role.act.action,
                role.act.direction
            );
            this->DrawDebugText(
                kDebugLeft,
                DebugRow(kScreenRows - 3),
                white,
                u8"[GUID %d] [Talk %d] [Path %d] [Argu %4x] [ArrIdx %d]",
                static_cast<u32>(role.id),
                static_cast<u32>(role.event),
                static_cast<u32>(role.unk1C),
                role.flags,
                index
            );
        }
    }
}

}  // namespace swd3
