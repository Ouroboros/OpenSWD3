# Workpack 316 执行计划：战斗角色逐帧动作 `sub_479850`

状态：执行计划，尚未验收。上位队列与关闭判据以
[`execution-plan-pi.md`](execution-plan-pi.md) 为准；本文件只规定当前工作包的
**执行顺序、每步产物与阻塞处理**，不建立第二套完成标准。
当前 inventory 为 `pending_audit`，已关闭 `315/422`，不得开始317。

进度记号：`[x]` 已完成且附限定证据；`[>]` 正在执行但未通过；
`[ ]` 未完成。**P0–P5 验证通过 0/6**，不把阶段数伪作工作量百分比。

## 0. 已完成的限定工作与执行纪律

- [x] LST主体、四处物理CALL和249块／98 CALL／22 RET的物理索引已锁定；
  只证明导航与机器字节，不证明全部语义。
- [x] 四处调用的合成父快照／无物理owner首读停点有受控测试；
  不等于四处真实生产绑定。
- [x] v3/v4/v5原版证据各自归档；v5的527次索引0默认返回
  只覆盖同一个run，不覆盖动作7与非默认路径。
- [ ] 四处生产绑定、深层语义、同次未命中路径、I5及最终REVIEW
  仍未完成；下面的P0–P5全部保留，不因以上局部成果勾选。

- 目标为 `0x00479850`，物理 caller 为 `0x004554F6`、`0x0045650F`、
  `0x0045AA33`、`0x0045ACBF`。机器指令以 `swd3.exe_export_for_ai/swd3.exe.lst`
  为准；现有[目标证据](../analysis/04-reverse-engineering/evidence/battle-actor-frame-presentation-00479850.md)
  和 `build/workpack316/` 台账仅供定位未决项，不把机械通过数当成功能进度。
- 原 `src/platform/sdl3/main.cpp` 的 `LegacyBattleScriptDispatchCall::frame`
  固定返回 `EAX=1`；现已改为缺生产帧端口时显式 `frame_typed_stop`，
  并使脚本各帧调用方在停点保留前缀、不执行正常后缀。SDL 收到该停点
  后记错并停止运行，避免下一帧重入；父战斗帧另以无返回值显式识别
  此停点，不再执行当次脚本正常RET后的音频尾调用。其他脚本
  typed-stop也不再被SDL映射成普通EAX=1；战斗资源或setup失败
  也不再被映射成普通EAX=0返世界。case 3／25／42 的
  脚本端口定向测试和父帧停点单测通过；音乐音量分立后定向app 4/4
  通过；音乐抑制同址修正后定向core 1/1、完整core／ASan各200/200、
  完整app206/206通过。**这些并非从实际 SDL 回调进入的测试，也不计
  最终316门禁**。LST `0x00425048–0x00425062`先将6写入
  `dword_4AB784`及独立的`dword_4C9A0C`；后者是音乐音量，
  `0x0045322B`在音乐gate为1时读取。SDL世界音乐原先借用
  `spatial_audio.mix_level`（即前一个sample音量owner）；现在分立
  `music_mix_level_`并以原版初值6驱动世界音乐。协调器已要求调用方
  引用该音乐音量owner，且直接借用脚本共享的`0x300`字节音乐路径缓冲区；
  SDL的`frame`回调已静态绑定这两个引用并执行活动／音乐前缀，又复用
  `sub_45FC60`鼠标首门及对话热点前缀；坐标变化或旧门非零时借用
  `world_dialogs_.messages`中同一选择链的临时热点视图，在前帧坐标与
  命中选择副作用之后，case 0读取真实消息和actor metrics并执行
  首三个判断；可确定零返回时在下一CALL
  `0x0045323E → sub_45F2A0`前停点，正数角色映射门在`0x0045FD49`
  前停点；消息值大于30直接默认零返回并停在下一CALL前，1至30的
  非零消息在已读状态后的`0x0045FCEF`比较前停点。同坐标且旧门为零也
  在下一CALL前停点，均不伪造帧返回。尚无实际SDL回调轨迹或完整帧输入和角色帧接线，
  不计P0完成。首门修正后定向core 1/1、完整Linux core／ASan各
  200/200及app206/206通过；热点前缀新增定向core 1/1、完整Linux
  core／ASan各200/200及app206/206通过，均不是实际SDL帧调用验证。首次完整ASan曾在测试
  函数栈溢出，隔离脚本测试调用帧后完整重跑通过。
  LST `0x00453218`读的`byte_53C4C0`在消息阶段`0x00467031`
  写1，重置`0x0045BB4B`写0；帧协调器现直接借用帧输入状态中的
  同一byte，不再保有独立副本。定向core 1/1验证抑制=1时不执行
  音乐启动或commit；尚未测试实际SDL回调。Core新增
  `LegacyTswFramePieceProvider`：借用同一TSW runtime、以原版低16位键
  加载帧，保留缓存逐出后的宿主像素租约；noexcept入口的宿主异常
  转为失败并显式留痕。合成逐出、宿主失败及真实TSW资产对照的
  `asset_runtime.legacy_tsw_runtime`定向1/1、该提供者改动后且共享音乐路径
  接口调整前的完整Linux core／ASan各200/200及app206/206通过。
  此提供者尚未绑定SDL帧
  回调，不证明原版释放后guest指针可读，也不计P0交付。
  生产帧协调器尚无`LegacyBattleFrameZeroContext`所需帧绘制状态，
  以及 `LegacyBattleFrameCoordinatorPort`、
  `LegacyBattleActorFrameAdvanceContext` 的实际绑定；四处调用的生产父快照
  仍缺失。既有本体局部测试及 v3/v4/v5 原版受限证据不能替代该接线。
