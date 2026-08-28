# 战斗组A角色元素析构 `0x0046E4D0`

状态：`platform_adapted`。完整主块、SEH外部chunk、typed析构链、vector边界、验证和inventory双生成均已收敛。

## 1. 完整权威范围与SEH

主块为`0x0046E4D0..0x0046E518`，外部`FUNCTION CHUNK`为`0x00498390..0x0049839D`；两部分合计62行、23条实际指令、2个call、2个跳转和1个返回点。chunk属于本析构函数，不能归入物理相邻函数。

函数是thiscall。入口建立MSVC SEH链并把unwind状态置0，随后调用扩展清理`0x00475180`；正常返回后把unwind状态置`-1`，再以同一this调用基础析构`0x00478300`。正常返回寄存器来自基础析构，函数仅恢复SEH与保存寄存器。

若扩展清理在状态0抛出，外部chunk从保存局部重载this并尾跳基础析构，随后由SEH描述符继续异常展开。基础析构因此在正常和扩展异常两条路径都执行一次；函数不吞掉原异常。

## 2. typed析构链

`release_legacy_battle_actor_group_a_element`复用构造工作包的唯一元素状态。扩展清理与基础析构各保留窄typed端口，因为其内部函数仍属后续工作包。

正常路径严格执行扩展→基础，并返回基础析构EAX/ECX/EDX。typed端口可在扩展阶段释放56-byte附属记录token与内容。扩展端口抛出时，catch路径先调用基础析构再原样`throw`，对应SEH chunk；异常路径不伪造正常结果对象，也不提前清扩展端口未完成的状态。

## 3. vector caller边界

本函数没有普通call caller，地址同时传给组A编译器向量构造迭代器的异常回滚参数和向量析构迭代器的元素回调。构造/析构元素回调现均关闭；两个包装器证据已更新，现存port只隔离MSVC对全局十对象数组的前向构造、失败逆向回滚、逆序析构与异常展开，不再包含未知元素业务。

## 4. 验证状态

正常测试验证扩展清理先于基础析构、附属记录由扩展端口释放、基础EAX/ECX/EDX原样返回。异常测试令扩展端口抛出，验证事件顺序仍为扩展→基础，附属记录保持扩展失败前状态，并在基础析构后把同一异常传播给caller。定向测试、AddressSanitizer、Linux core `188/188`和Linux app `194/194`全部通过，源码零warning。

inventory生成器连续双跑逐字节一致，正式计数为`169/422 = 160 platform_adapted + 9 assembly_exact + 253 pending_audit`，SHA256为`fa467eb8e05aaa1dd0a07bb27332df1a13b7fa8ad8ef9fddfaa08353e6bbb765`。原版两级析构内部副作用、全局对象字节、MSVC SEH和vector迭代器缺少联合捕获后端，`original_diff_verified`登记为`blocked_runtime_oracle`。
