# 字体 surface、字形缓存与软件绘制规则

## 证据边界

本结论只以 `swd3.exe_export_for_ai/swd3.exe.lst` 的完整反汇编为逻辑真值。ASM 与 IDA 伪码不参与范围或行为证明；默认字体名和两个静态对象的初始零值另由锁定哈希的原始 `swd3.exe` 字节验证。

本轮恢复的是文字从 Windows 字体到 16 位帧缓冲的完整物理链。结论不是“字体完全由 GDI 绘制”，也不是“EXE 内置位图字体”，而是混合管线：

1. Windows GDI 在固定的 DirectDraw 临时 surface 上产生字形像素。
2. EXE 把临时 surface 中任意非零的 16 位像素压成自己的 1-bit 字形缓存。
3. EXE 的五种软件 writer 再按原始裁剪、颜色和 footprint 写入最终 16 位帧缓冲。

所以 GDI 决定基础字形轮廓，EXE 决定最终像素覆盖、描边、阴影、颜色变化和裁剪。SDL3 可以替代窗口、输入、surface/纹理呈现等平台边界，但 SDL3 本身不提供与旧 GDI 等价的字体栅格结果。

## 字体定义

`sub_435500` 在 `0x00435500–0x00435591` 构造 `LOGFONTA` 并调用 `CreateFontIndirectA`。完整汇编中只有两个调用点 `0x0040F37C` 和 `0x004351A9`，当前都传入 EXE 中 `0x0049FB74` 的默认字体名。

原始字节为 `B2 D3 A9 FA C5 E9 00`，按 CP950 解码为 `細明體`。`LOGFONTA` 合同为：

- `lfHeight = -min(requested_size, 64)`；
- `lfWidth = 0`，`lfEscapement = 0`，`lfOrientation = 0`；
- `lfWeight = 400`；
- italic、underline、strikeout 均为零；
- `lfCharSet = 0x88`，即旧 Win32 的 `CHINESEBIG5_CHARSET`；
- output precision、clip precision、quality、pitch/family 均为零。

这里没有 EXE 自带的点阵字形数据，也没有请求灰度强度、抗锯齿质量或实际 glyph metrics 的逻辑。`quality=0` 把基础轮廓交给宿主 Windows/GDI/font 组合，因此不同 Windows、字体文件或字体替代规则可能产生不同的原始 mask。

## 固定 64×64 临时 surface 与 pitch

`sub_4353C0` 在 `0x004353CA–0x004353D0` 无条件以 `64, 64` 调用 `sub_435240`，创建 DirectDraw offscreen surface。它不按实际请求字高动态改变 surface 尺寸。

`sub_435240` 清零 surface 后调用虚表 `+0x58` 取得描述：

- 对象 `+0xFDC` 保存报告高度；
- 对象 `+0xFD8` 保存有符号 `lPitch >> 1`，单位是 16 位像素；
- 后续不能假设物理 surface 是紧密行排列。

`sub_4352E0` 的 GDI 调用顺序也属于合同：

1. 检查 surface 是否丢失；仅当结果是 `0x887601C2` 时 Restore。
2. 把 surface 清为 native color 零。
3. 通过虚表 `+0x44` 取得 HDC，SelectObject 当前 HFONT。
4. `SetBkColor(0)`、`SetBkMode(2)`、`SetTextColor(color)`。
5. 先在 `(0,0)` 对两字节字符串 `"  "` 调用一次 `TextOutA`，再对当前一或二字节字符调用一次 `TextOutA`。
6. 恢复先前 GDI object，并通过虚表 `+0x68` ReleaseDC。

字形缓存未命中时传入的绘制颜色固定为 `0xFFFFFF`。前置的两空格调用即使看起来无效也必须保留；初步还原不重排、不删除。

## surface 到 1-bit mask

`sub_436840` 先锁定临时 surface，再逐行复制：

- 每行只复制 `font_width * 2` 字节；
- 总共复制 `font_height` 行；
- 源行按物理 `lPitch` 推进；
- 目标是对象 `+0x18` 的紧密 16 位 scratch；
- 完成后按原顺序解锁 surface。

`sub_4368D0` 随后把 scratch 压为 1-bit mask：

- 每个 16 位 word 只测试是否 `!= 0`，强度值本身被丢弃；
- 非零 word 对应一个置位像素；
- 一行的位顺序是 MSB-first：`0x80, 0x40, ... 0x01`；
- mask 行跨度为 `ceil(font_width / 8)`；
- 行尾未使用 padding bit 保持零。

因此最终文字没有灰度覆盖率。旧 GDI 的任何非零覆盖都被量化为一，之后的轮廓和阴影也只由这一位 mask 扩张。

