# `0x00416D30` framebuffer 几何与行偏移

状态：B4.3 静态闭环；`assembly_exact`、`platform_adapted`、`blocked_runtime_oracle`

来源：`swd3.exe.lst` 完整指令。汇编是唯一行为真值；DirectDraw 类型名只用于解释物理边界。

## 1. 实际 surface 描述

三个静态初值位于：

- `0x004A0E74 = 0x500`：byte pitch。
- `0x004A0E78 = 0x280`：width 640。
- `0x004A0E7C = 0x1E0`：height 480。

`0x00423419..0x0042343C` 清零 0x7C 字节描述结构并锁定传入 surface。锁定失败立即返回零，不发布新几何。成功时，`0x00423450..0x0042347A` 先以 Lock 返回的像素地址调用 Unlock，再依次发布 width、byte pitch 和 height。

传入 surface 为零时，`0x004234A4` 只选择 RGB555 masks；它不会重置上述三个几何全局。因此首次调用使用静态 `640×480/0x500`，以后以零 surface 重入会保留上次成功查询的几何。

## 2. `0x00416D42..0x00416D80`

像素状态查询成功后，初始化子块执行：

```text
clip_width  = surface_width
clip_left   = 0
clip_top    = 0
clip_height = surface_height

offset = 0
for each row while signed height > 0:
    row_byte_offsets[row] = offset
    offset = (offset + pitch_bytes) & 0xFFFFFFFF
```

行偏移表从 `0x004CC2F8` 延伸到下一个状态 `0x004CD2F8`，物理空间为 `0x1000` 字节，即 1024 个 dword。当前 surface 高度固定为 480，所以只写前 480 项。

高度小于等于零时，四个 clip 字段仍被发布，但行表完全不写；较短高度重建也不清理旧表尾部。pitch 累加是 32 位 x86 `add`，必须保留回绕。

原代码没有检查高度超过 1024 的情况，而会继续覆盖相邻全局。现代平台边界拒绝这种 surface 描述并保留旧状态；这是阻止新平台返回值破坏进程的兼容隔离，不改变当前 `640×480` 路径。

## 3. owned framebuffer

原程序的 21 对公共 Lock/Unlock 中，19 对把 Lock 返回地址发布到 `0x004CD76C`，随即 Unlock，之后仍继续通过该地址软件绘制。这依赖旧 DirectDraw surface 内存解锁后仍稳定。

现代核心改为拥有一块生命周期稳定的 16 位物理缓冲。物理大小是 `pitch_bytes × height`，逻辑行只暴露 width 个像素，行首严格使用上述 byte offset；pitch padding 仍保留在物理缓冲中。输入几何必须为正高度、正宽度、偶数 pitch，且 pitch 至少容纳一行 16 位像素。

这个对象不把所有路径改成逻辑行访问。原汇编中的以下固定画布常量仍单独保留，后续效果函数按自己的地址逐项实现：

- `0x500`：固定 1280-byte 相邻行。
- `0x96000`：固定 `640×480×2` 字节。
- `0x4B000`：固定 307,200 个 16 位像素。
- `0x25800`：固定 153,600 个 dword。

因此 padding-aware 行算法与固定紧凑画布算法不会被错误合并。

## 4. SDL3 上传与回放哈希

SDL3 主程序不再另建一份紧凑 `std::vector<u16>` 作为画面。截图和呈现都直接
借用 `LegacyFramebuffer` 的稳定 owned storage；`SDL_UpdateTexture` 的地址来自
`physical_pixels().data()`，pitch 来自当前 surface geometry，而不是重新写死
`640×2`。因此带 padding 的现代 surface 描述不会在平台上传边界被压平。

帧回放使用 logical framebuffer 的 FNV-1a 64：逐行只取 width 个像素，每个
16 位像素严格按低 byte、再高 byte 输入哈希。该定义与宿主大小端和物理行
padding 无关，直接对应动态捕获协议中的 `framebuffer-logical.bin`。合成
`3×2`、8-byte pitch 向量的固定值为 `0x8109C18FE56669D3`；修改两行 padding
后结果不变。

这里没有伪造原版 framebuffer 样本。原版动态哈希仍为
`blocked_runtime_oracle`；需要捕获时由用户运行 Frida spawn 工具，Codex 不
自行启动原版。

SDL 恢复适配保留同一份 owned framebuffer，不在失焦时销毁它。恢复阶段若
纹理句柄缺失则以当前 geometry 重建 RGB565 streaming texture，随后重新上传
现有帧；idle presenter 持有纹理槽引用，因此不会继续使用重建前的旧指针。
任一 SDL 恢复或重新呈现失败都会令外壳停止主循环并记录错误，不能静默推进
游戏帧。

原版恢复会强制 `SetWindowPos(...,640,480)`，现代可缩放窗口则只执行 restore，
保留用户拖拽得到的窗口尺寸；逻辑画布、letterbox 和输入坐标仍固定为
`640×480`。这是满足现代窗口缩放需求的平台层扩展，不改变兼容核心像素。

## 5. 验证入口

UT 覆盖默认 `640×480×16`、带 padding 的 pitch、稳定物理地址、逻辑行不包含 padding、固定小端 logical hash、短高度重建保留旧尾部、非正高度不写行表、32 位 pitch 回绕、1024 项物理边界和 owned storage 的平台输入约束。Linux Clang `core` 54/54，Linux/Windows LLVM `app` 56/56 CTest 全部通过；实际窗口最小化/恢复仍待平台 smoke，未标记为动态验证。

原程序 framebuffer 动态捕获仍不可用，因此本单元不能标记 `original_diff_verified`。
