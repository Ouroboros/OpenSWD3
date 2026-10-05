# 战斗共享选择等待状态 `0x0053BFC0`

状态：单地址状态统一已实现并通过Linux完整验证。本REVIEW只统一此dword的存储与既有读写方，不代表完整战斗可运行，不推进Workpack 316/317。

## 权威访问范围

完整LST中有74处实际指令引用，分布在17个函数。导航摘录位于`build/tmp/runtime/shared-selection-gate-lst-references.tsv`；地址与指令须逐项核对，不能只根据现有字段名替换。

- `0x004527E0`：2处，视觉过渡开始置1、结束清零。
- `0x00453200`：2处，帧入口读门；门精确等于1且无优先角色时清零，门为零才允许延迟选择出队。
- `0x004539B0`：8处，动作提交和结束路径。
- `0x00455D60`：3处，对方动作处理。
- `0x00456680`：11处，组A角色逐帧读写；`0x004566EC`读门并要求零才进入AI。
- `0x004576A0`：7处，组B角色逐帧读写。
- `0x0045AA00`：3处，最终角色步进。
- `0x0045B630`：1处，全局重置`0x0045BC51`清零。
- `0x0045D8F0`：2处，调试快捷键清零或置1。
- `0x0045DEE0`：1处，调试文字读取当前值。
- `0x0045EA80`：1处，撤退成功在`0x0045EAC8`置1；失败和警告分支保留原值。
- `0x0045F2A0`：1处，输入阶段置1。
- `0x00461C10`：6处，菜单取消清零。
- `0x00462740`：4处，目标刷新清零。
- `0x00464270`：7处，选择绘制中的替换与选择状态写入。
- `0x00466F70`：5处，结果消息相关读写。
- `0x00469D20`：10处，脚本驱动的战斗阶段读写。

## 已确认的重复建模

- `LegacyBattleActionDispatchState::action_pending_aux`：组A在`0x004566EC`读取；组B和双方动作分派也使用此字段。
- `LegacyBattleFinalActorStepState::frame_gate_b`：帧协调、最终角色、输入、菜单取消和目标刷新使用此字段。
- `LegacyBattleInputDispatchState::selection_cache_gate_a`：选择绘制、结果消息和脚本使用此字段。
- `LegacyBattleRetreatCommitState::completion_gate_a`：撤退成功在`0x0045EAC8`写入此字段。
- `LegacyBattleTransitionState::active`：视觉过渡在`0x00452806`置1、`0x00452E5A`清零；不是帧协调器的独立活动返回值。

上阶段将菜单取消和目标刷新接到帧协调器当前读取的存储，只修复其十处写入；本轮范围覆盖上述五份存储的统一。不得同步副本，也不得用默认零值掩盖断开的调用链。

## 存储与调用合同

保留已有动作状态的`action_pending_aux`作为唯一存储，其地址语义补到声明。其余四字段删除；所有调用方借用现有动作状态。撤退叶函数和视觉过渡不拥有整份动作状态，必须显式借用该dword。全局重置只清唯一存储一次；调试快捷键不再同时写两份状态。

此选择依据已存在的组A、组B及双方动作分派数据流；不新增全局、port默认副本、兼容镜像或无来源初值。其他地址与整个动作状态结构的后续接线不在本REVIEW修改范围。

## 汇编独立验证要求

- 门值0、1、2及无优先角色组合，验证帧入口精确比较与延迟出队。
- 菜单取消后帧读到零；选择绘制和撤退提交后后继读到1；原未写路径保留非默认入口值。
- 原typed-stop之前未到达的写入不得发生；此前已到达的写入不得撤销。
- 脚本、消息、双方角色更新、最终角色及调试显示读取同一状态；旧成员删除后所有既有调用与测试必须重新编译。
- 全局重置将非默认门清零，且没有第二份可独立修改的存储。
- 最终源码通过定向、Linux core、ASan、Linux app完整门禁；再完整审查生产、测试、证据与PLAN差异，立即提交推送。

## 实现与调用映射

四个重复成员已删除。帧协调、最终角色、输入、菜单、目标、选择绘制、消息、脚本及调试直接借用现有动作状态；组A、组B和双方动作分派原本已经使用该存储。全局重置保留`actor_frames.shared.action.action_pending_aux`的一次清零，删除其他三次副本清零；调试快捷键删除向actor-frame状态同步第二次写入的代码。

撤退绑定增加必填`selection_gate`引用，唯一生产caller在动作3分支传入`state.action_pending_aux`。视觉过渡参数增加必填dword引用，带真实角色帧的测试借用`actor_frames.state.shared.action.action_pending_aux`；只验证转场前缀的测试提供独立测试存储并预置9。当前`run_legacy_battle_transition`尚无直接生产caller，SDL/脚本的visual-transition端口接线仍待后续完成；本次没有伪造该绑定。

各原函数的逻辑分支、写入值、异常停点和写入相对顺序保持原位，只改变所借用的存储。门0/1/2与优先角色存在/不存在六组向量依据`0x00453297..0x004532D3`导出；菜单取消→下一帧、动作caller→撤退、组A→最终角色、转场→真实角色帧等既有组合现在共用该dword。撤退警告和未就绪早退用非默认值9检查不写入。

## 验证断点与未决项

首次定向`battle.legacy_battle_setup`通过，日志`build/tmp/runtime/battle-shared-selection-single-owner-directed.log`。完整core首次204/205，唯一失败是组A行动测试错误地要求函数最终返回时门仍为零。LST明确在`0x00457579`清零后，经`0x0045766F`调用最终角色处理；此样本命中`0x0045AC14`再次置1并发布message 103。测试现检查移除数1、message 103与最终门1，保留此前随机和调用次数断言；没有改变生产代码来迎合旧断言。失败日志保留为`battle-shared-selection-single-owner-core-red.log`。重新运行的`./build.sh core --test`、`./build-asan.sh --test`、`./build.sh app --test`分别205/205、205/205、211/211通过；三个日志为`build/tmp/runtime/battle-shared-selection-single-owner-{core,asan,app}.log`，无编译warning/error。最终生产与测试diff已完整审查，并以字节比较确认与审查版本一致。本轮未执行Windows门禁或游戏EXE，不升级原版动态差分状态。

本轮同时观察到相邻`0x0053BFC4`在最终角色、结果协调和菜单等状态中仍有重复建模；不在此单地址REVIEW内改写。完整战斗的其他共享状态、SDL绑定与实机续玩仍未验收。Workpack 316保持`pending_audit`、315/422。
