# 战斗替代网格列表帧 `0x00465E50`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`、`caller_reclaimed`。

## 1. 完整权威范围

权威LST主体为`0x00465E50..0x00466181`，从proc到endp共357行、238条带机器码和真实助记符的实际指令、20个静态call、5个跳转、5个局部/返回标签和1个`retn`，没有外部`FUNCTION CHUNK`。唯一静态caller位于已关闭选择帧message 27路径，固定原点`(224,126)`并传live替代网格选择和live滚动偏移。

20个callsite覆盖面板动作更新、三处矩形/九宫格绘制中的一处矩形与两处九宫格、字体模式/宽度及四个样式代码点、五个原始字节文字代码点、角色列表初始化、两处角色刷新、替代行查询和Win32 `%2d`格式化。已关闭矩形和九宫格直接组合；原始字节文字、字体、角色对象和三类列表callee继续经窄typed端口表达，`wsprintfA("%2d")`改为等价有界格式化。

## 2. 零入口与双九宫格面板

函数清两组20-byte局部文字缓冲，仅令行文字首byte保持原始`0xFF`，并清display count、value和格式长度。queued actor为0时返回EAX/ECX/EDX全0，不读取四参数、不清共享面板记录，也不执行任何绘制。

非零路径完整清共享0x98-byte面板动作记录，写动作`0x233B`、variant 0并更新；更新失败没有原始分支，继续执行。第一矩形固定为`origin_x, origin_y, 190x188, RGB 0/-8/-8, mode 1`。矩形返回EDX只替换资源低word并保留高16位后，直连第一九宫格：`left=origin_x+6, top=origin_y+8, right=origin_x+186, bottom=origin_y+24, opacity 0, flags 0x80000008`。

第一九宫格callee返回EDX再次只替换同一资源低word，因此第二九宫格保留第一九宫格返回的另一组高16位：`left=origin_x+6, top=origin_y+40, right=origin_x+186, bottom=origin_y+184`，其余参数相同。矩形、第一或第二九宫格typed-stop都保留此前面板动作与绘制前缀；任一stop都阻断标题、字体和角色访问。测试用两组不同高word锁定该陈旧寄存器链。

## 3. 固定标题、字体与组A初始化

双九宫格后，以固定静态文字token表中的`0x004A76A0`在`origin_x+80, origin_y+8`绘制标题，颜色立即数`0xFFC0`、字体宽16。随后字体依次设置模式0、样式`0xFFFE`和宽度16。

函数再清共享panel row-limit word，以固定selector 4初始化组A角色列表。queued按u32 `queued-8`和组A步长构造actor token；十对象外只在首次列表初始化callee访问点typed-stop，保留面板、标题、三项字体和row-limit清零。合法对象初始化可发布row-limit，随后刷新actor。

## 4. 查询、七行上限与第八次副作用

iterator从`scroll_offset+1`开始，每轮替代查询固定接收0、iterator、20-byte行文字缓冲和value地址；本函数没有flags缓冲，也不读取bit15/bit14，不跳过隐藏项目。查询返回0时不绘制该行，按原顺序再次刷新actor，再进入最终字体样式。

与相邻网格函数不同，本函数在查询成功后才以display count低word做unsigned `>=7`判断。因此七行已完成时仍执行第八次查询；若第八次非零，保留其callee与缓冲写副作用，但不绘制、不刷新、不增加display count，直接进入最终字体样式。定向测试固定查询iterator `scroll+1..scroll+8`、`scanned_rows=8`、`displayed_rows=7`和第八项不进入trace。原循环由七行上限天然界定，不增加其他modern cap。

## 5. 行文字、数值与选中矩形

每个可见行先绘制名称，再格式化并绘制数值，顺序不得与相邻函数互换。主色低word分别覆盖查询返回ECX与格式化返回ECX，高16位各自保留。名称固定在`origin_x+16, origin_y+row*20+44`；value低word按signed i16用旧`%2d`最小宽度格式化，数值固定在`origin_x+144`和同一Y。测试锁定`" 5"`、`"-3"`、两组颜色高word及draw call寄存器形状。

完整dword selected row只有精确等于`display_count+1`时选中。选中文字颜色只替换selected row EAX低word；名称在`origin_x+15, origin_y+row*20+43`重绘，随后配置样式`0xFFFE`。选中矩形固定为`origin_x, origin_y+row*20+40, 194x24, RGB 31/20/0, mode 5`。只有矩形成功后才发布target argument=当前iterator和selection input gate=1；typed-stop保留名称与字体前缀但不发布两项输入。本函数没有第二全宽矩形、MAPS共享说明或底部说明文字。

选中或普通行最后都执行公共样式`0xFFFE`，再增加display count并把iterator保存为下一轮滚动基准。查询0、七行上限或自然退出最终再执行一次样式`0xFFFE`，返回该字体callee寄存器。

## 6. owner、caller回收与验证

面板动作记录复用主帧协调器唯一owner；row-limit、queued、输入门、target argument与主色复用既有owner；固定标题token直接复用21项静态文字token表。新alternate grid state只保存原函数栈上的两组20-byte缓冲、display count、value和格式长度，不复制物理全局，也不复用相邻函数带flags/共享说明的错误栈形状。

选择帧原`draw_grid_alternate` opaque槽保持枚举数值并改为reserved，生产代码零调用。message 27先置cache C再直连本函数；任一子typed-stop保留alternate前缀并阻断row-limit auxiliary、纵向面板及最终cache A/B。正常返回才按u16发布row-limit/scroll，并以unsigned大于7决定纵向面板。

定向测试覆盖queued零、动作更新失败继续、矩形/两处九宫格stop、两次资源陈旧高word、固定标题、三项字体、非法组A code、查询0二次刷新、七行后第八次查询、名称/数值顺序、signed `%2d`、主色高word、当前行重绘、单矩形发布门、最终字体寄存器及唯一caller正常/stop传播。验证：定向测试、AddressSanitizer、Linux core 188/188、Linux app 194/194 全部通过。源码构建零warning；app仅保留既有ALSA开发库CMake warning。

当前缺少原版组A对象、固定selector初始化/刷新/替代查询callee联合状态、字体/文字surface、面板/矩形/九宫格framebuffer及EAX/ECX/EDX联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
