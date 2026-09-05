# 组A战斗帧主循环 `0x00456680`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`。

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
query低word非零 && (primary suppression == 1 || split suppression == 1)
```

或独立global override等于1时，才向对象发布mode 1；否则发布mode 0。`&&`与`||`优先级保持LST，不把global override错误并入query门。

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

mode gate非0时，selected one-based值解释为组A对象。目标不busy、active effect target不等于`selected+7`且当前角色idle为0时清控制/呈现；delay callee返回1后发布`selected-1`、finalize actor并清一组selection门。

mode gate为0且当前角色idle为0时：

- 当前角色两个AI标记任一非0：selected opponent one-based解释为组B对象，清控制、选择目标、发布`selected-1`并清门；
- 两个AI标记均为0：delay成功后，另一个one-based选择仍解释为组B对象；随后选择、发布、finalize actor并更新UI门。

同一个共享one-based值在不同mode下可指向组A或组B；modern不统一成单一对象域。

### 6.2 action complete等于1

selection mode非0时遍历组A：两个AI标记都不等于1、other actor查询不等于1且当前角色idle为0时累加progress。达到`group_a_count - defeated byte - packed highword - excluded lowword`后，从最后一个组A对象向前找首个非terminal。

selection mode为0时遍历组B：terminal对象及已映射对象计入terminal-like；非terminal且映射全1时清控制、选择对象并累加progress。达到`group_b_count-terminal_like`后从第一个组B对象向后找首个非terminal。

选中后发布target-ready、prepare selection与索引。随后selection complete等于1时按AI标记或delay成功走两类门清理；delay mode等于4时另调用固定finalize。

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
- target busy为0时置execution、current actor；
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

## 10. 最终尾

无论普通路径如何，最后调用固定`final actor step(group_a_index,1)`。完整EAX等于1时清待执行动作提交与本函数共用的唯一activation latch，并把final selected word写`0xFFFF`。函数正常返回固定1。

## 11. closed callee、端口与typed边界

两个pending门中，第二项与战斗结果判定latch、组B帧和调试快捷键共用唯一`LegacyBattleOutcomeResolutionStatePort`，不再保留组A帧副本。最终尾activation latch也已回收到actor metric唯一state，由待执行动作提交置1、本函数成功尾清0、全局重置同步清0。

46个唯一callee中：

- `0x004539B0`已直接回收为typed动作主分派；
- `0x0045EE70`已直接回收为typed攻击顺序插入；
- `0x004698E0`的三处调用已直接回收为typed文字消息入链；
- 其余43个角色、AI、选择、文本、sample和数值callee继续使用单一typed token端口。

所有对象地址、one-based目标、固定前一槽、scene与文本地址均为`compat::u32` token，不转主机指针。

Typed-stop只位于：

- 入口组A首次对象callee；
- 计数循环每次首次组A/组B对象callee；
- queue code派生对象首次查询；
- one-based随机/选择首次对象callee；
- action target首次组B对象callee；
- resolved target零token后的word54读取；
- 已关闭动作分派自身真实访问点。

## 12. 验证与动态差分

定向测试覆盖：

- 入口组A越界；
- effect mode组合门与固定最终尾；
- AI terminal统计、one-based随机目标和两个完成标记；
- 十槽queue首个未完成项与左移；
- idle actor启动后的攻击顺序记录、队员暂存源和双尾门真实访问点；
- completed actor组B扫描与首个live选择；
- active action直接调用已关闭主分派，确认端口不再出现旧callee token并完成全cleanup；
- action target `0xFFFF`首次组B对象typed-stop；
- turn `0x4000→0x8000→0`同调用穿透；
- resolved word54最大值、stale turn参数与失败尾；
- queue code小于8的派生对象停点；
- 46个唯一callee全部存在，其中3个typed直连、43个端口边界。

## 13. `0x00478330`六处直接写入

相邻工作包278进一步关闭本函数内`0x004567EA`、`0x00456BC0`、`0x00456C1C`、`0x00456CF4`、`0x00456D88`、`0x00456E69`六处物理call。第一处把完整dword `1`写入当前组A角色`+0x2AE4`，其余五处写`0`；全部直接复用`LegacyBattleFinalActorStepState::group_a_availability_blocks`，旧callee token生产零调用。第一处保留末次组B完成查询EDX，后五处分别线程化最终处理、发布选择、选择完成或清展示callee的真实EDX；leaf在写失败时已把参数装入EAX，并保留角色token ECX与该EDX，父函数立即返回，不执行随机选择、门复位、最终处理或UI尾部。queue mode路径直连`0x00464CC0`时也通过同一owner完成其第七种组A帧到达方式。

当前缺少原版组A/B对象、43类剩余callee共享副作用、攻击顺序/队员暂存动态轨迹、AI/选择/队列表、text/sample、resolved target内存与回合状态联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。

## 14. 工作包282的回合门调用寄存器

上述端口数量保留为历史切片，不代表后续所有callee回收状态。本轮进一步修正两处`0x00471540`直连的原始输入和栈输出传播：

- `0x004573BA`：`0x00457398`读取`0x0053BF1C` DWORD，低WORD为turn，高WORD来自目标选择runtime的`transition_control_words`低WORD。入口EAX保留该完整值，EDX沿用`0x0045736D`末次terminal查询回复，ECX为actor token。缺少相邻WORD所有者时，在两个pending门写入之前typed-stop，不伪造高WORD零值。
- `0x004575C5`：入口EAX由`turn & 0x7FFF`得到，EDX为`1 << actor_index`，ECX为actor token。原参数为一，与模式零的零参数区分。
- 两处独立传递helper中`arg_0/var_4`的原始栈地址捕获。查询只覆盖WORD；正常返回ECX为覆盖低WORD后的保存栈DWORD，子typed-stop不执行正常pop，父函数阻断结算后缀。

core26/ASan17定向`battle.legacy_battle_setup`均`1/1`通过（2.92/4.76秒），覆盖两处root的真实入口寄存器组合、地址传递、正常/故障返回和相邻全局所有者缺失；无匹配编译/sanitizer诊断，diff check通过。原始栈地址未捕获时仍登记oracle缺口，本轮不是全量发布验收。
