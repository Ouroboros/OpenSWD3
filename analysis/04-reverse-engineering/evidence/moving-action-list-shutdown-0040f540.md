# 世界移动 action 链关闭生命周期闭环

状态：`platform_adapted`、`assembly_exact`（原版有效链域）、`unit_tested`；原程序动态
差分仍为 `blocked_runtime_oracle`。

唯一行为依据是 `swd3.exe.lst` 的 `0x0040F540..0x0040F567`。该函数逐头销毁
`dword_4AD3E8` 的普通单链；节点没有需要单独释放的内部指针。

## 1. ABI、调用点与物理节点

物理 ABI 无参数。四个直接调用点为 `sub_40A570:0x0040A66E`、
`sub_40C130:0x0040C5A3`、`sub_411D00:0x00411D06` 和
`sub_4251B0:0x0042520B`。四处调用后都直接调用 `sub_40F570`，不读取 EAX；IDA
伪码的 `int` 返回值不是调用合同。

opcode 79 在 `0x0042A0A6..0x0042A1CF` 分配、初始化并前插 `0xB4` 字节节点；
`sub_414B60` 是逐帧消费者。完整物理布局已由 `moving-actions-00414b60.md` 和
`LegacyMovingActionNode` 的 `static_assert` 固定：`0x98` 字节内嵌 action、四个
`i16` 坐标、四个 float 运动字段，以及 `+0xB0` 的旧 32 位 next。销毁函数只读取
`+0xB0`，随后释放整个节点。

## 2. 精确控制流

入口读取全局首；空首在 `0x0040F547` 直接返回。非空时每轮严格执行：

1. 从当前节点 `+0xB0` 读取 next；
2. 把 next 写回 `dword_4AD3E8`；
3. 调用 `sub_4885A0(current)` 释放当前 `0xB4` 字节节点；
4. 重新读取全局首，非空则继续。

现代 helper 每轮先把 list 头 `splice` 到临时独占链，确保业务链首在节点析构前推进，
再从临时链 `pop_front()`。直接调用容器 `clear()` 无法从代码层证明这项顺序，因此不
作为该汇编函数的实现。

## 3. 调用接线与平台边界

- `LegacyMovingActionList` 继续由 `std::list` 管理主机链接，节点的旧 `+0xB0` 字段
  保留在精确物理快照中但不存放截断的 64 位指针。
- 世界 owner 重建不再使用整体赋空，而复用显式 helper。
- SDL `release_0040f540` 关闭槽绑定到实际 `world_moving_actions_`；总关闭会清空同一
  跨帧链。
- `sub_40A570` 对应的主过渡端口把 `release_0040f540` 转发到同一 owner；相邻尚未
  闭环的释放槽不在本函数内实现。
- 裸链循环、悬空 next 和重复节点不复制到现代容器，属于明确的平台内存隔离。

## 4. 双向收敛与测试

- LST→C++ 已覆盖空首、`+0xB0` 读取、先推进根、后释放节点、逐头循环和最终空根；
- C++→LST 已反查 helper、owner 重建、主过渡与总关闭接线，没有多释放内嵌 action
  或误清 packed-row/role-head 等相邻链；
- 独立 UT 在既有 `0xB4/+0xB0` 布局断言之外覆盖三节点销毁、最终空根和空链零次释放；
- 四个调用点均已反查，无 EAX 消费或遗漏参数；最后一轮正向与反向逐块追溯没有剩余
  条件方向、字段宽度、顺序、循环边界或返回合同差异。
- Linux `core` 184/184、Linux `app` 189/189、Windows LLVM `app` 189/189 CTest
  全部通过；两端应用均成功链接，未启动任何 EXE。

核心实现为 `legacy_moving_actions.cpp::release_legacy_moving_actions`；SDL 接线位于
`main.cpp::SmokeShutdownPorts` 和 `SdlSmokeIdlePorts`。
