# 描边包装、固定 16×16 tile writer 与 packed-row 混合

状态：正常可寻址输入为 `assembly_exact`；危险输入保留显式宿主安全隔离

本单元只以完整 `swd3.exe.lst` 为行为真值，闭环六个无平台依赖的软件绘制函数：

| 地址 | 行为 | OpenSWD3 映射 |
|---|---|---|
| `0x00417050` | 四次对角偏移的常量色描边包装 | `blit_legacy_outline_copy_paths` |
| `0x004174D0` | direct-16 不透明 16×16 tile | `write_legacy_direct_16x16_tile` |
| `0x00417530` | direct-16 色键 16×16 tile | `write_legacy_direct_keyed_16x16_tile` |
| `0x004175B0` | indexed-8 不透明 16×16 tile | `write_legacy_indexed_16x16_tile` |
| `0x00417650` | indexed-8 索引 1 透明 16×16 tile | `write_legacy_indexed_keyed_16x16_tile` |
| `0x00417DE0` | 两像素一组的四项 packed 混合 | `blend_legacy_packed_row` |

## 1. `0x00417050` 四向描边

六个 cdecl 参数与 `sub_4170E0` 相同：`x/y/width/height/flags/auxiliary`。
包装器先对 flags 强制执行 `OR 0x24`，再严格按以下顺序调用现有公共 blitter：

```text
1. (x + 1, y + 1)
2. (x - 1, y - 1)
3. (x - 1, y + 1)
4. (x + 1, y - 1)
```

坐标增减是 32 位回绕。四次调用共享同一个全局 jitter/phase 状态，即使前一条返回
错误也不跳过后续调用；原函数最终只留下第四次调用的 EAX。现代结果额外保存四次
状态用于诊断，但不改变调用顺序和共享状态推进。

正常 source 是 RLE，`0x24/0x25/0x26/0x27` 分别落入既有正反常量填充分支；
`auxiliary` 至少提供四字节，低 word 是实际填充值。原调用点会在栈上放置两个相同
16 位颜色构成的 dword，例如 `0x07E007E0`。

## 2. `0x004174D0/0x00417530` direct-16 tile

两个参数都是目标 `(x,y)`。目标首地址按旧行表等价计算：

```text
framebuffer + row_byte_offsets[y] + 2*x
```

`0x004174D0` 固定复制 16 行，每行 32 byte；每行内部按四轮、每轮两个 dword
搬运，source 因而固定消费 512 byte。目标每行按实际 framebuffer byte pitch 推进，
source 行无 padding。

`0x00417530` 同样消费 256 个 little-endian `u16`，但逐像素与
`low16(0x004CD784)` 比较；相等时目标保持原值，不相等才写入。EAX 在循环前清零，
随后只写 AX，因此与低 16 位 key 的 32 位比较没有未知高位。

两条真实调用链以 tile 描述 bit `0x04000000` 选择不透明或色键版本；调用者已按
16×16 网格限制目标范围，writer 自己没有裁剪。

## 3. `0x004175B0/0x00417650` indexed-8 tile

两函数固定消费 256 个 source byte，并从 `0x004CD764` 指向的 256 项 `u16`
palette 直接取当前 framebuffer 格式的颜色。目标仍是 16×16、按实际 byte pitch
逐行推进。

`0x004175B0` 每个索引都写入，原汇编以四像素展开、每行执行四轮。
`0x00417650` 以两像素展开、每行执行八轮；source index 恰好等于 `1` 时不查
palette 也不写目标，其他 255 个值正常查表。透明值不是零、不是 palette 色值，
也不是可配置 key。

真实调用者从 `(tile_id + 2) << 8` 取得 source；与 direct-16 路径相同，由 tile
描述 bit `0x04000000` 选择不透明或透明 writer。

## 4. `0x00417DE0` packed-row

参数是 `destination/color_pattern/pixel_count`。入口先对无符号 pixel count 右移一位，
因此每轮处理连续两个 `u16` 组成的一个 `u32`，奇数尾像素保持原值。记目标 pair 为
`D`，颜色 dword 为 `C`，当前像素格式生成的双 lane mask 为 `M1..M4`：

```text
pair_count = unsigned(pixel_count) >> 1
out = ((D >> 1) & M1)
    + ((C >> 2) & M2)
    + ((D >> 3) & M3)
    + ((D >> 4) & M4)
```

四项使用普通 32 位加法，没有逐通道饱和、浮点 alpha 或奇数尾补写。该公共 helper
现在同时承接矩形效果 mode 0，以及 `0x00414E50` 行特效的 framebuffer port，消除了
B4.9c 中登记的最后一个底层像素端口缺口。

## 5. 安全隔离

四个 tile writer 的原始实现直接写全局指针，不验证 source、palette 或目标；现代接口
在任何写入前验证 512/256-byte source、256 项 palette、实际 pitch 和完整 16×16
目标范围。正常调用的像素顺序不变，异常输入只返回明确状态并阻止宿主内存破坏。

`0x00417DE0` 对 `pixel_count=0/1` 得到零 pair count，随后先减一再判断非零，原程序会
进入 `0xFFFFFFFF` 次循环；负值右移后同样形成巨大计数。现代 helper 对 `<2` 返回
`invalid_geometry`，目标 span 不足则返回 `destination_out_of_bounds`。这两类不声明为
`assembly_exact`，与既有矩形效果安全合同一致。

## 6. 验证

独立 UT 固定并通过：

- 四次描边的 offset 集合、调用结果、强制 `0x80000024` 选择和中心不写；
- direct-16 首/末像素、行跨度、色键保持及 512-byte 边界；
- indexed-8 全索引查表、固定 index 1 透明、256-byte/256-entry 边界；
- packed dword 四项公式、RGB555 固定向量、奇数尾保持、零 pair 非终止安全隔离；
- `LegacyFramebufferPackedRowDrawPorts` 的实际行寻址和负坐标隔离；
- 原有矩形 mode 0 向公共 helper 重绑定后像素向量不变。

Linux Clang `core` 为 61/61、Windows LLVM `app` 为 63/63 CTest。原程序
framebuffer 动态差分继续使用 B4 已登记的 `blocked_runtime_oracle`；本轮没有启动
原版程序。
