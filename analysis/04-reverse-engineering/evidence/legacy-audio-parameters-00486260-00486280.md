# 旧音频参数转换：`0x00486260/0x00486280`

状态：`assembly_exact`；Linux/Windows UT 通过

来源：`swd3.exe.lst` 完整反汇编。IDA 伪码只作导航。

## `0x00486260` 音量参数

函数读取完整 32 位参数，用有符号 `jle/jge` 实现：

```text
value > 127 → 127
value < 0   → 0
otherwise   → value
```

它由 sequence、sample 和 stream 路径共同调用，返回值直接交给 Miles 音量 API 或
乘以 16 保存为内部渐变状态。

## `0x00486280` 声像参数

函数先用 x86 `add eax, 0x3F` 做 32 位回绕加法，再对结果执行与上面相同的有符号
`[0,127]` clamp。公式是：

```text
shifted_bits = u32(value) + 63
shifted      = signed_i32_of_same_bits(shifted_bits)
result       = clamp_signed(shifted, 0, 127)
```

因此 `INT32_MAX` 不是 127，而是在加法回绕成负数后返回零。C++ 实现先在 `u32` 中
加法，再以 `std::bit_cast<i32>` 解释位型，避免把原 x86 回绕写成未定义的 signed
overflow。

独立 UT 覆盖两个端点、端点相邻值、负值及 `INT32_MIN/INT32_MAX`。验证结果为 Linux
`core` 65/65、Windows LLVM `app` 67/67 CTest。

