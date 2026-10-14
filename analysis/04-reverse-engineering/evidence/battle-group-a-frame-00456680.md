# 组A战斗帧主循环 `0x00456680`

状态：`pending`。

## B11：组A借实际优先角色

456DE8的CMP结果在idle callee返回后不重做；456E9C按下一站重读。
当前角色WORD从实际priority截取，53AE70不再由active_effect_target承接。
4572AB reset返回后，4572C0完整清priority及其六DWORD尾，随后发布全1。
非53AE70角色字段保持原用途。正常与失败向量、门禁状态见
[实际角色与优先角色](battle-frame-effect-00453580.md#21-b11画面效果借实际角色word及优先角色dword)。

## 行动后直接扣除角色消耗

0045703D借实际party的列表、物品效果、配置记录与workspace，直接扣除行动消耗。
删除该调用的寄存器回包和调用计数；失败保留已发生的数据变化，阻断后续帧处理。
上层续接45735E重载EAX，不读取行动函数寄存器结果。帧测试检查真实扣除和前置失败保留。
core/ASan定向及SDL构建通过；范围与剩余事项见
[角色行动消耗](battle-actor-list-action-00470820.md#6-行动消耗父协议迁移)。
其他组A协议与完整SDL接线仍pending。

## 启动计数的直接递增

00456FE1与0045707A直接借敌方共享WORD和DWORD，保留原访问顺序、
失败前驱、部分扫描和后缀抑制；删除本操作的请求、轨迹、计数与转发。
原目标路径的失败返回区分终态返回和实时runtime DWORD，
重新选择路径才采用地址算术结果。范围及本轮验证状态见
[启动计数递增](battle-actor-start-gate-increment-00478ac0.md)。
其他组A协议和完整资源路径仍未验收。

## 目标选择标记的直接读取

00456B60、00456F12和00457140直接读取当前队员的实际共享DWORD。
第一及第三处仅接受精确1，第二处保留任意非零的TEST分支。
查询请求、地址/寄存器轨迹、计数、offset和转发执行器已删除。
原字段/返回访问失败仍保留前缀并阻断后缀；首个敌方衰减在
0045714A按原版重新加载敌方数量。core/ASan setup与角色帧各1/1、
SDL链接均通过；两个敌方向量区分非零遍历与零值只递增选择对象。
本查询的范围、独立向量和门禁见
[目标选择标记](battle-actor-target-selection-latch-query-00478b40.md)。
整条组A及动作父链的其他通用协议继续待迁移，未升级整体验收。

## 实际角色叠加绘制清零

五处`0047C660`改为直接清除对应共享DWORD。所选队员、选择地址和
遍历对象各自借用实际字段，保持回绕别名、先清标记后计数/动作的顺序。
已删除这五处通用调用及对应操作编号；失败不再采纳端口伪造的EAX。
当前验证状态及剩余调用见[叠加绘制清零](battle-group-a-overlay-clear.md)。

## 目标数量的直接递增

00456A1F、00456CDD与00456D71均直接修改实际共享WORD。
请求/寄存器回复、地址表、请求数组、轨迹、计数、resolver及纯转发已删除。
扫描保留清叠加标记后计数、计数成功后progress的顺序；两条选择保留
清标记后重读选择、完整地址回绕及递增后DEC。字段/写入/返回故障保留各自前缀，
阻断目标发布和后续门清理；跳过路径没有伪造成功结果。
第一选择对应00456CEC，AI使用的第二选择对应00456D80，旧标注已纠正。
实际字段、索引端点/别名、独立向量及本批门禁见
[共享目标数量递增](battle-actor-target-selection-count-increment-00478aa0.md)。
本父函数的其他操作、资源及完整内存别名继续待迁移。

## 1. 完整LST范围

权威函数为`0x00456680..0x0045769B`，完整1795行、101个静态call站点、84个`loc_`标签，无外部FUNCTION CHUNK。唯一caller为尚未关闭的`0x0045B5E0`，当前不提前计数。

ABI读取一个组A索引并固定返回1。组A token按低32位建立：

```text
0x005029D0 + index * 0x2F34
```

入口索引只在首次对象callee处typed-stop；函数内部所有组A/组B计数循环同样只在对应迭代首次对象访问处停止，并保留此前迭代副作用。

## 2. 画面效果门

先查询当前组A对象的效果状态。只有：

```text
query低word非零 && (primary suppression == 1 || secondary suppression == 1)
```

或独立global override等于1时，才向对象发布mode 1；否则发布mode 0。`&&`与`||`优先级保持LST，不把global override错误并入query门。

B11的4566BC/4566C4现借实际BF94/BF98双门，BF30灰度分带独立。
收尾4572FE只检查primary精确1才触发淡出；secondary或BF30为1不替代。
四种完整门值的收尾回归通过core/ASan actor_frame_316各1/1，SDL链接通过。
范围与日志见[双门与实时颜色](battle-frame-effect-00453580.md#20-b11双抑制门与当前颜色共用实际存储)。

## 3. AI协调与随机对手

AI协调只在全局enable等于1、两个pending门都为0时进入：

1. 对当前对象执行prepare；
2. global gate完整EAX等于1后advance；
3. 当前角色两个AI标记任一等于1时，遍历`group_b_count`个组B对象统计terminal完整EAX等于1的数量；
4. 尚有非terminal对象时发布selection mode 1，并执行无现代上限的随机重试：`random(group_b_count)+1`作为one-based组B索引，直到terminal查询不等于1；
5. actor scene查询成功后分别查询两个AI完成callee并写当前角色状态。

计数为0或全部terminal时走对象reset。随机callee超约定结果只在首次one-based组B对象访问处typed-stop。

当前角色无两个AI标记时先播放固定sample，再扫描十槽actor queue的首个0。队列已满则不写；有空槽且queue mode等于1时，直接组合已关闭角色目标准备：向唯一已提交角色、动作workspace、动作/提交门和published actor owner发布`index+8`，按processed低byte与live group-B count决定是否从secondary RNG起点跳过已完成目标。其子typed-stop阻断本角色余下流程，旧callee token零调用。queue mode不等于1时仍把`index+8`写入空槽。

## 4. 十槽actor queue

从队首向后扫描，同时要求：

- queued actor code仍为0；
- current actor低word为`0xFFFF`；
- 当前槽非0。

槽值按`code-8`形成组A对象。queue completion不等于1时：

- queued actor code写该槽值；
- 若槽号小于9，把后续槽逐项左移；
- 固定把最后槽清零；
- 结束扫描。

`code<8`在首次派生组A对象查询点typed-stop，不提前清理非法队列。

## 5. 角色frame启动

当前角色frame尚未启动、queue completion为0、idle查询为1、available查询为1且两个动作阻塞门为0时进入。两个独立早期reset条件：

- queued selection word非`0xFFFF`，且`group_b_count - processed低byte <= 1`；
- message state为99，且独立actor-start guard word为0。

随后写frame started、active actor code=`index+8`，直接组合已关闭攻击顺序插入：以类型1、值`index+8`和全1位置把队员暂存数据搬入共享18条记录首空槽，并清对应源与双尾门。旧启动callee token删除；子typed-stop保留frame started和active actor发布，阻断后续AI与最终角色尾。actor-start guard位于相邻共享地址，不能与turn-resolution word合并。

## 6. 角色AI执行阶段

只有actor enabled等于1时处理。

### 6.1 action complete尚未置1

mode gate非0时，selected one-based值解释为组A对象。`0x00456DD6`直接借`group_a_action_execution[selected-1]`实际完成DWORD；查询不再携带编号、寄存器、栈或计数，原索引验证及字段失败时父级算术返回保持；完整latch为0、active effect target不等于`selected+7`且当前角色idle为0时清控制/呈现。delay callee返回1后发布`selected-1`、finalize actor并清一组selection门。getter停止发生在caller后续TEST前，阻断当前目标分支与全部后缀。

mode gate为0且当前角色idle为0时：

- 当前角色两个AI标记任一非0：selected opponent one-based按实时回绕地址借实际计数，清控制后递增，再在00456D80发布`selected-1`并清门；
- 两个AI标记均为0：delay成功后，selected actor one-based仍按实时回绕地址借计数；清控制后递增，再在00456CEC发布、finalize actor并更新UI门。

同一个共享one-based值在不同mode下可指向组A或组B；modern不统一成单一对象域。

### 6.2 action complete等于1

selection mode非0时遍历组A：两个AI标记都不等于1、other actor查询不等于1且当前角色idle为0时累加progress。达到`group_a_count - defeated byte - packed highword - excluded lowword`后，从最后一个组A对象向前找首个非terminal。

selection mode为0时遍历组B：terminal对象及已映射对象计入terminal-like；非terminal且映射全1时清控制、直接递增该轮实际目标计数，正常返回后才累加当前队员progress。失败保留此前轮次，阻断当前progress及余下选择。达到`group_b_count-terminal_like`后从第一个组B对象向后找首个非terminal。

选中后先发布target-ready，00456B51直接把当前队员共享目标标记写为1。
正常返回才选择索引并读取同一标记。原字段写/返回故障保留ready前缀，
阻断选择与查询；后续选择沿用实际terminal EAX/EDX及CMP结果。
嵌套动作只汇入已执行的置位状态；旧置位offset、轨迹和计数已删除。
见[共享目标标记置位](battle-actor-target-selection-latch-set-00478b30.md)。
双方帧及动作其他协议仍待迁移。

随后selection complete等于1时按AI标记或delay成功走两类门清理；delay mode等于4时另调用固定finalize。

## 7. 已关闭动作主分派caller回收

当active effect target等于`group_a_index+8`且action execution已激活：

1. 发布两个动作门与current actor；
2. prepare当前角色；
3. 查询目标低word；
4. **直接调用已关闭`dispatch_legacy_battle_action`**，不再保留`0x004539B0` opaque端口；
5. 合并callee端口计数、清屏次数、循环计数与typed-stop；
6. callee返回1才进入完成尾。

完成尾再次查询目标并清当前动作，随后清动作门、相邻高byte、stage word和selection mode。目标不为`0xFFFF`时：

- selection complete为1：遍历全部组B执行target reset；
- 否则按独立action side把目标解释为组A或组B并reset；
- 调用post action；
- 随后无论action side如何，都把同一目标解释为组B并查询terminal；
- terminal时可继续查询关联组A目标，并在`group_b_count + processed第三byte - processed低byte <= 1`时遍历全部组A发布固定值1；
- battle byte bit`0x80`置位时遍历组B，对非terminal对象执行clear，再只清最低byte的该bit。

公共cleanup调用actor reset，清连续28字节active-effect块、角色post/scene、多个共享门和值、两个stage word、cleanup word、action runtime低word与post counter，最后把active effect target写全1。primary suppression等于1时置fade；global phase低15位非零时只递减低word。

## 8. 动作准备路径

active effect target匹配但action execution尚未激活、action block为0时：

- 查询目标低word并形成组B对象；
- `0x00456EEB`直接借startup `group_b_lifecycle[].action_execution`实际完成DWORD；字段/RET失败抑制后缀，父级保留原算术返回或观察值，删除本查询运输的EDX/flags与栈协议；
- latch为0时置execution、current actor；getter停止保留动作目标查询残值并阻断current actor、selection及全部动作准备后缀；
- selection complete非0且action side为0时遍历全部组B：对非terminal对象做第二次terminal查询，第二次等于0才prepare target；一个非terminal都没有则撤销execution；
- selection complete为0、action side为0且独立target guard highword为0时，若原目标terminal且runtime word为0，则找首个非terminal组B对象。找到前按原顺序clear actor action、reset固定“组B前一槽”token，再发布索引；
- execution仍为1时prepare最终目标；
- 当前角色text byte非零时显示固定文本并清五个text runtime dword；
- 最后begin actor action。

独立target guard来自相邻未对齐dword的高word，不复用turn-resolution word。

## 9. 回合结算状态机

仅在action block为0、当前组A对象非terminal且两个AI标记都为0时进入。

### 9.1 bit`0x4000`

先置两个pending门。advance turn mode0返回1时，input mode只递增低word。比较域是u32：

```text
u16(input_mode) >=
    u32(group_a_count - defeated_byte - packed_highword - excluded_lowword)
```

达到阈值后清phase高word，遍历组B非terminal对象：resolve target零token在首次`+0x54`读取处typed-stop；取word54无符号最大值写phase高word。原BUG使用当前`group_a_index`查询映射，而不是循环组B索引；该映射非全1时强制最大值200。

phase最大值为0时先清turn word、input低word写1、清phase高word与两个pending门。随后仍调用turn commit；参数高word保留`group_b_count`高16位，低word覆盖为phase最大值。成功时：

- 对当前组A对象发布结果1；
- turn word写`0x8000`；
- 清selection/queue/phase；
- input低word使用独立尾算术：先u32减defeated byte，再只在低16位减packed highword，最后u32减excluded lowword。

失败时遍历全部组A发布结果0，按门显示固定文本并播放sample，随后清turn、input、phase和两个pending门。

### 9.2 signed bit`0x8000`

bit`0x4000`阶段结束后会在同一次调用重读turn word，因此成功写`0x8000`可立即进入本阶段。置两个pending门并清selection/queue；若当前actor bit未置且advance turn mode1返回1，则：

- overlay gate写1；
- OR入`1<<group_a_index`；
- defeated byte按u8加1回绕；
- 用u32比较`defeated >= group_a_count-packed_highword-excluded_lowword`。

达到阈值时显示最终文本、清十槽队列、message state写104、turn word清零、active effect target写全1并清504字节workspace。

两处turn advance都直连已关闭的回合门；其内部`0x0047172C`坐标publication现进一步直接组合typed leaf。publication fault由回合门完整传播到本函数，并在候选累计、bit标记、defeated计数、完成消息、最终尾与其他父级后缀之前返回；旧raw `0x004785C0`生产调用为零。

## 10. 最终尾

无论普通路径如何，最后调用固定`final actor step(group_a_index,1)`。完整EAX等于1时清待执行动作提交与本函数共用的唯一activation latch，并把final selected word写`0xFFFF`。函数正常返回固定1。

## 11. closed callee、端口与typed边界

两个pending门分别是BFC0与BFC4，当前由动作状态的`action_pending_aux`和`selection_cache_gate_b`保存，组A/组B及调试快捷键借用同一状态。结果判定BF5C另由动作状态的`resolution_latch`保存。此前将第二pending门与结果latch合并的结论错误，已按独立地址修正；本轮验证进度见[状态区分证据](battle-selection-wait-and-result-latch-0053bfc4-0053bf5c.md)。最终尾activation latch也已回收到actor metric唯一state，由待执行动作提交置1、本函数成功尾清0、全局重置同步清0。

46个唯一callee中：

- `0x004539B0`已直接回收为typed动作主分派；
- `0x0045EE70`已直接回收为typed攻击顺序插入；
- `0x004698E0`的三处调用已直接回收为typed文字消息入链；
- `0x00478330`的六处调用已直接回收为typed角色可用性写入；
- `0x00478690`两处调用直接借实际目标的完成DWORD，删除查询解析、机器请求回复、计数和转发；
- `0x004786A0`四处调用直接借当前队员实际空闲DWORD，查询不再携带编号、token解析、寄存器、栈或计数，亦不经过转发函数；
- `0x004786D0`的公共效果模式启动门调用已直接回收为typed word查询；
- 其余待审角色、AI、选择、文本、sample和数值callee继续使用单一typed token端口。

其余待迁移的对象、固定前一槽、scene与文本操作仍含`compat::u32`地址协议。已迁移的标记和数量操作借实际共享字段；数量递增只在调用方保留原回绕地址选择及后继需要的计算前驱。未映射宿主存储不证明原地址必然异常。

Typed-stop只位于：

- 入口组A首次对象callee；
- 计数循环每次首次组A/组B对象callee；
- queue code派生对象首次查询；
- one-based随机/选择首次对象callee；
- 两处回合完成getter的canonical字段或RET返回地址读取；
- 四处空闲状态getter的canonical字段或RET返回地址读取；
- 公共效果模式启动门getter的canonical字段或RET返回地址读取；
- 三处目标数量递增的实际字段读、写或RET返回访问；
- action target首次组B对象callee；
- resolved target零token后的word54读取；
- 已关闭动作分派自身真实访问点。

## 12. 验证与动态差分

定向测试覆盖：

- 入口组A越界；
- effect mode组合门与固定最终尾；
- AI terminal统计、one-based随机目标及属性辅助标记；旧两个角色完成替代的解释按第15节校准；
- 十槽queue首个未完成项与左移；
- idle actor启动后的攻击顺序记录、队员暂存源和双尾门真实访问点；
- completed actor组B扫描与首个live选择；
- 两处回合完成查询使用实际目标DWORD及可选结果，保留完整值、字段/RET失败顺序、前驱返回和后缀抑制；父项其它协议仍待迁移，见[回合完成查询](battle-actor-turn-completion-00478690.md)；
- 四处空闲查询使用共享实际字段和状态/可选值，保留完整DWORD、即时读取及字段/RET失败前缀；失败时父级保留已有返回，完整父项其它协议与资源仍待迁移。当前证据见[空闲状态查询](battle-actor-idle-state-004786a0.md)；
- 公共显示调用直接读取Group-A实际启动计数，完整WORD零/非零与门值精确1组合、失败保留前驱高EAX和观察值、显示及后缀抑制；查询协议已删除，当前证据见[启动计数查询](battle-actor-start-gate-004786d0.md)；
- active action直接调用已关闭主分派，确认端口不再出现旧callee token并完成全cleanup；
- action target `0xFFFF`首次组B对象typed-stop；
- turn `0x4000→0x8000→0`同调用穿透；
- resolved word54最大值、stale turn参数与失败尾；
- queue code小于8的派生对象停点；
- 原工作包记录46个唯一callee，其中4个整函数typed直连、42个端口边界；该计数是历史范围，不代表当前INVOKE验收。turn gate内部坐标publication另为typed leaf直连并覆盖fault后缀抑制。

## 13. `0x00478330`六处直接写入

相邻工作包278进一步关闭本函数内`0x004567EA`、`0x00456BC0`、`0x00456C1C`、`0x00456CF4`、`0x00456D88`、`0x00456E69`六处物理call。第一处把完整dword `1`写入当前组A角色`+0x2AE4`，其余五处写`0`；全部直接复用`LegacyBattleFinalActorStepState::group_a_availability_blocks`，旧callee token生产零调用。第一处保留末次组B完成查询EDX，后五处分别线程化最终处理、发布选择、选择完成或清展示callee的真实EDX；leaf在写失败时已把参数装入EAX，并保留角色token ECX与该EDX，父函数立即返回，不执行随机选择、门复位、最终处理或UI尾部。queue mode路径直连`0x00464CC0`时也通过同一owner完成其第七种组A帧到达方式。

当前缺少原版组A/B对象、其余待审callee共享副作用、攻击顺序/队员暂存动态轨迹、AI/选择/队列表、text/sample、resolved target内存与回合/空闲状态联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。

## 14. 动作目标word查询四处caller直连

工作包296关闭`0x00456EC9`、`0x004570CC`、`0x004570EC`和`0x004571E8`四处物理call，真实返回地址分别为`0x00456ECE`、`0x004570D1`、`0x004570F1`和`0x004571ED`。第一处为空闲角色目标读取；第二处继承动作启动callee回复并把AX作为嵌套动作分派目标；第三处继承嵌套分派成功回复，返回后才保存完整EAX并清动作；第四处继承目标terminal回复，返回后才比较AX与`0xFFFF`并按非哨兵值重置Group-A角色。

四处直接复用当前Group-A action-execution的canonical动作目标，不增加generic端口调用。嵌套结果按真实调用顺序合并逐次typed结果，保留首轮和后续轮EAX/EDX、ECX角色token、SUB/CMP flags及返回地址。待审`0x00478B20`清动作和`0x00478A70`目标重建均同步唯一canonical字段。typed-stop只保留当前物理caller之前的空闲查询、动作启动、嵌套分派或terminal前缀，并抑制对应符号扩展、清理、剩余循环和公共尾；生产`0x004786E0` raw调用为零。

当前缺少原版完整Group-A/Group-B actor、动作启动与terminal等剩余callee共享副作用，以及四处caller联合寄存器、flags和SEH捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。

## 15. 自动选敌人后的实际属性标记

5b44fdb2删除456859→47D8D0通用查询，直接读实际actor+0D94 BYTE bit0。
真写startup.reset辅助column1，假保留旧DWORD；4569C4精确1消费者读同一记录。
它不是actor+2AB0，真实进度前继合法置角色完成1；原最终45AA00→4750C0
清十个profile DWORD，当前属性随后清零，不能用返回后的保留预期否定前缀。
夹具借唯一startup.reset，285次新执行与既有路径同步，core/ASan
actor_frame_316各1/1 26.69/28.03秒及SDL实际链接、两源身份通过，见
[队员帧目标标记](battle-party-frame-target-selection-0047d8d0.md)。
当批456844的bit1仍有通用查询和错误辅助替代，随后按第16节校准。
其它父协议、计数、资源、完整出口与运行验证仍pending，不升级整父验收。

## 16. 坐标属性辅助记录与攻击顺序消费

45f5a77e回收456844→47D880，实际BYTE bit1真写startup.reset辅助column2。
假保留旧DWORD，actor+2AB8真实状态保持；它在46E520的精确1进度门独立有效。
生产frame_coordinator已让攻击顺序源借同一数组，夹具删除重复源数组。
新增279次帧及270次实际插入，验证五源值、WORD截断、两属性列、复制后清行；
285次bit0同步撤回纯叶回复并在readiness观察前缀。
core/ASan actor_frame_316各1/1 26.41/27.78秒及SDL实际链接、两源身份通过，见
[队员帧坐标属性](battle-party-frame-coordinate-attribute-0047d880.md)。
当前其它六/六/四属性父点及完整父协议/资源/出口/运行验收仍pending。
