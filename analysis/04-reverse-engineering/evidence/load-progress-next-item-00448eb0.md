# 在读取进度列表中选择下一项 `0x00448EB0`

状态：`assembly_exact`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。物理范围`0x00448EB0..0x00448ED2`，24行、0个call，无FUNCTION CHUNK。唯一引用为43B480的callback数据槽。

函数先计算`progress-1`。只有progress=1时才预增enabled；结果超过3时owner夹3，但返回夹前残值。其他progress不修改owner，直接返回`progress-1`。该函数不处理progress5六行设置，与48BB0的双阶段后移职责不同。

UT覆盖enabled3预增后owner仍为3但返回4，以及progress5返回4且enabled保持2。

workpack双生成稳定为`151/227`，SHA256均为`43ceabff8cc5c4e47553a51f13fced5327e55f272c2a501e986f263e33ddd777`；下一单元`0x00448EE0`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
