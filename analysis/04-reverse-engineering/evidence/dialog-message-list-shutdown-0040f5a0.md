# 对话消息链关闭生命周期闭环

状态：`platform_adapted`、`assembly_exact`（原版有效链域）、`unit_tested`；原程序动态
差分仍为 `blocked_runtime_oracle`。

唯一行为依据是 `swd3.exe.lst` 的 `0x0040F5A0..0x0040F5DF`。该函数逐头销毁
`dword_4ACF48` 对话消息链，并在链为空后只保留 `dword_4A9920` 的 bit 15。

## 1. ABI、调用点与物理节点

物理 ABI 无参数。四个直接调用点为 `sub_40A570:0x0040A67D`、
`sub_40C130:0x0040C5B2`、`sub_411D00:0x00411D10` 和
`sub_4251B0:0x0042521A`。四处调用后均继续执行相邻关闭步骤，不读取 EAX；函数末尾
虽然以掩码后的计数留在 EAX 中返回，但它不是调用合同中的消费值。

节点由既有 `LegacyDialogRecord32` 固定为 `0x4C` 字节：`+0x38` 是文本分配指针，
`+0x44` 是标题分配指针，`+0x48` 是 next。销毁函数只显式读取和释放 `+0x38`，随后
释放整个 `0x4C` 节点；汇编没有调用 `free(node+0x44)`。

## 2. 精确控制流

入口读取全局首。空首直接进入最终掩码；非空时每轮严格执行：

1. 从当前节点 `+0x48` 读取 next；
2. 把 next 写回 `dword_4ACF48`，先推进业务链首；
3. 读取当前节点 `+0x38`，调用 `sub_4885A0` 释放文本分配；
4. 调用 `sub_4885A0(current)` 释放整个 `0x4C` 节点；
5. 重新读取全局首，非空则继续。

链处理结束后读取完整 32 位 `dword_4A9920`，执行 `& 0x00008000` 并写回，因此低位、
高位和其他状态一律清零，仅可能保留 bit 15。空链也必须执行该掩码。

## 3. 调用接线与平台边界

- `LegacyDialogRuntimeState::messages` 继续以 `std::list` 管理主机链接；helper 每轮先把
  业务 list 头 `splice` 到临时独占链，再释放文本 vector 并销毁节点，从代码层保留
  “先推进根、后释放内部文本、再释放节点”的顺序。
- 原版没有显式释放 `+0x44`。现代 `LegacyDialogMessage` 销毁时，其 caption vector
  仍由 RAII 一并归还；这是用安全主机 owner 替代裸节点后的所有权适配，不声称汇编
  存在额外的 `free(+0x44)` 调用。
- `dword_4A9920` 对应 `close.flagged_dialog_counter`；helper 不修改 selection、advance、
  close mode 或 input hold 等相邻对话状态。
- 主过渡、世界 owner 重建和 SDL 总关闭的 `release_0040f5a0` 已复用同一 helper。
  `sub_40C130` 与 `sub_411D00` 的 owner 清理由世界 owner 重建边界承接。
- 损坏链、循环链和重复裸节点不复制到现代容器，属于明确的平台内存隔离。

## 4. 双向收敛与测试

- LST→C++ 已覆盖空首、`+0x48` 读取、先推进根、`+0x38` 先释放、节点后释放、逐头循环
  以及最终 32 位 bit-15 掩码；
- C++→LST 已反查 helper、主过渡、世界 owner 重建和总关闭接线，没有整体重置无关
  对话字段，也没有把 caption RAII 适配误记为原版显式释放；
- 独立 UT 覆盖三节点的文本/节点逐项释放计数、最终空根、bit 15 保留、其他位清零、
  无关字段保持，以及空链仍执行掩码；
- 四个调用点均已反查，没有 EAX 消费、遗漏参数、剩余条件方向、字段宽度、循环边界或
  返回合同差异。
- Linux `core` 184/184、Linux `app` 189/189、Windows LLVM `app` 189/189 CTest
  全部通过；两端应用均成功链接，未启动任何 EXE。

核心实现为 `legacy_dialog_runtime.cpp::release_legacy_dialog_messages`；SDL 接线位于
`main.cpp::SmokeShutdownPorts` 和 `SdlSmokeIdlePorts`。
