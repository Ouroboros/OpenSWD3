# 敌人开始行动时的群体效果属性 `0x0047D880`

状态：`pending`。已废弃GPT重建实现，本轮逆向尚未开始。

以下保留历史分析与证据；旧结论、完成记录和测试结果不作为本轮验收。
原版采集事实保留，所有解释和实现结论须重新核实。

## 历史资料（重新核实前不生效）


历史状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`。
源码提交：`43c7652e`；七文件710行增加、89行删除。
范围仅457B57/457CA9属性读取、实际共享标记、完成清零与必要调用方。
父链仍pending；不标`assembly_exact`。

## 1. 原指令与独立推导

完整父4576A0..4582AB从proc到endp1355行、86个call、66个局部标签、
116个跳转、三个plain RET，无FUNCTION CHUNK；入口为一个DWORD敌人索引。
唯一父caller为45B5E0。47D880..47D890完整十行、无参数、无写入或内部调用：
读DWORD[ECX+D94]，AND FF、SHR 1、AND 1后返回完整0/1，不修改EDX。

4576CD以525508+原index×2B28建立对象，457750恢复保存对象到EBP。
457B57/457CA9均以该EBP为this，读取当前行动敌人，不能读取选中队员。
文本或其它前继改实时选择，不改变保存的敌人对象。

普通路径457B36先查询readiness；完整值1才执行457B46低WORD索引发布、
457B4D清53BD0C、457B57属性读取。真在457B60写53BFF4 DWORD1，假保留旧值。
非1跳过此属性与文字，仍执行独立资料/bit0后继，不是整帧停止。
无文字时EDX保留readiness前继值，有文字时保留既有文字后继值。
纯叶没有EDX回复；该点按指令与源码追溯，不新增寄存器诊断字段。

标记路径457C55实时读状态高BYTE40，先设置显示值2并可能执行文字；
457C8D再实时读高BYTE20决定显示值6，随后457CA9读取保存敌人属性。
457CB7真写同一DWORD1，假保持。入口8000判断仍使用原入口快照。
478710..47876B完整51行：模式2/6写显示WORD2A70并清动作WORD2A6C；
6还OR BYTE2A87的8。不能把动作值当显示值，或以入口缓存代替两次重读。

4580EA真正完成时清同一53BFF4；此前实际衰减失败保留旧标记并阻断后缀。
完整效果消费者45C010..45D17A共1915行：45C19D读53BFF4，53BF84为0时，
45C1A8仅精确DWORD1进入458DE0群体效果，其余值进入45CA20单体效果。
全局45BC8D已有清零也作用于同一字段。

## 2. 实际数据与调用方

敌人profile生产者把40字节写入action_configuration.profile_buffer，
D94为byte4。两站直接读取该实际BYTE的bit1，没有叶编号、参数数组、
模拟回复、调用计数或新增停止边界。已有idle/status前继先建立实际生命周期。
完整拥有对象的BYTE读不证明原DWORD跨页部分地址故障等价。

真实53BFF4由LegacyBattleEffectCoordinatorState::group_b_effect_mode拥有。
敌人帧以必需语义引用借用，删除special_action_latch影子；两次置1与完成清零
均改写此共享字段。actor序列传递同一引用，没有增加生产Port/getter/adapter。
63个既有测试调用、两个帧协调器夹具及转场暂停夹具同步实际字段。

全仓advance context八个引用中只有三个测试构造，均已接实际效果持有者。
生产帧协调器借既有context，actor序列继续传递此引用；SDL尚无该context构造。
SDL链接证明静态接口，完整生产战斗帧接线和实际运行继续独立待验收。
profile的runtime-image/action-execution重叠视图仍是待审别名债务。

## 3. 独立向量与真实消费者

新增450次帧：正常/标记各216（八槽×九BYTE×三旧标记）；
非1readiness六次、真实前继变更六次、完成清零/实际衰减失败六次。
BYTE为00/01/02/03/7C/80/FD/FE/FF；旧DWORD为0/1/AABBCCDD。
主向量检查全部320个profile字节、反向队员属性、真写与假保留。
432次主向量各调用一次实际效果协调器，验证同一标记的精确1分流。
其它未迁移效果调用仍使用既有测试Port，不能据此宣称消费者整体迁移。

实时向量在readiness或文字前继改byte4、状态字和实时选择，
区分保存敌人5与实时敌人7；检查显示2/6、动作0及模式位8的原顺序。
完成向量用实际衰减访问失败，不伪造纯属性叶停止或成功。
原idle/profile/资源停止夹具继续在既有访问点停止。

## 4. 纠错与实际门禁

调用方迁移脚本在zero_port处写文件前失败，支持实际具名Port后同步63处。
首轮编译两条不存在的结果字段断言已撤回，没有新增诊断字段。
最初四项实时失败同时含错误显示字段观察，不能单独归因为缓存。
纠正观察后，仅恢复旧缓存两条件的proc_8d60仍恰好四项失败：
退出8、41秒、角色帧测试25.62秒；随后恢复两次实时读取。
受控反例见build/tmp/runtime/enemy-frame-coordinate-attribute-cache-only-red.log。

proc_7b61角色帧core1/1通过27.69秒；随后setup目标名写错，后缀未执行。
核对注册目标openswd3_battle_legacy_battle_setup_tests后，首轮编译发现
转场夹具缺少实际效果状态头文件；补入必需依赖，proc_f72c成功224秒：
setup core1/1为9.95秒，角色帧ASan1/1为26.90秒，setup ASan1/1为16.52秒。
SDL实际重链接、七源hash与空白检查通过。角色帧core不编译转场夹具，
其已验证相关源码未再改变。setup原2565行size_t→u32 warning保留。
proc_1639完整物理指令/源码/注册/四门/七身份审计成功2秒。
完整生产147行、测试1310行差异已人工复核，与暂存版本逐字相同。
全部原指令、独立分析、双向追溯、差异和日志见
build/tmp/runtime/enemy-frame-coordinate-attribute-*。

## 5. 未完成范围

后续c6fff61c回收两个敌人bit0父点，并将父目标计数与实际效果消费统一，见
[效果目标数量属性](battle-enemy-frame-target-count-0047d8d0.md)。
原450次bit1执行同步，纯叶bit0观察站和回复撤回，改观察真实最终数据。
后续e237b6e7回收四个菜单47D8B0站，见
[菜单标记数值](battle-menu-party-profile-word-0047d8b0.md)。
后续d8a93827回收四个47D880菜单站，见
[菜单坐标属性](battle-menu-party-coordinate-attribute-0047d880.md)。
后续371fffc0回收四个47D8D0菜单站，见
[菜单目标限制](battle-menu-party-target-selection-block-0047d8d0.md)。
原八个47D8D0站均读实际BYTE，完整父链继续pending。
actor-ready八槽、父编号/参数包/寄存器/轨迹/计数/Port、
资源生命周期、共享别名、完整父逐出口和生产帧接线仍pending。
没有原版差分、Windows、完整SDL运行或全量UT验收。
不关闭Workpack316/B11或全项目Goal。
相关范围见[敌人帧](battle-group-b-frame-004576a0.md)、
[实际效果消费者](battle-effect-coordinator-0045c010.md)、
[队员属性](battle-party-frame-coordinate-attribute-0047d880.md)、
[脚本属性](battle-script-party-coordinate-suppression-0047d880.md)、
[全项目清单](invoke-migration-progress.md)。