- 开工时核对 `git status`，保留现有 v5 只读审计脚本、报告和动作7证据的
  未提交修改；不覆盖、丢弃或混入无关文件。一个最终 REVIEW 单元不变。
  仅文档、单个校验器、未验证的 helper 或检查点不得单独提交。
- **每一阶段都先明确可运行入口和预期可观察结果，然后改生产代码、补
  定向测试、运行验证，最后只记录实际结果。** 原版未观察值不得由本地
  夹具推断。发现未知父现场或深层回包时，在物理边界显式停止，不返回
  伪造成功值；说明确切缺项后继续其他不依赖该缺项的本地工作。

## 1. [>] P0 · 接通战斗帧的生产入口（进行中，0/3交付项）

入口：`SdlSmokeIdlePorts::step_battle()` → `invoke_battle_script(frame)` →
战斗帧协调器 → 角色帧序列。相关现存位置：
`src/platform/sdl3/main.cpp`、
`include/openswd3/battle/legacy_battle_frame_coordinator.hpp`、
`src/battle/legacy_battle_frame_coordinator.cpp`、
`src/battle/legacy_battle_actor_frame_sequence.cpp`。

1. [>] 从现有战斗启动、动作更新、TSW 帧租约、渲染及输入状态中逐一确认
   `LegacyBattleFrameCoordinatorContext`、`LegacyBattleFrameCoordinatorPort`
   和 `LegacyBattleActorFrameAdvanceContext` 的实际 owner。只登记接入
   所需依赖；不重新启动全模块调研，不用合成栈地址或测试 owner 冒充生产值。
   源码中`0x0045320D → sub_485830`音乐查询已由SDL流管理器执行：
   帧回调共用协调器音乐前缀，gate=1且抑制byte=0时借用脚本路径播放并
   按有符号音乐音量提交；其后的`0x00453239 → sub_45FC60`首门
   读取真实归一化鼠标与共享帧输入状态。LST在`0x0045FCAD`先读取
   热点链头，`0x0045FCB2/BA`才写前帧坐标；世界对话热点证据将
   同一链的现代owner锚到`world_dialogs_.messages[].choices`，SDL
   复用世界帧现有的临时视图生成顺序，不创建持久副本。非早退路径
   执行热点查询及选择发布；case 0继续借用真实消息状态、actor metrics、
   最终角色排除数与输入选择字执行`0x0045FD07..44`计数／Y／组A数量门。
   继续复用启动时的十项角色来源视图与八项水平偏移（来源8..17
   物理同址借用来源视图，来源18起未绑定邻接byte表则typed-stop），按原版顺序扫描、
   选择并保留EAX／ECX／EDX；可证明零或一返回进入下一CALL
   `0x0045323E → sub_45F2A0`入口前缀，越过来源视图或偏移视图
   时分别在`0x0045FD49/4B`前typed-stop；消息6、7、9..26、28、29
   及大于30均经原版跳表默认零返回、进入上层下一CALL入口前缀；
   非默认消息中，case 1空角色、case 3三重抑制，以及case 5/8
   横向未命中时按原版零返回；其余未绑定分支分别停在相应首读／行扫描处，
   case 2/4先按LST把各自悬停项清为全1，然后分别在
   `0x0045FFD9/0x00460124`垂直分支前停点；case 27/30仍在已读状态后的
   `0x0045FCEF`前停点。
   前一热点版本完整Linux core／ASan各200/200及app206/206通过；
   本次case 0提取版定向core 1/1、完整Linux core／ASan各200/200
   及app206/206通过；其后大于30的默认返回和非零消息停点修正
   定向core 1/1、完整Linux core／ASan各200/200及app206/206通过；
   新角色来源前缀补齐物理别名、严格边界及未命中寄存器差异后
   定向core 1/1、完整Linux core／ASan各200/200、app206/206通过；
   新默认消息跳表前缀定向core 1/1、完整Linux core／ASan各
   200/200及app206/206通过，格式与diff检查通过；
   新case 1/3/5/8门控定向core 1/1、完整Linux core／ASan各
   200/200及app206/206通过，格式与diff检查通过；新case 2/4悬停复位
   定向core 1/1、完整Linux core／ASan各200/200及app206/206通过，
   格式与diff检查通过；上述门禁本身未执行实际SDL回调；后续用户开场战斗实测见下，角色帧仍未到达。
   SDL已改用与协调器共用的虚基类帧输入状态
   端口，并在战斗初始化复位同一状态；完整消息阶段与
   `LegacyBattleFrameCoordinatorPort`尚未接线。端口自带的 metric／input／message
   状态不能代替 SDL 已有的 `battle_actor_metrics_`／
   `battle_input_dispatch_`／`battle_message_state_`；须绑定同一 owner。
   上层下一CALL `sub_45F2A0` 在`0x0045F2A0`先读`dword_53C00C`，
   `0x0045F2B2`无条件清`dword_53BD9C`。SDL已借用协调器持有的
   `battle_frame_coordinator_state_.render_abort_latch`及输入状态
   `battle_input_dispatch_.menu_action`执行此入口前缀：latch精确等于1
   时停在下一CALL `0x00453243→sub_45D490`前，其余值按LST
   有符号消息值、排队角色及共享对话链空门；进入键盘分支后
   按原序查询同一SDL键盘快照DIK2..9，按第一个按下的键在对应
   `0x0045F2FE..0x0045F556`变更门前停点；无按键或未进入键盘分支
   时，从真实归一化输入的记录1与共享`input_gate`执行
   `0x0045F5A3..C4`的三门及`0x0053BDA4`按位或，然后在
   `0x0045F5C4..E0`记录9 rapid非零且held有符号正时把
   记录0 rapid置1并复制held；再按`0x0045F5E0..0x0045F629`
   读记录2 rapid／held、比较signed重复门与signed消息门：跳过时在
   `0x0045F629`记录18首读前、消息大于1时在`0x0045FC5B`
   早退前、可执行动作时清选项word和置action kind后在
   `0x0045F61D→sub_462320`前typed-stop。原版`0x0045F602`
   清BP，因此held=16且除3余1的选项也是0而非既有错误的3。
   继续路径执行记录18`0x0045F629..0x0045F672`：rapid为0或
   signed除3余数不为1时停在`0x0045F806`记录17首读前；
   活跃路径先检查真实输入owner的retreat word和action gate，需
   早退时停在`0x0045FC5B`前，其余在尚无SDL owner的
   `0x0045F672`debug actor-retarget gate首读前typed-stop。零rapid
   路径再按LST顺序扫描记录17／14／15／12／1／4／6／3／5／7／8／0，
   首个非零项在其后续指令前停止；已由第二次SDL实际运行证明的
   全零路径可越过`0x0045FC5B`正常RET，按caller原顺序进入
   `0x00453243→sub_45D490`预帧三门：从输入owner的
   `selected_actor_cleanup_gate`借用同一物理`0x0053C018`，
   依次核对terminal=1、active actor非零及message!=3；早退路径
   在下一CALL`0x00453248→sub_45B0E0`按LST顺序清真实metric
   owner的两张18-dword表，再读取同址启动owner的组B数量
   `battle_runtime_.enemy_count`；非零停在首次坐标CALL
   `0x0045B11F→sub_4783B0`前，零停在`0x0045B13E`组A数量读前。
   活跃路径在`0x0045D4C8`首次source actor读前停下。
   其他输入非零分支
   仍未绑定，且不把预帧局部结果冒充整帧回包。
   已修正core中原版`0x0045F67E`先加载message到EAX，故
   消息99/100早退回包须保留该值，而不是held值／商。
   该批定向core 1/1、Linux core／ASan各200/200、Linux／Windows
   app各206/206均已通过；这些门禁不执行实际SDL战斗回调，下面的
   第二次人工运行才覆盖本次新停点。记录9前缀定向core 1/1、
   完整Linux core／ASan各200/200及
   app206/206通过，格式与diff检查通过。SDL typed-stop日志另加
   menu/latch/记录0前后值与键盘查询计数；该日志版app206/206及
   Windows app206/206通过。用户在Windows实际从新游戏开场进入
   战斗的运行日志已原样归档至`build/vm/wp316-sdl-frame-user-run/
   openswd3-2026-10-03_18-15-50-20916.log`，与原日志SHA256均为
   `b25f02a17d29f02ce5d6049c241822a5caeaf33b90bb43c54df5b5ab1f5b36cd`。
   同次日志显示战斗资源id=98、setup=ready，实际脚本帧回调音乐
   启动1次、port_calls=2、keyboard_queries=0、hotspot_queries=0；
   菜单、latch、记录0前后均为0，首个未绑定物理读停在
   `0x0045F5E0`记录2 rapid，随后脚本status=32、opcode=1、
   offset=37并正常停止进程。这是生产回调进入与共享快照证据，
   不是角色帧执行、完整协调器或原版动态差分。第二次用户从新游戏
   开场进入同id=98战斗的实际日志已复制至`build/vm/
   wp316-sdl-frame-user-run-v2/openswd3-2026-10-03_19-14-31-66208.log`，
   源与副本SHA256同为
   `a14973592137527791bd614ee17621be6ddf9080e74de5764df3bef1d92cc7ac`。
   本次record2 rapid／held均0、record18 rapid／held均0，
   `idle_records_inspected=12`、记录0前后均0；第一物理停点已推进至
   `0x0045FC5B`输入分派RET前，随后仍是脚本opcode=1、offset=37、
   status=32的显式typed-stop及进程正常停止。没有实际接通RET后的
   `0x00453243→sub_45D490`或角色帧，不能把12项空闲扫描冒充
   正常返回；P0仍为0/3。此后新增预帧三门只由LST与定向core
   1/1、Linux core／ASan各200/200、Linux／Windows app各206/206
   证明；再新增metric双表清零的定向core 1/1、Linux core／ASan
   各200/200及Linux／Windows app各206/206通过，尚无第三次实际
   SDL回调。随后在SDL帧入口把启动owner的live敌方／玩家侧计数
   映射到metric视图，避免case 0误读默认0并在脚本写后按当帧刷新；
   这不证明补充角色已构造或完整启动已绑定；旧SDL日志的case 0
   默认零计数轨迹不能外推到新程序的分支。最新定向core 1/1、
   Linux core／ASan各200/200和Linux／Windows app各206/206通过；
   随后增加case 0从旧默认零人早退转为实际一敌一人的跨owner
   反例，最新定向core 1/1、Linux core／ASan各200/200、Linux／
   Windows app各206/206通过；这些CTest仍未执行新SDL帧回调。
   又将简化资源setup已算出的四项紧凑来源索引与四对阵型锚点
   写入共享启动owner，修正live人数分支可能误用默认零偏移；
   合成三人及战斗98真实资产单测已补；最新定向core 1/1、
   Linux core／ASan各200/200、Linux／Windows app各206/206通过。
   门禁未执行修正后的实际SDL输入分支。
   另为人工复测新增可选`[dialog] auto_advance`与`interval_ms`：
   SDL仅对无选项的互动对话合成限速单帧空格；默认关闭，手动输入
   仍直通。当时Windows EXE已重编，SHA256为
   `f8e4336db3dca815a9c3f581fb783a7f0eda795cde21b3dd6ddf6c021cf75126`；
   同目录配置显式启用120ms。该修改后的Windows app206/206、
   Linux core／ASan各200/200及Linux app206/206通过，均未执行
   实际SDL对话。后续用户在启用配置下实测自动对话仍无效，进入战斗
   也会自动退出；`build/app/src/platform/sdl3/Debug/logs/
   openswd3-2026-10-04_02-49-35-56760.log`记录了
   `dialog auto-advance enabled: interval_ms=120`，不能归因于配置
   未加载。这是用户观察，尚无修复后的SDL输入链或战斗帧验收。
   接着将`0x0045B0FE`组B计数首读改为真实启动owner
   `battle_runtime_.enemy_count`，而非metric port默认0；其零／非零／
   高位反例加入定向测试；定向core 1/1、Linux core／ASan各
   200/200及Linux／Windows app各206/206通过。下一个生产缺口为
   `0x0045B11F→sub_4783B0`角色坐标owner、两字栈局部地址及
   连续音乐／输入／预帧CALL后的ECX/EDX：SDL音乐端口当前只返回
   EAX，不能把另外两项默认0和虚构栈token传入坐标callee。
   只用部分`prepare_battle_setup`构造启动状态，不能因读取真实敌方
   数量就宣称startup `sub_451B10`、坐标CALLeffect或组A额外成员
   已接通；不以默认0数量跳过角色循环。
   此前记录1前缀和同址合并
   定向core 1/1、完整Linux core／ASan各200/200及app206/206
   通过，格式与diff检查通过。
   键盘查询前缀该版定向core
   1/1、完整Linux core／ASan各200/200及app206/206通过，格式与
   diff检查通过；该门禁未覆盖上述实际回调，且仍未到角色帧。
   分支门该版定向core 1/1、完整Linux core／ASan各200/200及
   app206/206通过，格式与diff检查通过；该latch其他写入生命周期尚未
   闭合。原先重复持有的`battle_target_selection_.candidate_gate_a`
   已去除，目标选择刷新、选择面板和全局重置统一借用
   `battle_input_dispatch_.menu_action`承载`0x0053BD9C`；同址修正
   定向core 1/1、完整Linux core／ASan各200/200及app206/206
   通过，格式与diff检查通过，不以该局部合并推断全帧语义闭合。
   `0x0053BDA4`的`input_latch`与目标刷新先前独立的
   `selection_aux_gate`也已收归同一输入owner：`0x0045F5BE`写位0、
   `0x00462A5B`清全dword；全局重置在LST不写该地址，测试核验
   保持入口值。该次同址合并的定向core 1/1、完整Linux
   core／ASan各200/200及app206/206通过，格式与diff检查通过。
   `advance_legacy_battle_pre_frame()`对应`sub_45D490`，不是此处的
   `sub_45F2A0`。新入口前缀定向core 1/1、完整Linux core／ASan各
   200/200及app206/206通过，格式与diff检查通过；实际SDL运行
   到达记录2首读前的显式停点，不能借默认零越过该停点。
   music gate 等于1时，`byte_53C198` 的现存 owner 为脚本共享状态
   `battle_script_shared_.music_path`（case 60 写入）；LST中该缓冲区
   自`0x0053C198`起占`0x300`字节，先前C++的260字节会对最多255字节
   的脚本文字过早停点，现已修复容量；又按LST修正case 60先复制
   `GetCurrentDirectoryA`的`Buffer`、再追加音乐目录与脚本文字并保留
   终止字节后的旧字节。SDL以配置数据目录代替原版当前目录（平台适配），
   case 60 的停止／启动／音量调用已绑定现有流管理器；短目录、255字节文字、
   长目录首次越界及保留旧尾字节的定向core 1/1、最新源码完整
   Linux core／ASan各200/200和app206/206通过；实际SDL帧日志仅证实该次`music_started=1`，不等于音乐路径全分支已运行。
   协调器上下文直接借用该缓冲区与已从sample音量分离的
   `music_mix_level_`；SDL前缀已借用同一owner，不使用空路径或测试
   音量执行`0x00453226/31`。完整帧上下文仍未接线。协调器已删除
   误当播放句柄的副本，按原始dword位模式提交；定向core 1/1通过。
   新帧提供者仅在core可用，尚未进入该实际SDL调用链。消息读取
   `0x0045FCEA`在case 0短路路径已使用现有`battle_message_state_`
   owner，但其他消息分支、逐帧输入分派和协调器端口均未绑定；
   不能把未绑定回包当成正常返回。
   `LegacyBattleFrameZeroContext`还需帧绘制状态、目标framebuffer、
   光栅／裁剪／共享blit请求和效果、jitter与TSW帧提供者的当次同址引用；
   `LegacyBattleActorFrameAdvanceContext`还需组B帧状态、动作分派端口及
   包含绘制／随机／音效／角色动作真实owner的动作分派上下文。SDL当前
   只有其中一部分已存在的对象，不能以测试夹具或默认构造替代缺失项。
   战斗入场的`release_display_and_world_for_battle_entry()`和
   `close_world_map_view()`仍为空实现。LST故事战入口
   `0x0040A7BD..0x0040A815`直连调用不含热点链释放
   `sub_40DBC0`，该释放在另一分支的`0x0040A678`；
   `sub_451B10`范围也无此直接CALL。这只排除直接清理，尚未排除
   间接callee写同一链；不能凭空在SDL入场清理旧选择。当前是静态缺口，
   尚非实际SDL回调的运行停点。
