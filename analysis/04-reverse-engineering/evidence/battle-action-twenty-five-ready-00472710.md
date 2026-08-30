# 战斗动作二十五资料就绪壳 `0x00472710`

状态：`platform_adapted`。完整LST、typed实现、caller回收、定向测试、AddressSanitizer、Linux完整门与inventory双生成均已关闭。

权威LST主体为`0x00472710..0x00472722`，proc至endp共12行、5条实际指令、1个call、0个跳转、0个局部标签、1个返回点，没有外部`FUNCTION CHUNK`。唯一caller是动作dispatch case 25。

函数以显式目标调用纯getter读取目标`+0x0C`资料token，把返回值写回即将由`retn 4`丢弃的参数栈槽，然后无条件返回一。该死栈写和getter返回均不被caller观察；唯一可观察边界是目标资料访问本身。

实现复用已建立的每个group-B目标资料typed owner：资料owner缺失时在原访问点typed-stop，存在时严格返回一，不虚构额外副作用。case 25 caller直接消费typed结果，不再调用整个旧地址。测试覆盖资料缺失、常量成功返回、普通switch caller及旧地址零调用。定向测试与独立AddressSanitizer均通过；Linux core为`188/188`，Linux app为`194/194`，源码零warning。inventory连续双生成逐字节一致，稳定为`217/422 = 208 platform_adapted + 9 assembly_exact + 205 pending_audit`，SHA256为`422395e4f1f60d05994409676b038da1d6e943e8f8a8fb9a6dfcc8911604ebbd`。动态差分因原版目标对象`+0x0C`与caller寄存器联合捕获后端缺失而登记为`blocked_runtime_oracle`。
