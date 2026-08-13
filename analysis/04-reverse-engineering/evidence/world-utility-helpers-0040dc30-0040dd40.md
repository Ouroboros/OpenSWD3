# 世界事件、移动步长与对象槽 helper 闭环

状态：`assembly_exact`（有效输入）、`unit_tested`；事件容器边界为
`platform_adapted`，原程序动态差分仍为 `blocked_runtime_oracle`。

唯一行为依据是 `swd3.exe.lst`。本轮先从 `0x0040DC30..0x0040DD50` 独立恢复三个
函数，再逐一反查所有汇编调用点和现代 C++ 消费点；实现后重新按基本块和副作用反向
核对，未继承既有完成结论。

## `sub_40DC30`

`0x0040DC30..0x0040DC4A` 从 `[0x004ACAC0]` 读取事件链头。空链直接返回零；非空链
按节点 `+0x00` 的 next 指针从头到尾遍历，比较节点 `+0x04` 与完整 32 位参数，返回
第一个相等节点；走到空 next 后返回零。它不比较其他字段，也不返回最后一个重复项。

现代 `LegacyWorldMapEvent` 的 `field_04` 对应节点 `+0x04`，事件数组保持原链表顺序。
`find_legacy_world_map_event` 因此返回 span 中第一个 `field_04 == event_code` 的对象，空
span 和 miss 均返回空指针。受检 span 不复刻悬空指针和环形链故障，故容器边界登记为
`platform_adapted`；有效有序链的比较、首命中和返回行为精确一致。

三个汇编调用者分别位于 `sub_402F80`、`sub_413FE0`、`sub_427300`。现代碰撞 Talk、
调试叠层和鼠标地图事件三条路径均已统一调用同一 helper，没有残留独立查询实现。

## `sub_40DD10`

`0x0040DD10..0x0040DD19` 只执行两次数据搬运：读取完整 32 位参数到 EAX，再原样写入
`[0x004AB2D0]`，随后直接返回。因此返回值也是传入值；没有截断、夹取或特殊值处理。

`set_legacy_world_movement_step` 对 `LegacyWorldMovementRuntimeState::movement_step` 保留
完全相同的写入和返回合同。玩家运动在动作状态改变、双速和调试固定速度计算完成后通过
该 helper 写回。调用点复核还发现 `sub_40E0B0` 在 `0x0040E2E9..0x0040E2F0` 明确以
`0x10` 初始化该全局；现代 `LegacyWorldFrameCoordinatorState` 现在也在首次世界帧前
把 movement step 初始化为 16，而不是依赖首次方向输入才产生值。

## `sub_40DD40`

`0x0040DD40..0x0040DD50` 保存 EDI，取目标指针，令 ECX 为 `0x87`、EAX 为
`0xFFFFFFFF`，再执行 `rep stosd`。结果是从目标开始连续写入 135 个全一 dword，恰好
覆盖 `0x87 * 4 = 0x21C` 字节；函数返回时 EAX 仍为 `0xFFFFFFFF`。

`LegacyWorldObjectSlot` 已由静态断言固定为 `0x21C` 字节。
`reset_legacy_world_object_slot` 覆盖完整 slot 并返回 `0xFFFFFFFF`。角色转队、角色换图、
PATH 请求与推进、剧情 PATH 和剧情 VM 中所有原对象槽重置点均复用该 helper；没有把
它误写成只清路径前缀或只写首字段。类型引用隔离了原版无效裸指针写入，但有效对象槽的
全部字节和返回值一致。

## 双向追溯与测试

- LST → C++：覆盖事件空头、重复 id 首命中、miss，移动值完整 dword 写回及返回，
  以及对象槽 135 次 dword 写入和 EAX 返回；
- C++ → LST：三个 helper 的每个判断、写入和返回均能回指上述指令；调用点没有增加
  排序、范围归一化、部分清理或额外业务副作用；
- UT 固定事件重复项顺序、空/miss、`0xFFFFFFFF` 移动值、完整 `0x21C` 字节哨兵覆盖、
  返回值以及首次世界帧前的 16 步长初始化；
- 既有碰撞 Talk、交互、调试叠层、角色转队、换图、PATH 和剧情 VM 回归继续覆盖组合
  调用链。

有效输入上的汇编行为已零未决收敛。剩余差异只有现代 span/类型引用对损坏链和无效裸
指针的隔离，以及尚未执行的原程序动态差分。
