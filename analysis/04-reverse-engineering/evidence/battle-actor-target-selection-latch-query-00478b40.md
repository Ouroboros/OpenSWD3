# 战斗角色读取目标选择标记（0x00478B40）

分类保持`platform_adapted`。本查询及全部四个实际caller的语义迁移已验证。
双方帧及动作父链仍有其他通用协议，不能升级整条父链验收。

## 1. 完整物理范围与顺序

唯一行为真值为LST `00478B40..00478B46`。完整7字节、2条指令，
无callee、分支、外部chunk或中段入口。`00478B47..00478B4F`为填充。

```text
00478B40  mov eax, [ecx+2AA8h]
00478B46  ret
```

读取完整DWORD，不截断、不把非零归一化。MOV/RET保持ECX、EDX和flags。
原字段访问失败发生于读取前，原RET访问失败发生于读取之后。

## 2. 实际共享字段与语义结果

`query_legacy_battle_actor_target_selection_latch`直接借用实际`const u32*`。
队员字段属于`startup.group_a_runtime_reset[index]`；敌方字段属于
`startup.group_b_lifecycle[index].runtime_reset`。生命周期及双方不同
索引的字段保持，查询不再经token resolver，不增加状态副本或写回。

结果仅保留状态及可选实际DWORD。字段不可读时value为空；RET访问失败
保留已经读取的实际值。空字段表示原共享存储不可达，仍发生原字段停点，
不会得到零或成功。访问描述仅表达这两个既有故障条件。
父结果的可选查询结果区分未执行与已执行，包括失败执行。

删除寄存器request/reply、调用/返回地址表、请求数组和offset、读与调用
计数、栈token、flags/ESP/EIP、trace和两层转发执行器。两个nested
动作dispatcher实际没有本查询调用；删除无业务消费的offset转发及trace合并。
相邻setter现直接置位同一共享字段，见
[目标标记置位](battle-actor-target-selection-latch-set-00478b30.md)。
target selection、gate decay和父级协议继续单独登记为待迁移。

## 3. 全部四个实际调用方

完整LST只有以下四个真实CALL：

```text
00456B60 -> 00456B65
00456F12 -> 00456F17
00457140 -> 00457145
00457EC3 -> 00457EC8
```

前三处读取当前队员的同一实际字段，第四处读取当前敌方字段。
读取仍发生在前驱操作之后，没有提前缓存或跨角色替代。

### 3.1 队员选择目标后：00456B60

先保留可达的标记置位和目标写入，再读当前队员。`CMP EAX,1`仅精确1
进入选择完成后缀；0、2、00010001、80000000及FFFFFFFF均不是1。
原EDX直接从前驱terminal或target selection沿用。失败保留置位/选择
前缀并阻断比较两侧后缀；字段失败沿用前驱EAX，RET失败使用实际读值。

### 3.2 队员检查目标：00456F12

前驱目标回合完成值为0时，先写53C010=1及角色WORD，再读当前队员。
`00456F17 TEST EAX,EAX`后，`00456F19 MOV EAX,[53AE7C]`不改变TEST
结果；`00456F1E JNZ 0045704B`接受任意非零DWORD。此处不能使用精确1
判断。零分支不替代非零分支的敌方启动门递增。失败保留前缀并阻断
TEST、MOV及全部候选/递增/列表后缀。

### 3.3 队员行动收尾：00457140

先清目标及本地共享门，再按已捕获的SI低WORD比较FFFF；哨兵绕过查询。
其他值查询后`CMP EAX,1`：精确1遍历敌方衰减，否则只衰减选择的对象。
两种查询故障均保留目标清理及九处共享清零，阻断衰减与提交后缀。

`0045714A`在精确1分支先重新加载敌方数量到EAX，再执行TEST和循环。
旧桥接遗漏此reload，本批首个衰减调用改用实际敌方数量，EDX仍从target
clear沿用。数量2、标记1的首个衰减读失败保留EAX=2；非一标记的
单对象分支仍按原索引算术重建EAX/EDX。其余循环和衰减协议尚未迁移。

### 3.4 敌方行动收尾：00457EC3

行动返回精确1后，先清53BF9C、读取并保存target signedWORD、清目标，
再读同一敌方字段。`CMP EAX,1`仅精确1遍历队员，其余走选择对象分支。
`00457ED1`按内存比较队员数量，不覆盖EAX，因此首次队员衰减仍接收
实际标记1及target clear的EDX。不能套用队员收尾的数量reload。
查询失败保留target clear和前面的共享清理，阻断比较、衰减、reset后缀。

## 4. 双向复核与独立向量

字段读对应实际借用及value发布；RET对应完成或已读值保留的原故障。
四处CMP/TEST对应各自完整值条件。仍未迁移callee的EDX来源和原EAX
reload在父层直接接线，叶结果不回传寄存器。原访问停点没有扩展。

独立测试覆盖七类完整DWORD、双方不同索引、后续写入重读、不改字段、
缺失字段、两种原故障，以及四个caller故障前缀。队员索引2读取自身
字段，索引0的精确1不能替代它。实际目标选择写入后读取同一标记。
队员目标检查借两名敌方：非零分支把门7→8及9→10；零只递增所选
对象，另一门保持9。00456F3C返回非一时，00457049跳到00456FE1递增
所选对象，不能把零分支断言为不递增。双方收尾通过
全体/单对象实际衰减分支及其失败前缀证明精确1比较；高位00010001
不被当成1。队员数量2验证首个衰减失败前的数量reload。

## 5. 定向验证与未完成范围

当前源码与测试的实际门禁均通过：

- core setup：1/1，10.87s。
- core actor_frame_316：1/1，31.05s。
- AddressSanitizer setup：1/1，19.06s。
- AddressSanitizer actor_frame_316：1/1，33.08s。
- SDL openswd3编译并链接。

日志为`build/tmp/runtime/target-latch-direct-{core,callers-core,asan,
callers-asan,sdl}.log`。只有ASan setup含既有结算测试133行WORD转BYTE
警告一次；其他四份日志无警告/错误，未修改该无关测试。
`verify-target-latch-direct-publication.py`及`target-latch-direct-publication-check.log`
确认八份源码/测试身份、四个真实测试、SDL、两条完整指令、四个实际
caller、原故障顺序、两类比较及不同EAX/EDX继续路径和空白检查。

数量reload前被停止的运行、前驱默认返回1导致递增向量未进入预期路径
的失败，以及误把零分支断言为不递增的失败/诊断，都仅为历史，不作为
验收。按LST 00457049→00456FE1修正两名敌方向量，并明确模拟前驱
真实条件后重跑全部门禁。源码或测试改变后旧日志不再支持验收。

Workpack313原CPU合同、199/199、205/205及十轮验证已归档在Git历史，
不代替当前语义接口和实际调用方的验证。分类仍为`platform_adapted`。
原版动态差分为`blocked_runtime_oracle`；没有启动游戏程序。
B10游标、Workpack316/318、B11实际续玩及整体通用调用迁移均未升级。
