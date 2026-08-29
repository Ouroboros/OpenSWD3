# 战斗组A profile 模式选择 `0x0046FF00`

状态：`platform_adapted`。完整LST、四门选择逻辑、共享owner、随机低word分支、定向测试、AddressSanitizer、Linux完整门与inventory双生成均已关闭。

## 1. 完整权威范围与ABI

权威LST主体为`0x0046FF00..0x0046FFD0`，从proc到endp共95行，其中92个非标签物理行、58条实际指令、1个call、8个跳转、3个局部标签、4个返回点，没有外部`FUNCTION CHUNK`。函数是无栈参数thiscall，四个出口均为普通`retn`。

入口先依次检查actor两项跳过dword、物品效果dword、actor identity是否等于共享last identity；任一门命中均返回EAX 0。identity比较前EAX先清零再只写AX，ECX发布共享last identity。

## 2. 直接状态位路径

通过四门后先检查actor `+0x26C8`状态bit0。命中时只把actor profile mode byte写1并返回EAX 1；不清共享完成计数，也不发布last identity。该状态继续复用第179项内嵌资料应用的唯一owner。

物品效果dword继续复用第180项owner；其任意非零值都在identity读取前阻止profile选择。

## 3. 完成计数路径

未命中状态bit时，ECX取共享profile阈值并加8，EDX取共享完成计数低byte。若`counter >= threshold + 8`，函数写profile mode 1、清完成计数、把actor identity零扩展写入共享last identity并返回1。最终ECX仍是阈值加8，EDX仍是清零前计数。

若计数小于阈值加8且低byte小于8，直接返回0，不调用随机。ECX/EDX保留阈值加8和计数。

## 4. 随机路径

计数至少8但仍未达到确定阈值时，以固定上界10调用随机callee。只比较返回AX；AX不大于5时走公共零返回，EAX被清零但保留callee ECX/EDX。AX大于5时写profile mode、清计数、以零扩展actor identity覆盖ECX并发布last identity，返回EAX 1，EDX保留callee值。返回EAX高word不参与比较。

随机callee仍属外部通用随机边界，当前以固定上界和完整入口/返回寄存器窄port保留。

## 5. owner与caller

actor profile mode与identity继续放在第184项每actor动作执行owner；profile阈值、完成计数和last identity放在同一共享owner。两项跳过状态由未来caller从胜利奖励唯一owner传入，物品与内嵌状态分别借用第180和179项owner，无第二份物理状态。

全程序唯一caller位于待审`audit_order=187`角色最终处理函数；当前机械确认它在第180项效果调用后紧接执行本选择器。按未审caller留到所属工作包的规则，本工作包不提前拆整个角色最终处理opaque边界，也不重复执行该调用。

## 6. 验证状态

单元测试覆盖首actor typed-stop、两项跳过、物品效果、重复identity、状态bit直接选择、确定阈值选择、计数小于8、随机AX等于5和高word非零低word6成功，并逐项断言profile mode、计数、identity与EAX/ECX/EDX。

验证结果：定向测试与独立AddressSanitizer均为`1/1`通过；Linux core为`188/188`，Linux app为`194/194`，源码零warning，app仅有既有ALSA提示。inventory连续双生成逐字节一致，稳定为`185/422 = 176 platform_adapted + 9 assembly_exact + 237 pending_audit`，SHA256为`5bbcff8346c51a72f94294b4442da14e3e6d9b8edc606d85c076f0f4ab96c866`。原版组A actor状态、共享随机计数、随机callee及第187项caller寄存器联合捕获后端缺失，动态差分登记为`blocked_runtime_oracle`。
