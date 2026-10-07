# 战斗剧情、菜单与行动规则共享

## 范围与依据

本批修正原址 `0x0053BC24` 的多份持有者，并接通 SDL 入战时的
`0x00C9` 剧情查询。完整 LST 的机器码与指令为依据；49 处直接引用
归入 28 组窗口。窗口行号与原指令保存在本批构建目录审计材料中。

不改变镜像来源、队伍扫描顺序、完整帧接线或返回世界流程。
本批不证明完整初始化、原版动态差分或实际续玩通过。

## 单份状态与借用

唯一规则值为 `LegacyBattleDebugHotkeyState::battle_mode_flags_53bc24`。
原有调试、输入、撤退、目标选择、消息及全局重置继续使用此值。
删除以下独立字段：

- `LegacyBattleStartupState::mode_flags`。
- `LegacyBattleScriptSharedState::control_flags`。
- `LegacyBattleActionDispatchState::battle_flags`。
- `LegacyBattleGroupAFrameState::battle_byte_flags`；组 B 原先也借用它。
- `LegacyBattleEffectFrameState::battle_byte_flags`。

启动、脚本、转场、行动摘要和效果端口通过同一虚拟状态端口借用。
行动及角色帧端口沿既有撤退端口继承同一状态。
组 B 的单效果适配器显式转发可写与只读访问，不能使用适配器自身存储。
输入与动作菜单的既有规则引用绑定到该值，不建立复制同步。

## 指令合同

- `451D8A..451DA7`：先查询 C9。完整 EAX 非零才读取规则 DWORD，
  OR AL 的 bit1，再写 DWORD。零不访问规则，也不清除陈旧 bit1。
  查询回调改写规则时，随后读取改写后的值。核心与 SDL 共用此入口。
  SDL 从当前世界剧情状态查询，位置在既有等级上限查询之前。
- `452F08`：低 BYTE bit6 控制转场早退。
  `45307D..453087`：文字调用后重新读取 DWORD，置 AL bit7。
- `4534E3`、`45E95E`：读取 AH bit0，即 `0x100`。
- `453E8B`：读取 AL bit5，即 `0x20`。
- `4551F6`：低 BYTE bit2 控制额外队员计数。
  `4553AE..4553BE`：被调函数返回后重读并清 AL bit2。
- `457261..4572A4`、`458031..458080`：先检查 AL bit7，
  遍历另一组角色后重读完整规则，清 bit7，保留回调写入的其他位。
  零人数仍执行清位；人数循环与其他动作保持原顺序。
- `458BE0..458BF3`：资格返回精确等于 1 才重读规则并置 AL bit5。
- `45B876`：既有全局重置写零 DWORD。本批不新增生命周期清零。
- `45DCA5..45DCC6`：延迟调用后读取规则，两支清除或设置 AL bit1。
- `45E5EF`、`467630`：结果处理和消息 103 读取 AL bit3。
- `45EAB6`、`45F71E`：撤退和输入读取 AH bit1，即 `0x200`。
- `45FA8B..45FA9B`：命中 AL bit5 后重读并清该位。
- `463AD1..463ADB`、`463B65..463B76`：物品调用后重读并置 AL bit2。
- `464EF4`：动作菜单读取 AL bit1；`46965F`：状态面板读取 AL bit5。
- 脚本 57、69、71、77 分别置 `1`、`8`、`0x10`、`0x40`。
  脚本 82 的 WORD 参数精确等于 1 时置 `0x100`，否则清除该位。
  脚本 83 置 `0x200`。以上均保留其余位。

原 BYTE 位修改可表达为同一 DWORD 上的 OR/AND；保留高 24 位。
不把读值移到回调之前，也不把无条件全字段赋值代替局部位修改。
本批不重排脚本游标或邻接字段的既有写入。

## 验证状态

本批定向验证通过；不增加战斗函数关闭数量。

独立向量包含 C9 的零、1、7、全一返回，以及查询后读取、陈旧位与高位保留。
跨端口向量先执行入战查询和脚本 83，再由真实撤退入口读取规则，
验证可撤退角色受到剧情规则阻止，并保留停止前的字段。
效果与双方角色帧向量检查回调改写后读取，以及只修改目标位。

核心 setup 已通过新增向量。首轮角色帧仅组 A 的调用次数预期失败。
诊断确认该夹具的查询顺序为组 A、组 B、组 B、组 A；第三次查询对应
`45727C` 遍历，第四次在清位后。测试已在第三次回调改写规则，
检查总共四次查询及最终 `0xCAFE0205`，临时诊断输出已移除。
最终 core 与 ASan 的 setup、actor_frame_316 各 1/1 通过，
SDL 构建通过。五份日志位于 `build/tmp/runtime/`，文件名为：

- `battle-mode-flags-final-setup-core.log`。
- `battle-mode-flags-final-actors-core.log`。
- `battle-mode-flags-final-setup-asan.log`。
- `battle-mode-flags-final-actors-asan.log`。
- `battle-mode-flags-final-sdl.log`。

两份 setup 构建保留未修改的结局测试第137行窄化警告；
无本批新增编译警告、测试失败或 sanitizer 错误。未启动游戏。
生产差异和测试差异已完整审查，清位的读取均位于原回调之后。
