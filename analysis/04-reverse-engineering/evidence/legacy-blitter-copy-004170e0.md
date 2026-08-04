# 稀疏 blitter 分派与基础 copy 路径

状态：`assembly_exact`、`asset_verified`；原程序动态 framebuffer 差分仍为 `blocked_runtime_oracle`。

## 1. 范围与真值

本单元只实现：

- `0x00416D90..0x00416F0E` 的 256 槽稀疏表；
- `0x004170E0..0x004174CA` 的普通裁剪、源族选择和最终分派；
- `0x004176D0`、`0x004177D0` 两条 raw copy；
- `0x00418350`、`0x004185C0` 两条 RLE/span copy。

完整 LST 是唯一行为真值。伪码只用于导航，未用于决定分支、类型或循环边界。颜色运算、透明度、重采样和其余已赋值效果留给 B4.5。

## 2. 分派合同

分派器的六个物理参数依次是目标 X、目标 Y、源宽、源高、flags 和辅助指针。源像素指针、调色板指针、framebuffer、pitch、clip 和临时裁剪量来自原全局状态；重写接口把这些输入显式化，但不改变其计算次序。

入口先无条件读取源首个 `u16`：

```text
source[0] == 0xFFFF && palette_pointer == 0
    => flags |= 0x80000000

flags bit31 == 1
    => slot = flags & 0xFFFF

flags bit31 == 0
    => slot = (flags & 0xFFFF) + 0x80
```

因此低槽是 RLE/span，`+0x80` 是 raw；不是相反。表中恰有 43 个已赋值槽、31 个唯一地址。空槽和表外索引没有 copy fallback。

`(flags & 0x0000FFFC) == 0x14` 时还要先应用全局透明度门：

- `opacity <= 0`：整次绘制跳过；
- `opacity > 15`：执行 `flags &= 0x80000003`，只保留源族和两个翻转位，退化到 copy；
- `1..15`：保留效果槽，本单元返回“已赋值但尚未实现”。

## 3. 普通裁剪与行寻址

无重采样、无逐行位移时，裁剪顺序严格为纵向后横向。纵向记录顶部与底部被裁行数，横向记录左侧被裁像素数。所有加减乘都按 32 位 x86 回绕后再作有符号比较。

目标首地址为：

```text
framebuffer + row_offsets[visible_y] + visible_x * 2
```

分派器还无条件读取 `row_offsets[visible_height]` 并建立目标末地址。bit 1 置位时：

```text
source_top_skip = source_bottom_skip
destination_start = destination_start + row_offsets[visible_height]
destination_row_step = -pitch
```

旧行表只初始化 `0..surface_height-1`，所以 `row_offsets[visible_height]` 可能读取保留尾值。实现没有把它改成 `(visible_height-1)*pitch`；UT 固定了由此产生的纵翻一行偏移。

完全裁掉时原函数在最终间接调用前返回，所以即使已选中空槽，也不会触发空指针调用。实现同样先完成裁剪，再报告可见空槽。

## 4. raw copy

`0x004176D0` 的正向函数按调色板指针区分两种源：

- 指针为零：逐 `u16` 复制，每行源步长为 `source_width*2`；
- 指针非零：逐 `u8` 取索引，再读取 `palette[index]`，每行源步长为 `source_width`。

bit 0 置位时，分派器先把源 X 改成：

```text
source_width - left_skip - 1
```

随后 `0x004177D0` 始终逐 `u16` 倒读，完全没有 indexed 分支。即使调色板指针非零，源首偏移仍按字节索引图计算，但循环继续执行未对齐 `u16` 读取、每像素减 2、每行净增 `source_width*2`。实现和 UT 保留这个危险的不对称合同，没有擅自进行反向调色板转换。

raw 的 `0x82/0x83` 没有赋值，所以 bit 1 不能执行 raw copy。

## 5. RLE/span copy

两条 RLE 函数先检查源 `[+6] & 0x10`；未置位便正常返回且不写像素。有效流从 `source+8` 开始：

- 每行首个 `u16` 是物理行长，跳转使用 `length & 0x7FFF`；
- 行长为零结束整张图；
- 命令低 14 位是 run 长度，零结束当前行；
- 高两位为零时，命令后携带 `run` 个 literal `u16`；
- 高两位任一置位时，本 copy 族只推进覆盖位置，没有像素载荷；
- literal 中的 `0x0000` 和 `0xFFFF` 都原样写入，不是 colorkey。

正向可见源窗口从 `left_skip` 开始。反向窗口从：

```text
source_width - visible_width - left_skip
```

开始；源命令和 literal 仍向前消费，只把目标写入方向改成从右到左。因此反向不是倒序解析压缩流。

顶部裁剪只按物理行长前进源行。bit 1 只替换顶部跳过量和目标行步长，源 RLE 行始终向前。

## 6. 逐行抖动状态

`0x004CD724 != 0` 时，两条 RLE copy 都使用 33 个 `i32` 为一组的行偏移表：

```text
base = 0x84 * (group - 1)
cursor = base + phase
每次读取一条待处理行前：cursor += 4，越过组尾则回到 base
destination += offsets[cursor / 4] * 2
```

偏移在测试行是否为终止行以及是否超过可见高度之前就应用。`0x00418350` 返回时把全局 phase 加 4，并在 `0x84` 回零；`0x004185C0` 没有对应更新。这一正反不对称来自完整 LST，不能合并成对称 helper 后统一推进。

## 7. 现代安全边界

原代码会直接调用空槽，也没有源、调色板、目标和抖动表容量参数。重写不能在 C++ 中复制任意地址执行或越界内存访问，因此用显式状态隔离这些异常：

- 空槽/表外槽：`unassigned_routine`；
- 已赋值但属于 B4.5 的槽：`unsupported_routine`；
- 截断源或非法 RLE：`malformed_source`；
- 调色板、目标或抖动表越界：对应边界错误。

这些状态都不会回退到其他像素算法。它们只替代原本无法在现代安全进程中定义的崩溃/越界边界，不改变有效输入的像素结果。

## 8. 验证

`tests/unit/rendering/legacy_blitter_test.cpp` 固定：

- 全部 43 个已赋值槽和 31 个唯一地址的数量，以及关键共享槽和空槽；
- RLE 自动标记、indexed 抑制标记、透明度门和无 fallback；
- raw 正向/反向、四边裁剪、padding、indexed 调色板和 indexed 反向未对齐读；
- RLE 三种高位 skip、literal `0/FFFF`、正反方向、水平裁剪和纵翻旧偏移；
- 抖动表首项时序、终止行访问、正向 phase 推进与反向不推进；
- `[+6] & 0x10` 未置位的正常空操作。

真实资产样本来自 `all_sys.tsw`：记录序号 128、资源 9128、variant 0、16 位、`16x16`。压缩流为 76 字节，SHA-256 为 `2d249843094cee2847c1d91cca1c3ace3dffa95ae17e07b873c5c7c127d7b18d`；经已闭环解压器得到 238 字节 RLE。以 `0xA55A` 初始化目标后，copy 结果的 little-endian FNV-1a 64 为 `0x7C38744AC87B8BE1`，其中 54 个 literal 零像素确实覆盖目标。

Linux Clang 22.1.8 的 `core` 42/42、`app` 44/44 CTest，以及 Windows LLVM `app` 44/44 CTest 均已通过。
