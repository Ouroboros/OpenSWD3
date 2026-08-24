# 特殊模式项目目录释放与消息 `0x0044B010`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。物理范围`0x0044B010..0x0044B069`，46行、3个call，无FUNCTION CHUNK。直接caller为4070A0、44C0E0、44C160，另由444FC0安装为callback；callee为4885A0、40DC50、423AF0。

调用和读取顺序严格保留：

1. 以当前目录owner调用共享释放。
2. 释放返回后读取message tail低字节和shared value低字节；回调可修改二者，因此不能使用入口snapshot。
3. 两个字节已snapshot后才把目录owner写0。
4. 以service id 48查询40DC50；查询时必须已能观察到owner为0。
5. 查询返回后才读取sample owner、font和value三个低字节；查询回调可修改它们。
6. 以固定两个字符串、三个后读字节、容量100、完整service EAX、先前shared snapshot及tail snapshot调用九参格式化，并透传其EAX到typed result。

目录entries与entry count不在此函数清理。null owner也仍传给共享释放，不短路。

UT让释放回调把tail/shared改为AA/BB，让查询再改为CC/DD并把后三字段写55/66/77；验证消息使用AA/BB而非CC/DD、使用查询后的55/66/77、service完整返回、容量100、owner在查询前已清零、调用顺序release→query→format，以及原entries/count保持。

workpack双生成稳定为`168/227`，SHA256均为`fbe027c0e5a0b1b0d7db9fe5f13745722401fadef54409c4e3f78786792a2d3b`；下一单元`0x0044B070`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
