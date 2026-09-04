# 战斗组B角色元素析构 `0x00475590`

状态：`platform_adapted`。完整主块、SEH外部chunk、typed元素资源、typed公共基础析构、vector callback边界与验证均已收敛。

## 1. 完整权威范围与SEH

权威行为真值仅为`swd3.exe.lst`。主块是`0x00475590..0x004755D8`，外部`FUNCTION CHUNK`是`0x004983B0..0x004983BD`；合计55行、23条实际指令、2个call、2个跳转、2个局部标签和1个返回点。chunk同时包含状态0清理funclet和SEH描述符，属于本函数而不是物理相邻函数。

函数是thiscall，入口ECX为尺寸`0x2B28`的组B角色对象。主块先建立MSVC SEH记录，保存旧`FS:[0]`、ESI与this，再把unwind状态置0并调用扩展资源析构`0x00476A60`。扩展正常返回后，函数重新发布`ECX=this`、把unwind状态置`-1`，再调用公共基础析构`0x00478300`。

基础析构正常返回后，函数以保存的旧SEH链token覆盖ECX并写回`FS:[0]`，再恢复ESI和栈。因此正常终端EAX/EDX来自基础析构，ECX来自入口旧SEH链。

## 2. 外部chunk与异常顺序

若扩展析构在unwind状态0抛出，`0x004983B0`从保存局部重载this到ECX并尾跳公共基础析构；随后SEH描述符继续原异常展开。基础析构因此在扩展异常路径仍执行一次，原异常不被吞掉。

正常主块在调用基础析构前已把状态改为`-1`。若基础析构自身抛出，状态0 cleanup不再生效，不能第二次调用基础析构。

## 3. typed owner与直组装

`LegacyBattleActorGroupBElementState`继续作为唯一物理owner，承接对象token、对象`+0x0C`资源token、164-byte独立资源记录及公共`actor+0x10..+0xB3` definition。扩展资源析构`0x00476A60`已typed关闭：非零`+0x0C` token只在固定CRT释放callee正常返回后清零并失效独立资源内容。

公共基础析构`0x00478300`现在直接复用`action_composition.resource_definition`末尾的`actor+0xB0`说明token与对应说明owner。零token只读不写；非零token先调用固定`0x004885A0`，callee正常返回后才清token。对象读、callee和对象写各自保留原访问点typed-stop；释放后写停止保留已完成说明失效副作用和未清的原token。

旧`destroy_base()`整函数端口已删除。组B元素析构端口只继承独立资源与公共说明的固定释放callee窄端口，不再隔离两个已关闭析构本体。

## 4. 两处基础析构到达点与寄存器

本元素析构贡献`0x00478300`的两个物理到达点：

- `0x004755C4`：扩展资源清理正常返回后的普通call；基础callee看到扩展返回EDX，EAX先由`actor+0xB0` token覆盖；
- `0x004983B3`：unwind状态0 cleanup chunk重载this后的尾跳；EAX/EDX来自异常展开上下文。

正常路径只有基础析构完整返回后才恢复旧SEH链ECX。基础typed-stop直接返回专用状态与停止寄存器，阻断该正常epilogue。扩展异常路径显式线程unwind EAX/EDX，基础清理完成后继续传播原异常。

## 5. vector callback边界

组B构造callback`0x00475560`与析构callback`0x00475590`现均已typed关闭。`0x00451810`仍传递固定`base=0x00525508,size=0x2B28,count=8`及两个callback token；`0x00451840`仍传递同一base/size/count和析构callback token。

MSVC向量helper自身仍负责八对象前向构造、构造失败逆向回滚、逆序析构和异常传播。缺少compiler helper及全局八对象联合后端时，两个包装器继续以单一vector port隔离编译器边界；callback token只是ABI数据，不代表重新执行原地址。

## 6. 双向追溯

- `0x00475590..0x004755A5`：建立SEH记录并保存旧链；
- `0x004755A6..0x004755AD`：保存ESI/this并把unwind状态置0；
- `0x004755B5`：调用扩展资源析构；
- `0x004755BA..0x004755BC`：恢复`ECX=this`并把unwind状态置`-1`；
- `0x004755C4`：调用公共基础析构；
- `0x004755C9..0x004755D8`：以旧SEH链覆盖ECX、恢复SEH/ESI/栈并返回；
- `0x004983B0..0x004983B3`：状态0异常时重载this并尾跳基础析构；
- `0x004983B8..0x004983BD`：发布SEH描述符并跳入compiler handler。

## 7. 验证与动态差分

聚合回归覆盖typed独立资源→typed公共说明释放顺序、两个固定CRT请求、唯一owner修改、基础EAX/EDX保留、旧SEH链ECX恢复、扩展异常后的cleanup chunk、基础callee异常不重复调用、基础typed-stop阻断epilogue及对象访问停止。当前Linux core`198/198`和定向`2/2`已通过；完整release门见[`battle-actor-base-release-00478300.md`](battle-actor-base-release-00478300.md)。

当前没有原版八个组B完整对象、真实资源/说明堆、CRT释放callee副作用、MSVC SEH链、向量迭代与异常回滚联合捕获后端，`original_diff_verified`登记为`blocked_runtime_oracle`。
