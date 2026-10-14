# 组B战斗帧主循环 `0x004576A0`

状态：`pending`。

B11的45820D/458215现借实际BF94/BF98双门，BF30灰度分带独立。
收尾4580CA只检查secondary精确1才触发淡出，primary为1不替代。
单效果适配器的const/非const getter转发实际画面控制状态，避免临时副本。
四种门值收尾回归通过core/ASan actor_frame_316各1/1，SDL链接通过。
全部访问与日志见[双门与实时颜色](battle-frame-effect-00453580.md#20-b11双抑制门与当前颜色共用实际存储)。

## 实际叠加标记与启动计数

00457864直接清被扫描队员的实际叠加绘制DWORD，不修改当前敌人的idle。
对应通用调用和编号已删除，后续失败不再使用端口伪造的EAX。
接线及当前验证状态见[实际叠加标记](battle-group-b-final-overlay-clear.md)。

0045786B在清叠加标记后直接更新队员共享字段，故障保留早先扫描提交，
抑制当前进度递增。00457E1C在目标发布后重读完整选择值，解析实际
回绕地址；两个分支合流执行一次，不再回传寄存器或维护调用轨迹。
范围及本轮验证状态见[启动计数递增](battle-actor-start-gate-increment-00478ac0.md)。
其他组B协议及完整资源路径仍未验收。

## B11：组B借实际优先角色

45774B读取与两条完成早退的全1写回都借同一metrics DWORD。
457E51的比较快照保留到后续EAX/flags；458085在收尾回调后重新读取，
4580A5按signed JGE决定是否清全部七DWORD，再发布FFFFFFFF。
高位DWORD、回调重读与六DWORD尾的向量及门禁状态见
[实际角色与优先角色](battle-frame-effect-00453580.md#21-b11画面效果借实际角色word及优先角色dword)。

## 目标选择标记的直接读取

00457EC3在目标读取和清理之后，直接读取当前敌方的实际共享DWORD。
仅精确1遍历队员衰减，其余值只处理选择对象。查询请求、轨迹、计数、
offset及转发执行器已删除。原字段/返回故障仍保留前缀并阻断后缀。
00457ED1按内存比较数量，不覆盖已读值；首次衰减保留前驱EDX。
core/ASan setup与角色帧各1/1、SDL链接均通过；范围、独立向量及门禁见
[目标选择标记](battle-actor-target-selection-latch-query-00478b40.md)。
整条组B及动作父链的其他通用协议继续待迁移，未升级整体验收。

## 实际敌人属性与群体效果共享

43c7652e将457B57/457CA9直接接到保存敌人的实际profile BYTEbit1。
真写效果协调器同一标记、假保留，4580EA完成清零，删除旧帧影子及叶协议。
两次状态读取按原指令实时重读；450次新帧、432次真实效果消费、
两套core/ASan定向门及SDL链接通过，详见
[敌人帧群体效果属性](battle-enemy-frame-coordinate-attribute-0047d880.md)。
完整SDL帧和其他协议、共享别名、资源及全部父出口仍pending。

## 实际敌人属性与效果目标数量共享

c6fff61c将457BAA/457CBF直接接到保存敌人的profile BYTEbit0。
真置目标扫描门并清实际效果目标数量；普通真分支跳选择、标记真分支继续选择。
删除phase_progress重复状态，父的六处计数访问借协调器同一目标数量，
保持回绕、signed比较、成功清零及真实失败保留。
450次新帧、432次实际计数消费、450次bit1同步、两core/两ASan与SDL链接通过，见
[敌人帧效果目标数量](battle-enemy-frame-target-count-0047d8d0.md)。
reset字节像及profile重叠别名、其它父协议和完整运行仍pending。

## 1. 完整LST范围

权威函数为`0x004576A0..0x004582AB`，本轮从proc到endp重抽校正为1355行，
86个静态call站点、66个`loc_`标签，无外部FUNCTION CHUNK。
唯一caller为尚未关闭的`0x0045B5E0`。

入口接收组B索引。对象token按低32位建立：

```text
0x00525508 + index * 0x2B28
```

索引只在首个实际组B对象callee处typed-stop。函数普通尾返回最终actor-step的完整EAX；两个入口完成早退返回reset-actor的完整EAX，不执行画面效果和最终尾。

## 2. 入口活动与完成早退

frame enable完整值不等于1时跳过主体，仍执行公共画面效果、pending effect和最终actor尾。

主体入口对当前组B对象执行terminal查询。仅terminal为0且两个pending dword都为0时调用update；第二pending dword是BFC4，现与组A帧、调试快捷键共用动作状态的`selection_cache_gate_b`；结果latch是另一个地址BF5C，由`resolution_latch`独立保存。此前合并两者的结论错误，本轮修正及未完成验证见[状态区分证据](battle-selection-wait-and-result-latch-0053bfc4-0053bf5c.md)。当前对象post-update门为0时，直接复用startup enemy中的进度与共享八槽动态资源唯一owner调用已关闭`0x004755E0`；参数按原32位位形传入，EDX继承update callee。进度返回EAX精确为1且message state不等于103时，直接组合已关闭攻击顺序登记，以固定类型2把当前索引写入共享18条记录的首个全1槽。资源typed-stop保留update前缀并阻断余下角色帧；旧进度函数和攻击顺序callback token均删除。

随后仅检查action auxiliary dword和turn-resolution **低word**。两者均为0且实际优先角色DWORD等于当前组B索引时：

- queue completion完整EAX等于1：清selection/action/pending状态，优先角色DWORD写全1，queued actor code写`index+1`，调用reset actor并直接返回完整EAX；
- 否则terminal完整EAX等于1：执行相同清理但不写queued actor code，再直接返回reset actor完整EAX。

相邻actor-start guard不在本函数该门中读取，不能与turn lowword合并。

## 3. phase mode

下文progress为原53BD7C，现全借效果协调器completion_target_count，
没有独立父字段或复制层；计数消费与全局reset共用此owner。

phase mode等于1时不执行后续随机初始化和packed status分派，而是直接进入公共动作阶段。

### 3.1 side非0

设置target-ready，向当前组B对象发布selection 0，并写：

```text
phase progress = u32(group_b_count) - processed low byte
```

### 3.2 side为0

遍历`group_a_count`个组A对象。只有以下条件全部成立才clear control、prepare target并累加progress：

- 非terminal；
- 固定actor AI dword不等于1；
- blocked查询不等于1；
- 直接读取当前队员共享启动标记，完整DWORD不等于1；
- `0x0045784D`直接借当前目标队员的`group_a_action_execution[]`完成DWORD，且完整值为0；
- 当前组B对象idle为0。

`00457842`直接借队员既有`start_gate_latch`，前驱blocked修改后才读取。
只有完整值1跳过候选；高位非一值继续。叶查询删除寄存器request/reply、
CALL轨迹、参数数组及计数；父层可选实际结果保留读取/返回故障的不同前缀。
下一个回合查询不再运输EDX/flags；字段失败时父级保留实际完整启动标记。
两种原查询故障均阻断回合、idle、control清理、启动门递增和候选后缀。
见[实际启动标记](battle-actor-start-gate-latch-query-00478b50.md)。

回合getter停止发生在后续TEST前，保留terminal、blocked与启动标记读取前缀，
阻断source idle、clear control、prepare target、progress累加和剩余循环。
生产路径不再通过generic端口调用`0x00478690`。

进入正数循环前，原EBX被写1；`group_a_count<=0`则保留入口的`stride*index`陈旧EBX。

阈值先按u32回绕计算：

```text
scanned - defeated highword - excluded lowword - packed actor lowbyte
```

最终比较使用signed `jl`，因此progress和阈值都按i32解释。
达到阈值后从组A索引0向后找首个非terminal。命中时先发布target-ready，
004578FB直接把当前敌方共享目标标记写为1，正常后才选择目标及写pending。
原字段写/返回故障保留ready前缀，阻断选择与pending；后续选择沿用
实际terminal EAX/EDX及CMP结果。正常搜索结束，无论是否找到都写pending。
00457754仍要求优先角色等于当前敌方索引；非零索引测试保留该前置条件。
见[共享目标标记置位](battle-actor-target-selection-latch-set-00478b30.md)。
双方帧及动作其他协议仍待迁移。

## 4. 随机选择初始化

phase mode为0且selection initialized为0时查询selection mode。

- selection mode非0，且`group_b_count-processed低byte==1`：在组A计数域随机，跳过两个AI dword任一等于1或terminal对象；
- selection mode非0，且剩余不等于1：在组B计数域随机，跳过terminal和当前组B对象，成功后side写1；
- selection mode为0：在组A计数域随机并跳过两个AI dword和terminal；随后直接组合已关闭`0x00476080`，按组B profile/resource与bound 10 secondary RNG判定，完整EAX等于1时side写1、随机索引改为当前组B索引。

三类随机重试都无modern上限。随机callee返回超约定索引时，只在首次AI数组或对象terminal访问点typed-stop。`0x00476080`的actor/resource typed-stop发生在随机消费后，保留此前组A目标选择并阻断side、自身索引与初始化尾。初始化正常尾固定把selection initialized写1。

## 5. packed status与陈旧EDX

当前组B对象idle为0时读取当前组B的packed status word，并先清phase mode。status高byte非0时，随机target索引只取status低byte。

随后以`14*profile_index`访问profile byte。机器码只执行：

```text
mov dl, profile_byte
```

因此profile参数低byte来自表，高24位来自显式陈旧EDX snapshot。profile索引越界在首个byte访问点typed-stop，并保留此前phase清零和target索引副作用。

status-action返回非0时，对当前组B对象依次发布selection 0、mode 17，
再直接写实际绘制步长2，然后进入公共动作阶段。步长操作的编号和通用调用已删除。
同帧后继行动17完成时会再写步长8；原前继失败不写步长。
字段借用、顺序和定向验证见[敌方绘制步长](battle-enemy-presentation-step-0047d860.md)。

## 6. signed/bit status分派

status按i16为负或bit`0x6000`任一置位时进入特殊分支：

- special-selection pending等于1时，side写1并清pending；
- signed负值直接组合已关闭`0x004761D0`，按selector/profile/resource恢复返回值与mode；结果写status action value，并写current actor低word；
- 实时bit`0x4000`发布显示mode 2、清动作值并写current actor；对象text byte非0时显示固定文本；
- 文字后重读实时bit`0x2000`发布显示mode 6、清动作值并再次写current actor；
- 直接读保存敌人profile BYTEbit1，真置效果协调器共享DWORD1，假保持旧值；
- 直接读保存敌人profile BYTEbit0，真置phase1并清共享效果目标计数，假保持；
- 固定清branch misc。

side为1时把随机索引解释为组B；side为0时解释为组A。两条路径若当前目标terminal，则递增共享索引：组B路径允许推进到8并在首次one-past对象查询typed-stop；组A路径以`>8`退出递增，可在公共查询访问有效索引9。最终非terminal时发布索引，组A路径另prepare target。

普通status路径先查询固定status-sequence token；完整返回1后写current actor、清status misc，读保存敌人实际profile BYTEbit1，真写共享效果标记1、假保留，再可显示文本。纯属性叶不产生EDX回复；无文字时保留readiness前继EDX。随后直接组合已关闭`0x00476140`，读取组B profile `+0x08` dword bit28；未置位时再返回profile `+0x04` dword bit12。结果等于1时side写1且随机索引改为当前组B。actor typed-stop保留status-sequence或文字路径后的陈旧EAX/EDX及此前副作用，并阻断side、目标、第二次current actor、bit0读取和后缀。最后直接读保存敌人profile BYTEbit0，真置phase1并清共享效果目标计数且跳选择；假保持，只有live phase完整零时按side查询组B或组A目标，非terminal则发布索引，组A另prepare target。

## 7. 公共动作阶段与陈旧EBX

到达公共动作阶段后查询当前组B对象idle：

- 完整EAX等于1：原EBX固定写1；active target等于当前索引时另置action block与pending，并调用action-start；随后写current actor、查询target低word并直接调用已关闭`dispatch_legacy_battle_opponent_action`；
- 完整EAX不等于1：action block写当前陈旧EBX。

陈旧EBX不是恒1：入口先计算`group_b_index*0x2B28`，只有正数phase组A扫描和动作调用前改为1。因此未经历这两条路径的idle失败会把stride offset写入action block。反编译把该值合理化为1，权威LST明确为`mov dword_53BF68, ebx`。

对手动作分派完整返回不等于1时同样把当时已固定为1的EBX写action block。typed-stop不执行该写入，因为原程序会在真实故障点终止。

## 8. 动作完成与组A完成值

已关闭对手动作分派返回1后：

1. 再次查询target低word；
2. selection clear；
3. selection complete等于1时遍历全部组A，先reset target；queue completion等于1且对应completion word非0时，清word、defeated低word和间隔五dword槽，再依次reset completion slot、reset actor、直接把共享行动阈值低word同步到startup组A角色进度，并把高word保留、低word覆盖后的完整item参数直连selector-zero玩家道具数量步进；
4. selection complete不等于1且side非0，只按组Btarget reset；
5. side为0时按组Atarget执行单槽完成逻辑，使用同一阈值同步和selector-zero玩家道具数量直连，再reset target；
6. reset当前组B source。

全组A分支中`0x00478370`入口EAX/EDX继承前一个reset actor返回；函数只替换AX，固定表word随后再次覆盖AX，所以完整item参数保留reset返回EAX高word。单组A分支中同步函数恢复角色ECX；caller随后完整覆盖EAX/EDX但只覆盖CX，所以item参数保留角色token的ECX高word。阈值读取或角色进度写入typed-stop保留完成状态清理与reset前缀，阻断表读取、玩家道具步进及对应后缀。固定表索引为：

```text
16 * u32(group_a_count - defeated highword)
```

首次表word访问越界typed-stop，保留此前reset与查询副作用。

## 9. battle bit与公共清理

battle byte低byte bit`0x80`置位时遍历全部组A。固定AI dword为0且terminal为0时，以该组A对象为ECX调用battle-bit边界并传0。循环后只清battle byte最低byte的bit`0x80`，保留高24位。

随后清selection initialized、action block、active-effect gate和pending。active target按unsigned小于8时清连续28字节active-effect块、再写target全1并清active target code；否则不清该块。之后清双stage word、target-ready、效果协调器同一群体效果标记、phase mode及共享效果目标计数、action runtime低word、当前组Bstatus word和post counter。primary suppression等于1时置fade；global countdown低15位非0时只递减低word。

## 10. completion surface

清理后把完成target固定解释为组A对象并查询completion effect。完整EAX等于1时按当前组B source为ECX依次发布固定ID、source、resource、mode；prepare surface完整EAX等于1后，先执行：

```text
mapping[group_b_index] = group_b_index
completion selected = 0xFFFFFFFF
completion gate = 1
```

随后按共享入战矩形的低32位`right*bottom*2`计算字节数，原代码向
`4CD76C`画布执行`rep stosd + rep stosb`写全FF。
当前typed实现直接借用context的实际画布，零字节不写，owned范围不足则
保留已写前缀后停止；上述三项状态副作用均已发生。
旧独立矩形与surface token/span已删除。宿主画布构造保证有效存储，
不再用独立零token模拟实际指针；原版非法地址异常未取得动态差分。
平台边界和验证见[入战矩形接线](battle-window-rectangle-runtime-binding.md)。

## 11. 公共最终尾

公共尾组合effect query、两个suppression dword和global override，向当前组B对象发布mode 0或1。

pending effect ID非全1时调用pending step `(source,shared_argument,index)`；完整EAX等于1才清该ID。

最后读取共享mapping数组当前索引并调用`final actor step(mapped,0)`，返回完整EAX。结果等于1时：

- mapping写全1；
- 当前final actor state清零；
- 当前final target写全1；
- queued selection word写`0xFFFF`；
- overlay gate写1；
- final gate清零。

## 12. callee、测试与动态差分

46个唯一callee中，已关闭对手动作分派`0x00455D60`、玩家道具双数量步进`0x0045D180`、攻击顺序登记`0x0045EDF0`、文字消息入链`0x004698E0`、组B行动进度`0x004755E0`、组B对手模式`0x00476080`、组B行动资料标记`0x00476140`、组B行动profile/mode组合`0x004761D0`、角色进度阈值同步`0x00478370`、回合完成查询`0x00478690`、空闲状态查询`0x004786A0`和启动门word查询`0x004786D0`直接typed组合；文字消息的两处调用复用启动状态唯一链头和动态节点owner，行动进度、三个组B判定/组合与阈值同步复用startup的actor/resource/profile/timing owner，回合完成查询复用共享Group-A action owner，三处空闲状态与公共效果模式启动门查询复用startup Group-B lifecycle action-execution owner。`0x004761D0`内部待审profile loader替代原整函数token进入窄端口，其余待审角色、AI、状态、文本、完成资源和效果callee继续使用共享typed token端口。

定向测试覆盖：

- 入口组B越界；
- frame disabled仍执行公共尾及完整EAX；
- live update、组B进度与攻击顺序共享记录直连、两个旧callback清零、资源typed-stop前缀、记录typed-stop前缀与陈旧stride EBX写block；
- queue完成直接返回reset完整EAX；
- 队员扫描直接借实际目标完成DWORD，删除查询解析、寄存器/栈、计数与转发；字段失败保留完整启动标记前驱，RET失败保留完成观察值，抑制来源idle、目标准备及剩余扫描，见[回合完成查询](battle-actor-turn-completion-00478690.md)；
- 三处空闲状态查询直接借敌方实际DWORD，移除查询协议、token解析、计数与转发；保留完整值、每次重读、字段/RET失败及已执行前缀。父级保留仍被消费的入口值，其余协议与资源继续待迁移；当前证据见[空闲状态查询](battle-actor-idle-state-004786a0.md)；
- 公共尾在前驱执行后直接读取Group-B实际启动计数，完整WORD零/非零与门值精确1组合、读取/返回失败保留前缀并抑制显示及最终尾；查询不再携带寄存器或调用计数，当前证据见[启动计数查询](battle-actor-start-gate-004786d0.md)；
- phase side跳过随机/status；
- 随机组B同伴；
- profile覆盖EDX低byte；
- 组B对手模式直接消费随机、发布自身目标及资源typed-stop前缀；
- signed status直接组合profile/mode、旧整函数零调用、actor/loader typed-stop前缀、双mode、文本、phase与目标；
- 普通status直接读取组B profile双bit、陈旧EAX/EDX与actor typed-stop前缀；
- 对手动作分派直连、未完成返回陈旧EBX写入及完整cleanup；
- 两处completion阈值同步直接写startup组A进度、旧`0x00478370`零调用，以及读取/写入停点的表/道具/目标reset后缀阻断；modern直接传完成表中的16位道具编号，不再传递寄存器高word。道具匹配与MON索引均截断为16位，详见[阈值同步证据](battle-actor-progress-threshold-sync-00478370.md)；
- completion surface实际画布的状态前缀、短缓冲停止和字节回绕零；
- pending effect及final actor成功尾；
- profile真实访问typed-stop。

当前缺少原版组A/B对象、其余待审callee（含profile loader）共享副作用、攻击顺序动态记录、随机状态、AI/packed-status表、completion表、文本、资源surface及空闲状态/陈旧寄存器联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。

## 13. 动作目标word查询两处caller直连

工作包296关闭`0x00457E8A`和`0x00457EAE`两处物理call，真实返回地址分别为`0x00457E8F`和`0x00457EB3`。第一处active actor分支继承动作启动callee的EAX/EDX，inactive actor分支继承此前寄存器并保留`index*24-index`的SUB flags；返回后才符号扩展AX并调用对手动作分派。第二处继承嵌套对手动作分派的EAX/EDX及其与成功值1比较的flags，返回后才保存目标并进入清动作和完成后缀。

两处直接读取startup Group-B lifecycle action-execution的canonical动作目标，只增加typed调用计数。待审`0x00478A70`的选择发布和`0x00478B20`的动作清除均同步当前lifecycle actor字段。字段或RET停止保留动作启动或嵌套分派前缀，抑制post-call符号扩展、选择清理、完成扫描和公共尾；生产`0x004786E0` raw调用为零。

当前缺少原版完整Group-A/Group-B actor、对手动作分派剩余callee共享副作用，以及两处caller联合寄存器、flags和SEH捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
