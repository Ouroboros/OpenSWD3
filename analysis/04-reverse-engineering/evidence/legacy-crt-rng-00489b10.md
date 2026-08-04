# `0x00489B10/0x00489B20` CRT-compatible 随机序列

状态：B3.2 闭环；`assembly_exact`、`platform_adapted`、`blocked_runtime_oracle`

这两个函数是静态 CRT 代码，不属于原游戏自有函数计数，但其状态和输出被游戏直接观察，因此 B3 必须交付精确兼容合同。完整汇编是唯一行为真值。

## 1. 状态与算法

全局 32 位状态位于 `0x004A833C`，镜像初值为 1。`0x00489B10(seed)` 只把参数原样写入该状态。

`0x00489B20` 每次执行：

```text
state = u32(state * 0x000343FD + 0x00269EC3)
result = (state >> 16) & 0x7FFF
```

汇编使用 `sar` 后再与 `0x7FFF`；对最终低 15 位而言与上述无符号表达等价。乘加必须按 32 位回绕，返回范围为 `0..32767`。

种子 1 的前十项为：

```text
41 18467 6334 26500 19169 15724 11478 29358 26962 24464
```

第十次后的状态为 `0xDF90722B`。种子 `0x12345678` 的前十项为：

```text
13289 23359 19469 24737 23446 14229 6193 18180 32073 13357
```

第十次后的状态为 `0x342D28BA`。

## 2. 调用与播种顺序

完整汇编有 14 个 rand 调用点，分布在世界和资产运行时的四个函数中。具体消费者必须保留调用条件和次数；B3 不把它替换成标准库 RNG。

正常启动在同步自定义消息 `0x404` 返回后执行：

```text
time(NULL) -> CRT-compatible seed
time(NULL) -> 250-word secondary seed
```

这是两次独立采样。跨秒时两个种子可以不同。现有 `app::seed_two_rng_streams` 已锁定“读时间、播 CRT、再读时间、播 secondary”的调用序列；SDL3 适配器现在以两次 `std::time(nullptr)` 的低 32 位驱动真实 `LegacyCrtRng` 与 `LegacySecondaryRng`，不再是空端口。

`0x004347BA..0x004347C0` 的一条资产路径会再次调用 `time(NULL)`，但只重播 CRT-compatible 序列，不影响第二套 RNG。该调用随 `asset_runtime` 实现时接线。

## 3. 实现与验证

`LegacyCrtRng::seed` 和 `LegacyCrtRng::next` 分别映射两个函数；对象保留 32 位状态，默认值为镜像中的 1。UT 固定两组十项输出和最终状态；既有 app 测试固定两次独立时间采样及播种顺序。

Windows LLVM `core` 与 `app` 均完成构建并通过 36/36 CTest，SDL3 EXE 已链接真实两套 RNG。64 位 `time_t` 到 32 位种子的截断属于显式 `platform_adapted` 边界。原程序运行时随机调用轨迹仍为 `blocked_runtime_oracle`；后续回放必须记录 seed、流 ID、调用序号、上界和返回值。
