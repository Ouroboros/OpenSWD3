# 战斗目标效果计算、累计与发布 `0x00474FC0`

状态：`platform_adapted`。完整LST、typed实现、七处caller回收、定向测试、AddressSanitizer、Linux完整门与inventory双生成均已关闭。

权威LST主体为`0x00474FC0..0x004750B3`，proc至endp共121行、72条实际指令、6个call、7个跳转、5个局部标签、1个返回点，没有外部`FUNCTION CHUNK`。隐藏this是group-A行动者，两个显式参数依次为目标token与mode；七处caller分布在group-A动作执行器两处、特殊动作400三处和动作4/404两处。

函数先把行动者motion word和共享motion word清零，再把入口EAX、ECX、EDX的低word分别替换为行动者`+0xF6`、`+0xF8`、`+0x2F1A`，连同固定曲线表token调用曲线callee，并只把返回AX写入共享motion word。行动者`+0x26C0`byte为负时方向参数取八，否则取零。mode精确为一、`+0x2B14`latch为零且目标skip-gate callee返回非零时跳过后续效果流程；曲线采样和最终latch写一仍然发生。

未跳过时先刷新目标，再把共享last-effect写零并调用效果计算callee。返回AX按signed word扩展为32位；非零时再加共享motion word的signed word扩展，保留32位回绕。结果按signed dword与9999比较，大于等于时夹到9999，负值不夹。随后无条件把结果加到既有pair-primary累计owner，连`-1`也先累计；只有结果不精确等于`0xFFFFFFFF`时，才依次把效果值发布给目标并设置目标属性一。最后无条件把行动者effect-application latch写一，并返回最后callee或陈旧分支寄存器。

实现新增行动者`+0xF6`、`+0xF8`、`+0x2F1A`曲线输入、`+0x26C0`方向byte和`+0x2B14`latch唯一owner，复用既有`+0x2954`motion word、共享`0x00521520`motion word、`0x0053AE8C`last-effect及pair-primary累计owner。六个未审callee继续保留窄port；输出栈locals仅作为平台化输出槽，不伪造宿主指针。actor缺失停在原版motion word写点；shared缺失停在随后共享motion word写点，保留此前行动者清零副作用。

七处production caller全部改为typed直连，旧`0x00474FC0`整函数opaque调用删除。测试覆盖两个owner故障点、入口高半保留与三个低word替换、曲线四参数顺序、负方向八、mode-one skip、mode-zero完整路径、双signed word相加、pair-primary累计、9999 inclusive夹值、负一先累计后抑制发布、最终寄存器、三类caller及旧地址零调用。旧动作4测试曾因opaque桩不更新共享motion而失败，已按真实typed helper的曲线返回值校准。

验证：定向测试通过；独立AddressSanitizer通过且零finding；Linux core `188/188`与Linux app `194/194`全部通过，源码零warning。inventory连续双生成逐字节一致，稳定为`231/422 = 222 platform_adapted + 9 assembly_exact + 191 pending_audit`，SHA256为`00d5ee47d782af9cd16de91e08cd818a5678fa578d047c38647c2ae3a71a4fce`。动态差分因原版行动者曲线字段、目标对象、共享motion/last-effect、pair-primary累计、六个callee和七处caller寄存器联合捕获后端缺失而登记为`blocked_runtime_oracle`。
