# 战斗消息阶段分派 `0x00466F70`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`、`caller_reclaimed`。

## 1. 完整权威范围

权威LST主体为`0x00466F70..0x004676BC`，从proc到endp共911行、560条带机器码和真实助记符的实际指令、40个静态call、47个跳转、43个局部标签、1个default标签和21个`retn`，没有外部`FUNCTION CHUNK`。唯一静态caller位于已关闭主帧协调器：HUD之后依次调用选择帧、本函数和后一战斗阶段；本工作包回收第二个后置槽并传播typed-stop。

40个callsite由31处未审战斗调用、一次已关闭胜利奖励、一次已关闭升级面板、三处已关闭目标选择进入、一次已关闭玩家道具数量和三处已关闭sample命令组成。胜利奖励、升级面板、目标选择与玩家道具数量直接组合；音效复用输入分派的sample typed接口；其余22类未审业务callee通过窄端口保留，循环和分支造成的31个静态位置均按原时序执行。

## 2. 入口双门与switch域

入口先读取`0x004ACF48`物理链门；非零立即返回该值。链门为零后比较`0x005214F8`首dword；非零同样立即返回，EAX仍为零。两门都为零才读取共享message并按u32减96；结果大于17进入默认返回。

有效消息为96–104和110–113共十三项。105–109与所有域外值不产生副作用。域下方95保留减法回绕EAX，域上方114保留EAX 18。

- 96：依次清输入cache B、选择输入门和message。
- 97：以live组A数量索引32-byte显示槽，按i16 sign-extension传X/Y与对应组A对象；返回精确1才把AL写当前过渡角色byte。
- 98：先置输入cache A，再调用准备阶段。

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

随后查询组A资源；从启动动作标签表读取低word角色项，再从56-byte步长profile物理表读取一个byte。profile参数只替换标签dword的CL，保留其高24位。以控制值高word零扩展索引组B对象，调用道具解析；返回AX为零时aux写2。非零时把AX作为道具编号、数量选择1直连已关闭玩家道具数量，保存返回payload token，aux写1，并按u32递减special count。玩家道具typed-stop保留此前全部重建与发布副作用。

## 4. 消息100–104与signed计时

100读取mode gate。精确1时只置completion gate。否则先直连胜利奖励与结算面板，再直连升级提示面板；两者都成功后才依次清actor retarget、置cache A/B与target-ready、清queued，再按u32递增timer；新值按i32不小于150时直连目标选择进入。任一子stop保留此前结算和画面前缀并阻断本段全部caller写入。

101在actor byte为`0xFF`时先选角；仍为`0xFF`则timer清零、message写112。actor存在且transition state为零时，按i8 sign-extension构造组A对象，完成查询返回零才以`2,0`分配transition并保存EAX。state仍为零直接返回；否则actor若变为`0xFF`同样转112，否则调用阶段101并递增timer。

102在sample word为零时只置completion gate。非零时先递增timer；新值按i32不小于150时直连目标选择进入，成功后无条件调用阶段102。

103总是先置cache A/B。battle flags bit3置位时递增timer，按i32达到30后清timer并置completion gate，不调用普通阶段。bit3未置位时先调用阶段103，再走signed 150阈值。

104总是置cache A/B，再递增timer；新值按i32大于20才直连目标选择进入。102、100/103公共路径和104对应原三个静态目标选择callsite；typed-stop保留计时与各自前缀。

## 5. 消息110–113

110在transition state为零时按i8 actor构造组A对象，完成查询返回零才以`4,0`分配并保存state；state非零才调用阶段110。已有state时不验证actor byte，保留`0xFF`仍调用阶段的原行为。

111固定以`0,0`调用阶段111，随后按u32递增timer。

112仅在actor为`0xFF`时执行选角；仍为`0xFF`则转113。选到角色后先以sample `0x160`和live signed mix level播放，再查询组A完成状态；返回零时以`8,1`分配transition但不保存返回。actor随后变回`0xFF`则转113，否则调用阶段112并递增timer。入口actor本来非`0xFF`时直接阶段112，不播放、不查询、不分配。

113与112的选角/sample/完成查询/`8,1`分配链一致。actor仍为`0xFF`时先把message写102并清timer；sample word非零才再播放`0x160`。存在角色时调用阶段113并递增timer。

## 6. typed owner与caller回收

message、双方数量、七dword优先记录、输入cache/文字门、目标选择状态、组A显示位置、动作标签、动作workspace、18条记录、50-dword表、sample mix、调试flags/committed、queued/published、显示门和目标就绪门均复用既有唯一owner。新增message state只承接此前没有typed存储的物理链门、mode gate和组B旁路门；消息99的当前道具payload实际是胜利奖励十项payload数组第0项，现复用该唯一owner。全局重置按原写集合清message的后两项、胜利奖励全部payload并保留链门。

主帧协调器在HUD完成并执行第一后置阶段后直连本函数；原第二后置槽保留枚举数值并改为reserved，生产代码零调用。消息100内部依次直连胜利奖励和升级提示面板，旧阶段100槽同样reserved且生产零调用。本函数或胜利奖励typed-stop均阻断第三后置阶段、packed-row、头顶动作、对话和后续全部帧尾流程。

定向测试覆盖入口双门、switch域、十三项有效消息、组A坐标sign-extension、双方live循环及第九/第十一个对象边界、消息99三种早退和完整道具路径、18条记录与七dword记录、workspace/50-dword表、profile高24位、各组A对象地址乘法的预调用EAX/ECX/EDX、道具typed组合、101/110分配差异、112/113 sample差异、102–104 signed阈值、目标选择子stop、动态调用trace超过40项、全局重置owner与唯一caller正常/stop传播。验证：定向测试、AddressSanitizer、Linux core `188/188`、Linux app `194/194`全部通过。源码构建零warning；app仅有既有ALSA开发库CMake warning。

当前缺少原版两组角色对象、22类未审业务callee共享副作用、显示/profile内容、音效与目标选择联合状态、动态链门及EAX/ECX/EDX联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
