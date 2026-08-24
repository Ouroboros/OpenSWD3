# 护驾记录链筛选与排序 `0x00441F70`

状态：`assembly_exact`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。物理范围`0x00441F70..0x0044201D`，110行，无callee、无FUNCTION CHUNK；caller为41160与42050。

函数入口无条件清destination head与offset8 word，保留offset4 sort key及其他字。随后以source link owner遍历：

- 每个节点读取`dword_49E344[filter_index]`，计算`node.flags & mask`，再精确清结果bit15（原`and ch,7Fh`），与原mask比较。mask自身带bit15时因此永不匹配，保留原BUG。
- flags匹配后才读取`word_499CD4[party.low16]`，要求与节点offset46 category按位相交。
- 不匹配节点只推进source link；匹配节点从source原地拆下，source前驱不推进。
- destination插入扫描保持原双条件：`current.key >= moved.key && previous.key < moved.key`。previous为destination sentinel时读取其offset4 sort key。因此普通正key升序，同key新节点插到已有同key之前；key不满足sentinel条件时保留原尾插行为。
- 插入只重写offset0 link，不复制、不分配、不释放节点。

`GuardianFilterDestination`显式表达head/sort-key/reserved/reset-word。动态filter/party表分别只在原读取点检查：入口先清destination；filter表越界保留source；party表仅在flags匹配后typed-stop。

41160的原opaque交换已拆为prepare/complete窄边界；caller在两者之间直接调用本helper，filter表归guardian state，结果destination head直接成为record head。42050留待其独立工作包关闭。

UT覆盖source拆链、升序、重复key前插、sentinel字段保留、offset8清零、bit15 BUG、两张表的原位置typed-stop，以及41160 prepare→41F70→complete直接链和后续列表重建。定向测试通过。

workpack双生成稳定为`83/227`，SHA256均为`8d969954138bb5093fab7fee97b07da9d248ef2265e12b5f94072370f7547e7a`；下一单元`0x00442050`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
