# 战斗组A角色16槽物品属性汇总 `0x0046EBB0`

状态：`platform_adapted`。完整LST、typed汇总、startup唯一caller、定向测试、AddressSanitizer、Linux完整门与inventory双生成均已关闭。

## 1. 完整权威范围与ABI

权威LST主体为`0x0046EBB0..0x0046EE52`，从proc到endp共265行、195条实际指令、2个call、18个跳转、10个局部标签和1个返回点，没有外部`FUNCTION CHUNK`。函数是thiscall，唯一参数是64-byte、16项记录指针表，由`retn 4`弹栈。

入口以82个dword清零角色`+0x158..+0x29F`的两份连续0xA4内嵌资料。参数表为零不是故障：函数仍执行固定16轮空循环并正常返回EAX/ECX零、EDX原this；不能把它改成提前返回。

## 2. 首槽资料与固定诊断

参数表非零时，槽0先把记录`+0x0C`起41个dword复制到角色`+0x0B4`主资料，再把记录u16 `+4`写到角色`+0x2F1A`。刚复制的主资料u16 `+0x48`，即原记录u16 `+0x54`，为零时调用公共诊断：窗口token、固定文字token、记录u16 `+4`、固定`manrole.cpp`源token和行号`0x182`；诊断后重载this并继续。

现代实现把世界物品节点的0xA0 definition snapshot与独立description compat token投影为原`+0x0C..+0xAF`的0xA4资料，不复制第二份角色物品物理状态。

## 3. 16槽公共word累加

每个槽都按原顺序重新读取live角色基础记录token，并把源记录word低16位直接加到角色基础记录；全部保持u16低位回绕，不夹值、不饱和：

- 源`+0x30`加角色`+0x26`；
- 源`+0x32`加角色`+0x28`；
- 源`+0x38,+0x3A,+0x3C,+0x3E`依次加角色`+0x16,+0x14,+0x18,+0x1E`；
- 源`+0x34`非零时，同时加角色`+0x10`和`+0x26`；
- 源`+0x36`非零时，同时加角色`+0x12`和`+0x28`。

以上word位于复制资料内的`+0x24,+0x26,+0x2C,+0x2E,+0x30,+0x32,+0x28,+0x2A`。零值只跳过`+0x34/+0x36`的两组附加写，六项公共写始终发生。

## 4. 槽7至10与其余槽

槽7和槽8完成公共word累加后，分别把源记录`+0x0C`的0xA4资料复制到角色`+0x158`和`+0x1FC`。源u16 `+4`不是`0xFFDC`时覆盖内嵌资料u16 `+0x50`；哨兵值则保留刚复制的原word。随后以对应内嵌资料调用待审`sub_46F030`。本工作包只把这两个callee收窄到专用port，不猜测其内部副作用。

槽9和槽10完成公共word累加后直接进入循环尾。其余12槽把源byte `+0x9E..+0xA6`依次加到基础记录`+0x2D..+0x35`，只保留u8低位回绕。

槽0至6在byte累加后额外检查源u16 `+0x48`。该word为零时，源u16 `+0x40,+0x42,+0x44`中的每个非零值分别累加角色`+0x2F1E,+0x2F20,+0x2F22`；槽7以后不执行这组三项。除槽7至10外，源u16 `+4`等于`0x039D`时把角色dword `+0x2B18`写1。

## 5. 返回寄存器与typed-stop

正常处理完16槽后，EAX是槽15记录token，EDX是原this。ECX高16位来自槽15记录token高16位；CH来自槽15源u16 `+0x36`高byte，CL来自槽15源byte `+0xA6`。这组陈旧寄存器拼接不现代化。

缺少角色聚合owner时在第一份内嵌资料清零前停止。16项表中任一nullable角色物品根缺失时，在该槽首次记录访问停止，保留此前槽的全部累加、callee和写入；表本身为零仍按原空循环成功。基础记录token为零时，槽0主资料复制、角色`+0x2F1A`和可选诊断已完成，当前源`+0x30`也已读取，只在第一次角色`+0x26`写入处停止。

## 6. shared owner与caller回收

唯一caller位于startup初始队伍第二轮。它以紧凑角色到原队伍槽映射选择`0x004C8AD0 + source * 0x40`，即共享`LegacyWorldItemListState::role_item_lists`中每角色16条sentinel根；typed caller直接借用这64条唯一物品链owner构造临时引用表。调用仍位于玩家/队伍物品排序之后、共享数值与资源双写之前。

旧整函数opaque槽保留为reserved且生产零调用；固定诊断和两次内嵌资料callee追加到枚举尾。角色`+8`资料token不再被旧opaque伪写为16槽表地址，而是复用前一配置函数已发布的辅助资料token及其唯一kind owner。子typed-stop阻断后续数值、资源、名称、比例和护援流程。

## 7. 验证状态

纯函数测试覆盖16槽公共word、两组条件word、12槽九byte、0至6槽早期三项、槽7/8复制与哨兵覆盖、槽9/10跳过、特殊编号latch、description token、固定诊断、空参数表、三类typed-stop以及最终EAX/ECX/EDX。startup回归覆盖64条共享角色物品根、固定诊断adapter、两次内嵌资料callee参数、辅助资料owner纠正、旧槽零调用和缺失sentinel停止。

验证结果：定向测试与独立AddressSanitizer均为`1/1`通过；Linux core为`188/188`，Linux app为`194/194`，源码零warning，app仅有既有ALSA提示。inventory连续双生成逐字节一致，稳定为`177/422 = 168 platform_adapted + 9 assembly_exact + 245 pending_audit`，SHA256为`35eb20034b71ca82216aba763330245915c0afd684be1fff71f97d1c3fa20970`。原版组A对象、64条动态角色物品根、真实资料与description指针、`sub_46F030`副作用、诊断和caller寄存器联合捕获后端缺失，动态差分登记为`blocked_runtime_oracle`。
