# 战斗行动者退却就绪位查询 `0x004728D0`

状态：`platform_adapted`。完整LST、typed实现、两处caller回收、定向测试、AddressSanitizer、Linux完整门与inventory双生成均已关闭。

权威LST主体为`0x004728D0..0x004728DF`，proc至endp共9行、5条实际指令、0个call、0个跳转、0个局部标签、1个返回点，没有外部`FUNCTION CHUNK`。两个真实caller分别位于退却提交`0x0045EA80`和输入dispatch `0x0045F2A0`。

函数只用`mov ax`覆盖入口EAX低word为行动者`+0x26D0`，随后对完整EAX执行NOT、右移11位并与一相与。最终结果等价于查询该word的bit11是否清零；入口EAX高半虽参与NOT和右移，但最终bit只来自被覆盖低word，因此返回严格为零或一。函数不改ECX和EDX。

实现把`+0x26D0`收敛为既有`LegacyBattleGroupAActionExecutionState`中的唯一`retreat_ready_flags` owner，并提供typed查询结果，显式保留ECX行动者token与陈旧EDX。退却提交在已关闭selected-actor查询之后直接读取第一group-A行动者，输入dispatch直接读取当前退却行动者；两处旧opaque调用均删除。退却提交枚举的旧primary查询槽和输入dispatch枚举的旧retreat查询槽均保留为reserved，禁止ABI平移。typed-stop位于原行动者word访问点，并保留退却提交此前selected-actor callee副作用。

测试覆盖bit11置位/清零、入口EAX高半、ECX/EDX保留、actor typed-stop、退却warning/commit、输入dispatch阻止/成功路径、旧枚举槽零调用、旧地址零调用以及commit分支的真实ECX base token与陈旧EDX。定向测试与独立AddressSanitizer均通过；Linux core为`188/188`，Linux app为`194/194`，源码零warning。inventory连续双生成逐字节一致，稳定为`219/422 = 210 platform_adapted + 9 assembly_exact + 203 pending_audit`，SHA256为`033fc9aa17a15173dc31d15998c498d18fa0e2f41a6abc39f43b9e28d511da58`。动态差分因原版行动者word与两处caller寄存器联合捕获后端缺失而登记为`blocked_runtime_oracle`。
