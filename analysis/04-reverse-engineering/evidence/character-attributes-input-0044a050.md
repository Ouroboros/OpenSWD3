# 处理角色属性页面的队员选择和返回按钮 `0x0044A050`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。物理范围`0x0044A050..0x0044A0CE`，67行、3个call，无FUNCTION CHUNK。唯一引用由43B480安装到callback槽；callee为40DC50、44A0D0、44A250。

入口先snapshot flags低字节。仅`flags&3`非零且interaction mode精确为2时检查无符号坐标：`10 < X < 468`、`4 < Y < 188`。进入该门后EAX先加载X，因此任一坐标失败时函数低字节返回X低字节。有效网格以`(X-10)/110`计算target 0..4，查询item `target+30`；presence精确为0时立即返回0，跳过flags bit4分派。

presence非零后以do-while调用44A0D0并在每次调用后重读mode word低16，直至等于target。44A0D0的原始mode域为0..3；target可为4，且不可用表也可令callee不收敛。typed实现只在完成四项原始域检查后以`cycle_domain_stopped`停止，保留最后一次cycle副作用，不继续执行44A250或伪造成功。

循环成功或未进入网格分支后，在原`0x44A0BF`位置重读flags；bit4非零才调用44A250，并以其EAX低字节返回。44A0D0与44A250仍为独立待审计边界。

UT覆盖target1网格、item31、cycle回调修改flags后重读并提交、presence零短路、X/Y边界失败保留X低字节、无网格时bit4直接提交、target4完整四项域typed-stop，以及初始mode已等于target时仍先cycle并完成四次do-while回环。

workpack双生成稳定为`159/227`，SHA256均为`9576adb0edf860ab0451f154c897a8be4b60b71c14613ce7f6f57ba7aa5c10f4`；下一单元`0x0044A0D0`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
