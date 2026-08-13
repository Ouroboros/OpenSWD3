# MAPS 四条队伍属性模板物化

状态：`assembly_exact`、`unit_tested`、`asset_verified`；原程序动态差分仍为
`blocked_runtime_oracle`。

唯一行为依据是 `swd3.exe.lst` 的 `0x0040DD60..0x0040DE41` 及调用点
`0x0040E76D..0x0040E7B2`。IDA 为 `sub_40DD60` 标出的两参数原型只用于导航；调用者
实际每次压入三个参数，第三个常量 `1/2/4/8` 不被 callee 读取。

## 调用合同

`sub_40E0B0` 从 MAPS 载荷头 `+0x18` 取得源目录，相对载荷基址定位后依次取四条
`0x34` 字节记录。目标为 `0x004AB790/0x004AB7C8/0x004AB800/0x004AB838`，步长
`0x38`。这四条运行时记录随后被世界、剧情和战斗系统共享，因此由 MAPS 数据库 owner
负责解码，而不是在某个消费模块内重复搬运。

## 精确字段搬运

`materialize_legacy_maps_party_attribute_record` 严格实现以下写入：

```text
dst +00..03 = src +00..03
dst +0A..0F = src +04..09
dst +04..09 = src +0A..0F
dst +10..1F = src +10..1F
dst +2A..2B = src +20..21
dst +20..23 = zero_extend_u16(src +22)
dst +24..29 = src +24..29
dst +2C..35 = src +2A..33
```

`dst+0x36/+0x37` 完全不写；函数返回最后读入 AL 的 `src[0x33]`，四个已知调用者均不
观察该返回值。实现没有在搬运前 memset 目标，也没有把源结构机械 memcpy 到目标。

## 双向追溯与真实数据

LST → C++ 逐条覆盖所有 dword/word/byte 写入；C++ → LST 逐个目标字节反查后，没有
额外业务写入。独立 UT 以递增源字节和 `0xA5` 目标哨兵固定完整 56 字节结果、零扩展和
尾部两字节保留。

当前 `MAPS.DAT` 去掉 `0x200` 前缀后的 `+0x18` 为 `0x185A`。四条物化记录的
FNV-1a 64 哈希为：

```text
0xCC4B8CF1942788FB
0xF4F20DE2292D8DA5
0x18C87379B4B15AF6
0xB3BDE5C0E26B9D24
```

真实资产解码测试固定该目录偏移和全部四条哈希；截断任一 `0x34` 字节源记录会在现代
MAPS 所有权边界返回显式错误。有效输入上的搬运行为已零未决收敛。
