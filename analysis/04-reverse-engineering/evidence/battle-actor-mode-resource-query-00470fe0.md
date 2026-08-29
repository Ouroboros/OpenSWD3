# 战斗角色模式资源显示查询 `0x00470FE0`

状态：`platform_adapted`。完整LST、typed实现、mode grid caller回收、定向测试、AddressSanitizer、Linux完整门与inventory双生成均已关闭。

## 1. 完整权威范围

权威LST主体为`0x00470FE0..0x00471070`，proc至endp共87行、51条实际指令、2个call、9个跳转、7个局部标签、2个返回点，没有外部`FUNCTION CHUNK`。callee为已关闭资源链头提交`0x00470900`一次，以及平台字符串复制`lstrcpyA`一次。

## 2. 两种筛选模式与陈旧计数

入口无条件提交next资源链头，之后每轮从current节点读取next并破坏性写回current。next为空返回0。

mode为零时固定比较resource id `0x0300`。命中该id直接进入输出门，不写selected token，也不检查occurrence；非固定id不增加计数，但仍比较零计数与occurrence。因此occurrence为零可在首个非固定节点陈旧成功并写selected token，正occurrence则持续扫描直到固定id或空链。

mode非零时仅当node `+0x2F` byte bit3置位且resource id不是`0x0300`时递增计数，也就是typed owner中的category mask bit27。每个非空节点之后都无条件比较计数与occurrence，因此occurrence为零仍可在首个不合格节点陈旧成功；固定id节点也可能在陈旧计数相等时被选中。普通计数命中先把节点token写入selected resource owner。

## 3. 输出门、寄存器与typed边界

候选成功后测试node `+0x46` byte mask `0x05`。mask为零时函数仍返回1，但不复制名称、不写数量，caller原有局部值保持陈旧。mask非零时复制node `+0x0C`名称，并以16位加法把secondary word `+0x08`与tertiary word `+0x0A`相加写入输出word，保留负数位型和回绕。扫描期间ECX固定为`0x0300`、EDX持有occurrence；发布数量时仅替换EDX低word。

唯一caller是已关闭mode grid frame `0x00466190`，共有primary mode零与secondary mode一两处调用。caller的`String1`位于`-0x14..-0x01`，容量为20 bytes；typed路径在名称连终止符无法容纳时于原始复制边界停止，不静默截断。资源链、selected token、名称和数量继续由`LegacyBattleActorListQueryState`唯一持有。

## 4. caller回收与验证

mode grid production现在从startup party span定位角色owner，primary与secondary两条查询直接调用typed实现；调用前后的两次已关闭资源链头提交也改为typed直连。尚未审计的`0x00471080` secondary count保持窄port，脚本化mode grid兼容仅由单测显式开启，production默认关闭。

测试覆盖mode零固定id直达且不改selected、mode一bit27计数并排除固定id、occurrence零陈旧选择、输出mask零保持陈旧值、16位数量回绕、缺项破坏性扫到空链、20-byte字符串边界typed-stop，以及mode grid production primary/group重复显示与两次typed链头提交。定向测试与独立AddressSanitizer均为`1/1`通过；Linux core为`188/188`，Linux app为`194/194`，源码零warning。inventory连续双生成逐字节一致，稳定为`202/422 = 193 platform_adapted + 9 assembly_exact + 220 pending_audit`，SHA256为`9c4944438acbe1b483f72b9df3a9383903af867abb8d37f9371d4b94c02604a5`。动态差分因原版mode资源链、selected token、caller字符串/数量局部和寄存器联合捕获后端缺失而登记为`blocked_runtime_oracle`。
