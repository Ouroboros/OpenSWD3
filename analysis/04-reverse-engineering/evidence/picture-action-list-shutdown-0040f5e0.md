# Picture-action 双链关闭生命周期闭环

状态：`platform_adapted`、`assembly_exact`（原版有效链域）、`unit_tested`；原程序动态
差分仍为 `blocked_runtime_oracle`。

唯一行为依据是 `swd3.exe.lst` 的 `0x0040F5E0..0x0040F62E`。该函数依次逐头销毁
`dword_4B7C70` 与 `dword_4B8968` 两条 picture-action 链。

## 1. ABI、调用点与物理节点

物理 ABI 无参数。四个直接调用点为 `sub_40A570:0x0040A664`、
`sub_40C130:0x0040C599`、`sub_411D00:0x00411D15` 和
`sub_4251B0:0x00425201`。调用后都直接执行后续关闭逻辑，没有一处读取 EAX，因此
IDA 伪码标出的 `int` 返回值不是调用合同。

opcode 58/153 共用 `0x0042B1F1..0x0042B282` 的创建路径：分配并清零 `0xA4` 字节，
在 `+0x08` 初始化 `0x98` 字节 action，把 `+0xA0` 写为原链首，再分别前插到
`dword_4B7C70` 或 `dword_4B8968`。既有 `LegacyPictureActionNode` 静态断言固定了
`sizeof == 0xA4`、`action == +0x08` 与 `next_pointer_32 == +0xA0`。

## 2. 精确控制流

第一段从 `dword_4B7C70` 取首。非空时每轮严格执行：

1. 读取当前节点 `+0xA0` 的 next；
2. 先把 next 写回 `dword_4B7C70`；
3. 调用 `sub_4885A0(current)` 释放完整 `0xA4` 节点；
4. 重读第一条全局链首，非空则继续。

第一条链为空后，函数无条件转入 `dword_4B8968`。第二段执行完全相同的逐头循环，只把
根替换为第二条全局首；第二条链为空后直接返回。两种空根都不会产生释放调用。节点没有
需要在本函数内单独释放的内部指针。

## 3. 调用接线与平台边界

- `LegacyPictureActionLists::primary/secondary` 分别对应两个原版全局根；opcode 58/153
  的前插和逐帧消费者继续使用相同 owner。
- helper 明确先调用 primary 的逐头释放，再调用 secondary；每轮先把 list 头
  `splice` 到临时独占链，再 `pop_front()`，从代码层保留“先推进根、后销毁节点”。
- 世界 owner 重建、主过渡和 SDL 总关闭的 `release_0040f5e0` 已复用同一 helper；
  不再以 `world_picture_actions_ = {}` 隐式整体替换 owner。
- 原 `+0xA0` 32 位 next 槽只保留在精确物理快照中，不存放截断的 64 位主机指针。
  损坏链、循环链和重复裸节点不复制到现代容器，属于平台内存隔离。

## 4. 双向收敛与测试

- LST→C++ 已覆盖第一根空/非空、`+0xA0` 读取、先推进第一根、逐头释放、无条件切换
  第二根、第二根空/非空及最终返回；
- C++→LST 已反查 helper、opcode 58/153 owner、世界 owner 重建、主过渡和总关闭接线，
  没有颠倒双链次序、释放 action 内部不存在的分配或清理相邻 owner；
- 独立 UT 覆盖 primary 两节点、secondary 一节点、两个最终空根和重复空链零次释放；
- 四个调用点均已反查，没有参数、EAX 消费、条件方向、字段宽度、循环边界或调用顺序
  差异。
- Linux `core` 184/184、Linux `app` 189/189、Windows LLVM `app` 189/189 CTest
  全部通过；两端应用均成功链接，未启动任何 EXE。

核心实现为 `legacy_picture_actions.cpp::release_legacy_picture_actions`；SDL 接线位于
`main.cpp::SmokeShutdownPorts` 和 `SdlSmokeIdlePorts`。
