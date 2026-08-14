# ANI 四槽漂移位置重置闭环

状态：`platform_adapted`、`assembly_exact`、`unit_tested`；原程序动态差分仍为
`blocked_runtime_oracle`。

唯一行为依据是 `swd3.exe.lst` 的 `0x0040F670..0x0040F689`。该函数只把四个漂移
状态槽的 `x` 字段写成待重生哨兵，不清理同槽其他字段，也不重建对应动作记录。

## 1. ABI 与调用点

物理 ABI 无参数。三个直接调用点为 `sub_40C130:0x0040C5BC`、
`sub_411D00:0x00411D1F` 和 `sub_4251B0:0x00425224`。三处调用后分别直接调用
`sub_40F690`、覆盖 ECX/EAX 以及重新装载 EAX，因此都不读取函数返回时保留的
`0x7FFFFFFF`；现代接口使用 `void`，没有丢失调用者可见合同。

## 2. 精确写入范围

函数先把 EAX 写为 `0x7FFFFFFF`，随后按地址顺序执行四个完整 dword 写入：

| 顺序 | 地址 | 现代字段 |
|---:|---:|---|
| 0 | `0x004B86F8` | `slots[0].x` |
| 1 | `0x004B8708` | `slots[1].x` |
| 2 | `0x004B8718` | `slots[2].x` |
| 3 | `0x004B8728` | `slots[3].x` |

四个地址间距均为 `0x10`。既有 `LegacyAniDriftSlot` 固定为四个 `i32` 字段和
`sizeof == 0x10`，字段零偏移是 `x`；`LegacyAniDriftState` 固定持有四槽。因此
`LegacyAniDriftEffect::reset_positions()` 按数组顺序只写四个 `x`，与 LST 的地址、
宽度、顺序和值一致。`y`、`velocity_x`、`velocity_y` 以及四个动作记录全部保持原值。

## 3. 所有权和生命周期接线

- `LegacyWorldFrameEffectState::drift` 是四槽状态与动作记录的实际现代 owner；构造时
  先初始化动作记录，再执行相同的四槽哨兵写入，对应进程初始状态。
- 世界 owner 重建已对实际 `drift` 调用 `reset_positions()`；
  `sub_40C130` 的完整外围次序仍由其尚未关闭的 B7 行复核。
- 总关闭 `release_0040f670` 现已绑定同一个实际 `drift` owner，不再只保留枚举占位。
- `sub_411D00` 的完整过渡次序继续归入该外层函数的独立 B7 行；本 helper 不据此
  提前宣布外层函数完成。

## 4. 双向收敛与验证

- LST→C++ 逐项核对常量、四个物理地址、四次 dword 写入、槽间距、字段偏移和普通
  `retn`，没有未映射指令效果；
- C++→LST 反查循环边界和所有可写字段，确认没有附带清零、动作重建、释放、条件门、
  参数读取或返回值消费；
- 独立 UT 在调用前污染全部四槽的四个字段，调用后验证四个 `x` 都精确等于
  `0x7FFFFFFF`，其余十二个字段和动作记录保持不变；
- Linux `core` 184/184、Linux `app` 189/189、Windows LLVM `app` 189/189 CTest
  全部通过；两端应用均成功链接，未启动任何 EXE。

核心实现为 `legacy_ani_drift_effect.cpp::LegacyAniDriftEffect::reset_positions`；SDL
总关闭接线位于 `main.cpp::SmokeShutdownPorts` 和 `SdlSmokeIdlePorts`。
