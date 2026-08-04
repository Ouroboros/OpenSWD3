# 原始色键复制与 RLE 饱和加减色

状态：本行为单元为 `assembly_exact`、`asset_verified`；原程序动态 framebuffer 差分仍为 `blocked_runtime_oracle`。

## 1. 范围与真值

本单元覆盖：

- `0x00417840/0x00417E40`：原始矩形槽 `0x84/0x85` 的色键复制；
- `0x00418840/0x00418EB0`：RLE 槽 `0x04/0x05` 的逐通道饱和加色；
- `0x00422030/0x004223A0`：RLE 槽 `0x2C/0x2D` 的逐通道饱和减色；
- `0x00417452..0x00417462`：每三行跳过效果的初始相位；
- `0x00423746..0x004237B3`：透明色键的格式转换与双字复制。

完整 LST 是主要汇编真值。旧清单曾把 `0x00417840/0x00417E40` 按 mode 名机械标成“原始饱和加色”，但函数体没有任何通道加法；本单元按实际指令把它纠正为色键复制。

## 2. `0x84/0x85`：原始矩形色键复制

`0x00423746` 先把默认色键设为 RGB555 `0x026B`，调用当前正向像素转换器后，在 `0x0042378B..0x004237B3` 取低 16 位并复制到双字的高低两半：

```text
converted_key = forward_convert(0x026B)
global_key = converted_key | (converted_key << 16)
```

正向 `0x00417840` 有两条源布局：

- 直接 16 位源在 `0x0041787F..0x004178A7` 把 `global_key` 先与 `0xFFFF` 相与；源 `u16` 不等于该低字时才复制。
- 索引 8 位源在 `0x00417909..0x00417927` 直接把调色板索引 `1` 当透明；其他索引才读取 `palette[index]` 并写目标。透明索引不会访问调色板项。

反向 `0x00417E40` 保留一个明确的不对称。`0x00417E79` 只在循环前把 EAX 清零，逐像素的 `mov ax,[esi]` 得到零扩展 16 位源值；`0x00417E8F` 却把它与未截断的重复 32 位 `global_key` 比较。正常非零色键的高 16 位也非零，因此比较不会相等，反向路径会把本应透明的色键一并复制。重写必须保留这个原始 BUG。

## 3. `0x04/0x05`：RLE 逐通道饱和加色

三个偏移来自 `0x004CD71C/0x004CD30C/0x004CD304`。三个偏移全零时，`0x00418849..0x0041886A` 与反向对应块选择独立快速循环；每个 literal 像素直接执行：

```text
out_c = saturate((source & mask_c) + (destination & mask_c), mask_c)
```

任一偏移非零时，正反向都先按 x86 32 位回绕调整源通道：

```text
candidate = (source & mask_c) + offset_c * (1 << shift_c)
adjusted_source_c = (candidate & ~mask_c) != 0
    ? (offset_c >= 0 ? mask_c : 0)
    : candidate

out_c = saturate(adjusted_source_c + (destination & mask_c), mask_c)
```

`saturate` 同样通过 `sum & ~mask_c` 判断溢出并返回完整通道 mask，没有跨通道进位。正向像素核位于 `0x00418A34..0x00418A99` 和 `0x00418D53..0x00418DE2`；反向对应核位于 `0x004190BE..0x00419126` 和 `0x00419400..0x00419492`。

## 4. `0x2C/0x2D`：RLE 逐通道饱和减色

两个方向没有全零偏移快速分支。正向 `0x0042222C..0x004222CA` 先调整源通道，再执行目标减源：

```text
out_c = max((destination & mask_c) - adjusted_source_c, 0) & mask_c
```

汇编通过测试 32 位差值的 bit 31 判断负数，负值直接清零。

反向 `0x004225B3..0x00422654` 又有一项必须保留的不对称：它把有符号偏移加到目标通道，而不是源通道，然后再减去未调整的源通道：

```text
out_c = max(adjusted_destination_c - (source & mask_c), 0) & mask_c
```

这不是可交换的重排。UT 使用同一源、目标和非零偏移分别固定正向结果 `0x0460` 与反向结果 `0x0C3E`，防止后续重构把两条路径错误合并。

## 5. 公共 RLE 行为与每三行门控

三条 RLE 效果仍只处理高两位为零的 literal run；`0x4000/0x8000/0xC000` 都只推进覆盖位置。源流始终正向解析，反向例程只反转目标写入方向。

分派器在 `0x00417452..0x00417462` 用裁剪后的目标 Y 建立有符号除法余数：

```text
row_phase = signed_remainder(wrapping_add(clipped_destination_y, 480), 3)
```

当 `0x004CD300 != 0` 时，加色和减色的每个有效物理行都先执行：

```text
++row_phase
if row_phase >= 3:
    row_phase = 0
    consume the physical RLE row without processing its commands
```

该门控位于行长、终止行和可见高度检查之后，命令解析之前；被跳过的行仍访问该行的 jitter 项并按物理行长进入下一行。分派结束在 `0x00417494..0x004174C1` 清除该效果状态。

四条 RLE 例程的 jitter 组步长均为 `0x84` 字节，正常退出都把 phase 加 4，并在 `0x84` 回零。

## 6. 验证

`tests/unit/rendering/legacy_blitter_test.cpp` 固定：

- 直接 16 位色键、索引 1 透明、RGB565 转换后色键；
- 反向色键因 16/32 位比较不对称而实际被复制；
- 加色逐通道溢出、非零源偏移、反向目标遍历；
- 正向减色调整源、反向减色调整目标以及负差归零；
- 基于裁剪后目标 Y 的每三行跳过相位；
- 四条 RLE 例程的 `0x84` jitter 组步长和退出 phase。

真实资产继续使用 `all_sys.tsw` 记录 128、资源 9128、variant 0。解压后 238 字节 RLE 有 54 个 literal 零像素；以 RGB555 `0x4210` 初始化并使用偏移 `+1/-2/+3`：

- 饱和加色 framebuffer FNV-1a 64 为 `0x870AB3FD82D197ED`；
- 正向饱和减色为 `0x44AABC486DCBAD05`；
- 反向饱和减色为 `0xBAF98799C8AA17B9`。

Linux Clang 22.1.8 `core` 42/42、`app` 44/44，以及 Windows LLVM `app` 44/44 CTest 均已通过。
