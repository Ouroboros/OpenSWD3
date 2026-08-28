# 战斗消息阶段分派 `0x00466F70`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`、`caller_reclaimed`。

## 1. 完整权威范围

权威LST主体为`0x00466F70..0x004676BC`，从proc到endp共911行、560条带机器码和真实助记符的实际指令、40个静态call、47个跳转、43个局部标签、1个default标签和21个`retn`，没有外部`FUNCTION CHUNK`。唯一静态caller位于已关闭主帧协调器：HUD之后依次调用选择帧、本函数和后一战斗阶段；本工作包回收第二个后置槽并传播typed-stop。

40个callsite由20处未审战斗调用、一次已关闭过渡控制选择、一次已关闭炼符结果面板、一次已关闭胜利奖励、一次已关闭升级面板、一次已关闭角色升级属性提交、一次已关闭角色成长对照面板、一次已关闭成长标题框、一次已关闭成长完成标题框、一次已关闭成长角色选择、一次已关闭法宝成长结果角色选择、一次已关闭法宝完全成长提示框、一次已关闭战利品清单面板、一次已关闭战败提示面板、三处已关闭目标选择进入、一次已关闭玩家道具数量和三处已关闭sample命令组成。过渡控制选择、炼符结果、胜利奖励、升级面板、角色升级、成长对照、双标题框、两类成长角色选择、法宝完全成长提示框、战利品清单、战败提示、目标选择与玩家道具数量直接组合；音效复用typed接口；其余11类未审业务callee通过窄端口保留，循环和分支造成的20个静态位置均按原时序执行。

## 2. 入口双门与switch域

入口先读取`0x004ACF48`物理链门；非零立即返回该值。链门为零后比较`0x005214F8`首dword；非零同样立即返回，EAX仍为零。两门都为零才读取共享message并按u32减96；结果大于17进入默认返回。

有效消息为96–104和110–113共十三项。105–109与所有域外值不产生副作用。域下方95保留减法回绕EAX，域上方114保留EAX 18。

- 96：依次清输入cache B、选择输入门和message。
- 97：以live组A数量索引32-byte显示槽，按i16 sign-extension传X/Y与对应组A对象；返回精确1才把AL写当前过渡角色byte。
- 98：先置输入cache A，再直连炼符结果面板。面板绘制双层底板，查询精确1时重读live结果byte；精确1显示“煉符成功”和格式化的“得到符咒:%s”，其他值显示“煉符失敗”和“沒有得到東西”。子stop保留cache与画面/文字前缀并阻断主帧后续；旧准备槽reserved且生产零调用。

## 3. 消息99：完整初始化链

入口先清16位阻断word与selected cleanup gate，再置选择抑制byte。两个旁路门都为零时，按live u32组B数量循环：每项先以参数0重置对象，再查询完成状态；只有返回精确1才计数。每次迭代重读live数量，不加现代上限；第九个组B对象在首次真实call typed-stop。完成数不等于循环结束时live数量时清message并返回。

随后按live u32组A数量逐项以参数0重置对象，同样不现代夹值；第十一个对象在首次真实call停止。准备callee发布四byte控制值：低word和高word都为零时清aux byte、message写100并返回；aux byte非零直接返回；否则低word零扩展选择组A对象。所选对象完成查询精确1时message写98、aux写2并返回。

未完成路径先遍历全部live组A对象，严格执行准备、重置和mode 1三call链。随后：

1. 清18条28-byte记录，再把每条首dword写全1；
2. 清`0x0053AE70`起七dword优先输出记录，首项改全1；
3. selected action写14，并把14写到共享opponent workspace的`10+低word`物理项；
4. 调用当前角色提交和动作14配置；
5. 写初始化门1，清提交/显示/目标就绪门；
6. active与committed代码写`低word+8`；
7. 共享50-dword表的`低word*5`项写1；
8. 清选择输入、两项cache与queued；published角色写控制值高word。

组A重置循环后直连过渡控制选择。控制pair高word非零时原样短路；否则按四行十列扫描共享word表，首个非零项清槽并把行号/值写入pair低/高word。全表为零时保留陈旧低word。函数返回EAX/ECX/EDX继续进入caller；旧准备控制槽reserved且生产零调用。

