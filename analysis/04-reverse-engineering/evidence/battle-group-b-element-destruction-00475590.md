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

`LegacyBattleActorGroupBElementState`承接对象标识、独立资源记录及公共definition。
扩展资源析构直接借用实际敌方存储，非零资源先撤销登记，成功后才清指针和记录。
敌方释放Port、callee、寄存器请求回复及计数已删除，详见[资源释放](battle-group-b-resource-cleanup-00476a60.md)。

公共基础析构直接复用`action_composition.resource_definition`末尾的说明token及文本所有权。
零token只读不写；非零先释放实际文本，成功后才清token。
读失败、释放失败及释放后写失败均保留对应前缀，已释放的共享文本视图同时失效。

基础说明Port及空的组B元素析构继承层已删除。
函数直接借用敌方存储，不再接收SEH/EDX参数；基础说明由其既有所有权释放。

## 4. 两处基础析构到达点与寄存器

本元素析构贡献`0x00478300`的两个物理到达点：

- `0x004755C4`：扩展资源清理正常返回后的普通call；基础callee看到扩展返回EDX，EAX先由`actor+0xB0` token覆盖；
- `0x004983B3`：unwind状态0 cleanup chunk重载this后的尾跳；EAX/EDX来自异常展开上下文。

上述为原机器寄存器事实。当前元素结果只返回实际清理状态，已删除寄存器及调用计数。
扩展异常路径直接释放说明，成功后继续传播原异常；不再传递unwind寄存器参数。

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

当前聚合回归检查独立资源→说明顺序、实际文本释放、异常展开及失败前缀。
历史Linux core`198/198`和定向`2/2`记录见基础析构文档；
本批验证见[说明所有权迁移](battle-actor-description-owned-release.md)。
向量函数编号和共享堆协议仍是当前Goal待迁移项，不以compiler边界排除。

当前没有原版八个组B完整对象、真实资源/说明堆、CRT释放callee副作用、MSVC SEH链、向量迭代与异常回滚联合捕获后端，`original_diff_verified`登记为`blocked_runtime_oracle`。
