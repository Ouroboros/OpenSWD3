# 战斗当前目标提示帧 `0x00466950`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`、`caller_reclaimed`。

## 1. 完整权威范围

权威LST主体为`0x00466950..0x00466BFE`，从proc到endp共317行、217条带机器码和真实助记符的实际指令、18个静态call、12个跳转、7个局部/返回标签和2个`retn`，没有外部`FUNCTION CHUNK`。唯一静态caller位于已关闭选择帧message 3末尾的live selection-input gate路径，cdecl读取固定提示原点X/Y两项参数。

静态callsite覆盖组B名称、`lstrlenA`、面板动作更新、矩形、九宫格、字体宽20/16、名称文字、目标指标来源与解析、指标pair、`wsprintfA`、格式化文字、渐变宽度、两条分支各自的颜色查询和常量色垂直渐变。动作更新、矩形、九宫格和垂直渐变已关闭并直连；字符串长度和格式化改为有界typed等价；剩余对象、字体、文字与指标查询经窄typed端口保留。

## 2. 入口门与组B对象

函数先将20-byte栈文字区首byte写旧空字符串首byte、其余19 byte清零。随后令dword index=`queued_actor_code*5-40`并读取`0x00520E90`五dword分组的首项；越界只在该首次真实读取typed-stop。读值为1、target-selection block完整dword为1、published code按i32小于等于0、或按i32大于group-B count时直接返回。

published code通过这些门后作为one-based组B code。名称查询前保留EAX=`345*code`、ECX=`0x005229E0+0x2B28*code`、EDX=前述dword index。count大于八仍可让code 9通过门，但在首次真实组B名称callee访问typed-stop；没有现代八对象夹值。

名称查询返回文字token及旧`lstrlenA`等价byte长度。旧`cdq/sub/sar`按i32向零除2，得到双字节字符数；高位畸形长度也保持该算术，不改成无符号除法。

## 3. 名称框与资源继承

mirror mode完整dword精确等于1时，名称框X=`630-20*字符数-arg_x`；其他值直接使用`arg_x`。Y为`arg_y`。函数不预清共享面板动作记录，只写动作`0x233B`与variant 0；更新失败没有原始分支。

矩形为`x-4,y-4,(20*字符数+8)x24`，RGB参数`0/-8/-8`，mode 1。矩形返回ECX只替换动作资源低word并保留高16位，绘制九宫格`x,y..x+20*字符数,y+16`，flags `0x80000008`。矩形或九宫格typed-stop各自保留此前前缀。

九宫格后设字体宽20，在`x+2,y`以颜色`0xFFFF`和绘制宽16显示名称；文字call入口EAX为字体token、ECX为文字对象token、EDX为`x+2`。随后恢复字体宽16。

## 4. 生命文字、镜像不对称与渐变

函数以组B对象查询指标来源，再用固定owner解析。解析返回低word小于10时直接返回；否则查询当前值和上限，并以CP950固定前缀“生命:”及signed `%d/%d`写20-byte局部缓冲。若第20字节后的首次写发生，typed实现保留此前字节并stop，不扩大栈缓冲或截断成功。

这里保留两个不同镜像判断：名称框只在mirror完整值`==1`时镜像；生命文字和渐变只判断mirror是否非0。mirror为0时生命文字位于`x+20*字符数+10,y-2`，文字call入口EAX为Y、ECX为文字对象、EDX为字体token；mirror非0时位于`x-140,y-2`，入口EAX为字体token、ECX为文字对象、EDX为X。mirror值2因此保留“名称不镜像、生命文字镜像”的原始不对称。

解析低word至少15时再查询渐变宽度，返回低word零扩展；0直接返回。非0时先按原分支在栈上形成三个未由callee读取的`0/0/24`预计算参数，再查询颜色。渐变X与生命文字X相同，Y=`arg_y+17`，宽为查询低word，高3，颜色为查询返回完整dword；直连已关闭常量色垂直渐变。callee typed-stop保留颜色槽发布和此前全部绘制。

## 5. owner、caller回收与验证

五dword分组复用startup reset唯一owner；queued/published复用final-actor owner；group-B count复用metrics owner；target-selection block复用frame-input owner；mirror复用startup owner；面板记录、framebuffer、raster、共享blitter请求/effects/jitter均复用选择帧既有绑定。垂直渐变四字节颜色槽由选择帧提示state唯一持有；20-byte文字区及两个输出值仍是每次调用局部，不建立物理全局owner，动态栈地址以request token表达而不转主机指针。

选择帧原`draw_selection_hint` opaque槽保持枚举数值并改为reserved，生产代码零调用。message 3仅在live input gate为1时以固定`12/14`直连本函数；子typed-stop直接传播。

定向测试覆盖所有入口门、读后寄存器、五dword越界、signed published范围、第九组B对象、名称长度除2、非镜像/镜像/值2不对称、动作更新失败继续、矩形与九宫格stop、矩形ECX资源高word、字体20/16、名称call寄存器、指标阈值9/10/15、CP950 signed格式、20-byte首次越界、生命文字两分支寄存器、渐变宽0、颜色预计算参数、垂直渐变成功/stop及唯一caller正常/stop传播。定向测试、AddressSanitizer、Linux core 188/188和Linux app 194/194完整门全部通过；源码构建零warning，app仅有既有ALSA开发库CMake warning。

当前缺少原版组B对象、名称/指标/字体/文字联合状态、动态栈地址、面板/矩形/九宫格/framebuffer及EAX/ECX/EDX联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
