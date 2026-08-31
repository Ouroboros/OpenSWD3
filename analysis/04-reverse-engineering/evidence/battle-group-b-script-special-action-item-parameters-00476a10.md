# 战斗组B脚本特殊行动道具参数写入 `0x00476A10`

状态：`platform_adapted`、`unit_tested`、`caller_reclaimed`。

## 1. 完整LST范围

权威主体为`0x00476A10..0x00476A56`，从`proc`到下一函数分隔前共50行、21条实际指令、0个call、4个跳转、4个局部/返回标签和1个返回点。没有外部`FUNCTION CHUNK`，也没有直接callee。

唯一静态caller是已关闭的战斗脚本分派`0x00469D20`，调用站点位于脚本case 75。完整caller主体和该case前后控制流均已从主LST重新提取。

## 2. ABI与唯一owner

函数使用thiscall形态：ECX为组Bactor token，栈上四个参数都是u16，`retn 0x10`由callee清理16-byte参数区。

四个目标资源word严格依次为`+0x72`、`+0x74`、`+0x76`、再次`+0x74`。前两个word是已关闭特殊行动道具选项加载器使用的定义槽；本函数的第4项没有写入看似连续的`+0x78`，而是按原指令覆盖第2项所在的`+0x74`。实现复用`LegacyBattleActorGroupBElementState::resource_token`与既有164-byte `resource_bytes`唯一owner，不建立平行配置记录，也不修正原目标别名。

## 3. 四段零值跳过与目标别名

每段均先把当前栈参数读入AX，再以`test ax, ax`判断。`TEST`把CF清零，所以后续`JBE`只在AX为零时成立：

- 参数为零：跳过actor和资源的全部访问，保留目标word旧值；
- 参数非零：从actor `+0x0C`重读资源token，再把完整u16按小端写入当前目标word。

前三次资源token重读使用EDX，第4次改用ECX。每个非零参数都独立重读资源token；零参数不重读。`0x8000`和`0xFFFF`等高bit值均属于非零并原样写入。

第2项非零而第4项为零时，`+0x74`保留第2项值；两项均非零时，第4项最终覆盖第2项。四项全零时函数从不访问actor，因此任意u16 actor token都必须完成返回。

## 4. 返回寄存器与typed故障点

callee入口EAX高word在四次`mov ax`间保留，正常返回时EAX低word总是第4个参数。ECX保持actor token，除非第4个参数非零，此时ECX成为该次资源token。EDX保留caller入口值，除非前三个参数中至少一个非零，此时EDX为最后一次对应资源token。

- actor缺失：仅在当前参数非零、首次实际读取actor `+0x0C`时停止；当前AX已经发布，零参数前缀已跳过；
- 资源token无效：actor `+0x0C`读取已经完成，并已把零token发布到当前使用的EDX或ECX，随后停止在当前目标word写入点；
- 固定164-byte资源owner完整覆盖所有目标word，不添加原版不存在的中间容量门。

## 5. caller回收与脚本读取前缀

脚本case 75先从`script+2`读取actor u16，写入共享packed actor状态高word，并以`MOV CX`保留入口ECX高word。四个参数按原caller压栈顺序逆序读取：`script+0x0A`、`+0x08`、`+0x06`，最后才读取`+0x04`。

读取期间AX和DX按原局部word覆盖保留入口高word；最后一个脚本参数读取前，caller把EAX清零并把actor写入AX。随后caller按原地址算式形成组Bactor token，并把callee入口EDX形成`1381 * actor`。

脚本读取typed-stop保留该读取点之前的packed actor与EAX/ECX/EDX前缀，不调用callee也不写资源。callee正常返回后，caller才把cursor按u32回绕前进12、返回EAX一并由epilogue恢复完整入口ECX；EDX保留callee结果。callee typed-stop阻断这些成功后缀。

actor范围不能在caller提前拒绝：全零参数路径必须允许任意u16 actor完成；只有首个非零参数的真实actor访问才能形成typed-stop。旧`0x00476A10`整函数opaque调用在production源码中为零，枚举数值只保留reserved稳定槽。

## 6. 验证

纯函数测试覆盖四参数全零且actor缺失、四个单独非零位置、四项完整非零、`+0x74`覆盖别名、每个首个非零位置的actor停止、每个目标word的资源写入停止、EDX前三段与ECX末段寄存器线程，以及`+0x78`保持不变。

caller集成测试覆盖四项完整写入和第4项覆盖、全零且actor越界、actor停止、第4参数资源停止、逆序首个脚本参数截断、actor word高byte截断、packed actor、成功cursor/ECX epilogue和旧opaque零调用。

战斗聚合定向测试、完整core AddressSanitizer `188/188`、Linux core `188/188`和Linux app `194/194`全部通过；最终日志零OpenSWD3源码warning、测试失败和sanitizer finding。

## 7. 动态差分

当前缺少原版组Bactor完整对象、动态资源token、资源三个物理目标word、脚本case 75逆序读取期间的原始参数地址和唯一caller寄存器联合捕获后端，`original_diff_verified`登记为`blocked_runtime_oracle`。
