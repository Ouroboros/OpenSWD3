# 战斗启动协调器 `0x00451B10`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`。

## 1. 完整LST范围与附件

权威LST主函数为`0x00451B10..0x004527DB`，共1351行、60个静态call站点、48个标签，无外部FUNCTION CHUNK。cdecl单参数按原规则同时保留完整32位battle ID和低16位ID。

主函数直接调用的邻接显示surface helper未在工作包单列：

- `0x00451AE0..0x00451B04`，29行，释放两个旧surface槽并逐槽清零；
- `0x00451A90..0x00451AD5`，41行，逐槽查询高/宽、创建两个surface，随后令EAX为全1并按低地址到高地址写三个`0xFFFF`完成word。

两附件随本caller typed闭合，但正式工作包只计`0x00451B10`一次。

## 2. 入口、阈值与固定reset区

入口先把参数低word写入旧battle ID高半word槽，再调用运行时准备。随后从速度设置调用已关闭`0x0044FFC0`，modern直接调用`publish_legacy_battle_action_threshold`，保留模`2^32`的`(20-speed)*100`。

接着按LST顺序清理固定状态：

- 零填充dword块：`0x26`、`0x14`、`0x3C`、`0x0A`、`0x14`、`9`、`8`、`0x32`、`0x12`项；
- 全1填充dword块：四个`0x12`项块及一个`0x7E`项块；
- 单独清零四槽、五槽、四槽、两槽及其他latch；
- 候选used八字节清零；
- 18条步长`0x1C`记录逐条写`+00=0xFFFFFFFF`、`+0A(word)=0`、`+0C=0`、`+14=0`、`+18=0`；
- 另把固定latch写全1或零。

modern以定长typed数组和字段建模全部块，不以宿主指针模拟旧地址。`LegacyBattleStartupResetRecord`现为精确`0x1C`布局；本函数只修改上述五个已证字段，`+04/+08/+10`保持入口值。`0x32`项零填充块由已关闭攻击顺序插入按十组、每组五dword直接消费并逐项清源，不建立平行暂存区。战斗调试W键和全局重置执行完整126 dword清零时则覆盖全部记录字段。角色优先索引与组B动态数量不再保留启动状态副本，本函数直接写唯一actor metric owner。测试先以非零值污染所有块，再逐块验证最终值与未写字段保留。

## 3. 控制块与队伍发现

四个控制switch先写1，再调用pending控制块初始化；调用后才写固定值`0x2329`、`0x0C`并snapshot运行时handle。

函数清零四字节presence表，但**不清零队伍总数**。它依次查询ID 30、31、32、33；仅返回严格等于1时写presence字节并递增旧总数。之后扫描四字节presence，把非零源索引顺序写入映射表。

这意味着陈旧队伍总数可大于本次presence数；modern保留入口旧总数，后续只在首次真实越界映射/对象访问处typed-stop，不提前“修复”为本次计数。

查询ID`0xC9`返回非零时只置共享flags低字节bit1；查询`0x1BB0`严格等于1时把默认延迟`0x3C`改为`0x12`。

## 4. 窗口、几何与显示surface

顺序固定为：

1. snapshot窗口矩形；
2. 对固定word对象执行参数`0x10`初始化；
3. 两组三参数查询，低word分别发布；
4. 对固定几何owner与surface源调用已关闭`0x00433DC0`；
5. 写两组逻辑尺寸`320×200`并调用输出配置；
6. 调用`0x00451AE0`释放两个旧surface；
7. 调用`0x00451A90`，每槽均按height→width→create顺序；
8. EAX snapshot为`0xFFFFFFFF`，完成word按索引`0→1→2`发布。

modern直接调用`rebuild_legacy_battle_render_surface`；其typed-stop阻断后续原本会访问无效行表的路径。显示surface由typed token保存，零token仍占一次创建调用。

## 5. `battle.ffd`加载与唯一普通早退

低word battle ID先送入准备callee。路径固定为`data_root / "battle.ffd"`；archive open收到固定对象、固定scratch与真实路径，definition load收到同一路径、固定对象、固定目标、完整32位battle ID和variant零。

