# 战斗组B行动profile选择 `0x00476250`

状态：`platform_adapted`。完整LST、两个caller回收、固定mode可达路径、重叠dword输出、共享owner、typed-stop、测试与inventory门全部收敛后关闭。

## 1. 完整权威范围、ABI与调用集合

权威LST主体为`0x00476250..0x004762E1`，从proc到endp共73行、45条实际指令、3个call、3个跳转、3个局部标签和2个返回点，没有外部`FUNCTION CHUNK`。三个call依次是profile loader `0x00476A80`、固定mode 2的`0x00478710`和固定mode 1的`0x00478710`。

函数是带两个栈参数的thiscall：ECX为组B actor，首个参数是完整32位selector，第二个参数是32位输出地址。函数保存ESI/EDI，两个出口都以`retn 8`清理参数。全程序真实机器指令caller只有动作dispatch `0x00454F09`和脚本dispatch `0x0046D0E6`两处；函数声明文本不是额外caller。

## 2. 清零前缀与资源选择

入口先把actor保存到ESI，以actor `+0x0D90`形成40-byte profile目标，固定`ECX=10`、`EAX=0`并执行`rep stosd`。因此actor owner缺失只在首个profile dword写点typed-stop，暴露寄存器为`EAX=0, ECX=10, EDX=actor+0x0D90`，不提前检查资源或输出。

完整profile清零后，ECX自然变为0；函数随后读取selector参数，以AX零清actor `+0x29A4`首个派生word，再读取actor `+0x0C`资源token。只有完整32位selector精确等于1时读取资源`+0x72`的profile id，其他所有值读取`+0x76`；比较不能缩成低word比较。因为ECX在`rep stosd`后为零，`mov cx`得到零扩展profile id。资源token缺失在对应资源word首次读取点typed-stop，保留profile与派生word清零前缀。

## 3. profile加载、派生word与mode分流

函数以`(actor+0x0D90, profile_id)`调用待审`0x00476A80`。callee对40-byte目标的写入和返回EAX/ECX/EDX均先发布；loader typed-stop保留这些前缀，阻断profile读取、输出和mode后缀。

loader正常返回后：

1. `mov al,[profile+0x0C]`只覆盖callee EAX低byte；
2. `mov dx,[profile+0x0E]`只覆盖callee EDX低word；
3. 该DX低word完整写入actor `+0x29A4`首个派生word；
4. profile byte的bit 1决定后续路径。

bit 1清零时，函数把selector的DI低word写入actor `+0x2A8C`，固定展开mode 1，只把行动种类写1；显示种类和mode flags保持。最终返回`EAX=1, ECX=actor token`，EDX保留loader高word与profile `+0x0E`低word的拼接。

bit 1置位时，函数把profile `+0x14` word零扩展为dword写入第二参数指向的输出，再把actor `+0x2A87` bit7或入并固定展开mode 2：显示种类写2、行动种类写0。selector字段保持原值。最终外层强制返回`EAX=0, ECX=actor token`，EDX同样保留拼接值。输出typed-stop位于真实dword写点，保留profile加载和派生word发布，阻断mode flags与mode 2。

## 4. 32位输出与脚本状态word重叠

动作caller把第二参数指向32位共享消息状态，普通dword写没有别名问题。脚本caller却把它指向`0x005028AC + actor_index*2`的16位状态word数组；原指令仍是`mov [ecx],eax`的32位写。因此bit 1路径不仅以profile word覆盖当前状态word，也以零扩展值的高word清零下一状态word。

typed实现没有用未对齐`u32*`或违反别名规则的强转，而是显式提供“单dword”与“相邻两个word”两种物理视图。脚本只在已由LST限制为0至7的索引上提供当前和下一word，逐字节结果与原32位小端写一致；bit 1清零路径完全不访问输出，下一word保持陈旧值。

## 5. owner与待审callee边界

40-byte profile继续复用`LegacyBattleGroupBActionConfigurationState::profile_buffer`，资源token和164-byte资源继续复用组Bactor lifecycle owner。首个派生word、selector、行动种类、显示种类和mode flags继续复用`LegacyBattleGroupBActionCompositionState`，没有新增平行物理状态。

`0x00476A80`整体仍是后续待审profile loader，只保留一个窄typed port并通过既有profile payload发布40-byte写入。`0x00478710`整体仍待后续审计；本工作包只展开参数1和2的固定可达字段写入，不提前关闭其他mode。

## 6. 两个caller回收

动作25在已发布choice cursor/commit及组B状态`0x8000`后，仅当message auxiliary gate非零才调用本函数。原调用把EBX零作为selector，把共享32位消息状态作为输出，并以stored group-B index选择actor；返回EAX不被消费。typed实现按同样顺序直连，成功后才执行状态低位合并、type-2攻击顺序追加和current actor清理。actor/resource/loader/output stop保留caller选择前缀和函数内部前缀，阻断共同后缀。生产不再执行`0x00476250`地址。

脚本opcode 54先发布三个signed工作值，按value B选择0至7的组B actor并清其状态word，再从value C开始扫描候选。原调用的this是value B actor，selector是value A，输出也是value B状态word；旧typed代码曾把this误接到value A并把candidate当参数，本工作包按LST纠正。函数返回1时caller OR bit15，返回0时OR bit14；随后才追加type-2攻击顺序、执行一帧、清三个工作值并推进cursor 8。typed-stop保留候选扫描、状态word和profile前缀，阻断mask、攻击顺序、frame、清理与cursor后缀。稳定枚举数值保留为reserved，但生产call trace中旧整函数为零。

## 7. 测试、验证与动态差分

纯函数测试覆盖首个`rep stosd` actor stop、40-byte profile与派生word清零顺序、资源首次读取、完整selector等于1与低word伪相等、`+0x72/+0x76`、profile loader ABI与stop、callee EAX低byte/EDX低word拼接、mode 1低word selector、mode 2输出、输出stop和寄存器线程。

inventory生成器连续双跑逐字节一致，结果为`247/422 = 238 platform_adapted + 9 assembly_exact + 175 pending_audit`，SHA256为`ab7d35841c7e125acaea17efacaebf0c0aebd14233b9a0360547dc4d9efa6095`。

动作25集成测试覆盖固定selector零、32位消息输出、mode 2、共同攻击顺序后缀、loader ABI、loader stop前缀及旧地址零调用。脚本54集成测试覆盖value B actor/value A selector纠正、返回1/0对应bit15/bit14、bit 1路径清下一状态word、bit 0路径保留下一word、攻击顺序/frame/cursor后缀、loader stop及reserved整函数零调用。

最终验证使用仓库脚本完成：Linux core `188/188`、完整core AddressSanitizer `188/188`、Linux app `194/194`全部通过；三份最终日志均无OpenSWD3源码warning，ASan无finding，app仅有既有ALSA开发库环境提示。

当前缺少原版八个组B完整actor、动态164-byte资源、40-byte profile、真实profile loader、两个caller共享状态与寄存器联合捕获后端，因此`original_diff_verified`登记为`blocked_runtime_oracle`。该限制不影响完整静态闭环和Linux验证。
