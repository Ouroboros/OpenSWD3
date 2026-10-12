# 战斗脚本指定队员动作时读取标记数值

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`。
实现源码：`a0bfde98e155178e891ecec69d48b25954b07047`。
本批只回收脚本23的46ACD9→47D8B0；父链仍pending，不标`assembly_exact`。

## 1. 完整原指令与消费宽度

完整叶47D8B0..47D8C5共15行；无参数、call、写入，两个plain RET。
MOV AX[actor+0DAE]，TEST AH80，JZ：bit15为1时AND EAX7FFF；
为0时XOR AXAX。后一出口保留原高WORD，不是完整EAX零。
脚本46ACE1先AND EAXFFFF，46ACE6只消费低WORD并零扩展写入辅助DWORD。
所以业务值为bit15为1时的低15位，否则0；不重建寄存器回包。

五物理调用为00462C9D、00463475、004637C2、00463906、0046ACD9。
前四个菜单父点已在后续e237b6e7迁移，见
[菜单标记数值](battle-menu-party-profile-word-0047d8b0.md)。
脚本23完整315行，范围46AC13..46AF0F。
独立记录原叶、分支、宽度和向量后，再核对当前C++和测试。

## 2. 实际来源、借用与原序

已有startup.party[index].final_processing.profile_buffer为actor+D90十DWORD。
0DAE相对偏移1E，对应DWORD7高WORD；直接读取该缓冲区，不新增独立字段。
GroupAStorage配置借同一party.final_processing，runtime reset序列化及恢复
同一profile_buffer。其它同址逻辑字段仍有别名债务，不宣称整体已合并。

原源处理4707B0..47081B完整54行，调用MON、478220及476A80，
后者写actor+D90；再处理D9E和2A87。该源处理协议本体继续待迁移。
源前继返回后46ACB7/BD保存实际选择；查询该保存对象，再写index*5+3。
随后读取同对象坐标抑制属性并发布标记，最后到47D8D0和frame。
保留其它半WORD、相邻DWORD、未选对象及完整辅助行。
没有纯叶回调、模拟EAX、调用计数、新停点或资源分配。

未拥有保存对象仍在既有group_a_token访问边界停止，保留源前缀。
这是完整十槽所有权模型范围；不能证明原未知地址或部分对象故障等价性。
剩余source、47D8D0和frame协议的停止仍保持其既有前缀。

## 3. 当前实际测试与历史夹具校准

新增200次执行：十槽×10 WORD×坐标bit1两侧，覆盖0000/0001/7FFE/7FFF/
8000/8001/ABCD/F00F/FFFE/FFFF。源前继后改写实际profile高WORD，
检查后继看到的发布顺序、完整50 DWORD辅助块和十对象全部profile DWORD。
初始值7654与后继值不同，不依赖旧模拟query EAX1。

同步207次：180 BYTE/缓存，20实际源前继换角色/属性/profile及游标，
3剩余前继/frame失败，4未拥有保存对象。另适配既有party入口1次，
保留既有nonparty两次。全部实际注册到test_battle_script_dispatch/setup。

47D8B0是纯读取，撤回其制造live/saved分离的8次注入和1次停止。
20次改写移至仍待迁移的4707B0源前继；只验证现有协议模型的读取时机。
当批407次不覆盖或替代上一批历史214次记录，也不是原动态差分。

proc_c4eb成功162秒：core setup1/1 10.92秒，ASan setup1/1 17.92秒，
SDL实际重链接，三源身份及空白检查通过。
proc_4a47源码/指令/五caller/实际缓冲借用/注册/身份审计成功3秒。
完整生产差异39行、测试差异284行已读；stage与已读差异逐字相同。
旧2562行size_t→u32 warning保留；SDL无warning/error。
没有全量UT、Windows、游戏执行、原版差分或真实MON/script资产验收。
原指令、独立分析、双向复核、差异、hash及日志见
build/tmp/runtime/script-party-marked-word-*。

## 4. 剩余范围

五个47D8B0站均使用实际WORD；其余47D880菜单父点仍有四站。
完整菜单与脚本父协议仍pending。
4707B0资源/输出、入口三WORD和额外写入、晚+6短读前缀、
状态/actor-slot影子、53BD50别名、父寄存器/轨迹/计数/Port仍pending。
旧属性证据保留历史范围并同步后续校准，见
[坐标抑制属性](battle-script-party-coordinate-suppression-0047d880.md)、
[脚本父证据](battle-script-dispatch-00469d20.md)、
[项目清单](invoke-migration-progress.md)。

## 5. 后续目标属性校准

be9645e4读取实际bit0并写真实输入标记，删除脚本47D8D0查询/影子及其停止1次。
200次数值观察移到frame，旧407范围同步为406；再新增270次目标限制执行。
当前676次、共享接线及两套战斗core/ASan门禁见
[目标选择限制](battle-script-party-target-selection-block-0047d8d0.md)。
后续双方帧切片后，47D8D0菜单余量四站；历史源码、向量、门禁和验证边界保留。
