# 普通世界帧外层协调器（`0x004120B0`）

状态：`assembly_exact`、`unit_verified`、`asset_verified`、
`sdl_runtime_integrated`；尚未 `original_diff_verified`

本文只以 `swd3.exe.lst` 的机器码和指令为行为真值。它恢复的是普通世界唯一主帧函数
`sub_4120B0` 的外层控制顺序；`0x00412930` 内部画面组合的证据和实现仍由
[`world-frame-composition-004120b0-00413370.md`](world-frame-composition-004120b0-00413370.md)
负责。

## 1. 一帧的物理顺序

| LST 范围 | OpenSWD3 责任 | 当前状态 |
|---|---|---|
| `0x004120B7..0x004120F7` | [倒序维护八个 HeadSgn 动作记录](world-head-sign-actions-004120b7-004120f7.md) | 已接入真实 action updater |
| `0x004120F9..0x00412197` | 玩家与相机按四个 transition 和步长移动 | 已接入真实 helper |
| `0x004121A1..0x004124D1` | [地图角色动作/路径账本](world-map-role-paths-004121a1-004124d1.md) | 已接入真实 owner |
| `0x004124DC..0x00412681` | [队伍角色动作账本](world-party-role-actions-004124dc-00412681.md) | 已接入真实 owner |
| `0x0041268C` | [`sub_414570` 脚本相机逐帧平移](world-camera-pan-00414570.md) | 已接入真实状态机 |
| `0x00412691` | `sub_4148F0` 选择序列临时滚动 | 已接入真实状态机 |
| `0x00412696` | 第一次 `AIL_serve` | audio port 原槽调用 |
| `0x004126A2..0x004126B3` | 锁定并绑定软件 source surface | 由传入 framebuffer 表达 |
| `0x004126B8` | `sub_412930` 世界画面组合 | 已接入真实 runtime vertical slice |
| `0x004126C7` | [`sub_4308C0(400, 8, 0)` 倒计时绘制](legacy-countdown-004308c0-00430b60.md) | 已接入真实 action/TSW/blit 路径 |
| `0x004126CC..0x004126E8` | [开发工具总门与 `sub_413FE0(left, top)` 调试叠层](world-debug-overlay-00413fe0.md) | 已接入真实 owner |
| `0x004126F0` | 第二次 `AIL_serve` | audio port 原槽调用 |
| `0x004126FF..0x00412716` | 普通世界唯一一次 `Blt` | world presentation port |
| `0x00412719..0x0041287C` | [玩家格指针、transition、快照与动作校验](world-player-post-frame-00412719-0041287c.md) | 已接入真实 helper |
| `0x0041287F..0x004128DA` | tile 层折返动画和 layer offset | 已接入真实状态机 |
| `0x004128DF..0x0041291D` | 条件恢复选择滚动前视口 | 已接入真实状态机 |

`run_legacy_world_frame` 因此不是第二套 renderer。它只拥有外层顺序，并在原始
`0x004126B8` 槽调用已有 `compose_legacy_world_runtime_frame`；呈现没有被挪到通用帧尾。

## 2. 三个不能合并的门

### 2.1 队伍角色数量

`0x004124DC` 把 `EBP` 设为 1，随后用无符号 `JBE` 判断队伍角色数。数量为 0 或 1 都
直接跳到 `0x0041268C`，只有 `count > 1` 才进入 `0x004124EF` 循环。真实 owner 保留
这个门，并只扫描队伍槽 `1..count-1`；索引 0 的当前主角不进入该循环。

### 2.2 开发工具调试叠层

`0x004126D4` 是 `CMP EAX, EBP` 后的 `JNZ`，所以只有总门**恰好等于 1**才调用调试
叠层；状态 2 不能按 truthy 处理。调用者压入当时临时滚动后的 camera left/top，另多压
一个常量 2 并回收 12 字节；被调函数只读取前两个参数，所以常量 2 是未使用的调用者
栈字，不是第三参数。完整碰撞格与诊断文字行为见
[`world-debug-overlay-00413fe0.md`](world-debug-overlay-00413fe0.md)。

### 2.3 呈现后的玩家对齐

