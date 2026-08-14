# Framebuffer 变形节点链清空闭环

状态：`platform_adapted`、`assembly_exact`、`unit_tested`；原程序动态差分仍为
`blocked_runtime_oracle`。

唯一行为依据是 `swd3.exe.lst` 的 `0x0040F690..0x0040F6C2`。该函数逐头摘除并销毁
全局 framebuffer 变形节点链，不销毁链表哨兵本身。

## 1. ABI、调用点与链头

物理 ABI 无参数，唯一直接调用点是 `sub_40C130:0x0040C5C1`。调用后立即写 ECX、
清零 EAX 并执行另一段状态清零，因此不消费本函数最终留下的空链头值；现代接口使用
`void`。

`0x004AC990` 是由 `sub_430C60(1,1,0,0,1,1)` 构造的静态 `0x2C` 字节哨兵；
`dword_4AC9B8` 正好是该哨兵的 `+0x28` next 字段，不是另一个独立全局。动态节点同为
`0x2C` 字节，`+0x20/+0x24` 是两个拥有型缓冲区，`+0x28` 是 next。

## 2. 精确控制流

空链时读取 `dword_4AC9B8 == 0` 后直接返回，不调用析构或释放。非空时逐节点执行：

1. 保存当前链头；
2. 读取当前节点 `+0x28` 的 next；
3. 先把 next 写回哨兵 `+0x28`；
4. 以当前节点为 this 调用 `sub_430CF0`，依次释放 `+0x20` 和 `+0x24` 缓冲区；
5. 调用 `sub_489D00` 释放完整节点；
6. 重读哨兵 next，非空则继续。

循环结束时哨兵 next 为零。`test esi,esi` 位于已确认非空的循环体内，且在它之前已经
读取 `[esi+0x28]`，所以有效链域不会走到其空分支；现代实现不为这个不可达分支制造
另一套生命周期语义。

## 3. 所有权适配

`LegacyDeformationList` 以一个持续存在的 `LegacyDeformationNode` 表达静态哨兵，
以 `unique_ptr` 表达 `+0x28` 拥有链。`clear()` 每轮先把当前 head 移入局部 owner，再把
当前节点的 next 移回哨兵，随后才让当前节点析构，保留原版“先推进链头、后销毁节点”
的顺序。

节点成员逆序析构时，已被移空的 next 不再递归销毁后继，随后依次归还工作场和源快照，
对应 `sub_430CF0` 的 `+0x20`、`+0x24` 释放顺序；最后由 `unique_ptr` 释放节点本体。
哨兵自己的两个 `1×1` 缓冲区不在 `clear()` 中释放，因此清空后仍可继续头插新节点。

实际 owner 是 `LegacyWorldFrameEffectState::deformation`，世界 owner 重建已调用同一个
`clear()`。`sub_40C130` 的完整外围次序仍由其尚未关闭的 B7 行复核，不在本 helper
中提前宣布完成。

## 4. 双向收敛与验证

- LST→C++ 已逐项覆盖空链早退、`+0x28` next、先推进哨兵 head、两缓冲区析构顺序、
  节点本体释放、重读 head 和逐头循环；
- C++→LST 已反查每个 owner 转移和析构字段，确认没有连带销毁静态哨兵、保留动态
  节点、倒置释放顺序或递归删除未摘链后继；
- 独立 UT 覆盖三节点清空、最终空链、重复空链清空以及清空后复用同一哨兵重新插入；
- Linux `core` 184/184、Linux `app` 189/189、Windows LLVM `app` 189/189 CTest
  全部通过；两端应用均成功链接，未启动任何 EXE。

核心实现为 `legacy_frame_deformation.cpp::LegacyDeformationList::clear`；SDL owner
接线位于 `main.cpp::SdlSmokeIdlePorts::initialize_new_game_state_and_world`。
