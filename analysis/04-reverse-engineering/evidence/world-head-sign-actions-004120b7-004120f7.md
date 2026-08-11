# 世界 HeadSgn 动作记录（`0x004120B7..0x004120F7`）

状态：`assembly_exact`、`unit_verified`、`asset_verified`、
`sdl_runtime_integrated`；尚未 `original_diff_verified`

本文只以 `swd3.exe.lst` 的机器码与指令为行为真值。`HeadSgn` 来自原程序诊断字符串，
只作为这组记录的稳定标签，不据此推断尚无汇编证据的视觉用途。

## 1. 物理所有权与初始化

`sub_40E0B0` 在 `0x0040E463..0x0040E497` 清零从 `0x004B9F68` 开始的
`0x130` 个双字，即连续 `0x4C0` 字节；随后以 `0x98` 为步长，对
`[0x004B9F68, 0x004BA428)` 的每个元素调用 `sub_40DC00`。因此这不是若干无关全局量，
而是恰好八个公共 `LegacyActionRecord`：

| 槽 | 原地址 | 初始 action id | 初始 base variant |
|---:|---:|---:|---:|
| 0 | `0x004B9F68` | `0x232E` | 0 |
| 1 | `0x004BA000` | `0x232E` | 1 |
| 2 | `0x004BA098` | `0x232E` | 2 |
| 3 | `0x004BA130` | `0x232E` | 3 |
| 4..7 | `0x004BA1C8..0x004BA390` | 0 | 0 |

前四槽赋值由 `0x0040E499..0x0040E4DB` 逐条确认，其余槽保持清零值。OpenSWD3 的
`LegacyWorldHeadSignActionsState` 复现“整块清零 → 八次公共动作初始化 → 前四槽赋值”的
顺序，并由世界帧状态拥有存储；ACT 模块只借用记录并负责 `sub_4321E0` 的更新语义。

## 2. 每帧遍历

`sub_4120B0` 在世界帧最前执行以下固定流程：

1. `0x004120B7` 从最高槽地址 `0x004BA390` 开始；
2. `0x004120BC` 检查记录 `+0x00` 的 action id，为零便跳过；
3. 非零时把记录地址传给 `sub_4321E0`；
4. `0x004120EB` 每次减去 `0x98`；
5. `0x004120F1..0x004120F7` 在地址仍不低于 `0x004B9F68` 时继续。

初始状态的实际调用顺序因此严格为变体 `3 → 2 → 1 → 0`。循环仍访问全部八槽，但只对
非零 action id 调用更新器；不能改成只存四个活动记录，也不能正序遍历。

## 3. 失败不是整帧失败

`0x004120CA` 只在 `sub_4321E0` 返回零时读取 action id、base variant 和
variant delta，随后调用 `nullsub_1`。无论返回值如何，控制流最终都到
`0x004120EB`，继续下一个记录。OpenSWD3 因此累计失败数供现代日志/诊断使用，但不停止
其余记录，也不把失败升级为世界帧失败；这保留了原程序的可观察控制流。

## 4. 接线与验证

- `run_legacy_world_frame` 在玩家/相机移动前直接执行该 owner，不再转交
  `head_sign_actions_004120b7` 空 stage；
- 独立 UT 固定八槽初始化、四个固定 action/variant、倒序调用、零槽跳过和失败后继续；
- coordinator UT 固定四次更新位于所有其他世界帧事件之前，并证明单槽失败后整帧仍完成；
- 真实 `MAPS + LMF + ACT + TSW` 初始世界测试通过，动作 `0x232E` 的四个变体均由真实
  ACT runtime 成功推进；
- Linux LLVM `core`：148/148 CTest 通过；
- Windows LLVM `app`：152/152 CTest 通过。

当前仍未由原程序动态捕获对比动作记录逐帧状态，因此不写
`original_diff_verified`。如需动态验证，只准备捕获工具并等待用户运行原程序。
