# `huge.lmf` 尾索引读取与地图查找 `0x00425CFA`

状态：B2.13 已实现；`assembly_exact`、`asset_verified`、`platform_adapted`、`blocked_runtime_oracle`

完整汇编是唯一行为真值。IDA 伪码和符号名只用于定位。本单元是 `0x00425BE0` 地图装载器中可独立验证的首段，只负责打开 `huge.lmf`、读取尾索引并返回地图块偏移，不提前实现地图业务字段。

## 1. 文件读取与记录数

`0x00425CFA` 取得 32 位文件长度，再从文件起点读取一个小端 `u32 index_offset`。随后按 32 位减法得到：

```text
tail_size = file_size - index_offset
```

原函数为完整 `tail_size` 分配内存，从 `index_offset` 定位并读取整个文件尾。它计算可查记录数的指令序列不是普通的 `tail_size / 16`，而是对 `tail_size - 1` 执行带负数偏置的算术右移，等价于 32 位有符号除法向零截断：

```text
search_record_count = signed_i32(tail_size - 1) / 16
```

当前 `huge.lmf` 的尾部恰有 310 条 16 字节物理记录，因此函数读取全部 310 条，但只查找前 309 条。最后一条全零记录被这一减一公式排除。若在当前文件尾额外增加一个字节，计算结果会变成 310，全零记录就会进入查找范围；实现和 UT 保留了这个边界。

## 2. 查找合同

每条物理记录仍为四个小端 `u32`：

```text
+0x00 map_offset
+0x04 physical_map_span
+0x08 map_id
+0x0C reserved
```

查找从首记录线性前进，以完整 32 位比较 `record.map_id == requested_map_id`，命中第一条后返回 `+0x00` 的地图块偏移。`+0x04` 和 `+0x0C` 不参与这段原控制流。扫描到 `search_record_count` 仍未命中则进入原 `MapCode fail` 路径；当前实现返回独立的 `map_not_found` 状态，避免把合法的零偏移与失败混淆。

## 3. 现代安全边界与验证

`legacy_lmf_lookup_map` 接受显式归档路径，复用 `LegacyFile` 的 `OPEN_EXISTING + read` 后端，并保留整尾读取、减一计数、首个匹配和完整 dword 比较。原函数不检查首双字短读、尾部读取结果或越界 `index_offset`；现代实现分别返回 `header_read_failed`、`tail_read_failed` 和 `index_offset_out_of_range`，防止未初始化内存和越界分配参与查找，这些损坏文件路径标记为 `platform_adapted`。

合成 UT 覆盖缺文件、短头、越界索引、完整 32 位地图号、首个重复项、正常哨兵排除和额外尾字节使哨兵可查。当前 472,447,346 字节 `huge.lmf` 还验证了地图 22、24、500 的偏移分别为 `0x00000004`、`0x026698A3`、`0x1C16E962`，地图号零不命中。Windows LLVM `core`/`app` 的 33/33 CTest 均已通过；原程序动态轨迹仍登记为 `blocked_runtime_oracle`。
