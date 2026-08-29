# 战斗组A角色内嵌物品资料应用 `0x0046F030`

状态：`platform_adapted`。完整LST、八项跳表、typed实现、上一属性汇总caller、定向测试、AddressSanitizer、Linux完整门与inventory双生成均已关闭。

## 1. 完整权威范围与ABI

权威LST主体为`0x0046F030..0x0046F1CC`，从proc到endp共225行，其中213个非标签物理行、146条实际指令、2个call、6个跳转、12个局部标签、9个返回点，没有外部`FUNCTION CHUNK`。紧邻主体后的八项跳表`0x0046F1D0..0x0046F1EC`也已逐项核对。函数是thiscall，唯一栈参数为0xA4内嵌物品资料token，所有出口均以`retn 4`弹栈。

入口把资料i16 `+0x48`减50并以unsigned范围检查选择八项跳表；范围外直接返回该低32位差值。只有精确50至57进入专用分支。

## 2. 六类状态位

资料类型50、53、54、55、56、57分别读取角色dword `+0x26C8`，只在AL执行OR `0x01,0x04,0x08,0x10,0x20,0x40`，再把完整EAX写回。其余三字节保持，EAX返回新完整状态，ECX保持角色token，EDX保持入口。

状态使用属性汇总记录中的唯一typed owner。startup较早的角色重置调用已由其权威写点确认把该dword写零，因此typed caller在同一重置边界同步清唯一owner；属性应用本身只OR，不自行重置。

## 3. 类型52的word调整

类型52读取资料u16 `+0x50`，以固定玩家物品链调用待审数量查询callee。查询返回AX先逻辑右移1；非零时以角色基础记录u16 `+0x26`乘该比例并向零除100得到附加值。随后再次读取live `+0x26`，固定计算10%，把`原值 + 10% + 附加值`的低word写回。

全部乘法保持原32位范围和magic除100结果。正常返回EAX是第二次magic乘法低dword，ECX是符号修正临时值，EDX从内嵌资料token高word与旧角色word低word拼接后再累加10%及附加值。不能把EDX简化为新word或角色token。

## 4. 类型51的首个非零byte调整

类型51先以资料u16 `+0x50`查询玩家物品数量，再从资料byte `+0x92..+0x9A`顺序扫描九项。九项全零时不访问角色基础记录，返回EAX 8、ECX 9，EDX只把查询返回EDX低byte替换为最终零。

遇到首个非零byte后立即停止扫描。原指令先对源byte取八位二补数，与signed byte 10相乘，再只取乘积低byte作signed扩展；乘物品数量并向零除100后，又只保留商低word。该u16与signed magic常量相乘并向零修正，等价于对该低word取负十分之一。最终只把delta低byte回绕加到角色基础记录`+0x2D+index`，然后返回；后续byte绝不处理。

正常EAX保留第二次magic乘法低dword并只以更新后角色byte覆盖AL，ECX返回命中索引，EDX返回完整signed delta。实现保留两次低位截断和由负商转u16造成的大幅负delta，不现代化为普通百分比。

## 5. typed-stop与callee边界

资料token为零或owner缺失时，在首次资料`+0x48`读取处停止并保留入口EAX/ECX/EDX。六类状态分支只在首次角色状态访问停止。类型51/52在数量查询完成后才首次读取角色对象中的基础记录token；角色token缺失时保留查询返回寄存器。

基础记录token缺失的停止点依分支保持原顺序：类型52比例非零时先把EAX收窄为比例、ECX发布零token并清EDX；比例为零时在后续word读取处保留查询返回ECX/EDX与右移后的EAX。类型51九byte全零不停止；首个非零byte完成全部算术后才在对应角色byte读取处停止。

`sub_4779F0`本身位于后续`audit_order=271`，当前只收窄为固定玩家物品链与item id的数量查询port，不提前关闭其链扫描行为。

## 6. caller回收

全程序唯一静态caller位于上一项16槽物品属性汇总的槽7/8公共分支；同一callsite在固定16轮中实际执行两次。调用前EAX为对应内嵌资料token，ECX/EDX均为角色token。typed汇总在复制0xA4资料并执行可选item id覆盖后直接调用本实现，子stop保留此前七或八槽的复制、累加、诊断与副作用。

旧整函数opaque枚举槽保留为reserved且生产零调用；startup只继续转发待审数量查询这一窄边界。两个typed结果随属性汇总结果发布，后续槽和最终寄存器仍由上一函数按原循环覆盖。

## 7. 验证状态

纯函数测试覆盖范围外类型、六种状态mask、类型52数量奇数右移、固定10%、附加百分比、资料token高word返回、类型51首个非零扫描、两次低位截断、九项全零、回绕byte、全部真实访问typed-stop和寄存器。属性汇总回归覆盖槽7/8两次直连及共享状态累积；startup回归覆盖角色重置清零、窄数量查询参数与旧opaque零调用。独立位级脚本对类型51执行102,816组向量、对类型52执行524,288组向量，均与x86高低乘积及符号修正逐项一致。

验证结果：定向测试与独立AddressSanitizer均为`1/1`通过；Linux core为`188/188`，Linux app为`194/194`，源码零warning，app仅有既有ALSA提示。inventory连续双生成逐字节一致，稳定为`179/422 = 170 platform_adapted + 9 assembly_exact + 243 pending_audit`，SHA256为`c0013b8f0767ddb8f6d379c478c0eade88fd17dbe537000054268d6835ca7304`。原版组A角色对象、动态内嵌资料、玩家物品链、数量查询callee与caller寄存器联合捕获后端缺失，动态差分登记为`blocked_runtime_oracle`。
