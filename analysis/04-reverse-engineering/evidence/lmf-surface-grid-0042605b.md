# LMF 地表原样表与压缩网格 `0x0042605B..0x00426190`

状态：B2.15 已实现；`assembly_exact`、`asset_verified`、`platform_adapted`、`unreachable_current_assets`、`blocked_runtime_oracle`

完整汇编是唯一行为真值。IDA 伪码和符号名只用于定位。本单元从 B2.14 得到的地图块偏移、格式、运行时五字和名称末端继续，只恢复紧随名称的原样表、两种格式的负载定位、地表压缩块及其尾随记录数。

## 1. 原样表定位与长度

`0x0042605B..0x0042606B` 以 32 位加减计算地图块基址与名称终止 NUL 后的相对位置，再调用一基 begin seek。因此本段起点是 B2.14 恢复的 `map_offset + raw_table_offset`，不是固定对齐地址。

`0x00426070..0x0042609C` 按原运行时字段计算：

```text
raw_entry_count = width * height * layers
raw_table_bytes = raw_entry_count * 4 + 4
```

随后分配并读取该长度。每个原样项物理宽度为四字节；最后额外四字节不属于原样项。

## 2. 低 word 原地压紧

`0x004260A6..0x004260EB` 先从表尾取得额外 `u32`，再以 `(actual_read - 4) / 4` 为项数遍历原样表。每轮从当前四字节项读取低 `u16`，写到同一缓冲连续的两字节目标位置，因此结果等价于：

```text
compact[i] = u16(raw_dword[i] & 0xFFFF)
```

高 word 被丢弃。当前接口只公开这组压紧后的 `u16`，不在资源层猜测其地图业务含义。

## 3. `MSFp` 与 `MSF2` 的负载定位

表尾额外 `u32` 的用途取决于地图格式：

- `MSFp`：`0x004260F2..0x00426116` 把它作为相对当前文件位置的跳过长度，跳过一段旧负载，再读取下一 `u32` 作为地表压缩长度。
- `MSF2`：`0x00426118..0x00426122` 直接把它作为地表压缩长度。

当前 `huge.lmf` 的 309 张地图全部为 `MSF2`；`MSFp` 路径由合成档案测试锁定，登记为 `unreachable_current_assets`，不能因当前资产不可达而删除。

## 4. 压缩块与尾随记录数

`0x00426126..0x00426159` 分配并读取 `compressed_size + 4` 字节。最后四字节先从缓冲取出并保存，传给公共包装器 `0x00426820` 的源长度则减去四字节。因此物理结构是：

```text
byte compressed_payload[compressed_size]
u32  post_surface_record_count
```

尾随双字不属于 LZO 输入。目标容量按 `0x00426160..0x0042617F` 固定为：

```text
surface_grid_bytes = width * height * 4
```

原调用点不检查包装器返回值，也不比较实际输出长度；当前 309 条合法流都返回成功并恰好填满目标。

## 5. 实现与验证

`legacy_lmf_read_surface_grid` 保留名称后定位、三维原样表长度、低 word 压紧、`MSFp/MSF2` 分支、`compressed_size + 4` 读取及尾随双字分离。尺寸乘加溢出、短读、失败 seek 和损坏压缩流返回确定状态，隔离原程序可能发生的越界访问或未初始化数据使用，属于 `platform_adapted` 损坏文件边界；合法输入仍服从汇编顺序和 32 位字段合同。

合成 UT 分别覆盖 `MSFp` 和 `MSF2`，锁定原样项低 word、旧负载跳过、八字节地表输出及尾随记录数，并覆盖无效头、尺寸溢出和损坏压缩流。真实验证沿尾索引逐一读取 309 张地图：309/309 地表流成功，压紧项数均等于 `width * height * layers`，输出均等于 `width * height * 4`，总输出为 9,830,932 字节。Windows LLVM `core` 的 33/33 CTest 与显式真实档案测试均通过；原程序动态轨迹仍登记为 `blocked_runtime_oracle`。
