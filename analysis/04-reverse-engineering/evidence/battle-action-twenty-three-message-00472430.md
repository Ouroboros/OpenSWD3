# 战斗动作二十三消息码消费 `0x00472430`

状态：`platform_adapted`。完整LST、typed实现、caller回收、定向测试、AddressSanitizer、Linux完整门与inventory双生成均已关闭。

权威LST主体为`0x00472430..0x004724C2`，proc至endp共79行、49条实际指令、3个call、5个跳转、5个局部标签、3个返回点，没有外部`FUNCTION CHUNK`。唯一真实caller是动作dispatch的case 23，紧随已关闭的动作二十三双层演出。

函数先从显式group-B目标的资料记录读取`+0x86`消息码；零值立即返回`0x61A8`且不刷新行动者。非零时以动作号23刷新当前group-A行动者`+0x26DC`百分比。百分比不小于100直接接受；否则消费一次bound 10随机数，以`percent / 25`作无符号商，随机值不小于商时相减，否则夹为零。调整值无符号不小于资料`+0x88`阈值时返回零并保留消息码；其余路径返回原消息码并破坏性清零`+0x86`。

目标资料`+0x86/+0x88`建立为dispatch内每个group-B目标的唯一typed owner，行动者百分比归入既有action-execution actor owner。未审计百分比刷新与旧随机发生器保持窄port；资料指针getter不再作为整函数opaque边界。case 23 caller改为typed调用并继续消费低word执行原消息与物品路径。

测试覆盖零码sentinel早退、非零码actor typed-stop、随机拒绝保留、随机接受清零、百分比100短路、caller消息/物品路径及旧地址零调用。定向测试与独立AddressSanitizer均通过；Linux core为`188/188`，Linux app为`194/194`，源码零warning。inventory连续双生成逐字节一致，稳定为`215/422 = 206 platform_adapted + 9 assembly_exact + 207 pending_audit`，SHA256为`defe4cdd4cf11bc2a2d536d703e48f59f6939f82ec772d5c6ff33ca4743dc4a0`。动态差分因原版目标资料、行动者百分比刷新、随机状态与caller寄存器联合捕获后端缺失而登记为`blocked_runtime_oracle`。
