# 战斗候选扫描读取角色启动标记（0x00478B50）

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`。
本查询及唯一实际调用方的通用调用迁移已验证；整体父链继续待迁移。

## 1. 完整物理范围

唯一行为真值为LST `00478B50..00478B56`，完整7字节、2条指令，
无callee、分支、外部chunk或中段入口。`00478B57..00478B5F`是填充。

```text
00478B50  mov eax, [ecx+2AE0h]
00478B56  ret
```

字段是完整DWORD，不截断、不把非零归一化成1。MOV/RET不改变ECX、
EDX及flags；RET读取栈返回地址在字段读之后。

## 2. 实际共享字段及结果

`query_legacy_battle_actor_start_gate_latch`借用实际`const u32*`，
读取既有`start_gate_latch`，不复制角色、不创建另一份标记。
A字段位于`action.group_a_action_execution[index]`；B字段位于
`startup.group_b_lifecycle[index].action_execution`。

结果只描述读取状态及可选实际DWORD值。字段不可读时value为空；
原返回地址访问失败发生于读取之后，value保留已读DWORD。
两种原故障状态及其父层前缀保持，没有新增成功值或停止点。
访问描述仅保留两种原故障条件，不包含寄存器、栈token或调用编号。

删除寄存器request/reply、CALL/返回地址数组、lazy请求数组、
请求offset、读/调用次数、flags/ESP/EIP及转发执行器。
父结果使用可选实际查询结果区分未执行和已执行，包括失败执行。

## 3. 唯一实际调用方

完整LST仅有`00457842 -> 00457847`，位于敌方帧的队员候选扫描。
其前缀顺序为terminal不等于1、AI完整DWORD不等于1、blocked不等于1，
再读取当前队员启动标记。前驱callee修改字段时，读取发生于修改之后。
循环逐个读取实际队员；不提前缓存字段或引用另一个角色。

`0045781D`先设EBX=1；`00457847 cmp eax,ebx`仅在完整DWORD
恰好为1时跳到`00457876`。
0、2、7FFFFFFF、80000000、FFFFFFFF以及00010001均不被当成1。
非一值继续回合完成查询；该callee仍待迁移，接收实际字段值及
父层新计算的CMP flags。EDX直接沿用blocked返回，因为本查询没有
EDX副作用；叶函数不再回传寄存器。

两种读取/返回故障都保留terminal、AI、blocked的已执行前缀，
阻断回合完成、当前敌方idle、control清理、启动门递增、progress
累加及余下当前候选。字段故障保留blocked EAX；返回故障使用已读值。
这是尚未迁移父层的局部失败桥接，未重新加入叶寄存器协议。

## 4. LST与实现双向核对

- 唯一DWORD读对应实际字段及value；唯一RET对应正常状态或原返回失败。
- 两种故障的value存在性对应字段尚未读取及已经读取两条前缀。
- 父层三道前门、精确1比较、下一callee的值/EDX/flags及后缀阻断保持。
- C++不执行原本不存在的标记写入、数值截断或非零布尔化。
- 地址与历史CPU合同留在证据中，不参与生产调用选择和执行计数。

独立向量覆盖六类完整DWORD、实际A/B字段及重复读取、不可用字段、
原字段/返回故障，以及父层精确1跳过和高位非一继续。
前驱回调把两队员标记分别改为1和00010001，证明实时读取与完整比较。
父层既有回合故障向量证明blocked EDX和CMP结果原样进入下一callee。

## 5. 验证范围及未完成项

本批实际定向门均通过：

- core `battle.legacy_battle_setup`：1/1，11.91s。
- core `battle.actor_frame_316`：1/1，30.62s。
- AddressSanitizer `battle.legacy_battle_setup`：1/1，17.44s。
- AddressSanitizer `battle.actor_frame_316`：1/1，31.65s。
- SDL `openswd3`编译并链接。

日志为`build/tmp/runtime/start-latch-direct-{core,callers-core,asan,
callers-asan,sdl}.log`。两个setup日志只有既有结算测试133行WORD转BYTE
警告；caller及SDL日志无警告或错误。未修改该无关测试。
检查器`build/tmp/runtime/verify-start-latch-direct-publication.py`及
`start-latch-direct-publication-check.log`确认六份源码/测试校验和对应
实际门禁、两指令完整LST、唯一caller、字段读取/返回顺序、上层桥接、
旧协议全调用方扫描及空白检查。初次检查只因禁止全部既有警告而失败，
日志保存在`start-latch-direct-publication-check-existing-warning.log`；
精确核对已知位置后重跑通过，未修改生产代码或测试。
源/测试修改后旧日志不能作为本批验收。

Group-B父层、动作状态其他请求、回合/idle/启动门递增等协议继续待迁移。
角色数字记录更新`0047C1F0`尚需实际记录存储和生产路径，不能把本查询
替代其业务。B10游标、Workpack316/318及B11实际续玩均未升级。
原版动态差分仍为`blocked_runtime_oracle`；未启动游戏程序。

历史工作包314的CPU合同验证与199/199、205/205门禁已归档在Git历史，
不能替代当前语义接口、实际调用方和本批定向验证。
