# G08首项初始化 `0x00445430`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。物理范围`0x00445430..0x00445599`，148行，无FUNCTION CHUNK。无code caller；FC0把它安装到二级初始化表索引11。

函数先发布`entry_count-1`、初始化word5，并直接复用Act记录初始化后写动作232A/variant7。随后发布首项横坐标30，按LST顺序清六个前置owner、窗口范围480及列表offset/local，再调用尚未关闭448230的记录初始化端口。

记录初始化成功后，函数把可用动作数清0，依次查询物品30、31、32、33并按非零各加一；然后以`offset+local`直接复用B9C0选择记录和B9E0发布共享文本。文本成功后写布局宽96、模式2、发布横坐标，分配40字节workspace，最后清六个布局owner和两个尾部owner，发布workspace token并返回该token。

448230只保留最窄typed端口。记录初始化停止保留此前动作、坐标及前置清零；选择缺失只在B9C0原读取点停止；文本失败保留四项计数和选中记录副作用；三类停止均不写布局或workspace。

UT覆盖entry_count无符号预减、Act部分重置、首项动作键、六前置/六布局/两尾部owner、四物品非零计数、FFDC文本、布局与40字节workspace，以及记录初始化、空选择和文本停止前缀。

workpack双生成稳定为`123/227`，SHA256均为`f60dc00d130b5291b146fd6177a3e14d597e802122cf93b85659c94f208a2f7b`；下一单元`0x004455A0`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
