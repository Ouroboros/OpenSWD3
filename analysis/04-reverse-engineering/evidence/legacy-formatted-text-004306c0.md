# 格式化原始字节文字 `0x004306C0`

状态：正常游戏输入路径已按完整 LST 逐基本块复核并实现；原程序会越过输入或
64 字节栈缓冲的异常输入由有界状态显式隔离。

## 证据范围

唯一行为真值是 `swd3.exe.lst`：

- 主函数：`sub_4306C0`，`0x004306C0–0x004308BB`；
- 颜色打包：`sub_4239D0`，`0x004239D0–0x00423A0D`；
- 最终原始字节文字绘制：`sub_436AD0`；
- 20 像素 renderer 初始化：`0x0040A881–0x0040A8E9` 和
  `0x00424CE6–0x00424D23`。

完整 LST 中只有四个直接调用点：`0x0043BE33`、`0x0043CED7`、
`0x00444D96`、`0x0044D0D5`，没有发现取得函数地址后再间接调用的路径。

## 实际调用合同

四个调用点都压入六个 dword，并在调用后清理 `0x18` 字节：

```text
(text, x, y, maximum_line_count, maximum_width, ignored_4)
```

函数体只读取前五项；第六项在四处都等于 `4`，但从未读取。IDA 给出的五参数
原型只能描述被读取的栈槽，不能推翻调用点实际存在的第六项。

| 调用点 | text | x | y | 最大行数 | 最大宽度 | 未读项 |
|---|---|---:|---:|---:|---:|---:|
| `0x0043BE33` | 调用参数 | 220 | `dword_4FC830` | 5 | 360 | 4 |
| `0x0043CED7` | 对象 `+0xAC` | 242 | 336 | 5 | 360 | 4 |
| `0x00444D96` | 记录 `+0xAC` | 220 | `dword_4FD07C` | 5 | 360 | 4 |
| `0x0044D0D5` | 脚本文字指针 | 调用参数 | 调用参数 | 2 | 340 | 4 |

## 进入函数时的 renderer 状态

函数每次都先修改全局 20 像素 renderer `word_4AB998`，且退出前不恢复：

- 背景色写成 `0xFFFE`，即禁用背景；
- secondary color 写成 `sub_4239D0(6,4,3)`；
- 初始前景色为 `sub_4239D0(25,23,17)`。

`sub_4239D0` 先按 `((r&31)<<10)|((g&31)<<5)|(b&31)` 组成 RGB555，
复制到两个 16 位 lane，再调用当前 forward 像素转换。RGB555 identity 下两种
颜色的低 16 位分别是 `0x1883` 和 `0x66F1`。实现把这一共享合同收口为
`legacy_pack_color_pair`，矩形效果和本函数不再各自复制公式。

这里的 20 像素 renderer 自身固定 advance 是 24；本函数另用
`dword_4A99F8 == 11` 计算换行和分色段起点。两者不是同一参数，不能合并：
ASCII 在布局计数中占一个 byte unit（11 像素），高位双字节占两个 unit
（22 像素），而 `sub_436AD0` 在段内部仍按 renderer 的 12/24 像素推进。

## 原始字节和三个控制标记

本函数不解码 Unicode，也不验证 Big5：

- 当前 byte `<0x80` 时原样复制一个 byte；
- 当前 byte `>=0x80` 时无条件再复制一个 byte；
- 复制 byte 数同时是布局 unit 数；
- 每次交给 `sub_436AD0` 的临时段都补 NUL。

控制标记是逐 byte 比较的 ASCII 大写字面量：

| 标记 | 行为 |
|---|---|
| `%Q` | 不消费标记，终止解析并执行最后一次绘制；允许最后一段为空 |
| `%N` | 先绘制当前段；若仍可增加行，则消费两 byte、x 复位、y 增加 25 |
| `%C?` | 先绘制当前段，不换行；消费三 byte，并令新前景色为 `signed(?) - '0'` |

`%C` 不要求第三 byte 是十进制数字，也不查颜色表。例如第三 byte 为 `0xFF`
时，传给 `sub_436AD0` 的前景色低 16 位为 `0xFFCF`。每个段调用的 style 都由
函数体硬编码为 `4`；这与调用者额外压入但函数不读取的 `4` 是两个独立事实。

## 宽度、坐标和分段

记 `F` 为本行此前因 `%C` 已绘制的 byte 数，`S` 为当前临时段 byte 数：

```text
segment_x = x + 11 * F
occupied_width = 11 * (F + S)
line_y = y + 25 * completed_line_break_count
```

全部运算保留 x86 32 位回绕和有符号比较。宽度测试发生在复制当前字符之前，
条件是 `occupied_width > maximum_width`，不是 `>=`，也没有先把当前字符宽度
加入。因此恰好占满时仍会再接受一个字符；只有处理下一个 token 前才换行。
因宽度触发换行时当前 token 不消费，会在新行重新处理。

每次 `%C` 先在 `x+11*F` 绘制旧颜色段，然后执行 `F += S`。最终绘制即使段
为空也会调用 `sub_436AD0`，原函数返回值就是这最后一次调用的返回值；四个
调用者均不使用它。

## 达到最大行数后的旧异常

显式换行或宽度溢出都会先绘制并清零 64 字节临时段。只有
`current_line < maximum_line_count` 时，函数才把 `F/S` 清零、增加 y 并在
`%N` 情况下消费标记。

若已经达到最大行数，临时段虽已清零，`F/S` 计数却保留，随后当前 `%N` 会
按普通 `%`、`N` 两个 byte 再进入新段；后续 `%C` 还会把这份陈旧计数加入
横坐标。实现和 UT 原样保留这个可观察 BUG，没有把它修正为截断或正常换行。

## 64 字节栈缓冲与安全边界

原函数的临时段是 64 字节栈数组。正常四个调用者的最大宽度为 340/360，宽度
逻辑会把段限制在远低于危险长度的范围。异常调用若允许一个段写入 64 个数据
byte，随后补 NUL 会越过数组；悬空高位 byte、被截断的 `%C` 或完全缺少
`NUL/%Q` 也会继续读出输入范围。

现代有界接口只对这些原程序本就没有稳定语义的输入返回：

- `missing_terminator`；
- `dangling_double_byte`；
- `truncated_color_control`；
- `segment_buffer_overflow`。

有效段最多保留 63 个数据 byte 加一个 NUL。安全状态不改变四个真实调用者的
路径，也没有修复达到最大行数后的游戏逻辑 BUG。

## 实现与验证

- `include/openswd3/rendering/legacy_formatted_text.hpp`；
- `src/rendering/legacy_formatted_text.cpp`；
- `tests/unit/rendering/legacy_formatted_text_test.cpp`。

布局核心以 `LegacyFormattedTextSegmentSink` 暴露每一次 `sub_436AD0` 边界，UT
因此能直接验证空段、原始 byte、颜色、坐标、调用顺序和旧计数异常，而不是只
从最终 framebuffer 反推。完整 adapter 再接入既有 `draw_legacy_text`、缓存和
GlyphProvider，并验证 background/secondary 的永久状态更新及 style `4`
footprint。

当前验证结果：Linux Clang `core` 53/53、Windows LLVM `app` 55/55 CTest
全部通过；验证过程没有启动原版或 OpenSWD3 EXE。
