# 战斗组A角色物品效果应用 `0x0046F1F0`

状态：`platform_adapted`。完整LST、16项跳表、typed实现、重置owner、唯一caller归属、定向测试、AddressSanitizer、Linux完整门与inventory双生成均已关闭。

## 1. 完整权威范围与ABI

权威LST主体为`0x0046F1F0..0x0046F560`，从proc到endp共410行，其中392个非标签物理行、226条实际指令、12个call、17个跳转、18个局部标签、2个返回点，没有外部`FUNCTION CHUNK`。紧邻主体后的16项跳表`0x0046F564..0x0046F5A0`也已逐项核对。

函数是thiscall，唯一栈参数为32位物品效果类型，两个出口均以`retn 4`弹栈。唯一caller预先连续压入下一callee参数和本函数参数；本函数只消费栈顶效果类型，下面的预压值留给紧随其后的caller内callee，不能误判为第二参数。

入口先读角色word `+0x2F12`。为零时以当前效果类型调用内嵌资料物品id查询并把AX写回；随后以完整32位效果类型减21做unsigned范围检查。只有21至36进入16项跳表，范围外在可选缓存查询后直接返回该差值。

## 2. 类型21至30、32与33

类型21把角色dword `+0x26CC` OR 1，把word `+0x2A6C`写1，刷新进度乘数，执行公共物品数量变化，最后把byte `+0x2A9B`写1。该最后写点位于公共callee返回之后。

类型22与25把word `+0x2A6C`清零，只在EAX低byte分别OR 2和16后写回完整状态dword，同时把byte `+0x2A87` OR 4，并把word `+0x2A70`分别写22和25。类型23、26、27、29、30把word `+0x2A6C`写对应类型，并分别在状态dword OR 4、32、64、256、512；29与30原指令只修改BH，但结果等价于对应完整mask。

类型24先只在状态低byte OR 8，写word `+0x2A6C`为24并刷新进度乘数。类型28、32、33只写对应word `+0x2A6C`并刷新乘数。所有有效类型最终都以固定玩家物品链token、当前类型和数量5调用公共物品数量变化callee；其返回EAX/ECX/EDX就是本函数最终寄存器。

## 3. 类型31派生值

类型31把byte `+0x2A87` OR 4，清word `+0x2A6C`，把word `+0x2A70`写31并刷新进度乘数。随后把live u16乘数乘4，以signed magic乘法向零除100并固定加1，将低word写入角色`+0x29A4`。虽然输入非负，typed实现仍保留原乘积高位、算术右移、符号修正和公共callee前的EAX/EDX寄存器。

## 4. 类型34至36

三类先刷新进度乘数。类型34与36先把live u16逻辑右移1，再减去该半值的五分之一，故新乘数为原值约40%；类型35直接把live u16向零除5。三类均只把低word写回共享进度乘数owner。

之后分别读取角色基础记录signed word `+0x0A`、`+0x0C`、`+0x08`。派生结果均为`向零除100的新乘数乘记录值 + 向零除10的记录值`，低word分别写入角色`+0x29A6`、`+0x29A8`、`+0x29AA`。实现以32位回绕乘法、signed高位、magic常量和两次符号修正逐指令复现，不替换为浮点或扩大精度。

基础记录token为零时，停止点位于乘数已缩减并写回之后、对应signed word首次解引用处。类型34与36保留零扩展新乘数、零token及缩减商；类型35保留零token、除法符号修正与零扩展新乘数。

## 5. 状态owner、callee与caller边界

角色`+0x26DC`继续复用唯一`LegacyBattleActorProgressState::progress_multiplier`，角色基础记录继续复用组A配置owner。新增物品效果owner只保存缓存物品id、效果状态、动作与显示word、模式byte、激活byte及四个派生word。

角色重置callee的完整权威写点已确认只清本owner中的`+0x26CC`、`+0x2A6C`和首个派生word `+0x29A4`；startup在同一重置边界只同步这三项，缓存、显示、模式、激活及后三个派生word均保持陈旧值，不做现代化全清零。

三个直接callee仍属后续待审函数：内嵌资料物品id查询、按资料刷新进度乘数、按角色资料改变玩家物品数量。当前各自保持带明确寄存器与副作用的窄typed port，不提前宣称其内部链扫描关闭。

全程序唯一caller位于`audit_order=187`的角色最终处理函数。本工作包确认其只传一个效果类型，并机械锁定前后栈与寄存器；该caller整体仍是组A帧中的opaque边界，按未审计caller留到所属工作包的规则不在第180项提前拆开或重复执行本效果。

## 6. 验证状态

单元测试覆盖首次actor访问typed-stop、缓存命中与缺失、unsigned默认分支、全部16种有效类型、六类状态mask、动作和显示word、模式与激活byte、三类乘数刷新、公共数量变化参数、类型31派生、类型34至36正负signed记录、基础记录typed-stop、最终寄存器及startup只清三项的陈旧状态保留。

独立位级脚本对类型31执行65,536组全u16向量，对三类乘数缩减执行131,072组向量，对signed记录派生执行1,114,112组交叉向量，合计1,310,720组，均与x86 signed乘积高位、算术右移和符号修正逐项一致。

验证结果：定向测试与独立AddressSanitizer均为`1/1`通过；Linux core为`188/188`，Linux app为`194/194`，源码零warning，app仅有既有ALSA提示。inventory连续双生成逐字节一致，稳定为`180/422 = 171 platform_adapted + 9 assembly_exact + 242 pending_audit`，SHA256为`6461117e3146c8594d8e1972c7b3d827868d07ab290a210c719225ed4029f10f`。原版组A角色对象、动态内嵌资料、玩家物品链、三个待审callee副作用及第187项caller寄存器联合捕获后端缺失，动态差分登记为`blocked_runtime_oracle`。
