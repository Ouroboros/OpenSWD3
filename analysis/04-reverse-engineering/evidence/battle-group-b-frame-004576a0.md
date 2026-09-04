# 组B战斗帧主循环 `0x004576A0`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`。

## 1. 完整LST范围

权威函数为`0x004576A0..0x004582AB`，完整1356行、86个静态call站点、66个`loc_`标签，无外部FUNCTION CHUNK。唯一caller为尚未关闭的`0x0045B5E0`。

入口接收组B索引。对象token按低32位建立：

```text
0x00525508 + index * 0x2B28
```

索引只在首个实际组B对象callee处typed-stop。函数普通尾返回最终actor-step的完整EAX；两个入口完成早退返回reset-actor的完整EAX，不执行画面效果和最终尾。

## 2. 入口活动与完成早退

frame enable完整值不等于1时跳过主体，仍执行公共画面效果、pending effect和最终actor尾。

主体入口对当前组B对象执行terminal查询。仅terminal为0且两个pending dword都为0时调用update；第二pending dword现与组A帧、调试快捷键和结果判定latch共用唯一`LegacyBattleOutcomeResolutionStatePort`。当前对象post-update门为0时，直接复用startup enemy中的进度与共享八槽动态资源唯一owner调用已关闭`0x004755E0`；参数按原32位位形传入，EDX继承update callee。进度返回EAX精确为1且message state不等于103时，直接组合已关闭攻击顺序登记，以固定类型2把当前索引写入共享18条记录的首个全1槽。资源typed-stop保留update前缀并阻断余下角色帧；旧进度函数和攻击顺序callback token均删除。

随后仅检查action auxiliary dword和turn-resolution **低word**。两者均为0且active effect target等于当前组B索引时：

- queue completion完整EAX等于1：清selection/action/pending状态，active target写全1，queued actor code写`index+1`，调用reset actor并直接返回完整EAX；
- 否则terminal完整EAX等于1：执行相同清理但不写queued actor code，再直接返回reset actor完整EAX。

相邻actor-start guard不在本函数该门中读取，不能与turn lowword合并。

## 3. phase mode

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
- excluded查询不等于1；
- target busy为0；
- 当前组B对象idle为0。

进入正数循环前，原EBX被写1；`group_a_count<=0`则保留入口的`stride*index`陈旧EBX。

阈值先按u32回绕计算：

```text
scanned - defeated highword - excluded lowword - packed actor lowbyte
```

最终比较使用signed `jl`，因此progress和阈值都按i32解释。达到阈值后从组A索引0向后找首个非terminal，设置target-ready、prepare selection并发布索引；无论是否找到都把action pending写1。

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

status-action返回非0时，对当前组B对象依次发布selection 0、mode 17和status mode 2，然后进入公共动作阶段。

## 6. signed/bit status分派

status按i16为负或bit`0x6000`任一置位时进入特殊分支：

- special-selection pending等于1时，side写1并清pending；
- signed负值直接组合已关闭`0x004761D0`，按selector/profile/resource恢复返回值与mode；结果写status action value，并写current actor低word；
- bit`0x4000`发布mode 2、写current actor；对象text byte非0时显示固定文本；
- bit`0x2000`发布mode 6并再次写current actor；
- special-action查询等于1时置latch；
- phase-mode查询等于1时置phase并清progress；
- 固定清branch misc。

side为1时把随机索引解释为组B；side为0时解释为组A。两条路径若当前目标terminal，则递增共享索引：组B路径允许推进到8并在首次one-past对象查询typed-stop；组A路径以`>8`退出递增，可在公共查询访问有效索引9。最终非terminal时发布索引，组A路径另prepare target。

普通status路径先查询固定status-sequence token；成功后写current actor、清status misc、查询special action并可显示文本。随后直接组合已关闭`0x00476140`，读取组B profile `+0x08` dword bit28；未置位时再返回profile `+0x04` dword bit12。结果等于1时side写1且随机索引改为当前组B。actor typed-stop保留status-sequence或文字路径后的陈旧EAX/EDX及此前副作用，并阻断side、目标、第二次current actor、phase查询和后缀。最后查询phase mode；未进入phase时按side查询组B或组A目标，非terminal则发布索引，组A另prepare target。

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

随后清selection initialized、action block、active-effect gate和pending。active target按unsigned小于8时清连续28字节active-effect块、再写target全1并清active target code；否则不清该块。之后清双stage word、target-ready、special latch、phase mode/progress、action runtime低word、当前组Bstatus word和post counter。primary suppression等于1时置fade；global countdown低15位非0时只递减低word。

## 10. completion surface

清理后把完成target固定解释为组A对象并查询completion effect。完整EAX等于1时按当前组B source为ECX依次发布固定ID、source、resource、mode；prepare surface完整EAX等于1后，先执行：

```text
mapping[group_b_index] = group_b_index
completion selected = 0xFFFFFFFF
completion gate = 1
```

再读取目标surface token并按低32位`right*bottom*2`计算字节数，以`rep stosd + rep stosb`写全`0xFF`。typed实现使用u16 span得到同一全1位形：零token在首字节前停止；owned span不足时写满前缀后在首个越界字节停止；三项状态副作用均已发生。

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

46个唯一callee中，已关闭对手动作分派`0x00455D60`、玩家道具双数量步进`0x0045D180`、攻击顺序登记`0x0045EDF0`、文字消息入链`0x004698E0`、组B行动进度`0x004755E0`、组B对手模式`0x00476080`、组B行动资料标记`0x00476140`、组B行动profile/mode组合`0x004761D0`和角色进度阈值同步`0x00478370`直接typed组合；文字消息的两处调用复用启动状态唯一链头和动态节点owner，行动进度、三个组B判定/组合与阈值同步复用startup的actor/resource/profile/timing owner。`0x004761D0`内部待审profile loader替代原整函数token进入窄端口，其余37个角色、AI、状态、文本、完成资源和效果callee继续使用共享typed token端口。

定向测试覆盖：

- 入口组B越界；
- frame disabled仍执行公共尾及完整EAX；
- live update、组B进度与攻击顺序共享记录直连、两个旧callback清零、资源typed-stop前缀、记录typed-stop前缀与陈旧stride EBX写block；
- queue完成直接返回reset完整EAX；
- phase side跳过随机/status；
- 随机组B同伴；
- profile覆盖EDX低byte；
- 组B对手模式直接消费随机、发布自身目标及资源typed-stop前缀；
- signed status直接组合profile/mode、旧整函数零调用、actor/loader typed-stop前缀、双mode、文本、phase与目标；
- 普通status直接读取组B profile双bit、陈旧EAX/EDX与actor typed-stop前缀；
- 对手动作分派直连、未完成返回陈旧EBX写入及完整cleanup；
- 两处completion阈值同步直接写startup组A进度、旧`0x00478370`零调用、全目标分支保留reset EAX高word、单目标分支保留角色ECX高word，以及读取/写入停点的表/道具/目标reset后缀阻断；
- completion surface零token首字节停点、状态前缀与越界停点；
- pending effect及final actor成功尾；
- profile真实访问typed-stop。

当前缺少原版组A/B对象、38类剩余callee（含profile loader）共享副作用、攻击顺序动态记录、随机状态、AI/packed-status表、completion表、文本、资源surface及陈旧寄存器联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
