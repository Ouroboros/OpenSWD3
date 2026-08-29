# 战斗目标演出阶段初始化 `0x004710D0`

状态：`platform_adapted`。完整LST、typed实现、action dispatch caller回收、定向测试、AddressSanitizer、Linux完整门与inventory双生成均已关闭。

## 1. 完整权威范围

权威LST主体为`0x004710D0..0x0047126B`，proc至endp共171行、104条实际指令、7个call、3个跳转、3个局部标签、1个返回点，没有外部`FUNCTION CHUNK`。五个函数callee为`0x00478620`、`0x00478470`、`0x004019A0`、`0x0047CE70`与已关闭host-surface设置`0x00433F30`；另有两次`GetSystemMetrics`导入调用。

## 2. 资源、演出记录与算术

函数先调用目标资源准备并把返回token写入actor `+0x255C`，再查询两项坐标差，随后把actor `+0x0E14`起始的`0x58`字节演出记录清零。资源对象或其首字段不可表示时，typed路径在原始解引用点停止，并保留两次callee与记录清零前缀。

资源可用时调用图像解码窄port，把返回token写入演出记录；resource `+0x0C/+0x0E`两个word分别发布为宽高。坐标差按signed low-word扩展，第一项减一；演出X按`source_x - source_y + actor_x - 0x32`的32位回绕算术生成，演出Y按`actor_y - vertical_adjustment + 0x28`生成。固定宽高为`0x14/0x1E`，两个active dword为1，三项step为5，spacing为`0x28`。

resource height按unsigned word分支：小于100时16位减10，否则逻辑右移一位。flags初值`0x56`；目标属性查询严格等于1时并入bit0。actor `+0x2A87` byte再并入bit3。

## 3. host surface、清零与owner

原版用系统宽高重建host surface。OpenSWD3复用startup中的唯一`LegacyBattleRenderGeometry` owner，并以当前SDL raster surface宽高完成平台适配，直接调用已关闭`set_legacy_battle_host_surface`。其后按原顺序把actor `+0x2680`清零，并清空`+0x0DF4`的8个dword、`+0x0500`的38个dword和`+0x2BC8`的190个dword。

action dispatch为每个group-A行动者持有唯一`LegacyBattleTargetPhaseState`；其中复用既有`LegacyBattleImageParticleEmitter`作为actor `+0x0E14`物理记录owner，并持有资源token、mode byte与三段清零区，不复制第二份状态。隐藏this是group-A行动者，显式参数才是group-B目标token。`0x00478620`、`0x00478470`、`0x004019A0`和`0x0047CE70`尚未审计，继续作为窄callee port；不保留整个`0x004710D0` opaque adapter。

## 4. caller回收与验证

唯一caller为action dispatch `0x004539B0`的action 6阶段零分支。production现在按group-A index定位行动者的typed target-phase owner，以group-B token作为显式目标参数并直接调用初始化，失败映射到caller typed-stop；后续演出门、暗化、阶段word与刷新保持原顺序。

测试覆盖完整资源/坐标/解码/属性调用、signed与回绕坐标、低高度减十、flags、mode bit、host surface、全部236个尾部dword清零、资源对象原始解引用点typed-stop，以及action 6不再调用整函数地址。定向测试与独立AddressSanitizer均为`1/1`通过；Linux core为`188/188`，Linux app为`194/194`，源码零warning。inventory连续双生成逐字节一致，稳定为`204/422 = 195 platform_adapted + 9 assembly_exact + 218 pending_audit`，SHA256为`01134ec0bd9e21316e2cab6823607c18a4f1b8f2d2716028b6f0441ae1cec2e6`。动态差分因原版group-B资源对象、图像解码分配、坐标/属性callee和caller寄存器联合捕获后端缺失而登记为`blocked_runtime_oracle`。
