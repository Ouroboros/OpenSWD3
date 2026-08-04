# `0x004267E0` 存档包装与两套 LZO1X 压缩器

状态：B2.4 闭环；`assembly_exact`、`asset_verified`、`platform_adapted`、`blocked_runtime_oracle`

来源：`swd3.exe` 完整汇编、40 份现有存档及其 200 条压缩流。IDA 伪码只辅助辨认参数，不作为行为依据。

## 1. 单元边界与调用域

本单元覆盖一个存档分配包装和四个压缩函数：

- `0x004267E0..0x0042681E`：普通存档块包装；四个保存调用点使用。
- `0x00439120..0x0043920A`：14 位字典包装器；唯一直接调用者为 `0x004267E0`。
- `0x00439210..0x00439572`：14 位字典 helper；只由前一包装器调用。
- `0x00439580..0x0043966A`：15 位字典包装器；唯一直接调用者为 `0x00477C90`。
- `0x00439670..0x004399D2`：15 位字典 helper；只由前一包装器调用。

最后一条指令是 `0x004399D2 retn`。因此先前队列中的近似范围 `0x00439120..0x004399DF` 修正为实际完整函数范围 `0x00439120..0x004399D2`；下一活动公共解压器从 `0x004399E0` 开始。

`0x004267E0` 为普通存档块分配 `source_size + 0x20` 字节输出和 `0x10000` 字节工作区，再调用 14 位包装器。保存路径有四个调用点进入该包装层。

其三个参数依次是源指针、32 位源长度和实际输出长度指针；返回值是新分配的压缩缓冲。工作区在调用结束前释放，输出缓冲由调用者继续写入存档并释放。C++ 的 `compress_legacy_save_block` 以 owned block 表达相同所有权，把完整的 `source_size + 0x20` 存储和实际写入长度分开交付；超过 32 位加法范围时进入明确的现代安全状态。

`0x00477C90` 为 Fame 块分配 `source_size + source_size / 64 + 0x13` 字节输出和 `0x20000` 字节工作区，再调用 15 位包装器；结果不小于源长度时进入原有失败分支。

## 2. 包装器合同

两套包装器的物理接口均为：

```text
int compress(
    const uint8_t* source,
    uint32_t source_size,
    uint8_t* destination,
    uint32_t* actual_output_size,
    void* work_memory);
```

`source_size` 以无符号数和 13 比较。不大于 13 时不调用 helper，完整输入直接作为尾 literal；更长时 helper 写回已经产生的输出长度并返回未编码的尾 literal 数量。

包装器随后按当前输出位置编码尾 literal，复制剩余源字节，追加 `11 00 00`，写回最终输出长度并恒返回零。尾 literal 的三种分支为：

- 尚无输出且长度不大于 `0xEE`：首字节为 `length + 0x11`。
- 长度 1 至 3：写入前一个 match 偏移字节的低两位。
- 长度 4 至 18：写入 `length - 3`；更长时写零 token 和每 255 字节一个零的扩展长度。

## 3. 字典与匹配搜索

共同哈希按汇编的 32 位回绕顺序计算：

```text
value = source[p + 3] << 6
value = (value ^ source[p + 2]) << 5
value = (value ^ source[p + 1]) << 5
value = (value ^ source[p]) * 0x21
index = value >> 5
```

对两套 helper 的指令序列去除地址和局部标签后，只有两条算法指令不同：

- 14 位版本：主索引 `index & 0x3FFF`，备用索引 `(index & 0x07FF) ^ 0x201F`。
- 15 位版本：主索引 `index & 0x7FFF`，备用索引 `(index & 0x07FF) ^ 0x401F`。

其他搜索规则完全相同：

- 源长度大于 13 时先无条件检查偏移 4；此后仅在下一位置小于 `source_end - 13` 时继续搜索。
- 候选必须位于当前源位置之前，距离为 `1..0xBFFF`。
- 主候选距离大于 `0x800` 且第四字节不同时，才改查备用槽。
- 前三字节相同才接受匹配；接受或拒绝后都只更新实际检查的那个槽。
- 匹配先固定前三字节，再从第四字节扩展，最长一直到源末端。

原 helper 把工作区槽解释为 x86 源指针，调用者用未清零的 `malloc` 提供工作区。C++ 使用 32 位源偏移和无效哨兵，避免读取未初始化指针和在宿主指针宽度变化后复制未定义行为。当前 200 条真实存档流仍全部产生与原流逐字节相同的结果，因此该隔离没有改变当前资产上的可观察编码。

## 4. literal 与 match 编码

helper 在每次匹配前先输出自上次匹配结束以来的 literal。长度 1 至 3 写入前一个 match 的低两位；4 至 18 写 `length - 3`；更长使用零 token 和 255 字节扩展。

