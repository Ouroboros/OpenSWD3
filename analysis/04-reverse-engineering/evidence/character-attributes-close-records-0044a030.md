# 关闭角色属性页面时释放两份临时记录 `0x0044A030`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。物理范围`0x0044A030..0x0044A04A`，20行、2个call，无FUNCTION CHUNK。直接caller为44A250，另由444FC0安装到callback槽。

函数严格先以first owner调用共享释放，再以second owner调用共享释放，并返回第二次释放的EAX。原程序不检查null，不因第一次返回值短路，也不在释放后把任一owner清零；typed实现保留两次调用与悬挂owner值。

复用49FF0的双缓冲生命周期state与port。44A250尚未关闭，不提前回收其调用；callback槽也不伪造运行时分派。

UT在first=1111、second=null条件下验证释放顺序`[1111,0]`、两次helper、第二次返回32透传，以及释放后两个owner保持原值。

workpack双生成稳定为`158/227`，SHA256均为`c96d567228a8040933b616d1391d72a2aa1cf6159891a563c677e112e6097858`；下一单元`0x0044A050`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
