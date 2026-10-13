# 战斗角色资源链头发布 `0x00470900`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`。
本次INVOKE切片源码为`3eda365b`；完整列表及三个网格接口族仍为pending。

## 1. 完整权威范围

权威LST主体为`0x00470900..0x0047090C`，proc至endp共8行、3条实际指令，
0个call、0个跳转、0个标签、1个返回点，没有外部`FUNCTION CHUNK`。

```text
00470900 mov eax, [ecx+2ECCh]
00470906 mov [ecx+2EC8h], eax
0047090C retn
```

原ABI以ECX借actor，EAX为读取的next head，EDX未写。实际数据行为只有
把`+2ECC`完整DWORD发布到`+2EC8`，零、相等值、高位和未知节点均写入。
本叶不访问节点，不改next、selected、数量或节点所有权，不分配和释放。

## 2. 十五个实际调用与发布顺序

只统计带opcode的call，排除一条`__thiscall sub_470900`说明行。
15处call分属11个parent；独立摘录窗口共770行。

- 普通网格：`465B95/465E2A`。初始化计数后发布，尾发布仅在查询耗尽分支。
- 替代网格：`465FE6/466166`。满页仍查第八项；非零时保留该节点，不尾复位。
- 模式网格：`466318/466365`。首组查询和row-limit写入后、次组累计后分别发布。
- 列表状态：`47045F`。先写primary required，再发布并读取实际资源链。
- 资源行查询、计数、选择：`470918/470A60/470B4D`。分别保持发布后清flags、
  保留调用者计数初值、先清选择前缀再发布的顺序。
- 释放：`470E26/470F22`。前批`5ad2a9aa`已改为实际共享写，本批不重复改动。
- bit13查询、模式查询、模式累计：`470F76/470FE7/471084`。后继重读current；
  471084发布后先清输出WORD，首次节点失败保留两项副作用。

这些调用的后继在业务消费前覆盖叶EAX或重读共享head，不需要叶寄存器回包。
模式格公共文字前重写EAX=cell、ECX=selected cell，颜色不依赖发布回包。
未知head只在后继首次节点访问处沿原有失败状态停止，发布本身不增加节点检查。

## 3. 实际接口与父消费

叶只借`LegacyBattleActorListQueryState*`，返回现有访问状态。
缺list沿既有`list_owner_typed_stop`；存在时直接发布actual next到current。
actor token与入口EDX不再是本叶参数；不以诊断token替代实际owner存在性。

删除叶result、EAX/ECX/EDX、head_writes、六查询/选择的commit_calls，
以及列表状态的rebuild_calls和嵌套发布回包。七处列表消费和三网格六处发布同步。
三网格发布始终借原actual party槽位，脚本兼容查询也不得伪造该叶回复。

共用GridCall::refresh_actor和协调器映射保留：窄网格`466500`调用的是
`46FFE0`索引发布。首编译发现误删共用标签后恢复该独立路径并重跑，未扩大本批。
原失败日志与源码身份保存在`build/tmp/runtime/`同前缀材料。

列表及网格其余查询、字体、参数包、寄存器、计数和Port仍待迁移。
这些剩余项和特殊模式同名操作不因本叶清理而关闭或盲删。

## 4. 固定状态与实际门禁

新增83次固定执行，使用既有注册入口：

- 六DWORD值与相同/陈旧current，共12次，核对next、selected、节点地址和全部数据保持。
- 缺list一次，沿原访问失败返回。
- 五实际查询/累计consumer与有效/缺失首根，共10次；核对清零或保留初值时点。
- 三网格各十实际槽位与两条资源链，共60次；核对满页、耗尽复位和后继selected。

既有列表、释放、菜单和网格测试同步；本批12份源码身份与完整暂存差异一致。
core setup/actor定向各1/1，分别11.99/27.69秒；ASan各1/1，分别17.39/29.56秒。
SDL 136/136实际重新链接。四次测试均真实通过，构建未运行游戏。
LST摘录、独立分析、双向审查、首失败、四门日志、源码SHA及发布身份均保存于
`build/tmp/runtime/resource-head-publication-*`。

未执行全量UT、Windows、actual MON、游戏运行或原版联合动态差分。
不标assembly_exact，不改变315/422、Workpack316 pending_audit、B11或整体Goal状态。

## 5. 历史还原验收

原196/422阶段曾执行core188/188、app194/194及双生成，历史SHA256为
`61edff9d9172acbf3fe675a6ca334657d402de59dffe303618b924e2f702b412`。
这些旧全量结果不作为本次门禁。原版链头与caller联合捕获仍缺失，
`original_diff_verified`保持`blocked_runtime_oracle`。
