# 战斗组A角色元素析构 `0x0046E4D0`

状态：`platform_adapted`。完整主块、SEH外部chunk、typed双资源清理、typed公共基础析构、vector边界与验证均已收敛。

## 1. 完整权威范围与SEH

主块为`0x0046E4D0..0x0046E518`，外部`FUNCTION CHUNK`为`0x00498390..0x0049839D`；主块39行加chunk与分隔共55行、23条实际指令、2个call、2个跳转、2个局部标签和1个返回点。chunk属于本析构函数，不能归入物理相邻函数。

函数是thiscall。入口建立MSVC SEH链并把unwind状态置0，随后调用扩展清理`0x00475180`；正常返回后把unwind状态置`-1`，再以同一this调用公共基础析构`0x00478300`。基础析构后EAX/EDX保持其返回，ECX被保存的旧SEH链token覆盖；函数随后恢复`FS:[0]`与保存寄存器。

若扩展清理在状态0抛出，外部chunk从保存局部重载this并尾跳基础析构，随后由SEH描述符继续异常展开。基础析构因此在正常和扩展异常两条路径都执行一次；函数不吞掉原异常。

## 2. typed析构链

`release_legacy_battle_actor_group_a_element()`复用构造工作包的唯一元素状态。扩展清理`0x00475180`按顺序处理行动者`+0x2BC4` secondary token和`+0` primary/description token；每个非零token通过固定`0x004885A0`窄释放端口，callee正常返回后才清字段。primary成功释放后同步清除已失效的56-byte宿主description内容。

公共基础析构直接复用`base_initialization.resource_definition`末尾的说明token及文本所有权。
零token只读不写；非零先释放实际文本，成功后才清token。
基础释放Port、callee编号、寄存器回复与计数已删除，三类访问失败前缀保持。

正常路径严格执行双资源清理→公共说明释放。
扩展清理抛出时，catch释放同一文本所有权并继续传播原异常。
基础释放本身抛出时不再次清理；原基础释放失败状态继续向外传递。
元素结果已删除寄存器残值与调用计数，异常展开不再传递unwind寄存器参数。

## 3. 两处基础析构到达点

本元素析构贡献`0x00478300`的两个物理到达点：

- `0x0046E504`：扩展清理正常返回后的普通call；
- `0x00498393`：unwind状态0 cleanup chunk重载this后的尾跳。

两处直接调用`release_legacy_battle_actor_base()`，基础文本释放Port已删除。
空的组A元素析构继承层同时删除，函数直接借用尚待迁移的双资源释放接口。

## 4. vector caller边界

本函数没有普通call caller，地址同时传给组A编译器向量构造迭代器的异常回滚参数和向量析构迭代器的元素回调。构造/析构元素回调现均关闭；两个包装器证据已更新，现存vector port只隔离MSVC对全局十对象数组的前向构造、失败逆向回滚、逆序析构与异常展开。

## 5. 验证状态

当前聚合测试检查资源→说明释放顺序、实际文本失效、扩展异常清理及失败前缀。
寄存器观察断言随旧合同删除。历史Linux core`198/198`和定向`2/2`记录见基础析构文档；
本批验证见[说明所有权迁移](battle-actor-description-owned-release.md)。
向量函数编号与扩展资源协议仍是当前Goal待迁移项，不以compiler边界排除。

原版`0x004885A0`allocator副作用、全局组A对象字节、说明堆、MSVC SEH与vector迭代器缺少联合捕获后端，`original_diff_verified`登记为`blocked_runtime_oracle`。
