# 战斗护驾面板帧 `0x004667B0`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`、`caller_reclaimed`。

## 1. 完整权威范围

权威LST主体为`0x004667B0..0x0046694E`，从proc到endp共184行、119条带机器码和真实助记符的实际指令、11个静态call、3个跳转、4个局部/返回标签和1个`retn`，没有外部`FUNCTION CHUNK`。唯一静态caller位于已关闭选择帧message 5路径，cdecl签名读取面板列表原点X/Y两项参数。

11个callsite覆盖面板动作更新、固定面板矩形、两处九宫格、固定标题、选中矩形、循环字体宽度、组A名称查询、循环文字、缺省文字和最终字体宽度。已关闭矩形和九宫格直接组合；字体、文字和名称查询继续经窄typed端口表达。

## 2. 固定面板与两段资源高word

函数不清0x98-byte共享面板动作记录，只覆盖动作`0x233B`和variant 0后更新；更新失败没有原始分支，记录其余内容仍由callee按原逻辑处理。固定矩形与caller参数无关：`196,176,188x88,RGB 0/4/4,mode 0`。

矩形返回EAX只替换资源低word并保留高16位，绘制第一九宫格`200,180..376,196`。第一九宫格后先绘制固定CP950标题“佈置護駕”，token为`0x004A79D0`，坐标`252,180`、颜色`0xFFC0`、宽16。标题文字callee返回EDX再只替换资源低word并保留高16位，绘制第二九宫格`200,212..376,260`。因此第二段资源高word来自标题文字而不是第一九宫格返回。

固定矩形、两处九宫格typed-stop各自保留此前前缀；第二九宫格stop还保留标题绘制。

## 3. 先选中矩形、后护驾列表

第二九宫格后无条件读取共享group-B row selection，按`row*22`计算选中框：X=`arg_x-2`，Y=`arg_y+row*22-18`，尺寸`188x24`，RGB `31/20/0`、mode 5。选框typed-stop阻断全部字体与名称查询。

护驾数量只读取共享target-effect dword高word并零扩展为u32。原代码随后对ESI做signed `jle`，但零扩展保证只有0跳过；`0x8000..0xFFFF`仍进入循环，不能按i16解释。循环只按该值递减到0，没有现代上限；畸形大值在首次真实组A对象访问typed-stop。

每轮先配置字体宽18，再令actor index=`group_a_count-remaining`，即从组A尾部连续数量段的首项向后枚举。组A对象访问入口EAX为index的1007倍数、ECX为对象token、EDX为3021倍数。十对象外在名称查询点typed-stop，并保留该轮字体宽18调用。

名称查询返回EAX作为文字token。绘制X=`arg_x+12`，Y=`arg_y+8+22*(total-remaining)`，颜色`0xFFFF`、宽16；文字call入口EAX为Y、ECX为文字对象token、EDX为显示行乘11。循环完成后，如果原数量低word精确等于1，额外在`arg_x+12,arg_y+30`绘制CP950“無”，其call入口EAX保留上一文字返回、EDX为字体token。数量0或大于1均不绘制缺省行。

最后无条件配置字体宽16并返回callee寄存器。

## 4. owner、caller回收与验证

共享面板动作记录复用主帧协调器唯一owner；group-B row selection复用帧输入owner；group-A count复用actor metrics owner；target-effect复用目标选择owner。本函数没有新增物理状态owner，结果trace只用于测试观测。

选择帧原`draw_message_five` opaque槽保持枚举数值并改为reserved，生产代码零调用。message 5以固定参数直连本函数并直接返回；任一子typed-stop传播到选择帧，不执行旧opaque端口。

定向测试覆盖动作更新失败继续、动作记录不预清、固定矩形/双九宫格/标题、EAX与标题EDX两段资源高word、选框先行及stop、数量0/1/2、高位数量零扩展、无现代循环上限、组A尾段索引、字体18/16、名称查询三寄存器、行几何、文字call寄存器、“無”精确条件、非法组A index typed-stop及唯一caller正常/stop传播。定向测试、AddressSanitizer、Linux core 188/188和Linux app 194/194完整门全部通过；源码构建零warning，app仅有既有ALSA开发库CMake warning。

当前缺少原版组A对象、名称查询callee联合状态、字体/文字surface、面板/矩形/九宫格framebuffer及EAX/ECX/EDX联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
