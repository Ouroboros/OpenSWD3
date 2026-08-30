# 战斗奖励值百分比缩放 `0x00472C70`

状态：`platform_adapted`。完整LST、typed实现、四处caller回收、定向测试、AddressSanitizer、Linux完整门与inventory双生成均已关闭。

权威LST主体为`0x00472C70..0x00472CD0`，proc至endp共48行、30条实际指令、2个call、1个跳转、1个局部标签、2个返回点，没有外部`FUNCTION CHUNK`。四个真实caller都位于战斗效果协调器`0x0045C010`，处理group-B当前行动者与pair-primary或逐槽feedback值。

函数先读取行动者`+0x26C8` byte的bit4；未置位时不调用callee、不改percent和值并返回零。置位时固定调用百分比刷新callee，参数55；再调用动作配置callee，参数为固定源token、55和12。随后对行动者`+0x26DC` unsigned word原地逻辑右移一位。

值计算先以32位`imul`低半得到`value * halved_percent`的回绕结果，再把该32位结果解释为signed整数，以magic乘法实现signed除以100、向零截断，最后加一并写回原dword。该顺序意味着高位值会按signed乘积缩放，不能改成64位或unsigned现代化计算。值指针的原始访问位于两个callee与percent破坏性减半之后；typed-stop必须保留这些副作用。

实现新增group-B专属`LegacyBattleRewardScaleActorState`唯一owner，包含status byte与percent word，并由现有action-dispatch state持有八槽数组。effect coordinator通过frame coordinator现有action-dispatch引用借用该owner，不复制状态。四处`publish_reward` caller全部typed化；缩放返回一且写回值signed大于零时才继续发布固定奖励ID、值、模式和feedback。旧整个`0x00472C70` opaque调用已移除，两个未审callee保留窄port。

测试覆盖bit门零调用、percent破坏性减半、固定callee参数、32位乘法回绕、signed除法向零、加一、value原访问点typed-stop、actor typed-stop、group-B非完成group-effect production caller及旧地址零调用。定向测试与独立AddressSanitizer均通过；Linux core为`188/188`，Linux app为`194/194`，源码零warning。inventory连续双生成逐字节一致，稳定为`221/422 = 212 platform_adapted + 9 assembly_exact + 201 pending_audit`，SHA256为`73fbce77d3a4b1d8548cf973447e0c9805888e04bf4c3c7bc9e17bc7395e21a3`。动态差分因原版group-B行动者status/percent、值槽、两个callee与四处caller寄存器联合捕获后端缺失而登记为`blocked_runtime_oracle`。
