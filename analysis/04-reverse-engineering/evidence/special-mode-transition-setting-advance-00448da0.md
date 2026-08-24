# 特殊模式转场设置向后调整 `0x00448DA0`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。物理范围`0x00448DA0..0x00448E8F`，144行、3个call，无FUNCTION CHUNK。唯一引用为43B480的callback数据槽。

progress=1复用四段selector预增语义：owner超过3时夹3，返回夹前低字节。其他非5 progress返回`progress-5`低字节。

progress=5按signed velocity分派六行。row0增加sample index并在>11时夹11，再调用样本2E；row1增加surface index，但原程序在>11时异常夹10，再调用激活；row2先计算spacing+40作为返回低字节，写回后>=140时owner夹140；row3只调用服务48启用；row4减少source owner，<=0时夹0，并同步当前source token，返回夹后值；row5增加auxiliary，>11时owner夹11但返回夹前低字节。selector不在0..5时返回其原低字节且无副作用。

UT覆盖六行owner、helper参数和调用顺序；重点覆盖sample 11保持11、surface 11写回10、spacing 140写回140但返回180低字节、source 0保持0、auxiliary 11写回11但返回12，以及progress1和无关progress残值。SDL设置port保持最小平台适配。

workpack双生成稳定为`150/227`，SHA256均为`2dffdcef45c81e43e6207e5a2a77676adb0215f294a680e14c7e8a99eb57e303`；下一单元`0x00448EB0`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