## 原始字节协议与缓存

`sub_436AD0` 接收 NUL 结尾的原始 byte string，不接收 Unicode string：

- 当前 byte `< 0x80`：消费一个 byte，并把字符 scratch 的第二 byte 置零；
- 当前 byte `>= 0x80`：不验证 Big5 lead/trail 合法性，直接再消费下一个 byte；
- 两 byte 按内存小端组成无符号 `u16` cache key；
- ASCII 前进量为 `FE0 - (FE0 >> 1)`；高位双字节前进量为完整 `FE0`；
- 布局完全不查询 GDI 的实际 glyph advance、kerning 或 shaping。

对象 `+0xFC8/+0xFC9` 保存当前一或二字节字符。紧邻的 `+0xFCA` 没有任何显式读写指令；两个静态 renderer 对象都位于 PE `.data` 零填充区，代码隐式依赖这里长期为 NUL 终止符。不能把它擅自解释成普通、每次都会显式写终止符的字符串缓冲。

缓存是 2000 个固定槽：

- `+0x20` 是 2000 个按无符号 `u16` 升序排列的 key；
- `+0xFC0` 指向对应的连续 mask 槽，每槽大小为 `font_height * mask_row_bytes`；
- 查找使用二分搜索；插入会同时移动 key 和整个 mask 槽；
- 新增后若 count 达到 `0x7CF`（1999），代码把 count 强制为 1998，并清除 slot 1998 的 mask。

最后一条是原始淘汰/上限异常，不能用无界 map、LRU 或“修正后的 2000 槽”替换。它属于要保留的游戏行为。

当前确定性兼容核心已经把这一段落实为
`include/openswd3/rendering/legacy_glyph_cache.hpp` 与
`src/rendering/legacy_glyph_cache.cpp`：`pack_legacy_glyph_mask` 保留
`sub_4368D0` 的“只 OR 置位、不负责清空目标槽”合同，缓存则明确拆开
`insert_empty` 和绘制后的 `finish_miss_after_draw`，没有把原调用顺序折叠成
常规容器插入。UT 覆盖非零强度量化、MSB-first、行尾 padding、无符号 key
顺序、整 mask 槽搬移，以及第 1999 次 miss 后 key 仍留在物理 slot 1998、
mask 被清零且 live count 回到 1998 的状态。

## 五种软件 footprint

以下以字形 mask 中的置位像素为 `p=(x+gx,y+gy)`，offset 表示最终帧缓冲相对 `p` 的写入位置。`foreground` 是调用参数中的 16 位前景色，`secondary` 是对象 `+0xFE4` 的 16 位阴影/轮廓色。

| 首个命中 selector bit | 函数链 | foreground 写入 | secondary 写入 | 关键裁剪 |
|---|---|---|---|---|
| `0x01` | `sub_435680` | `(0,0)` | 无 | `y>=top && y<bottom`；`x>=left && x+1<right` |
| `0x02` | `sub_435AF0` | `(0,0)` | `(0,+1),(+1,+1)` | `y>=top && y+1<bottom`；`x>=left && x+1<right` |
| `0x04` | `sub_435D80` | `(0,0),(+1,0)` | `(+1,+1),(+2,+1)` | `y>=top && y+1<bottom`；`x>=left && x+2<right` |
| `0x08` | `sub_436030 → sub_435680` | overlay `(0,0)` | prepass `(0,-1),(-1,0),(0,0),(-1,+1),(0,+1),(+1,+1)` | prepass：`y>top && y+1<bottom`、`x>=left-1 && x+1<right`；overlay 使用 `0x01` 裁剪 |
| `0x10` | `sub_436410 → sub_4358C0` | overlay `(0,0),(+1,0)` | prepass `(0,-1),(-1,0),(+2,0),(-1,+1),(0,+1),(+1,+1),(+2,+1)` | prepass：`y>top && y+1<bottom`、`x>=left-1 && x+2<right`；overlay：`x>=left && x+2<right` |

这些比较有意保留多余的右侧/下侧空间，不能统一成通用“每个实际写入点单独检查是否在 clip 内”的现代裁剪器。`0x08/0x10` 先写 secondary prepass，再写 foreground overlay；两阶段裁剪范围不同，边缘处可能留下通常会被 overlay 覆盖的 secondary 中心像素。

style 参数的低五位不是互斥枚举。`sub_436AD0` 按 `0x01 → 0x02 → 0x04 → 0x08 → 0x10` 的优先级依次测试，第一个置位 bit 获胜；低五位没有任何已知 selector 时不会调用 glyph-pixel writer。

