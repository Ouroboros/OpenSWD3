# 战斗脚本与动作控制状态的同址访问

状态：工作包282内修正中；不是整个脚本、动作调度或调试入口的关闭证明。

## 1. 两个独立地址域

LST把原先`LegacyBattleScriptSharedState::actor_state_words`混用的访问分为两个地址域：

- 脚本10的`0x0046AFC9`写`0x00502984 + actor*4`；脚本12的`0x0046B450..0x0046B460`从同一地址逐DWORD比较`0xFFFFFFFF`。实际owner是`LegacyBattleActorPublicationState::slots`，不是动作控制区。脚本port虚继承既有publication state port，两个入口直接借用其slots。
- 脚本23的`0x0046AD59`及脚本58的`0x0046D238`写`0x0053AF38 + raw_actor_code*4`。既有动作控制区`LegacyBattleActionDispatchState::opponent_workspace`以`0x0053AF30`为基址，因此对应其`raw_actor_code+2`项；组A编码8..17对应10..19项。脚本新增`actor_control_words`借用span，不新建数组。先做32位地址运算，再解析借用范围；缺失时在真实store停下。

已删除混用的`actor_state_words`。不根据相同C++字段名推定物理同址。

## 2. 调试X开关是同一个动作控制DWORD

`0x0045D98A/0x0045D996`在Sleep之后读写`0x0053AF68`，执行`old==0 ? 1 : 0`。该地址是动作控制区第14项，也是脚本23组A编码12的状态写入位置。

已删除`LegacyBattleDebugHotkeyState::toggle_53af68`副本。调试快捷键通过已有`bindings.action`读写`opponent_workspace[14]`；调试覆盖层也读取同一项，不同步复制。`committed_actor_code`仍由原debug state port持有，与该控制DWORD不同。

调试C键的`0x0045DBB2`清零`0x0053AE70`起七个DWORD；其后六项是`0x0053AE74..0x0053AE88`。这与`0x0053AF68`不相交，不应把`selection_workspace_tail`合入本次控制区。其与其他同址工作区的关系仍需单独审计。

## 3. 脚本58紧邻写入修正

组A路径`0x0046D1B9..0x0046D252`：

1. 有符号比较actor WORD与7；先复制旧queued到`workspace.coordinate_x`，再发布committed actor，随后才读target WORD。
2. target符号扩展后加1；结果大于7时，EDX为`(actor-8)*5`，写startup的真实五DWORD记录首项。否则EDX保持入口值。
3. `0x0046D221`直接调用可用状态写入函数，保留其返回寄存器。
4. 从实际committed owner重读actor；`0x0053BCE8`写1，再写借用控制区。不能把这一步替换为`selected_action_kind=6`，也不能把queued改为新actor。
5. 只有store正常完成后才按当前cursor加4并恢复保存ECX。target截断及控制区缺失分别保留已经到达的不同前缀。

组B及负数actor路径`0x0046D253..0x0046D2C0`：把`workspace.coordinate_y`复制到`workspace.value_a`；EDX装入非零固定函数地址`0x0046E0A0`，随后的条件跳转必定跳过两个死块。不读target WORD，不调用getter或插入动作。保留这一原始固定地址判断，不修复为函数调用。

### 脚本9的组A路径

`0x0046AA05`用actor WORD替换跳转表索引的AX；target截断时EAX因此是actor编码，而脚本58仍是跳转表索引59。随后`0x0046AA0F..0x0046AA7D`依次复制queued到coordinate_x、发布committed、读target并按符号扩展加1、按五DWORD步长写真实runtime记录、写actor可用状态、重读committed、写action_state与控制槽。已修正旧实现对queued及group_a_slot_values的误写。

`0x0046AA84`跳到`0x0046ABE6`，跳过组B目标WORD及攻击队列插入；旧C++把这些组B后缀无条件应用于组A，现已移入组B分支。帧调用前gate清零。帧故障保留callee寄存器和callee已改写cursor，不能恢复gate或pop ECX。正常返回才重读当前cursor加6、gate置1、恢复入口ECX。

新增加54组组A输入（槽0/4/9、target 6/7/0xFFFF、正常/actor截断/target截断/可用状态不可写/缺失控制区/帧故障）。覆盖目标及攻击队列完全不变、两个旧投影不写、真实五DWORD槽、各故障寄存器及帧把cursor改为12后正常返回18。core38/ASan25定向均1/1通过（3.35/5.20秒），四份stdout/stderr日志无匹配编译/sanitizer诊断，diff check通过。

组B路径的完整目标扫描、随机选择、callee改写与寄存器仍需继续收敛；本次组A测试不能证明组B完整等价。

### 脚本78的初始化与等待

已按`0x0046DBB6..0x0046DD9E`修正初始化与等待路径：

