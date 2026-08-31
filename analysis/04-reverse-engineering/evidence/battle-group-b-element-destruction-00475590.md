# 战斗组B角色元素析构 `0x00475590`

状态：`platform_adapted`。完整主块、SEH外部chunk、typed元素状态、析构顺序、异常清理、vector callback边界与验证均已收敛。

## 1. 完整权威范围与SEH

权威行为真值仅为`swd3.exe.lst`。主块是`0x00475590..0x004755D8`，外部`FUNCTION CHUNK`是`0x004983B0..0x004983BD`；合计55行、23条实际指令、2个call、2个跳转、2个局部标签和1个返回点。chunk同时包含状态0清理funclet和SEH描述符，属于本函数而不是物理相邻函数。

函数是thiscall，入口ECX为尺寸`0x2B28`的组B角色对象。它没有普通call caller；地址由组B向量构造包装器`0x00451810`和向量析构包装器`0x00451840`作为callback参数传给MSVC compiler helper。两个callee固定为已关闭组B资源释放`0x00476A60`和公共基础析构`0x00478300`。

## 2. 正常析构与寄存器

主块先建立MSVC SEH记录，保存旧`FS:[0]`、ESI与this，再把unwind状态置0并调用扩展析构。扩展正常返回后，函数重新发布`ECX=this`、把unwind状态置`-1`，再调用公共基础析构。

基础析构返回后，函数把保存的旧SEH链token装入ECX并写回`FS:[0]`，然后恢复ESI和栈并返回。因此正常终端寄存器是：

- EAX：公共基础析构EAX；
- ECX：入口前的旧SEH链token，不是基础析构ECX；
- EDX：公共基础析构EDX。

扩展析构的返回寄存器均不成为终端结果，函数也不把EAX规范化为布尔值。

## 3. 外部chunk与异常顺序

若扩展析构在unwind状态0抛出，`0x004983B0`从保存局部重载this到ECX并尾跳公共基础析构；随后SEH描述符继续原异常展开。基础析构因此在扩展异常路径仍执行一次，原异常不被吞掉。

正常主块在调用基础析构前已把状态改为`-1`。若基础析构自身抛出，状态0 cleanup不再生效，不能第二次调用基础析构。modern分别以“扩展抛出后基础调用一次并传播”和“基础抛出时不重复调用”回归锁定这两条边界。

## 4. typed owner与窄callee

`LegacyBattleActorGroupBElementState`继续作为唯一物理owner，承接对象token、对象`+0x0C`资源token与164-byte资源记录。本工作包不建立第二份析构状态。

扩展资源析构`0x00476A60`现已独立关闭：包装器在SEH状态0内直接调用typed组B资源释放，复用同一元素owner；非零token只在CRT释放边界正常返回后清零并失效164-byte内联资源，零token跳过释放，callee异常保留当前token和内容。旧`release_extension`整函数端口已删除，只在其深层CRT释放callee保留窄typed边界。公共基础析构`0x00478300`仍由既有窄端口接收同一owner并返回三寄存器。

资源typed-stop按状态0异常清理等价规则仍执行一次公共基础析构，再返回显式typed状态。释放端口抛出时外部cleanup chunk执行一次基础析构并传播原异常；基础析构自身抛出时不重复调用。

`LegacyBattleActorElementDestructionRequest::seh_chain_token`显式承接原版保存的旧`FS:[0]`值，同时把入口EDX线程给扩展资源释放。正常返回以旧SEH token覆盖ECX；物理SEH地址仍只作为`compat::u32` token，不转换为宿主指针。

同轮也修正同型组A元素析构`0x0046E4D0`：其完整范围同样是主块39行加外部chunk、合计55行与23条指令；正常及typed结果的ECX恢复旧SEH链token，而不是误传公共基础析构ECX。

## 5. vector callback边界

组B构造callback`0x00475560`与析构callback`0x00475590`现均已typed关闭。`0x00451810`仍传递固定`base=0x00525508,size=0x2B28,count=8`及两个callback token；`0x00451840`仍传递同一base/size/count和析构callback token。

MSVC向量helper自身仍负责八对象前向构造、构造失败逆向回滚、逆序析构和异常传播。缺少该compiler helper及全局八对象联合后端时，两个包装器继续以单一vector port隔离编译器边界；callback token只是ABI数据，不代表重新执行原地址。

## 6. 双向追溯

- `0x00475590..0x004755A5`：建立SEH记录并保存旧链；
- `0x004755A6..0x004755AD`：保存ESI/this并把unwind状态置0；
- `0x004755B5`：调用扩展资源析构；
- `0x004755BA..0x004755BC`：恢复`ECX=this`并把unwind状态置`-1`；
- `0x004755C4`：调用公共基础析构；
- `0x004755C9..0x004755D8`：以旧SEH链覆盖ECX、恢复SEH/ESI/栈并返回；
- `0x004983B0..0x004983B3`：状态0异常时重载this并尾跳基础析构；
- `0x004983B8..0x004983BD`：发布SEH描述符并跳入compiler handler。

C++到LST反向追溯覆盖主块与完整chunk、全部23条实际指令、两个callee、两类异常路径和三项终端寄存器。

## 7. 验证与动态差分

正常回归覆盖typed资源释放→基础析构顺序、CRT callee请求、唯一资源owner修改、每个callee单次调用、基础EAX/EDX保留和旧SEH链ECX恢复。异常回归分别覆盖资源释放抛出后基础析构一次且原异常传播、资源typed-stop仍执行基础清理，以及基础析构抛出时不重复调用。组A回归同步锁定旧SEH链ECX。

定向测试、AddressSanitizer、Linux core `188/188`与Linux app `194/194`全部通过，源码零warning。

当前没有原版八个组B完整对象、真实扩展/基础资源、CRT释放callee副作用、MSVC SEH链、向量迭代与异常回滚联合捕获后端，`original_diff_verified`登记为`blocked_runtime_oracle`。该阻塞不影响完整主块与chunk的静态闭环和Linux验证。
