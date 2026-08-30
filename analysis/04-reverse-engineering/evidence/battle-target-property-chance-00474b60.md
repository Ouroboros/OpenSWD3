# 战斗目标属性百分比判定 `0x00474B60`

状态：`platform_adapted`。完整LST、typed实现、唯一caller回收、定向测试、AddressSanitizer、Linux完整门与inventory双生成均已关闭。

权威LST主体为`0x00474B60..0x00474B9B`，proc至endp共30行、22条实际指令、1个call、0个跳转、0个局部标签、1个返回点，没有外部`FUNCTION CHUNK`。唯一caller位于动作dispatch的动作33分支：目标就绪、解析成功且object bit5未置后，先查询编号33的百分比word，再调用本函数；返回一才启用模式七、发布presentation、目标索引与frame刷新并清framebuffer。

函数无条件调用既有bounded random owner，bound固定为100，并把完整返回EAX保存后才读取唯一参数。参数先按32位回绕乘70：LST使用`7*x`、`35*x`、左移一组合，typed实现不得提升到64位。随后以signed乘法魔数链得到按100向零截断的商；C++实现对同一wrapped i32直接除100，保留负值向零截断。

原函数只对商的低word加10，高word保持商的原bit；随后以unsigned word比较threshold与随机返回低word。`cmp dx,si`后的`sbb/inc`严格产生零或一，等于阈值时接受。正常返回EAX为判定值，ECX保留wrapped `70*x`，EDX保留signed商但低word已加10。

测试覆盖参数零的最低阈值十、随机值等于十接受、十一拒绝、参数一百得到阈值八十并等值接受，以及全一参数乘70回绕为负七十、signed除法向零得到零、EDX低word加十和ECX回绕寄存器。production动作33 caller验证typed随机调用、接受分支、目标发布、frame刷新及旧地址零调用。定向测试与独立AddressSanitizer均通过且findings为零；Linux core为`188/188`，Linux app为`194/194`，源码零warning。inventory连续双生成逐字节一致，稳定为`228/422 = 219 platform_adapted + 9 assembly_exact + 194 pending_audit`，SHA256为`c4ae7d8ef735de6e05aa6af58103cb3c62f74a78b30a1c54393b202154c5f323`。动态差分因原版CRT随机流、百分比query与唯一caller寄存器联合捕获后端缺失而登记为`blocked_runtime_oracle`。
