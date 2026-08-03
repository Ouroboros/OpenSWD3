# `0x004399E0` C++ 实现逐基本块复核

状态：B2.1 行为单元闭环；`assembly_exact`、`asset_verified`、`platform_adapted`

来源：`swd3.exe` 完整汇编

SHA-256：`0bac897a7557735b22607d8c8f0a79a3e7ae7729deb56593fd91c21e10baee0c`

## 1. 实现边界

实现位于 `src/resource_io/legacy_lzo1x.cpp`。公开接口接受源和目标 span，返回状态及已经写入的字节数。span 容量检查、非法 lookbehind 和 32 位大小溢出是防止现代 C++ 未定义行为的 `platform_adapted` 边界，不声称复刻原函数在损坏输入上的越界读写。

合法流的 token、长度、距离、复制顺序、结束标记和输入尾判断只取自汇编。IDA 伪码没有用于裁定分支。

## 2. 地址映射

| 汇编范围 | 行为 | C++ 对应 | 复核 |
|---|---|---|---|
| `0x004399E0..0x00439A1E` | 初始化输出长度、保存源末端、两种初始 literal | `LegacyLzo1xDecoder::run` | 首字节 `>0x11`、`count<4` 和 `count>=4` 三条转移一致 |
| `0x00439A20..0x00439A89` | 普通/零扩展 literal，长度为 token 加三 | `decode_literal_state`、`read_extended_count`、`copy_literals` | 扩展常量 `0x0F` 与最终加三一致 |
| `0x00439A89..0x00439AC1` | literal 后三字节短 match | `copy_short_match_after_literal` | 距离 `0x801 + 4*next + (token>>2)`，长度三 |
| `0x00439AC1..0x00439ADA` | 低两位尾 literal `0..3` 和状态转移 | `decode_match_done_state` | 读取 `source[ip-2]&3`；零回 literal，非零复制后直入 match |
| `0x00439ADB..0x00439B11` | `0x40..0xFF` match | `decode_match` 第一分支 | 距离与 `(token>>5)+1` 长度一致 |
| `0x00439B13..0x00439AC1` | match 状态两字节短 match | `decode_match` 的 `token<0x10` 分支 | 长度严格为二，不沿用 literal 后的三字节路径 |
| `0x00439B48..0x00439B7D` | `0x20..0x3F` 普通/扩展 match | `decode_match` 第二分支 | 扩展常量 `0x1F`、距离 `(u16>>2)+1`、最终加二一致 |
| `0x00439B7F..0x00439BCB` | `0x10..0x1F` 两段距离、扩展长度与结束标记 | `decode_match` 最后一分支 | `0x4000/0x8000` 距离域、扩展常量七和零距离结束条件一致 |
| `0x00439BCB..0x00439C11` | 普通或四字节快速重叠复制 | `copy_match` | 汇编只在距离至少四时使用 DWORD 路径；逐字节前向复制产生相同重叠结果 |
| `0x00439B29..0x00439B47` | 写输出长度并按源位置返回 `0/-8/-4` | `run` 的结束结果 | 精确消费和尾输入分别对应 `success`、`input_not_consumed`；提前源耗尽留在现代安全状态 |

无直接调用交叉引用的 `0x00439C20` 快速例程没有进入实现映射，也没有用于解释活动资产输出。

## 3. 当前验证

- 四个补充分支向量逐字节通过，覆盖两种初始 literal 和两种短 match。
- 控制向量覆盖精确结束、结束后尾字节、截断输入、目标不足和非法 lookbehind。
- `all_char.tsw` 记录 24、资源 24、变体 4 的真实 23 字节压缩帧解出 202 字节期望结果；该向量覆盖普通 literal、扩展 `0x20` match、尾 literal 二和精确结束。
- `analysis/tools/verify_legacy_lzo1x_tsw.cpp` 使用当前 C++ 实现流式解压全部六个 TSW，共 20,091 帧、558,351,505 字节压缩输入和 1,378,998,573 字节输出。
- 六个包的聚合 SHA-256 依次为 `c6c401cf...fff3`、`27a624cd...402`、`0c797e28...9b1`、`88868022...934`、`79c8f249...1f8`、`45a68c03...c17`，逐项等于既有汇编转写登记值。
- Windows LLVM `core` 和 `app` 构建通过；加入数据目录与包装器回归后，两套 CTest 均为 25/25。
- 原程序动态差分仍为 `blocked_runtime_oracle`；既有 LZO 2.10 对照只作为独立合法流证据。

## 4. B2.1 结论

当前实现已覆盖汇编状态机全部 16 条分支，并在全部 TSW 有效流上得到一致输出。损坏流边界仍明确属于现代安全隔离，不能把 B2.1 标成 `original_diff_verified`。`0x00426820` 参数重排和 11 个包装器调用环境已经在 `legacy-lzo1x-wrapper-00426820.md` 闭环。
