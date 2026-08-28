# 战斗窄网格帧 `0x00466500`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`、`caller_reclaimed`。

## 1. 完整权威范围

权威LST主体为`0x00466500..0x004667AD`，从proc到endp共293行、197条带机器码和真实助记符的实际指令、17个静态call、6个跳转、7个局部/返回标签和1个`retn`，没有外部`FUNCTION CHUNK`。唯一静态caller位于已关闭选择帧message 8路径，cdecl签名读取固定原点X、原点Y和selected display row三项参数。

17个callsite覆盖面板动作更新、一处面板矩形、两处九宫格、固定标题、字体模式/宽度及两处样式、窄列表初始化、两处actor刷新、一处窄行查询、普通/选中两处文字和选中矩形。已关闭矩形和九宫格直接组合；字体、文字和三类组A列表callee继续经窄typed端口表达。

## 2. 零入口与窄面板

函数先清两个dword栈局部。queued actor为0时返回EAX=0，并保留入口ECX和EDX；不清共享面板记录、共享byte或物理文字工作区，也不绘制。

非零路径完整清共享0x98-byte面板动作记录，写动作`0x233B`、variant 0并更新；更新失败没有原始分支。面板矩形固定为`origin_x, origin_y, 190x172, RGB 0/-8/-8, mode 1`。矩形返回EDX只替换资源低word并保留高16位后直连第一九宫格：`left=origin_x+6, top=origin_y+8, right=origin_x+186, bottom=origin_y+24, opacity 0, flags 0x80000008`。

第一九宫格返回EDX再只替换同一资源低word，因此第二九宫格保留另一组陈旧高16位：`left=origin_x+6, top=origin_y+40, right=origin_x+186, bottom=origin_y+168`。矩形/两处九宫格typed-stop各自保留此前面板前缀并阻断标题、字体和角色访问。

双九宫格后，标题直接复用21项静态文字token表索引19的`0x004A7648`，绘制于`origin_x+80, origin_y+8`，颜色`0xFFC0`、宽16；随后配置字体模式0、样式`0xFFFE`和宽度16。

## 3. 共享byte、actor寄存器与物理文字区

标题与字体后先清共享单byte row-limit，再访问组A actor。因此十对象外typed-stop保留该byte清零，并以预调用EAX=`(code-8)*3021`、ECX=组A对象token、EDX=原code返回；初始化callee未发生。

合法actor固定初始化请求为`mode=0, selector=1, shared-byte`。初始化入口EAX为3021倍数、EDX为原code；其后第一次刷新入口EAX为1007倍数、EDX为3021倍数。source iterator从1开始；每次查询固定请求`mode=0, selector=1, iterator, shared-text, row-value`，查询入口EAX为3021倍数而EDX仍为原code。这三种寄存器形状不能合并。

查询使用的20-byte文字缓冲物理地址与输入分派五dword选择工作区完全重叠。本实现对同一五dword owner做显式x86 little-endian装载/写回，没有创建第二份共享文字owner；包括零结果和终止哨兵在内，只要callee发布文字，覆盖就立即保留。

## 4. 稀疏扫描与七个有效行

每次查询后先从dword row-value重载EAX，再只检查低word：

- 低word为`0xFFFF`时，立即执行第二次actor刷新，清row-value并结束；iterator不增加；
- 低word为0时不绘制、不增加display count，但仍清row-value并把iterator加1；
- 其他值绘制一个有效行。

循环只在每轮末尾以display count低word做unsigned `<7`比较。零结果不会计入七行上限，也没有现代查询次数上限；原始流若持续返回0就持续扫描。达到第七个有效行时，仍完成该行全部副作用，再把iterator加1后结束，不额外执行第八次查询。

有效行几何只使用display count低word：X固定`origin_x+16`；Y为`origin_y+40+22*display_index`。因此零结果只推进source iterator，不留下视觉空行。普通文字颜色用row-value dword高16位加主色低word；文字call入口固定EAX为字体token、ECX为文字对象token、EDX为X。测试用独立高word锁定这项陈旧域。

## 5. 选中重绘与发布顺序

普通文字完成后，函数把selected row参数装入EAX，再以`display_index+1`比较；selected按显示行一基计数，不按source iterator计数。未选中时，该参数EAX会保留到循环尾；若第七行结束，成为最终样式callee的入口EAX。

选中时先用普通文字callee返回ECX高16位加主色低word，在`origin_x+15, base_y+39`再次绘制同一物理文字；该call入口EAX为偏移X、ECX为文字对象token、EDX为偏移Y。然后直连矩形`origin_x, base_y+37, 192x24, RGB 31/20/0, mode 5`。矩形typed-stop保留普通与选中两次文字，但不调用选中后样式、不增加display count、不发布输入或candidate。

矩形成功后配置样式`0xFFFE`，再发布selection input gate=1和candidate argument=当前source iterator；最后才增加display count。函数退出循环后无条件再配置一次样式`0xFFFE`并返回最终callee寄存器。

## 6. owner、caller回收与验证

面板动作记录复用主帧协调器唯一owner；共享byte复用帧输入owner；主色复用启动owner；输入门和candidate复用目标选择owner；20-byte文字直接复用输入分派五dword选择工作区。新narrow state只保存原函数两个dword栈局部，不保存文字副本。

选择帧原`draw_narrow_frame` opaque槽保持枚举数值并改为reserved，生产代码零调用。message 8仍先发布cache C并装载live selected row，再直连本函数；任一子typed-stop保留narrow前缀并阻断cache A/B，正常返回才发布两项cache。

定向测试覆盖queued零ECX/EDX与工作区保留、动作更新失败继续、矩形/双九宫格stop、两段资源高word、固定标题、非法组A code与共享byte时序、三种actor寄存器形状、零结果稀疏扫描、七有效行上限、无第八次查询、终止哨兵刷新、物理工作区覆盖、有效行几何、row-value/文字返回高word、普通/选中两次文字、选中矩形发布门、candidate source iterator、最终selected EAX及唯一caller正常/stop传播。定向测试、AddressSanitizer、Linux core 188/188和Linux app 194/194全部通过；源码构建零warning，app仅保留既有ALSA开发库CMake warning。

当前缺少原版组A对象、窄列表初始化/刷新/查询callee联合状态、字体/文字surface、面板/矩形/九宫格framebuffer及EAX/ECX/EDX联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
