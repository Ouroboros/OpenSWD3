# 战斗组A角色元素析构 `0x0046E4D0`

状态：`platform_adapted`。完整主块、SEH外部chunk、typed双资源清理、typed公共基础析构、vector边界与验证均已收敛。

## 1. 完整权威范围与SEH

主块为`0x0046E4D0..0x0046E518`，外部`FUNCTION CHUNK`为`0x00498390..0x0049839D`；主块39行加chunk与分隔共55行、23条实际指令、2个call、2个跳转、2个局部标签和1个返回点。chunk属于本析构函数，不能归入物理相邻函数。

函数是thiscall。入口建立MSVC SEH链并把unwind状态置0，随后调用扩展清理`0x00475180`；正常返回后把unwind状态置`-1`，再以同一this调用公共基础析构`0x00478300`。基础析构后EAX/EDX保持其返回，ECX被保存的旧SEH链token覆盖；函数随后恢复`FS:[0]`与保存寄存器。

若扩展清理在状态0抛出，外部chunk从保存局部重载this并尾跳基础析构，随后由SEH描述符继续异常展开。基础析构因此在正常和扩展异常两条路径都执行一次；函数不吞掉原异常。

## 2. typed析构链

`release_legacy_battle_actor_group_a_element()`复用构造工作包的唯一元素状态。扩展清理`0x00475180`按顺序处理行动者`+0x2BC4` secondary token和`+0` primary/description token；每个非零token通过固定`0x004885A0`窄释放端口，callee正常返回后才清字段。primary成功释放后同步清除已失效的56-byte宿主description内容。

公共基础析构`0x00478300`现已typed关闭。它直接复用`base_initialization.resource_definition`末尾的`actor+0xB0`说明token与对应说明owner：零token只读不写；非零token先调用同一固定释放包装器，callee正常返回后才清token。对象读、callee和对象写各自保留原访问点typed-stop。

正常路径严格执行双资源清理→公共说明释放；只有基础析构完整返回后才恢复旧SEH链ECX。扩展callee抛出时，catch路径以请求中的unwind EAX/EDX和重载this执行同一typed基础清理并继续传播原异常。基础callee抛出时不会再次调用基础析构；基础typed-stop阻断正常epilogue。

## 3. 两处基础析构到达点

本元素析构贡献`0x00478300`的两个物理到达点：

- `0x0046E504`：扩展清理正常返回后的普通call；
- `0x00498393`：unwind状态0 cleanup chunk重载this后的尾跳。

旧`destroy_base()`整函数端口已删除。两处均直接组合`release_legacy_battle_actor_base()`；剩余端口只隔离固定CRT释放callee，不再隔离公共基础析构本体。

## 4. vector caller边界

本函数没有普通call caller，地址同时传给组A编译器向量构造迭代器的异常回滚参数和向量析构迭代器的元素回调。构造/析构元素回调现均关闭；两个包装器证据已更新，现存vector port只隔离MSVC对全局十对象数组的前向构造、失败逆向回滚、逆序析构与异常展开。

## 5. 验证状态

聚合测试验证typed资源清理→typed基础析构顺序、两个固定释放请求、primary与公共说明owner失效、基础EAX/EDX保留、旧SEH链ECX恢复、SEH chunk的unwind寄存器线程、扩展异常继续传播，以及基础callee异常/typed-stop不重复调用或伪造正常epilogue。当前Linux core`198/198`和定向`2/2`已通过；完整release门见[`battle-actor-base-release-00478300.md`](battle-actor-base-release-00478300.md)。

原版`0x004885A0`allocator副作用、全局组A对象字节、说明堆、MSVC SEH与vector迭代器缺少联合捕获后端，`original_diff_verified`登记为`blocked_runtime_oracle`。
