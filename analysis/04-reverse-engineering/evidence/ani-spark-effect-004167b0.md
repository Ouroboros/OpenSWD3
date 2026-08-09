# ANI 九点星芒效果（0x004167B0）

最后更新：2026-08-10

唯一行为真值：`swd3.exe.lst`。槽数、计数器重置、RNG 顺序、相位表、硬编码
framebuffer 边界和九次像素调用均以 LST 指令为准，IDA 伪码只用于导航。

## 1. 物理状态与残留槽

`0x00416867` 从活动字段 `0x004C8C8E` 开始，以 `0x10` 字节步长扫描；递增后的
活动字段地址达到 `0x004C928E` 时停止。因此槽存储从 `0x004C8C80` 开始，共有
96 个 16 字节槽。每槽全部是 `i16`：

| 偏移 | 实现字段 | 用途 |
|---:|---|---|
| `+0x00` | `fixed_x` | 1/16 像素 x |
| `+0x02` | `fixed_y` | 行坐标 |
| `+0x04` | `horizontal_step` | 点内及帧间水平步进 |
| `+0x06` | `vertical_step` | 帧间垂直步进 |
| `+0x08` | `point_count` | 本槽本帧绘制点数，创建时固定为一 |
| `+0x0A` | `remaining_height` | 亮度来源及结束条件 |
| `+0x0C` | `phase` | 八段水平摆动表的相位 |
| `+0x0E` | `active_flags` | 只以低字节 bit 0 判定活动 |

上一帧存活计数位于 `0x004B737C`，每帧创建目标位于 `0x004BAB80`。场景初始化
`0x0040C234/0x0040C24A` 只清这两个 `i16` 计数器；没有清除 `0x004C8C80`
的 96 槽池。进程首次建立时槽池依赖 loader 零填充，以后的场景重置会保留槽内容。
实现的 `reset_counters` 因此不得顺手清槽。

## 2. 概率门和创建顺序

每次调用先无条件执行 `0x00439070(1000)`。结果严格大于 900 时才查询
`0x0040DC50(0x16)`：

- service 非零时，创建目标按 `i16` 加一；有符号结果大于一才夹到一；
- service 为零时，创建目标按 `i16` 减一；负数夹到零。

目标和上一帧存活计数同时为零时，在首次 RNG 之后立即返回。进入扫描后先清存活
计数。每帧至多按照目标数创建新槽；目标最大为一，所以正常路径每帧至多创建一个。
创建严格消耗四个后续 RNG：

```text
fixed_x         = random(640) * 16
fixed_y         = 0
horizontal      = random(3); fixed_x > 320*16 时取负
vertical_step   = random(3) + 1
point_count     = 1
remaining       = vertical_step * 160 - random(160)
phase           = 0
active_flags    = 1
```

新槽在创建帧直接进入下一槽，不绘制也不增加存活计数。种子 39 的首次概率值为
953，首槽向量为 `(fixed_x=5712, horizontal=0, vertical=1,
point_count=1, remaining=97)`，总共消耗十个 RNG 原始 word。

## 3. 相位、亮度和九点核

函数栈上建立八字节有符号相位表：

```text
0, 1, 1, 1, 0, -1, -1, -1
```

槽相位以向零截断的 `phase / 4` 索引该表。正常相位为 `0..28`。初始亮度为：

```text
31 - trunc_toward_zero((480 - remaining_height) / 6)
```

随后以无符号比较判断亮度是否大于 1000；负值也会落入该分支并被改成零。

行地址只允许严格位于 `base+0x500` 和 `base+0x95B00` 之间。原版 1280 字节
pitch 下等价于只绘制第 2..478 行，为上下邻点各保留一行。可绘制点先调用一次
结果未使用的 `0x004239D0(i,i,i)`，再按下列顺序调用九次
`0x00420490(pixel,1,delta,delta,delta)`：

```text
中心：i
右、左、下、上：i >> 1
右下、左下、右上、左上：i >> 2
```

x 只做向零截断的 `fixed_x / 16` 并加相位偏移；没有水平裁剪。负 x 或超过 639
的 x 会按平坦 framebuffer 地址跨到相邻扫描线，这一旧行为不能改成逐行裁剪。
每点后工作 x 加水平步进、行地址加 pitch、亮度加一；当
`remaining_height == 32` 时亮度再额外加一。

## 4. 帧尾与原始异常

绘制完成后按 16 位低字行为更新：

```text
fixed_x += low16(horizontal_step * vertical_step)
fixed_y += vertical_step
phase++ ; signed phase > 28 时归零
remaining_height -= vertical_step
```

`remaining_height <= 0` 时清整个 active word，否则增加存活计数。行地址达到或越过
`base+0x95B00` 时也会清 active，但函数仍继续帧尾更新；只要减法后的
`remaining_height > 0`，该槽仍会增加本帧存活计数。这会产生 active 已清零而
live 非零的一帧不一致，必须保留。

## 5. 实现与验证

- `LegacyAniSparkSlot` 固定为 `0x10` 字节，状态固定为 96 槽；
- 合成 UT 锁定 loader 初态、只清计数器、service 22 条件门、四个创建 RNG、
  新槽当帧跳过、相位表、亮度、九点调用、横向跨行、479 行清除、存活计数异常和
  `i16` 回绕；
- 固定绘制向量使用 `fixed_x=1600, fixed_y=100, phase=4,
  remaining_height=480`，RGB555 中心/邻点/对角分别为
  `0x7FFF/0x3DEF/0x1CE7`，framebuffer FNV-1a 64 为
  `0xF7080E84910EFC5B`。

现代实现只在 framebuffer 不足 `0x96000` 字节、计算后的像素位置无法提供
`0x00420490` 所需的额外一个可读 `u16`，或内部相位不再位于正常表域时增加确定的
内存安全边界；正常状态路径不变。

Linux Clang `core` 为 `94/94`、Windows LLVM `app` 为 `98/98` CTest，全套通过。
当前证据状态为 `assembly_exact`；原程序像素差分仍为 `blocked_runtime_oracle`。
需要时由 Codex 准备 Frida spawn 一键工具并等待用户执行，OpenSWD3 不自行启动
原 EXE。
