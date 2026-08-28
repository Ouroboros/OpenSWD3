# 战斗胜利战利品清单面板 `0x00469080`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`、`caller_reclaimed`。

## 1. 完整权威范围

权威LST主体为`0x00469080..0x00469211`，从proc到endp共182行、115条带机器码和真实助记符的实际指令、10个静态call、3个跳转、2个局部标签和1个返回点，没有外部`FUNCTION CHUNK`。10个callsite依次为字体18、动作记录更新、矩形效果、首层九宫格、标题文字、次层九宫格、面板查询、逐行`wsprintfA`、逐行文字和字体16。

唯一静态caller位于已关闭消息阶段的消息102分支。原caller在战利品数量非零时先递增timer，新值按i32不小于150时调用已关闭目标选择入口，随后每帧无条件调用本函数；数量为零时不调用本函数，直接发布完成门。

## 2. 局部缓冲、字体与固定面板

入口建立64-byte局部文字缓冲：首byte复制live seed，其余63 byte清零。随后把战利品数量按u16零扩展，计算清单底边`212 + count * 20`，以字体对象调用字号18。入口EAX和ECX在此前初始化链中被覆盖，EDX保留到字体调用。

函数把共享动作记录写为动作`0x233B`、variant零并更新；矩形使用`x=196`、`y=176`、宽184、高`live stage + 40`、RGB `0,4,4`和mode零。矩形返回EAX只替换低word为共享动作资源，保留高16位后绘制`(200,180)..(376,196)`首层九宫格。

标题使用CP950“戰利品”，在`264,180`以颜色`0xFFC0`和字体参数16绘制。标题文字返回EAX再次只替换资源低word并保留高16位，绘制`(200,212)..(376, live stage + 212)`次层九宫格。矩形或任一九宫格typed-stop保留此前字体、动作、矩形、资源和画面前缀，不执行后续标题、查询、行文字或字体恢复。

## 3. 查询门与live清单循环

次层九宫格正常返回后固定查询`212, 212 + entry_count * 20, 3`。只有返回EAX精确等于1且live u16数量非零才进入清单；其他返回值或零数量均跳过行循环，但仍在尾部把字体恢复为16。

每行按物理索引同时读取十项共享名称token表和十项u16数量表。名称以完整dword进入EDX；数量先清ECX再写CX，因此严格零扩展。随后使用CP950格式`%-12s X %2d`写入同一64-byte局部缓冲，并在`x=210`、`y=212 + index * 20`绘制。格式化后的文字调用保留`wsprintfA`返回EAX，同时把ECX设为局部缓冲token、EDX设为framebuffer token。

每行绘制后索引递增，再重新读取live u16数量并按i32执行`index < count`。callee缩短或扩展数量会改变同一帧的后续行数；不会使用入口快照作为循环上限。数量扩展到十一项时，在首次真实读取第十一项名称/数量处typed-stop，不增加现代上限；此前双层面板、查询和前十行保持可观察。

`wsprintfA`写入最多允许63个非NUL byte。格式长度达到64时保留完整64-byte目标前缀，并在随后NUL的首次目标访问处typed-stop；当前行不绘制，字体不恢复。正常完成全部行或跳过行循环时，最后以同一字体对象设置字号16并返回该call的完整EAX/ECX/EDX。

## 4. owner、caller回收与验证

本函数不新增持久业务owner。战利品u16数量复用`LegacyBattleTargetSelectionRuntimeState::transition_sample_word`；十项名称token和十项u16数量复用`LegacyBattleVictoryRewardState`；动作记录、stage、framebuffer、矩形/九宫格资源与字体均复用既有owner。动态局部缓冲地址只作为`compat::u32` token，不转换为宿主指针。

消息102已在非零数量的timer与目标选择链之后直连本实现。面板typed-stop保留caller先前timer写和目标选择副作用，并阻断主帧后续stage。旧消息102面板槽保留枚举数值并改名为reserved，生产代码零调用。主帧适配分别映射字体、标题、查询、行格式和行绘制五类服务，格式文字使用独立发布位与显式长度，合法零长度不与缺失回复混淆。

定向测试覆盖64-byte局部seed、u16入口数量、动态底边与stage尺寸、动作记录、矩形和两次EAX资源高字链、CP950标题、查询精确1两侧、查询成功但live零数量、名称dword、数量零扩展、格式参数、20像素行距、live数量缩短、十项物理边界、64-byte格式边界、首层九宫格stop、消息102零数量/阈值/直连/旧槽零调用/子stop传播及主帧五服务映射。验证：定向测试、AddressSanitizer、Linux core 188/188、Linux app 194/194 全部通过。源码构建零warning；app仅有既有ALSA开发库CMake提示。

当前缺少原版局部seed、真实framebuffer/字体/边框资源、面板查询callee、名称指针及格式化联合状态、动态栈地址、动作/矩形/九宫格/文字返回和EAX/ECX/EDX联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
