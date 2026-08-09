# `0x00415B70` 双槽方向状态效果

状态：完整 LST 控制流、三组物理状态、初始化生产者、secondary RNG 顺序、ACT→TSW→blitter 路径和四向重生分支已闭环；真实 variant 0 framebuffer 向量已通过。

唯一行为真值是 `swd3.exe.lst`。本文只按已证明的方向输入、位置、速度和绘制行为命名；不根据视觉猜测把它固定解释为雨、雪或其他具体玩法效果。

## 调用与启用边界

唯一每帧调用点是 `0x00412AD0`。外层世界画面协调器先要求 service `0x48` 为零、`[0x004C9A18] bit 0` 为零，再依次调用 `0x004161C0`、`0x00416590`、`0x004167B0`、本函数、`0x004163C0` 和 `0x00416CC0`。这些外层条件属于 world owner。

本函数自己的第一条业务操作是 `sub_40DC50(5)`。返回零时 `0x00415B7A..0x00415B7C` 立即返回，不消耗 RNG、不更新动作、不访问状态或 framebuffer。重写把它保留为 service 5 端口。

## 三组分离的物理状态

原程序没有一块连续的“效果对象”。它用同一个字节索引 `ESI` 访问三处相距很远的并行数组，步长都是 `0x10`：

| 物理基址 | 槽内偏移 | 汇编证明的含义 |
|---|---:|---|
| `0x004C97A0` | `+0x00` | 世界 X |
|  | `+0x04` | 世界 Y |
|  | `+0x08` | X 速度 |
|  | `+0x0C` | Y 速度 |
| `0x004ACDC0` | `+0x00` | 当前 RGB 公共偏移 |
|  | `+0x08` | 目标 RGB 公共偏移 |
| `0x004ACE00` | `+0x00` | 帧计数器 |
|  | `+0x04` | 当前动作 variant |
|  | `+0x08` | 目标更新间隔 |
|  | `+0x0C` | 当前更新间隔 |

三组的 `+0x04/+0x0C` 未列字段在本路径不读写。重写继续保存三个独立的四槽 `0x10` 数组，没有为了 C++ 方便伪造原程序不存在的连续大结构。

`0x0040C1B9..0x0040C1CD` 和 `0x0040E606..0x0040E61A` 以 `rep stosd` 把 `0x004C97A0` 起的 16 个双字写成 `0x0F0F0F0F`，恰好覆盖四个 motion 槽；color 与 timing 两组不在这次重置范围内。

地图装载器 `0x0040C7D3..0x0040C859` 在 service 5 已启用时初始化四个物理槽，每槽严格消耗六次 secondary RNG：

```text
target_interval = random(3) + 1
target_color    = 0
variant         = random(variant_count) + base_variant
x               = random(map_width  << 4)
y               = random(map_height << 4)
velocity_x      = random(2) - 2
velocity_y      = random(2) - 2
```

该生产者不清当前 color、帧计数器或当前 interval。与此不对称，`0x00415B70` 的循环只令 `ESI` 取 `0`、`0x10`，因此每帧只更新前两个槽。四槽初始化、两槽更新是原始行为，不合并成二槽或擅自更新后两槽。

`variant_count` 通常先写 4；地图头标志可把 service 5 打开并把 count 改为 8。`base_variant` 和方向字段也来自地图装载状态。非法零 RNG 上界在原程序会进入未定义失败；重写只在这种宿主不安全边界返回显式错误，不改变有效资产路径。

## 每槽概率值与包含边界

通过 service 门后，每个被更新的槽都先无条件执行一次 `random(1000)`。设结果为 `roll`。可见范围使用有符号、包含边界的拒绝条件：

```text
x <= -640                         -> outside
x >= (map_width_tiles  + 40) << 4 -> outside
y <= -320                         -> outside
y >= (map_height_tiles + 20) << 4 -> outside
```

所有加法、乘法和左移保持 32 位回绕。outside 槽只有 `roll >= 990` 才重生；`roll < 990` 时完整保留该槽且不绘制。inside 和 outside 路径复用同一个 `roll`，不能为不同概率判断重新取随机数。

## 范围内更新

inside 槽先按 32 位回绕增加帧计数器。只有新的 `frame_counter > current_interval` 时才执行位置与目标逼近，并把计数器清零：

