# 打开角色属性页面时分配当前值和加成值两份记录 `0x00449FF0`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。物理范围`0x00449FF0..0x0044A022`，26行、2个显式call，无FUNCTION CHUNK。唯一引用为444FC0安装的callback槽；末尾以tail jump进入44AB00。

函数先只检查mode word低16位。精确等于5时仅清低16位，保留high16；其他值完全不改。随后严格按顺序请求两份56字节owner，第一份写first owner，第二份写second owner。两次分配无条件完成后执行尾分派，并透传44AB00返回值。

原malloc允许返回null且函数仍继续第二次分配及尾分派，因此typed实现不对null owner停止、回滚或伪造成功。分配和44AB00分别保持为最窄生命周期端口；44AB00未关闭，不提前实现其职责。

UT覆盖`0xABCD0005`只清为`0xABCD0000`、两次size56顺序、第一份非空/第二份null仍执行一次尾分派、返回值透传与helper计数；另覆盖低16为6时完整mode word保持不变。

workpack双生成稳定为`157/227`，SHA256均为`7a9d669249cc967cc69f9795a0ae2cc360860b3359f7295e4d9999f228e6dac6`；下一单元`0x0044A030`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
