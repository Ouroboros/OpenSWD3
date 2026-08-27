# 战斗可用角色轮转 `0x00464DD0`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`、`caller_reclaimed`、`callee_reclaimed`。

## 1. 完整权威范围

权威LST主体为`0x00464DD0..0x00464E33`，从proc到endp共68行、43条实际指令、2个静态call、6个跳转、6个局部/返回标签、2个`retn`，没有外部`FUNCTION CHUNK`。

唯一caller是已关闭正向角色动作轮转；其四项跳表路径分别在`0x00462340`、`0x00462351`、`0x00462362`和`0x00462373`调用本函数。两个静态call都是已关闭角色动作候选可用性，已直接组合typed实现。

## 2. 主候选表

入口栈参数作为starting actor code装入ECX；caller EAX被候选表基址覆盖，EBX和ESI清零，EDX保持caller值。主表四个dword按物理顺序为`10,9,8,11`。

函数从表首逐项完整比较starting code。命中后从该项开始查询；每次读取候选后先递增ESI。候选可用性完整EAX等于1时立即返回该候选，ECX/EDX保留winning callee返回。

失败后只在`ESI == 4`时回绕0，不使用大于等于比较；EBX随后加1。EBX达到signed 4时返回EAX=0，ECX/EDX保留最后callee结果。因此合法起点的四条轨迹为：

- 10：`10,9,8,11`；
- 9：`9,8,11,10`；
- 8：`8,11,10,9`；
- 11：`11,10,9,8`。

最多查询四次，不增加现代上限或提前去重。

## 3. 未命中主表的物理邻接

starting code不在主表时，扫描结束时ESI恰为4、EAX恰为`0x004A7970`，随后不经边界检查直接读取下一物理dword。由于第一次读取后ESI已经成为5，后续`ESI == 4`永远不成立；四次候选固定读取相邻表`2,1,0,3`。

实现把审计到的八个连续dword保存为单一常量物理视图，并由已关闭反向轮转复用；不把无效起点回绕到主表，也不在候选code小于8时提前拒绝。若相邻候选命中actor order，已关闭候选可用性会完成u32组A地址计算并在首次真实对象call停止；若均未命中，则四次扫描后正常返回0。

## 4. caller与callee回收

正向动作轮转原先通过`actor_action_resolve_available`窄端口调用本函数。该枚举数值现改为`reserved_available_actor_cycle_slot`，生产代码零调用；四个原静态caller由共享跳表分支统一直连本实现。

每次候选查询直接复用已关闭可用性函数：入口只快照一次group-A count，扫描共享actor order，并在首个匹配角色上调用既有对象查询。子typed-stop保留已扫描候选、actor-order读取与地址计算并阻断后续候选及动作提交。成功结果继续作为已关闭动作提交的actor code，ECX/EDX完整传入提交路径。

定向测试覆盖主表命中与下一候选、四次失败回绕、四条合法起点轨迹、无效起点相邻`2/1/0/3`、相邻候选首次对象访问停止、winning callee寄存器、正向caller直连、子typed-stop传播和reserved槽零调用。

验证：定向测试、AddressSanitizer、Linux core 188/188、Linux app 194/194 全部通过。源码零warning；app仅保留既有ALSA开发库CMake warning。工作包稳定为`127/422 = 122 platform_adapted + 5 assembly_exact + 295 pending_audit`，连续双跑逐字节一致，SHA256为`acf996591f919299b57cd117bf4c83807c3c7d17ac3915a39473d02e01c53313`。

当前缺少原版actor order、group-A对象查询共享副作用及EAX/ECX/EDX联合动态捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