匹配编码按距离和长度分三档：

- M2：距离不大于 `0x800` 且长度 3 至 8；两字节 token 使用 `distance - 1`。
- M3：距离不大于 `0x4000`；长度不大于 `0x21` 时写 `0x20 | (length - 2)`，否则使用 `length - 0x21` 的扩展长度。
- M4：距离大于 `0x4000`；先减 `0x4000`，长度不大于 9 时直接编码，否则使用 `length - 9` 的扩展长度。

长 literal 和长 match 的扩展值每超过 255 写一个零，最后写 `1..255` 的余数。匹配完成后不回填中间字典位置，直接从匹配末端继续搜索；到达 `source_end - 13` 后把剩余字节交还包装器。

## 5. 逐基本块实现映射

- `0x004267E0..0x0042681E`：`compress_legacy_save_block` 的容量计算、14 位入口调用、实际长度和输出所有权。
- `0x00439120..0x0043920A`、`0x00439580..0x0043966A`：`LegacyLzo1xCompressor::run`、`write_trailing_literals` 和结束标记输出。
- `0x00439237..0x00439313`、`0x00439697..0x00439773`：`dictionary_index`、主/备用槽选择、距离与前三字节检查。
- `0x00439315..0x004393B1`、`0x00439775..0x00439811`：`write_literal_run` 及扩展长度。
- `0x004393B3..0x00439538`、`0x00439813..0x00439998`：匹配扩展和 M2/M3/M4 编码。
- `0x00439539..0x00439572`、`0x00439999..0x004399D2`：搜索终止、helper 输出长度和尾 literal 数量。

实现位于 `src/resource_io/legacy_lzo1x_compressor.cpp`。模板参数只选择 14/15 位字典宽度；共享状态机没有复制成两份。公共接口位于 `include/openswd3/resource_io/legacy_lzo1x.hpp`。

原函数没有目标容量参数。现代接口使用 `std::span`，在容量不足时返回 `destination_exhausted`，在源或目标长度超过 32 位时返回 `size_overflow`。这些是 `platform_adapted` 安全边界；容量充分的合法输入仍按上述汇编分支编码，安全失败不冒充原包装器恒零返回值。

## 6. 验证结果

`tests/unit/resource_io/legacy_lzo1x_compression_test.cpp` 对两套入口共同覆盖：

- 普通存档包装保留精确的 `source_size + 0x20` 存储容量、实际输出长度和固定压缩前缀。
- 空输入、1/13/14 字节和 300 字节长 literal 的精确输出。
- M2 固定向量、匹配后 0 至 3 字节尾 literal，以及 600 字节重复输入跨两个 255 字节扩展段的精确输出。
- 距离 `0x800`、`0x801`、`0x4000`、`0x4001`、`0xBFFF` 的 token 边界和逐字节回解。
- 固定碰撞输入下 14 位输出 518 字节、FNV-1a64 为 `0x28C584A5307464A9`；15 位输出 517 字节、哈希为 `0x4C542BBD551818C9`。
- 目标容量恰好和少一字节的结果；32 位长度上限由入口先验检查逐代码路径复核。

`analysis/tools/verify_legacy_lzo1x_save_roundtrip.cpp` 按原容器布局读取 `Save/` 与 `Save1/` 的 40 份存档。每条流执行“原流解压、按对应 14/15 位入口重压、重压流再解压”，并要求重压大小不超过原调用者容量、最终数据等于源、重压字节等于原流。结果为：

```text
saves=40
normal_blocks=160  exact_normal=160
fame_blocks=40     exact_fame=40
original_compressed=941822  recompressed=941822
decompressed=1666172
original_hash14=0xea93b6d541aa50b3
recompressed_hash14=0xea93b6d541aa50b3
original_hash15=0xf03d1f408737ac46
recompressed_hash15=0xf03d1f408737ac46
```

Windows LLVM `core-debug` 与 `app-debug` 均完成构建并通过 34/34 CTest。原程序动态输入捕获后端仍不可用，因此没有把汇编逐块复核、历史原流逐字节差分和真实存档验证升级为运行时 `original_diff_verified`；该项继续登记为 `blocked_runtime_oracle`。

## 7. 1:1 重写约束

- 两套包装器和 helper 必须共同保留；不能用系统 LZO 版本或一个任意压缩级别替换。
- 哈希、备用槽条件、只更新当前槽、匹配搜索终点、距离档位、扩展长度和结束标记必须保持。
- 14 位入口用于四个普通存档块，15 位入口用于 Fame；不能按压缩率自动互换。
- 未初始化指针与无容量写入只允许在明确的安全边界隔离；不得借此改变合法流字节结果。
- 不得把更高压缩率、不同字节但可回解的流或库版本输出当成 1:1 完成。
