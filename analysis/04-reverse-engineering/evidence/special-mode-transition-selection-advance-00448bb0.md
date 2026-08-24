# 特殊模式转场选择后移 `0x00448BB0`

状态：`assembly_exact`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。物理范围`0x00448BB0..0x00448BF2`，40行、0个call，无FUNCTION CHUNK。唯一引用为43B480的callback数据槽。

progress=1时先对enabled执行32位递增并把未夹值保留在EAX；写回值大于3时只把owner夹为3，不改EAX。因此owner原为3时最终仍为3，返回4。

其他progress先计算`progress-5`作为EAX残值；不等于5时立即返回。progress=5时对velocity执行32位递增并保留未夹值；写回值大于5时把owner改4，不改EAX。因此原为5时最终owner为4，返回6。该“超过末项回到倒数第二项”行为按原BUG保留，不改成常规夹5或回0。

UT覆盖progress1末项的owner3/返回4、progress5末项的owner4/返回6，以及无关progress3返回-2且不修改selector。

workpack双生成稳定为`145/227`，SHA256均为`70c68145631eb68bd9dba061f14163a35042da537a3409b206180b3a57b28eef`；下一单元`0x00448C00`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
