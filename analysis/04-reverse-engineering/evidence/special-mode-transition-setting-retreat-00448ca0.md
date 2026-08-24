# 特殊模式转场设置向前调整 `0x00448CA0`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。物理范围`0x00448CA0..0x00448D82`，139行、3个call，无FUNCTION CHUNK。唯一引用为43B480的callback数据槽。

progress=1复用四段selector预减语义：owner负值夹0，返回夹前低字节FF。其他非5 progress返回`progress-5`低字节。

progress=5按signed velocity分派六行。row0预减sample index，结果<=0时owner和helper参数均为0，再调用样本2E；row1对surface index同样处理并调用激活；row2先计算spacing-40作为返回低字节，写回后<=60时owner夹60；row3只调用服务48禁用；row4增加source owner，>4时夹4，并同步当前source token，返回夹后值；row5预减auxiliary，负值时owner夹0但返回夹前FF。selector不在0..5时返回其原低字节且无副作用。

UT覆盖六行owner、helper参数和调用顺序；重点覆盖spacing 60写回60但返回20、source 4保持4并返回4、auxiliary 0写回0但返回FF，以及progress1和无关progress低字节残值。SDL设置port保持最小平台适配。

workpack双生成稳定为`149/227`，SHA256均为`b38bb387117364cb5ebbe56eddadfb45c81c56a5458698473242b0b35eb113c9`；下一单元`0x00448DA0`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
