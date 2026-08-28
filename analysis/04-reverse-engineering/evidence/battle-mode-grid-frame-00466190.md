# 战斗模式网格帧 `0x00466190`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`、`caller_reclaimed`。

## 1. 完整权威范围

权威LST主体为`0x00466190..0x004664FB`，从proc到endp共381行、260条带机器码和真实助记符的实际指令、20个静态call、10个跳转、7个局部/返回标签和1个`retn`，没有外部`FUNCTION CHUNK`。唯一静态caller位于已关闭选择帧message 30路径；caller按原代码多推滚动参数，但本函数cdecl签名只读取原点X、原点Y和selected cell，第四参数保持未读。

20个callsite覆盖面板动作更新、一处面板矩形、两处九宫格、固定标题、字体模式/宽度及四处样式、首/次组行查询三处代码点、次组总量查询、两处actor刷新、选中矩形和三处单元文字代码点；另有Win32 `lstrcpyA`。已关闭矩形和九宫格直接组合；固定CP950复制在typed缓冲内等价执行；字体、文字和三类组A列表callee继续经窄typed端口表达。

## 2. 零入口与宽面板

函数清20-byte行文字缓冲，仅令首byte保持原始`0xFF`，并清三个dword局部计数。queued actor为0时返回EAX=0、ECX=0并保留入口EDX，不读取三个有效参数、不清共享面板记录，也不绘制。

非零路径完整清共享0x98-byte面板动作记录，写动作`0x233B`、variant 0并更新；更新失败没有原始分支。面板矩形固定为`origin_x, origin_y, 242x156, RGB 0/-8/-8, mode 1`。矩形返回EAX只替换资源低word并保留高16位后直连第一九宫格：`left=origin_x+6, top=origin_y+8, right=origin_x+238, bottom=origin_y+24, opacity 0, flags 0x80000008`。

第一九宫格返回EDX再只替换资源低word，因此第二九宫格保留另一组陈旧高16位：`left=origin_x+6, top=origin_y+40, right=origin_x+238, bottom=origin_y+152`。矩形/两处九宫格typed-stop各自保留此前面板前缀并阻断标题、字体和角色访问。

双九宫格后，标题直接复用21项静态文字token表索引9的`0x004A7688`，绘制于`origin_x+106, origin_y+8`，颜色`0xFFC0`、宽16；随后配置字体模式0、样式`0xFFFE`和宽度16。

## 3. 首组、次组总量与刷新寄存器

合法组A actor第一次在首组查询callee访问：固定请求`mode=0, page=1, row_text, primary_count`。十对象外在此typed-stop，保留完整面板、标题和字体前缀，且不改共享row-limit。成功后把primary count低word发布到共享row-limit，并执行第一次actor刷新；刷新EDX使用组A步长中间值。

随后固定查询次组总量`mode=0, secondary_count`，再执行第二次actor刷新。第二次刷新EDX故意保留次组总量callee返回EDX，而不是重新计算步长；测试用独立值锁定这项非对称。两类查询返回EAX都没有原始分支，不作为成功门。

列表开始前清复用的group-slot-count局部，把共享target argument预置1；page从1、group index从0、cell从1开始。

## 4. 两列五行与三段来源

函数严格绘制十格：第一列X=`origin_x+16`，第二列再加112；每列五行，Y从`origin_y+44`开始每次加20。cell按1..10连续递增，不读取caller多推的滚动参数。

每格来源分三段：

- cell `<= primary_count`时，不再查询，复用入口首组查询留下的同一行文字；
- cell大于首组但不大于`primary_count + secondary_count(low16)`时，调用次组行查询`mode=1, page`，接收文字和group-slot-count；查询入口EDX保持行文字缓冲token；
- cell超过两组总量时，等价执行`lstrcpyA(row_text, byte_49F9FC)`，只覆盖CP950`B5 4C 00`即“無\0”三字节，第三byte后的旧缓冲尾不清零。

每次次组查询后，先把group-slot-count低word按signed i16扩展到EDX，再增加group index；只有完整u32 group index精确等于该signed值时，page加1且group index清0。选中比较发生在这次推进之后。测试锁定两项容量2完成page前进、后续容量3保持组内索引，以及缺省“無”后仍保留前一字符串第3byte起的尾内容。

## 5. 选中目标与公共重绘

selected cell按完整dword与cell精确比较。匹配时先直连矩形`x-4, y-3, 106x24, RGB 31/20/0, mode 5`；typed-stop保留此前来源查询/复制但不绘制选中文字、不发布输入。矩形成功后，矩形返回EAX只替换主色低word并保留高16位，在原格再次绘制文字，再配置样式`0xFFFE`。

随后发布selection input gate=1，target argument先写推进后的page；若推进后的group index精确等于1，则改写为`page+1`；若共享primary row-limit为0，再把target完整dword减1。该顺序保留“次组查询先完成page/group推进，再进行目标修正”的原始非直观行为。

无论是否选中，每格最后都再次绘制公共文字并配置样式`0xFFFE`。非选中公共颜色只替换selected cell ECX低word，高16位保留；选中公共颜色的高word来自选中样式callee返回ECX。选中格因此执行“矩形后文字+公共文字”两次绘制，不能合并。

第十格后，EAX和EDX均保留一过尾cell 11，再执行最终样式`0xFFFE`。随后ECX低word替换为secondary count低word，把共享row-limit按u16回绕加上该值并返回；最终ECX高16位保留最终字体callee返回。

## 6. owner、caller回收与验证

面板动作记录复用主帧协调器唯一owner；row-limit、queued、主色、输入门和target argument复用既有owner；标题token复用既有静态文字表。新mode grid state只保存原函数栈上的20-byte缓冲和三个dword计数，不复制物理全局，也不接受相邻两个网格函数的滚动、flags、数值或共享说明状态。

选择帧原`draw_grid_mode` opaque槽保持枚举数值并改为reserved，生产代码零调用。message 30仍先读取live滚动值到ECX并压入未读第四参数的原寄存器形状，置cache C后直连本函数；任一子typed-stop保留mode-grid前缀并阻断最终cache A/B，正常返回才发布两项cache。

定向测试覆盖queued零EDX保留、动作更新失败继续、矩形/两处九宫格stop、EAX/EDX两段资源高word、固定标题、非法组A code、首/次组查询与两次不对称刷新、两列五行、首组复用、次组signed容量、CP950“無”三字节复制和旧尾保留、选中双绘制、矩形发布门、page/group目标修正、首组零减一、最终row-limit加法/ECX及唯一caller正常/stop传播。定向测试、AddressSanitizer、Linux core 188/188和Linux app 194/194全部通过；源码构建零warning，app仅保留既有ALSA开发库CMake warning。

当前缺少原版组A对象、首组/次组/总量callee联合状态、字体/文字surface、面板/矩形/九宫格framebuffer及EAX/ECX/EDX联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
