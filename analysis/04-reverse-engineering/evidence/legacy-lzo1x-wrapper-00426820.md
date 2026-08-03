# `0x00426820` 公共解压包装器复核

状态：B2.2 闭环；`assembly_exact`、`asset_verified`、`platform_adapted`

来源：`swd3.exe` 完整汇编

## 1. 物理合同

包装器的调用方参数顺序为：

```text
(source, destination, source_size, actual_output_size*)
```

`0x00426820..0x00426835` 只读取四个栈参数，将它们重排为活动解压器 `0x004399E0` 的顺序：

```text
(source, source_size, destination, actual_output_size*)
```

随后它额外压入一个被解压器忽略的字面零，在 `0x00426836` 调用解压器，`0x0042683B` 按五个双字恢复栈，最后直接 `retn`。包装器没有比较、转换或覆盖 EAX，因此返回值原样传播。

完整汇编是上述合同的唯一真值；IDA 伪码 `return sub_4399E0(a1, a3, a2, a4)` 只与汇编结果一致，不作为独立依据。

## 2. 调用层策略

包装器共有 11 个调用点，全部不按返回值分支：

- `persistence`：`0x00408371`、`0x00408439`、`0x00408546`、`0x00408B0E`、`0x0040976A`、`0x0040980B`。六处都忽略返回值和实际输出长度。
- `asset_runtime`：`0x0040ADC6`、`0x00415B57`。两处都忽略返回值和实际输出长度；ANI 缓存重读真实产生 `input_not_consumed` 等价状态但继续使用相同输出。
- `world_map`：`0x00426182`、`0x0042660E`、`0x00426FDB`。三处都忽略返回值；前后两处也忽略实际长度，`0x0042660E` 只把实际长度指针继续传给 `0x00401B70`，自身不比较。

这些调用者属于后续各自模块。B2 只冻结共享包装接口和调用策略，不提前把存档、ANI 或地图业务逻辑搬进 `resource_io`。各所有者模块接入时必须分别保留“忽略两者”或“忽略状态、转交长度”，不得新增统一的非零返回拒绝。

两个 TSW 直接调用点和 Fame 直接调用点不经过本包装器；它们覆盖返回值并严格比较实际长度，继续由 `common-decompressor-004399e0.md` 约束。

## 3. C++ 映射

`decompress_legacy_resource_block(source, destination, actual_output_size)` 位于 `include/openswd3/resource_io/legacy_lzo1x.hpp` 和 `src/resource_io/legacy_lzo1x.cpp`：

- source 与 destination 的逻辑顺序对应包装器调用方顺序；两个 span 自带源长度与现代目标容量。
- 调用 `decompress_legacy_lzo1x` 后原样返回 `LegacyLzo1xStatus`。
- 入口先把实际输出长度清零；到达结束标记的 `success` 或 `input_not_consumed` 才写回最终长度，对应原函数先写长度再判断输入尾。
- `source_exhausted`、`destination_exhausted`、`invalid_lookbehind` 和 `size_overflow` 是现代安全边界。它们提前返回时保持入口零值，不伪造原汇编可能越界后才形成的正常尾部状态。

未来调用者可以忽略返回状态、忽略实际长度或转交实际长度；共享包装器本身不代替调用者作策略选择。

## 4. 验证

UT 固定了两条包装行为：

- 一个合法输出后带尾字节的流返回 `input_not_consumed`，同时写回一字节实际长度并保留已解出的 `A`，证明忽略状态的调用者仍能使用输出。
- 截断输入进入现代 `source_exhausted`，实际长度保持入口清零值。

全量 TSW 验证器已经改为通过公共包装接口解压六个包。20,091 帧、558,351,505 字节压缩输入和 1,378,998,573 字节输出全部返回 success 且实际长度等于描述符声明值。六个拼接 SHA-256 为：

- `all_char.tsw`：`c6c401cfdd33d047ad16afb0e3af047f5f24a203a76fe2b0679c70e07235fff3`
- `all_item.tsw`：`27a624cd08659b0723839b8b4ae8673192346fea8455aca0cdd631c2b76c2402`
- `all_magic.tsw`：`0c797e28826a8da9cd18dcf265126b6b9ff86726520f509a07237dc89b8b59b1`
- `all_map1.tsw`：`888680220725bee08718b6203c8f63cad8f9ed84c09e6e4dc913e80bca101934`
- `all_map2.tsw`：`79c8f249ce2fb2a17e677f05078f88b70736ada3b554eec05bc5c8a4e453f1f8`
- `all_sys.tsw`：`45a68c036ed196e91414502c24b1ea12f826832c0a25e98929f7bb8628a27c17`

这些哈希逐项等于既有汇编安全转写登记值。Windows LLVM `core` 当前为 25/25 CTest 通过；原程序动态差分仍登记为 `blocked_runtime_oracle`。