2. [ ] 写一个从实际 `frame` 回调进入的定向反例：旧实现无条件 `EAX=1`
   但帧协调器未执行。然后接线并消除该反例；缺失必要 owner 时明确
   typed-stop，不允许把端口空回包映射为正常 `EAX=1`。
3. [ ] 阶段交付不是“有接口”：必须有生产入口实际调用轨迹、首次角色帧
   调用或明确物理停点、共享状态前后快照及定向测试的运行结果。
   仅走到协调器入口而未到角色帧时，报告首个未接依赖，不勾选 P0。

## 2. [ ] P1 · 接通最终角色步进的两处调用（0/2生产绑定）

依赖 P0。先 `0x0045AA33` 组A，再 `0x0045ACBF` 组B；实现位置为
`src/battle/legacy_battle_final_actor_step.cpp` 及其真实上游。父调用
现场由真实执行链提供，不以测试注入的 `caller_snapshot` 宣称生产绑定。
当前逐处状态：组A `0x0045AA33` `[ ]`；组B `0x0045ACBF` `[ ]`。

1. [ ] 逐一绑定入站寄存器、已定义 FLAGS、DF、ESP、返回槽、父栈参数，
   以及 actor/action/TSW/raw 图像的同一 owner；核入参读写先后。
2. [ ] 各跑 EAX=0 与 EAX=1 后缀、提前停点与 RET 故障的定向测试；只有
   子函数正常 RET 后才可进入父成功后缀，不跨故障继续执行。
