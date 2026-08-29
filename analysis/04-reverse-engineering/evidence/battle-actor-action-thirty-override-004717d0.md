# 战斗角色动作三十覆盖位查询 `0x004717D0`

状态：`platform_adapted`。完整LST、typed实现、目标选择caller回收、定向测试、AddressSanitizer、Linux完整门与inventory双生成均已关闭。

## 完整权威范围

权威LST主体为`0x004717D0..0x004717DD`，proc至endp共6行、4条实际指令、0个call、0个跳转、1个返回点，没有外部`FUNCTION CHUNK`。函数以ECX为角色对象，只用`mov ax`读取角色`+0x2A86`低word，再右移十三位并与一，最终EAX严格为零或一；ECX与EDX不变。

## typed实现与caller

字段归入既有每角色`LegacyBattleGroupAActionExecutionState::action_override_flags`唯一owner。typed getter只在原始word读取位置对空owner停止；非空时返回bit13，不把相邻位现代化为布尔状态。

唯一caller为目标选择刷新`0x00462740`的动作三十提交路径。它在已关闭资源释放之后，对已提交group-A角色调用本getter；命中时把动作改写为十三、置special gate并递增special action count，未命中时停止固定effect sample。production已删除旧runtime `query_action_thirty_override`边界并直接读取typed owner。

测试覆盖bit13置位、bit13清除且其他位全置、空owner原访问点typed-stop，以及production动作三十caller命中后发布动作十三且不再调用旧opaque查询。定向测试与独立AddressSanitizer均通过；Linux core为`188/188`，Linux app为`194/194`，源码零warning。inventory连续双生成逐字节一致，稳定为`208/422 = 199 platform_adapted + 9 assembly_exact + 214 pending_audit`，SHA256为`9683f3206166bfd7a3823be063ba3df678a87b4e4e15a6d06b3a76e4fa304563`。动态差分因原版角色flags word与目标选择caller寄存器联合捕获后端缺失而登记为`blocked_runtime_oracle`。