加载callee返回均不作成功门；caller直接读取definition字段。enemy count和secondary count只取低16位。

若enemy count为零，函数调用固定失败文本token与低16位battle ID，然后立即返回该callee完整EAX。此前阈值、所有reset、队伍扫描、几何/surface重建和文件加载副作用全部保留；背景、角色和补位阶段均不执行。

## 6. 背景初始化直连

有敌人时先调用无偏随机`random(4)`，结果加1后只写低word背景资源号。随后把definition中的signed旋转除数、动作lowword、B4、B8、随机资源号传给已关闭`0x00451940`。

modern直接调用`initialize_legacy_battle_background`，不保留opaque entry。原caller忽略callee返回，因此普通image load失败仍继续；除零、命令流、旋转或动作缓存typed-stop则在原故障域阻断。返回后再次清零`0x26`项scratch块。

## 7. 敌方组B物化

enemy count按signed正数门进入循环，因已mask为u16，只有零跳过。每项固定：

1. 以基址`0x00525508`、步长`0x2B28`选择组B对象；
2. 调用pending对象reset；
3. 清零`0x29`个dword scratch；
4. 从definition按原stride读role、X、Y、mode；
5. 清record尾dword；
6. mirror mode严格等于1时调用actor mode并令X低word为`640-X`；
7. 配置actor；
8. mode word严格等于1时调用额外模式callee。

镜像路径在actor mode返回后只用`mov cx`覆盖下一参数低word，因此ECX高word为callee陈旧snapshot。modern端口显式携带该snapshot，并组合为`stale_high | role_low`；测试锁定非零高word。

definition与组B都只有八槽。第九项在首次actor对象访问处typed-stop，保留前八项全部副作用。

## 8. 初始队伍组A物化

先按映射源把role写入步长32的队伍记录并置active=1。随后只在队伍总数严格为1、2、3、4时写固定坐标：

- 1人：`(527,287)`；
- 2人：`(490,275)`、`(555,370)`；
- 3人：`(504,272)`、`(565,353)`、`(462,224)`；
- 4人：`(526,298)`、`(497,277)`、`(464,217)`、`(588,359)`。

其他数量不写坐标。无论人数多少，caller都读取前四槽坐标计算八个signed偏移；缺席槽因此使用入口陈旧word，modern不得清零。

随后按组A基址`0x005029D0`、步长`0x2F34`逐项：reset；mirror mode为1时调用actor mode、令X=`640-X`并令对应X偏移=`624-old`；再以源索引派生固定`0x38`、`0x60`表token和placement token配置actor。actor mode查询严格等于1时递增陈旧byte计数，按u8回绕。

两个pending全局阶段后，再为每个初始队员依次调用profile、value、palette、name四个callee，固定表步长分别为`0x40`与`0x10`。

## 9. 三组x87比率

每名初始队员执行三组查询。每组均为：

```text
ratio = low_dword(fistp_qword_trunc((signed numerator / signed denominator) * 56.0f))
```

第一组使用i32；后两组只读取callee输出word并符号扩展。`0x00489654`把x87控制字改为向零后`fistp qword`，只返回低dword。零除、NaN、无穷或qword越界产生integer-indefinite，其低dword为零。

modern以80位`long double`执行同序计算，有限域向零转i64并取低32位，非法域发布零。每个ratio都复制到两张旧表；三个原numerator和最终actor首dword也分别保存。

## 10. 候选补位与陈旧分支word

固定候选查询ID为：

```text
34, 35, 38, 44, 45, 46, 47, 49
```

对应补位role为：

```text
3, 4, 10, 33, 34, 37, 38, 40
```

第一次八项扫描只对非零查询递增`word_53BF0C`，但该word入口**不清零**。分支判断使用“陈旧入口值+本轮命中数”的u16回绕结果；两条分支随后才清零word。

- 分支值大于2：反复`random(8)`，查询失败或used字节为1则无上限重试，直到成功加入两人；
- 分支值不大于2：顺序再扫八项，加入命中者，达到两人即退出。

每个补位记录写role、`X=750`、`Y=310`、active=1；mirror mode为1时X按低word变为`640-750`。随后以组A首对象取seed，配置当前组A槽、激活；mirror mode为0时才调用actor mode。最后递增队伍总数与补位word；随机分支还写used字节。

