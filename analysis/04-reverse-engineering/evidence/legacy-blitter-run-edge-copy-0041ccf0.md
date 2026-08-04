# RLE literal run 边缘覆盖复制

状态：本行为单元为 `assembly_exact`、`asset_verified`；原程序动态 framebuffer 差分仍为 `blocked_runtime_oracle`。反向例程可能写到 framebuffer 之前的情况以显式边界错误隔离，记为 `platform_adapted`。

## 1. 范围与真值

本单元覆盖：

- `0x0041CCF0..0x0041D00A`：RLE 槽 `0x18` 正向例程；
- `0x0041D010..0x0041D336`：RLE 槽 `0x19` 反向例程；
- `0x00423610..0x0042361C`、`0x0042375C..0x0042376B`：边缘替换值的建立与转换；
- `0x00416E76/0x00416E80`：两项分派表赋值。

完整 LST 是本单元的汇编真值。此前概括把两个方向都写成“覆盖实际复制的首尾像素”，但反向函数的指针更新并不对称；本单元按实际指令修正该结论并保留原始 BUG。

## 2. 边缘替换值

`0x00423610..0x0042361C` 先把格式选择后 EBX 中的有效绿色 mask 复制到双字高低两半。`0x0042375C..0x0042376B` 随后只把其低 16 位作为一个像素再次交给当前正向转换器；blitter 最终也只读取低 16 位。

因此实现必须计算：

```text
edge_pixel = forward_convert(low16(effective_green_mask))
```

这不总是最终像素格式的绿色 mask：

- RGB555 与红色字段左移格式得到 `0x03E0`；
- 整字左移格式与 RGB565 得到 `0x0F80`。

RGB565 的 `0x0F80` 看起来会越过常规绿色字段，但这是初始化指令的直接结果，不能美化成 `0x07C0` 或 `0x07E0`。

## 3. 正向 `0x0041CCF0`

literal run 经过源窗口左右裁剪后，`0x0041CE55..0x0041CEB3` 复制实际可见的源像素。复制前的 EDI 保存在局部变量，复制后的 EDI 指向最后一个写入像素之后；随后：

```text
first_actual = destination_before_copy
last_actual  = destination_after_copy - 2
*first_actual = edge_pixel
*last_actual  = edge_pixel
```

因此边界属于每个实际可见的 literal run 段，而不是 sprite 外框。被左侧裁掉的源前缀不算首像素；长度一时同一目标像素写两次。高两位非零的三种 run 都只推进覆盖位置。

正向例程使用 `0x84` 字节 jitter 组步长，并在包括 header bit `0x10` 未置位在内的正常退出路径把全局 phase 加 4，达到 `0x84` 时回零。

## 4. 反向 `0x0041D010` 的越界一像素 BUG

反向复制前，`0x0041D192..0x0041D197` 保存 `EDI-2`，即即将写入的最右侧首像素。复制循环每次先把 EDI 减 2 再写入；复制结束时 EDI 已经指向最左侧最后一个实际写入像素。

`0x0041D1F4..0x0041D20C` 的第二次覆盖却再次执行 `sub edi,2`：

```text
rightmost_actual = destination_before_copy - 2
leftmost_actual  = destination_after_copy

*rightmost_actual       = edge_pixel
*(leftmost_actual - 2)  = edge_pixel
```

所以反向模式不覆盖最左侧实际复制像素，而会改写 literal run 左边紧邻的一个目标像素。长度一时也会写两个不同位置。该行为由明确的 `sub edi,2` 产生，重写保留它，不能改成表面对称的“首尾覆盖”。

如果额外位置仍在 owned framebuffer 内，正常写入并可观察；如果它位于 framebuffer 之前，原程序会越界写内存，现代实现完成已在范围内的写入后返回 `destination_out_of_bounds`，不复制未定义的进程内存破坏。

反向例程同样使用 `0x84` 字节 jitter 组步长，但 `0x0041D330` 直接退出，不推进全局 phase。

## 5. 验证

合成 UT 固定：

- 同一行两个 literal run 分别覆盖自己的正向可见首尾；
- 左侧裁剪后，以第一个实际写入像素作为正向首边；
- 反向三像素 run 得到“右端实际像素 + 左端外一像素”，左端实际像素保留源值；
- RGB565 边缘值为 `0x0F80`；
- 正向 phase 加 4、反向 phase 保持不变；
- 反向额外位置越过 owned framebuffer 时返回显式边界错误。

真实资产使用 `all_sys.tsw` 记录 128、资源 9128、variant 0。独立通过系统 `liblzo2` 解压并解析后，238 字节 RLE 中有 6 个 literal run、54 个 literal 像素。以 `0xA55A` 初始化 RGB555 framebuffer 后：

- 12 个 run 端点像素为 `0x03E0`；
- little-endian framebuffer FNV-1a 64 为 `0x28FED51BD6E4E461`。

Linux Clang 22.1.8 `core` 42/42、`app` 44/44，以及 Windows LLVM `app` 44/44 CTest 均已通过。
