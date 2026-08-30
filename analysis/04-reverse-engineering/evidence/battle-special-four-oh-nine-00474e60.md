# 战斗特殊动作四百零九四阶段协调器 `0x00474E60`

状态：`platform_adapted`。完整LST、typed实现、唯一caller回收、定向测试、AddressSanitizer、Linux完整门与inventory双生成均已关闭。

权威LST主体为`0x00474E60..0x00474FB4`，proc至endp共158行、97条实际指令、4个call、9个跳转、7个局部标签、2个返回点，没有外部`FUNCTION CHUNK`。隐藏this是group-A行动者，唯一显式参数是目标token；唯一caller位于动作dispatch的动作409分支。

函数入口先把EAX、ECX、EDX全部清零，再以行动者profile低word加`0x5DC`发布主记录动作id，以special profile variant低word发布base variant，并把转身完成latch写一。gate零调用主记录更新callee；gate一先把base variant整dword加一，再查询目标坐标并把两个输出与主记录交给坐标更新callee；gate二把直接效果模式byte的bit3置位，把base variant整dword加二，并以目标token、动作id、variant调用阶段callee，只有返回一才把gate写三。

每次调用完成阶段处理后都检查主记录`field_8c`。精确为一时先完整清零152字节主记录，再把gate整dword加一；因此阶段callee、记录完成和最终完成允许在同一帧级联。主记录`field_5a`低byte bit3命中时，在共享完成flags低word中置bit15并完整清零该word。最终只有共享完成flags bit0已置且gate按signed dword比较不小于三时返回一；成功路径先把gate写零，再完整清主记录。其余路径返回零，保留阶段callee后的陈旧EDX、gate比较覆盖的EAX/ECX，以及两次`rep stosd`后ECX归零的行为。

实现复用既有group-A行动者的主记录、runtime gate、direct mode、profile值、special variant和转身latch唯一owner，以及既有group-A共享完成flags owner。主更新、目标坐标、坐标更新和阶段二callee保留窄typed边界；坐标更新边界显式携带主记录引用。共享owner缺失只在原版首次共享flags访问点typed-stop，此前动作id、variant、latch、阶段更新、坐标和记录清理副作用全部保留。actor owner缺失时停在原版首次profile读取点，并保留此前三个寄存器清零。

动作dispatch的动作409 production caller已改为typed直连，旧`0x00474E60`整函数opaque调用删除。测试覆盖入口寄存器清零、gate零主记录更新、gate一variant与坐标传递、gate二mode bit和返回一门、主记录完成清理与gate递增、field5a到共享bit15发布、同帧三级联、共享flags最终门、signed gate比较、typed-stop副作用，以及production caller零旧地址调用。

验证：定向测试通过；独立AddressSanitizer通过且零finding；Linux core `188/188`与Linux app `194/194`全部通过，源码零warning。inventory连续双生成逐字节一致，稳定为`230/422 = 221 platform_adapted + 9 assembly_exact + 192 pending_audit`，SHA256为`0cc03fb3dea71c05aef606c1dadbb43d79921904905afebdaac83e922720d37c`。动态差分因原版行动者主记录、目标坐标、直接效果、共享完成flags、四个callee和唯一caller寄存器联合捕获后端缺失而登记为`blocked_runtime_oracle`。