```text
x += velocity_x + arithmetic_shift_right(movement_scale * player_delta_x, 2)
y += velocity_y + arithmetic_shift_right(movement_scale * player_delta_y, 2)

if roll < 25:
    target_interval = random(5) + 1
current_interval += sign(target_interval - current_interval)

if roll < 5:
    target_color = random(4) - 5
current_color += sign(target_color - current_color)
```

`roll < 5` 同时满足前一条门，所以 RNG 次序必定先是 interval、再是 color。计数器未超过当前 interval 时，位置、interval 和 color 全部保持，只保留计数器加一；该槽仍在本帧绘制。乘法取低 32 位，右移为有符号算术移位，所有状态加减继续按 `i32` 回绕。

## 越界重生与复用 RNG 的 variant 分桶

当 outside 且 `roll >= 990` 时，函数先执行：

```text
target_color    = 0
target_interval = floor(roll / 100) + 1
variant         = base_variant + floor(variant_count * roll / 1000)
```

两个除法在原汇编中用乘法常量实现。特别是 variant 不调用 RNG，而是复用最初的 `roll` 分桶；把它写成 `random(variant_count)` 会让后面的 X 和速度随机序列整体错位。

随后按无符号方向值 `0..3` 分派：

| 方向 | Y | X 速度 | Y 速度 |
|---:|---:|---:|---:|
| `0` | `(map_height << 4) + 118` | `random(1) - 2` | `random(1) - 2` |
| `1` | `(map_height << 4) + 118` | `random(2)` | `random(2)` |
| `2` | `-300` | `random(2) - 2` | `random(2) - 2` |
| `3` | `-300` | `random(2)` | `random(2)` |

四种情况都先以 `random(map_width << 4)` 建立 X。方向 0 的两个 `random(1)` 虽恒为零，调用仍必须发生。方向大于 3 走 switch default：已经写入 target color、target interval 和 variant，但不消耗 X/速度 RNG，也不修改 motion 槽。成功重生的槽同样在本帧不绘制。

## 共享动作、颜色与绘制

每个 inside 槽都复用共享动作记录 `0x004ACAF8`；该记录也被 `0x00415EE0` 使用。固定顺序为：

1. action 写 `0x232B`，base variant 写当前槽 timing `+0x04`；
2. 调用 `0x004321E0` 更新同一动作记录；
3. 以动作 `+0x4A/+0x4C` 调用 `0x004315D0`；
4. variant count 为 4 时使用 flags `4`，否则使用 `0x2C`；
5. 把当前 color 同时写入蓝、绿、红偏移，并把动作字节 `+0x8A` 零扩展为 opacity 状态；
6. 以动作 `+0x18` 之外的上述固定 flags 调用 blitter。

绘制坐标是：

```text
draw_x = x - signed(action.draw_offset_x) - camera_x
draw_y = y - signed(action.draw_offset_y) - camera_y
```

位置不除以 16；地图宽高左移只把 tile 数转换为世界像素边界。blitter 返回值被原函数忽略，重写继续执行并仅把失败计数作为诊断信息暴露。最后一槽写入的 RGB/opacity 共享状态继续保留。

## 验证

合成 UT 固定覆盖：

- 三个独立 `0x10` 物理组、四槽初始化与两槽逐帧更新差异；
- `0x0F0F0F0F` motion-only 重置，以及初始化对 current color/counter/interval 的保留；
- 每槽六次初始化 RNG、固定 seed 状态和四槽完整向量；
- service 5 零副作用、四条包含边界和 1% 严格重生门；
- 帧计数、移动缩放的算术右移、目标 interval/color 的同 roll 门与 RNG 顺序；
- 四个方向、两个不可省略的 `random(1)`、无效方向 default 和重生帧不绘制；
- variant 复用 roll 分桶、4/非 4 两套 flags、`i32` 回绕与被忽略的 blitter 失败；
- ACT/TSW 失败隔离，以及真实 action `0x232B` variant 0 到软件 blitter 的路径。

真实 variant 0 framebuffer 的 FNV-1a64 为 `0xE216591950463029`。Linux `core`
100/100、Windows LLVM `app` 104/104 CTest 通过。最终原程序 framebuffer/状态差分仍按
工程规则登记为 `blocked_runtime_oracle`；需要时只准备 Frida spawn 工具并等待用户执行，
不由 OpenSWD3 开发流程自行启动原版。
