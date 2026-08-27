# 战斗组B目标轮转 `0x00465090`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`、`caller_reclaimed`。

## 1. 完整权威范围

权威LST主体为`0x00465090..0x00465163`，从proc到endp共98行、59条实际指令、3个静态call、6个跳转、6个局部/返回标签、1个`retn`，没有外部`FUNCTION CHUNK`。

五个静态caller全部位于已关闭目标选择状态刷新。两个完成查询call和一个选择重置call复用该caller已有的group-B窄平台操作，不新增opaque边界。

## 2. 当前目标与首次查询

函数忽略caller EAX，先读取当前零基group-B目标和live group-B count。只有当前目标按i32 signed小于count时保持原值；否则立即把共享当前目标写0。count为0或负数也不提前返回，仍查询物理对象0。

首次完成查询保留独立寄存器形状：EAX=`index*0x159`、ECX=`0x00525508 + index*0x2B28`，EDX保持caller值。对象索引只在首次真实call停止，因此停止结果保留完整u32地址与乘法寄存器。

完成查询返回不精确等于1时，函数直接进入选择重置；只有完整EAX等于1才扫描后续目标。

## 3. live顺序扫描与耗尽行为

每轮都在完成查询之后重新读取live target cursor和group-B count。cursor先按u32加1并回写；signed结果大于live count时写1回绕。随后以该一基cursor读取`0x00520DF4`起的九dword物理顺序表，增加扫描计数并把结果发布为当前零基目标。

顺序表在首次真实读取停止；cursor、callee返回EDX和此前当前目标保持。扫描计数按i32与本轮count快照比较，不增加八对象现代上限，也不动态重读该轮count。

当扫描计数signed达到或超过count时，函数先把共享message owner `0x0053BCEC`写1，然后不再查询最后读到的候选，仍继续选择重置。未耗尽时，候选查询使用另一寄存器形状：EAX=`index*0x565`、ECX为group-B对象token、EDX=`index*0x159`；完整EAX等于1才进入下一轮。

## 4. 选择重置与返回寄存器

最终目标从共享当前目标重新读取。选择重置以栈参数1调用同一group-B对象，调用前EAX为零基index、ECX为对象token、EDX=`index*0x565`。对象越界在首次真实重置call停止，并保留此前message 1和当前目标。

重置普通返回后再次读取live当前目标，写selection input gate 1，再发布一基`current+1`到共享published actor。EAX返回该一基code；ECX/EDX完整保留重置callee结果。因此重置callee若改写当前目标，最终发布采用改写后的live值。

## 5. caller回收与唯一owner

目标选择状态刷新中的三个共享默认目标分支和message 7目标轮转均直接组合本typed实现；共享C++路径覆盖权威LST五个静态callsite。message 1公共尾只在pre-frame gate B为0且live共享message等于3时调用，普通返回后才预置输入记录。message 7删除旧的部分内联扫描，恢复选择重置、一基发布、message 3、动画缓存`0/0/4`与输入记录预置尾部。任一子typed-stop保留caller此前message、action与候选副作用并阻断后续尾部。

原`prepare_default_target`枚举数值改为`reserved_group_b_target_cycle_slot`，生产代码零调用。完整LST交叉扫描确认`0x0053BCEC`就是既有共享message owner，而不是独立目标轮转状态：启动、最终角色、输入、菜单、目标刷新和本函数的1/3/0x63/0x67等写全部继续复用`LegacyBattleSharedPhaseStatePort`单一存储，global reset只清一次。交叉审计同时确认`0x0053BFBC`就是既有final-actor pre-frame gate B；选择帧删除第二份suppression存储并直接复用该owner，global reset只清一次。

定向测试覆盖signed当前目标归零、count零仍查询、首次与循环查询的两套寄存器、live count/cursor重读、一基顺序回绕、全部完成时跳过最后查询但仍重置、重置后live当前目标重读、顺序表/首次查询/循环查询/最终重置四类typed-stop、五个caller路径、message 7尾部恢复、reserved槽零调用、共享message无副本及pre-frame物理owner的全局重置回归。验证：定向测试、AddressSanitizer、Linux core 188/188、Linux app 194/194全部通过。源码零warning；app仅保留既有ALSA开发库CMake warning。工作包稳定为`130/422 = 125 platform_adapted + 5 assembly_exact + 292 pending_audit`，连续双跑逐字节一致，SHA256为`45827505c66405dd7ed878f49107828225c07f42e53ed6e108865d5eeef3825f`。

当前缺少原版group-B对象、完成查询与选择重置共享副作用、五处caller联合动态轨迹及EAX/ECX/EDX捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
