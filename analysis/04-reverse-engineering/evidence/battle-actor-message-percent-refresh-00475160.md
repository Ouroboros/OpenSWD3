# 战斗行动者消息百分比刷新 `0x00475160`

状态：`platform_adapted`。本页以完整`Swd3.exe.lst`机器码与指令为行为真值；反编译、命名和未关闭callee仅用于导航。

## 静态审计

权威主体为`0x00475160..0x00475172`，proc至endp共11行、9条实际指令、1个call、0个跳转、0个局部标签、1个返回点，没有外部`FUNCTION CHUNK`。唯一caller为message phase `0x00466F70`内的`0x0046726F`，唯一callee为尚未审计的`0x00482F10`。

函数保存ESI，把入口ECX行动者token同时保存为ESI和callee this指针，向`0x00482F10`压入固定参数30。callee返回后，函数以`mov ax,[esi+0x26DC]`只覆盖EAX低16位；EAX高16位、ECX和EDX均保留callee返回状态，随后恢复ESI并返回。因行动者字段访问发生在callee之后，typed owner缺失只能在callee调用及其已发布副作用之后停止。

未审`0x00482F10`继续保留为窄typed port token。其reply可发布更新后的`+0x26DC`消息百分比，再由本函数按权威LST从typed行动者owner读取该word并覆盖AX；不得把整个`0x00475160`继续作为opaque调用，也不得把最终读取现代化为返回值直传或EAX全宽覆盖。

## Typed实现与caller回收

新增`LegacyBattleActorMessagePercentRefreshPort/Request/Result`与`refresh_legacy_battle_actor_message_percent`。实现固定callee token和参数30、保留入口及返回寄存器、接收callee百分比发布，并在原版后置字段访问点执行typed-stop。`+0x26DC`复用既有`LegacyBattleGroupAActionExecutionState::message_percent` owner，不增加重复物理状态。

message phase消息99的唯一caller改为typed直连。调用前仍保留原版EAX=`actor_index * 3021`、ECX=group-A行动者token、EDX=`actor_index * 5`；helper返回EAX继续作为后续group-B item resolver的第一个参数。旧`query_actor_resource`整函数opaque槽被收窄并改名为`refresh_actor_message_percent`，frame coordinator保持原枚举数值位置，只转发尚未关闭的`0x00482F10` callee。

## 测试与oracle

独立单元测试覆盖固定callee token/参数、入口寄存器、callee字段发布、EAX高16位保留、AX从`+0x26DC`覆写、ECX/EDX返回、typed owner缺失和零legacy token均在callee之后停止并保留callee寄存器。message phase回归覆盖唯一production caller、行动者token、固定参数30、原寄存器快照、发布后的百分比向resolver传递以及nested结果与调用计数。

动态差分登记为`blocked_runtime_oracle`：当前缺少原版group-A行动者完整对象、`0x00482F10`内部查找与`0x004779F0`副作用、message phase caller前后寄存器及`+0x26DC`联合捕获后端。该阻塞不影响完整LST静态闭环、typed实现和Linux验证。

定向battle测试为`1/1`，AddressSanitizer为`1/1`且无AddressSanitizer或LeakSanitizer finding，Linux core为`188/188`，Linux app为`194/194`，全部构建日志零warning。inventory连续双生成逐字节一致，稳定为`233/422 = 224 platform_adapted + 9 assembly_exact + 189 pending_audit`，SHA256为`6e03c5b7714886623eb36f34c9d4217dcd356abcc529ab0a7cfca172d9764b52`。
