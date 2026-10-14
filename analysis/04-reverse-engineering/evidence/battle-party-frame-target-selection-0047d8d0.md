# 战斗队员自动选敌人后记录目标选择标记

状态：`pending`。
实现源码：`5b44fdb2deb7a3c56226130743b21a7585d0767e`。
本批只迁移456859→47D8D0及其实际写入、后续读取、借用和测试。
完整456680父链继续pending，不标`assembly_exact`。

## 1. 原指令、范围与出口

完整父函数456680..45769B共1795行、101静态call、84标签、157跳转、一plain RET。
参数为一个DWORD角色索引，无外部chunk。唯一原caller为45B61F。
完整47D8D0..47D8DE共9行：MOV EAX[actor+0D94]，AND FF，AND1，plain RET。
无参数、内部调用、写入或失败分支；完整EAX只能0/1，实际条件为BYTE bit0。
八个原物理caller的清单保留于[脚本属性证据](battle-script-party-target-selection-block-0047d8d0.md)。

456859读当前角色属性；45685E比较EAX与EDI1。
真时456862计算index×5，456865写DWORD520E94[index×5]=1；假时保留旧DWORD。
4569C4在后续角色选择分支比较同一DWORD与精确1，非1跳456C38。
它不是actor+2AB0完成标记，也不是脚本使用的全局53BF88。
两处53BF88写入456BDC/456C27属于后续另一业务分支。

原前缀：入口展示/开始门→AI条件→47DAD0→真实46E520进度→46EE60属性效果。
actor+2B00/+2B04为DWORD精确1条件；扫描实时敌人数，terminal精确1累计。
全terminal或非正数量走真实478850重置；否则478330写availability1，
RNG每次重读数量并发布one-based选择，terminal候选精确1重试。
随后456829先置53BF60，再执行480220；只有完整返回1才进入属性读取。
独立记录完整范围、条件、位宽和测试后，核对当前C++；没有把纯叶变成资源停点。

## 2. 实际来源、共享存储与调用方

直接读取既有action.group_a_action_execution[index]中的实际BYTE。
条件真写startup.reset.block_520e90[index×5+1]，假不清旧值。
后续分支直接读同一数组，删除对actor.action_complete的错误替代。
actor.action_complete仍是actor+2AB0，真实46E520前继可合法置1。
没有新增owner、字段、包装、编号、request/reply、counter或停点。

生产唯一源码caller为actor_frame_sequence，仍使用既有context和shared.action。
SDL的battle_action_dispatch借battle_actor_frames.shared.action；
GroupAStorage借同一action与battle_runtime；startup.reset继续是辅助块持有者。
夹具删除独立startup_reset影子，context.startup_reset借startup.reset。
与本次读取有关的旧分支夹具同步实际辅助块，合法角色完成状态保留。

## 3. 最终处理与失败诊断校准

父帧末尾45766F调用45AA00(index,1)；其45AAC9调用4750C0。
完整4750C0..47515D共62行，4750C6令ECX=10，4750CB清EAX，
4750CD指向actor+0D90，4750DA REP STOSD清十DWORD，包含0D94属性。
当前真实final_actor_step→group_a_actor_cleanup执行同一profile重叠清零。
查询读取和整个父帧返回后的属性值不能混为一谈。

首轮proc_746b core失败276项：测试错误期待进度前继不置角色完成1。
该事实已校准；此前“未到预期前缀”的推断撤回。
次轮proc_4d49失败240项，proc_365d/proc_a049/proc_7ede进一步定位为
最终处理清零当前属性，数组写入、读取条件和前缀本身通过。
核对原最终处理与完整cleanup后，测试在真实前继验证原属性、
返回后验证仅当前属性为0、其它角色保持，以及真实cleanup发生一次。
临时诊断输出全部删除，没有改生产后缀来迎合测试。

## 4. 当前实际验证

新增285次注册执行：270次十角色×九BYTE×旧DWORD0/1/AABBCCDD；
6次readiness0/2/FFFFFFFF；9次辅助DWORD与角色完成DWORD独立交叉。
九BYTE为00/01/02/03/7C/80/FD/FE/FF。
完整50DWORD辅助数组比较保证只有当前column1按bit0修改；
观察availability/selection/进度前缀、独立bit1调用及最终清零。
消费者测试区分精确1与非1；沿既有真实idle访问失败保留前缀。
当批47D880以既有测试前继隔离，不冒充其原版行为验收。
后续45f5a77e撤回此纯叶回复，285次执行观察移至真实readiness，
两列由实际属性共同更新，见[队员帧坐标属性](battle-party-frame-coordinate-attribute-0047d880.md)。

proc_a65e成功127秒：core actor_frame_316 1/1 26.69秒，
ASan同入口1/1 28.03秒；SDL实际重新链接，无warning/error。
两源SHA、空白及完整原范围/leaf/借用/注册/身份审计均通过。
生产39行与测试374行完整差异已读取，暂存与已审差异逐字相同。
两个实现文件186行增加、23行删除，已提交并推送。
原指令、独立分析、双向复核、失败/最终日志、差异与hash见
build/tmp/runtime/party-frame-target-block-*。

## 5. 保留范围

后续c6fff61c回收457BAA/457CBF，当批四个47D8D0菜单父点为
462D0C、4634E3、463830、463974，见
[敌人帧效果目标数量](battle-enemy-frame-target-count-0047d8d0.md)。
后续菜单标记WORD切片已回收四个47D8B0站，见
[菜单标记数值](battle-menu-party-profile-word-0047d8b0.md)。
后续d8a93827回收四个47D880菜单站，见
[菜单坐标属性](battle-menu-party-coordinate-attribute-0047d880.md)。
后续371fffc0回收上述四个菜单父点，见
[菜单目标限制](battle-menu-party-target-selection-block-0047d8d0.md)。
原八站均读实际BYTE，完整父链继续pending。
当批保留的bit1辅助替代已在该后续独立切片校准，完整父链仍待迁移。
actor-ready八槽、其它编号/参数数组/寄存器报告/轨迹/计数/Port及资源仍待迁移。
完整父函数逐出口收敛、共享别名、原地址保护、实际资产、原版动态差分、
完整SDL战斗运行和Workpack316/B11仍未验收；不得据本批升级父项。
没有全量UT、Windows、原版/游戏执行，完整拥有对象读取不证明部分地址fault等价。
[父帧证据](battle-group-a-frame-00456680.md)、
[叠加历史](battle-group-a-overlay-clear.md)、
[全项目清单](invoke-migration-progress.md)。
