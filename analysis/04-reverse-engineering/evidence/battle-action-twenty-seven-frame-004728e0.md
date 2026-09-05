# 战斗动作二十七效果累计与三层逐帧演出 `0x004728E0`

状态：历史工作包220为`platform_adapted`。工作包282正在回收坐标callee；历史门禁不验证本轮改动，当前发布门尚未完成。

权威LST主体为`0x004728E0..0x00472C60`，proc至endp共379行、244条实际指令、13个call、13个跳转、10个局部标签、2个返回点，没有外部`FUNCTION CHUNK`。唯一caller是动作dispatch case 27。

函数以行动者`+0x338`主动作记录写入profile action、variant 48与special mode，先置`+0x2AAC` latch，再执行动作更新与frame查询。frame发布后复制原x偏移、flags和辅助x；`+0x2B08`为一时翻转flags低位并按frame宽度镜像两项x。目标坐标callee的第二输出以signed word减record y偏移，frame高度发布三分之一与四分之一。`+0x2B00`为一时三项运动值取负一，否则取负六。

sample参数保留frame token高半和record sample word低半。声像根据signed行动者x减signed镜像偏移是否大于等于320，右侧使用当时EAX高半与正16，左侧使用sample callee返回EDX高半与负16。随后清sample word，第一层绘制使用行动者x减镜像偏移、`record y + signed relative y - height/3`及修改flags；第二层使用相同x、行动者y减完整32位record y及镜像后flags。

行动者action flags低字节bit0或bit3任一非零时，函数清整word并把`+0x267C`完成门置为`0x8000`。随后刷新目标、计算signed AX效果值，仅在值大于等于9999时上夹值；负值保持不变。该值写入`0x0053AE8C` owner，并以完整32位回绕累加到既有`0x0052441C` pair-primary owner，再依序调用值发布与目标属性窄port。

完成门等于`0x8000`后，函数只写行动者`+0x630`第二动作记录的action id和variant零，保留其余陈旧字段，再调用待审第二记录callee。随后重新发布frame源并以行动者`+0x29BC/+0x29BE`执行第三层绘制。只有主记录完成位等于一时才清完成门，先清第二条152字节记录，再清主记录，并返回一；其余路径返回零。

实现复用既有`primary_action_record`与`effect_action_record`，以及现有完成门、运动模式和最后效果值owner；`0x0052441C`直接复用现有pair-primary owner。已关闭动作更新、frame provider与坐标查询typed直连；音频、绘制、目标刷新、效果计算/发布和第二记录处理保持窄port。typed-stop位于原frame发布、shared owner与镜像门访问点，保留此前record、latch、动作更新、frame查询和token副作用。case 27不再调用整个旧地址。

## 工作包282坐标调用回收

`0x004729E7`的X输出为`var_8`低WORD，Y输出为`var_C`低WORD；两个DWORD局部此前已清零。
`0x004729DF`把X输出地址装入EDX；EAX不能统一替换为某个输出地址：

- 不镜像时保留`0x0047296E`读出的完整主记录mode flags。
- 镜像且辅助X为零时保留`0x004729AC`读取的原frame指针。
- 镜像且辅助X非零时，`0x004729C9/0x004729CD`只替换AX为帧宽减辅助X，保留frame指针高WORD。

两个局部token和原frame EAX快照由同一dispatch context显式传入；未捕获的默认零不构成原寄存器证据。
原frame EAX快照与现有兼容frame句柄不得混同。原版动态frame与栈地址仍需联合捕获。
查询停止保留latch、帧发布、镜像字段、首WORD与到达寄存器，阻断共享绘制参数、sample、效果累计和第二记录处理。
成功路径继续按原WORD解释Y，两个已清零DWORD的低WORD输出也传给既有效果计算边界，不接受旧opaque回复的任意高WORD。
新增矩阵覆盖三种EAX来源、两条坐标分支、门/第二读停止、成功值和根dispatch传播；第十五轮core定向`1/1`通过，日志无编译诊断。
随后从入口复核全部出口，发现完成分支`0x00472C46 rep stosd`应耗尽ECX，而原C++错误保留最后绘制callee的ECX。
现已修正，并补充非零绘制ECX/EDX回复测试，要求正常清理后ECX为零而EDX保持。
第十轮ASan定向`1/1`通过，日志无编译或sanitizer诊断；当前全量发布门尚未完成。

## 历史工作包220验证记录

测试覆盖镜像偏移、陈旧sample高半、左右声像、三层坐标、负效果不夹值、32位累计、完成门、第二记录初始化、非完成保留、完成双记录清零、production caller及旧地址零调用。定向测试与独立AddressSanitizer均通过；Linux core为`188/188`，Linux app为`194/194`，源码零warning。inventory连续双生成逐字节一致，稳定为`220/422 = 211 platform_adapted + 9 assembly_exact + 202 pending_audit`，SHA256为`34f686fa2e25bca2074d491a38e22e61c23d1e5265c57633bdb9b7ff6a9145e2`。动态差分因原版行动者/目标、两条动作记录、frame、坐标、音频、绘制、效果callee和caller寄存器联合捕获后端缺失而登记为`blocked_runtime_oracle`。
