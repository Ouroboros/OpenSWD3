# 战斗选择等待与结果判定标志 `0x0053BFC4` / `0x0053BF5C`

状态：核心REVIEW已收敛，待阶段发布。这是B11续玩接线前的独立REVIEW。生产字段和测试绑定已迁移；全局重置、调试快捷键与选择绘制夹具借用角色帧持有的动作对象，相关core与ASan定向测试均1/1通过。随后逐指令复核修正选择绘制的消息读取顺序，同一core目标复验1/1通过。核心caller、63处地址访问、生产与测试差异已复核；SDL接线不在本阶段完成结论内。按用户要求，UT仅按影响范围分批执行，不重复无必要的全量验证。前一选择等待状态修正已独立发布；本REVIEW不推进Workpack 316/317。

## 范围与原版区别

LST中`0x0053BFC4`有42处实际指令访问，分布于12个函数：组A帧10、组B帧1、最终角色3、全局重置1、调试快捷键2、调试显示1、撤退1、输入1、菜单取消6、目标刷新4、选择绘制7、消息5。导航为`build/tmp/runtime/shared-selection-cache-b-lst-accesses.tsv`。

`0x0053BF5C`另有21处实际指令访问；导航为`build/tmp/runtime/battle-result-latch-lst-references.txt`。两个地址是独立dword，不可共用同一变量。

- 组A在`0x004566F9..0x00456700`读取BFC4，只在完整值为零时继续AI入口；此前独立检查BFC0也为零。
- 组B在`0x004576EB`读取BFC4，控制对方角色更新。
- 结果判定`0x0045E5B4`和`0x0045E62E`写BF5C为1；没有读取或写入BFC4。
- 调试取消`0x0045DBBE/0x0045DBC4`分别清BFC0/BFC4，不写BF5C。
- 调试结果路径`0x0045DD52/0x0045DD57`先置BFC0/BFC4为1；稍后`0x0045DD97`才清BF5C。
- 组B两条早退路径在`0x0045776D/0x004577AE`清BF5C；邻接的BF74、BF68和BFC0是另外的写入，不可按字段名合并。

## 重审发现的旧映射

组A、组B入口把BFC4误读成`port.outcome_resolution_state().resolution_latch`。组A相应写入和调试快捷键也使用此结果状态；结果判定本体则确实把同一成员当作BF5C。

BFC4同时还有`LegacyBattleFinalActorStepState::frame_gate_a`、`LegacyBattleInputDispatchState::selection_cache_gate_b`和`LegacyBattleRetreatCommitState::completion_gate_b`三个副本。只删除其中副本，仍会保留跨地址误绑定。

BF5C的其他已见映射也需要逐指令核对：选择绘制的`display_gate`、组A的`selection_aux_gate`、组B早退对`shared.selection_mode`的写入，以及目标刷新入口使用的`bindings.target_ready_gate`。后两项的同名状态在其他函数可能对应不同地址，禁止全仓机械替换。

原组A、组B证据中“第二pending与结果判定共用唯一状态”的结论已被上述LST反例否定，本轮须同步修正。此前单元测试通过不证明该所有权结论成立。

## 存储设计

两类状态都由既有动作状态持有，但保持两个独立dword：把input的`selection_cache_gate_b`迁入动作状态作为BFC4存储，把结果状态的`resolution_latch`迁入同一动作状态作为BF5C存储。删除各自原字段及同址副本；所有相关核心caller已经借用动作状态，撤退叶函数继续显式借用dword引用。结果判定的动作借用由只读改为可写，用来发布原BF5C写入。无需新增port默认状态或同步镜像。

组B早退只替换那两处BF5C写入，不删除或全局替换`shared.selection_mode`；它在组A中承担其他地址语义。目标entry/refresh直接借用动作中的BF5C，不再从另一个target-ready引用读取该地址。组A的公共清门helper须逐调用点展开核对，避免将已在前缀清过的BF5C再写一次。

## 逐指令映射

以下地址均来自权威LST实际指令，原始导航上下文在`build/tmp/runtime/battle-selection-wait-result-context.lst`。这里仅核对本REVIEW的两个dword，不把整函数其余分支升级为已完成。

BFC4的42处访问均映射到动作状态的`selection_cache_gate_b`：

