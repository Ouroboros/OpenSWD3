# 战斗菜单应用行动配置后按队员属性限制目标选择

状态：`pending`。已废弃GPT重建实现，本轮逆向尚未开始。

以下保留历史分析与证据；旧结论、完成记录和测试结果不作为本轮验收。
原版采集事实保留，所有解释和实现结论须重新核实。

## 历史资料（重新核实前不生效）


历史状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`。
实现源码：`371fffc0c47bc89052c271bfbbdcf7f1012ef241`。
仅回收四个菜单47D8D0站；完整父链保持pending。
不标`assembly_exact`，不改变Workpack316/B11验收。

## 1. 独立原指令与实际对象

父462740..4640F1完整2820行，95静态call、37plain RET，无外部chunk。
叶47D8D0..47D8DE完整9行，ECX=this，无参数、写入、调用或分支。
MOV DWORD[ECX+D94]、AND FF、AND1、plain RET，完整结果0/1，EDX不变。
四站462D0C/4634E3/463830/463974在位1前缀后按live 53BD54选择对象。
this为5029D0+(live actor-8)*2F34；先清真实53BF88，再读取实际BYTE位0。
此前456859/457BAA/457CBF/46AD3B已迁移，原八站现均读实际BYTE。

## 2. 数据流、分支与出口

直接借action.group_a_action_execution的既有BYTE，语义bool进入业务条件。
删除query_actor_property_c、四个协议调用和六个纯叶零回复。
不新建EAX回包、参数数组、字段、同步副本、Port、计数或叶停点。
未拥有对象复用删除调用原有的group_a_actor_typed_stop边界。
完整拥有对象下BYTE读取不证明原DWORD跨页故障或地址回绕等价。

真分支按原顺序分别处理：

- message2先置阻断和输入门，再发布实际敌人数；分类完整DWORD2或输出低WORD0
  时先置辅助门再写live辅助column0=1。两条件同真时只写该行一次。
- message4先发布实际敌人数，再置两门并清live column0；分类完整DWORD2时
  写column0=1后置辅助门，输出低WORD0时另写column0=1。
- message30/8只置两门并发布实际敌人数，保留旧column0及辅助门。

假分支保留默认敌人目标、角色目标轮转、动作重置与实际帧输出。
message2/4的剩余量为DWORD数量减两个零扩展WORD，按u32回绕后unsigned比较4。
真实子失败保留属性、辅助记录和目标前缀，阻断后续输入预置。
message2/8末尾按原指令写动画0/0/4；message4/30保留动画，仅发布message3并预置输入。
本批修正旧公共回退的额外行写入、辅助门顺序及后两路径额外动画清零。
纯叶读取没有人工变更或停止；真实配置前继之后仍重新读取队员。

## 3. 固定状态与真实消费者

新增2160帧：四message×十槽×九BYTE×六分类/输出/旧行组合。
BYTE00/01/02/03/7C/80/FD/FE/FF；分类0/1/2/10002（十六进制）；
输出0/1/FFFF/10000/10001仅比较低WORD；旧column0为0/1/AABBCCDD。
验证位0与位1独立、完整DWORD分类、WORD截断、目标和辅助门、动画保持，
十BYTE、百profile DWORD及全50辅助DWORD保留未写部分。

2160帧均执行实际input case3阻断判断与攻击顺序插入。
前者在00460527..00460547按同一53BF88精确1判定；其它两门为0。
后者复制同一五字段，column3按WORD取值，复制后仅清本行及双门，邻居保持。
没有用测试Port替代这些真实消费者。

另八帧覆盖剩余量3/4及两种减法回绕；游标0、拥有槽2命中原4A7970首候选2，
真实轮转保持原signed循环，不增加上限。
八帧覆盖真实默认目标边界及真属性绕过；八帧覆盖真实输入预置部分写入失败。
合计2184新菜单执行；1080坐标帧与424既有菜单帧同步。
八个真实前继向量令旧/新队员两位分别相反；十二未拥有对象、四早返继续执行。
旧WORD/位1的纯叶观察撤回，改检查实际状态和队列；两个轨迹各减删除的一次调用。

## 4. 实际验证与纠错

proc_5c1c成功319秒：core setup1/1为11.57秒，ASan1/1为19.70秒。
SDL实际重新链接，三源SHA256与whitespace通过。
使用build.sh core、build-asan.sh和build.sh app；
定向target为openswd3_battle_legacy_battle_setup_tests，
regex为`^battle\.legacy_battle_setup$`，SDL target为openswd3。
proc_0ba5源码审计成功2秒：原范围、四站、实际状态、向量、门日志与源身份通过。
production214行/tests555行完整差异及stage均已读取，逐字一致。
实现三文件433行增加、95行删除，已提交并推送。

首轮及诊断轮各两失败来自count4夹具遗漏初始化游标：默认1导致四次读取。
显式设置向量要求的0后通过，保留原循环及生产逻辑，两份失败日志保留。
首源码检查因clang换行误判调用文本，改为忽略空白后通过，错误日志保留。
原指令、独立分析、双向复核、hash、差异、门禁及错误日志见
build/tmp/runtime/menu-target-block-*。

## 5. 未完成范围

47D8D0与47D880的原八站、47D8B0的原五站均已迁移实际属性。
MON生产、D94/profile别名、父编号/参数包/寄存器报告/轨迹/计数/Port、
其它资源和全部父出口、完整SDL战斗帧与全项目Goal继续pending。
没有全量UT、Windows、原版动态差分、真实MON验收或游戏运行。
相关范围见[菜单父](battle-target-selection-refresh-00462740.md)、
[前一属性](battle-menu-party-coordinate-attribute-0047d880.md)、
[实际队列](battle-attack-order-insert-0045ee70.md)、
[全项目清单](invoke-migration-progress.md)。
