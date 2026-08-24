# 打开增加道具页面时建立299项清单 `0x0044AF30`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。物理范围`0x0044AF30..0x0044B004`，94行、2个物理callsite，无FUNCTION CHUNK。由444FC0安装到两个callback槽；callee为487C10与40DC50。

函数只把mode word低16写5并保留high16，随后依次清三个早期owner，申请256字节目录缓冲并清零。目录容量按原大小固定为128个u16。

扫描item id `0xE75..0xF9F`，共299项。每项调用40DC50，只有返回精确等于1才写入；返回2等其他非零值不写。写入值为`item_id-0xE74`，因此范围为1..299。正常路径执行1次分配和299次查询，共300次helper。

若分配返回null，原程序在紧随其后的memset失效；typed实现只在该清零点停止，保留mode、三个早期owner清零及null owner发布，不清旧entries或entry count。

若第129个item命中，原程序开始越过256字节缓冲。typed实现保留该次presence查询，在原u16写入点以`capacity_stopped`停止；此前128项、count和所有查询副作用保留，函数尾部owner清理与shared值发布不执行。全命中时停止于item `0xEF5`，helper count 130。

完整扫描后先snapshot共享值，再按原顺序清8个窗口owner与余下3个主owner，发布共享值并以其EAX返回。

UT覆盖三个精确命中及一个返回2排除、299次首尾查询、存储1/3/299、所有尾owner清零、shared发布和返回；另覆盖分配失败保留旧列表，以及全命中第129项停止、128项既有列表和尾owner未清语义。

workpack双生成稳定为`167/227`，SHA256均为`025cf8d67e5731d01e832a55dcc677d0fc82ec31be700c019b6abfb3aa65a6a1`；下一单元`0x0044B010`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
