# 装备物品动作数量初始化 `0x00444F60`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。物理范围`0x00444F60..0x00444FA3`，38行，无FUNCTION CHUNK。callers为已关闭442E40模式初始化和43A60 party循环，两处均已直接回收。

函数先无条件把发布动作数写3。随后读取完整32位party selector，以u32环绕分别计算`selector*2+0x15`和`selector*2+0x16`，在原40DC50入参边界缩为u16。两次存在性结果均按非零判断并各自加一；返回值保持第二次40DC50的原始i32结果，而不是布尔化或最终计数。

最小平台边界为`LegacyStandardModeEquipmentActionCountPorts::query_equipment_item_presence`；初始化和party循环端口以虚基共享它。原两个可失败opaque初始化方法及其不可达停止状态已删除。helper result记录两次查询和第二次残值，状态直接发布到typed owner。

UT覆盖完整selector高位参与环绕、派生ID、负数与正数均按非零计数、初值覆盖、两次查询及第二次原值返回。442E40回归覆盖selector先从5归零后调用F60；43A60回归覆盖F60发布3后，后续444FB0失败仍保留该前缀；F40不再伪造F60改写selector的无进展路径。

workpack双生成稳定为`114/227`，SHA256均为`b87faef3140c269d8681fb18d23729da60892fd61e0d3f87c0e91bb4aad81c8b`；下一单元`0x00444FB0`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
