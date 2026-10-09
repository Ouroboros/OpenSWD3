# 战斗角色公共前部析构 `0x00478300`

历史分类：`platform_adapted`。当前实现直接释放MON文本所有权，已删除基础释放Port、寄存器合同和访问计数。
本批范围及验证见[说明所有权迁移](battle-actor-description-owned-release.md)。

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

- 对象读停止：在`actor+0xB0`读取处停止，不释放、不改token或宿主说明；
- 释放调用停止：实际所有权释放失败，保留原token与宿主说明，不执行后续清零；
- 对象写停止：实际释放已完成，说明视图失效，但原token仍保留；
- 正常完成：token与宿主说明均清除。

零token路径仍要求原始对象读取可达；没有空对象或短对象的防御性继续路径。
非零token通过其已有`LegacyBattleMonText`释放实际分配，成功后才清末尾DWORD。
结果只返回状态、原说明标识与失败位置，不再返回上述原机器寄存器残值。

## 4. 五处物理到达点与caller回收

完整LST有五处物理到达点：

- `0x00451895`：单例析构包装器`0x00451890`装入固定token `0x00521598`后尾跳；
- `0x0046E504`：组A元素析构正常路径call；
- `0x004755C4`：组B元素析构正常路径call；
- `0x00498393`：组A元素析构状态0的SEH cleanup chunk尾跳；
- `0x004983B3`：组B元素析构状态0的SEH cleanup chunk尾跳。

三类逻辑caller均已删除本函数的opaque边界：

- 组A先执行既有双资源清理，再直接释放公共definition说明；扩展清理抛出时，catch调用同一实际说明释放函数，然后继续传播原异常；
- 组B先执行既有`+0x0C`资源清理，再直接释放公共definition说明；其正常与SEH顺序同样保持；
- 单例析构直接释放构造阶段的同一owner，返回基础释放的实际状态。

C++由语言异常处理维持展开，不再模拟SEH链寄存器输出。
基础释放失败返回实际状态；基础释放抛出时不重复调用基础析构。

## 5. 双向追溯

- `0x00478300..0x00478301`：保存ESI并把this复制到ESI；
- `0x00478303`：读取`actor+0xB0`到EAX；
- `0x00478309..0x0047830B`：按完整dword零值分支；
- `0x0047830D..0x00478313`：压入token、调用`0x004885A0`并回收参数；
- `0x00478316`：callee正常返回后清零`actor+0xB0`；
- `0x00478320..0x00478321`：恢复ESI并返回。

C++保留唯一对象读写、零/非零分支、正常与展开到达点及释放顺序；
上述寄存器说明记录原指令事实，不再是C++接口合同。

## 6. 验证与动态差分

当前测试检查实际分配、共享文本视图、零token、三类失败前缀及释放异常。
聚合生命周期测试检查组A/B与单例的释放顺序、异常展开和基础异常不重复释放。
当前SDL尚未显式调用这些完整元素析构入口，不将core测试冒充生产退出覆盖。

历史工作包验证曾通过定向测试`2/2`、Linux core`198/198`、AddressSanitizer`198/198`、Linux app`204/204`及连续10轮完整core，源码零warning，无sanitizer finding或runtime error。inventory连续双生成逐字节一致，正式计数为`277/422 = 267 platform_adapted + 10 assembly_exact + 145 pending_audit`，SHA256为`1c69ea1c505789e9aca8f17d3312c303745d05abaf767ba4189b04a372e043c9`；release审计全部通过。

当前没有原版组A/组B/单例完整对象、说明堆、CRT释放边界、异常访问及五处caller联合寄存器/SEH捕获后端，`original_diff_verified`登记为`blocked_runtime_oracle`。该阻塞不影响完整11条指令、五处到达点与typed所有权的静态闭环。
