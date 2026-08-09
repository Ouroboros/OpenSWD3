# 10.10 缩放 RLE writer：`0x00422C70` / `0x00423020`

状态：`assembly_exact`；独立 UT 已覆盖；原程序动态差分仍为
`blocked_runtime_oracle`

## 1. 证据与边界

唯一行为依据是完整 LST：

- `sub_422C70`：`0x00422C70..0x00423017`，从左向右写入；
- `sub_423020`：`0x00423020..0x004233C8`，从右向左镜像写入；
- 调用点：`sub_4527E0` 的五次正向调用，以及 `sub_479850` 的一正一反调用。

两条函数都只读取四个栈参数：目标 `x/y` 与源 `width/height`。`sub_4527E0`
若干调用额外压入的第五个值没有被函数读取，不能扩展成新参数。源图来自
`dword_4CD730`，行流从源对象 `+8` 开始；目标来自 `dword_4CD76C`，byte pitch
来自 `dword_4A0E74`。裁剪矩形对应 `0x004CD2F8/0x004CD734` 的 left/top 与
`0x004CD310/0x004CD720` 的 width/height。

四个调用者可变的 10.10 变换状态是：

| 全局 | 初值 | 含义 |
| --- | ---: | --- |
| `0x004A0698` | `40` | 水平锚点 |
| `0x004A069C` | `72` | 垂直锚点 |
| `0x004A06A0` | `0x600` | 水平 10.10 步长 |
| `0x004A06A4` | `0x400` | 垂直 10.10 步长 |

现代接口把这些全局值变成显式 `LegacyScaledRleTransform`，没有改变调用时的
有符号 32 位乘加和朝零截断。

## 2. 坐标与行选择

正向左端点为：

```text
x0 = destination_x + anchor_x - trunc(horizontal_step * anchor_x / 1024)
```

反向右端点为：

```text
x0 = destination_x + anchor_x
   + trunc(horizontal_step * (source_width - anchor_x) / 1024)
```

两者的纵向起点相同：

```text
y0 = destination_y + anchor_y - trunc(vertical_step * anchor_y / 1024)
```

纵向 phase 从零开始。每个源行先加一次 `vertical_step`，每满 `0x400` 产生
一个目标行，因此放大会重复行、缩小会跳过行。顶部预裁剪会完整消费位于 clip
上方的源行；到达首个可见输出时先按原指令归一化余数，再由同一源行进入正常
输出循环。`0x600` 向量锁定了 `2,1,2` 的前三个源行重复数。

反向函数保留一项不能“修正”的原始不对称：它在进入行循环前要求
`scaled_top >= clip_top` 且 `scaled_bottom < clip_bottom`；精灵顶端被裁或底端恰好
等于 clip bottom 都直接返回。正向函数使用通常的相交判断。

## 3. RLE 与水平 phase

行头的低 14 位是该行总 byte 长度；零行头结束图像。每条命令的低 14 位是 run
长度，高两位均为零时后随 `run` 个 `u16` literal pixel，否则是无 payload 的透明
run。只有完整命令字 `0x0000` 才结束行；例如 `0x8000` 是合法的零长度透明命令，
不能误当终止符。

每条命令独立计算：

```text
scaled_run = trunc(horizontal_step * run / 1024)
```

literal 的水平 phase 在同一目标行的命令之间延续；每个源像素把
`horizontal_step` 加入 phase，每满 `0x400` 写一次相同像素。透明命令保留原程序
的二次缩放怪异行为：

```text
phase += horizontal_step * scaled_run
```

这里使用的是已经缩放过的 `scaled_run`，不是源 `run`。整条命令完全落在水平
clip 外时，原程序直接跳过命令，literal 与透明命令都不推进 phase；实现和 UT
均保留该差异。

## 4. 裁剪不对称与安全隔离

每个 run 先按其名义 `scaled_run` 计算可写上下界，literal 的实际重复次数则由
跨命令 phase 决定。正向路径在 run 名义右端没有越过 clip right 时，把
`destination + scaled_run` 当成包含端点；phase 恰好产生额外像素时会写到
clip-right 的一像素之外。反向路径使用相应但不完全对称的右到左指针比较。
这些比较和提前停止位置按 LST 保留，UT 明确锁定正向 one-past 写入。

原程序会直接写裸指针。现代实现允许该 one-past 落入 framebuffer 的实际 pitch
存储（包括 padding 或下一物理行），以保留可观察结果；只有指针真正离开 owned
framebuffer 时返回 `destination_out_of_bounds`，防止宿主内存破坏。截断行、越界
payload 与非法 surface/clip 同样只在异常边界返回状态，不改变正常游戏逻辑。

## 5. 实现与验证

- 公共合同：`include/openswd3/rendering/legacy_scaled_rle_writer.hpp`；
- 实现：`src/rendering/legacy_scaled_rle_writer.cpp`；
- UT：`tests/unit/rendering/legacy_scaled_rle_writer_test.cpp`。

固定向量覆盖正反顺序、`0x600` 纵向重复、顶部预裁剪、透明 run 二次 phase、
完全裁掉的命令不更新 phase、clip-right one-past、反向纵向拒绝、带标志零长度
命令、零图像与畸形行。未启动原版；以后若需要动态差分，仍按项目约束准备捕获
工具并由用户运行。
