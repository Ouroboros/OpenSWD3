# 战斗组B行动进度更新 `0x004755E0`

状态：`platform_adapted`。完整LST、typed状态、两个caller、定向/ASan/Linux验证和inventory双生成收敛后关闭。

## 1. 完整权威范围与ABI

权威LST主体为`0x004755E0..0x0047570E`，从proc到endp共153行、101条实际指令、0个call、9个跳转、9个局部标签和4个返回点，没有外部`FUNCTION CHUNK`。函数是thiscall并由callee弹出一个i32参数；两个直接caller为startup `0x00452743`和组B帧`0x0045770E`。

入口以行动者`+0x26D0`低word依次检查bit6和bit14；任一命中都不写状态，返回`EAX=0`、`ECX=this`，EDX保持入口陈旧值。随后把`+0x2A12`进度低word零扩展，与全局i32阈值做signed `jl`比较。

## 2. 完成路径

进度不小于阈值时先读取`+0x2B20`完整dword到EDX，然后固定写`+0x2AB0=1`并清`+0x2AEC`。原frame-started不等于一时直接返回；精确等于一时再清`+0x2B20`和`+0x2B08`。

完成路径返回`EAX=1`、`ECX=this`，EDX保持原frame-started值。与组A `0x0046E520`不同，本函数不检查special/action完成门，不访问scene identity，也不清cache或设置update-ready。

## 3. 继续推进与资源故障点

进度小于阈值时按原顺序读取行动者`+0x0C`资源token、以delay flags低byte覆盖AL，再在资源`+0x5A`的u16读取点首次解引用。资源token为零或typed owner缺失时严格停在该读取点：不写行动者状态，返回EAX为`(progress & 0xFF00) | low_byte(delay_flags)`、ECX为行动者token、EDX为资源token，并保留此前入口门和阈值比较。

资源可用时，u16基值先逻辑右移2；delay flags低byte bit6置位时再算术右移1。调用参数精确等于一时追加`trunc(base/4)+1`。三类调整保持原signed向零除法：

- bit29：正向增加`30*base/100`；
- bit27：负向扣除`30*base/100`；
- bit31：在现有负向值上再扣`10*base/100`。

最后按原低32位顺序计算`old-progress - negative + positive + base`，先清`+0x2AB0`，再只把结果低16位写回`+0x2A12`，保留进度dword高16位；不夹值、不饱和，并保持word回绕。

继续路径固定返回`EAX=0`、`ECX=this`。EDX保留最后真实覆盖：无参数增强和调整时为资源token；参数一且无调整时为零；否则依次由bit29商、bit27商、bit31额外商中的最后执行者覆盖。

## 4. typed owner与caller回收

行动者字段继续复用既有`LegacyBattleActorProgressState`唯一owner；资源token与164-byte资源块复用惰性堆分配的八槽`LegacyBattleActorGroupBElementState`唯一owner，不建立平行base-speed副本，也不把八个大资源块压入startup或聚合测试栈。startup按相同索引把enemy进度视图与共享生命周期槽绑定到同一组B行动者；未审`configure_enemy_actor`只允许窄发布资源token与`+0x5A`基值，缺失发布时后续首次资源读取typed-stop。

startup随机循环删除整函数opaque调用，第一次调用继承随机callee的EDX，后续迭代继承上次进度函数EDX。组B帧在opponent更新后直接调用typed实现，继承更新callee的EDX，仅在返回EAX精确等于一且消息门允许时加入攻击顺序；资源typed-stop保留此前opponent更新副作用。两处旧整函数调用均为零。

同轮纠正共享组A进度实现的低word写回：`0x0046E520`原指令只写`+0x2A12`低word，typed owner不再误清高16位。

## 5. 验证与动态阻塞

纯函数回归覆盖bit14早退、frame-started一/非一完成尾、资源读取typed-stop寄存器、delay bit6、参数一增强、bit29正向、bit27+bit31负向、EDX资源token陈旧值和进度高word保留。startup回归覆盖窄资源发布、随机循环typed直连、迭代结果、旧opaque槽零调用和缺资源停止；组B帧回归覆盖直接完成、攻击顺序、资源停止前opponent更新及旧opaque地址零调用。

定向CTest `1/1`、AddressSanitizer定向CTest `1/1`、Linux core `188/188`和Linux app `194/194`全部通过；四份构建日志与四份测试日志均无编译warning、sanitizer finding、runtime error或失败项。原版八个组B完整对象、动态资源`+0x5A`、全局阈值、delay flags、startup随机callee、组B更新callee及两个caller寄存器缺少联合捕获后端，因此`original_diff_verified`登记为`blocked_runtime_oracle`。
