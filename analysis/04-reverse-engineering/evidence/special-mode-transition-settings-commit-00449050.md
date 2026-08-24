# 特殊模式转场设置提交 `0x00449050`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。物理范围`0x00449050..0x004490B2`，55行、2个call，无FUNCTION CHUNK。直接caller为48840，另由43B480发布到callback槽。

函数先计算`progress-1`。progress1只把enabled写3并返回0。其余阶段继续得到`progress-5`；非5直接返回该残值且无副作用。

progress5先缓存auxiliary与source，随后立即把progress写1；再查询服务48，并只使用原AL布尔值。最终按严格参数顺序格式化：sample index、surface index、spacing、固定capacity100、服务布尔值、source、auxiliary。返回格式化helper的EAX。

48840外部激活分支删除`exit_transition_settings` opaque端口，直接调用typed提交函数；状态写回与两个helper调用顺序均可观察。SDL仅保留服务查询和格式化两个最窄平台边界。

UT覆盖progress5的完整九参数等价顺序、服务返回0x101只向格式化传1、progress先回1、格式化返回值透传；另覆盖progress1写末项及progress2返回-3且无helper。定向交互UT验证48840 caller直连。

workpack双生成稳定为`153/227`，SHA256均为`613beb69656ff2f14626ef6cde3b82465661b1c905282336835c0d2bbcad36e5`；下一单元`0x004490C0`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
