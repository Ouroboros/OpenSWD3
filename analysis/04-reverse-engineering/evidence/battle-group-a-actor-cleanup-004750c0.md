# 战斗group-A行动者状态与资源计数清理 `0x004750C0`

历史状态：`platform_adapted`。工作包282正在修正重叠资料视图；旧全量门与caller回收记录不能替代当前完整验收。

权威LST主体为`0x004750C0..0x0047515D`，proc至endp共62行、55条实际指令、0个call、2个跳转、2个局部标签和1个返回点，没有外部`FUNCTION CHUNK`。隐藏this为group-A行动者token。八处caller为`0x00457F1C`、`0x0045AAC9`、`0x00461D6B`、`0x00461DE1`、`0x00461FE3`、`0x00462072`、`0x00464334`与`0x00467140`，分别属于group-B frame、final-actor step、menu finalize四个分支、selection frame和message phase。

函数先把入口ECX复制到EDX，令ESI、EAX为零并令ECX为10。随后按原始物理偏移顺序清理行动者：`+0x2A86`一个word；`+0x0D90..+0x0DB7`十个dword；`+0x2630..+0x263F`四个dword；读取`+0x2ED0`到EAX；再依次清零`+0x2F10`、`+0x2F12`、`+0x2A8A`、`+0x2F16`、`+0x2F18`、`+0x29A6`、`+0x29A8`、`+0x29AA`、`+0x29A4`和`+0x2F14`的word。`+0x2F1A`不在清理范围内，必须保留。

工作包282已把`+0x0D90`及其中四个执行字段统一为组A执行状态的一块资料存储，删除同步清理helper。`0x004750DA rep stosd`仅清这十个DWORD；不再在该访问之前要求final-processing存在。缺少随后`+0x2630`视图时返回`pre_effect_state_typed_stop`，保留十DWORD清理，ECX=`actor+0x2630`。测试同步验证重叠字段自动清零及该停止点前缀。

`+0x2F10..+0x2F18`、`+0x29A4..+0x29AA`等跨状态投影仍需在完整所有权收敛中继续审计，不能把历史同步helper视为唯一所有权的证明。本轮core29/ASan19定向`1/1`通过（2.91/4.76秒），无匹配诊断、diff check通过；没有本包全量发布验收。

如果最初读取的`+0x2ED0`为零，函数直接带陈旧EAX零、ECX=`actor+0x2630`、EDX=actor返回。如果非零，则把`+0x2EC8`资源头读入EAX，再从该节点`+6`读取CX；该word按unsigned与零比较，正数时执行32位`dec ecx`但仅把低word写回节点。因为这里是`mov cx`而不是`movzx ecx`，ECX高16位继续继承`actor+0x2630`。无论数量是否递减，最后才把`+0x2ED0`清零。typed实现只在真实节点`+6`访问点对缺失资源节点停止，停止时保留全部前置清理，且不提前清除`+0x2ED0`。

八处production caller全部改为typed直连：group-B frame与final-actor step分别绑定同一startup party owner；menu finalize四个callsite由两个共享lambda覆盖并保留各分支入口寄存器；selection frame在目标准备后、其余角色循环前执行；message phase在每个prepare后、mode-one设置前执行。旧`0x004750C0`整函数opaque调用已从业务源码删除；原枚举数值槽改名为`reserved`，避免改变其后稳定枚举值。

定向测试覆盖首写typed-stop、前置副作用保留、十个profile dword、四个pre-effect dword、十一个显式word、重叠owner同步、`+0x2F1A`保留、无资源快路径、正数量递减、零数量不递减、CX高半继承、资源节点故障点、八处caller的旧地址零调用及上层寄存器/序列回归。

验证：定向测试通过；独立AddressSanitizer通过且零finding；Linux core `188/188`与Linux app `194/194`全部通过，源码零warning。inventory连续双生成逐字节一致，稳定为`232/422 = 223 platform_adapted + 9 assembly_exact + 190 pending_audit`，SHA256为`8efdb823624eec1dbbfff179321885871b1e17dc052420e7ce5e01dd47a15f05`。动态差分因原版十个group-A行动者完整对象、`+0x2EC8/+0x2ED0`资源链、八处caller前后寄存器及重叠物理字段联合捕获后端缺失而登记为`blocked_runtime_oracle`。
