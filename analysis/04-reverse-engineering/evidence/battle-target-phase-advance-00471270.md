# 战斗目标演出阶段推进 `0x00471270`

状态：`platform_adapted`。完整LST、typed实现、action dispatch caller回收、定向测试、AddressSanitizer、Linux完整门与inventory双生成均已关闭。

## 1. 完整权威范围

权威LST主体为`0x00471270..0x004714AF`，proc至endp共239行、159条实际指令、7个call、7个跳转、6个局部标签、2个返回点，没有外部`FUNCTION CHUNK`。callee为已关闭image-particle frame `0x00434790`一次、资源释放`0x004885A0`一次，以及尚未审计的粒子槽准备`0x00471FC0`最多五次。

## 2. 逐帧前缀与typed particle owner

每帧先让actor `+0x2F26` word加一并保留16位回绕，把emitter三项published值重写为5，再把actor `+0x2AFC` active gate写为1。之后直接调用已关闭`update_legacy_battle_image_particles`，复用group-A actor的唯一`LegacyBattleImageParticleEmitter`、共享节点池、CRT RNG、共享粒子参数、诊断与pixel conversion owner。缺少这些owner或typed particle frame在原访问点停止时，保留此前tick、published值和active gate副作用。

`0x004710D0`与本函数的隐藏this均为action dispatch中的group-A行动者；显式参数仅是前一函数的group-B目标token。为此上一工作包中错误挂到opponent record的临时owner已纠正为`group_a_target_phases[group_a_index]`，坐标输入复用`LegacyBattleGroupAActionExecutionState`中的actor物理字段，不复制group-B状态。

## 3. 完成分支

particle frame返回1时，tick与active gate清零；decoded resource token非零才调用窄释放port。随后清空完整emitter记录、五项spawn counter、`+0x0DF4`的8个dword和`+0x0500`的38个dword，保留`+0x2BC8`粒子槽记录，最后返回EAX=1。实现保持释放先于记录清零的顺序。

## 4. 非完成分支与五档发射

particle frame未完成且remaining-batches为零时直接返回0。非零时无条件调用第一档粒子槽准备；tick以signed word比较10、20、30、40，达到每个阈值后重复调用对应档位。因此tick 40及以上每帧都会发出五次调用，而不是一次性latch；tick从`0x7FFF`回绕到`0x8000`后四个signed阈值均失败，但无条件第一档仍执行。

五档参数严格保留resource id、kind/index、宽度四分之一、height减remaining-batches、纵向偏移`-5/-10/-15/0/+5`和迭代数`14/10/12/8/16`。所有坐标按32位回绕位型传给尚未审计的`0x00471FC0`窄port。

## 5. caller回收与验证

唯一caller为action dispatch `0x004539B0`的action 6公共推进点。production现在直接调用typed推进器；返回1才进入原后续完成提交、计数、attack-order移除、阶段word与物品发布路径，返回0保持原普通返回。整函数地址port已删除。

测试覆盖tick前缀、五档参数、tick 40重复五次、signed回绕只留第一档、particle完成资源释放与精确清零范围、`+0x2BC8`保留、既有action 6物品发布，以及action dispatch不再调用整函数地址。定向测试与独立AddressSanitizer均为`1/1`通过；Linux core为`188/188`，Linux app为`194/194`，源码零warning。inventory连续双生成逐字节一致，稳定为`205/422 = 196 platform_adapted + 9 assembly_exact + 217 pending_audit`，SHA256为`874eb006550504b7b8ff044e090b29cbdd3fef4f47b5d871d133720ea842ea32`。动态差分因原版decoded buffer、粒子槽callee、CRT seed和caller寄存器联合捕获后端缺失而登记为`blocked_runtime_oracle`。
