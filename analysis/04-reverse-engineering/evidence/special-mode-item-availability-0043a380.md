# 标准特殊模式可用项目状态 `0x0043A380`

状态：`platform_adapted`

## 1. LST锁

权威范围为`swd3.exe.lst`的`0x0043A380..0x0043A46C`。唯一调用者是`0x0043A2A0`，固定传入`5`；唯一callee是已关闭的剧情标志查询`0x0040DC50`。

函数扫描4个固定记录，步长为`0x1C`，基址从`0x004A6A38`递增至但不包含`0x004A6AA8`。`0x004A6AA8`是第五个terminal记录的起始word。

## 2. 部分重置与查询时序

进入函数后只清前4个记录的以下word：

- `+0x00`。
- `+0x0A`。
- `+0x0C`。
- `+0x0E`。

其他字段不清零，必须保留旧值。第五个记录只把`+0x00`写为`0xFFFF`。

每轮先把当前记录`+0x00`写为`0xFFFF`，再依次查询剧情标志`0x1E, 0x1F, 0x20, 0x21`。因此第一次查询发生时，记录0已为`0xFFFF`，记录1..3的`+0x00`仍为入口清零值；这个可观察顺序由测试锁定。

## 3. 可用序号与选择状态

剧情标志返回任意非零值时，当前记录写入：

- `+0x00 = 原始项目索引0..3`。
- `+0x12`、`+0x16`、`+0x1A = 可用序号 + 8`。
- `+0x0C`、`+0x0E = 1`。

选择参数与“可用序号”比较，而不是与原始项目索引比较；相等时两个状态word改为`2`。可用序号仅在flag非零时递增。真实调用参数为`5`，最大可用数为4，因此真实初始化不会选中前4项。

## 4. 非直觉terminal覆盖

四轮结束后，机器以`available_count * 0x1C`定位记录，并执行：

```text
records[available_count].field_0c =
    records[available_count].field_10
```

随后EAX等于该16位`field_10`值。该定位按“可用数量”而非最后一个可用原始索引：稀疏flag可能覆盖一个已经写入状态的记录。例如flag模式`1,0,1,1`产生`available_count=3`，最终会覆盖原始记录3的`+0x0C`，但不覆盖其`+0x0E`。现代实现原样保留此行为，不把它修正为下一个空记录。

## 5. 平台接线与验证

`LegacyStandardModeItemState`保存4个项目记录加1个terminal记录；`LegacyStandardModeItemPorts`只查询现有剧情VM flag owner。`0x0043A2A0`的item callee现在直接进入该typed owner。

`special_modes.legacy_initial_menu`覆盖：

- 固定4次查询及`0x1E..0x21`顺序。
- 查询前的部分重置时点。
- flag任意非零即视为可用。
- 稀疏flag、可用序号、选择参数和共享索引`8..11`。
- 未触及字段保留旧值。
- `records[available_count]`的非直觉terminal覆盖。
- 真实参数`5`下4项全可用但均不选中。
- terminal索引和EAX返回值。

Linux core `186/186`与Linux app `192/192`通过。按阶段门禁，本单入口不重复执行Windows BUILD。workpack连续两轮生成均为`4/227`，SHA256均为`0a4e0ab3cada3c7463ba367cf94842e97c4c0a6207c73585165ab8a6a28e93ba`，只新增关闭`0x0043A380`。
