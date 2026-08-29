# 战斗角色bit13资源显示查询 `0x00470F70`

状态：`platform_adapted`。完整LST、typed实现、alternate grid caller回收、定向测试、AddressSanitizer、Linux完整门与inventory双生成均已关闭。

## 1. 完整权威范围

权威LST主体为`0x00470F70..0x00470FD5`，proc至endp共60行、36条实际指令、2个call、4个跳转、2个返回点，没有外部`FUNCTION CHUNK`。callee为已关闭资源链头提交`0x00470900`一次，以及平台字符串复制`lstrcpyA`一次。

## 2. 遍历、计数与异常occurrence

入口无条件提交next资源链头，计数器从零开始，固定测试node `+0x4C` word bit13。每轮先从current节点读取next并破坏性写回current；next为零时返回0。bit13命中才递增计数，但每个非空node之后都无条件比较计数与occurrence。

因此occurrence为零时，如果第一个候选未设置bit13，零计数会立即相等并陈旧成功；如果之前已遇到bit13，后续未设置节点不会重置计数。实现保留该比较顺序。负值或超出范围的occurrence持续扫描到空链。

## 3. 输出与owner

命中后复制node `+0x0C`字符串，并以16位加法把secondary word `+0x08`与tertiary word `+0x0A`相加写入输出word，保留负数位型和回绕。成功EAX为1；失败EAX为0。

资源链、节点、名称和字段继续由`LegacyBattleActorListQueryState`唯一持有。唯一caller是已关闭alternate grid frame `0x00465E50`。production行初始化后直接调用typed查询，结果写入既有20-byte行文本与row value owner；脚本化grid port仅由显式compat flag启用，默认关闭。原始caller的local字符串缓冲区为20 bytes；typed路径在名称连终止符无法容纳时于原始复制边界停止，不再静默截断。

## 4. 验证状态

测试覆盖bit13第N项选择、secondary与tertiary 16位回绕求和、occurrence零未标记首项陈旧成功、缺项破坏性扫到空链、20-byte字符串边界typed-stop，以及alternate grid production两次查询和一行显示集成。定向测试与独立AddressSanitizer均为`1/1`通过；Linux core为`188/188`，Linux app为`194/194`，源码零warning。inventory连续双生成逐字节一致，稳定为`201/422 = 192 platform_adapted + 9 assembly_exact + 221 pending_audit`，SHA256为`83d366d03421276e5ce1e0580e59cdce4a115028c0d4493d763a5d12f5afb40a`。动态差分因原版bit13资源链、名称缓冲区、数量word和caller寄存器联合捕获后端缺失而登记为`blocked_runtime_oracle`。
