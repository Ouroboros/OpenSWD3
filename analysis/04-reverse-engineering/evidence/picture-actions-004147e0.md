# 图片动作链表（`0x004147E0..0x004148ED`）

状态：`assembly_exact`、`unit_verified`、`world_runtime_integrated`；尚未
`original_diff_verified`

本文只以 `swd3.exe.lst` 的机器码和指令为行为真值。IDA 伪码与符号只用于定位。

## 1. 节点和所有权

剧情 opcode 58/153 在 `0x0042B1F1..0x0042B282` 分配并清零 `0xA4` 字节，随后对
`node + 0x08` 初始化通用 `0x98` 字节动作记录。四个脚本参数分别写入节点 `+0x00`、
`+0x02`、动作 `+0x00` 和动作 `+0x08`；节点按 `+0xA0` 前插到两条链之一。

```text
+0x00  u16 screen_x
+0x02  u16 screen_y
+0x04  u16 zero/reserved
+0x06  u16 zero/reserved
+0x08  LegacyActionRecord[0x98]
+0xA0  legacy 32-bit next
```

现代 `LegacyPictureActionNode` 仍固定为 `0xA4`，并用 `static_assert` 固定动作记录与旧
next 槽的偏移。主机链指针由 `std::list` 管理，旧 `+0xA0` 槽仍保留在元素载荷中，
不能因 64 位主机指针宽度而挤压或移动动作字段。

两条链的场景意图仍由 `story_scene` 拥有；世界帧 runtime 只通过端口借用并逐帧更新、
绘制和退休节点，没有把它们改成 world-map 全局状态。

## 2. 单节点执行顺序

`0x004147F7..0x004148DC` 对每个节点严格执行：

1. 把 `node + 0x08` 传给 `sub_4321E0` 更新动作；返回零只调用原诊断空函数，随后仍继续；
2. 用动作 `+0x4A/+0x4C` 请求 TSW 帧；
3. 从动作 `+0x8A` 取 opacity byte，从 `+0x18` 取 flags；
4. 将节点两个 `u16` 坐标零扩展，再分别减去动作 `+0x10/+0x14` 的 32 位绘制偏移；
5. 如果动作 `+0x58` 非零，以 `screen + camera` 的 32 位回绕坐标播放空间音效，随后
   把该 word 清零；
6. 只有动作 `+0x8C` **恰好等于 1** 才按 `+0xA0` 摘链并释放；其他值继续保留。

更新失败不能复用会提前返回的 `sub_40EBF0` 现代桥语义。独立 owner 因此直接调用动作
更新、帧请求和绘制端口，保留 `sub_4147E0` 独有的“诊断后仍绘制”顺序。

## 3. 帧内调用位置

普通世界 normal 路径在 `0x00412A92` 以 `unk_4B7BD0` 调用主链，在
`0x00412AF2` 以 `unk_4B88C8` 调用副链。clear-only 路径在 `0x00412B12` 仍无条件
执行副链。因此 `LegacyWorldFrameStage` 中主、副两个槽都接到同一个 owner，但借用不同
链表；clear-only 不会错误执行主链。

## 4. 验证边界

独立 UT 固定 `0xA4/+0x08/+0xA0` 物理布局、更新失败仍请求并绘制、帧请求失败后的音效
和摘链、音效 word 单次清零、坐标零扩展/回绕、blit 诊断，以及完成值必须精确等一。
帧 runtime UT 另固定十九个原 stage 中主/副图片动作均在原槽执行，generic delegation
由十七项降为十五项；clear-only 的原顺序由 composition UT 保持。

Linux Clang `core` 155/155、Windows LLVM `app` 159/159 CTest 通过；Windows app 已
成功链接，未启动新版或原版 EXE。

原程序动态 framebuffer 与音效调用差分仍为 `blocked_runtime_oracle`。需要时只准备
Frida spawn 工具并等待用户执行，不由开发流程启动原版。