## 行颜色变化与两阶段旧行为

每条 mask 行结束后，前景 packed `u16` 做原始整数加法：

- `flags & 0x100`：每行 `-1`；
- 否则若 `flags & 0x80`：每行 `+1`；
- 否则不变。

这是对整个 packed 16 位颜色的低 16 位加减，不是分别调节 RGB 通道，也没有饱和保护。

`0x08` 和 `0x10` 的 secondary prepass 虽不把 foreground 写到屏幕，循环仍然按每行推进 foreground。后续 overlay 接收到的是 `original_color + font_height * delta`，并再推进一遍。这个两阶段颜色起点异常必须原样保留。

当前 291 个直接调用点中，276 个 style 是立即数、15 个来自运行时寄存器。已观察到的组合立即数进一步证明优先级合同，例如：

- `0x15`、`0x19`、`0x0F`、`0x1F` 最终都选择 `0x01`；
- `0x18` 选择 `0x08`；
- `0x84` 选择 `0x04`，同时每行前景色 `+1`；
- `0xD0` 选择 `0x10`，同时每行前景色 `+1`。

## 背景矩形

对象 `+0xFE6` 是背景色；`0xFFFE` 表示禁止背景。启用时，`sub_436EA0` 在 glyph writer 前填充矩形，并有末字符宽度修正。其裁剪边界是整个目标 framebuffer 的宽高，而不是对象 `+0xFE8..+0xFF4` 的 glyph clip。

背景、glyph clip 和最终 footprint 是三个不同边界，重写时不能合成一个 rectangle API 后期待等价结果。

## 对跨平台重写的约束

SDL3 在 Windows、Linux、macOS 等平台都能承接窗口、事件、输入和最终纹理呈现，但这里还需要独立的 `GlyphProvider` 边界。该边界只能负责产生原程序定义的 MSB-first 1-bit mask；字符解析、固定 advance、缓存插入/淘汰、style priority、footprint、裁剪和 packed-color 行变化仍属于确定性的兼容核心。

为了 1:1，不能直接让各平台自由选择系统字体或默认 FreeType 参数，因为 host font、hinting、fallback 和 rasterizer 差异会改变“非零即一”后的 mask。后续需用原程序/目标旧 Windows 环境建立 glyph-mask oracle，再评估两条可验证路线：

1. 冻结已验证字体文件、栅格器版本和全部参数，并逐 glyph 与 oracle 比较；
2. 对确定字符域预生成原始 1-bit mask，同时为运行时动态输入定义不改变旧 byte 协议的补充策略。

目前不提前决定二者，也不把 GDI 继续作为跨平台硬依赖。Windows GDI 路径可以作为研究期 oracle 或兼容后端；SDL3 路线不因此改为 DirectX 路线。

## 必须保留的 1:1 行为

- 字体 size 上限 64、固定 64×64 临时 surface 和物理 pitch 处理。
- `TextOutA("  ")` 后再画原始字符的调用顺序。
- CP950-era 的单字节/盲取双字节分类，不替换成 UTF-8 解码或 Unicode shaping。
- 任意非零 16 位像素变成一，MSB-first mask 与行尾零 padding。
- 固定整数 advance，不采用字体实际 metrics。
- 1999 到 1998 的缓存 count/清槽行为。
- selector bit 优先级，而不是互斥 style enum。
- 五种严格且不对称的 clip、footprint 和写入顺序。
- packed `u16` 每行 `-1/+1`，以及 `0x08/0x10` prepass 后 overlay 再次推进的异常。
- `0xFFFE` 背景禁用值和背景只按 framebuffer 边界裁剪。
- `+0xFCA` 隐式零终止依赖；初步还原不得用“更安全”行为改变可观察结果。

当前实现只对无法形成原 64×64 正常字体对象的非正 geometry 建立显式安全
边界；正常执行的 mask、排序、搬移、绘制后计数与清槽顺序均按完整 LST
保留。该边界不改变任何已确认游戏路径。

## 可复现产物

- 生成器：`tools/build_font_glyph_inventory.py`
- 对象字段：`inventory/font-renderer-object-fields.tsv`
- 字形生产管线：`inventory/font-glyph-pipeline.tsv`
- 五种 footprint：`inventory/font-glyph-style-footprints.tsv`
- 291 个直接调用点：`inventory/font-render-callsites.tsv`

生成器同时锁定原 EXE 与完整 LST SHA-256、默认字体原始字节、PE `.data` 零填充边界、关键指令、两个字体设置调用点、291 个文字调用点及 style operand 分布。任一锁定事实变化都会停止生成，而不是静默沿用旧结论。
