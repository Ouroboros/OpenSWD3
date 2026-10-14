# 战斗网格列表帧 `0x004659C0`

状态：`pending`。已废弃GPT重建实现，本轮逆向尚未开始。

以下保留历史分析与证据；旧结论、完成记录和测试结果不作为本轮验收。
原版采集事实保留，所有解释和实现结论须重新核实。

## 历史资料（重新核实前不生效）


历史还原状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`、`caller_reclaimed`。
当前完整INVOKE接口族仍`pending`。

## 当前行查询迁移

465BDD直接借实际list、配置、类别和序号，写入调用方名称、flags及数量。
删除普通query操作、脚本回包开关、query计数和子寄存器/字符串副本。
每次查询、耗尽复位和选中行说明均按LST时点重读角色。
名称与数字使用实际共享40字节存储，长名称允许跨入数字区；格式化保留旧尾。
共享文字运输容量同步到40字节，其余网格保持原20字节有效内容。
本批core/ASan的setup与actor316各1/1、SDL构建及14源身份通过，见
[行查询证据](battle-actor-resource-list-query-00470910.md)。
完整字体/绘制Port及其他寄存器协议仍pending。

## 绘制前实际条目统计

5a60ef7d使465B72直接借live队员list、完整类别及共享panel WORD累计条目。
删除计数脚本、actor/output token、子寄存器回包/数量副本与调用计数。
字体前继后读取live队员；父清WORD，叶保初值和逐次回绕，成功零仍查询行。
失败保面板/字体与0或2累计，沿原group_a_actor状态阻断后继根复位、绘制及输入修改。
270次新增网格、6992次叶及70次替代网格执行、四定向门各1/1、SDL136/136通过，见
[实际条目累计](battle-actor-resource-list-count-00470a10.md)。
共用initialize_rows映射撤回；其它查询、字体、寄存器、调用计数和Port继续pending。
以下全量门禁是历史记录，不是本轮验证或完整SDL链验收。

## 资源链头发布

3eda365b将465B95/465E2A改为直接发布actual party资源链头，删除纯转发回复与计数。
发布借用十个实际槽位；只在原耗尽分支尾复位，满七行不额外发布。
十槽两场景和门禁见[链头发布](battle-actor-resource-list-commit-00470900.md)。
共用refresh_actor枚举仍用于窄网格46FFE0索引提交，不是本叶协议。
首段及以下全量门禁为历史记录；完整查询、字体、寄存器与Port仍pending。

## 1. 完整权威范围

权威LST主体为`0x004659C0..0x00465E45`，从proc到endp共503行、337条带机器码和真实助记符的实际指令、27个静态call、11个跳转、11个局部/返回标签和1个`retn`，没有外部`FUNCTION CHUNK`。唯一静态caller位于已关闭选择帧message 4路径，参数固定为原点`(224,126)`、live网格选择和live滚动偏移。

27个callsite覆盖两处偏移动作帧代码点、六处字体样式及字体模式/宽度、面板动作更新、三处矩形、九宫格、角色列表初始化、两处角色刷新、行查询、五处原始字节文字、共享文字解析和两个Win32字符串操作。偏移动作帧循环运行四次并另绘制当前分类，因此正常路径共五次；每个可见行都执行公共字体样式，当前行再多执行一次。已关闭动作帧、矩形、九宫格与共享文字均直接组合，`wsprintfA("%2d")`和`lstrlenA`改为等价有界typed处理。

## 2. 零入口、五项动作框与面板

函数先清零40-byte局部文字区，令名称区和数字区首byte均保持原始`0xFF`，再清flags/value/display-count局部。queued actor为0时返回EAX/ECX/EDX全0，不读取四个参数，也不执行任何绘制或共享写。

非零路径以Y=`origin_y+32`、X从`origin_x+126`开始每次减42，按variant `3/2/1/0`直连动作`0x2394`四次；再读取live action category，按variant `category+4`、X=`origin_x+category*42`绘制当前分类，不预先限制category或u32回绕。任一动作帧typed-stop保留已完成框和该callee寄存器快照，并阻断全部面板/角色副作用。

五框后字体样式依次为`0xF000/0xFFFE`。函数完整清共享面板动作记录，写动作`0x233B`、variant 0并更新；更新失败没有原始分支，仍以清零记录的资源低word继续。面板矩形固定为`origin_x, origin_y+36, 204x152, RGB 0/4/4, mode 2`。矩形返回ECX只替换低word为资源，保留高16位后直连九宫格：`left=origin_x+6, top=origin_y+40, right=origin_x+200, bottom=origin_y+184, opacity 0, flags 0x80000008`。矩形/九宫格typed-stop均保留此前五框、字体和面板动作记录。

## 3. 角色初始化与无上限筛选

九宫格完成后依次配置字体模式0、样式`0xFFFE`和宽度16，读取queued及category后再清共享panel row-limit word。queued按u32 `queued-8`定位实际party list；十对象外在原首次计数对象访问点失败，保留完整面板前缀。

合法对象以live action category向共享row-limit WORD直接累计条目，随后发布资源根。
计数后重新读取queued并发布资源根。iterator从`scroll_offset+1`开始；每轮先读live category再读queued，直接查询实际list并写入共享40-byte名称/数字区、flags WORD及数量WORD。查询耗尽时不推进iterator，重读queued并复位该角色资源根，再进入最终字体尾。

非零查询只有flags bit15置位才成为可见行；未置位行只增加iterator，不增加display count、不配置公共行字体，也不消耗七行上限。原循环没有扫描上限，typed实现同样不增加modern cap；当前真实资源测试跨过512个不合格节点后显示7行；另以scroll回绕产生occurrence零隐藏行。旧脚本回包构造的9个隐藏项场景已撤销。每个可见行完成后才增加display count和iterator，并仅以display count低word做unsigned `<7`判断，因此第七行后不执行第八次查询。

## 4. 可见行文字、颜色与数值

value低word按signed i16使用旧`%2d`最小宽度格式化，保留一个前导空格、负号及超过两位的完整结果。flags bit14置位选择次色，否则主色；数值颜色只替换格式化返回ECX低word并保留高word。数值固定绘制于`origin_x+144, origin_y+display_row*20+44`。

数值文字callee返回EDX只替换同一颜色低word后用于名称颜色，高16位继续保留。名称从共享40-byte区起点读取，绘制于`origin_x+16`和同一Y。名称超过19字节时可覆盖从偏移20开始的数字区；格式化随后写入数字及NUL，名称绘制观察到该覆盖结果。隐藏行不格式化、不绘制，也不覆盖已保留的缓冲之外状态。测试锁定两条颜色路径、高word、`" 5"`/`"-3"`字节、动态iterator及每个draw call的EAX/ECX/EDX输入形状。

## 5. 当前行、发布顺序与底部说明

完整dword selected row只有精确等于`display_count+1`时进入当前行路径。bit14路径的选中文字颜色高word来自selected row，普通路径来自前一次名称callee返回EDX；函数在`origin_x+15, origin_y+display_row*20+43`再次绘制名称，随后先配置样式`0xFFFE`。

第一矩形固定为`origin_x, origin_y+display_row*20+40, 206x24, RGB 31/20/0, mode 5`。只有第一矩形成功后，才按原顺序先写target argument=当前iterator、selection input gate=1，再调用第二矩形`0,350,640x40, RGB 0/5/31, mode 5`。因此第一矩形stop不发布输入，第二矩形stop保留两项发布；两者都阻断共享说明与本行display count增加。

第二矩形后，重读live queued，再从该组A对象说明记录取得record token，并以同一对象的typed text index直连已关闭共享文字解析。MAPS payload和固定128-byte buffer只接受外部唯一owner span，battle不建立副本。成功后按首个NUL长度计算`320-8*(length>>1)`，在Y=360、颜色`0xFFFF`绘制说明。缺目录、terminator或buffer时在原访问点typed-stop，保留双矩形和输入发布。说明后执行公共样式`0xFFFE`、增加display count/iterator；正常循环结束再执行一次最终样式并返回其寄存器。

## 6. owner、caller回收与验证

五项动作框复用选择帧偏移动作帧唯一state，面板动作记录复用主帧协调器唯一owner；category、queued、row-limit、selection gate、target argument、颜色、说明record/index及MAPS/共享文字均复用已有owner。grid state只保存原函数栈上的共享文字区、flags/value和格式长度，不复制任何物理全局。

选择帧原`draw_grid_frame` opaque槽保持枚举数值并改为reserved，生产代码零调用。message 4先置cache C再直连本函数；任一子typed-stop保留grid前缀并阻断row-limit/auxiliary、纵向面板及最终cache A/B。正常返回才按u16发布row-limit/scroll，并按unsigned大于7决定纵向面板。

定向测试覆盖queued零、五项动作框顺序/位置、面板更新失败继续、陈旧资源高word、面板矩形/九宫格stop、非法组A code、隐藏行无上限扫描、两种颜色与`%2d`、当前行双绘制、第一/第二矩形发布差异、真实共享文字、缺MAPS目录、查询0刷新、七行低word上限、最终寄存器及唯一caller传播。验证：定向测试、AddressSanitizer、Linux core 188/188、Linux app 194/194 全部通过。源码构建零warning；app仅保留既有ALSA开发库CMake warning。

当前缺少原版组A对象、三类角色列表callee联合状态、字体/文字surface、动作/矩形/九宫格framebuffer及EAX/ECX/EDX联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
