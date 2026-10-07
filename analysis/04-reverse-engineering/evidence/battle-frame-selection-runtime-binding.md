# 战斗帧选择等待与行动出队生产绑定

状态：B11限定生产接线及共享状态迁移已通过定向验证。
范围：`0045328C..0045331C`及出队调用内组A查询的固定记录映射。
不关闭WP316或通用`0047F920`，不声称完整战斗帧或实际续玩通过。

## 1. 原指令顺序

- `45328C/453291`分别加载选择mode和选择值；只有mode精确1且值全1时清mode。
- `4532A7..4532C2`依次判断队列首项非全1、选择active为0、帧开关精确1、
  已加载mode为0；不把非零开关等同于1。
- `4532C4`按unsigned WORD比较延迟和16。小于16时只递增WORD；其余调用
  `45F020`，不把FFFF误解释为负值或先递增至零。
- 调用完成后`4532D8`重读输出首DWORD。非全1时，依次清延迟、置active为1、
  把该位形写入`53CE70`。空结果保留这三项原值。
- 沿用ECX中的选择值。全1时才读`53BD54`，先发布`53C044=1`，非零再改0；
  非全1直接发布0。下一项从`45331C`读取画面效果状态。

## 2. 唯一共享状态

`prepare_legacy_battle_frame_selection`由完整核心协调器和SDL帧入口共用：

- `53BFC0`仍是`action.action_pending_aux`。
- `53AE70..53AE8B`仍是metric优先角色与后六DWORD。
- 攻击队列仍是startup的18个28字节记录；相邻区域借用效果协调器的强度记录。
- `53BF74`与`53BD54`分别借用final actor的selection gate和queued actor code。
- `53BF1A`保留协调器的16位delay，未建立另一份计时器。
- `53CE70`直接写实际`LegacyBattleScriptWorkspace.coordinate_y`，通过位形转换
  保留高位；删除协调器的auxiliary副本。
- `4A7B58`统一到`action.frame_enabled`，初值来自原DATA的DWORD 1。
  删除脚本shared、协调器、组A帧、组B帧、调试显示中的五个独立副本。
  脚本50处写入、选择判断、双方角色帧和调试文字都使用该字段。
- `53C044`统一到`action.actor_progress_gate`，删除协调器及双方角色帧的副本。
  两组角色进度调用仍在原位置读取，保留原有signed参数转换。

SDL的action引用绑定`battle_actor_frames_.shared.action`；脚本与选择前缀借用
同一对象。全局重置不写上述两个地址，新测试确认原有非默认值仍被保留。
旧组帧测试此前依赖私有字段默认0，迁移后将其暂停输入显式写为0；生产初值
不为满足旧夹具而改回0。

## 3. 实际组A查询

`47F920..47F934`先测试`byte[actor+26D0] & 40`，命中返回1；否则返回
`DWORD[actor+2AD0]`，不写ECX或EDX。

`LegacyBattleAttackOrderRuntimePort`按实际计算后的guest地址解析十个组A槽，
读取`startup.party[index].progress.mode_gate`及同一action的`special_mode`。
前者是启动重置及实际角色记录映射使用的存储，不读取帧结构的progress副本。
不以当前活动人数限制物理槽，也不以逻辑actor index代替地址解析：
`40000008`的乘法回绕可落回第一槽，必须读该真实槽。

值7形成基址之前的地址；未映射地址显式返回callee未完成，出队在查询后停止，
不把缺失映射当返回零。此前已完成的查询副作用保留，不复制输出或清理队列。
完整协调器窄端口同时传播该标记，阻断效果、角色帧和绘制后缀。

寄存器字段属于显式调用模型；SDL不声称捕获了原版EDX或CPU现场。
本固定调用域不替代其他`47F920`调用者的最终审查，WP379保持pending。

## 4. 既有出队语义保持

[完整出队证据](battle-attack-order-dequeue-0045f020.md)中的signed比较、
七DWORD复制、无界28字节扫描、相邻强度块、满表从原选中索引清尾以及逐访问
typed-stop均保持。只新增角色查询未完成的传播，不改原队列算法。

## 5. 入口及停止线

只有前帧、输入、调试和画布前缀正常继续时，SDL才执行本批。
画布中止仍按原路径返回；输入的已有停点没有被绕过。
选择正常完成后，SDL停在`45331C`后续画面效果首读前；查询失败保留此前状态，
以`4532D3 -> 45F020`报告边界，不伪造正常帧返回。

## 6. 验证与剩余范围

新增向量覆盖精确DWORD门、WORD阈值0/15/16/FFFF、七DWORD输出、空结果保留、
已选值与queued值组合、bit6与special mode完整值、值7映射失败、地址回绕、
查询回调改写后的重读与失败前缀、脚本暂停/恢复控制同一选择门，以及完整
协调器停止后零后续调用。双方角色帧与原全局重置测试同步验证共享字段。

`proc_73ce`完成最终验证并退出0：

- core `battle.legacy_battle_setup` 1/1，4.39秒。
- core `battle.actor_frame_316` 1/1，25.68秒。
- ASan相同两个目标各1/1，分别6.92秒和27.69秒。
- SDL `openswd3`目标链接通过，无warning/error。

日志位于`build/tmp/runtime/battle-frame-selection-final-`前缀的
`core.log`、`callers-core.log`、`asan.log`、`callers-asan.log`和`sdl.log`。
setup在core和ASan重编译时均出现既有
`legacy_battle_outcome_resolution_test.cpp:137`的u16到u8转换警告；没有修复
无关警告，也不声称所有日志零warning。

初次迁移编译发现四处旧字段引用，补齐后四个setup夹具和角色帧目标的16条
断言依赖旧私有门默认0；显式保留其暂停输入后重跑上述目标，全部通过。
生产DATA初值仍为1；脚本→选择的独立组合测试覆盖暂停和恢复，未跳过原行为。

未启动游戏，未新增原版动态差分。完整帧、四处角色帧生产绑定及实际续玩
仍待完成；WP316整体工作量暂估60%，不是验收比例。
