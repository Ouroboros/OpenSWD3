# 特殊模式转场选择前移 `0x00448C00`

状态：`assembly_exact`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。物理范围`0x00448C00..0x00448C3C`，38行、0个call，无FUNCTION CHUNK。唯一引用为43B480的callback数据槽。

progress=1时对enabled执行32位预减，把原始减法结果同时写owner和保留在EAX；若按signed比较为负，只把owner改0，EAX保持负值。因此owner原为0时最终仍为0，返回-1。

其他progress先计算`progress-5`作为EAX残值；不等于5时立即返回。progress=5时对signed velocity预减并写回；若结果为负，只把owner改0，EAX保持负值。因此原为0时最终owner为0，返回-1。

UT覆盖progress1零项的owner0/返回-1、progress5零项的owner0/返回-1，以及无关progress2返回-3且不修改selector。

workpack双生成稳定为`146/227`，SHA256均为`063c4e2890a822f5a874fec7d8731a58b053231addcddee23f7789da0eb19088`；下一单元`0x00448C40`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