- `legacy_battle_group_a_frame.cpp`：`004566F9`读完整dword并比较零；`00456BFC/00456D0C/00456D99/00456E7A/00457126/0045749E/0045757F`清零；`004573B4/004575A8`置1。展开原清门helper，使BF5C、BFC4、BFC0与相邻状态保持各路径原顺序。
- `legacy_battle_group_b_frame.cpp`：`004576EB`读完整dword并比较零。
- `legacy_battle_final_actor_step.cpp`：`0045AB69`清零；`0045AC19/0045AD84`置1。组A结束先写BFC0再写BFC4，组B结束反向写入，保持两处不同顺序。
- `legacy_battle_global_reset.cpp`：`0045BC57`清零；typed状态只在角色帧持有的动作对象上清一次。
- `legacy_battle_debug_hotkeys.cpp`：`0045DBC4`清零；`0045DD57`置1。
- `legacy_battle_debug_overlay.cpp`：`0045E3CD`读取并作为格式串的第三个整数参数。
- `legacy_battle_retreat_commit.cpp`：`0045EAD2`置1，使用动作3caller显式借用的dword引用。
- `legacy_battle_input_dispatch.cpp`：`0045F7AB`置1。
- `legacy_battle_menu_input_finalize.cpp`：`00461C5F/00461D2A/00461EF1/00461F29/00461F93/0046200F`清零。消息7先清BFC0再清BFC4；其他路径保持原各自顺序。
- `legacy_battle_target_selection_refresh.cpp`：`0046306F/0046336E/00463A89/00463DE5`清零。`00463367`写角色记录后才执行`0046336E`；本轮修正了`commit_actor_action_common`原先提前清BFC4的顺序。`00463A82/00463A89`同样先写记录再清门。
- `legacy_battle_selection_frame.cpp`：`004642E1`清零；`0046467D/00464700/004647A1/004647D8/00464C21/00464C5C`置1。通用发布先BFC4后BFC0，撤退确认`00464C1B/00464C21`先BFC0后BFC4。
- `legacy_battle_message_phase.cpp`：`00466FAB/0046724E`清零；`00467319/00467642/0046769A`置1。

BF5C的21处访问均映射到动作状态的`resolution_latch`：

- `legacy_battle_group_a_frame.cpp`：`00456BD6/00456C21/00456D06/00456D93/00456E74/004574E0/004575AE`清零。目标分支前缀已经清过的BF5C不在后续公共清门处重复写入。
- `legacy_battle_group_b_frame.cpp`：`0045776D/004577AE`清零；两条早退不写BFC4，也不写另一个地址对应的`selection_mode`。
- `legacy_battle_global_reset.cpp`：`0045BB74`清零；typed状态在动作对象上单次清零。
- `legacy_battle_debug_hotkeys.cpp`：`0045DD97`清零；取消路径不写BF5C。
- `legacy_battle_outcome_resolution.cpp`：`0045E5B4/0045E62E`置1，写入后才判断变暗门；不写两个等待状态。
- `legacy_battle_retreat_commit.cpp`：`0045EADC`在两个等待置1后清零，使用caller传入的引用。
- `legacy_battle_target_selection_entry.cpp`：`0046218A`读取，完整值等于1才进入目标处理分支。
- `legacy_battle_target_selection_refresh.cpp`：`00462743`读取并与1比较；`00463DEB/00463F12/00463F4F`清零。
- `legacy_battle_selection_frame.cpp`：`00464415`先读取消息至EAX，`0046441A`再置BF5C为1，然后递减EAX进行分支选择；本轮最终复核修正了原先先写BF5C再读消息的顺序。
- `legacy_battle_message_phase.cpp`：`0046720D`清零。

## 实现约束与验证要求

- 完成两个地址全部63处访问的双向映射后再修改生产代码。
- BFC4在选择处理、角色帧、最终角色、调试及撤退之间只保留一份存储；BF5C独立保存结果/选择标志，不能再承接BFC4写入。
- 从真实调用链选择既有owner并回收caller；禁止增加同步副本、默认port状态或静默成功。
- 保留原读取顺序、完整dword比较、0/1写入及typed-stop前缀；不能借状态修正改变其他地址行为。
- 独立测试组合BFC4的0/1/2与BF5C的不同非默认值，证明AI/角色更新由原地址控制，结果标志不被误写。
- 覆盖菜单→角色更新、最终角色→下一帧、调试取消/结果、撤退与结果判定的真实组合。
- 按改动影响完成相关战斗目标的定向验证及适用的ASan验证，完成源码、测试、证据和PLAN的完整差异审查后，立即提交、推送和TG。全量UT仅在确有必要时执行。

调试显示的`0x0045E3CD..0x0045E3EB`先将BFC4压栈，再压BFC0和消息值；因此`MS:%d Stop:%d mStop%d`的实参顺序必须是消息、BFC0、BFC4。原C++与旧测试将后两项颠倒，本轮同步修正，并用BFC0=3、BFC4=2验证显示为`MS:6 Stop:3 mStop2`。

## 本轮验证记录

