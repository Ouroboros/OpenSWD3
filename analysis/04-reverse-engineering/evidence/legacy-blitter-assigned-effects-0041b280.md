# 目标颜色偏移、常量填充与灰度 blitter

状态：本行为单元为 `assembly_exact`、`asset_verified`；原程序动态 framebuffer 差分仍为 `blocked_runtime_oracle`。

## 1. 范围与真值

本单元实现三个已赋值 RLE 效果族及其正反向例程：

- `0x0041B280/0x0041B620`：槽 `0x10/0x11`，目标颜色偏移；
- `0x00421230/0x00421540`：槽 `0x24/0x25`，常量填充；
- `0x00421850/0x00421BE0`：槽 `0x28/0x29`，目标灰度化；
- 槽 `0x26/0x27` 与 `0x2A/0x2B` 分别复用同一对常量填充和灰度例程；
- `0x00423400..0x00423896`：有效通道 mask、shift 和单位状态。

完整 LST 是本单元的主要汇编真值，已逐地址核对函数体、分支和全局状态访问。ASM 不提供 LST 缺失的行为信息，不作为额外的必经复核步骤；只有 LST 存在反汇编边界疑义时才需要回到原 EXE 机器码。

## 2. 公共 RLE 合同

三族都沿用基础 copy 已恢复的物理行和命令格式：

- 源 `[+6] & 0x10` 未置位时不处理像素；
- 每行首个 `u16` 是物理行长，下一行使用 `length & 0x7FFF`；
- 命令低 14 位是覆盖长度，零结束当前行；
- 高两位为零的 literal run 才触发本族像素操作；
- `0x4000/0x8000/0xC000` run 只推进覆盖位置，不携带像素数据；
- literal 载荷仍按 `run*2` 消费，但这三族都不读取其中的颜色值。

正向例程从左到右写目标；反向例程仍正向解析源流，只把目标写入位置从右向左推进。顶部裁剪、水平可见窗口、纵翻目标行步长和终止行访问顺序均复用基础 RLE 遍历合同。

## 3. `0x10`：目标颜色偏移

三个有符号偏移来自 `0x004CD71C/0x004CD30C/0x004CD304`。`0x0041B2A9..0x0041B320` 与反向对应块先计算：

```text
DR = red_offset   * (1 << red_shift)
DG = green_offset * (1 << green_shift)
DB = blue_offset  * (1 << blue_shift)
```

乘法和加法均按 x86 32 位回绕。每个 literal 覆盖位置读取目标像素，对任一通道执行：

```text
candidate = u32((destination & mask) + delta)

if (candidate & ~mask) != 0:
    candidate = offset >= 0 ? mask : 0
```

三个结果按位 OR 后只写回低 `u16`。源 literal 颜色不会参与结果。

`0x0041B289..0x0041B2A4` 与 `0x0041B629..0x0041B644` 还固定了特殊早退：三个偏移全零时直接返回，不读取源 `[+6]`，不访问 jitter 表，也不推进 phase。这不是执行一次恒等像素循环。

逐行 jitter 的不对称合同为：

- 正向 `0x0041B32B`：组基址步长 `0x528` 字节；
- 反向 `0x0041B6CB`：组基址步长 `0x84` 字节；
- 两个方向正常退出时都把 phase 加 4，并在 `0x84` 回零。

## 4. `0x24`：常量填充

`0x00421239..0x0042123E` 和 `0x00421549..0x0042154E` 都先从第五个参数指向的位置读取一个完整 `u32`，随后只使用其低 `u16`。每个 literal 覆盖位置写入同一个低字常量；源 literal 颜色被跳过而不读取。

两个方向的 jitter 组基址步长都是 `0x84`。正向在正常退出时推进 phase；反向没有对应 phase 更新。这一差异不能因共用 RLE 遍历器而被抹平。

现代接口要求辅助字节视图至少有四字节，容量不足时返回 `auxiliary_out_of_bounds`，不把原来的任意地址四字节读取复制成 C++ 越界行为。

## 5. `0x28`：目标灰度化

`0x00421A38..0x00421A93` 与 `0x00421DE4..0x00421E42` 对每个 literal 覆盖位置读取目标 `u16`，按有效通道状态执行：

```text
r = (destination & red_mask)   >> red_shift
g = (destination & green_mask) >> green_shift
b = (destination & blue_mask)  >> blue_shift
q = (r + g + b) >> 2

out = (q << red_shift) + (q << green_shift) + (q << blue_shift)
```

它除以 4，不是除以 3，也没有权重或舍入常数。RGB555 全白 `0x7FFF` 的结果为 `0x5EF7`；单个满通道的结果为 `0x1CE7`。

逐行 jitter 的合同为：

- 正向 `0x004218DB`：组基址步长 `0x528` 字节；
- 反向 `0x00421C6B`：组基址步长 `0x84` 字节；
- 两个方向正常退出时都推进 phase。

## 6. `0x00423400` 的有效像素状态

DirectDraw 报告的 mask 会先原样保存，再由受支持格式分支建立 blitter 实际使用的 mask 和 shift：

- `7C00/03E0/001F` 变为 `7C00/03E0/001F`，shift 为 `10/5/0`；
- `F800/07C0/003F` 把蓝 mask 收窄为 `003E`，shift 为 `11/6/1`；
- `F800/07E0/001F` 把绿 mask 收窄为 `07C0`，shift 为 `11/6/0`；
- `FC00/03E0/001F` 把红 mask 收窄为 `F800`，shift 为 `11/5/0`。

不受支持的 mask 仍成为当前有效 mask，但不会重写既有 shift 或正反像素转换函数。这种重入继承是原全局状态行为，不能用“未知格式恢复 RGB555 默认值”替代。

## 7. 验证

`tests/unit/rendering/legacy_pixel_conversion_test.cpp` 固定四种受支持格式的有效 mask/shift，以及不受支持格式继承既有 shift 的重入行为。

`tests/unit/rendering/legacy_blitter_test.cpp` 固定：

- 三通道正负偏移、饱和、32 位回绕公式和源颜色不参与结果；
- 正反方向及两侧裁剪后的覆盖位置；
- 三偏移全零时早于 RLE 与 jitter 的返回顺序；
- 常量参数的四字节读取边界与低字填充；
- RGB555 及其余三种受支持格式的灰度结果；
- 六条例程各自的 jitter 组步长和 phase 更新差异。

真实资产继续使用 `all_sys.tsw` 的记录 128、资源 9128、variant 0。解压后的 238 字节 RLE 有 54 个 literal 覆盖像素：

- 以 `0xA55A` 初始化并填充 `0x1234` 后，framebuffer FNV-1a 64 为 `0x96240FB8764F3C39`；
- 以 RGB555 `0x7FFF` 初始化并灰度化后，framebuffer FNV-1a 64 为 `0xCE3B416A93211135`。

Linux Clang 22.1.8 的 `core` 42/42、`app` 44/44 CTest，以及 Windows LLVM `app` 44/44 CTest 均通过。缺少原程序 framebuffer 捕获，所以本单元不能标记为 `original_diff_verified`。
