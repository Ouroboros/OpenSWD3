# 战斗列表内容 `0x004655B0`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`、`caller_reclaimed`。

## 1. 完整权威范围

权威LST主体为`0x004655B0..0x004659BB`，从proc到endp共465行、338条实际指令、21个静态call、16个跳转、16个局部/返回标签和1个`retn`，没有外部`FUNCTION CHUNK`。唯一静态caller位于已关闭选择帧message 2路径，参数固定为面板原点`(232,134)`、live列表选择和live滚动偏移。

21个callsite覆盖三种字体配置、角色列表初始化、两处角色刷新、行查询、负/普通行解析、资源帧、四处原始字节文字、两处矩形、共享文字解析和两个Win32字符串操作。已关闭资源帧、矩形与共享文字解析均由caller直接组合；`wsprintfA("%3d")`和`lstrlenA`改为等价的有界typed格式化/长度计算。本次行查询迁移后，游标复位直接写角色状态，行查询直连真实列表与MON；计数现也直接写共享行上限；其余行解析、字体和文字协议仍待迁移。

## 2. 入口、字体与角色初始化

函数先构造十byte局部数值缓冲及两项局部输出。queued actor为0时返回EAX 0、ECX 0并保留入口EDX，不执行字体或共享写。

非零路径依次调用字体模式0、样式`0xFFFE`和宽度16；每次只重装字体owner ECX，EAX/EDX沿用前一callee返回。随后先把共享panel row-limit byte清0，再按u32 `queued-8`与组A固定步长构造actor token。已知十对象外只在首次初始化callee访问点typed-stop，保留三次字体和row-limit清零前缀。

合法对象直接统计实际角色列表，参数为live action category、type 0及共享row-limit BYTE引用；每个匹配节点递增该BYTE，保留回绕与游标副作用。计数完成后重新读取实时queued actor，把next游标写入current。删除初始化操作、参数包、行上限回包和调用计数。计数遇到缺失节点时，保留已累计的行上限及失败游标，不进入复位与逐行绘制；详见[计数证据](battle-actor-list-count-004702e0.md)。

## 3. 行查询、选择器与七行循环

iterator从`scroll_offset+1`开始，row counter从0开始。每轮行查询直接接收live action category、0、iterator、共享selection workspace、局部value及实际MON资料；显示帧号作为可选资源帧，输出value低word为`0xFFFF`时再次复位actor并结束。`00465965/0046599D`确认每行及失败出口清值；不得跨行保留上一行value。

非终止value按bit15选择两种角色解析callee：置位走负选择器并在callee后把完整value按`&0x7FFF`清高域；未置位走普通选择器。两条路径都可更新陈旧局部limit word/byte。limit与value只按i16 signed比较，决定主/次文字颜色；不把它们现代化为unsigned域。

每轮尾把dword row counter和iterator各加1，只以row counter低word做unsigned `<7`判断。普通路径最多查询七行且不会进行第八次查询；早期`0xFFFF`终止则保留当前iterator。循环结束统一再次配置样式`0xFFFE`，函数返回该callee的EAX/ECX/EDX。

## 4. 可选资源、名称与三位数值

行查询返回低word不等于`0xFFFF`时，先递减完整返回dword作为帧索引，再直接调用已关闭资源帧绘制：资源`0x241C`、X=`origin_x-4`、Y=`origin_y+row*20+37`。子typed-stop保留角色查询/解析和帧发布前缀，并阻断本行全部文字、选中面板及循环推进。

名称前20字节直接写输入分派的五DWORD；更长名称继续覆盖LST紧邻的共享音乐路径，至首NUL停止，剩余字节保持。名称固定使用共享selection workspace地址，坐标`origin_x+12, origin_y+row*20+37`。limit小于value时，次色只替换解析callee返回EAX低word并保留高word；否则主色只替换EDX低word，EAX重装名称X。随后value按signed i16和最小宽度3格式化，保留前导空格及负号；数值坐标为`origin_x+144`和同一Y。数值的次色参数保留格式化后ECX高word，主色参数从格式化返回长度替换低word。测试逐callsite锁定参数及EAX/ECX/EDX形状。

## 5. 当前行、双矩形与底部说明

完整dword selected row只有精确等于`row+1`时进入选中路径。函数先在名称坐标各减1处再次绘制同一名称与同一主/次色分支，再按原顺序直接执行：

```text
矩形一：x=origin_x-8, y=row_y+33, width=192, height=24,
        RGB=31,20,0, mode=5
矩形二：x=0, y=350, width=640, height=40,
        RGB=0,5,31, mode=5
```

两处矩形typed-stop分别保留选中文字及第一处矩形前缀，并阻断共享说明、输入门和iterator发布。

矩形完成后，从组A对象`0x00505890 + index*0x2F34`字段取得记录token，并使用该记录`+4`的typed text index直连已关闭共享文字解析。MAPS payload与固定128字节共享buffer由外部唯一owner以span绑定，battle不建立副本；缺目录、terminator或目标空间时在原解析访问点typed-stop并保留此前绘制。成功后按首个NUL计算旧`lstrlenA`长度，以`320-8*(length>>1),360`绘制说明文字。随后再设样式`0xFFFE`，才依次写selection input gate=1与candidate iterator。

## 6. caller回收、owner与验证

选择帧原`draw_list_contents` opaque槽保持枚举数值并改为reserved，生产代码零调用。message 2只有在列表框完成且frame B=7、frame A=10时直连本函数；任一资源、矩形或共享文字typed-stop保留cache C与本函数前缀，并阻断row-limit发布、纵向面板及最终cache A/B。

物理owner继续复用：queued actor、action category、panel row-limit、selection workspace、selection input gate、candidate iterator、主/次文字颜色及组A对象。启动状态新增十项说明记录token/text-index视图并在原角色重置时清零；共享文字buffer只接受外部现有owner，不新增battle存储。

历史workpack曾通过定向测试、AddressSanitizer、Linux core 188/188和Linux app 194/194；这些完整门不覆盖本次修改。本批以真实列表和合成MON代替行回包夹具，覆盖负/普通值、帧号、七行上限、链尾复位、逐行清值、共享名称及音乐路径覆盖；最新门禁见[行查询证据](battle-actor-list-query-00470180.md)。计数批次的新增夹具检查真实行上限及缺失节点前缀，当前验证状态见[计数证据](battle-actor-list-count-004702e0.md)。LIST整链仍pending，其他解析、字体与文字协议未关闭。

当前缺少原版组A对象、五类角色列表callee联合状态、字体/文字surface以及完整EAX/ECX/EDX捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
