# 特殊模式转场配对模式推进 `0x0044A0D0`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。物理范围`0x0044A0D0..0x0044A151`，69行、2个call，无FUNCTION CHUNK。直接caller为44A050，另由43B480等安装为callback；callee为44AB00与485610。

函数读取mode word低16，按u16先加1再对4取模，并立即只写回mode低16、保留high16。以28字节步长检查四个记录首word；值FFFF的槽继续取下一模4候选，首个非FFFF候选写回mode低16。四槽全FFFF时原程序无限循环；typed实现完成四项原始域检查后以`unavailable_mode_domain_stopped`停止，保持首次候选已经写回的副作用，不调用后续helper。

找到候选后先调用44AB00重建，再以sample id `0x107`及原sample owner调用485610，丢弃重建返回并透传音效helper EAX。44A050原opaque cycle端口已删除并直接调用本typed helper；callee停止状态立即向caller传播，target循环仍按44A050自身LST只计一次44A0D0调用。

UT覆盖mode high16保留、从3推进到0、连续跳过两个FFFF后选2、44AB00先于音效、sample参数及返回透传、四槽全FFFF时保留首次候选0且零helper；既有44A050 UT改为真实typed推进，覆盖target1、target4不可达和初始等于target仍do-while四次。

workpack双生成稳定为`160/227`，SHA256均为`1a58cd1edd388c1b3852f53d67de597dd801ecc55f535fba714058189efdccfe`；下一单元`0x0044A160`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
