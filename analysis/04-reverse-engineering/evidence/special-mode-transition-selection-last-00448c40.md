# 特殊模式转场选择末项 `0x00448C40`

状态：`assembly_exact`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。物理范围`0x00448C40..0x00448C62`，25行、0个call，无FUNCTION CHUNK。唯一引用为43B480的callback数据槽。

函数先以`progress-1`判断progress=1；命中时直接把enabled写3，并返回减法残值0。否则继续从该EAX减4，等价于`progress-5`；命中progress=5时把velocity写5并返回0。其他progress不改状态，直接返回`progress-5`残值。

UT覆盖progress1写enabled3/返回0、progress5写velocity5/返回0，以及无关progress4返回-1且不修改selector。

workpack双生成稳定为`147/227`，SHA256均为`bebadb4475657d285dc134879b3c8f9617b026c6e8a83d697e763b4c19388aa9`；下一单元`0x00448C70`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
