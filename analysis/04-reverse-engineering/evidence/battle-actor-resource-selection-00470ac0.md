# 战斗角色资源选择与资料应用 `0x00470AC0`

状态：`platform_adapted`。完整LST、typed实现、三处production caller回收、定向测试、AddressSanitizer、Linux完整门与inventory双生成均已关闭。

## 1. 完整权威范围

权威LST主体为`0x00470AC0..0x00470E15`，proc至endp共401行、244条实际指令、5个call、41个跳转、25个局部标签、4个返回点，没有外部`FUNCTION CHUNK`。内部链头提交`0x00470900`已关闭；三次待审profile加载`0x00476A80`和一次诊断`0x00431150`保留为窄port。

## 2. 前缀、遍历与选择

入口无条件清零10 dword profile buffer、4 dword pre-effect buffer和actor `+0x2F14`word。occurrence为零时在这些副作用后返回0。category 0至5映射与第198项相同，其余值原样使用。

非零occurrence先提交资源链头，再破坏性推进。普通匹配按mode bits 0/2、category和signed派生正值递增；category 4的node word `+0x4C` bit13再独立递增。两次递增完成后才比较occurrence，因此可跨过中间计数。

## 3. 选中后副作用

选中后对actor mode OR bit6，发布selected resource token并清输出mode。category 4且node bit13时，以node `+0x4A`加载profile，把node `+0x30`写入derived slot0并立即返回1。

普通路径优先使用node `+0x54`非零profile id，否则使用`+0x4A`。node bit15/14分别发布primary/secondary required，并与live actor record `+0x06/+0x08` signed容量比较，超限保留required后返回0。随后按node `+0x48/+0x49`门控复制三项derived word与profile copy latch。

函数输出actor `+0x0DA4`runtime word，零值触发诊断port。node category bit11发布mode bit4和`+0x2F14`；category 3/0发布输出mode 1；category 5发布mode 2，resource id非0x300时改为3并撤销mode bit5。最终除category 3及特定category flag组合外，对node `+0x06`做16位回绕递增。

## 4. owner、caller与验证

profile/pre-effect/latch复用final owner；mode与derived words复用item owner；required与selected复用链表owner；`+0x2F14`复用workspace owner；runtime word复用action execution owner；live容量复用configuration owner。第198项曾将word `+0x4C` bit13另建byte字段，本项已立即消除该重复并统一使用word owner。

三处静态caller均在已关闭目标选择刷新`0x00462740`：动作提交分支使用动态category，message 27固定category4，message 30固定category5。production party/action/startup输出均typed直连；脚本化单测compat开关默认关闭。本轮审计相邻释放函数时按权威地址重新核对caller，纠正了固定category两处先前误接到动作枚举槽的集成位置，并补充message 27 production caller测试。

测试覆盖普通alternate profile、容量门、copy latch、零runtime诊断、输出mode、数量递增、category 4早退及occurrence零清零前缀。定向测试与独立AddressSanitizer均为`1/1`通过；Linux core为`188/188`，Linux app为`194/194`，源码零warning。inventory连续双生成逐字节一致，稳定为`199/422 = 190 platform_adapted + 9 assembly_exact + 223 pending_audit`，SHA256为`3e76f8dad71e3912b77eaca20b3db2c7f854c5ccab607ad9a4fb0a343a96b156`。动态差分因原版资源节点、三profile加载、诊断、live容量和caller寄存器联合捕获后端缺失而登记为`blocked_runtime_oracle`。
