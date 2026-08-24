# 切换到上一名队员并重新计算属性 `0x0044A160`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。物理范围`0x0044A160..0x0044A1C7`，54行、2个call，无FUNCTION CHUNK。由43B480安装为callback；callee为44AB00与485610。

函数从mode word低16开始，每轮先按u16减1。LST随后把零扩展值与`0x000FFFF0`比较；零扩展u16永远不可能命中，该疑似拼写BUG必须保留，不能改成FFFF。随后`and eax,0x80000003`在当前非负域等价保留低2位，形成0..3反向循环。

以28字节步长检查四个记录首word，FFFF继续后退；首个非FFFF候选才写回mode低16并保留high16。四槽全FFFF时原程序无限循环且从未写回mode；typed实现完成四项原始域检查后以`unavailable_mode_domain_stopped`停止并保持入口mode不变。

选中后严格先调用44AB00重建，再以sample id `0x107`和sample owner调用485610，丢弃重建返回并透传音效EAX。两个未关闭callee维持共享typed端口。

UT覆盖从mode0后退、槽3为FFFF后选择槽2、high16保留、重建先于音效、sample参数和返回，以及四槽全FFFF时mode原值不变且零helper。

workpack双生成稳定为`161/227`，SHA256均为`96833fbf1cef1be3a0c64c9e1d563498e1620b7dd7f79a6efdc3174be46f5128`；下一单元`0x0044A1D0`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
