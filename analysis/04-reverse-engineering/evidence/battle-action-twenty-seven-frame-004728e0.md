# 战斗动作二十七效果累计与三层逐帧演出 `0x004728E0`

状态：`platform_adapted`。完整LST、typed实现、caller回收、定向测试、AddressSanitizer、Linux完整门与inventory双生成均已关闭。

权威LST主体为`0x004728E0..0x00472C60`，proc至endp共379行、244条实际指令、13个call、13个跳转、10个局部标签、2个返回点，没有外部`FUNCTION CHUNK`。唯一caller是动作dispatch case 27。

函数以行动者`+0x338`主动作记录写入profile action、variant 48与special mode，先置`+0x2AAC` latch，再执行动作更新与frame查询。frame发布后复制原x偏移、flags和辅助x；`+0x2B08`为一时翻转flags低位并按frame宽度镜像两项x。目标坐标callee的第二输出以signed word减record y偏移，frame高度发布三分之一与四分之一。`+0x2B00`为一时三项运动值取负一，否则取负六。

sample参数保留frame token高半和record sample word低半。声像根据signed行动者x减signed镜像偏移是否大于等于320，右侧使用当时EAX高半与正16，左侧使用sample callee返回EDX高半与负16。随后清sample word，第一层绘制使用行动者x减镜像偏移、`record y + signed relative y - height/3`及修改flags；第二层使用相同x、行动者y减完整32位record y及镜像后flags。

行动者action flags低字节bit0或bit3任一非零时，函数清整word并把`+0x267C`完成门置为`0x8000`。随后刷新目标、计算signed AX效果值，仅在值大于等于9999时上夹值；负值保持不变。该值写入`0x0053AE8C` owner，并以完整32位回绕累加到既有`0x0052441C` pair-primary owner，再依序调用值发布与目标属性窄port。

完成门等于`0x8000`后，函数只写行动者`+0x630`第二动作记录的action id和variant零，保留其余陈旧字段，再调用待审第二记录callee。随后重新发布frame源并以行动者`+0x29BC/+0x29BE`执行第三层绘制。只有主记录完成位等于一时才清完成门，先清第二条152字节记录，再清主记录，并返回一；其余路径返回零。

实现新增唯一`action_twenty_seven_record`、完成门、运动模式和最后效果值owner；`0x0052441C`直接复用现有pair-primary owner。已关闭动作更新与frame provider typed直连；坐标、音频、绘制、目标刷新、效果计算/发布和第二记录处理保持窄port。typed-stop位于原frame发布、shared owner与镜像门访问点，保留此前record、latch、动作更新、frame查询和token副作用。case 27不再调用整个旧地址。

测试覆盖镜像偏移、陈旧sample高半、左右声像、三层坐标、负效果不夹值、32位累计、完成门、第二记录初始化、非完成保留、完成双记录清零、production caller及旧地址零调用。定向测试与独立AddressSanitizer均通过；Linux core为`188/188`，Linux app为`194/194`，源码零warning。inventory连续双生成逐字节一致，稳定为`220/422 = 211 platform_adapted + 9 assembly_exact + 202 pending_audit`，SHA256为`34f686fa2e25bca2074d491a38e22e61c23d1e5265c57633bdb9b7ff6a9145e2`。动态差分因原版行动者/目标、两条动作记录、frame、坐标、音频、绘制、效果callee和caller寄存器联合捕获后端缺失而登记为`blocked_runtime_oracle`。