3. [ ] 组A默认支可对照**同一个 v5 run** 已观察的索引0、527次 EAX0、
   角色字节不变；它只证明这一条分支。组B只使用其自身同 run 原版
   切片，不把 v3/v4 的内部回复拼接进 v5。两个调用各自有真实生产
   入口、返回及状态证据后才勾选 P1。

## 3. [ ] P2 · 接通动作7的两处调用及来源（0/2生产绑定）

依赖 P0；两个物理调用为 `0x004554F6` 组B 和 `0x0045650F`
组A。C++ 位置为
`src/battle/legacy_battle_action_dispatch_cases_low.cpp`、
`src/battle/legacy_battle_opponent_action_dispatch.cpp`。
当前逐处状态：组B `0x004554F6` `[ ]`；组A `0x0045650F` `[ ]`。

1. [>] 沿 LST 的动作字段写入链回溯到生产数据源，分清主动作、备用动作
   与絕招列表游标；只使用可证实的触发值，不猜菜单操作。
2. [ ] 从实际父分派生成并绑定两处 caller 现场，测试缺现场继续抛
   `NOTIMPLEMENTED`，有现场则逐一验证 EAX 0/1 的原父后缀、
   栈与共享资源写入。不能以合成快照测试计入生产绑定。
3. [ ] 现有原版 run 均未覆盖这两个父调用；如静态证据和本地实现仍不足以
   完成同次差分，先准备已验证的便携采集工具及**具体可重复触发**步骤，
   再按 `AGENTS.md` 通过 TG 和聊天同步编号操作及回传要求。
   在没有可靠触发方式前不请求盲试、不自行启动原版、不索要 VM 哈希。
   这个动态缺口不阻止 P3 的本地实现。

