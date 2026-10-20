# 存档格式（save\\<n>.sav）

依据：swd32.exe 0x004070A0（存读档页，写入与读取两条路径）、0x00409600（存档预览）、0x004094A0（槽位可否打开）。
槽位 0..0x62 共 99 个，文件名 `%ssave\%d.sav`（安装目录 + 槽号）。所有整数为小端。

## 压缩块

下文的「块」指：`u32 压缩长度`、`u32 原始长度`、LZO1X 压缩数据（MapCache::Pack 0x004267E0 写，MapCache::Unpack
0x00426820 读）。读取时只用压缩长度，原始长度只决定块 C 的缓冲区（原始长度的两倍）。

## 布局

| 偏移 | 长度 | 内容 | 写入来源 | 读回 |
|---|---|---|---|---|
| 0x0000 | 0xC | 存档时刻 `%04d%02d%02d%02d%02d`（年月日时分，无结尾 NUL） | GetLocalTime | 只用于预览 |
| 0x000C | 0x9600 | 缩略图 160×120，原始像素格式（Painter::ConvertPixelsBack 后的 0x4B00 个 u16） | 地图画到 drawSurface 后 ShrinkScreenQuarter | 只用于预览 |
| 0x960C | 0x20 | 地点名（NUL 结尾，余下为 0） | GameState::map.name（0x004B7954，LoadCity 写入的地图名） | 只用于预览 |
| 0x962C | 块 A | 剧情旗标 0x400 字节（GameState::flags 0x004AB384） | | 全部覆盖 |
| | 块 B | maps.dat 表 1：角色记录，0x16 字节一条（Scene::rolesTable 起，长 Scene::rolesTableBytes） | | 重新读入 maps.dat 后逐条交给 Game::UpdateRoleRecord（字 0 地图、1 角色号、2 动作档、3 动作、4 方向、5 x、6 y、7 事件、8 对话、10 置位；字 9 不用） |
| | 0x1C | 7 个 u32：partyLeaderId、currentMap、主控角色的列（x>>4）、行（y>>4）、角色记录 +0x40、+0x48、+0x74 | | 回填主控角色 |
| | 块 C | 队伍资料（见下） | | |
| | 0x40 | 四个角色名（Party::names，各 0x10） | | |
| | 4 | 游戏时间（秒），写入时加上自上次计时以来的 difftime | Game::playSeconds | 重新开始计时 |
| | 0x40 | 返回点：u16 returnColumn、u16 returnRow、u16 returnMap、u16 warpLeaderAct、u32 计时器 0x004CAD20、u32 timerEvent，余下为 0 | | returnMap 为 0 时改用当前地图与主控角色位置 |
| | 0x180 | 四个角色的 0x60 字节战斗块（0x004ACF50） | | |
| | 4 + n | Fame::Save 的长度与原始数据（不压缩） | 0x00477C90 | Fame::Free、Fame::Load |
| | 0x84 | u16 shakeHold、u16 shakeFrames、0x80 字节震动偏移（Scene::shake） | | 缺失时 shake 填 0xCF |
| | 块 D | 0x68 字节：两个 0x34 字节名字（Scene::noTownWarpText、noExitWarpText，空时全 0） | | 首字节非 0 才建立 |

块 A 之后的各段读取时逐段比较文件长度，文件到头即停止，旧版较短的存档也能读入（停在哪一段，后面的状态保持原值，
shake 除外）。

## 块 C：队伍资料

| 偏移 | 长度 | 内容 |
|---|---|---|
| +0x000 | 4 | Party::clock（0x004ACDB0） |
| +0x004 | 0x100 | Party::variables（64 个 i32，0x004ACBD0） |
| +0x104 | 0xE0 | 四个队员记录（0x38 字节，0x004AB790）；每个记录 +0x26..+0x29 写成 0，读回后也清 0 |
| +0x1E4 | 0x80 | 64 个装备栏物品号（Party::equipment，0xFFDC 为空） |
| | 变长 | 四个技能表，各为物品号序列，以 0 结尾（表头的「無」不写） |
| | 变长 | 物品栏：数量大于 0 时写 `id, count`，待定数量大于 0 时写 `id|0x8000, pending`；以 u16 0 结尾 |
| | 变长 | maps.dat 表 2 项 1 所指偏移数组的各条 8 字节场景音乐记录（+0 地图、+2 曲 1、+4 曲 2、+6 提示位）；写到偏移为 0 为止，以 u16 0 结尾 |

读回场景音乐记录时按地图号在同一偏移数组中查找（遇到地图 0 的记录为止），记录 +6 的字带 0x800 时不覆盖。读回技能时每个技能插在表头之后，顺序与存档相反。读回装备与技能时以 Database::LoadObject 取物品资料，
取不到的装备栏改为空（0xFFDC），取不到的技能丢弃。

## 原有的不对称（保留）

- 主控角色位置写入的是格子坐标（x>>4、y>>4），读回时直接作为像素坐标交给 LoadCity；返回点为 0 时又右移一次。
- 场景音乐记录写到偏移为 0 为止，读回时查到地图号为 0 的记录为止。
- 技能读回顺序与存档相反。
