# 战斗角色动作候选可用性 `0x004624C0`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`。

## 1. 完整权威范围

权威LST主体为`0x004624C0..0x0046250A`，从proc到endp共47行、30条实际指令、1个静态call、3个跳转、3个局部/返回标签、2个`retn`，没有外部`FUNCTION CHUNK`。

三个静态caller分别位于尚待独立审计的正向候选轮转两处和反向候选轮转一处。唯一callee为既有group-A角色状态查询边界。

## 2. 固定count与队列扫描

函数唯一栈参数为候选actor code。入口先把group-A count装入EDX并把EAX清零；count为0时立即返回0，ECX保持caller入口值，EDX返回0。

count非零时才装载参数与十槽actor order起始token。扫描从索引0开始，逐dword完整比较候选code；不跳过零。未命中时EAX与索引同时加1，ECX加4，再以unsigned `eax < fixed_count`继续。count只在入口读取一次，循环中不重读live值。

普通未命中返回0，并保留ECX为队列起点加`count*4`的一过尾token、EDX为固定count。原循环不增加十槽现代上限；count大于10时，在第十项前缀完成后，于第十一次真实队列读取typed-stop，返回EAX 10、十槽一过尾token和入口count。

## 3. 候选角色查询

首个相等队列项立即结束扫描，不继续查找重复项。候选code按`code-8`保留完整乘移序列；角色查询前EAX为索引乘`0xBCD`，ECX为group-A对象token，EDX为入口固定count。

物理角色索引只允许0..9。code 7等非法候选在完成u32回绕地址计算后、首次真实对象call前typed-stop，保留EAX与ECX计算结果且不调用port。

## 4. 布尔返回与寄存器

角色查询返回后只把EAX归一：完整EAX为0时返回1，任何非零bit pattern都返回0。ECX/EDX完整保留callee返回。函数不读取或写入选择缓存，也不修改actor order和queued角色。

## 5. caller边界与验证

正向和反向候选轮转caller尚未达到各自工作包，因此当前既有动作轮转typed port边界保持不变；其后关闭caller时必须直接组合本实现，不能继续发出该callee的opaque操作。

定向测试覆盖count 0、固定count无匹配、零查询归一为1、非零查询归一为0、角色调用前EAX/ECX/EDX、十槽一过尾以及code 7回绕角色停点。

当前缺少原版十槽角色队列、group-A对象、角色查询callee共享副作用、三个caller候选表轨迹及EAX/ECX/EDX联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
