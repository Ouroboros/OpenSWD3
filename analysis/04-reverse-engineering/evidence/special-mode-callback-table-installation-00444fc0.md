# 特殊模式回调表安装 `0x00444FC0`

状态：`assembly_exact`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。物理范围`0x00444FC0..0x004450DD`，98行，无FUNCTION CHUNK。code callers为已关闭439DE0全局初始化和3B480回调绑定，两处均已直接回收。

函数按地址顺序无条件写三张各7槽表：draw、initialization、cleanup，共21次写入。随后调用40DC50查询story flag49并保留原始i32返回。只有结果精确等于1时，分别重写三张表的槽4和槽5，共6次额外写入；非零但不等于1不能触发交换。

三表以typed u32目标地址数组归属共享`LegacyStandardModeCallbackState`；39DE0和3B480访问同一owner。最小平台边界只剩story flag查询。result记录原始返回、21或27次写入和一次查询。439DE0原`install_mode_callbacks` opaque端口和3B480原`initialize_secondary_dispatch`空端口均已删除。39DE0先直接调用FC0并计入一次嵌套flag查询，初始化三条action记录前缀后再按LST独立查询同一flag，因此总查询数为2；3B480 G08在写主callback槽前直接调用FC0并计入一次查询。

UT覆盖21项默认表、flag1三对交换及27次写入、负数原始返回、非1不交换、FC0查询发生在action记录初始化之前、439DE0第二次查询发生在三记录前缀之后、3B480 G08直接安装，以及39DE0后续18记录初始化。

workpack双生成稳定为`116/227`，SHA256均为`84304acb5f7d401f0e7e43b8f5882983a95cae0f6e6c7c169fac0f9bb544b87a`；下一单元`0x004450E0`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
