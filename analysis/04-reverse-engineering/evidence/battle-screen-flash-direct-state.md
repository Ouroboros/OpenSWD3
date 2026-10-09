# 战斗闪光直接共享状态

## 本批边界

删除`LegacyBattleScreenFlashStatePort`、其私有状态与全部继承关系。
`LegacyBattleStartupState.screen_flash`持有开关和强度；动作上下文必须
显式借用闪光引用，不通过可空startup指针取得它。脚本、效果协调器、
核心帧、转场和全局复位直接借用既有startup中的同一对象。
动作及敌方清屏同时删除仅用于取得闪光状态的Port实参。

未增加转发接口、复制回写、默认成功或新的失败条件。
SDL已有长期存活的battle_runtime及脚本startup绑定承载新字段；
本批未证明完整战斗流程在SDL中可达，也未启动游戏。

## LST与顺序

28处`53BFCC`访问及强度访问范围沿用
[battle-frame-effect-00453580.md](battle-frame-effect-00453580.md)第18节。
本批重新核对LST访问清单及`453716..453793`颜色消费指令。

- 触发开关保持DWORD，仅精确1进入闪光；强度保持BYTE，初值16。
- 三通道颜色操作后重新读取强度，按BYTE加FC；零时恢复16并清触发。
- 保留清屏前写入及容量失败后的已写像素、闪光和角色选择状态。
- 脚本10触发后的返回或帧失败不清除已消费的闪光状态。
- 全局复位维持强度恢复与开关清零顺序；画布释放失败不执行复位后缀。
- 动作405的53BF94保持独立，不改成闪光开关。

测试观察改为实际持有者；两个效果测试显式保留startup，避免调用结束后
无法观察临时状态。原实际像素、跨帧衰减、抑制和失败前缀断言保留。

## 验证

- core与ASan：`battle.legacy_battle_setup`各1/1通过。
- core与ASan：`battle.actor_frame_316`各1/1通过，覆盖动作及组帧调用方。
- SDL应用目标编译链接通过，未运行游戏。
- include/src/tests中旧闪光状态Port及getter零匹配；完整差异逐项复核。

日志位于`build/tmp/runtime/`：
`screen-flash-shared-final-{core,asan,sdl}.log`及
`screen-flash-shared-actor-{core,asan}.log`。
初轮编译失败保留于`screen-flash-direct-core.log`：两处测试缺少持久
startup变量，已修正；差异审查另修正一处测试的指针访问。
setup构建保留既有outcome-resolution第133行整数窄化警告。

本记录只证明闪光状态访问层已移除，不代表相邻通用调用或全项目迁移完成。
