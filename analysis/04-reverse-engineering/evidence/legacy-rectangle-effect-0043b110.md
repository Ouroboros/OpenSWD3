# 六模式矩形画面效果 `0x0043B110`

状态：`sub_43B110` 的裁剪、六个模式、行步进和异常边界已按完整 LST 逐基本块复核；正常可寻址输入为 `assembly_exact`，原程序会越界或超长执行的输入由显式宿主安全状态隔离。

## 证据范围

唯一行为真值是 `swd3.exe.lst`：

- 主函数：`0x0043B110–0x0043B466`；
- 模式 0 helper：`sub_417DE0`，`0x00417DE0–0x00417E34`；
- 模式 1/5 helper：`sub_420490`，`0x00420490–0x00420559`；
- 模式 3 helper：`sub_421FB0`，`0x00421FB0–0x00422025`；
- 颜色打包：`sub_4239D0`，`0x004239D0–0x00423A0D`；
- packed 通道 mask 建立：`sub_423400` 的 `0x00423686–0x00423740`。

完整 LST 中共有 87 条对 `sub_43B110` 的直接 `call`，来自 36 个调用函数；没有发现取得函数地址后再间接调用的路径。调用覆盖通用界面、特殊模式和战斗画面，不能把本函数降格成某个单一菜单的局部效果。

## 参数与共享状态

cdecl 参数依次为：

```text
(x, y, width, height, red, green, blue, mode)
```

函数借用以下既有渲染状态：

| 地址 | 含义 |
|---|---|
| `0x004CD2F8` | clip left |
| `0x004CD734` | clip top |
| `0x004CD310` | clip width |
| `0x004CD720` | clip height |
| `0x004A0E74` | framebuffer byte pitch |
| `0x004CD76C` | 16 位 framebuffer 首地址 |
| `0x004CD788/8C/90/94` | 三通道分别右移 `1/2/3/4` 后合并并复制到两个 16 位 lane 的 mask |

`sub_4239D0(red,green,blue)` 先按每通道低五位组成 RGB555，再通过当前 forward 像素转换函数处理两个相同像素，返回高低 16 位相同的 packed 颜色。

## 裁剪与分派顺序

四边裁剪全部使用 32 位回绕算术和有符号比较：

```text
if x < clip_left:
    width += x - clip_left
    x = clip_left

if y < clip_top:
    height += y - clip_top
    y = clip_top

if x + width > clip_left + clip_width:
    width = clip_width - x + clip_left

if y + height > clip_top + clip_height:
    height = clip_height - y + clip_top
```

裁剪后 `width<=0` 或 `height<=0` 立即返回，因而不会调用颜色打包。非空矩形先执行颜色打包，再读取 pitch 并按 `mode` 分派；`mode>5` 是无像素写入的默认分支。

所有模式的首像素地址均为：

```text
framebuffer + 2 * (y * (pitch_bytes / 2) + x)
```

没有使用固定 `640` 代替实际 pitch。

## 六个模式

记 `D` 为连续两个 16 位目标像素组成的 32 位值，`C` 为两个相同效果色组成的 32 位值；`M1..M4` 是当前有效 RGB 字段分别右移一至四位后合并、再复制到两个 lane 的 mask。

### 模式 0：四项 packed 混合

每行调用 `sub_417DE0(destination,C,width)`：

```text
pair_count = width >> 1
out = ((D >> 1) & M1)
    + ((C >> 2) & M2)
    + ((D >> 3) & M3)
    + ((D >> 4) & M4)
```

一次读写两个像素；奇数宽度最后一个像素不处理。不得将公式替换成浮点 alpha blend，也不得补奇数尾像素。

### 模式 1：逐行三通道有符号偏移

每行以完整裁剪宽度调用 `sub_420490`。通道先做模 32 位 `delta×unit` 和加法，再按字段外位及 delta 符号饱和到 mask 或零。精确公式和 helper 的四字节读取、两字节写回合同见 [`frame-color-adjustment-and-combine.md`](frame-color-adjustment-and-combine.md)。

### 模式 2：packed 四分之一亮度

```text
pair_count = width / 2
out = (D >> 2) & M2
```

一次处理两个像素，奇数尾像素保持原值。

### 模式 3：除以四的暗灰度

每行以完整裁剪宽度调用 `sub_421FB0`：

```text
q = (red_scalar + green_scalar + blue_scalar) >> 2
out = q << red_shift | q << green_shift | q << blue_shift
```

这里不是除以三的平均灰度；全白会得到通道值 23。

### 模式 4：packed 八分之一亮度

```text
pair_count = width / 2
out = (D >> 3) & M3
```

同样不处理奇数尾像素。

### 模式 5：从上下边缘向中间递增的对称偏移

```text
pair_count = (height + 1) / 2
step_c = wrapping_i32(component_c << 10) / pair_count
fixed_c = 0

repeat pair_count times:
    delta_c = fixed_c / 0x400       // 有符号、向零截断
    offset(top_row, delta)
    offset(bottom_row, delta)
    top_row += pitch
    bottom_row -= pitch
    fixed_c = wrapping_i32(fixed_c + step_c)
```

奇数高度的中央行会在同一次循环中被处理两遍。这是可观察行为，不能去重。`cdq/and/add/sar` 产生的负定点转换与 C++20 有符号除法一致，必须向零截断。

## 异常与宿主安全边界

`sub_417DE0` 在进入循环前把 `width` 无符号右移一位，但没有零检查。模式 0 的裁剪宽度为 1 时，pair count 从零递减为 `0xFFFFFFFF`，若内存不先异常会执行 `2^32` 次 packed 写入。现代实现返回 `invalid_geometry` 且不写 framebuffer；这是显式安全隔离，不宣称该输入为 `assembly_exact`。

现代接口还在实际写入前验证 pitch、矩形和 owned framebuffer 的物理范围，失败返回 `invalid_geometry` 或 `destination_out_of_bounds`。原函数会直接按全局指针寻址；这些状态只阻止宿主内存破坏，不改变正常可寻址调用的像素结果。

模式 1/3/5 的 helper 原本会在最后一个逻辑像素处额外读取后续两个字节。兼容核心保留其 16 位输出公式，但不人为制造页边界越读故障；该差异同属宿主安全隔离。

## 实现与验证映射

- 公共合同：`include/openswd3/rendering/legacy_rectangle_effect.hpp`；
- 实现：`src/rendering/legacy_rectangle_effect.cpp`；
- UT：`tests/unit/rendering/legacy_rectangle_effect_test.cpp`。

UT 固定以下汇编可区分行为：

- 模式 0 四项精确常量和奇数尾像素；
- 模式 2/4 packed 右移、mask 与奇数尾像素；
- 模式 1 正负饱和及 RGB565 有效字段；
- 模式 3 全白与纯红输入；
- 模式 5 上下对称、奇数中央行双写及负定点向零截断；
- 四边裁剪、空矩形、未知模式和模式 0 宽度 1 的安全状态。

当前证据等级：正常可寻址输入为 `assembly_exact`；尚未取得原程序 framebuffer 输出，动态差分为 `blocked_runtime_oracle`。
