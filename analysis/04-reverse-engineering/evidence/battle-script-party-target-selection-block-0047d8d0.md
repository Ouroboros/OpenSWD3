# 战斗脚本指定队员动作时限制目标选择

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`。
实现源码：`be9645e454b6ca96d4efa494fe6d26cadad7711d`。
本批仅脚本23的46AD3B→47D8D0及必要共享接线；父链仍pending。

## 1. 完整原指令与条件

完整47D8D0..47D8DE共9行：MOV EAX[actor+0D94]，AND FF，AND1，plain RET。
无参数、调用、分支或写入；完整EAX只能0/1，业务条件仅BYTE bit0。
八物理caller为456859、457BAA、457CBF、462D0C、4634E3、463830、463974、46AD3B。
当批其余七父点待迁移。脚本23完整46AC13..46AF0F共315行。
后续5b44fdb2回收456859，当前剩余六点，见
[队员帧属性](battle-party-frame-target-selection-0047d8d0.md)。
46AD40比较完整EAX与EDI1，真时46AD44写DWORD53BF88=1；假时保留原值。
独立记录指令、条件、读写顺序及向量后，再核对现有C++和测试。

## 2. 实际来源、共享存储与原序

直接读已有action.group_a_action_execution[].special_particle_coordinate_suppression
BYTE，来源仍是实际actor+0D94。runtime reset保存/恢复，效果读bit1、cleanup清零。
保存对象负责profile/bit1读取；随后按实时选择读取bit0，保留既有拥有对象边界。
原序为profile数值→bit1全局/辅助行→bit0目标限制→状态/角色槽→frame→live cursor+8。
零路径不清旧DWORD；其它属性位和对象不写，不重建寄存器式结果。

删除脚本shared.target_selection_block影子；53BF88借现有
LegacyBattleFrameInputResolutionState.target_selection_block。
目标选择、菜单、信息框、reset及真实输入判断均使用该既有持有者。
SDL接battle_frame_input_resolution_state()，三个实际bindings构造均借对应存储。
没有新增生产owner、查询包装、操作编号、回包、counter或停点。
SDL旧state port实际拥有存储，本批未增加转发adapter。

opponent-action MirrorScriptFixture同步新引用，并补上此前遗漏的required items引用，
使用实际test-owned物品列表。该文件在独立actor_frame_316，不在setup编译范围；
因此本批额外验证这个实际调用方的core/ASan，不能把setup通过当作它的覆盖。

## 3. 当前执行与纯叶夹具校准

新增270次：十槽×九BYTE×旧DWORD0/1/AABBCCDD。
BYTE为00/01/02/03/7C/80/FD/FE/FF；bit0 true置1，否则保持旧值。
真实is_legacy_battle_frame_input_case_three_blocked在frame前观察同一存储，
旧非1保持但不阻止该判断，旧1在false路径继续阻止。
同时验证实际profile ABCD→2BCD、独立bit1、完整辅助块、所有属性及邻近输入字段。

同步406次：180属性/缓存、20真实源前继改写、2源/frame停止、200标记WORD、
4未拥有保存对象。再适配既有party1次，保留nonparty2次。
撤回唯一47D8D0纯叶停止；200/20次发布观察移到真正frame前继，无纯叶callback。
两真实剩余停止保留可观察前缀；未拥有对象仍保留既有访问边界及旧共享标记。
当前676次是注册循环执行，不替代历史407/214，也不是原动态资源行为证明。

proc_10d4成功269秒：core setup1/1 11.21秒，ASan setup1/1 18.01秒，
core actor_frame_3161/1 26.29秒，ASan同入口1/1 26.98秒；SDL实际重新链接。
五源SHA及空白检查通过；proc_4851原指令/八caller/借用/注册/身份审计成功3秒。
已完整读取生产75行及测试318行差异；stage与已读差异逐字相同。
原测试size_t→u32 warning保留，当前定位2565行；SDL无warning/error。
原指令、独立分析、双向复核、差异、hash及实际日志见
build/tmp/runtime/script-party-following-attribute-*。

## 4. 验证边界与未完成范围

不标`assembly_exact`；完整拥有对象下的BYTE读取不证明原部分地址/page fault等价。
后续队员帧切片后，剩余六个47D8D0、七个47D880、四个47D8B0父点仍pending。
4707B0资源/输出、入口三WORD/晚+6前缀、状态/角色槽影子、53BD50别名、
父参数/寄存器报告/轨迹/计数/Port，以及真实资产与完整SDL运行仍pending。
没有全量UT、Windows、原版差分或游戏执行验收；Workpack316状态不变。
详见[数值历史](battle-script-party-profile-word-0047d8b0.md)、
[属性历史](battle-script-party-coordinate-suppression-0047d880.md)、
[脚本父证据](battle-script-dispatch-00469d20.md)、
[全项目清单](invoke-migration-progress.md)。
