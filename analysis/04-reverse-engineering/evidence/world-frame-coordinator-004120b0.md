# 普通世界帧外层协调器（`0x004120B0`）

状态：`assembly_exact`、`unit_verified`；尚未 `sdl_runtime_integrated`、
`original_diff_verified`

本文只以 `swd3.exe.lst` 的机器码和指令为行为真值。它恢复的是普通世界唯一主帧函数
`sub_4120B0` 的外层控制顺序；`0x00412930` 内部画面组合的证据和实现仍由
[`world-frame-composition-004120b0-00413370.md`](world-frame-composition-004120b0-00413370.md)
负责。

## 1. 一帧的物理顺序

| LST 范围 | OpenSWD3 责任 | 当前状态 |
|---|---|---|
| `0x004120B7..0x004120F7` | 倒序维护 head-sign 动作记录 | 显式 delegated stage |
| `0x004120F9..0x00412197` | 玩家与相机按四个 transition 和步长移动 | 已接入真实 helper |
| `0x004121A1..0x004124D1` | 地图角色动作/路径账本 | 显式 delegated stage |
| `0x004124DC..0x00412681` | 队伍角色动作账本 | `count > 1` 时 delegated |
| `0x0041268C` | `sub_414570` 帧前维护 | 显式 delegated stage |
| `0x00412691` | `sub_4148F0` 选择序列临时滚动 | 已接入真实状态机 |
| `0x00412696` | 第一次 `AIL_serve` | audio port 原槽调用 |
| `0x004126A2..0x004126B3` | 锁定并绑定软件 source surface | 由传入 framebuffer 表达 |
| `0x004126B8` | `sub_412930` 世界画面组合 | 已接入真实 runtime vertical slice |
| `0x004126C7` | `sub_4308C0(400, 8, 0)` | delegated，实参固定 |
| `0x004126CC..0x004126E8` | 状态恰为 1 时 `sub_413FE0(left, top, 2)` | delegated，门和实参已固定 |
| `0x004126F0` | 第二次 `AIL_serve` | audio port 原槽调用 |
| `0x004126FF..0x00412716` | 普通世界唯一一次 `Blt` | world presentation port |
| `0x00412719..0x0041287C` | 玩家格指针、transition、快照与动作校验 | 见第 2 节 |
| `0x0041287F..0x004128DA` | tile 层折返动画和 layer offset | 已接入真实状态机 |
| `0x004128DF..0x0041291D` | 条件恢复选择滚动前视口 | 已接入真实状态机 |

`run_legacy_world_frame` 因此不是第二套 renderer。它只拥有外层顺序，并在原始
`0x004126B8` 槽调用已有 `compose_legacy_world_runtime_frame`；呈现没有被挪到通用帧尾。

## 2. 三个不能合并的门

### 2.1 队伍角色数量

`0x004124DC` 把 `EBP` 设为 1，随后用无符号 `JBE` 判断队伍角色数。数量为 0 或 1 都
直接跳到 `0x0041268C`，只有 `count > 1` 才进入 `0x004124EF` 循环。coordinator 不会
为了统一 stage 表而无条件调用该循环。

### 2.2 条件地图标记

`0x004126D4` 是 `CMP EAX, EBP` 后的 `JNZ`，所以只有状态**恰好等于 1**才调用标记；
状态 2 不能按 truthy 处理。传入实参保留当时临时滚动后的 camera left/top 和常量 2。

### 2.3 呈现后的玩家对齐

`0x00412719` 与 `0x00412726` 分别检查玩家 X/Y 的低四位。任一未对齐便直接跳到
`0x0041283C`，因此原程序不会清除四个 movement transition，也不会更新格指针和玩家
快照。两轴都对齐时的顺序才是：

```text
0041272E..0041277D  更新 tile/action 指针（delegated）
00412788..0041279A  清零 camera/player 的 X/Y transition（已实现）
004127A0..00412839  条件复制三个 0x7C 快照并更新动作 flags（delegated）
0041283C..0041287C  无条件动作校验（delegated）
```

因此实现把帧后逻辑拆成三个 stage；不能用一个“post present”回调把 transition 清零的
相对位置藏起来。

## 3. 共享状态与现代边界

- 玩家、角色 span、相机矩形、frame runtime、raster、framebuffer、row jitter 和 tile
  layer state 在同一次调用中共享；选择滚动后的临时相机直接传给背景与角色组合。
- 帧尾恢复后再把 camera left/top 同步回 frame runtime，下一帧不会遗留临时滚动值。
- 当前 tile layer offset 在组合前复制进 background source；动画只在呈现和玩家帧后
  账本之后推进，供下一帧使用。
- 原程序假定玩家索引和 64-word 选择表永远有效。OpenSWD3 只在现代 span 所有权无效时
  于首次访问前返回；有效输入的调用、修改与门控顺序不变。
- delegated stage 返回失败时停在对应原槽并报告失败，不把尚未恢复的行为伪装成完整帧。

## 4. 验证边界

组合 UT 已固定完整 outer/inner/audio/presentation 事件序列，并覆盖：

- 玩家与相机位移后，选择滚动使用同一临时相机完成 `0x00412930`；
- 固定 UI 和地图标记的实参；
- company count `0/1` 跳过、marker state `2` 跳过；
- 玩家对齐时清 transition、未对齐时保留 transition；
- 无效玩家索引、缺失选择 Y word、outer stage 失败和 composition 失败的原槽停止。

Linux Clang `core` 133/133、Windows LLVM `app` 137/137 CTest 通过。SDL app 目前仍未
提供真实地图 session、角色/动作其余 stage 和 coordinator ports，因此本页不写
`sdl_runtime_integrated`。需要原程序动态差分时，只准备 Frida spawn 工具并等待用户执行，
不由开发流程启动原版。
