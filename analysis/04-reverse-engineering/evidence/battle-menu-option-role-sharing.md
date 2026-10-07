# 战斗菜单角色检查的共享参数

状态：核心共享读取已通过定向验证。此批只修正角色检查参数的存储来源；SDL完整菜单、
资格callee及完整战斗帧仍未接通，不能据此宣称实际菜单可用。

## 原指令与读取域

45FE36..45FF38的case1先检查当前角色、严格面板矩形、选项范围和许可字节。
45FED1机器码位移为`CA E5 4F 00`，读取`4FE5CA + selected*2`的WORD，
零扩展后交给482FC0。不能改读看似相邻的4FE5CC三项行动列表。

矩形与整数除法产生八项索引。索引0..3不调用角色检查；索引4..7读取：

- 4：4FE5D2/D3，已有重映射间隙的两个字节。
- 5：4FE5D4 DWORD低WORD。
- 6：4FE5D4 DWORD高WORD。
- 7：4FE5D8 DWORD低WORD。

资格返回零直接结束；非零后45FF00重新读取许可字节。许可已被清除时也不
发布选择。通过后才把索引加一、按变化播放选择音效并发布原状态。

451B60..65清零4FE5D4起的十个DWORD；451B7A和451B85另清CC/D0，
不清D2/D3。全局重置45B780..85同样清十个DWORD。入战不能把间隙的旧值
当作总是零，也不能保留该DWORD数组的旧副本。

## 唯一存储

删除无生产写入的`LegacyBattleFrameInputResolutionState::option_role_ids`。
索引4直接取`LegacyBattleTargetSelectionRuntimeState::action_remap_gap`；
其余三项直接取`LegacyBattleStartupResetBlocks::block_4fe5d4`。
输入端口原已继承目标选择运行时端口，不增加状态副本或新注入参数。

目标选择刷新、菜单收尾和入战重置已有的写入，现在可由核心鼠标检查直接
读取。核心帧协调器沿原端口转发校验参数，无需复制数组。

SDL目前在非空角色的case1入口45FE44前停止，尚未执行此次修正的读取。
本批不删除这个停止，也不以测试端口的资格结果冒充实际482FC0接线。
其余SDL整体重置与脚本文字问题另行处理，不混入本批。

## 验证

旧实现的定向回归出现9项预期失败：索引4的三个阶段，以及索引5..7的
重置前两阶段。失败均为实际共享值未传给资格检查。

新增向量覆盖四个读取位置、8000/FFFF零扩展、直接修改后的再次读取，
以及入战重置后的DWORD清零与间隙保留。许可、资格返回和回调撤销许可的
组合检查停止与选择发布顺序。

另逐项验证索引0..3不调用资格检查。最终core/ASan setup各1/1通过，
用时5.12秒/7.87秒；SDL构建通过。日志位于`build/tmp/runtime/`，前缀
`battle-option-role-sharing-final-`（测试）与`battle-option-role-sharing-sdl`
（构建）。SDL构建后仅增加测试断言，没有再修改生产源码。

首次core/ASan重编译各有一处既有测试的窄化警告，位于
`tests/unit/battle/legacy_battle_outcome_resolution_test.cpp:137`；
本批未修改该文件。最终增量测试日志无新warning，不表示该既有警告已消除。

审查按原读取域核对索引4..7、字节拼接、WORD零扩展和调用顺序；原来的
数组越界检查由前置selected<8条件保证，不增加可达成功或停止分支。
没有运行游戏或新增原版动态差分。B10仍315/422，316保持pending_audit。
