# 战斗固定键计数链查询 `0x00477800`

状态：`platform_adapted`、`unit_tested`、`caller_reclaimed`

## 1. 完整权威范围

唯一行为真值为`swd3.exe.lst`。完整主体为`0x00477800..0x00477821`，从proc到endp共26个物理行、12条实际指令、0个call、3个跳转、3个局部/返回标签和1个返回点，没有外部`FUNCTION CHUNK`。

七个静态callsite为`0x00409D34`、`0x00410846`、`0x0041086D`、`0x0043C1BC`、`0x00442D53`、`0x00454670`和`0x00466AD3`。其中`0x00409CE0` Fame载入过程仍为`pending_audit`，不提前修改；其余六个物理站点位于五个已关闭caller，均已回收为typed直连。

## 2. 链扫描与返回合同

入口先把根参数发布到ECX，只以`mov dx`替换EDX低word为查询键并保留高word，随后以`xor eax,eax`把完整EAX清零。固定根不是纯哨兵，函数先比较`word [root+4]`；不匹配时严格循环：

1. 从当前记录`+0x00`读取next到ECX。
2. ECX为零时直接返回，EAX保持零。
3. 比较`word [ECX+4]`，不匹配则继续扫描。

根或动态节点命中后只读取`word [ECX+6]`到AX。由于EAX已完整清零，成功结果是计数word的零扩展值；ECX保留命中记录token。缺失结果为EAX零、ECX零。EDX在所有正常路径均保留入口高word并把低word替换为查询键。

函数没有分配、写入或外部callee，不增加现代数量上限、环检测、排序或nil替代值。

## 3. 唯一owner与typed-stop

固定根和动态20-byte节点继续由`LegacyBattleFixedObjectStatePort`唯一持有，与`0x004776F0`、`0x00477710`和`0x00477780`共享同一物理状态。lookup不建立副本，也不依赖完整`openswd3_battle`目标，因此特殊模式caller不会形成循环依赖。

原访问点typed-stop为：

- 根`+0x04`键读取不可访问：EAX已清零，ECX为根token，EDX低word已替换。
- next指向未映射记录或动态记录`+0x04`键不可访问：此前link读取已把ECX发布为该token。
- 命中记录`+0x06`计数读取不可访问：键比较已经完成，EAX仍为零，ECX保留命中token。

这些停止点不伪造零值成功，也不读取或写入未到达的后缀。

## 4. 已关闭caller回收

- `0x00410730`两个站点直接查询共享链。第三mask命中与低ID最终覆盖仍按原顺序各查询一次，返回低word作为附加值并把分母设为`-1`；typed-stop阻断当前行替换。
- `0x0043C0D0`按1..500顺序查询并把返回低byte写第二张状态表；typed-stop保留已写前缀并阻断字符串、entry和action初始化后缀。
- `0x00442CA0`仅在guardian slot 9/10且seed不是`0xFFDC`时查询；结果低word零扩展写`+0x4C`，typed-stop保留三个sentinel前缀。
- `0x004539B0`动作6在目标阶段开始前查询target code计数；无符号计数至少20时发布辅助门。typed-stop阻断目标阶段检查、启动和推进。
- `0x00466950`先查询组B指标来源，再以其返回EAX作为键查固定计数；低word阈值10/15继续控制生命文字和渐变。typed-stop阻断全部指标后缀。

上述caller的旧opaque查询槽已删除或改为reserved稳定枚举值，生产路径零调用。Fame载入caller仍保持原隔离边界。

## 5. 验证与限制

叶函数UT覆盖根命中、动态节点命中、缺失、顺序扫描、EAX清零、ECX命中/缺失状态、EDX低word替换、根键读取、未映射next和命中计数读取的全部typed-stop前缀。

caller回归覆盖Dialog两次覆盖、标准模式500键状态表、guardian slot9完成与故障、动作6阈值与后缀阻断、提示帧阈值与故障寄存器；旧接口名称全仓扫描为零。定向4/4、battle聚合连续10次、Linux core`194/194`、AddressSanitizer`194/194`和Linux app`200/200`全部通过，最终源码零warning、sanitizer finding或runtime error。验证期间未启动原版或OpenSWD3游戏程序。

当前缺少原版固定键链、Fame载入状态及七个callsite的EAX/ECX/EDX联合捕获后端，动态差分登记为`blocked_runtime_oracle`；这不阻止完整LST静态闭合和Linux门禁。
