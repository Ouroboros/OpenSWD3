# `0x00438FA0..0x00439119` 第二套随机序列

状态：B3.1 闭环；`assembly_exact`、`platform_adapted`、`blocked_runtime_oracle`

来源：去除 FLIRT 名称后的完整汇编与 IDA 反汇编视图 1:1 LST。完整汇编是唯一行为真值；伪码和名称只用于定位。

## 1. 状态与初始化

该随机序列拥有 250 个 32 位状态字 `0x004A6610..0x004A69F7`、当前索引 `0x004FB7F0` 和只在初始化使用的 32 位 LCG 状态 `0x004A69F8`。`WinMain` 第二次独立调用 `time(NULL)` 后把截断的 32 位结果传给 `0x00438FA0`；它不能与前一个 CRT-compatible RNG 种子采样合并。

`0x00439110` 先保存 LCG 种子。`0x004390F0` 每次执行：

```text
state = u32(state * 0x015A4E35 + 1)
result = (state >> 16) & 0x7FFF
```

`0x00438FA0` 的顺序为：

1. 索引清零，连续 250 次 LCG 输出填充状态字。
2. 再连续 250 次；输出严格大于 `0x4000` 时，对对应状态字执行 `OR 0x8000`。汇编的 `or ah,80h` 设置的是 bit 15，不是 bit 31。
3. 对状态索引 `3,14,25,...,168` 依次执行递减 mask 与强制 bit：首项 `word=(word&0xFFFF)|0x8000`，随后 mask/bit 每项右移一位，共 16 项。

整个初始化固定消费 500 次 LCG。以种子 `0x12345678` 初始化后，LCG 状态为 `0x12E95E44`，250 个小端 dword 的 FNV-1a64 为 `0x08D74AADBF54491DE`。

## 2. 原始 xor 生成器

`0x00439020` 对当前索引 `i` 选择另一状态字：`i < 147` 时为 `i + 103`，否则为 `i - 147`，等价于模 250 的固定偏移。随后：

```text
state[i] ^= state[other]
result = state[i]
i = (i == 249) ? 0 : i + 1
```

状态初始只占低 16 位，xor 后仍处于 `0..0xFFFF`。种子 `0x12345678` 的前十个原始输出为：

```text
A606 E086 549E 9B28 01BD B7AB 703C 377A 3C79 FC46
```

## 3. `0x00439070` 有界随机

参数是无符号 32 位上界，返回 `[0, upper_bound)`。函数先计算：

```text
acceptance_limit = floor(0xFFFF / upper_bound) * upper_bound
```

每次尝试固定推进生成器两次：第一次结果直接丢弃；第二次才作为 candidate。candidate 大于等于 acceptance limit 时整对拒绝并重试，小于时返回 `candidate % upper_bound`。因此不得替换为标准库分布，也不得省略每次尝试的第一个随机值；调用次数会影响此后全部游戏逻辑。

`upper_bound == 0` 在第一条 `div` 触发整数除法错误。现代实现以确定性终止保留“不返回”级别，不把它修复成零或错误码。`upper_bound > 0xFFFF` 使 acceptance limit 为零，循环会永久消费成对随机值而不返回；实现保留该循环。当前已确认调用者没有把这些异常上界作为正常合同。

种子 `0x12345678` 下，连续十次 `next_bounded(100)` 为：

```text
78 20 19 2 82 45 73 24 95 89
```

索引从 0 变为 20。`next_bounded(32768)` 会拒绝三对候选，第四对返回 14202，索引变为 8。

## 4. C++20 映射与验证

`LegacySecondaryRng` 直接持有 250 个状态字、索引和初始化 LCG 状态：

- `seed` 映射 `0x00438FA0`、`0x004390F0` 与 `0x00439110`。
- `next_raw` 映射 `0x00439020`。
- `next_bounded` 映射 `0x00439070`，包括每次尝试消费两项和拒绝边界。

UT 固定初始化状态哈希、四个种子的首项、前十个 raw 输出、249 到 0 的索引回绕、bound 1、bound 100 和产生三次拒绝的 bound 32768。Windows LLVM `core` 与 `app` 均完成构建并通过 35/35 CTest。

原程序动态状态捕获后端仍为 `blocked_runtime_oracle`。后续集成测试必须把 seed、每次上界、返回值和累计调用序号写入回放记录；只比较最终结果不足以发现随机调用顺序漂移。
