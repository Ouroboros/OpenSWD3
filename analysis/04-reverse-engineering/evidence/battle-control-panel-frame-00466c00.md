# 战斗控制面板帧 `0x00466C00`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`、`caller_reclaimed`。

## 1. 完整权威范围

权威LST主体为`0x00466C00..0x00466F6D`，从proc到endp共384行、247条带机器码和真实助记符的实际指令、24个静态call、8个跳转、6个局部/返回标签和1个`retn`，没有外部`FUNCTION CHUNK`。唯一静态caller位于已关闭选择帧message 7，cdecl读取原点X、原点Y和selected index三项参数。

24个callsite覆盖两处已关闭边框面板、九处文字、一次字体reset、九处字体style、三次主选项查询、两次特殊选项查询和一次`wsprintfA`。边框面板直接组合；特殊文字格式化改为固定24-byte typed写入；字体、文字和两类组B查询经窄typed端口保留。

## 2. 固定标题、双边框与基础动作

第一边框固定为资源`0x234E`，位置`origin-8,-40`，横向重复4、纵向重复2、底板颜色72。成功后在`origin_x,origin_y-32`绘制CP950“控制”。第二边框位置`origin-8,-8`，横向重复4，纵向重复完整alternate selection limit，颜色72。两个边框复用同一边框state与同一垂直渐变颜色槽；任一typed-stop保留此前边框/标题前缀。

第二边框后字体reset参数0，再设normal style `0xFFFE`，在原点绘制CP950“攻擊”。selected index完整dword等于1时设selected style `0xF000`并重绘同一文字。攻击固定占选择索引1。

## 3. 三项主选项与压缩索引

函数把循环索引置0、可见计数置1，并在每轮查询前以六次`stosd`清`0x0053C16C`起24-byte共享文字区。selected group-B index从共享u16按i16 sign-extension；对象token为组B物理基址加`0x2B28*index_bits`。负索引和八外索引只在首次真实查询typed-stop。

主查询固定执行三次，入口EAX=`345*actor_index_bits`、ECX=组B对象token、EDX=源索引；同时传共享文字token和动态栈输出token。返回不等于1时只推进源索引，不增加可见计数或Y。成功时可见计数加1、Y加20，normal style后绘制共享文字。

selected index等于本次压缩后可见计数时，把查询输出写transition A、transition B清0，再selected style重绘。本工作包以request token表达原动态栈地址，不转主机指针。

## 4. 两项特殊选项、释放与尾style

主循环后第二循环固定两次，同样每次先清24-byte文字区并查询当前组B对象。失败只推进源索引。成功时可见计数加1，再次清24-byte区，以CP950“特殊”加one-based源序号和NUL写入；Y加20，normal style后绘制。

selected index匹配时把transition A清0、transition B写one-based特殊序号，再selected style重绘。特殊格式固定只可能1或2，不扩大共享区。

第二循环结束后设normal style，可见计数加1、Y加20，在该压缩索引绘制CP950“釋放”；匹配selected时selected style重绘。最后无条件恢复normal style，并以该callee返回AX作为旧函数返回低word。攻击索引1，所有成功动态项连续占2起的索引，释放索引恒为成功动态项总数加2。

## 5. owner、caller回收与验证

alternate selection limit与selected index分别复用frame-input和input-dispatch owner；transition A/B复用frame-input owner。`0x0053C16C..0x0053C183`新增为input-dispatch的六dword唯一文字owner，并与既有`0x0053C184`五dword选择工作区保持物理相邻但不重叠。边框state由控制面板state唯一持有；垂直渐变颜色槽复用当前目标提示state唯一owner。

选择帧原`draw_message_seven` opaque槽保持枚举数值并改为reserved，生产代码零调用。message 7仍先读取live面板原点与alternate selection，再以`x+12,y+8,selected`直连本函数；子typed-stop直接传播。

定向测试覆盖双边框几何与stop、固定标题、攻击/释放索引、selected重绘、三次主查询与两次特殊查询、成功/失败压缩、组B寄存器、signed负索引stop、共享24-byte清零、三项主文字、两项CP950特殊格式、transition A/B互斥发布、最终normal style及唯一caller正常/stop传播。验证：定向测试、AddressSanitizer、Linux core `188/188`和Linux app `194/194`全部通过。源码构建零warning；app仅有既有ALSA开发库CMake warning。

当前缺少原版组B对象、主/特殊查询、字体/文字、动态栈输出地址、边框/framebuffer及EAX/ECX/EDX联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
