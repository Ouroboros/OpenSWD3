# 入战配置直接写敌方绘制抑制

## 原版顺序与两条实际路径

00451FE9配置当前敌方后，00451FEE比较定义WORD与1。
只有相等才在00451FFD写该对象完整DWORD2AF0=1；非1保持此前值。
下一条00452002重新加载敌方计数，不使用字段写入留下的EAX。
配置失败保留原先记录、资源及已执行状态，不执行该后续字段写入。

startup与SDL存储路径共用configure_legacy_battle_group_b_startup_placement。
原SDL路径经StoredEnemyModes写实际字段，startup路径经set_enemy_mode事件转发。
现共用函数在配置成功且suppress_drawing为真时直接写actor.runtime_reset.field_2af0。
两个生产入口仍通过原定义WORD恰等于1生成该语义参数。

删除set_extra_mode虚方法、两处实现及set_enemy_mode枚举项；不保留空回调或占位事件。
本批当时StoredEnemyModes只保留实际镜像状态所需的粒子借用；其后续回收见
[敌方启动镜像](battle-startup-enemy-mirror-0047f900.md)。
剩余启动枚举保持原值，未借此更改本批之外的协议。
MON配置及其余父接口仍待迁移；本批不声明整个启动接口完成。

## 独立验证

- 首末敌方、镜像与否、抑制与否、正常/资源分配失败/配置档分配失败，共24组。
- 验证写完整DWORD1，未命中及失败保留FFFFFFFF，相邻字段保持89ABCDEF。
- 在配置的说明资源释放处观察到旧抑制值，验证写入在配置及释放成功之后。
- 完整startup覆盖定义WORD0/1/2/FFFF，只有1写实际字段。
- 原startup完整场景改验两个敌方实际字段，不再验转发事件数量。
- 同一启动聚合目标继续覆盖已有SDL存储、实际资产和失败测试。

首轮新配置测试漏设已构造对象的资源标识，16组停在既有资源读取检查；
补齐测试前提后重跑，生产实现未因此改变。首次core编译同时报告既有
`legacy_battle_outcome_resolution_test.cpp:133`的u16到u8窄化警告，未修改该文件。
最终仓库脚本执行`openswd3_battle_legacy_battle_setup_tests`及
`^battle\.legacy_battle_setup$`：core为1/1通过、15.18秒，ASan为1/1通过、18.98秒。
SDL实际重新链接，七文件身份一致。最终core与ASan仍报告上述既有窄化警告；SDL无警告。
日志前缀为`build/tmp/runtime/startup-drawing-suppression-`。
此前七处父链写入见队员、目标及敌方绘制抑制证据；本次回收最后一处启动转发。
