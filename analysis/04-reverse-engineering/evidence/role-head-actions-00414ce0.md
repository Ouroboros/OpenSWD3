# 角色头顶动作链（`0x00414CE0..0x00414E40`）

状态：`assembly_exact`、`asset_verified`、`world_runtime_integrated`；尚未
`original_diff_verified`

本文只以 `swd3.exe.lst` 的机器码和指令为行为真值。它替代早期不保留完整动作记录和
节点偏移的中性精灵视图；同为 `0xB4` 只能证明物理宽度相同，不能把本链与
`0x00414B60` 世界移动动作链声明为同一业务类型。

## 1. 物理节点与创建

opcode 81 的 `0x0042A200..0x0042A2A6` 分配并清零 `0xB4` 字节，对节点起点初始化
完整 `0x98` 字节动作记录，再按 `+0xB0` 前插到 `[0x004BA6E0]`：

```text
+0x00  LegacyActionRecord[0x98]
+0x98  i16 current_x
+0x9A  i16 horizontal_motion
+0x9C  i16 target_x
+0x9E  i16 y
+0xA0  16 bytes reserved/zero in this producer and consumer
+0xB0  legacy 32-bit next
```

脚本 `+2/+4` 两个 `u16` 分别零扩展写入动作 `+0x00/+0x08`。脚本 `+6` 原始 word
写入 `target_x`；脚本 `+8` 的低 15 位写入 `y`。普通模式把 motion 清零，并按有符号
`target_x <= 320` 选择从 `-120` 入场，否则从 `760` 入场。脚本 `+8` bit 15 非零时，
改为令 `current_x = target_x`、`horizontal_motion = 0x8000`。

`LegacyRoleHeadActionNode` 用 `static_assert` 固定全部已使用偏移、保留段和总宽度。现代
`std::list` 管理主机链接，但元素仍保留旧 `+0xB0` 槽；不会在 64 位构建中把主机指针
塞入原始载荷。

## 2. 两条脚本变更路径

opcode 82 的 `0x0042A2C6..0x0042A33C` 只处理第一个同时匹配动作 `+0x00/+0x08`
的节点：

- motion bit 15 已置位：把 motion 写成 `10000`；
- 否则先写 `-1`，若当前有符号 `current_x > 320` 再改写为 `+1`。

未找到匹配项时静默消费指令。opcode 86 的 `0x0042A673..0x0042A6C6` 同样只处理
第一个匹配项，把新的两个 `u16` 零扩展写回动作 `+0x00/+0x08`；它不重启动作记录，
也不改变坐标和 motion。

## 3. 单节点帧顺序

`0x00414CF6..0x00414E32` 对每个节点严格执行：

1. 把节点起点传给 `sub_4321E0`；失败只触发空诊断，仍继续；
2. 用更新后的动作 `+0x4A/+0x4C` 取帧，以 palette=null 的 16 位直接源绘制；
3. 绘制点为 `signed(current_x)-action.draw_offset_x`、
   `signed(y)-action.draw_offset_y`，flags 取动作 `+0x18`，opacity 取 byte `+0x8A`；
4. 绘制不减相机坐标，也没有可见性或动作 `+0x44` hold 门；
5. 最后才更新横坐标，并按结果决定保留或摘链。

motion 的低 15 位为零时，`0x0000` 与 `0x8000` 都走趋近分支：

```text
step = trunc_toward_zero(2 * (target_x - current_x) / 3)
current_x = low16(current_x + step)
if -1 <= step <= 1: current_x = target_x
```

该分支永不删除节点。其余 motion 走飞出分支：先以 16 位回绕执行
`current_x += motion`，再令 `motion = low16(motion * 3)`；更新后的有符号
`current_x <= -120` 或 `current_x >= 760` 时立即摘链释放。两个边界包含等号，而且节点
始终用运动前坐标绘制一次。

## 4. 帧槽、所有权与验证

`0x00412930` 三条主体路径会合并恢复全屏 clip 后，严格执行
`0x00414E50 → 0x0042ED40 → 0x00414CE0`。`LegacyWorldFrameRuntimePorts` 现在借入
独立 `LegacyRoleHeadActionList` 并在该原槽执行；generic inner delegation 从十四项降为
十三项。剧情意图和 opcode 81/82/86 的最终 VM 接线仍归 `story_scene`，世界 runtime
只借用列表完成每帧更新、绘制和退休。

独立 UT 固定 `0xB4/+0x98..+0xB0` 布局、更新失败后继续、更新后动作字段、运动前绘制、
`0x8000` 分支、`[-1,1]` snap、16 位乘三回绕和包含等号的删除边界。世界 runtime UT
固定十九个原 stage 中本槽不再进入 generic delegation；真实 TSW resource 1 / frame 0
同时通过 bit-29 角色、普通角色、moving action 与本链，叠加底图后的逻辑 framebuffer
FNV-1a64 为 `0x3EAF7C3143994E65`。

Linux Clang `core` 为 157/157、Windows LLVM `app` 为 161/161 CTest；Windows app
成功链接，本轮没有启动新版或原版 EXE。

原程序逐帧 framebuffer 与节点状态差分仍为 `blocked_runtime_oracle`。需要动态验证时只
准备 Frida spawn 工具并等待用户执行，不由开发流程启动原版。
