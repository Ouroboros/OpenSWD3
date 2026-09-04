# 战斗角色目标准备 `0x00464CC0`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`、`callers_reclaimed`。

## 1. 完整权威范围

权威LST主体为`0x00464CC0..0x00464D96`，从proc到endp共95行、73条实际指令、4个静态call、5个跳转、3个局部标签、1个`retn`，没有外部`FUNCTION CHUNK`。

两个静态caller分别位于已关闭组A逐角色帧与已关闭选择帧。四个原始call中，secondary RNG已由typed随机端口直连，`0x00478330`已由共享owner直连；仅组B完成查询callee仍通过窄端口。

## 2. 角色发布与首次真实访问

入口参数是group-A actor code。函数按原顺序：

1. 把actor code写入`0x0053BD50`已提交角色owner；
2. 计算u32 `index = actor_code - 8`；
3. 把选择动作写1、角色提交门写1；
4. 把共享动作workspace的`10 + index`项写1；
5. 以`index * 0xBCD`形成组A物理token，并把完整dword参数1直接写入共享availability owner。

`0x0053BD50`此前在debug状态和target runtime中保留了两份typed存储；本工作包删除runtime副本，以debug状态内语义化的`committed_actor_code`作为唯一owner。目标刷新、撤退提交、调试清理和global reset全部改读写该owner。

workspace按u32地址公式映射真实`0x0053AF30`物理数组。此前debug状态还保留相同地址前十项副本；本工作包删除该副本，调试全清与global reset都改为只清动作owner前十项。actor code 7的index为全1，`10 + index`回绕到物理第9项，先完成该写，再在组A基址前一对象的首次准备call停止；不提前拒绝。只有映射结果首次超出共享workspace时停止，并保留此前三项全局发布。

准备call前寄存器固定为EAX=`index*0xBCD`、ECX=组A token、EDX保留caller值。index 10可先写workspace第20项，再在第11个组A对象call停止。

## 3. 处理数门与随机起点

typed写入返回后，函数读取live group-B count到EAX，并把共享`opponent_processed_counter`低byte零扩展到ECX。比较使用signed dword：`processed >= count`直接返回；负count不会进入随机调用。

只有正signed count大于processed时，直连既有secondary RNG执行`random(count)`，不夹值也不增加重试上限。结果加1后立即发布为one-based group-B actor code，再按真实one-before基址构造对象token。第一次对象call前寄存器为：

- EAX=`code*0x565`；
- EDX=`code*0x345`；
- ECX=`0x005229E0 + code*0x2B28`。

code 0或9在首次真实对象查询停止；已提交角色、动作workspace、动作/提交门与published code均保留。

## 4. 跳过已完成目标

首次group-B完成查询不等于1时直接返回callee寄存器。等于1时进入原循环：

1. live published code加1并先写回；
2. 按signed `code > group-B count`回绕到1；
3. ESI加1；
4. signed `ESI >= count`时返回，不再查询；
5. 否则按相同乘法寄存器形状查询下一对象；返回1继续，其他值结束。

循环每次重新读取live count与published code；不快照、不增加八对象现代上限。全部目标完成时，完成恰好count次查询后返回，published code已再次轮到起点，EAX为live count，ECX为当前code，EDX保留最后callee值。

## 5. caller回收与验证

选择帧完成角色路径删除旧opaque准备调用并直接组合本实现；子typed-stop保留五项选择指针清理、message/cache/runtime清理、target gate与animation phase发布，并阻断后续group-B重置和actor-order交换。原选择帧callee枚举值保留为reserved且零调用。

组A逐角色帧在queue mode等于1时也直接组合本实现。主帧在进入角色序列前把唯一action、final actor和target runtime owner注入其dispatch context；组A caller不再写自身同名副本。子typed-stop阻断后续角色帧流程，旧callee token零调用。

定向测试覆盖：actor code 7的workspace前邻接写与组A停止、workspace首次越界、processed/count signed门、负count、随机one-based寄存器、回绕后找到首个未完成目标、全部完成的count次退出、第九个组B对象stop、选择帧caller共享发布与组A caller直连。验证：定向测试、AddressSanitizer、Linux core 188/188、Linux app 194/194 全部通过。源码零warning；app仅保留既有ALSA开发库CMake warning。

## 6. `0x00478330`组A准备写入

工作包278关闭`0x00464D04`唯一物理call。actor code大于7时，caller先把角色写入queued owner，再以EAX构造组A对象地址并把完整dword `1`写入同一对象`+0x2AE4`；该对象直接复用最终角色状态的availability owner。caller在call前没有改写EDX，因此写停止时EAX为1、ECX为角色token、EDX保留函数入口值，并阻断后续组B完成查询循环。选择帧与组A逐角色帧两个上层caller均传播该停止，不制造suffix。

当前缺少原版组A对象、组B对象完成查询callee、两组对象共享副作用及EAX/ECX/EDX联合动态捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