- 初次进入先写packed_value_a高WORD，再读target。actor按WORD零扩展；target按WORD符号扩展加1。每个callee之前重新读取原指令要求的packed actor和published target；不把首次求得的token跨callee缓存。
- `0x0046DC82`在设置目标的callee之前清queued。可选准备仅由查询结果恰好等于1触发。每个callee的typed-stop立即阻断未到达后缀。
- `0x0046DCC9..0x0046DCF2`先捕获最新actor高WORD到EDX，清EAX，action_state写6，清final_actor.actor_order十DWORD，再清startup.reset.records_524788全部126DWORD，最后写control槽6。两个独立shared数组已删除。记录类型默认首项是FFFFFFFF，故清零阶段显式改为0；不能提前使用默认哨兵。
- 缺失控制视图时，两个数组已全零、EAX/ECX为0、EDX为actor编码。正常store后才对18条记录首DWORD写FFFFFFFF。
- `0x0053BF74`由final_actor.selection_gate持有。此归属与`0x004532B0/0x004532ED`的frame coordinator消费、`0x0045DBAC`调试C清理吻合。`0x0053BF68`使用既有SelectionFrameStatePort.secondary_actor_gate；不再误清`script_aux_gate`（53C014）。也不再把action_state=6错误投影为selected_action_kind。
- `0x0046DD4A`没有栈参数；对象地址是`0x005229E0 + published_target*0x2B28`，EAX为target*0x159，EDX保留捕获的actor。已移除固定对象地址和多余栈参数。
- 初始化完成写word_a=1。等待期间只调用frame，不重复初始化。frame typed-stop不清WORD、不递增cursor、不恢复保存ECX。正常等待返回及完成返回恢复入口ECX；完成路径按frame改写后的cursor加6，清word_a和packed_value_a高WORD，保留低WORD。

36组新向量覆盖三个初始槽、查询返回0/1/2、正常/缺control/初始化callee故障/frame故障。callee依次将actor改为9、10、11、17、12，并改写published target，用来区分首次token与各次真实重读。core39/ASan26定向均1/1通过（3.51/5.29秒），日志无匹配编译/sanitizer诊断，两个被删除的shared数组在src/include/tests中零引用。

额外发现：frame coordinator把`0x0053AE70`输出借给ActorMetricState.priority_actor_index，final actor状态则另有标注同地址的active_actor_code。此同址重复尚未合并，不能把当前脚本78路径视为整个运行态所有权已闭合。

## 4. 测试及当前验证等级

脚本23正常矩阵扩展到四个组A槽（0、1、4、9），共72组；原有动态committed改写/地址回绕及七组故障输入继续保留。夹具现在由一个真正的`LegacyBattleActionDispatchState`同时提供组A执行状态和控制区，不另建第二套执行数组。

新增跨入口序列：脚本23将控制第14项写2，Ctrl+X读到该值并写0，再次Ctrl+X写1；相邻控制项保持不变。缺失控制span在最终store停止，保留profile及三个actor查询前缀。独立publication槽仍保持哨兵；脚本10发布后，脚本12才在同一槽观察到非哨兵并等待。

脚本58新增27组组A输入（槽0/4/9，target 6/7/0xFFFF，正常/target截断/缺失控制区），以及0/7/0x8000/0xFFFF四种非组A编码，后者只提供四字节脚本窗口以证明不读取target。

core36/ASan23定向均1/1通过（2.55/4.33秒），但均报告测试计数从size_t到u32的窄化警告，不能记为零诊断。已把计数改为size_t，并补正复核发现的组B工作区复制；core37/ASan24定向均1/1通过（3.25/5.10秒），四份stdout/stderr日志无匹配编译/sanitizer诊断，diff check通过，暂存区为空。src/include/tests中两个被删除的旧字段均零引用。没有运行新的全量门、十轮复跑或独立审查。

## 5. 未完成项

- SDL脚本入口仍未借用实际组A执行状态和动作控制区，初始化仍是有限建场切片；不能凭新增字段默认空span宣称平台接线完成。
- 脚本78已修正上述真实数组及控制store，并通过本轮定向验证。`0x0053AE70`同址投影仍需合并。脚本9组B与脚本10/12仍有未收敛行为，不能据本次修正宣称整个入口完成。
- `opponent_workspace`还被其他路径用作记录清理投影。已看到对手调度中`0x004562BA..0x004562BF`实际清理`0x00524788`，C++却清`opponent_workspace`；需改为真实startup记录owner，并核对组A帧的相似路径。不能因当前借用消除了一个副本就宣称整个控制区所有权闭合。
- 原版callee、共享状态和寄存器联合捕获仍缺失；本次固定状态UT不是原版动态差分证据。
