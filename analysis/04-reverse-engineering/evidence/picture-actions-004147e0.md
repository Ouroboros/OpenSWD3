# 图片动作链表（`0x004147E0..0x004148ED`）

状态：`assembly_exact`、`unit_verified`、`platform_adapted`、
`sdl_runtime_integrated`；尚未 `original_diff_verified`

本文只以 `swd3.exe.lst` 的机器码和指令为行为真值。IDA 伪码与符号只用于定位。
`sub_4147E0` 的完整物理范围是 `0x004147E0..0x004148ED`；三个调用点
`0x00412A97/0x00412AF7/0x00412B17` 都传入一个父对象指针。后两处立即清四字节，首处把
该参数留到后续 service 参数一起清理，因此仍是一参数 cdecl。函数保存 `EBX/ESI/EDI`，
plain `retn`；没有统一 `EAX` 合同，三个调用点均忽略返回值。

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
2. 更新 callback 后重读动作 `+0x4A/+0x4C` 请求 TSW 帧；两个 dword 参数的高 16 位沿用
   寄存器残值，但 `sub_4315D0` 的 cache/physical callee 在使用前截为低 16 位；
3. frame callback 返回后立即在 `0x00414843` 解引用帧源；成功时才继续重读动作
   `+0x8A` opacity byte、`+0x18` flags、坐标与 offset；
4. 将节点两个 `u16` 坐标零扩展，再分别减去动作 `+0x10/+0x14` 的 32 位绘制偏移；
5. 如果动作 `+0x58` 非零，以 `screen + camera` 的 32 位回绕坐标播放空间音效，随后
   把该 word 清零；
6. 只有动作 `+0x8C` **恰好等于 1** 才按 `+0xA0` 摘链并释放；其他值继续保留。

更新失败不能复用会提前返回的 `sub_40EBF0` 现代桥语义。独立 owner 因此直接调用动作
更新、帧请求和绘制端口，保留 `sub_4147E0` 独有的“诊断后仍绘制”顺序。

复核发现旧现代实现把 frame miss 当可继续诊断：它仍播放音效、摘除当前节点并扫描后续
节点。原汇编会在 load 返回后的第一条 `[eax]` 读取停止；现改为 `frame_load_failed`，只保留
此前的动作更新与帧请求副作用，并让 frame runtime 在主/副对应 stage 停止。音效、清 sound
word、完成值读取、摘链和后续节点均不再越过原 unsafe point。

## 3. 帧内调用位置

普通世界 normal 路径在 `0x00412A92` 以 `unk_4B7BD0` 调用主链，在
`0x00412AF2` 以 `unk_4B88C8` 调用副链。clear-only 路径在 `0x00412B12` 仍无条件
执行副链。因此 `LegacyWorldFrameStage` 中主、副两个槽都接到同一个 owner，但借用不同
链表；clear-only 不会错误执行主链。

## 4. 验证边界

原裸父对象/节点/帧指针、手工摘链/free 和 blitter globals 改由受检 `std::list` owner、typed
frame piece 与 draw/audio ports 承担；有效节点的遍历、reload 和调用顺序不变，因此 closure
disposition 为 `platform_adapted`。

独立 UT 固定 `0xA4/+0x08/+0xA0` 物理布局、更新失败仍请求并绘制、frame miss 的原解引用
停止点、音效 word 单次清零、坐标零扩展/回绕、blit 诊断、完成值精确等一，以及 update→
frame→draw→audio callback 之间的字段重读。frame runtime UT 另固定主链 frame miss 在原
stage 停止，音效/摘链/后续 stage 不执行；主副 normal/clear-only 顺序由 composition/runtime
synthetic/real 保持。五项定向 CTest 通过；Linux core `185/185`、Linux app `191/191`、
Windows LLVM app `191/191` 完整门禁通过，两端应用成功链接且未启动新版或原版 EXE。

原程序动态 framebuffer 与音效调用差分仍为
`blocked_runtime_oracle`。