随后查询组A资源；从启动动作标签表读取低word角色项，再从56-byte步长profile物理表读取一个byte。profile参数只替换标签dword的CL，保留其高24位。以控制值高word零扩展索引组B对象，调用道具解析；返回AX为零时aux写2。非零时把AX作为道具编号、数量选择1直连已关闭玩家道具数量，保存返回payload token，aux写1，并按u32递减special count。玩家道具typed-stop保留此前全部重建与发布副作用。

## 4. 消息100–104与signed计时

100读取mode gate。精确1时只置completion gate。否则先直连胜利奖励与结算面板，再直连升级提示面板；两者都成功后才依次清actor retarget、置cache A/B与target-ready、清queued，再按u32递增timer；新值按i32不小于150时直连目标选择进入。任一子stop保留此前结算和画面前缀并阻断本段全部caller写入。

101在actor byte为`0xFF`时先直连角色升级属性提交；该函数可发布首个升级角色，若仍为`0xFF`才调用既有选角，仍无actor则timer清零、message写112。actor存在且transition state为零时，按i8 sign-extension构造组A对象，完成查询返回零才以`2,0`分配transition并保存EAX。角色升级子stop阻断选角、完成查询和后续transition/message/timer写入。state仍为零直接返回；否则actor若变为`0xFF`同样转112，否则调用阶段101并递增timer。

102在战利品u16数量为零时只置completion gate。非零时先递增timer；新值按i32不小于150时直连目标选择进入，成功后无条件直连战利品清单面板。面板先绘CP950“戰利品”双层底板，共享stage商零且live数量非零时按十项名称token/u16数量表逐行绘制`%-12s X %2d`；每行后重读live数量。面板typed-stop保留caller的timer与目标选择前缀并阻断后续主帧。旧阶段102槽reserved且生产零调用。

103总是先置cache A/B。battle flags bit3置位时递增timer，按i32达到30后清timer并置completion gate，不调用普通阶段。bit3未置位时先直连战败提示面板：双层底板显示CP950“戰鬥失敗”，共享stage商零时以字体17显示“隊伍全滅!!”再恢复16。面板正常返回后才走signed 150阈值；子stop阻断timer和目标选择。旧阶段103槽reserved且生产零调用。

104总是置cache A/B，再递增timer；新值按i32大于20才直连目标选择进入。102、100/103公共路径和104对应原三个静态目标选择callsite；typed-stop保留计时与各自前缀。

## 5. 消息110–113

110在transition state为零时按i8 actor构造组A对象，完成查询返回零才以`4,0`分配并保存state；state非零时直连角色成长对照面板。已有state时caller不验证actor byte，保留`0xFF`仍进入面板并由callee立即早退的原行为。成长面板子stop直接传播；caller没有后续写入。旧阶段110槽改为reserved且生产零调用。

111固定直连成长标题框；正常返回后重新读取timer并按u32递增。标题框mode非1时无画面副作用但仍递增；子typed-stop模拟callee不返回并阻断timer。旧阶段111槽reserved且生产零调用。

112仅在actor为`0xFF`时直连成长角色选择；该函数按live组A数量扫描两项精确1跳过门、角色完成查询、物理角色道具链、signed成长计数和精确1道具黑名单，成功时追加派生道具节点、复制24-byte标题并发布最后一个符合条件的角色。仍为`0xFF`则转113。选到角色后先以sample `0x160`和live signed mix level播放，再查询组A完成状态；返回零时以`8,1`分配transition但不保存返回。actor随后变回`0xFF`则转113，否则直连成长完成标题框并递增timer。入口actor本来非`0xFF`时直接进入该标题框，不执行caller的选角、播放、查询或分配；标题框内部仍按自身live stage零门决定是否再播放。角色选择或标题框子typed-stop阻断全部后置写，旧选角槽与旧阶段112槽均reserved且生产零调用。

113在actor为`0xFF`时先直连法宝成长结果角色选择；该函数按live组A数量、两项精确1字段和完成查询扫描，只测试成长结果callee的AX，首个非零结果依次加载/释放定义、写mode、复制标题并发布actor后立即返回。选到角色后才执行sample/完成查询/`8,1`分配链；仍为`0xFF`时把message写102并清timer，sample word非零才再播放`0x160`。存在角色时直连法宝完全成长提示框；该函数仅mode精确1时格式化CP950“法寶%s已完全成長!!”、绘制动态单行框，并在查询精确1时以字体17绘制后恢复16。正常返回后才递增timer；选角或提示框子typed-stop阻断全部后置写，旧选角槽与旧阶段113槽均reserved且生产零调用。

