# 敌方入战镜像直接写实际状态

状态：`pending`。已废弃GPT重建实现，本轮逆向尚未开始。

以下保留历史分析与证据；旧结论、完成记录和测试结果不作为本轮验收。
原版采集事实保留，所有解释和实现结论须重新核实。

## 历史资料（重新核实前不生效）


## 原版行为

00451F93/9F/A6先发布角色、X和Y的WORD，00451FB0清记录DWORD运行值。
00451FAD比较完整镜像DWORD与1；非1跳过镜像及横坐标翻转。
00451FC0调用47F900：读取参数DWORD1并写角色+2B08，ECX保持角色对象地址。
00451FC5/C9/D0随后按WORD计算640-X并写回；配置调用位于00451FE9。
配置失败保留此前的镜像、坐标和记录写入。

00451FD7只覆盖CX，故镜像分支定义参数高WORD来自实际角色对象地址。
删除转发之后使用该实际地址，不再接受可注入的ecx_snapshot回复。
未镜像路径沿用既有参数来源，本批不改配置函数其他寄存器结果。

## 两条生产路径

共用配置函数借用镜像DWORD并直接写1。
SDL传入已有粒子状态render_toggle_gate；独立startup传入已有敌方进度
post_action_value，该字段对应同一角色偏移2B08。
不新增独立状态、资源或失败出口。

删除LegacyBattleGroupBStartupModePort、StoredEnemyModes、StartupEnemyModePort，
以及测试中该敌方镜像操作的模拟回复。
当时保留的队员启动apply_actor_mode后续回收见
[队员启动镜像](battle-startup-party-mirror-0047f900.md)。配置结果及启动父协议继续pending。

## 验证

24组共用配置组合涵盖首末敌方、镜像真假、绘制抑制真假和两类配置失败。
在实际MON流分配入口观察镜像、坐标及记录清零已经发布；失败仍保留此前写入。
非镜像保留FFFFFFFF；镜像完整覆盖为1。
完整startup组合覆盖定义WORD0/1/2/FFFF和镜像DWORD0/1/2/FFFFFFFF，
直接检查实际敌方字段，并将完整场景的模式事件断言限定为剩余队员调用。

仓库脚本执行`openswd3_battle_legacy_battle_setup_tests`及
`^battle\.legacy_battle_setup$`，core为1/1通过、11.27秒，ASan为1/1通过、16.77秒。
SDL实际重新链接，六文件身份一致，三份最终构建日志无警告。
日志前缀：`build/tmp/runtime/startup-enemy-mirror-`。
