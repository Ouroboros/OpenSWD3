# LMF 地图头 `+0x14` 记录目录 `0x004262CC..0x0042641E`

状态：B2.18 已实现；`assembly_exact`、`asset_verified`、`platform_adapted`、`blocked_runtime_oracle`

完整汇编是唯一行为真值。IDA 伪码和符号名只用于定位。本单元把 `0x004262CC..0x0042641E` 切为两层：B2 只恢复地图头 `+0x14` 指向的物理目录；坐标缩放、位域拆分、运行时对象初始化和视野筛选属于 `world_map`，不在资源层提前实现。

## 1. 固定窗口与顺序记录

`0x004262CC..0x004262E5` 按 32 位加法 seek 到 `map_offset + header.offset_14`，这一次明确检查一基 seek 返回值，零时走地图装载失败。随后固定请求读取 `0x10000` 字节到暂存区，但不检查读取返回值。

暂存区物理组织为：

```text
u32 count
u32 declared_relative_offset[count]
variable_record records[count]
```

`0x00426308..0x0042631A` 保存计数后，直接把顺序游标设为 `window + 4 + count * 4`。与 B2.16 相同，后续循环不按声明偏移定位记录。

## 2. 记录字段与名称边界

每条顺序记录的固定前缀为 12 字节：

```text
+0x00  u16 field_00
+0x02  i16 field_02
+0x04  u16 field_04
+0x06  u16 field_06
+0x08  i16 field_08
+0x0A  u16 field_0a
+0x0C  byte name[]，包含终止 NUL
```

`+2` 和 `+8` 在 `0x00426378/0x00426384` 由 `movsx` 明确按有符号 word 读取；其他字段保持物理 word。`0x004263EC..0x004263F8` 从固定前缀后逐字节跳到 NUL 后，形成下一记录起点。名称内容在这段原代码里不复制，但仍是决定物理游标的格式字段。

循环中同时出现 `field_00 - 1`、有符号字段乘 16、`field_0a` 位域拆分、运行时对象构造和依据调用者状态的筛选。这些是物理记录的消费者行为，将随 `world_map` 按相同汇编地址实现；B2 只交付原始字段，避免形成跨模块反向依赖。

## 3. 实现与验证

`legacy_lmf_read_offset14_directory` 保留固定 64 KiB 请求、计数和表后顺序游标、六个 word 的符号宽度、NUL 名称边界及 32 位相对位置。高位/过大计数、窗口越界、位置溢出和无终止名称返回确定状态，隔离原程序的失控指针与越界读取，属于 `platform_adapted` 损坏文件边界。

合成 UT 覆盖负 `i16`、未解释 word、原始双字节名称、声明偏移被篡改仍顺序解析、下一段边界和损坏输入。真实 `huge.lmf` 中 273 张地图目录非空，共 5,471 条记录，单图最多 168 条；全部声明偏移等于顺序记录位置，全部目录末端精确等于地图头 `+0x18`。按尾索引顺序拼接每条记录的相对偏移和六个物理 word，FNV-1a64 为 `0xA984705A3EDEE9FD`。Windows LLVM `core` 的 33/33 CTest 与显式真实档案测试均通过；原程序动态轨迹仍登记为 `blocked_runtime_oracle`。