随机重试不加modern上限，保持原非终止域。端口若违背`random(bound)`合同返回越界值，则在首次候选数组访问处typed-stop。

## 11. 最终阶段与返回

初始组A角色配置后的两个全局阶段均已回收。第一阶段对玩家道具链按u16 item id稳定升序，每次比较先清当前selected count，交换后从head重扫；第二阶段接收第一阶段EAX，依次稳定排序四个队伍道具sentinel链，不清selected count，交换后只重扫当前根。第一阶段typed-stop阻断第二阶段，第二阶段typed-stop阻断资料绑定和补位。

补位后固定调用三个pending全局阶段。每名敌人调用`random(6)`，结果为N就对该组B对象调用N次固定参数零的动作推进。之后按补位后的队伍总数，对所有组A对象调用最终化callee。

正常返回EAX按u32顺序计算：

```text
remaining = party_count
remaining -= final_subtract_word
remaining -= low16(supplemental_count_word)
```

再以unsigned比较：若陈旧party actor mode byte不小于`remaining`，把唯一共享战斗消息/阶段写`0x67`。相邻角色预处理关闭后，该dword与动作、效果和逐帧路径共用`LegacyBattleSharedPhaseStatePort`，不再保留startup副本。该写不改变EAX；无论条件真假都返回同一个回绕`remaining`。测试覆盖等于零时成立及正数域。

## 12. 双向追溯

- `0x00451B10..0x00451C53`：参数lowword、阈值、零/全1块与固定latch；
- `0x00451C55..0x00451D8A`：presence、控制块、四ID队伍扫描及映射；
- `0x00451D8A..0x00451E53`：flags、延迟、窗口、几何、逻辑尺寸和两个surface附件；
- `0x00451E53..0x00451EEC`：battle ID、`battle.ffd`及零敌人唯一普通早退；
- `0x00451EED..0x00452018`：随机背景、已关闭背景helper与组B物化；
- `0x00452018..0x0045227D`：队伍记录、1–4人坐标、陈旧槽偏移及组A配置；
- `0x0045227D..0x00452449`：已关闭玩家与四队伍道具排序、四类资料绑定与三组x87比率；
- `0x00452449..0x004526F2`：陈旧word门、随机/顺序两条补位路径；
- `0x004526F2..0x004527A5`：三个全局阶段、敌方随机动作与队伍最终化；
- `0x004527A5..0x004527DB`：两次u32减法、unsigned门、可选`0x67`与EAX返回。

C++到LST反向追溯覆盖1351行、全部48个标签、60个静态call站点、两个邻接附件、唯一普通早退、所有循环和正常尾段。

## 13. 验证与动态差分

定向合成测试覆盖：

- 所有固定reset块、18条混合宽度记录和四个控制switch；
- 完整ID与低word分离、flags/延迟查询、窗口、几何和两个surface释放/创建；
- surface完成word正序写与全1EAX snapshot；
- archive真实路径、固定owner/scratch/目标、variant零及零敌人callee EAX早退；
- 背景image load零返回仍继续；
- 两名敌人、镜像低word及陈旧ECX高word；
- 1–4人全部固定坐标和缺席槽不改写；
- 三组比率、负比率与零除integer-indefinite低dword零；
- 玩家与四队伍道具升序、差异化selected count、陈旧EAX和双阶段排序停点；
- 顺序补位、陈旧word触发随机补位、重复随机候选重试；
- 敌人随机动作次数、补位后组A最终化、u32尾减法和`0x67`门；
- 第九名敌人在前八名副作用后typed-stop；
- battle聚合目标零warning，普通定向与独立ASan定向均`1/1`通过。

`battle.ffd`具体open/load callee属于后续`audit_order=107/108`，角色、AI和其余全局阶段callee也各有后续工作包；当前只以typed端口关闭本caller顺序和数据流。原版文件对象、全部共享表、18个角色对象、窗口surface、随机状态与后续状态联合捕获后端缺失，`original_diff_verified`为`blocked_runtime_oracle`。
