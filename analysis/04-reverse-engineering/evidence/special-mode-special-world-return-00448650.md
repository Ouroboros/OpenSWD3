# 特殊模式世界转场返回 `0x00448650`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。物理范围`0x00448650..0x004486F3`，76行、5个call，无FUNCTION CHUNK。code caller为4070A0与409540，均归尚未关闭的外部owner，暂不回收。

函数先发布return mode owner 43，释放当前active inventory根；随后把485F0保存的inventory clone换为active root，把typed selection clone head换回当前记录head，并立即清空两个clone owner。platform adaptation将原裸选择链地址收紧为`LegacyStandardModeForwardNode*`，不是不可恢复的整数token。

随后直接复用已关闭BCC0，以visible limit 13重算总数、窗口offset、local cursor、visible count、visible head与共享文本。按LST顺序又直接调用B9C0，以`window_offset+local_cursor`从head重取选中记录，再调用B9E0重复解析共享文本。缺失记录与MAPS解析失败均只在原读取点typed-stop，clone换回和窗口副作用不回滚。

参数原值作为缺省返回；只有精确等于1时，调用inventory mutation对2D9执行delta=-1、mode=0，并以该返回覆盖EAX。其他值不消费。

UT覆盖active inventory释放、两份clone换回及owner清零、return owner 43、13行窗口重建、visible head、FFDC文本路径，以及参数1精确消费2D9。独立ASan通过。

workpack双生成稳定为`142/227`，SHA256均为`11d26d6dc1d0d930e9da4e9dbe1492c89b66fe75b64e31ad9fc16344cbcbe1ef`；下一单元`0x00448700`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
