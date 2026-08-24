# 特殊模式转场选择首项 `0x00448C70`

状态：`assembly_exact`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。物理范围`0x00448C70..0x00448C92`，25行、0个call，无FUNCTION CHUNK。唯一引用为43B480的callback数据槽。

函数先以`progress-1`判断progress=1；命中时直接把enabled写0，并返回减法残值0。否则继续从该EAX减4，等价于`progress-5`；命中progress=5时把velocity写0并返回0。其他progress不改状态，直接返回`progress-5`残值。

UT覆盖progress1写enabled0/返回0、progress5写velocity0/返回0，以及无关progress6返回1且不修改selector。

workpack双生成稳定为`148/227`，SHA256均为`0422067bb10d9f0a05570a31a01465f2b2595b66a037d1eb491c862a88a2bd71`；下一单元`0x00448CA0`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
