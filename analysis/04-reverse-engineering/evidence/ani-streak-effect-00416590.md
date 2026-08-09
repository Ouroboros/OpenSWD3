# ANI 下落拖尾效果（0x00416590）

最后更新：2026-08-10

唯一行为真值：`swd3.exe.lst`。槽布局、RNG 调用顺序、有符号比较、像素访问
和计数异常均以 LST 指令为准，IDA 伪码只用于导航。

## 1. 物理状态

`0x0040C1FB..0x0040C22D` 从 `0x004B7520` 开始清零 `0xC0` 个 dword，即
48 个 16 字节槽；`0x00416630..0x00416795` 却以 16 字节步长遍历到
`0x004B7926`，总计 64 个槽。因此重置只清前 48 槽，不能为了整齐而把后
16 槽一并清掉。进程首次建立的 C++ 存储仍按 loader 初态全零填充。

每槽全部是 `i16`：

| 偏移 | 实现字段 | 用途 |
|---:|---|---|
| `+0x00` | `fixed_x` | 1/16 像素 x |
| `+0x02` | `fixed_y` | 行坐标 |
| `+0x04` | `horizontal_step` | 拖尾每点水平步进 |
| `+0x06` | `vertical_step` | 每帧纵向步进 |
| `+0x08` | `trail_limit` | 包含上界的拖尾点索引 |
| `+0x0A` | `remaining_frames` | 寿命，并决定亮度步长 |
| `+0x0C` | `field_c` | 创建时清零，本函数不读 |
| `+0x0E` | `active_flags` | 仅 bit 0 用于活动判定 |

另有上一帧存活计数 `0x004B88BC` 和每帧创建目标 `0x004AAEC8`，均为
`i16`。

## 2. 概率门与创建

每次调用首先无条件消耗 `0x00439070(1000)`。只有结果严格大于 900 时
才查询 `0x0040DC50(8)`：

- service 非零：目标按 `i16` 加一，大于 8 时夹到 8；
- service 为零：目标按 `i16` 减一，结果为负时夹到零。

概率结果未命中时不得预先查询 service。目标和上帧存活计数同时为零便
立即返回，但首次 RNG 已经消耗。

进入槽扫描后先把存活计数清零。每遇到一个 bit 0 为零的槽，只要本帧
创建数仍小于目标，就严格按以下顺序消耗第二套 RNG：

```text
fixed_x        = random(640) * 16
fixed_y        = 0
horizontal     = random(3); fixed_x > 320*16 时取负
vertical_step  = random(30) + 16
trail_limit    = random(16) + 8
remaining      = vertical_step > 15 ? 32 : 16
field_c        = 0
active_flags   = 1
```

新槽在创建帧不进入绘制，也不加入该帧存活计数。

## 3. 拖尾绘制

活动槽从 `fixed_y*pitch` 定位行指针，拖尾循环是 `0..trail_limit` 的
包含上界。只有行指针严格大于 framebuffer 基址且严格小于
`base+0x96000` 才调色，所以第 0 行被故意跳过。

对每个可绘制点，原函数先调用结果未被使用的 `0x004239D0(i,i,i)`，再以
`0x00420490(pixel,1,i,i,i)` 对一个 16 位像素做三通道饱和加。像素 x 为
当前 fixed x 除以 16 并向零截断。每点后 fixed x 加水平步进、行指针加
pitch；亮度 `i` 通常加一，但寿命精确等于 32 时再额外加一。

行指针达到或越过底边会立即把 active word 清零，但拖尾循环仍然继续。

## 4. 帧尾状态异常

拖尾后使用 16 位乘加/加法更新：

```text
fixed_x += low16(horizontal_step * vertical_step)
fixed_y += vertical_step
remaining_frames--
```

只有寿命减到精确零时才清 active；否则无条件增加存活计数。因此槽即使
已因越过底边而清除 active，只要寿命仍非零，也会在本帧把存活计数
加一。这个计数与 active word 不一致的行为必须保留。

## 5. 实现与验证

- `LegacyAniStreakSlot` 固定为 `0x10` 字节；`reset` 只清前 48/64 槽。
- 合成 UT 锁定未命中不查 service、命中时的 service 8 增减、四个创建 RNG
  顺序、右半区水平取负、新槽当帧不绘制、第 0 行跳过、包含上界、
  `i16` 乘加回绕以及越底边后的存活计数异常。
- 种子 39 的首个概率值为 953，首槽创建向量为
  `(fixed_x=5712, horizontal=0, vertical=22, trail=23, lifetime=32)`；
  第二帧绘制 23 个像素，固定 framebuffer FNV-1a 64 为
  `0x7403975F3AB69BDD`。

原版正常 framebuffer 固定为 `0x96000` 字节。现代实现只在存储不足或
计算后像素指针无法提供 `0x00420490` 需要的额外一个可读 `u16` 时增加
确定的内存边界，有效状态路径不变。

Linux Clang `core` 为 `93/93`，Windows LLVM `app` 为 `97/97` CTest，全套通过。
当前证据状态为 `assembly_exact`；原程序像素差分仍为 `blocked_runtime_oracle`。
需要时由 Codex 准备 Frida spawn 一键工具并等待用户执行，OpenSWD3 不自行
启动原 EXE。
