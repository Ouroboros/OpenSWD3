# 战斗固定对象五字清零 `0x004776F0`

状态：`assembly_exact`、`unit_tested`、`caller_integrated`。

## 1. 完整LST范围与ABI

权威LST函数范围为`0x004776F0..0x00477704`，从`proc`到`endp`共14个物理行、8条实际指令。函数无callee、无跳转、无局部标签、无外部`FUNCTION CHUNK`，仅以plain `retn`返回。

函数采用cdecl单参数ABI。入口从`[ESP+4]`读取对象地址，callee不回收参数。唯一caller `0x00451A20`连续调用三次，并在三次调用后统一以`add esp,0x0C`回收三个参数。

## 2. 顺序写入与边界

完整指令顺序为：

```text
ECX = object
EAX = 0
[ECX+0x00] = EAX
[ECX+0x04] = EAX
[ECX+0x08] = EAX
[ECX+0x0C] = EAX
[ECX+0x10] = EAX
return
```

因此可观察写入严格为五个连续dword、共20字节，顺序固定为低地址到高地址。函数不读取旧值，不跳过任何字段，也不清理第20字节之后的数据。

modern helper接收`span<u32>`而不把旧地址解释为宿主指针。每次写入前检查当前dword是否可访问；长度为0至4时分别在原版对应的`+0x00`、`+0x04`、`+0x08`、`+0x0C`、`+0x10`写入点typed-stop，已完成的低地址前缀保持为零，尚未访问的后缀保持旧值。完整五字span严格完成五次写入。

## 3. 寄存器合同

`mov ecx,[esp+4]`首先发布对象token，随后`xor eax,eax`在第一次对象访问前把EAX清零。五次写入都使用同一零值，函数从不修改EDX。因此正常返回及任一原访问点故障前缀均保持：

- EAX为0；
- ECX为对象token；
- EDX为入口EDX；
- 成功路径的写入计数为5；
- typed-stop路径的写入计数等于故障写入之前已完成的dword数。

modern结果显式发布EAX、ECX、EDX、写入计数和故障偏移，没有把正常零返回误判为失败。

## 4. 共享owner与caller回收

唯一caller按固定顺序处理三个20字节物理表头：

1. `0x004B9F00`；
2. `0x004ACBA8`；
3. `0x004B8A00`。

三者由`LegacyBattleFixedObjectStatePort`中的唯一typed state统一拥有，每个对象只保存五个物理dword。组A奖励资料状态端口虚继承该owner，使`0x004B8A00`的物理表头与既有奖励链语义状态在同一端口层次内复用，不建立第二份物理header。

已关闭caller `reset_legacy_battle_objects`删除`LegacyBattleFixedObjectResetPort::reset_fixed_object` opaque边界，直接按上述顺序调用typed helper。全局链清理返回的EDX依次穿过三次helper；随后384字节表清零把EAX和ECX变为0而保留EDX，再进入组B和组A角色重置循环。

## 5. 双向追溯

- `0x004776F0`：参数对象进入ECX；
- `0x004776F4`：EAX清零；
- `0x004776F6`：写`+0x00`；
- `0x004776F8`：写`+0x04`；
- `0x004776FB`：写`+0x08`；
- `0x004776FE`：写`+0x0C`；
- `0x00477701`：写`+0x10`；
- `0x00477704`：plain返回。

C++到LST反向追溯只有五次顺序零写、五个可能的原访问故障点和EAX/ECX/EDX返回合同。没有额外分支、初始化、分配、释放、诊断、异常转换或相邻状态写。

## 6. 验证

独立定向测试覆盖完整五字清零及0、1、2、3、4个可访问dword的全部typed-stop前缀，并逐项验证旧后缀、故障偏移、写入计数和三寄存器结果。

caller聚合测试从三个全非零物理header启动，证明三者在角色循环前均清零、顺序token固定、384字节表随后清零、首个角色调用接收EAX零和全局清理后的EDX、后续18次角色调用逐次继承前次reply，最终返回末个组A角色callee的EAX、ECX和EDX。

该叶函数全部可观察输入域由完整LST和穷举写入边界测试覆盖，不依赖原版动态oracle。
