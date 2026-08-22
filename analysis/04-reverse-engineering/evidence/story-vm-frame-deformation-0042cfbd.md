# 剧情 VM framebuffer 变形请求 `0x0042CFBD`

状态：`platform_adapted`、`unit_tested`、`asset_absence_verified`、`external_dependency_tested`、`sdl_runtime_integrated`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x0042CFBD..0x0042D03C`，共享推进尾`0x0042A2AC..0x0042A2C1`

opcode：179 / `OP_179_ENQUEUE_FRAME_DEFORMATION`

公共数值依赖：[`frame-deformation-00430c60.md`](frame-deformation-00430c60.md)

## 1. operand与读取顺序

物理记录固定10字节，四项operand均由`movsx word`符号扩展为i32：

```text
+0 u16 opcode
+2 i16 center_x
+4 i16 center_y
+6 i16 field_radius
+8 i16 strength
```

机器读取顺序不是物理顺序，而是`center_y → center_x → field_radius → strength`。读取全部完成后才分配或发布任何状态。现代按相同阶段检查窗口：缺`+4`时不读`+2`；缺`+6`或`+8`时也不建立节点、不改IP、不发previous。

## 2. 节点构造、注入与发布

handler调用`operator new(0x2C)`并以`sub_430C60`构造实际 framebuffer deformation节点。参数严格为：

```text
framebuffer = 640 × 480
origin      = (center_x - field_radius, center_y - field_radius)
field       = (2 * field_radius) × (2 * field_radius)
```

四项script operand均只经过i16符号扩展；origin减法和半径倍增在该输入域不会溢出i32。传入构造器的field尺寸保留负数的u32位型，不做正规化。

构造返回后，`sub_430FF0`的四个参数固定为：

```text
x        = field_radius
y        = field_radius
radius   = 24
strength = script strength
```

因此`+6`同时决定field半径和注入中心，但注入半径不是script值，永远为24。径向内点使用严格`dx²+dy² < 24²`，中心增量为`24 * strength`，i16工作场加法回绕。非负field radius不消费CRT RNG；负中心的原路径按x后y顺序调用进程CRT RNG，但此类负尺寸已进入原始裸分配/索引危险域。

注入完整结束后才读取哨兵next，把旧head写入新节点`+0x28`，再把新节点发布为`dword_4AC9B8` head。现代直接复用`LegacyWorldFrameEffectState::deformation`的actual `LegacyDeformationList`和进程`LegacyCrtRng`，不建立VM镜像；其world-frame consumer已在原`0x00416CC0`阶段接线。

成功发布后进入共享尾：物理指针和u16 IP固定+10，`ESI=1`，common join发布normalized previous179并在同一VM调用读取后继；不service audio、不yield。

## 3. 平台失败边界

原版对以下边界没有安全合同：

- 44字节节点、双工作场或640×480快照分配失败；
- 零、负、乘法回绕或过大的field geometry；
- 随后由无效尺寸形成的工作场索引和framebuffer采样。

分配返回空时机器仍以null this调用注入；内部buffer分配也无空检查。现代用`unique_ptr/vector`承接所有权：分配异常返回`frame_deformation_allocation_failed`；构造storage不可用或注入检查失败返回`frame_deformation_injection_failed`。两者都销毁未发布临时节点并保留链、IP、previous和CRT RNG。缺actual list/RNG binding返回`runtime_unavailable`。这些隔离只覆盖原内存破坏域，合法geometry的构造、注入、头插和控制流不变，因此分类为`platform_adapted`。

## 4. 资产锁与验证

完整线性TALK目录没有opcode179记录，使用`asset_absence_verified`。全文件双字节候选为：

```text
00B3 = 14
40B3 = 0
80B3 = 0
C0B3 = 12
```

这些候选均位于operand、文本或其他非线性入口字节中，不能冒充真实记录。

synthetic覆盖四raw alias、四项i16符号扩展、640×480固定surface、origin、双倍field、固定注入半径24、signed strength、严格半径边缘、无RNG合法路径、y/x/radius/strength分阶段截断、actual owner缺失、零geometry typed-stop、头插发布、previous、same-call、无audio及完整记录精确窗口尾。frame-deformation与actual world-frame consumer依赖、Story VM synthetic/real/initial-session共5/5通过，SDL app编译通过。Linux core完整门186/186、app完整门192/192通过。未启动原版或OpenSWD3游戏EXE。
