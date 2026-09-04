# 战斗MON定义说明释放 `0x00478220`

状态：`platform_adapted`、`unit_tested`、`caller_reclaimed`。

## 1. 完整LST范围

唯一行为真值为`swd3.exe.lst`。完整主体为`0x00478220..0x00478243`，从`proc`到`endp`共23个物理行、11条实际指令、1个call、1个条件跳转、1个局部标签和1个返回点，没有外部`FUNCTION CHUNK`。唯一callee为CRT风格释放包装`0x004885A0`；该包装把token与固定参数1转发到更深的`0x004885C0`堆边界。

物理xrefs共有16处：`0x00454C4C`、`0x00455ECE`、`0x00468D08`、`0x00468DEA`、`0x00469058`、`0x0046E8C4`、`0x0046E9F4`、`0x004707CC`、`0x004757D1`、`0x0047585F`、`0x00477A46`、`0x00477B6B`、`0x00477BA3`、`0x0048035D`、`0x004803AF`和`0x004809A3`。前13处属于已关闭caller并在本工作包回收；后三处均位于待审`0x00480220`，继续保留`pending_478220`调用隔离，未提前宣告caller关闭。

## 2. 读取、释放与清零顺序

入口从cdecl参数取得定义对象token，随后无条件读取对象`+0xA0`的完整dword到EAX。对象token为零、对象不足`0xA4`字节或该读取不可达时，只能在这次原始读取处typed-stop，不调用释放且不写对象。

读取值为零时，原`test/jz`直接到统一尾部：EAX固定为零，ECX和EDX保持入口值，对象`+0xA0`不执行写入，宿主说明vector也不改变。

读取值非零时，先把该token压栈调用`0x004885A0`。只有释放边界正常返回后才执行对象`+0xA0 = 0`；释放调用typed-stop保留旧token与说明内容，阻断清零。释放完成后若对象写权限不足，则释放副作用和宿主说明清空已经发生，但对象中的旧token仍保留，严格对应原程序在释放之后、清零访问处失败的顺序。

## 3. 寄存器与返回合同

函数只以EAX承载从`+0xA0`读出的token。零token路径返回EAX零并完整保留入口ECX/EDX。非零路径把token作为释放参数；正常返回后执行的内存清零不改变EAX/ECX/EDX，因此三者完整保留`0x004885A0`返回残值。

modern结果分别记录对象读取次数、释放调用次数、对象写入次数、原token、故障token/偏移和最终EAX/ECX/EDX。typed-stop不生成防御性成功值，也不回滚已经完成的释放或更早caller副作用。

## 4. typed owner与端口边界

`LegacyBattleMonDatabasePort`新增窄的`release_legacy_battle_mon_definition_text()`自由边界；默认实现转发到既有`LegacyBattleMonDatabaseCall::release_definition_text`，不建立第二份分配器或说明堆状态。typed叶函数`release_legacy_battle_mon_definition_text()`接收实际`0xA4`定义span与其唯一宿主说明vector，按原顺序读取token、调用自由边界、清空宿主说明并写零token。

所有已关闭caller复用其原有物理owner：固定定义曲线和动作/对手scratch复用MON definition scratch；成长选择复用成长scratch或新建道具节点的定义/说明字段；组A召唤、NPC与角色资料准备复用`LegacyBattleGroupAConfigurationState::profile_record/profile_description`；组B配置与重配置复用元素`resource_bytes/resource_description`。没有复制`+0xA0`token到第二个可独立变化的owner。

## 5. caller回收

13个已关闭站点在原调用顺序直接组合typed叶函数：动作23消息、对手wave scratch、成长角色选择两处、成长结果选择、组A召唤、组A NPC、角色profile准备、组B行动配置、组B行动重配置、定义驱动固定曲线设置以及固定定义曲线查询的命中/缺键两支。每个caller在线程自身MON loader返回寄存器后执行释放，并只在叶函数完成后继续原后缀。

对手wave路径特别复用紧邻前一MON定义读取所写的同一scratch及说明vector；新建成长道具路径在caption复制前补回`0x00469058`的第三次释放；组B重配置保留一个仅供`0x00480220`使用的窄caller隔离端口，使该待审函数的三个物理站点仍通过`pending_478220`，而已关闭的`0x0047585F`直接走typed叶函数。

旧组A/组B/成长结果opaque release枚举位置改为`reserved_*`以保持数值稳定，生产代码不再调用这些已回收槽。`pending_478220`只在待审脚本caller适配器中保留。

## 6. 验证与阻塞

独立leaf回归覆盖零token、非零释放并清零、释放寄存器残值、对象读取typed-stop、释放调用typed-stop和释放后对象写typed-stop。caller回归覆盖13个已关闭站点所属的动作、对手、成长、组A、组B与固定定义曲线路径，并锁定说明token清零、宿主说明释放、调用次数、寄存器线程和待审`0x00480220`隔离不被回收。

验证：定向测试`2/2`、Linux core`196/196`、AddressSanitizer`196/196`、Linux app`202/202`、连续10轮完整core、changed-range clang-format和release审计全部通过；最终日志零OpenSWD3源码warning、测试失败、sanitizer finding或runtime error。库存生成器连续双跑逐字节一致，工作包为`275/422 = 265 platform_adapted + 10 assembly_exact + 147 pending_audit`，SHA256为`d34db5958221db9cc7ddef777d4fd8b90d076466b2ef8009ce3c2612518da23a`。系统TMP分类没有选中或可疑仓库产物，且未启动原版或OpenSWD3游戏程序。

原版定义对象、说明堆、CRT free边界及16个caller/callee联合寄存器捕获后端缺失，`original_diff_verified`登记为`blocked_runtime_oracle`。该阻塞不影响完整LST静态闭环、typed故障隔离、13个已关闭caller回收和Linux验证。