`0x00412719` 与 `0x00412726` 分别检查玩家 X/Y 的低四位。任一未对齐便直接跳到
`0x0041283C`，因此原程序不会清除四个 movement transition，也不会更新格指针和玩家
快照。两轴都对齐时才执行空间链重插、旧格清除、格指针移动、新格标记、transition
清零、三组历史移动和地图格 flags 投影。精确顺序、mask 和历史布局见
[`world-player-post-frame-00412719-0041287c.md`](world-player-post-frame-00412719-0041287c.md)。
随后 `0x0041283C..0x0041287C` 的动作校验不受对齐门限制；更新失败只诊断，不终止
整帧。

```text
0041272E..0041277D  重插空间链，清旧占用，移动格索引并标记新占用
00412788..0041279A  清零 camera/player 的 X/Y transition
004127A0..00412839  条件移动三个 0x7C 历史并更新地图格投影 flags
0041283C..0041287C  不受对齐门限制的动作校验
```

实现把这段紧耦合状态变更放进一个真实 owner，但 owner 内部仍逐槽保留以上物理顺序；
原先三个 delegated stage 已删除。

## 3. 共享状态与现代边界

- 玩家、角色 span、相机矩形、frame runtime、raster、framebuffer、row jitter 和 tile
  layer state 在同一次调用中共享；玩家 transition、脚本相机平移和选择滚动依次修改
  同一个相机，叠加后的临时坐标直接传给背景与角色组合。
- 帧尾恢复后再把 camera left/top 同步回 frame runtime，下一帧不会遗留临时滚动值。
- 当前 tile layer offset 在组合前复制进 background source；动画只在呈现和玩家帧后
  账本之后推进，供下一帧使用。
- 倒计时 state 和静态动作记录由 coordinator 持有；显示时复用同一 framebuffer、clip、
  effect、jitter、action updater 与 TSW provider，未激活门在动作字段写入前返回。
- 72 槽地图角色 owner 与 composition 共用 Talk source；地图角色循环中新建立的 Talk
  context 会在同一帧进入角色组合门。`sub_42D920` 仍是显式 story/path 跨模块端口。
- 原程序假定玩家索引和 64-word 选择表永远有效。OpenSWD3 只在现代 span 所有权无效时
  于首次访问前返回；有效输入的调用、修改与门控顺序不变。
- 仍 delegated 的角色/界面 stage 返回失败时停在对应原槽并报告失败，不把尚未恢复的
  行为伪装成完整帧；HeadSgn 和帧后玩家动作更新失败都按原汇编只记录诊断并继续。

## 4. 验证边界

组合 UT 已固定完整 outer/inner/audio/presentation 事件序列，并覆盖：

- HeadSgn 的八槽遍历、四槽倒序更新及非致命失败；
- 地图角色固定 72 槽空态扫描已在 coordinator 中执行；活动路径的门、移动、到达、
  空间/表面账本、自动 Talk 和动作更新由独立 UT 固定；
- 队伍数量 `0/1` 的直接跳过，以及多队员槽的等待/停用 action 更新、单次步长、对齐后
  只从空间链移除、表面迁移和非致命 action 失败；
- 玩家与相机位移后，`sub_414570` 的正负脚本步长、逐轴完成清零、32 位回绕及共享
  更新体先推进同一相机，选择滚动随后叠加并完成 `0x00412930`；
- `sub_4308C0` 的 primary flag、抑制门、`M:SS` piece 顺序、固定 `(400,8,0)` 请求、
  静态动作记录延迟写入和受检资源失败；
- 调试叠层的两个真实参数、入口文字样式配置和原槽执行；
- party count `0/1` 跳过、developer-tools state `2` 跳过；
- 玩家对齐时完成空间链、格占用、transition 和历史账本，未对齐时保留这些状态但仍
  执行动作校验；
- 无效玩家索引、缺失选择 Y word、调试叠层失败和 composition 失败的原槽停止。

Linux LLVM `core` 154/154、Windows LLVM `app` 158/158 CTest 通过。SDL app 已用真实
地图 session、ACT/TSW runtime、软件 framebuffer 和 audio/presentation ports 调用该
coordinator；地图角色、队伍角色、脚本相机平移、倒计时绘制和开发调试叠层均已替代
outer 外部占位。需要原程序动态差分时，只准备
Frida spawn 工具并等待用户执行，不由开发流程启动原版。
