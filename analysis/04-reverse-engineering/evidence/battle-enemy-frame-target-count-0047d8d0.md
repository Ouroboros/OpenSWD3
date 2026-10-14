# 敌人开始行动时的效果目标数量属性 `0x0047D8D0`

状态：`pending`。已废弃GPT重建实现，本轮逆向尚未开始。

以下保留历史分析与证据；旧结论、完成记录和测试结果不作为本轮验收。
原版采集事实保留，所有解释和实现结论须重新核实。

## 历史资料（重新核实前不生效）


历史状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`。
源码提交：`c6fff61c`；七文件336行增加、385行删除。
范围为457BAA/457CBF两个属性站、实际目标计数及必要调用方。
父链仍pending；不标`assembly_exact`。

## 1. 原指令与独立推导

47D8D0..47D8DE完整九行：DWORD[ECX+D94] AND FF AND1，EAX完整0/1。
无参数、call、写入或分支，plain RET，不修改EDX。
父4576A0..4582AB完整1355行、86calls、66labels、116jumps、三个plain RET，
无chunk，唯一caller45B5E0。两个属性站均以保存敌人EBP为this。
实时角色或随机目标改变不改变该对象。

普通457BAA先经历readiness、可选文字和476140实际profile前继；
457BA3再次发布原index低WORD。真457BB3置53BF8C DWORD1，
457BB9清53BD7C DWORD0并跳公共动作；假保留字段，457BC8只有live gate
完整零才选择目标，任何非零跳公共动作。readiness非1仍继续profile与bit0。

标记457CBF在实时40/20、显示模式及bit1之后读取同一保存敌人。
真457CC8置gate1、457CCE清同一计数；假保持前继值。
随后无条件执行457CD4目标分支，不能照搬普通真分支的跳过选择。
纯叶不新增寄存器结果、观察回调、虚假停止或空值fallback。

## 2. 实际计数与所有权

53BD7C由本父累加、按signed比较、复制目标数量、两属性清零和完成清零；
同时由45CDCC/45CF7B/45D0F3效果完成判断读取，45B9EA全局reset清零。
三个消费点先递增53BD80，再与53BD7C作完整DWORD精确相等比较。
此前phase_progress与协调器completion_target_count是同址的重复状态。

本批删除phase_progress。父的六处访问全借既有
LegacyBattleEffectCoordinatorState::completion_target_count，
保持DWORD减法/递增回绕和原signed阈值比较。
两写、成功完成清零、扫描累加与真实消费者共用一个字段，无复制同步层。
原全局reset已清该协调器字段，不修改其行为。
53BF8C父gate保留原字段；其global-reset字节像别名仍待后批审查。

实际效果状态引用取代上一批marker标量引用，同时借群体marker和目标计数。
actor sequence、67个旧测试调用、两个coordinator构造及transition夹具同步。
没有增加生产Port/getter/adapter。实际profile byte4仍来自MON的40-byte owner；
完整BYTE验证不证明原DWORD跨页故障等价，profile重叠别名仍待审。
全仓只有三个测试advance context构造，SDL完整生产战斗帧仍未接通。

## 3. 实际向量与消费

新增450帧：432主向量（正常/标记×八槽×九BYTE×旧计数0/1/AABBCCDD），
6个非1readiness，12个真实readiness/文字前继修改。
九BYTE为00/01/02/03/7C/80/FD/FE/FF；检查320 profile bytes、反向队员/执行
属性视图、两个bit独立效果、普通真值跳过与标记真值继续目标选择。
前继改保存enemy5、live actor7、count和gate；假保持gate2/FFFFFFFF及旧计数。

432主帧各调用实际效果协调器，使用真实完成记录、实际pair/reward/resource
操作到达目标计数比较：计数精确1返回1并清completed_count，
其余0/AABBCCDD返回0并保留completed_count1；目标计数自身保持。
其余尚未迁移调用使用既有测试Port；不证明消费父整体迁移。

原450 bit1帧同步，虚假47D8D0观察站和EAX回复撤回；
改观察最终真实marker/显示状态，E002用实际BYTE3表达两个属性。
其中六个完成/实际衰减失败向量同时验证目标计数清零或旧值保持。
已有扫描累加、signed比较、候选重建和真实选择访问失败测试观察同一字段。

## 4. 门禁与审查

proc_7694成功47秒：角色帧core1/1为24.45秒。
proc_7a38成功213秒：setup core1/1为10.24秒，角色帧ASan1/1为26.89秒，
setup ASan1/1为17.00秒，SDL实际重新链接；七源hash与空白检查通过。
setup原2565行size_t→u32 warning保留。
proc_9006完整物理范围/八caller/六处借用/消费/身份审计成功2秒。
生产176行、测试1305行完整差异已逐行读取，stage与已审版本逐字相同。
调用方脚本成功2秒；一次精确替换因两个相同区域拒绝，限定上下文后成功，
没有以最终通过抹去该工具纠错。原指令、独立分析、双向复核、日志与hash见
build/tmp/runtime/enemy-frame-following-attribute-*。
proc_2efb计量成功7秒，proc_3c6a证据审计成功21秒；源身份/远端、
可复现874/324/27/293清单、范围false、无关分类及文档链接通过。

## 5. 未完成范围

后续e237b6e7回收四个菜单47D8B0站，见
[菜单标记数值](battle-menu-party-profile-word-0047d8b0.md)。
后续d8a93827回收四个47D880菜单站，见
[菜单坐标属性](battle-menu-party-coordinate-attribute-0047d880.md)。
后续371fffc0回收四个47D8D0菜单站，见
[菜单目标限制](battle-menu-party-target-selection-block-0047d8d0.md)。
原八站均读实际BYTE，完整菜单父仍待迁移。
actor-ready八槽、父编号/参数包/寄存器/轨迹/计数/Port、共享别名、资源、
全部父出口和SDL生产帧接线仍pending。无全量UT、Windows、原版差分或游戏运行。
不关闭Workpack316/B11或全项目Goal。相关范围见
[敌人帧](battle-group-b-frame-004576a0.md)、
[前一属性](battle-enemy-frame-coordinate-attribute-0047d880.md)、
[实际效果消费者](battle-effect-coordinator-0045c010.md)、
[全项目清单](invoke-migration-progress.md)。
