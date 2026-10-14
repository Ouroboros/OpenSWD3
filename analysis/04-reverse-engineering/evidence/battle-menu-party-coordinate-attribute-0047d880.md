# 战斗菜单应用行动配置后记录队员的坐标抑制属性

状态：`pending`。

## 1. 独立原指令与范围

父462740..4640F1完整2820行，95静态call、140实际跳转、
110局部/默认标签、37plain RET，无外部chunk。
入口无栈参数，唯一静态caller在4620D0的462318。
叶47D880..47D890完整10行，ECX=this，无栈参数或副作用。
MOV EAX[ECX+D94]；AND FF；SHR1；AND1；plain RET。
完整结果仅0/1，EDX不变；不制造叶回包、mutation或stop。

本批站为462CCD、4634A5、4637F2、463936。
456844、457B57、457CA9、46AD03在此前切片已迁移。
原八站现均使用实际BYTE；完整父链不因此关闭。
首次地址提取多带入各函数七行前后说明，按proc到endp修正。
实际父/叶/消费者分别2820/10/156行，机器指令没有删除。

## 2. 保存对象、共享状态与原序

四站this为5029D0+(保存actor-8)*2F34。
实际配置前继后保存队员；先清53BFF0，再写保存行column3。
位1读取借同一十槽action.group_a_action_execution的实际
special_particle_coordinate_suppression BYTE，不增加字段或同步层。

读取后重读53BD54。真分支先写共享selection_target_cache=1，
再写live角色block_520e90的column2=1。
假分支保留旧column2；前置清零后的全局仍为0。
四组global/row写地址分别为：

- message2：462CE0/462CE6。
- message4：4634B7/4634BD。
- message30：463804/46380A。
- message8：463948/46394E。

公共后继清真实53BF88并进入尚未迁移的bit0查询。
保存角色和live写行保持独立；原前继门与早返不改。
删除query_actor_property_b及六个测试假回复。
不增加Port、Adapter、参数包、寄存器回包、计数或叶停点。
未拥有对象沿用既有group_a_actor_typed_stop和失败前缀。
完整拥有对象下的BYTE读取不证明原DWORD跨页故障或地址回绕等价。
profile与其它重叠视图的生产/生命周期仍待全链审计。

## 3. 固定状态与真实消费者

新增1080帧：四路径×十槽×九BYTE×三旧column2。
BYTE为00/01/02/03/7C/80/FD/FE/FF；旧值为0/1/AABBCCDD。
邻居属性位1取相反值，检查保存对象选择及完整只读数据。
检查整个50 DWORD辅助块、十个BYTE、百个profile DWORD、
真实后继可见前缀、message与输入预置结果。

1080帧随后均执行真实攻击顺序插入。
45EF1D/45EF23将同一column2复制到record.value_18；
其它四列也复制，column3按WORD读取，再清本行五DWORD及双门。
验证复制后清行、邻居保持，未以测试Port代替该消费者。

既有424帧同步：400标记WORD、八前继重读、十二未拥有对象、四早返。
八前继向量令旧/新角色位1相反；变更发生于真实配置前继。
观察点移到剩余bit0后继，纯位1叶无人工副作用。
两个目标循环轨迹各删除一次协议调用；完整相关门通过。

## 4. 实际验证与证据身份

proc_f6a5成功337秒：core setup1/1为10.75秒，
ASan setup1/1为17.71秒；SDL实际重新链接。
使用build.sh core、build-asan.sh及build.sh app。
定向target为openswd3_battle_legacy_battle_setup_tests，
regex为`^battle\.legacy_battle_setup$`，SDL target为openswd3。
三源SHA256与whitespace通过；既有两处无关warning保留。
proc_350b源码审计成功3秒：范围、四站、实际数据/原序、向量、
消费者、门日志及冻结差异全部通过。
完整production33行、tests268行差异已读，与stage逐字相同。
实现三文件145行增加、32行删除，已提交并推送。
临时原指令、独立分析、双向复核、日志、差异及hash保留于
build/tmp/runtime/menu-coordinate-attribute-*。

## 5. 未完成范围

后续371fffc0回收462D0C/4634E3/463830/463974四个47D8D0菜单站，见
[菜单目标限制](battle-menu-party-target-selection-block-0047d8d0.md)。
纯叶观察已改为实际状态和消费者；完整菜单父继续pending。
MON源处理、父编号/参数包/寄存器报告/轨迹/计数/Port、
共享别名、全部父出口与完整SDL战斗帧继续pending。
没有全量UT、Windows、原版动态差分、真实MON验收或游戏运行。
相关范围见[菜单父](battle-target-selection-refresh-00462740.md)、
[菜单标记数值](battle-menu-party-profile-word-0047d8b0.md)、
[全项目清单](invoke-migration-progress.md)。