## 6. typed owner与caller回收

message、双方数量、七dword优先记录、输入cache/文字门、目标选择状态、组A显示位置、动作标签、动作workspace、18条记录、50-dword表、sample mix、调试flags/committed、queued/published、显示门和目标就绪门均复用既有唯一owner。新增message state只承接此前没有typed存储的物理链门、mode gate和组B旁路门；消息99的当前道具payload实际是胜利奖励十项payload数组第0项，现复用该唯一owner。消息99过渡控制选择复用startup reset的唯一40-word表与target-selection唯一控制pair；消息98炼符结果面板复用胜利奖励十项payload数组第0项作为名称token，并复用同一动作记录、stage、结果byte、framebuffer和字体owner；消息102清单数量继续复用transition u16，名称token和数量复用胜利奖励十项数组；消息103战败面板复用同一动作记录、stage、framebuffer和字体owner。消息112成长角色选择新增的四项计数/限制/道具码复用胜利奖励连续profile，物理角色道具链复用世界道具唯一owner；消息113成长结果选择继续复用同一160-byte定义scratch。两函数和法宝提示框共用角色升级owner中的24-byte标题。全局重置按原写集合清message的后两项与胜利奖励payload，保留入口链门和三组成长profile。

主帧协调器在HUD完成并执行第一后置阶段后直连本函数；原第二后置槽保留枚举数值并改为reserved，生产代码零调用。消息98直连炼符结果面板，消息99直连过渡控制选择，消息100内部依次直连胜利奖励和升级提示面板，消息101在缺少actor时直连角色升级属性提交，消息102在战利品非零时直连战利品清单面板，消息103普通路径直连战败提示面板，消息110在transition存在时直连角色成长对照面板，消息111固定直连成长标题框，消息112在缺少actor时直连成长角色选择，消息113在缺少actor时直连法宝成长结果角色选择并固定直连法宝完全成长提示框；旧消息98准备槽、消息99准备控制槽、阶段100、102、103、110、111、112、113与112/113选角槽均reserved且生产零调用。本函数或任一子函数typed-stop均阻断第三后置阶段、packed-row、头顶动作、对话和后续全部帧尾流程。

定向测试覆盖入口双门、switch域、十三项有效消息、组A坐标sign-extension、双方live循环及第九/第十一个对象边界、消息99三种早退和完整道具路径、18条记录与七dword记录、workspace/50-dword表、profile高24位、各组A对象地址乘法的预调用EAX/ECX/EDX、道具typed组合、101/110分配差异、110成长面板直连、111成长标题直连/timer回绕/子stop阻断、旧槽零调用、112成长角色选择直连/派生节点/子stop阻断及成长完成标题直连/variant寄存器/sample门/timer阻断、113成长结果选择直连/AX门/首成功/标题边界/子stop阻断及sample差异/法宝提示框/字体恢复/timer后置、102战利品清单直连/阈值顺序/十项表/格式边界/子stop阻断、103战败面板直连/调试旁路/ECX资源链/字体恢复/子stop阻断、98炼符结果直连/双资源链/live结果byte/成功失败文字/格式边界/子stop阻断、99过渡控制高word短路/四十项扫描/清槽/陈旧低word/寄存器链/旧槽零调用、102–104 signed阈值、目标选择子stop、动态调用trace超过40项、全局重置owner与唯一caller正常/stop传播。第155项验证：定向测试、AddressSanitizer、Linux core 188/188、Linux app 194/194 全部通过。源码构建零warning；app仅有既有ALSA开发库CMake提示。

当前缺少原版两组角色对象、11类未审业务callee共享副作用、显示/profile内容、真实道具链与成长定义、成长结果profile链、战利品名称/查询/字体/格式、战败查询/字体、炼符结果查询/格式/字体、法宝提示查询/字体、音效与目标选择联合状态、过渡控制表/pair动态联合状态、动态链门及EAX/ECX/EDX联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
