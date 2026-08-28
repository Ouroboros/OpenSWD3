# 战斗脚本关闭与状态重置 `0x0046E260`

状态：`platform_adapted`。完整主块、外部FUNCTION CHUNK、typed重置、两个caller、验证和inventory双生成均已收敛。

## 1. 完整权威范围

主块为`0x0046E260..0x0046E285`，外部`FUNCTION CHUNK`为`0x0046E390..0x0046E489`；两部分合计119行、53条实际指令、2个call、3个跳转和1个返回点。chunk属于本函数，不能计入物理相邻函数。

主块先读取共享FIGTALK句柄。句柄不等于`0xFFFFFFFF`时调用`CloseHandle`并把句柄写回全1；无论是否关闭都清文件已开门，然后跳入外部chunk。

## 2. chunk精确重置集合

chunk先把帧门与脚本完成门写1，再以零寄存器按权威宽度清理：

- 四个连续dword脚本辅助值；
- 两个独立i32工作值；
- X/Y坐标dword、两组坐标word；
- packed actor两个word、等待参数word、等待状态低word；
- 两个packed值各自两个word、四个独立word；
- list count、动态等待dword、page offset、一个辅助dword和文件已开门。

固定frame value写`0xFFFF`。函数没有清动态命令token、value A、对象token、文字offset、frame-after-move门、completion门或动态对象容器；typed实现逐项保留这些未写状态。

最后读取脚本base。base为零时直接返回，并保留当前cursor；base非零时先释放，再把base和cursor同时清零。返回EAX在零base路径为0，释放路径保留释放callee返回；两个已关闭caller都不消费该返回值。

## 3. typed实现与caller回收

`ScriptRunner::shutdown_script_direct`直接复用`LegacyBattleAssets`、`LegacyBattleScriptWorkspace`和`LegacyBattleScriptSharedState`唯一owner。平台固定数组以`script_capacity != 0`代表live分配：只有live时才将容量、实际长度和cursor清零；数组字节保留但不可访问，不模拟宿主free。持久Win32句柄已由RAII文件适配，无额外CloseHandle端口。

两个caller分别是终止opcode与case1完整帧返回非1路径。终止opcode保持组B、组A、等待word、全局重置、脚本shutdown的顺序并返回0。case1先保存入口cursor，再执行双方清理、全局重置和shutdown；shutdown清零cursor后仍按原ESI语义写回`入口cursor+4`，设置完成门并传播帧返回2或3。旧枚举值改为reserved槽，后续数值不平移且生产零调用。

## 4. 验证状态

终止回归用非零值污染权威清理字段和应保留字段，验证两组角色清理后只清精确集合、低word清理保留等待状态高word、固定frame value、脚本容量和page offset归零。case1回归验证帧返回2时shutdown后cursor仍为入口加4、完成门置位并原样传播2。定向资产/setup测试、AddressSanitizer、Linux core `188/188`和Linux app `194/194`全部通过，源码零warning。

inventory生成器连续双跑逐字节一致，正式计数为`166/422 = 157 platform_adapted + 9 assembly_exact + 256 pending_audit`，SHA256为`81364312d136d0458c0e02d45666f9d87beb28d6f50b57a1f3cae320e2c531ff`。原版持久句柄、CloseHandle结果、动态分配地址和释放返回缺少联合捕获后端，`original_diff_verified`登记为`blocked_runtime_oracle`。
