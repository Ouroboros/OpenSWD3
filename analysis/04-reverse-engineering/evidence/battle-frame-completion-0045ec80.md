# 战斗逐帧双方完成数协调 `0x0045EC80`

## 行为依据

权威LST为`0x0045EC80..0x0045EDEF`，唯一caller为`0x00453200`。
两个静态call均调用`0x0047E580..0x0047E5B6`，固定mask为4。
该callee从actor+0x2584读取链表头，沿node+0的next遍历，检查node+0x9C
与mask低16位的交集。首个命中返回1，空链或遍历结束返回0。
没有写入、嵌套调用或角色数量修改；此前测试端口模拟的数量修改和任意返回值
不代表该callee行为。

## 实际数据与调用

帧完成检查不再使用Call/Request/Reply、CompletionPort或操作编号。
组A链表头借用`startup.party[].base_initialization`，组B链表头借用
`startup.group_b_lifecycle[].base_initialization`。
节点借用`LegacyBattleActorFrameLinkedNode`集合；它与角色帧释放路径使用同一
节点类型，保存身份、next和状态mask。查询直接遍历节点，不调用返回预设布尔值的端口。
协调器通过`actor_action_nodes`借用调用方持有的节点集合，不复制或取得其所有权。

组A跳过字段直接读取实际动作状态的`action_twenty_seven_motion_mode`
（actor+0x2B00）和角色进度的`scene_identity`（actor+0x2B04），
删除原frame-completion专用字段副本。

节点非空token必须能在借入的实际集合中解析，不能用默认false掩盖缺失数据。
未提供节点或组B角色存储时，报告首次不可读取对象并保留此前扫描结果；
这些错误对应真实数据访问缺失，不是未实现查询的替代返回。
空链不要求节点存储；命中后不再读取next指向的对象。
本批不声称完整SDL角色动作链生产和消费已经接通。

## 分支和发布顺序

当前角色索引不为全1时，不读取两组角色。组A数量为零时直接进入组B门。
组A逐对象读取两个跳过字段，任一完整dword精确等于1时跳过查询。
有效查询后重读数量；跳过轮次保留原上界。第11次组A字段访问仍在读取前停止。

组A完成计数按u8递增，非零时计算：

```text
required = u8(group_a_count.low - action_phase.byte2 - excluded_group_a.low)
available = zero_extend(removed_group_a) + zero_extend(ready_count)
```

满足阈值且共享暗化门为零时，依次写回removed低byte的回绕和、message 0x67，
返回语义结果`group_a_committed`。这些参与比较的值均为非负的小范围整数，
有符号和无符号比较结果相同。

组A未提交时读取组B完成门；非零时返回。组B逐角色遍历链表，每次查询后重读数量。
ready为零时不发布。否则比较packed actor counter低byte加ready与组B完整有符号数量，
再检查暗化门。满足条件时依次写packed counter低byte的回绕和、完成门1、
terminal mode 0及message 0x63，返回`group_b_committed`。

删除入口和出口EAX/ECX/EDX、查询次数、协调器完成检查调用计数和仅供断言的扫描次数。
协调器不再携带post-actor-frame寄存器快照；后续pending-action的入口EDX
原本只用于保存无业务用途的结果残留，其首个prepare调用前已被独立计算值覆盖，
因此不再从完成检查传播该值。pending-action本身剩余通用调用留待后续批次。

## 验证

测试直接提供节点和真实角色状态，覆盖空链、首节点/后续节点命中、非目标位、
命中后的无效next短路、共享节点改写、双字段精确1跳过、组A阈值和u8回绕、
组B packed低byte回绕、暗化门、完成门及读取缺失的前缀保持。
协调器测试验证节点缺失阻断后缀，补齐同一token的实际节点后能够继续。

core及ASan的setup和actor-frame-316定向测试各2/2通过，SDL链接通过。
最终删除协调器调用计数并补充阈值下溢、空链、缺失角色存储及组B暗化门测试后，
core和ASan的setup各1/1重验通过，SDL再次链接通过。
日志位于`build/tmp/runtime/frame-completion-direct-*.log`。
初次setup构建显示既有outcome-resolution测试第133行的窄化警告；最终增量构建无新增警告。
未启动游戏，未声称原版动态差分或完整SDL战斗流程验收。
