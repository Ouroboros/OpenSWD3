# 战斗网格列表帧 `0x004659C0`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`、`caller_reclaimed`。

## 1. 完整权威范围

权威LST主体为`0x004659C0..0x00465E45`，从proc到endp共503行、337条带机器码和真实助记符的实际指令、27个静态call、11个跳转、11个局部/返回标签和1个`retn`，没有外部`FUNCTION CHUNK`。唯一静态caller位于已关闭选择帧message 4路径，参数固定为原点`(224,126)`、live网格选择和live滚动偏移。

27个callsite覆盖两处偏移动作帧代码点、六处字体样式及字体模式/宽度、面板动作更新、三处矩形、九宫格、角色列表初始化、两处角色刷新、行查询、五处原始字节文字、共享文字解析和两个Win32字符串操作。偏移动作帧循环运行四次并另绘制当前分类，因此正常路径共五次；每个可见行都执行公共字体样式，当前行再多执行一次。已关闭动作帧、矩形、九宫格与共享文字均直接组合，`wsprintfA("%2d")`和`lstrlenA`改为等价有界typed处理。

## 2. 零入口、五项动作框与面板

函数先把两组20-byte局部文字缓冲清零，仅令行文字首byte保持原始`0xFF`，再清flags/value/display-count局部。queued actor为0时返回EAX/ECX/EDX全0，不读取四个参数，也不执行任何绘制或共享写。

非零路径以Y=`origin_y+32`、X从`origin_x+126`开始每次减42，按variant `3/2/1/0`直连动作`0x2394`四次；再读取live action category，按variant `category+4`、X=`origin_x+category*42`绘制当前分类，不预先限制category或u32回绕。任一动作帧typed-stop保留已完成框和该callee寄存器快照，并阻断全部面板/角色副作用。

五框后字体样式依次为`0xF000/0xFFFE`。函数完整清共享面板动作记录，写动作`0x233B`、variant 0并更新；更新失败没有原始分支，仍以清零记录的资源低word继续。面板矩形固定为`origin_x, origin_y+36, 204x152, RGB 0/4/4, mode 2`。矩形返回ECX只替换低word为资源，保留高16位后直连九宫格：`left=origin_x+6, top=origin_y+40, right=origin_x+200, bottom=origin_y+184, opacity 0, flags 0x80000008`。矩形/九宫格typed-stop均保留此前五框、字体和面板动作记录。

## 3. 角色初始化与无上限筛选

九宫格完成后依次配置字体模式0、样式`0xFFFE`和宽度16，再先清共享panel row-limit word。queued按u32 `queued-8`和组A步长构造actor token；十对象外只在首次列表初始化callee访问点typed-stop，保留完整面板前缀。

合法对象以live action category和row-limit地址初始化列表，随后刷新actor。iterator从`scroll_offset+1`开始；每轮查询固定接收category、iterator、20-byte行文字缓冲、flags地址和value地址。查询返回0时不推进iterator，立即再次刷新actor并进入最终字体尾。

非零查询只有flags bit15置位才成为可见行；未置位行只增加iterator，不增加display count、不配置公共行字体，也不消耗七行上限。原循环没有扫描上限，typed实现同样不增加modern cap；定向测试连续跳过9个隐藏项后仍继续到7个可见项。每个可见行完成后才增加display count和iterator，并仅以display count低word做unsigned `<7`判断，因此第七行后不执行第八次查询。

## 4. 可见行文字、颜色与数值

value低word按signed i16使用旧`%2d`最小宽度格式化，保留一个前导空格、负号及超过两位的完整结果。flags bit14置位选择次色，否则主色；数值颜色只替换格式化返回ECX低word并保留高word。数值固定绘制于`origin_x+144, origin_y+display_row*20+44`。

数值文字callee返回EDX只替换同一颜色低word后用于名称颜色，高16位继续保留。名称固定使用查询写入的20-byte缓冲，绘制于`origin_x+16`和同一Y。隐藏行不格式化、不绘制，也不覆盖已保留的缓冲之外状态。测试锁定两条颜色路径、高word、`" 5"`/`"-3"`字节、动态iterator及每个draw call的EAX/ECX/EDX输入形状。

## 5. 当前行、发布顺序与底部说明

完整dword selected row只有精确等于`display_count+1`时进入当前行路径。bit14路径的选中文字颜色高word来自selected row，普通路径来自前一次名称callee返回EDX；函数在`origin_x+15, origin_y+display_row*20+43`再次绘制名称，随后先配置样式`0xFFFE`。

第一矩形固定为`origin_x, origin_y+display_row*20+40, 206x24, RGB 31/20/0, mode 5`。只有第一矩形成功后，才按原顺序先写target argument=当前iterator、selection input gate=1，再调用第二矩形`0,350,640x40, RGB 0/5/31, mode 5`。因此第一矩形stop不发布输入，第二矩形stop保留两项发布；两者都阻断共享说明与本行display count增加。

第二矩形后，从组A对象说明记录取得record token，并以同一对象的typed text index直连已关闭共享文字解析。MAPS payload和固定128-byte buffer只接受外部唯一owner span，battle不建立副本。成功后按首个NUL长度计算`320-8*(length>>1)`，在Y=360、颜色`0xFFFF`绘制说明。缺目录、terminator或buffer时在原访问点typed-stop，保留双矩形和输入发布。说明后执行公共样式`0xFFFE`、增加display count/iterator；正常循环结束再执行一次最终样式并返回其寄存器。

## 6. owner、caller回收与验证

五项动作框复用选择帧偏移动作帧唯一state，面板动作记录复用主帧协调器唯一owner；category、queued、row-limit、selection gate、target argument、颜色、说明record/index及MAPS/共享文字均复用已有owner。新grid state只保存原函数栈上的两组文字缓冲、flags/value和格式长度，不复制任何物理全局。

选择帧原`draw_grid_frame` opaque槽保持枚举数值并改为reserved，生产代码零调用。message 4先置cache C再直连本函数；任一子typed-stop保留grid前缀并阻断row-limit/auxiliary、纵向面板及最终cache A/B。正常返回才按u16发布row-limit/scroll，并按unsigned大于7决定纵向面板。

定向测试覆盖queued零、五项动作框顺序/位置、面板更新失败继续、陈旧资源高word、面板矩形/九宫格stop、非法组A code、隐藏行无上限扫描、两种颜色与`%2d`、当前行双绘制、第一/第二矩形发布差异、真实共享文字、缺MAPS目录、查询0刷新、七行低word上限、最终寄存器及唯一caller传播。验证：定向测试、AddressSanitizer、Linux core 188/188、Linux app 194/194 全部通过。源码构建零warning；app仅保留既有ALSA开发库CMake warning。

当前缺少原版组A对象、三类角色列表callee联合状态、字体/文字surface、动作/矩形/九宫格framebuffer及EAX/ECX/EDX联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
