# 世界软件鼠标与右边条（`0x004149B0`）

状态：`assembly_exact`、`unit_verified`、`asset_verified`、`platform_adapted`、
`world_runtime_integrated`；尚未 `original_diff_verified`

来源：`swd3.exe.lst` 完整汇编。机器码与指令是唯一行为真值；IDA 名称只用于定位。
`sub_4149B0` 的完整物理范围是 `0x004149B0..0x00414B58`，无栈参数、不修改需保存寄存器，
以 plain `retn` 返回。三个调用者均忽略 `EAX`；正常出口保留最后一次 `sub_4170E0` blit
结果，但不存在调用方可见的统一返回合同。

## 1. 所有者与调用位置

`sub_4149B0` 由普通世界 `0x00412930`、标准特殊模式 `0x0043A610` 和商店
`0x0044EA60` 共用。普通世界已接入真实 frame runtime；特殊模式与商店仍由各自模块恢复，
只共享本函数 owner，不在 B7 复制状态机。函数自身负责两个固定动作记录：

- `0x004ACD18`：主软件鼠标，动作号 `0x2329`、初始变体 0；
- `0x004C83A8`：右边条，动作号 `0x2329`、初始变体 8。

`0x0040E2F5..0x0040E366` 先清零并初始化主记录，再立即调用一次
`sub_4321E0`；失败只进入诊断分支。`0x0040E4A3..0x0040E516` 初始化右边条记录和
两个独立状态：`edge_x = 2`、`idle_frames = 0`，但不预先更新右边条动作。

现代 owner 是 `LegacyWorldCursorState`。普通世界已在原 stage 接线；特殊模式和商店的
调用点以后复用同一实现，不复制状态机。

## 2. Delete 与特殊模式门

`0x004149B0..0x004149CA` 通过 `sub_4372D0(0x2E)` 查询原始 DIK Delete。非零时直接把
主鼠标 `+0x08 base_variant` 写成 15。

随后读取 `0x004B8740`。该特殊模式请求/活动状态非零时，整个右边条维护和绘制路径均
跳过，但主鼠标仍继续更新和绘制。普通世界的现代端口直接借用
`FrameCoordinatorState::battle.special_mode_state`，没有复制成第二份状态。

## 3. 右边条状态机

`0x004149D7..0x00414A87` 同时读取世界玩家 X/Y movement component：

- 任一非零：`edge_x` 以 32 位回绕加一；有符号结果大于零时钳到零；
- 两者都为零：`idle_frames` 以 32 位回绕加一；只有有符号值大于 16 时才令
  `edge_x` 回绕减一，并在小于 `-32` 时钳到 `-32`；
- 空闲计数 `<=16` 只是不继续收起，不是跳过绘制。只要后面的 Talk 门允许，右边条仍
  每帧调用 `sub_40EBF0`；
- 绘制位置固定为 `(edge_x + 642, 0)`，加法保持 IA-32 回绕。

当空闲计数已经大于 16、左键记录 15 的 `rapid_press_multiplicity != 0`、无符号鼠标
`x > 610`、无符号 `y < 24` 且 Talk target 为 `0xFFFF` 时，函数写入
`0x80000001`，请求特殊模式。右边条最终绘制门为：Talk target 等于 `0xFFFF`，或
Talk phase 无符号小于 8。

右边条沿用 `sub_40EBF0` 合同。动作更新失败会在 helper 的诊断分支正常 `retn`，因此本函数
继续处理主鼠标；frame miss 则不同：helper 在 `0x0040EC30 [eax]` 首次解引用即停止，根本
不会返回本函数。旧现代实现错误地忽略 `frame_load_failed` 并继续主鼠标；现返回
`edge_frame_unavailable`，让 world frame runtime 在 `world_indicator_004149b0` stage 停止。

## 4. 主软件鼠标

`0x00414A8F..0x00414B58` 的顺序固定为：

1. 比较 `0x004CC2E0` 保存的旧变体和主记录 `+0x08`；不同则清零
   `+0x44 wait_remaining`，随后保存当前变体；
2. 调用 `sub_4321E0` 更新主记录；返回零只输出 `Act Err(RefrashCursor)` 诊断，仍继续；
3. 以记录 `+0x4A/+0x4C` 调用 `sub_4315D0` 取得 TSW frame；
4. 从 frame `+0x0C/+0x0E` 取无符号宽高，从记录 `+0x10/+0x14` 取绘制偏移，按
   32 位减法回绕得到 `mouse - offset`；
5. 将记录 `+0x18 mode_flags` 原样传给 `sub_4170E0`，并把 `+0x8A` 字节零扩展为
   opacity step。

因此主鼠标不能直接调用“更新失败即停止”的通用 `update_draw_legacy_action`。独立实现
保留其非致命诊断路径；只有现代受检 TSW 查询确实拿不到主 frame 时，才返回明确的
`cursor_frame_unavailable`，避免解引用旧程序中的无效指针。

## 5. 实现与验证

- `include/openswd3/world_map/legacy_world_cursor.hpp`
- `src/world_map/legacy_world_cursor.cpp`
- `tests/unit/world_map/legacy_world_cursor_test.cpp`

UT 覆盖初始化/单次预热、空闲第 17 帧热区请求、移动与 Talk 门、`-32` 钳位、Delete
变体、更新失败后继续、坐标回绕、完整 flags/opacity、主 frame 缺失边界，以及右边条
frame miss 在主记录更新前停止。独立 callback 向量在 frame 请求回调内改写 action 的绘制
偏移/flags/opacity 和鼠标 X/Y，固定原 `load→live globals reload→blit` 顺序。普通世界
runtime 测试另证明右边条 frame miss 报告 cursor stage 失败，且不再更新或绘制主记录。

真实 ACT/TSW 组合帧在包含底图、角色、两条 `0xB4` 动作链和鼠标后，RGB565 逻辑
framebuffer FNV-1a64 为 `0x5889E0547682E179`。Linux Clang `core` 159/159、Linux
Clang `app` 163/163、Windows LLVM `app` 163/163 CTest 均通过，Windows 应用成功
链接且未启动任何 EXE。

原 DIK/鼠标/动作/TSW/blitter globals 与裸 frame 指针改由 frame input、owned action、typed
ports 和受检 frame piece 承担；有效路径的门、回绕、helper 分支、live reload 与绘制顺序
不变，因此 closure disposition 为 `platform_adapted`。Linux core `185/185`、Linux app
`191/191`、Windows LLVM app `191/191` 完整门禁通过，两端应用成功链接且未启动游戏
EXE。尚未取得原程序同帧 framebuffer 差分，因此不是 `original_diff_verified`。
