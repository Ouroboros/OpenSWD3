# 战斗窄网格帧 `0x00466500`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`、`caller_reclaimed`。

## 1. 完整权威范围

权威LST主体为`0x00466500..0x004667AD`，从proc到endp共293行、197条带机器码和真实助记符的实际指令、17个静态call、6个跳转、7个局部/返回标签和1个`retn`，没有外部`FUNCTION CHUNK`。唯一静态caller位于已关闭选择帧message 8路径，cdecl签名读取固定原点X、原点Y和selected display row三项参数。

17个callsite覆盖面板动作更新、一处面板矩形、两处九宫格、固定标题、字体模式/宽度及两处样式、窄列表初始化、两处actor刷新、一处窄行查询、普通/选中两处文字和选中矩形。矩形和九宫格直接组合；本次游标复位改为直接写共享角色，行查询直连实际列表与MON。计数现也直接写共享行上限；字体与文字协议仍待迁移。

## 2. 零入口与窄面板

函数先清两个dword栈局部。queued actor为0时返回EAX=0，并保留入口ECX和EDX；不清共享面板记录、共享byte或物理文字工作区，也不绘制。

非零路径完整清共享0x98-byte面板动作记录，写动作`0x233B`、variant 0并更新；更新失败没有原始分支。面板矩形固定为`origin_x, origin_y, 190x172, RGB 0/-8/-8, mode 1`。矩形返回EDX只替换资源低word并保留高16位后直连第一九宫格：`left=origin_x+6, top=origin_y+8, right=origin_x+186, bottom=origin_y+24, opacity 0, flags 0x80000008`。

第一九宫格返回EDX再只替换同一资源低word，因此第二九宫格保留另一组陈旧高16位：`left=origin_x+6, top=origin_y+40, right=origin_x+186, bottom=origin_y+168`。矩形/两处九宫格typed-stop各自保留此前面板前缀并阻断标题、字体和角色访问。

双九宫格后，标题直接复用21项静态文字token表索引19的`0x004A7648`，绘制于`origin_x+80, origin_y+8`，颜色`0xFFC0`、宽16；随后配置字体模式0、样式`0xFFFE`和宽度16。

## 3. 共享byte、actor寄存器与物理文字区

标题与字体后先清共享单byte row-limit，再访问组A actor。因此十对象外typed-stop保留该byte清零，并以预调用EAX=`(code-8)*3021`、ECX=组A对象token、EDX=原code返回；初始化callee未发生。

合法actor直接以category 0、type 1统计实际列表，逐个命中递增共享row-limit BYTE；删除初始化操作、参数包、行上限回包和调用计数。计数失败保留面板、标题、字体、已累计BYTE和失败游标，不进入后续复位及行查询；完成后重新读取实时queued actor并把next游标写入current。iterator从1开始；查询直接接收category 0、type 1、序号、共享文本、输出值、MON资料，不再传寄存器包。详见[计数证据](battle-actor-list-count-004702e0.md)。

名称前20字节与输入分派五DWORD工作区重叠；LST确认其后紧接768字节音乐路径。本实现直接写两段实际owner，长名称的后段保留原版对音乐路径的覆盖。绘制参数读取这两段，首NUL之后不清零；链尾不改文本。没有第二份持久共享文字owner。

## 4. 稀疏扫描与七个有效行

每次查询后先从dword row-value重载EAX，再只检查低word：

- 低word为`0xFFFF`时，立即执行第二次actor刷新，清row-value并结束；iterator不增加；
- 低word为0时不绘制、不增加display count，但仍清row-value并把iterator加1；
- 其他值绘制一个有效行。

循环只在每轮末尾以display count低word做unsigned `<7`比较。父函数仍保留零结果跳过分支；本次实际type 1查询在正常命中时最后强制输出1，不再用假回包制造0、5、6等结果。不符合条件的节点由查询核心过滤，iterator按命中序号递增。达到第七个有效行后仍完成全部副作用并推进iterator，不做第八次查询。

有效行几何只使用display count低word：X固定`origin_x+16`；Y为`origin_y+40+22*display_index`。因此零结果只推进source iterator，不留下视觉空行。普通文字颜色用row-value dword高16位加主色低word；文字call入口固定EAX为字体token、ECX为文字对象token、EDX为X。每行按`00466751`清零DWORD，真实查询只写低WORD，因此普通文字该高WORD为零；选中文字仍保留前次文字callee的ECX高WORD。

## 5. 选中重绘与发布顺序

普通文字完成后，函数把selected row参数装入EAX，再以`display_index+1`比较；selected按显示行一基计数，不按source iterator计数。未选中时，该参数EAX会保留到循环尾；若第七行结束，成为最终样式callee的入口EAX。

选中时先用普通文字callee返回ECX高16位加主色低word，在`origin_x+15, base_y+39`再次绘制同一物理文字；该call入口EAX为偏移X、ECX为文字对象token、EDX为偏移Y。然后直连矩形`origin_x, base_y+37, 192x24, RGB 31/20/0, mode 5`。矩形typed-stop保留普通与选中两次文字，但不调用选中后样式、不增加display count、不发布输入或candidate。

矩形成功后配置样式`0xFFFE`，再发布selection input gate=1和candidate argument=当前source iterator；最后才增加display count。函数退出循环后无条件再配置一次样式`0xFFFE`并返回最终callee寄存器。

## 6. owner、caller回收与验证

面板动作记录复用主帧协调器唯一owner；共享byte复用帧输入owner；主色复用启动owner；输入门和candidate复用目标选择owner；20-byte文字直接复用输入分派五dword选择工作区。narrow state保存两个DWORD局部及查询资料存储；资料初始零值属于宿主初始化，不冒充原版未初始化栈捕获。不保存第二份共享名称或音乐路径。

选择帧原`draw_narrow_frame` opaque槽保持枚举数值并改为reserved，生产代码零调用。message 8仍先发布cache C并装载live selected row，再直连本函数；任一子typed-stop保留narrow前缀并阻断cache A/B，正常返回才发布两项cache。

历史workpack的定向测试、AddressSanitizer、Linux core 188/188及Linux app 194/194不覆盖本次修改。本批夹具改为真实列表及MON，覆盖节点过滤、命中序号、七行上限、链尾复位、长名称与音乐路径覆盖、完整名称绘制、两处文字和矩形前缀。最新验证见[行查询证据](battle-actor-list-query-00470180.md)。计数批次另加真实行上限与失败前缀检查，当前验证状态见[计数证据](battle-actor-list-count-004702e0.md)。LIST整链仍pending，绘制与父协议未关闭。

当前缺少原版组A对象、窄列表初始化/刷新/查询callee联合状态、字体/文字surface、面板/矩形/九宫格framebuffer及EAX/ECX/EDX联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
