# 特殊模式资源提交 `0x00448360`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。物理范围`0x00448360..0x004485E4`，286行、12个call，无FUNCTION CHUNK。唯一code caller为`0x00446700`。

函数以216字节临时记录owner表达原malloc/清零/free生命周期，初始化记录内部owner后加载`selected_row+0x47`资源；无论分支均把offset40写232B，把选中source的offset04/08低4位清零，并对offset10 OR 8000。

加载后offset48非零走已有资源分支：先清offset48/74，动作参数末值固定1000。世界记录从索引1扫描；action id匹配时先对offset10 AND 3FFF，释放旧动作，再OR 10000000，并以action id、低2位mode、0、1刷新。可匹配多个记录；返回0。

加载后offset48为零走新资源分支：写`selected_row*4+4`和source mode低2位，动作参数末值使用ArgList signed word；随后完成临时记录。世界记录从索引1寻找首个同action id槽，命中则覆盖，否则按当前count追加；最后按复制后offset10低2位初始化目标记录并返回1。原固定数组写域由typed vector owner隔离，不更改分支顺序。

446700模式11不再调用opaque `finalize_mode_resource`，而是直接调用本helper。返回0保留原JZ短路：interaction mode已先写10，不进入后续退出；返回1继续写2并按mode10 availability执行原退出链。

UT覆盖已有资源重配、固定1000、世界记录索引1起扫、多字段mask、旧动作释放、状态刷新、返回0；覆盖新资源追加、匹配槽覆盖、动态offset48、source mode、signed ArgList bit pattern、返回1及446700回归。独立ASan通过。

workpack双生成稳定为`140/227`，SHA256均为`2ce33d8ac1d2df78779be08412b6a0cfa64f6f20fcb0c9ddae9a42c9755ca8ad`；下一单元`0x004485F0`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
