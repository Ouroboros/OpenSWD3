# 战斗目标效果计算、累计与发布 `0x00474FC0`

状态：`platform_adapted`、`caller_refreshed`。完整LST、typed实现、七处caller回收及内部固定曲线callee回收均已关闭。

权威LST主体为`0x00474FC0..0x004750B3`，proc至endp共121行、72条实际指令、6个call、7个跳转、5个局部标签、1个返回点，没有外部`FUNCTION CHUNK`。隐藏this是group-A行动者，两个显式参数依次为目标token与mode；七处caller分布在group-A动作执行器两处、特殊动作400三处和动作4/404两处。

函数先把行动者motion word和共享motion word清零，再把入口EAX、ECX、EDX的低word分别替换为行动者`+0xF6`、`+0xF8`、`+0x2F1A`，连同固定曲线表token调用`0x00477830`，并只把返回AX写入共享motion word。该内部callee现已直接组合typed fixed-curve helper：共享根先参与键匹配，命中时计数递增并按最大值夹限，缺键时分配20字节节点，两条路径都以x87比值分别产生百分比word和最终motion。行动者`+0x26C0`byte为负时方向参数取八，否则取零。mode精确为一、`+0x2B14`latch为零且目标skip-gate callee返回非零时跳过后续效果流程；曲线推进和最终latch写一仍然发生。

未跳过时先刷新目标，再把共享last-effect写零并调用效果计算callee。返回AX按signed word扩展为32位；非零时再加共享motion word的signed word扩展，保留32位回绕。结果按signed dword与9999比较，大于等于时夹到9999，负值不夹。随后无条件把结果加到既有pair-primary累计owner，连`-1`也先累计；只有结果不精确等于`0xFFFFFFFF`时，才依次把效果值发布给目标并设置目标属性一。最后无条件把行动者effect-application latch写一，并返回最后callee或陈旧分支寄存器。

实现新增行动者`+0xF6`、`+0xF8`、`+0x2F1A`曲线输入、`+0x26C0`方向byte和`+0x2B14`latch唯一owner，复用既有`+0x2954`motion word、共享`0x00521520`motion word、`0x0053AE8C`last-effect及pair-primary累计owner。固定曲线根和动态节点复用`LegacyBattleFixedObjectStatePort`唯一owner；其余五个未审callee继续保留窄port。actor缺失停在原版motion word写点；shared缺失停在随后共享motion word写点；fixed-curve访问失败停在原`0x00474FF3`调用位置，保留两项motion清零及链内部前缀，阻断共享motion发布、后续callee和最终latch。

七处production caller全部改为typed直连，旧`0x00474FC0`整函数opaque调用删除；内部`0x00477830`opaque调用也已删除。测试覆盖actor/shared/fixed-curve三个故障域、入口高半保留与三个低word替换、真实固定链计数/百分比/motion、负方向八、mode-one skip、mode-zero完整路径、双signed word相加、pair-primary累计、9999 inclusive夹值、负一先累计后抑制发布、最终寄存器、三类caller及两个旧地址零调用。fixed-curve stop保留调用前清零且不写最终latch。

本页原工作包的定向、AddressSanitizer、Linux core与Linux app门禁均已通过；`0x00477830`回收后再次通过battle聚合定向测试，完整门禁结果记录在对应fixed-curve证据和模块摘要。动态差分因原版行动者曲线字段、固定曲线链、目标对象、共享motion/last-effect、pair-primary累计、其余五个callee和七处caller寄存器联合捕获后端缺失而登记为`blocked_runtime_oracle`。
