# 战斗组B脚本行动道具参数写入 `0x004769A0`

状态：`platform_adapted`、`unit_tested`、`caller_reclaimed`。

## 1. 完整LST范围

权威主体为`0x004769A0..0x00476A08`，从`proc`到`endp`共59行、31条实际指令、0个call、6个跳转、6个局部/返回标签和1个返回点。没有外部`FUNCTION CHUNK`，也没有直接callee。

唯一静态caller是已关闭的战斗脚本分派`0x00469D20`，调用站点位于脚本case 56。完整caller主体和该case前后控制流均已从主LST重新提取。

## 2. ABI与唯一owner

函数使用thiscall形态：ECX为组Bactor token，栈上六个参数都是u16，`retn 0x18`由callee清理24-byte参数区。

目标是actor `+0x0C`动态资源中的六个word：`+0x66/+0x68`、`+0x6A/+0x6C`和`+0x6E/+0x70`，构成脚本配置的三组主行动道具选项参数。实现直接复用`LegacyBattleActorGroupBElementState::resource_token`与既有164-byte `resource_bytes`唯一owner，不新增平行选项记录。

## 3. 六段零值跳过与写入顺序

每段均先把当前栈参数读入AX，再以`test ax, ax`判断。`TEST`会把CF清零，所以后续`JBE`只在AX为零时成立：

- 参数为零：跳过actor和资源的全部访问，保留目标word旧值；
- 参数非零：从actor `+0x0C`重读资源token，再把完整u16按小端写入对应目标word。

前五次资源token重读使用EDX，第六次改用ECX。每个非零参数都独立重读资源token；零参数不重读。`0x8000`和`0xFFFF`等高bit值都属于非零并原样写入，不能按signed值过滤。

如果六个参数全零，函数从不访问actor；即使caller算出的actor地址不在现代八槽owner内，也必须完成返回。

## 4. 返回寄存器与typed故障点

callee入口EAX高word在六次`mov ax`间保留，正常返回时EAX低word总是第六个参数。ECX保持actor token，除非第六个参数非零，此时ECX成为该次资源token。EDX保留caller入口值，除非前五个参数中至少一个非零，此时EDX为最后一次对应资源token。

- actor缺失：仅在当前参数非零、首次实际读取actor `+0x0C`时停止；当前AX已经发布，零参数前缀已经跳过；
- 资源token无效：actor `+0x0C`读取已经完成，并已把零token发布到该段使用的EDX或ECX，随后停止在当前目标word写入点；
- 固定164-byte资源owner完整覆盖六个目标word，不添加原版不存在的中间容量门。

## 5. caller回收与脚本读取前缀

脚本case 56先从`script+2`读取actor u16，写入共享packed actor状态高word，并以`MOV CX`保留入口ECX高word。六个参数按原caller的栈压入顺序逆序读取：`script+0x0E`、`+0x0C`、`+0x0A`、`+0x08`、`+0x06`，最后才读取`+0x04`。

读取期间AX和DX按原局部word覆盖保留入口高word；最后一个脚本参数读取前，caller先把EAX清零并把actor写入AX。随后caller按原地址算式形成组Bactor token，并把callee入口EDX形成`1381 * actor`。

脚本读取typed-stop保留该读取点之前的packed actor与EAX/ECX/EDX前缀，不调用callee也不写资源。callee正常返回后，caller才把cursor按u32回绕前进16、返回EAX一并由epilogue恢复完整入口ECX；EDX保留callee结果。callee typed-stop阻断这些成功后缀。

actor范围不能在caller提前拒绝：全零参数路径必须允许任意u16 actor完成；只有首个非零参数的真实actor访问才能形成typed-stop。旧`0x004769A0`整函数opaque调用在production源码中为零，枚举数值只保留reserved稳定槽。

## 6. 验证

纯函数测试覆盖六参数全零且actor缺失、六个单独非零位置、六个完整非零u16、每个首个非零位置的actor停止、每个目标word的资源写入停止、EDX前五段与ECX末段寄存器线程，以及目标范围外字节不变。

caller集成测试覆盖混合零/非零三组选项、仅第六项非零、全零且actor越界、actor停止、第二参数资源停止、逆序首个脚本参数截断、actor word高byte截断、packed actor、成功cursor/ECX epilogue和旧opaque零调用。

战斗聚合定向测试、完整core AddressSanitizer `188/188`、Linux core `188/188`和Linux app `194/194`全部通过；最终日志零OpenSWD3源码warning、测试失败和sanitizer finding。

## 7. 动态差分

当前缺少原版组Bactor完整对象、动态资源token、资源六个目标word、脚本case 56逆序读取期间的原始参数地址和唯一caller寄存器联合捕获后端，`original_diff_verified`登记为`blocked_runtime_oracle`。
