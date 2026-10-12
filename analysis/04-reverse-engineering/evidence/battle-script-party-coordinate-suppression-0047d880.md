# 战斗脚本选择队员时读取坐标抑制属性

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`。
实现源码：`21b157ea71cefa0eb9d97229548f9a1274f89983`。
本批仅回收脚本23的46AD03→47D880；父链仍pending，不标`assembly_exact`。

## 1. 权威范围与独立推导

完整叶47D880..47D890共10行，无参数、call或分支，plain RET。
读取DWORD[actor+0D94]，AND FF，SHR1，AND1，返回完整DWORD0或1。
业务消费的是低BYTE bit1；bit0或泛化EAX非零不能代替。
完整脚本23 46AC13..46AF0F共315行，下一case从46AF10开始。
先独立记录机器指令、全部分支和出口，再审查当前C++和测试。

八个物理调用为00456844、00457B57、00457CA9、00462CCD、004634A5、
004637F2、00463936、0046AD03；当批前七个父入口待迁移。
后续45f5a77e回收456844，当前剩余六父点，见
[队员帧坐标属性](battle-party-frame-coordinate-attribute-0047d880.md)。

## 2. 实际数据与原序

实际读取action.group_a_action_execution[index]的坐标抑制属性BYTE。
GroupAStorage配置、runtime reset与效果402借同一action execution；
效果原有逻辑也测试此bit1。没有另建属性DWORD、回包或默认角色。
原DWORD访问与现代BYTE字段在完整已拥有十槽对象域内给出同一业务值。
未覆盖部分原对象、未知地址、页边界异常或原运行保护。

46AC28/46AC33保留53BD54队列值，备份至实际脚本53CE6C。
46AC3D使用现有secondary_actor_code承接53BD50；45AC4E及现有收尾
同字段赋值可复核。active_actor_code等其他同址存储合并仍未完成。
46AC59的辅助行首使用index*5，修正此前直接index的偏移。

46AC85..46ACB2重新取角色，4707B0仍用原有待迁移协议；参数修正为
共享53CCE8有符号DWORD及4FE5D4+index*4输出地址。
46ACB7/46ACBD在47D8B0前保存实际索引。
46ACE1/46ACE6将前继返回低WORD写到520E9C，即辅助块index*5+3。
46AD03随后直接读保存角色的实际bit1，不改成live角色，也不模拟叶寄存器回复。

46AD08..46AD1B为1时先置53BFF0，再根据live53BD50写520E98行，
即index*5+2；为0不清旧缓存或旧辅助标记。
47D8D0按46AD22..46AD3B重新使用live角色，仍待迁移。
前继原有停止回包复用已有状态，保留已提交前缀并阻断后缀。
未拥有角色/辅助行使用既有模型访问边界；这不是原异常等价性证明。

## 3. 实际执行与门禁

新增214次执行，实际注册到test_battle_script_dispatch及setup入口：

- 180次：十槽×九BYTE属性×两个旧缓存；00/01/02/03/7C/80/FD/FE/FF。
- 20次：十保存槽×bit1两侧；前继模型改写实际属性和live选择，查询旧对象、写新行。
- 4次：剩余源处理、WORD查询、后继属性查询及frame的停止回包前缀。
- 8次：未拥有live行，覆盖标记两侧和全局先写顺序。
- 2次：未拥有saved对象，保留availability/source前缀。

另适配既有party入口1次，保留两次既有nonparty执行。
比较完整50 DWORD辅助块、未选角色、邻近属性、队列备份和共享游标。
frame前继修改游标12后成功得到20；没有用模拟属性EAX作业务判定。
47D8B0原纯叶不能自行改写选择；20次注入验证的是当前剩余协议模型，
不是原运行时变更证明。停止回包也不是原纯叶故障证明。

首轮proc_cc61成功166秒：core/ASan setup各1/1、11.65/18.07秒，
SDL实际重链接、三源身份和空白检查通过。复核后补10次访问边界执行。
最终proc_e621成功165秒：core/ASan各1/1、12.13/18.03秒，SDL实际重链接。
proc_f287源码/指令/八调用/实际借用/注册/三源身份审计成功3秒。
旧测试2562行的size_t→u32 warning保留；SDL无warning/error。
未运行全量UT或Windows门禁；不能代替游戏执行、原版差分或真实脚本资产验证。

临时原指令、独立推导、双向复核、完整差异、格式身份及门禁日志统一位于
build/tmp/runtime/script-actor-metric-*。

## 后续纯数值查询的校准

后续源码a0bfde98将脚本47D8B0接到实际profile缓冲高WORD。
纯叶内改写live选择的8次及停止1次夹具撤回；20次改写移到真实源前继。
当批该属性/前缀执行207次，加新数值200次；历史214次及其门禁记录保留。
本文件的前继回包描述对应21b157ea历史版本；当前纯数值读取无回包。
当批范围见[标记数值](battle-script-party-profile-word-0047d8b0.md)。
后续be9645e4将bit0接到实际输入共享标记，撤回其纯叶停止1次。
属性/前缀现206次，数值200次观察移至frame，再加新目标限制270次。
当前676次及两套实际战斗门禁见
[目标选择限制](battle-script-party-target-selection-block-0047d8d0.md)。

## 4. 明确待迁移项

脚本入口仍一次读取三个WORD，party仍额外写value_b/value_c，
晚+6短读前缀尚未校准；本批不宣称完整脚本23收敛。
五个47D8B0站已读取真实profile；四菜单站见
[菜单标记数值](battle-menu-party-profile-word-0047d8b0.md)。
后续d8a93827回收四个47D880菜单站，见
[菜单坐标属性](battle-menu-party-coordinate-attribute-0047d880.md)。
后续371fffc0回收四个47D8D0菜单站，见
[菜单目标限制](battle-menu-party-target-selection-block-0047d8d0.md)。
原八站均读实际属性，完整父链仍pending。
4707B0、状态/角色槽影子、非party retry/RNG及资源处理继续pending。
操作编号、参数包、寄存器结果、轨迹、计数、Port和实际SDL整链均未完成。
见[脚本父证据](battle-script-dispatch-00469d20.md)及
[全项目迁移](invoke-migration-progress.md)。