## 4. [ ] P3 · 沿实际执行路径关闭函数本体与深层调用（整链未收敛）

从 P1/P2 中实际到达的非零门路径开始，先处理动作更新
`0x00479921 → sub_4321E0`、资源查询 `0x00479940 → sub_4315D0`，
再按实际路径处理解码／分配、绘制、音频、释放和效果。每走通一条
路径，立即将新出现的 CALL 与返回点纳入同一验证，不先机械扫完
所有可达子函数后才写代码。

1. [ ] 对每条路径记录输入、比较方向、数据宽度、状态与 owner 写入顺序、
   子调用正常／故障退出；从 LST 独立构造两侧及边界测试，然后修 C++。
2. [ ] 逐项消除 `build/workpack316/call-audit.tsv` 的部分状态和
   `return-audit.tsv` 的深层未决；249 块的局部 `reviewed`、98 个
   CALL 字节正确、22 个 RET 局部栈正确**均不自动算语义收敛**。
   没有可证明的 Win32／CRT 内部指令时，严格限定外部 ABI 和平台适配，
   不伪称可见原版内部状态。
3. [ ] 用真实 ACT/TSW 资产和已有同 run 切片在其各自覆盖范围内复放。
   一处新差异要修代码、测试及证据，并从函数入口重新做双向核对；
   直到目标及四处 caller 在声明范围内没有未解释的可观察差异。

