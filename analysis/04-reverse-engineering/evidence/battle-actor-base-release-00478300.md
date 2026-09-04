# 战斗角色公共前部析构 `0x00478300`

状态：`platform_adapted`。完整LST、五处物理到达点、说明token释放顺序、寄存器残留、typed停止点与caller直组装均已收敛。

## 1. 完整权威范围

唯一行为真值为`swd3.exe.lst`。完整主体是`0x00478300..0x00478321`，从`proc`到`endp`共21个物理行、11条实际指令、1个call、1个条件跳转、1个局部标签和1个返回点，没有外部`FUNCTION CHUNK`。

函数为thiscall。入口`ECX`是角色对象，先保存入口`ESI`并令`ESI=this`，再无条件读取`actor+0xB0`说明token。正常返回恢复入口`ESI`。

## 2. 精确行为与寄存器

### token为零

`mov eax,[esi+0xB0]`先把完整token装入EAX；零值直接跳到统一尾部，不调用释放器且不写对象。正常返回`EAX=0`，ECX/EDX保持入口值，ESI恢复入口值。

### token非零

函数按下列顺序执行：

1. 把token压栈；
2. 调用固定cdecl释放包装器`0x004885A0`；
3. caller回收4-byte参数；
4. 只有callee正常返回后，才把`actor+0xB0`完整dword写零；
5. 恢复ESI并返回。

最终EAX/ECX/EDX均保留`0x004885A0`返回残值，不把EAX规范化成零或this。callee异常不会到达对象清零。

## 3. typed owner与停止点

公共构造`0x00478250`把`actor+0x10..+0xB3`建模为164-byte definition；因此本函数的`actor+0xB0`恰是该definition视图的末尾dword，即视图内`+0xA0`。`release_legacy_battle_actor_base()`直接复用同一owner：组A和静态单例使用`LegacyBattleActorBaseInitializationOwner::resource_definition`，组B使用既有`action_composition.resource_definition`，不复制第二份token或说明内容。

实现分离三个原始边界：

- 对象读停止：在`actor+0xB0`读取处停止，保留入口EAX/ECX/EDX，不调用释放器、不改token或宿主说明；
- 释放调用停止：保留callee返回/停止寄存器、原token与宿主说明，不执行后续清零；
- 对象写停止：callee释放已完成，宿主说明内容已经失效并清除，但原token仍保留，EAX/ECX/EDX保持callee残值；
- 正常完成：token与宿主说明均清除。

零token路径仍要求原始对象读取可达；没有空对象或短对象的防御性继续路径。

## 4. 五处物理到达点与caller回收

完整LST有五处物理到达点：

- `0x00451895`：单例析构包装器`0x00451890`装入固定token `0x00521598`后尾跳；
- `0x0046E504`：组A元素析构正常路径call；
- `0x004755C4`：组B元素析构正常路径call；
- `0x00498393`：组A元素析构状态0的SEH cleanup chunk尾跳；
- `0x004983B3`：组B元素析构状态0的SEH cleanup chunk尾跳。

三类逻辑caller均已删除本函数的opaque边界：

- 组A先执行既有双资源清理，再直接释放公共definition说明；扩展清理抛出时，cleanup chunk以显式unwind EAX/EDX及重载this调用同一typed helper，然后继续传播原异常；
- 组B先执行既有`+0x0C`资源清理，再直接释放公共definition说明；其正常与SEH顺序同样保持；
- 单例析构包装器直接对构造阶段的同一owner释放，固定覆盖ECX为单例token，并保留包装器入口EAX/EDX线程。

正常组A/组B外层析构只在基础析构完整返回后恢复旧SEH链到ECX。基础析构typed-stop阻断该正常epilogue；基础callee抛出时不重复调用基础析构。

## 5. 双向追溯

- `0x00478300..0x00478301`：保存ESI并把this复制到ESI；
- `0x00478303`：读取`actor+0xB0`到EAX；
- `0x00478309..0x0047830B`：按完整dword零值分支；
- `0x0047830D..0x00478313`：压入token、调用`0x004885A0`并回收参数；
- `0x00478316`：callee正常返回后清零`actor+0xB0`；
- `0x00478320..0x00478321`：恢复ESI并返回。

C++到LST反向追溯覆盖全部11条指令、唯一对象读写、唯一callee、零/非零两条路径、五处物理到达点及EAX/ECX/EDX残留。

## 6. 验证与动态差分

独立定向测试覆盖零token、正常释放、对象读停止、释放调用停止、释放后对象写停止、callee异常及说明owner保留/失效顺序。聚合生命周期测试覆盖组A、组B和固定单例的正常直组装、两个SEH cleanup chunk、基础callee异常不重复调用、typed-stop阻断外层epilogue及单例尾跳入口寄存器线程。

验证已通过定向测试`2/2`、Linux core`198/198`、AddressSanitizer`198/198`、Linux app`204/204`及连续10轮完整core，源码零warning，无sanitizer finding或runtime error。inventory连续双生成逐字节一致，正式计数为`277/422 = 267 platform_adapted + 10 assembly_exact + 145 pending_audit`，SHA256为`1c69ea1c505789e9aca8f17d3312c303745d05abaf767ba4189b04a372e043c9`；release审计全部通过。

当前没有原版组A/组B/单例完整对象、说明堆、CRT释放边界、异常访问及五处caller联合寄存器/SEH捕获后端，`original_diff_verified`登记为`blocked_runtime_oracle`。该阻塞不影响完整11条指令、五处到达点与typed所有权的静态闭环。
