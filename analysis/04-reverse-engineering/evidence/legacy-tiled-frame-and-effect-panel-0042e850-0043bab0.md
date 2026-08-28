# 九宫格边框与效果面板 `0x0042E850 / 0x0043BAB0`

状态：两段函数已按完整 LST 逐基本块复核并实现；正常可寻址输入为 `assembly_exact`，缺失资源、零尺寸平铺块和会破坏宿主内存或形成无限循环的几何输入由显式安全状态隔离。

## 证据范围

唯一行为真值是 `swd3.exe.lst`：

- 九宫格绘制：`sub_42E850`，`0x0042E850–0x0042ED3D`；
- 效果面板组合：`sub_43BAB0`，`0x0043BAB0–0x0043BB30`；
- clip 设置：`sub_416FF0`；
- 实际 blit：`sub_4170E0`；
- 帧块查询：`sub_4315D0`；
- 动作记录更新：`sub_4321E0`；
- 面板底色效果：`sub_43B110`。

完整 LST 中分别存在 66 条对 `sub_42E850`、23 条对 `sub_43BAB0` 的直接 `call`。两者被剧情、特殊模式和战斗等多条路径复用，不是单一菜单专用代码。战斗网格列表帧`0x00465B13`caller现已直接组合本typed九宫格实现；同函数的`0x00465AEA`、`0x00465D46`、`0x00465D75`三处底色效果也直连typed矩形，并与相邻列表框共享frame provider、raster、effect和jitter owner。替代网格列表帧`0x00465EF4`矩形及`0x00465F1B`/`0x00465F42`双九宫格caller也已直接组合同一typed实现，分别保留矩形返回和首个九宫格返回的资源高16位。模式网格帧`0x00466215`矩形及`0x0046623F`/`0x00466267`双九宫格caller同样已直连，其中第一段资源高16位改由矩形返回EAX保留，第二段仍由首个九宫格返回EDX保留。窄网格帧`0x00466553`矩形及`0x0046657A`/`0x004665A1`双九宫格caller也已直连，两段分别保留矩形返回和第一九宫格返回的EDX高16位。

## `sub_42E850` 参数与分派

cdecl 参数依次为：

```text
(resource_id, left, top, right, bottom, opacity_step, flags)
```

`flags` 的最高位和低 31 位分别表示：

```text
border_only = (flags & 0x80000000) != 0
margin      = flags & 0x7fffffff
```

`opacity_step` 总会写入旧全局 `0x004CC2F0`。为零时每次 blit 使用 flags `0x00`，非零时使用 `0x14`；像素族仍由已有的 `sub_4170E0` 稀疏分派和源头部共同决定。

`sub_4315D0(resource_id,piece_index)` 返回的描述符中，`+0` 是源字节指针，`+0x0C/+0x0E` 分别是无符号 16 位宽高。现代核心通过 `LegacyFramePieceProvider` 借用这些数据，不让 rendering 反向拥有 B6 的 TSW/ACT 运行时。

## 中心和特殊第 9 块

最高位未置位时才绘制中心。若 `resource_id` 的低 16 位等于 `0x234A`，中心之前先绘制第 9 块；这里查询参数不是原始的完整 `resource_id`，而是汇编硬编码的：

```text
piece = load(0x234A, 9)
x = right + piece.width - margin
y = sar32(top + bottom, 1) - 12
```

随后 clip 设为 `[left,top,right,bottom)`，只查询一次第 4 块，并以其宽高先横向、后纵向平铺完整中心。右边界和底边界只负责终止起点循环，越出的最后一块由 clip 截掉。

## 九宫格边框顺序

边框始终执行，顺序固定为：

```text
0 左上角
1 顶边
2 右上角
3 左边
5 右边
6 左下角
7 底边
8 右下角
```

外框四边由 `margin` 以 32 位回绕加减得到。每块绘制前都按所在区域调用 `sub_416FF0`，因此角块和最后一个平铺块可依赖 clip 截断。

以下查询与循环细节是可观察合同：

- 第 1 块在顶边每次平铺前重新调用 `sub_4315D0`，包括第一次；宽度可以逐次变化。
- 第 7 块只查询一次，再用同一个描述符平铺整条底边。
- 每轮垂直平铺分别重新查询第 3、5 块；下一轮 `y` 只增加第 5 块的高度，不使用第 3 块高度。
- 左边的 clip 从当前 `y` 开始，右边的 clip 每轮仍从原始 `top` 开始。
- 用作底边起点的 `EBX` 保持为 `left`；即使 `top>=bottom`、垂直循环完全跳过，底边仍从左端平铺。
- 结束时无条件执行 `sub_416FF0(0,0,640,480)`；现代实现同样恢复完整逻辑 clip，并按实际 owned surface 尺寸进行原 helper 已有的上限截断。

## `sub_43BAB0` 组合顺序

参数与 `sub_43B110` 相同：

```text
(x, y, width, height, red, green, blue, mode)
```

函数没有条件分支，严格按以下顺序执行：

```text
sub_43B110(x - 8, y - 8, width + 16, height + 16,
           red, green, blue, mode)

static_action[+0x00] = 0x233B
static_action[+0x08] = 0
sub_4321E0(&static_action)
frame_resource_id = u16(static_action[+0x4A])

sub_42E850(frame_resource_id,
           x, y, x + width, y + height,
           0, 0x80000008)
```

因此效果矩形必须先于动作资源更新，边框固定为 8 像素、只画边框且不启用 opacity blitter。现代 `LegacyEffectPanelActionPorts` 只表达“以动作 ID `0x233B`、动作索引 `0` 更新并取出 `+0x4A` 帧资源”这一条跨模块合同；完整动作记录仍由 B6 `asset_runtime` 恢复。

## 宿主安全边界

原循环没有处理零宽或零高帧块：相应坐标不会前进，会形成无限循环。现代实现返回 `invalid_frame_geometry`。帧资源缺失、blitter 拒绝地址和 32 位循环范围无法安全遍历也返回显式状态，并在已进入九宫格函数时恢复完整 clip。

`sub_43BAB0` 对 `sub_43B110` 的原始异常路径同样没有恢复能力；现代组合层只在矩形返回 `completed`、`clipped_out` 或 `unsupported_mode` 时继续。`invalid_geometry` 和 `destination_out_of_bounds` 会停止后续动作与边框调用，避免把原本的越界或超长执行扩散到宿主进程。

## 实现与验证映射

- 九宫格合同：`include/openswd3/rendering/legacy_tiled_frame.hpp`；
- 九宫格实现：`src/rendering/legacy_tiled_frame.cpp`；
- 效果面板合同：`include/openswd3/rendering/legacy_effect_panel.hpp`；
- 效果面板实现：`src/rendering/legacy_effect_panel.cpp`；
- UT：`tests/unit/rendering/legacy_tiled_frame_test.cpp`、`tests/unit/rendering/legacy_effect_panel_test.cpp`。

UT 固定边框/中心的逐像素布局、查询次序、`0x234A` 第 9 块硬编码资源、零高度底边、完整 clip 恢复、矩形→动作→边框顺序，以及资源和几何安全状态。

回归结果：Linux `core` 50/50、Windows LLVM `app` 52/52 CTest 通过；Windows 构建只执行编译和测试，没有启动 OpenSWD3 或原程序。

当前证据等级：正常可寻址输入为 `assembly_exact`；尚未取得原程序 framebuffer 输出，动态像素差分为 `blocked_runtime_oracle`。
