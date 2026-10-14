# 战斗角色可用性阻断值写入（0x00478330）

状态：`pending`。

## LST 行为

权威源为 `swd3.exe_export_for_ai/swd3.exe.lst`。
完整函数为 `00478330..0047833A`，3条指令，无分支和子调用：

```text
00478330  mov eax,[esp+4]
00478334  mov [ecx+2AE4h],eax
0047833A  retn 4
```

将完整32位参数写入角色的 `+2AE4` 字段。
原版 EAX 装入参数，ECX/EDX不变；这些寄存器不是新的业务结果。
对象写入失败时，字段保留原值，调用方不执行后续步骤。

## 当前共享接口

`set_legacy_battle_actor_availability_block(State*, u32)`
直接修改 `LegacyBattleActorAvailabilityBlockState::value`，只返回操作状态。
已删除寄存器式 Request、EAX/ECX/EDX回复、字段写次数和父调用次数。
调用方使用可选结果区分未执行与实际完成/失败。
保留原有不可写和缺少字段的失败处理，不增加参数规范化或替代写入。

唯一存储仍为
`LegacyBattleFinalActorStepState::group_a_availability_blocks`。
不同入口借用同一角色元素，不创建镜像字段或第二份对象数组。
原版 Group-A 对象步长为 `0x2F34`，地址计算中的 `0xBCD` 是其四分之一。

## 24处原版调用

- Group-A frame `00456680`：`004567EA`写1；
  `00456BC0`、`00456C1C`、`00456CF4`、`00456D88`、`00456E69`写0。
- 最终角色步骤 `0045AA00`：`0045AC54`写1。
- 动作后处理 `0045ADF0`：`0045AEEF`写0。
- 帧前处理 `0045D490`：`0045D51C`、`0045D5BB`写1，`0045D647`写0。
- 输入 `0045F2A0`：`0045F765`写1。
- 目标选择刷新 `00462740`：`0046292B`、`004629F3`、`00462B02`、
  `00462ED5`、`00463108`、`00463335`、`00463A50`、`00463CF0`均写1。
- 角色目标准备 `00464CC0`：`00464D04`写1。
- 脚本 `00469D20`：`0046AA6C`、`0046AC80`、`0046D221`均写1。

合计 `6+1+1+3+1+8+1+3=24`，分布于8类实现入口。
选择帧通过目标准备的可选结果间接观察此次写入。

## 最新提交的修正关联

`7feb7c40` 在 Group-A frame 中保留了 `0047CC50` 的通用调用。
其中 `00456BB2` 之后紧接 `00456BC0`，旧 C++ 将前者的虚拟 EDX
传入当前写入接口，形成不必要的机器回复依赖。

当前 `0047CC50` 的三处调用直接清共享 `field_2ad4`；
当前接口只接收真实状态和值，因此这条 EDX 传递也被删除。
Group-A 的末次 terminal EDX、选择完成 EDX收集不再为此接口保留。
动作后处理仍按原顺序保留门衰减、动作设置、阻断值写入和角色重置。
写入失败不会执行角色重置，也不回滚前面的真实状态变化。

八类父函数仍有其他尚未移除的通用协议。
本文件不将它们或完整战斗调用链宣称为迁移完成。
相邻查询 `0047C670` 和基础重置 `0047D350` 的状态按各自证据记录。

## 测试与验证

独立写入测试覆盖0、1、7FFFFFFF、80000000、89ABCDEF、FFFFFFFF，
重复写入、不可写字段和空字段。
八类调用方及间接选择帧测试检查共享字段、实际执行结果和失败后的状态保留。
旧调用计数和叶寄存器断言已删除；原有业务结果及失败后缀断言继续保留。

独立修正批次通过3项core及相同3项ASan测试：
`battle.legacy_battle_setup`、`battle.actor_frame_316`、
`battle.legacy_battle_actor_availability_block`，各1/1通过。
测试入口实际执行上述8类调用方及间接选择帧用例。
SDL目标`openswd3`实际链接通过；26个源码/测试文件的门前后SHA256一致。
日志为`build/tmp/runtime/action-mode-published-repair-`前缀的
`core`、`callers-core`、`availability-core`、`asan`、`callers-asan`、
`availability-asan`和`sdl.log`；验证文件为同前缀的`validated-source.sha256`。
core与ASan各报告既有`legacy_battle_outcome_resolution_test.cpp:133`
的u16到u8转换warning，该文件未修改；没有测试失败或sanitizer错误。
先前高位清除改动已独立保存，本次验证和发布不依赖该改动。
旧工作包278的全量测试数字不作为本次验证结果。

原版完整对象、异常内存页和联合 SEH 捕获仍缺失，
原版动态差分状态为 `pending`。
宿主测试不等同于原版异常动态验证。
