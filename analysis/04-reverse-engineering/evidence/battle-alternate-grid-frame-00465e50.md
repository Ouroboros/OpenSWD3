# 战斗替代网格列表帧 `0x00465E50`

状态：`pending`。已废弃GPT重建实现，本轮逆向尚未开始。

以下保留历史分析与证据；旧结论、完成记录和测试结果不作为本轮验收。
原版采集事实保留，所有解释和实现结论须重新核实。

## 历史资料（重新核实前不生效）


历史还原状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`、`caller_reclaimed`。
当前INVOKE整族仍`pending`，不将历史还原分类当作完整迁移。

## 绘制前实际条目统计

5a60ef7d使465FC2在字体前继后借live队员list，清共享WORD并直接累计类别4条目。
普通增量与额外增量独立，失败保持标题/三项字体、head及0或2累计，阻断后继修改。
删除计数请求/回包、寄存器token、子计数和仅计数使用的scripted兼容开关。
70次新增十槽资格/失败/live执行、6992次叶与270次普通网格、四定向门各1/1、
SDL136/136及12源身份通过，见[实际条目累计](battle-actor-resource-list-count-00470a10.md)。
成功零仍沿原查询/收尾；七行后第八次查询顺序保持，message27始终借startup.party。
字体、绘制寄存器/计数、Port及完整SDL链继续pending。

## 实际资源名称与数量消费

84d3f985删除替代行查询的脚本回复、参数/寄存器转发、嵌套回包及两诊断计数，
实际叶直接写原文字缓冲和数量WORD。计数后根发布、每次查询与耗尽复位重读live队员，
不会固定借用初始party；正常耗尽保留上次输出，真实失败保持已发布/已绘制前缀。
七行后仍查询第八项，成功时保留该节点及输出而不绘制、不尾发布。

73次新增网格固定执行及原十槽满页/耗尽、真实页行为和选择帧同步；两core各1/1
为11.79/31.85秒、两ASan为18.56/31.79秒，SDL136/136重新链接及九源身份通过。
首轮固定借用队员的纠错、字体颜色消费和完整双向差异见
[带标记资源查询](battle-actor-flagged-resource-query-00470f70.md)。
3eda365b根发布历史见[链头发布](battle-actor-resource-list-commit-00470900.md)。
该批当时保留的初始化计数协议已由5a60ef7d回收；字体、绘制寄存器与计数、Port继续pending。
以下全量数字仅为历史。
本批未跑全量UT、Windows、实际MON、游戏或原版联合差分。

## 1. 完整权威范围

权威LST主体为`0x00465E50..0x00466181`，从proc到endp共357行、238条带机器码和真实助记符的实际指令、20个静态call、5个跳转、5个局部/返回标签和1个`retn`，没有外部`FUNCTION CHUNK`。唯一静态caller位于已关闭选择帧message 27路径，固定原点`(224,126)`并传live替代网格选择和live滚动偏移。

20个callsite覆盖面板动作更新、矩形与两处九宫格、字体模式/宽度及四个样式代码点、
五个原始字节文字代码点、角色列表初始化、两处资源根发布、替代行查询和Win32格式化。
矩形、九宫格、资源根发布、本行查询与条目累计直接组合；其余字体/文字端口待迁移。
`wsprintfA("%2d")`保留signed WORD和最小宽度的有界等价格式化。

## 2. 零入口与双九宫格面板

函数清两组20-byte局部文字缓冲，仅令行文字首byte保持原始`0xFF`，并清display count、value和格式长度。queued actor为0时返回EAX/ECX/EDX全0，不读取四参数、不清共享面板记录，也不执行任何绘制。

非零路径完整清共享0x98-byte面板动作记录，写动作`0x233B`、variant 0并更新；更新失败没有原始分支，继续执行。第一矩形固定为`origin_x, origin_y, 190x188, RGB 0/-8/-8, mode 1`。矩形返回EDX只替换资源低word并保留高16位后，直连第一九宫格：`left=origin_x+6, top=origin_y+8, right=origin_x+186, bottom=origin_y+24, opacity 0, flags 0x80000008`。

第一九宫格callee返回EDX再次只替换同一资源低word，因此第二九宫格保留第一九宫格返回的另一组高16位：`left=origin_x+6, top=origin_y+40, right=origin_x+186, bottom=origin_y+184`，其余参数相同。矩形、第一或第二九宫格typed-stop都保留此前面板动作与绘制前缀；任一stop都阻断标题、字体和角色访问。测试用两组不同高word锁定该陈旧寄存器链。

## 3. 固定标题、字体与组A条目统计

双九宫格后，以固定静态文字token表中的`0x004A76A0`在`origin_x+80, origin_y+8`绘制标题，颜色立即数`0xFFC0`、字体宽16。随后字体依次设置模式0、样式`0xFFFE`和宽度16。

函数再清共享panel row-limit WORD，以固定selector 4向实际list直接累计条目。
成功后重读queued并借实际party发布资源根；正常零不新增成功门或早退。
十对象外或真实资源访问失败沿原对象通道停止，保持面板、标题、三项字体和累计前缀。

## 4. 查询、七行上限与第八次副作用

iterator按完整DWORD从`scroll_offset+1`开始，每次借live queued所指party的实际list、
20-byte行文字缓冲和数量WORD；原第一个固定0参数未读，已删除。本父没有flags缓冲，
不读取bit15/bit14。completed/false才不绘制、重读live队员发布根并进入最终字体样式；
节点或文字失败保留前缀并阻断收尾，不伪装成正常耗尽。

成功后才以display count低WORD做unsigned `>=7`判断。七行之后仍查询第八项，
保留实际节点、文字和数量，但不绘制、不发布根、不增加显示数。
测试直接核第八项head、文字H和数量12；查询/访问诊断计数及伪回包已删除。
保留原循环与WORD判断，不增加扫描上限。

## 5. 行文字、数值与选中矩形

每个可见行先名称、再格式化并绘制数量。名称主色只读实际WORD；字体flags10路径
最终435991 AND FFFF，高位不进入颜色像素，不再借查询的模拟ECX高位。
名称固定在`origin_x+16, origin_y+row*20+44`；实际数量WORD按signed i16
用`%2d`最小宽度格式化，数值在`origin_x+144`和同一Y。数量格式化和选中重绘
的其余寄存器协议仍待迁移，保留原颜色读取时点。
测试锁定0、3、WORD回绕、-32768、-2以及旧5/-3的名称/数量绘制顺序。

完整dword selected row只有精确等于`display_count+1`时选中。选中文字颜色只替换selected row EAX低word；名称在`origin_x+15, origin_y+row*20+43`重绘，随后配置样式`0xFFFE`。选中矩形固定为`origin_x, origin_y+row*20+40, 194x24, RGB 31/20/0, mode 5`。只有矩形成功后才发布target argument=当前iterator和selection input gate=1；typed-stop保留名称与字体前缀但不发布两项输入。本函数没有第二全宽矩形、MAPS共享说明或底部说明文字。

选中或普通行最后都执行公共样式`0xFFFE`，再增加display count并把iterator保存为下一轮滚动基准。查询0、七行上限或自然退出最终再执行一次样式`0xFFFE`，返回该字体callee寄存器。

## 6. 历史owner、caller回收与验证

面板动作记录复用主帧协调器唯一owner；row-limit、queued、输入门、target argument与主色复用既有owner；固定标题token直接复用21项静态文字token表。新alternate grid state只保存原函数栈上的两组20-byte缓冲、display count、value和格式长度，不复制物理全局，也不复用相邻函数带flags/共享说明的错误栈形状。

选择帧原`draw_grid_alternate` opaque槽保持枚举数值并改为reserved，生产代码零调用。message 27先置cache C再直连本函数；任一子typed-stop保留alternate前缀并阻断row-limit auxiliary、纵向面板及最终cache A/B。正常返回才按u16发布row-limit/scroll，并以unsigned大于7决定纵向面板。

定向测试覆盖queued零、动作更新失败继续、矩形/两处九宫格stop、两次资源陈旧高word、固定标题、三项字体、非法组A code、查询0二次刷新、七行后第八次查询、名称/数值顺序、signed `%2d`、主色高word、当前行重绘、单矩形发布门、最终字体寄存器及唯一caller正常/stop传播。验证：定向测试、AddressSanitizer、Linux core 188/188、Linux app 194/194 全部通过。源码构建零warning；app仅保留既有ALSA开发库CMake warning。

当前缺少原版组A对象、固定selector初始化/刷新/替代查询callee联合状态、字体/文字surface、面板/矩形/九宫格framebuffer及EAX/ECX/EDX联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