## 5. [ ] P4 · 同次原版证据与战斗生命周期（动作7同次0/2，I5未通过）

- [ ] 若 P2 的两个 caller 已有可重复触发方式，按已准备的人工操作流程
  获取**各自同次**入口、子调用与返回证据；无法取得时保留
  `blocked_runtime_oracle`，不得用静态或跨 run 拼接伪造差分通过。
- [ ] 从真实游戏数据固定输入进入战斗、完成一次战斗并沿原返回路径回到
  世界：保存种子／调用序列、关键状态快照和 framebuffer 哈希，
  实际执行并核对 I5。P0 到达帧入口不等于 I5 通过。
- [ ] 如某条路径仍有真实后端／资源／现场缺口，记录首个停止地址、缺失
  owner 或输入、已经通过的定向测试和下一次可执行动作；不以
  `NOTIMPLEMENTED`、测试夹具或普通返回替代完整生命周期。

## 6. [ ] P5 · 最终 REVIEW、门禁与发布（0/3交付项）

- [ ] 从 `sub_479850` 入口覆盖全部 249 块、98 CALL、22 RET 与四处父
  调用，完成 LST→C++ 和 C++→LST 双向复查；关闭或明确登记每个
  资源所有权、异常和动态证据缺口。最终 REVIEW 作用域不因阶段切片
  缩小。未闭合时仍为 `pending_audit`、315/422，不开始317。
