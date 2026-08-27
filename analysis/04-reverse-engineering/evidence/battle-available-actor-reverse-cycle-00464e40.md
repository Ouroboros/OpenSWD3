# 战斗可用角色反向轮转 `0x00464E40`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`、`caller_reclaimed`、`callee_reclaimed`。

## 1. 完整权威范围

权威LST主体为`0x00464E40..0x00464E8D`，从proc到endp共56行、35条实际指令、1个静态call、5个跳转、4个局部/返回标签、2个`retn`，没有外部`FUNCTION CHUNK`。

唯一caller是已关闭角色动作反向轮转；其四项跳表路径分别在`0x004623C0`、`0x004623D1`、`0x004623E2`和`0x004623F3`调用本函数。唯一静态call为已关闭角色动作候选可用性，现已直接组合typed实现。

## 2. 主表反向循环

入口栈参数作为starting actor code装入ECX；caller EAX被主候选表基址覆盖，EBX和ESI清零，EDX保持caller值。函数仍从物理主表`10,9,8,11`的首项正向扫描starting code。

命中后读取当前候选，再先递减ESI。signed ESI小于0时立即置3，因此后续查询沿主表反向循环。候选可用性完整EAX等于1时立即返回当前候选，ECX/EDX保留winning callee返回。合法起点的四条轨迹为：

- 10：`10,11,8,9`；
- 9：`9,10,11,8`；
- 8：`8,9,10,11`；
- 11：`11,8,9,10`。

每次失败后EBX加1；signed EBX小于4时继续，达到4时返回EAX=0并保留最后callee的ECX/EDX。函数不去重，也不增加现代循环上限。

## 3. 无效起点与共享物理表

starting code不在主表时，扫描结束时ESI为4、EAX为`0x004A7970`。函数先从主表尾后一dword读取相邻值2，再把ESI递减为3；后续三次读取重新进入主表，因此固定轨迹为`2,11,8,9`。

实现复用正向函数登记的连续八dword共享typed常量，不复制主表或相邻数据，不把无效起点现代化回绕。候选2若命中actor order，已关闭候选可用性完成u32组A地址计算并在首次真实对象call停止；若没有命中则继续原逆序轨迹。

## 4. caller与callee回收

角色动作反向轮转原先通过`actor_action_resolve_available_reverse`窄端口调用本函数。该枚举数值现改为`reserved_available_actor_reverse_cycle_slot`，生产代码零调用；四个原静态caller由共享跳表分支统一直连本实现。

每次候选查询直接复用已关闭候选可用性，只在其入口快照live group-A count，并扫描唯一actor order。子typed-stop保留物理候选读取、索引递减、actor-order扫描和地址计算，阻断后续候选与动作提交。成功code及callee ECX/EDX继续直连已关闭动作提交。

定向测试覆盖下一逆向候选、四次失败、四条合法起点完整轨迹、无效起点`2/11/8/9`、相邻候选首次对象访问停止、winning callee寄存器、反向动作caller直连、逐帧输入两个caller、子typed-stop传播和reserved槽零调用。

验证：定向测试、AddressSanitizer、Linux core 188/188、Linux app 194/194 全部通过。源码零warning；app仅保留既有ALSA开发库CMake warning。工作包稳定为`128/422 = 123 platform_adapted + 5 assembly_exact + 294 pending_audit`，连续双跑逐字节一致，SHA256为`14933e3422080906cbb8a67432b800e9ada693d5be9affc512e64beb3ff4827b`。

当前缺少原版actor order、group-A对象查询共享副作用及EAX/ECX/EDX联合动态捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
