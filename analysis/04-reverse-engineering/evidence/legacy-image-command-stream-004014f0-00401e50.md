# legacy image command-stream 编解码与像素转换

状态：`assembly_exact`、`asset_verified`；原程序动态差分待统一 oracle

完整 LST `0x004014F0..0x0040202D` 是唯一行为真值。IDA 伪码只用于定位变量，
不覆盖指令控制流。本单元对应五个函数：

| 地址 | 行为 | OpenSWD3 映射 |
|---|---|---|
| `0x004014F0` | raw 8/16-bit 图像编码为逐行 command stream | `encode_legacy_image_command_stream` |
| `0x004019A0` | command stream 解码为连续 raw pixels | `decode_legacy_image_command_stream` |
| `0x00401B70` | 原地转换 16-bit literal payload | `convert_legacy_image_command_stream_literals_in_place` |
| `0x00401C70` | 外置 palette 的 indexed8 stream 转 direct16 | `convert_legacy_image_command_stream` |
| `0x00401E50` | 512-byte 内嵌 palette 的 indexed8 stream 转 direct16 | `convert_legacy_embedded_palette_image_command_stream` |

## 1. 物理格式

所有多字节字段均为小端。8-byte 头为：

```text
u16 0xFFFF
u16 width
u16 height
u16 format
```

其后每行先有一个 `u16 row_size_and_flag`，再有 command 与 payload，最后以
`u16 0` 结束该行。全部行后还有一个独立 `u16 0` 行哨兵。command 高两位与
低十四位含义为：

| 高两位 | 行为 |
|---|---|
| `0x0000` | literal；低十四位是像素数，随后是 `count` 个 byte 或 word |
| `0x8000` | 重复特殊值；8-bit 为 `3`，16-bit 为 `0x319F` |
| `0xC000` | 重复特殊值；8-bit 为 `1`，16-bit 为 `0x026B` |
| `0x4000` | 原解码器不输出像素，直接继续下一 command |

解码器不依据行头低位跳转，也不依据 `height` 限制行数；它只沿 command 直到
行零和最终行哨兵。行头仍由编码器和两个 indexed 转换器按实际输出字节数重建。

## 2. `0x004014F0` 编码

- 只有 `format == 16` 走 word 分支；其他值全部走 byte 分支，并把原值写回头部。
- word 分支只把 `0x319F` 和 `0x026B` 压为两种特殊 command；其他值组成 literal。
- byte 分支对应的特殊值固定为 `3` 和 `1`；literal payload 可以是奇数字节，所以下一
  个 `u16` command 可以处于非对齐地址，不能用对齐后的宿主结构解释。
- word 行头的 `0x8000` 只在首像素不是两种特殊值时设置。
- byte 行头的 `0x8000` 只在首像素不是 `1` 时设置；首像素为 `3` 时仍设置。这是
  `0x004017B4..0x004017D6` 的确定行为，不能概括成“首 command 是 literal”。
- `width == 0 && height != 0` 时原 do-while 仍读取并编码每行一个像素。实现保留该
  可观察读取合同；安全解码器随后把它报告为目标像素数不匹配。

`0x319F/0x026B` 是 command stream 固定标记。`0x00423746..0x00423785` 只对其
他渲染副本调用 forward converter；`0x004CDE20/0x004CDE78` 自身保持 RGB555
基准。因此这里不能先把两个 stream 标记转换成 RGB565。

## 3. `0x004019A0` 解码

- 对 `format` 使用 `0x3FFF` 掩码，只接受 8 或 16；宽、高和掩码后的深度先成为
  输出合同，再选择分支。
- literal 原样复制；两种特殊 command 展开为上述固定标记。
- `0x8000/0xC000` 的低十四位为零时，原 do-while 会执行 65,536 次；正常编码流
  不产生可安全消费的这种状态。
- 原函数按 `width*height*bytes_per_pixel` 分配后不做界限检查。现代实现对截断输入
  和目标越界返回显式状态，隔离旧堆越界；正常 command 顺序、非对齐读取和像素结果
  不变。

## 4. `0x00401B70/0x00401C70/0x00401E50` 转换

`0x00401B70` 对深度使用 `0x7FFF` 掩码，只接受 16。它遍历全部行，仅对 literal
word 调用当前 forward pixel converter；三种非 literal command、行头和头部均保持
原字节，同时向可选输出发布宽、高和掩码后的深度。

`0x00401C70` 的 indexed8 分支：

- 把每个 literal byte 作为 0..255 palette 下标，取出 RGB555 word 后逐像素调用
  forward converter；
- 特殊 command 原样复制，不转换语义标记；
- 输出 `format = (input_format & 0x8010) | 16`；
- 每行重新写入 direct16 的真实行字节数，不复制输入行头高位；
- 输入不是 indexed8 时转交 `0x00401B70` 的 16-bit 原地转换合同。

`0x00401E50` 使用输入开头 512 字节作为 256 项小端 `u16` palette，command stream
从 `+0x200` 开始，其转换规则与上段一致。`0x00401FEB` 把最终复制长度截断到
`u16`；实现保留该长度回绕，而不复制旧分配器可能造成的堆越界机制。

## 5. 验证

独立 UT 固定了两套逐字节向量：

- `6×2×16`：44-byte stream，覆盖两种特殊 run、多个 literal、行头高位和 roundtrip；
- `6×2×8`：38-byte stream，覆盖奇数 payload 导致的非对齐 command、byte 行头异常、
  palette 展开及 RGB555→RGB565。

真实资产门使用 `huge.lmf` 地图 72 的第一个 indexed object。该块确实由
`0x0042660E` 解压后流向 `0x00401B70`：

| 项 | 结果 |
|---|---:|
| command stream 头 | `1072×1024×16` |
| packed bytes | `1,790,338` |
| RGB565 literal 转换后 packed FNV-1a 64 | `a70ae50b232b53de` |
| 解码 raw bytes | `2,195,456` |
| 解码 raw FNV-1a 64 | `3c444615b499c161` |

Linux Clang `core` 为 56/56 CTest，Windows LLVM `app` 为 58/58 CTest，均包含上述
真实资产用例。
