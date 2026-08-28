# 战斗动作模式刷新 `0x00464E90`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`、`closed_callers_reclaimed`。

## 1. 完整权威范围

权威LST主体为`0x00464E90..0x0046508E`，从proc到endp共215行、128条实际指令、3个静态call、14个跳转、10个局部/返回标签、1个`retn`，没有外部`FUNCTION CHUNK`。

九个静态caller现已全部关闭：逐帧输入分派七处、目标选择入口一处、动作摘要一处。三个callee均为group-A角色对象查询，当前通过三项窄平台操作保留。

## 2. 权限与动作workspace初始化

函数不使用caller EAX/ECX。入口先把权限dword `0x00524414`和`0x00524418`写成`0x01010101`，再读取queued actor code；随后依次清三个动作文字token、把EDX低word清零、清权限地址`0x00524419..1B`和动作计数word。EDX高word严格保留caller值。

九字节权限域继续由`0x00524413`前置byte与后续两个dword组成；入口不写前置byte。初始化后物理权限1..5为1，权限6..8为0。动作文字token复用启动reset的三项连续dword；动作code复用`0x004FE5CC`低/高word及`0x004FE5D0`，入口不清旧code，只以count界定有效前缀。

queued code按u32减8形成actor index。index为0且第一个party presence byte精确等于1时使用source 0；mode flags bit1置位时先发布固定code 6、固定文字token、count 1和权限6。其余路径在首次真实访问时把actor index限制到物理四项映射，再读取映射source；source超出四组时在首次option pointer读取停止。

## 3. 两项动作来源与物理文字查找

每个source只读前两个对象token。对象token读取后，首次访问对象`+0x54`的u16动作code；缺失token在该字段访问停止。ECX只替换低word为动作code并保留token高word。

code仅在unsigned `[0x15,0x32)`内登记。每个命中项按旧count先写权限`6+count`，再把DX低word加1，依次写动作code、共享count和文字token，最后恢复EBX=1。最多两项普通动作；加固定项时最多三项，不预清未覆盖的旧workspace尾部。

文字查找保留原始`dword_4A74A0[code]`越界物理语义：

- code `0x15..0x29`读取审计到的21项固定token；该物理表由本实现唯一持有，动作摘要直接复用其中索引16..19，替代网格列表标题直接复用索引6的`0x004A76A0`，模式网格标题直接复用索引9的`0x004A7688`；
- code `0x2A..0x31`跨入相邻live全局，依次复用action kind、published actor、target cursor、独立`0x004A7554`值、三项列表选择及selection actor code owner。

因此没有把该读取现代化为独立安全字符串表；动态全局变化会直接成为动作文字token。

## 4. 三次角色裁剪与最终回退

动作workspace完成后，函数以同一queued actor执行三次对象查询：

- 第一次EAX=`index*0x3EF`、ECX为group-A对象token、EDX保留入口高word与动作count；返回0时清权限2；
- 第二次EAX=`index*0xBCD`、ECX为同一对象token、EDX为queued code；返回0时清权限4；
- 第三次EAX=`index*0x3EF`、ECX为同一对象token、EDX=`index*0xBCD`。

第一次真实对象call才检查十槽group-A边界，并保留此前权限与动作workspace副作用。第三次返回不等于1时直接返回callee寄存器。

第三次返回1时，函数读取live action kind，清两个权限dword，再只置权限2与权限5。随后按action kind读取九字节物理权限；索引越界在该最终读取停止，并保留稀疏重置。权限byte为0时把action kind改为2；非零时保持原值。EAX返回读取到的零扩展byte，ECX保持刷新前action kind，EDX保持第三callee结果。

## 5. caller回收与唯一owner

逐帧输入分派七个静态callsite由八键循环的一条typed分支统一直连。刷新普通返回后才从共享权限owner实时读取对应byte，不再使用刷新前快照；刷新typed-stop阻断动作kind、gate、selection及目标选择发布。

目标选择入口在两条原分支统一直连刷新；刷新typed-stop保留sample、gate和角色原点发布并立即返回。原`refresh_action_mode`枚举数值改为`reserved_action_mode_refresh_slot`，生产代码零调用。尚未审计的`0x004651D0`caller留到所属工作包。

`0x004A75C8`起的物理映射已收敛为启动状态中的单一十dword视图：启动与逐帧输入路径保持前四项边界，选择帧复用同一视图并保留后六项相邻数据，不再维护第二份标签数组。option source对象token与动作code由启动状态提供单一typed存储。

定向测试覆盖初始化写集、EDX高word、code范围两侧、两项普通动作、固定动作、21项静态与8项live相邻查找、三次对象寄存器形状、两类权限裁剪、最终回退/保持/越界、映射/source/object停点、逐帧输入live权限、两个已关闭caller普通与typed-stop传播，以及reserved槽零调用。定向测试、AddressSanitizer、Linux core 188/188、Linux app 194/194全部通过；源码零warning，app仅保留既有ALSA开发库CMake warning。工作包连续双跑逐字节一致，稳定为`129/422 = 124 platform_adapted + 5 assembly_exact + 293 pending_audit`，SHA256为`f0bc8129c6aff3c5b88836eb909085558c2a9f2eeffde7bd998016a7f9f0c071`。

当前缺少原版四组option对象、三个角色查询callee共享副作用及EAX/ECX/EDX联合动态捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
