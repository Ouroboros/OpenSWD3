# 世界移动动作链（`0x00414B60..0x00414CDF`）

状态：`assembly_exact`、`asset_verified`、`world_runtime_integrated`；尚未
`original_diff_verified`

本文只以 `swd3.exe.lst` 的机器码和指令为行为真值。它替代早期只覆盖消费者字段的
简化语义视图；IDA 伪码和既有命名不覆盖节点宽度、字段偏移或执行顺序。

## 1. 物理节点与生产者

opcode 79 的 `0x0042A0A6..0x0042A1CF` 分配并清零 `0xB4` 字节，对节点起点初始化
完整 `0x98` 字节动作记录，再按 `+0xB0` 前插到 `[0x004AD3E8]`：

```text
+0x00  LegacyActionRecord[0x98]
+0x98  i16 start_x
+0x9A  i16 start_y
+0x9C  i16 target_x
+0x9E  i16 target_y
+0xA0  float velocity_x
+0xA4  float velocity_y
+0xA8  float position_x
+0xAC  float position_y
+0xB0  legacy 32-bit next
```

四个脚本坐标先以 16 位左移四位写入 `+0x98..+0x9E`。生产者以 32 位整数计算两轴
差、平方和及 `fsqrt`，再用脚本 `s16 movement` 求两个 float 增量；当前位置由两个起点
转换为 float。零距离仍发生 x87 零除，平方和仍可能先发生 32 位溢出，不能在后续
story VM producer 中改成安全向量算法。

`LegacyMovingActionNode` 用 `static_assert` 固定全部偏移和 `0xB4` 总宽度。现代
`std::list` 管理主机链接，但元素仍保留旧 `+0xB0` 槽；64 位指针不能挤压原载荷。
`0x0042A200` 生产的角色头像链虽同为 `0xB4`，扩展字段语义不同，不复用本类型。

## 2. 单节点执行顺序

`0x00414B78..0x00414CC4` 严格执行：

1. 将移动前 `position_x/position_y` 经 `sub_489654` 向零截断，取结果低 32 位，再以
   32 位回绕减去相机坐标；
2. 把节点起点传给 `sub_4321E0`；失败只触发空诊断，仍继续；
3. 仅在 `-72 < screen_x < 712` 且 `-72 < screen_y < 552` 时，以更新后的动作
   `+0x4A/+0x4C` 取帧，并用 `+0x10/+0x14` 偏移、`+0x18` flags、`+0x8A`
   opacity 绘制；四条边界等号均不可见；
4. 动作 `+0x44` 非零时直接保留，不移动也不退休；
5. 否则分别执行一次 float `position += velocity` 并写回，再向零截断新位置；
6. 只有新位置同时严格进入目标横纵 `±32` 内才立即摘链释放；恰好在四条边界上保留。

因此绘制使用移动前坐标，退休判断使用移动后坐标；屏幕外节点仍更新动作和位置，只有
取帧/绘制被可见性门抑制。更新或现代资源端口失败仅进入结果诊断，不改变其余节点顺序。

## 3. 帧槽与所有权

普通世界 normal 路径在 `0x00412A9C` 调用本函数，位于主图片动作
`0x004147E0` 之后、ANI drift 之前。`LegacyWorldFrameRuntimePorts` 现在借入
`LegacyMovingActionList`，在 `moving_action_sprites_00414b60` 到达时执行真实 owner；
generic inner delegation 从十五项降为十四项。

剧情意图和未来 opcode 79 producer 仍归 `story_scene`；世界 runtime 只借用列表完成
本帧更新、绘制和终结删除。SDL composition root 保存跨帧列表，并在建立新世界 session
时清空；在剧情 VM 接通前不会伪造生产节点。

## 4. 验证

独立 UT 固定 `0xB4/+0x98..+0xB0` 布局、更新失败继续、更新后动作字段、移动前绘制、
严格可见边界、hold 门、float 推进、严格目标窗口以及即时删除。世界 runtime UT 固定
十九个原 stage 中本槽不再进入 generic delegation，并用实际 TSW runtime 绘制
resource 1 / frame 0；叠加底图、bit-29 角色、普通角色和 moving action 后的逻辑
framebuffer FNV-1a64 为 `0x990CD049E2EE092A`。

Linux Clang `core` 为 156/156、Windows LLVM `app` 为 160/160 CTest；Windows app
成功链接，本轮没有启动新版或原版 EXE。

原程序逐帧 framebuffer 与动作状态差分仍为 `blocked_runtime_oracle`。需要动态验证时只
准备 Frida spawn 工具并等待用户执行，不由开发流程启动原版。
