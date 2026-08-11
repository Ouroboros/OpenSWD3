# 普通世界脚本相机平移（`0x00414570`）

状态：`assembly_exact`、`unit_verified`、`sdl_runtime_integrated`；尚未
`original_diff_verified`

本文只以 `swd3.exe.lst` 的机器码和指令为行为真值。`sub_414570` 的完整范围是
`0x00414570..0x004145EF`，无被调函数，唯一直接调用点是普通世界主帧
`0x0041268C`。

## 1. 状态映射

| 原地址 | OpenSWD3 字段 | 含义 |
|---|---|---|
| `0x004A99F0` | `remaining_x` | X 轴尚未完成的脚本位移 |
| `0x004A98C0` | `remaining_y` | Y 轴尚未完成的脚本位移 |
| `0x004A9480` | `step_x` | 每帧 X 位移 |
| `0x004A93D8` | `step_y` | 每帧 Y 位移 |
| `0x004AB980/+4/+8/+C` | `camera.left/top/right/bottom` | 当前世界视口矩形 |

`0x00429066..0x00429342` 的剧情命令分支负责建立 remaining/step，
`0x00429362..0x00429390` 则等待四个字段全部归零。当前单元只拥有逐帧消费者；剧情
解释器的生产和等待分支仍按 B7 后续顺序恢复，不在这里提前扩张。

## 2. 逐指令行为

入口只检查两个 remaining 字段。两者均为零时直接返回，连非零的 dormant step 也不
清理。任一 remaining 非零时进入一个共享更新体：

```text
camera.left   += bits(step_x)
camera.right  += bits(step_x)
camera.top    += bits(step_y)
camera.bottom += bits(step_y)
remaining_x   = wrapping_sub(remaining_x, step_x)
remaining_y   = wrapping_sub(remaining_y, step_y)
if remaining_x == 0: step_x = 0
if remaining_y == 0: step_y = 0
```

四条相机加法和两条 remaining 减法全部按 x86 32 位回绕实现，不借用会触发 C++ 有符号
溢出未定义行为的普通 `i32` 算术。步长只在减法结果**恰好**为零时清除；本函数不修正
越过零点或不可整除的脚本参数。

共享更新体也没有独立的轴门：如果 Y remaining 非零而 X remaining 已为零，非零的
X step 仍会移动视口并把 X remaining 减成负值。这在正常生产者合同下不应出现，但
属于汇编可观察行为，UT 明确保留，不能按更合理的逐轴写法改掉。

## 3. 主帧接线

`LegacyWorldFrameCoordinatorState::camera_pan` 持有四个字段。
`run_legacy_world_frame` 在队伍角色循环结束后调用
`advance_legacy_world_camera_pan`，随后才执行 `sub_4148F0` 对应的临时选择滚动。因此
玩家/相机 transition、脚本相机平移、选择序列滚动依次作用在同一个 camera 上；组合
和可选开发调试叠层读取三者叠加后的坐标，帧尾只撤销选择序列的临时增量，不撤销脚本
平移。

原 `precompose_00414570` 外部占位和枚举项已经删除。紧随世界组合后的
`sub_4308C0` 也已复用既有 countdown owner；条件 `sub_413FE0` 现已由完整开发调试
叠层 owner 接管，outer stage 不再保留 generic 占位。

## 4. 验证

独立 UT 固定：

- 两个 remaining 都为零时，camera、remaining 和 dormant step 全部不变；
- 正负步长同时移动四条视口边，并只清除恰好完成的轴；
- 一轴激活时，另一非规范零 remaining/非零 step 仍进入共享更新体；
- camera 加法及 `INT32_MIN - 1` 按模 `2^32` 回绕。

coordinator UT 另以 active pan 固定物理顺序：玩家 transition 后 camera 为
`(14,16)`，脚本平移后为 `(16,15)`，选择滚动后的组合坐标为 `(19,13)`，帧尾恢复到
`(16,15)`。Linux LLVM `core` 154/154、Windows LLVM `app` 158/158 CTest 通过。
原程序动态差分若需要，只准备 Frida spawn 工具并等待用户执行，不由开发流程启动原版。
