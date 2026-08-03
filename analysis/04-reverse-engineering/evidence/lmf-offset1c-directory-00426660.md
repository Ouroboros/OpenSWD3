# LMF 地图头 `+0x1C` 原样记录目录 `0x00426660..0x00426798`

状态：B2.20 已实现；`assembly_exact`、`asset_verified`、`platform_adapted`、`blocked_runtime_oracle`

完整汇编是唯一行为真值。IDA 伪码和符号名只用于定位。本单元恢复地图头 `+0x1C` 指向的相对偏移表、顺序变长记录与原始名称；记录进入世界对象数组后的坐标、筛选和初始化留给 `world_map`。

## 1. 定位与固定读取

`0x00426660..0x00426697` 以 32 位加法计算 `map_offset + header.offset_1c`，调用一基 begin seek，随后固定请求读取 0x10000 字节。原调用点不检查 seek 和 read 返回值，并在两次 I/O 后调用 `AIL_serve`。

原程序复用同一个 128 KiB 缓冲，短读或失败后可能继续观察旧内容。独立 C++20 入口无法也不应伪造未初始化或上次读取残留；失败 seek、失败 read、实际窗口越界和无终止 NUL 因此作为 `platform_adapted` 安全状态停止。合法资产的读取顺序和字节解释保持不变。

## 2. 目录与记录布局

`0x0042669C..0x004266AD` 读取首 dword，并用 `lea esi, [buffer + count*4 + 4]` 跳过声明偏移表：

```text
u32 record_count
u32 declared_relative_offset[record_count]

repeat record_count:
    u16 field_00
    u16 field_02
    u16 field_04
    u16 field_06
    u16 packed_field_08
    byte name_until_and_including_NUL[]
```

循环条件是有符号 `count > 0`。零和负计数都直接跳过；现代入口保留该控制流。正计数的声明偏移只决定首条记录指针，循环内不按表项重新定位。B2 接口同时公开声明值和实际顺序相对位置，便于验证而不改变解析行为。

每条记录在固定十字节后只扫描到 NUL，并不把名称传给后续运行时构造。资源层仍原样交付包含终止 NUL 的名称字节，不提前解码为 Unicode。

## 3. 汇编消费边界

`0x004266BB..0x0042670A` 对物理字段执行以下转换：

- `field_00`、`field_02` 零扩展到运行时对象 `+0x40`、`+0x48`；
- `field_04`、`field_06` 零扩展后左移四位，写入坐标 `+0x04`、`+0x08`；
- `packed_field_08 & 3` 写入类型；
- `packed_field_08 >> 12` 写入 word `+0x28`；
- `(packed_field_08 >> 8) & 0x0F` 写入 word `+0x2A`。

随后代码设置运行时标记、把类型并入 `0x9000`、按地图高度筛除记录，并可能调用 `0x00411490`。这些是世界对象数组和场景业务行为，不属于资源容器解析；B2 只保留五个物理 word、相对位置和原始名称。

## 4. 验证

合成 UT 覆盖两条记录、全部物理字段、packed word、双字节原始名称、声明偏移不参与逐条定位、负计数跳过、失败 seek、偏移表乘法回绕和无终止名称。

真实 `huge.lmf` 的 309 张地图中，282 张目录为四字节零计数，27 张共有 329 条记录，单图最多 81 条。全部声明偏移等于顺序记录位置，全部最终游标精确等于地图头 `+0x20`。按尾索引和记录顺序计算：

- 相对位置和五个物理 word 的 FNV-1a64：`0x850EBBDFD6772EF6`；
- 包含终止 NUL 的原始名称 FNV-1a64：`0x4AD20ABEAA5E0C02`。

Windows LLVM `core` 与 `app` 均通过 33/33 CTest，两套构建产物对真实 `huge.lmf` 的显式全量测试均以零退出；原程序动态轨迹仍登记为 `blocked_runtime_oracle`。
