# 战斗列表框 `0x00465480`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`、`caller_reclaimed`。

## 1. 完整权威范围

权威LST主体为`0x00465480..0x004655A3`，从proc到endp共131行、91条实际指令、7个静态call、2个跳转、1个循环标签、1个返回标签和1个`retn`，没有外部`FUNCTION CHUNK`。唯一静态caller位于已关闭选择帧message 2入口，固定传入`(224,126)`。

五个唯一callee中，动作帧偏移绘制、动作更新、矩形效果和九宫格边框均已关闭并直接组合；两次字体样式继续复用选择帧已有窄typed端口。

## 2. 三项基础动作框与当前分类框

入口先把Y固定为`origin_y+32`，再按variant `2,1,0`绘制三项动作框。首项X为`origin_x+84`，每项按u32减42；四项动作号均为`0x2394`、偏移模式均为0。三次调用不依赖callee返回值，按固定三轮执行，不增加现代循环门。

随后live读取共享action category。当前分类框使用base variant `category+4`与X=`origin_x+category*42`，全部算术保持u32回绕，不预先限制category。四次调用直接复用动作帧偏移函数的单一持久state；动作更新后EDX高字继续由每个callsite的寄存器快照进入帧号。任一子typed-stop保留此前已绘制动作框，并阻断字体、面板记录、底色、边框和动画推进。

## 3. 字体样式与共享面板动作记录

四框完成后，字体owner固定依次收到`0xF000`与`0xFFFE`。第二次调用前只重装ECX字体owner，EAX/EDX继续来自第一次字体callee返回。

随后严格清零共享面板动作记录全部`0x98`字节，再把选择动画frame A置10，依次写动作号`0x233B`与base variant 0并调用动作更新。该callee返回没有条件分支：即使更新失败，也继续使用清零记录中未被更新的内容执行底色与边框，不能现代化为提前停止。

物理记录不建立列表框私有副本，直接复用主帧协调器已有panel action record owner。四次`0x2394`绘制则继续复用动作帧偏移state port，保持另一物理记录唯一。

## 4. 底色、陈旧资源高字与九宫格

动作更新后固定调用矩形效果：

```text
x      = origin_x
y      = origin_y + 36
width  = 190
height = 152
RGB    = 0,4,4
mode   = 2
```

矩形callee返回后只以`mov cx`替换ECX低word为面板动作记录`+0x4A`资源号，高16位必须保留矩形返回ECX；不能把资源擅自零扩展。随后九宫格调用参数固定为：

```text
resource = rectangle_ecx_high | panel_resource_low
left     = origin_x + 6
top      = origin_y + 40
right    = origin_x + 186
bottom   = origin_y + 184
opacity  = 0
flags    = 0x80000008
```

原调用顺序是动作更新→矩形→九宫格，不等同于现有通用效果面板的矩形→动作更新→九宫格组合，故不能错误复用后者改变副作用顺序。矩形或九宫格typed-stop分别保留已完成动作框、字体、记录更新和已到达绘制前缀，并阻断动画推进。

## 5. 动画推进与返回寄存器

九宫格正常完成后，函数读取live frame B并按u32加2，把未夹值结果写回并以signed i32和7比较。只有signed结果大于7时才把共享frame B改写为7；高位为1的负值与回绕值保持原signed行为。

EAX返回始终是加2后的未夹值结果，即使共享frame B最终写成7也不回写EAX。ECX/EDX保留九宫格callee返回。选择帧caller随后重读live frame B决定是否继续列表内容，所以typed实现既发布未夹值返回，也保留共享状态夹值。

## 6. caller回收、测试与动态差分

选择帧原`draw_list_frame` opaque槽保持相同枚举数值并改为reserved，生产代码零调用。message 2先置cache C，再直连本实现；子typed-stop映射为列表框停止并阻断列表内容、row-limit发布、纵向面板及最终cache A/B。主帧适配层只把列表框的两次字体调用映射到既有字体样式操作。

定向测试覆盖三项基础几何、当前分类variant与X、四次动作顺序、两次字体寄存器链、共享记录完整清零、面板更新失败仍继续、矩形固定参数、资源陈旧高word、九宫格参数、signed frame回绕、未夹值EAX、动作/矩形/九宫格三类typed-stop，以及唯一caller正常、子stop传播与reserved零调用。验证：定向测试、AddressSanitizer、Linux core 188/188、Linux app 194/194 全部通过。源码零warning；app仅保留既有ALSA开发库CMake warning。

当前缺少原版四次动作更新联合状态、字体callee、面板动作更新、矩形/九宫格framebuffer与EAX/ECX/EDX联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