首次定向日志：`build/tmp/runtime/battle-wait-result-directed.log`。编译通过；`battle.actor_frame_316`通过，`battle.legacy_battle_setup`失败。失败位于消息104的目标选择停止前缀夹具，仍通过旧的`target_ready_gate`赋值触发目标入口。LST的`0x0046218A`实际比较BF5C；夹具现改为设置动作的`resolution_latch=1`，并把另一状态预置9、检查停止后保持9。未根据旧断言恢复错误的生产绑定。

新增组A/组B入口向量分别交叉组合BFC4的0/1/2与BF5C的0/7/9，检查`0x0047DAD0`调用只由等待门决定，BF5C保持输入值。`build/tmp/runtime/battle-wait-result-independent-vectors.log`中消息测试已通过，组A新增向量有6次失败；拆分断言的`battle-wait-result-group-a-diagnostic.log`确认入口调用、完成状态与BF5C均符合预期，只有等待值保持断言失败。

原因是组A夹具继续调用最终角色，移除计数达到剩余角色阈值后，LST的`0x0045AC14/0x0045AC19`重新将BFC0/BFC4置1，并在`0x0045AC1E`发布消息103。已将后置断言修正为两个等待均为1、移除计数为1和消息103，不改生产写入。组B向量仍检查两个状态保持各自输入值。修正后的定向日志为`build/tmp/runtime/battle-wait-result-directed-corrected.log`，受管进程退出0，两项定向测试均通过。随后完整Linux core 205/205、ASan 205/205和app 211/211通过，对应`battle-wait-result-{core,asan,app}.log`。这些结果对应新增帧协调跨调用用例之前的版本，不能作为当前测试改动后的最终门禁。

调用方复核发现帧协调测试`Fixture`仍分别持有`action_dispatch`和`actor_frame_state.shared.action`：协调入口绑定前者，角色帧序列读取后者。`shared_action_dispatch`指针只供部分嵌套调用使用，不会替换角色帧的内嵌动作状态。夹具已改为直接引用`actor_frame_state.shared.action`，不再持有第二份动作状态。菜单取消用例预置BFC4=2与BF5C=9，检查取消仅清等待，再让帧协调器真实调用组B角色更新。新增路径最初因敌人坐标为零，排序扫描不到非零候选而停止；补入非零坐标后，组B调用计数和`0x0047DAD0`更新计数均符合预期，后续组B路径仍有typed-stop。已核对后续停止为`group_b_opponent_mode_typed_stop`：夹具明确未绑定敌人资源，正常执行角色更新后在资源读取处停止。用例保留该停止，并检查帧完成和绘制尚未执行；此跨调用前缀验证已通过新增定向运行。不能拿先前默认零角色的空序列成功证明该链路，也不能把本用例计作完整战斗。诊断日志为`battle-wait-result-cross-call*.log`。后续所有权检查另发现全局重置、调试快捷键和选择绘制夹具同时分配独立动作对象与角色帧动作对象。现已统一为借用`actor_frames.shared.action`，且角色帧声明在引用之前；这三个源文件均属于`battle.legacy_battle_setup`，本次只重验该目标及其ASan配置。完整SDL生产绑定仍属于后续未完成范围。

`battle-wait-result-expanded-directed.log`中角色帧、跨调用前缀、调试取消和两条早退验证通过；结果判定新增向量的三个组B分支失败。LST `0045E60F/0045E615/0045E61B`取低字节减去第2字节，原向量误把2写入高部，现改为低字节2。最终角色→下一次角色更新用例明确模拟`00453297`只清第一等待，验证最终角色发布的第二等待仍抑制AI，同时保持BF5C原值。结果判定与角色等待保持独立的修正已进入定向复验。原先包含全量测试的受管任务已停止，不能记为当前版本的完整门禁通过；剩余夹具修正单独进行定向验证，日志为`battle-shared-fixture-core.log`与`battle-shared-fixture-asan.log`。

`battle-wait-result-final-directed.log`确认两项目标2/2通过。随后三处夹具统一借用，`battle-shared-fixture-core.log`与`battle-shared-fixture-asan.log`分别确认`battle.legacy_battle_setup`1/1通过。最终生产差异799行及两个地址的922行LST导航上下文均已完整阅读；发现并修正上述`00464415/0046441A`顺序差异，`battle-selection-order-core.log`确认同一目标1/1通过。ASan记录对应此两行顺序修正之前，不冒称当前快照的ASan结果。最后两行仅调整两个独立dword的读取与写入顺序，不改变存储、索引或生命周期，故此处仅重跑受影响的core目标。三处夹具差异348行、其余测试差异779行均已完整审查。LST导航中的867行原文已与权威文件逐行比较，零差异；其余行为映射结合指令与C++人工核对，不能由该文本一致性检查代替。阶段发布材料正在收尾。

完整SDL战斗输入、角色/绘制循环、结束返回世界与实机续玩仍待实现和验收。此REVIEW不替代B11或全项目最终完成条件。