- [ ] 在**最终源码**上运行定向 `battle.actor_frame_316`、仓库脚本
  `./build.sh core --test`、`./build-asan.sh --test`、
  `./build.sh app --test`；到主 PLAN 规定的 Windows 边界再运行
  对应 Windows 完整门禁。旧源码的通过记录不能复用。
- [ ] 审阅完整 staged/unstaged diff，确认无原版输出、构建物或无关
  文件混入。只有满足唯一 REVIEW 的生产代码、四 caller、测试、
  证据和规定门禁，才同步 inventory、`modules/battle.md` 与主 PLAN，
  按仓库 `$commit` 流程提交、推送并发送阶段 TG。

## 7. 进度与工期的记账方式

- 上述状态必须随实际生产代码、测试结果及剩余缺口更新：目前
  P0进行中且交付0/3，P1–P5未完成，整包尚无验证通过的阶段。
  遇实际外部阻塞才在对应项注明事实；机器码数、文档行数和测试
  枚举数不充当进度百分比。
- 第一个工作时段只做 P0，结束前报告实际 diff 与定向结果；若没有
  打通，必须给出阻止到达角色帧的第一个具体接口／owner，而不是转去
  无界审计。此后每个工作时段最多维持一条在做的生产路径；一条路径
  不能完成时说明故障和新的依赖，不悄悄切换题目。
- **总工期暂不能由249块或旧测试数线性推算。** P0实际运行后，以
  P1/P2各 caller 的真实绑定耗时和 P3剩余深层路径重新估时，向用户
  报告估计依据、区间及未确定的原版现场；未测量前不报拍脑袋百分比
  或承诺日期。用户要求停止时立即停止。
