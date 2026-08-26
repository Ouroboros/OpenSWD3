# 战斗角色metric重建 `0x0045B0E0`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`、`caller_reclaimed`。

## 1. 完整LST范围

权威函数为`0x0045B0E0..0x0045B18B`，从proc到endp完整92行、2个静态call站点、4个`loc_`标签，无外部FUNCTION CHUNK。两个call站点都调用同一角色metric callee。

共有14个静态caller站点。七个已关闭站点位于战斗启动、逐帧协调、角色动作分派、对手动作分派两处和最终角色步进两处；它们全部回收。其余七处位于尚未关闭的后续战斗函数，不提前修改。

## 2. 双18 dword清零

入口固定把第一张18 dword角色metric表清零，再把第二张18 dword角色顺序表清零。两次都是`ECX=0x12`、`EAX=0`与正向`rep stosd`；角色数量为零也不跳过清零。后一相邻函数直接覆盖顺序表前缀。

两张物理表只保存在动作、启动与帧协调端口共同虚继承的单一typed状态中。同一组合端口跨多类caller只有一份存储。角色优先索引也与逐帧选择值、启动初始化和调试C/W键共用该owner；后续优先阶段的写入会覆盖早先选择刷新值，不再维护独立副本。

## 3. 栈局部初值

函数的首条`push ecx`同时保存callee-saved ECX并提供4字节栈局部：

- `var_4`是保存的caller ECX低word；
- `var_2`是同一保存值的第三byte。

两个局部不预清零。首个角色callee若不写输出，group B存入caller ECX低word的i16符号扩展；后续角色继续复用前一次callee留下的同一byte与word。typed状态显式保存入口ECX、两个局部值及其32位兼容token。

## 4. group B循环

先读取group B完整u32数量；只有零跳过。循环从索引0开始：

1. EAX载入word局部token；
2. ECX载入组B角色token，基址固定、步长固定；
3. 栈参数依次表示byte局部token与word局部token；
4. callee返回后把word局部按i16符号扩展到EDX；
5. 重新读取共享group B数量到EAX；
6. 把EDX写入第一张表当前槽；
7. 索引加1，并与刚重新读取的数量作unsigned比较。

数量可由callee在同一次调用中修改，下一轮边界立即使用新值。角色token按u32回绕，不增加modern上限。

## 5. group A循环

组A表从索引8开始。入口先计算`group_a_count + 8`的u32结果，并以unsigned `<= 8`决定是否跳过；因此零跳过，接近全1的数量保留原始回绕域。

每轮使用组A固定基址与步长调用同一callee。callee后把word局部按i16符号扩展到EAX，重新读取group A数量到ECX，再写第一张表。索引加1后才计算新的`group_a_count + 8`边界，所以callee修改数量会立即缩短或扩展当前循环。

组A从表索引8开始，按原物理布局覆盖该区；不为异常组B数量建立隔离副本。

## 6. 返回与typed-stop

正常返回保留路径相关完整EAX：

- 无组A迭代时返回`group_a_count + 8`的u32结果；
- 有组A迭代时返回最后一次word局部的i16符号扩展。

ECX由函数尾`pop ecx`恢复为caller入口完整值。EDX不保存：group B尾为最后metric符号扩展，group A尾为最后callee EDX，无角色时保留caller入口EDX。

第一张typed表只有18槽。异常数量导致第19次写时，callee、局部输出、数量重载和寄存器覆盖已经发生；实现只在该首次真实store处typed-stop，保留此前36 dword清零与18次完整写入。

## 7. caller回收与测试

已关闭caller删除`0x0045B0E0` opaque边界并直接组合统一实现：

- 战斗启动以敌方与队伍数量发布共享计数，随后保留后两阶段；
- 逐帧协调使用同一端口共享计数，随后保留下一阶段与完成门；
- 角色动作分派、对手动作分派和最终角色步进在原调用点发布各自当前计数，并传播callee动态修改后的数量；
- 子函数typed-stop立即阻止caller后续阶段。

定向测试覆盖双表固定清零、caller ECX局部别名、两组token与步长、i16符号扩展、byte/word跨角色保留、动态数量增长与缩短、组A加8回绕、路径相关EAX/EDX、ECX恢复、第19次store时机、跨动作/启动端口的单一物理状态及七处已关闭caller回收。

当前缺少原版两组角色对象、metric callee、动态数量修改、metric与角色顺序两张物理表、栈地址和寄存器联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
