# 战斗角色动作四覆盖位查询 `0x004717E0`

状态：`platform_adapted`。完整LST、typed实现、目标选择caller回收、定向测试、AddressSanitizer、Linux完整门与inventory双生成均已关闭。

## 完整权威范围

权威LST主体为`0x004717E0..0x004717ED`，proc至endp共7行、4条实际指令、0个call、0个跳转、1个返回点，没有外部`FUNCTION CHUNK`。函数以ECX为角色对象，只用`mov ax`读取角色`+0x2A86`低word，再右移十二位并与一，最终EAX严格为零或一；ECX与EDX不变。高半EAX虽由caller保留，但在最终与一后不可观察，不能据此改写word读取顺序。

## typed实现与caller

本函数与`0x004717D0`共享同一物理word，继续使用既有每角色`LegacyBattleGroupAActionExecutionState::action_override_flags`唯一owner，不复制第二份状态。typed getter只在原始word读取位置对空owner停止；非空时严格返回bit12。

唯一真实caller位于目标选择刷新`0x00462740`的动作四路径。它先完成目标解析，再以既有group-A角色索引构造EAX和ECX后调用本getter。bit12命中时按目标效果值进入警告、行选择或直接动作十五路径；未命中时继续角色属性查询。production已删除旧runtime `query_action_four_override`边界，原枚举数值槽改为reserved且不平移后续ABI值。

测试覆盖bit12置位、bit12清除且其他位全置、空owner原访问点typed-stop，以及production动作四caller命中后进入第二行目标路径且不再调用旧opaque查询。定向测试与独立AddressSanitizer均通过；Linux core为`188/188`，Linux app为`194/194`，源码零warning。inventory连续双生成逐字节一致，稳定为`209/422 = 200 platform_adapted + 9 assembly_exact + 213 pending_audit`，SHA256为`3cf766089c6ed5cc1e7c5e48f32fa89e869be306d7ef2a38c907b0318dc295d8`。动态差分因原版角色flags word、目标解析callee与caller寄存器联合捕获后端缺失而登记为`blocked_runtime_oracle`。
