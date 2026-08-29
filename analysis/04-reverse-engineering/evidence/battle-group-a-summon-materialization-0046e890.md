# 战斗组A召唤角色资料物化 `0x0046E890`

状态：`platform_adapted`。完整LST、typed资料物化、action15唯一caller、完整验证和inventory双生成均已收敛。

## 1. 完整权威范围与ABI

权威LST主体为`0x0046E890..0x0046E9BD`，从proc到endp共120行、102条实际指令、4个call、1个条件跳转、1个局部标签和1个返回点，没有外部`FUNCTION CHUNK`。函数是thiscall并由callee弹出一个dword参数。

参数是共享`0x0053AF70 + index * 0x20`的32-byte召唤源记录，this是同索引组A角色。正常尾返回EAX为新0xA4资料token、EDX为角色首token指向的基础记录token；ECX低word来自资料`+0x60`，高word保留资料`+0x96` dword的高word。

## 2. 分配、callee与停止顺序

函数先以固定0xA4调用旧分配边界，立即对返回记录执行41个dword清零，然后才把token写入角色`+0x0C`。随后首次读取源记录`+0x14`的角色编号，依次调用资料加载和资料内动态文字释放边界。两个callee对0xA4记录的变更由窄typed port回传，函数不伪造其文件、动态内存或寄存器副作用。

分配token为零时，typed-stop放在原始第一次清零写；缺少角色typed owner时，保留成功分配和41-dword清零后在角色`+0x0C`首次写停止；缺少源记录时，保留资料token发布后在源`+0x14`首次读停止。

## 3. 源记录复制、诊断与资料投影

资料加载和释放完成后，源记录8个dword先复制到角色`+0x0D50`，再完整复制到`+0x0D70`；源`+0x1C`另写角色`+0x2AA0`，源`+0x14`的角色编号另写`+0x2A0C`。角色编号为零时，以窗口token、固定文字token、flags零、固定源文件token和固定行号调用公共诊断边界，诊断后继续。

角色基础记录按原顺序接收资料word `+0x56→+0x26`、`+0x58→+0x28`、`+0x5A→+0x16`、`+0x5C→+0x14`；资料byte `+0x90`零扩展写为基础记录`+0x1E`的word。资料`+0x92`起9个byte按两个dword加一个byte的原布局写到基础记录`+0x2D..+0x35`。随后角色`+0x2A93`取基础记录`+0x1E`低byte，资料`+0x64`的word依次写基础记录`+4`和`+0x0A`，角色`+4`改写为基础记录token，资料`+0x60`的word写角色`+0xF2`。

基础记录token为零时，typed-stop放在第一次`+0x26`写；此前三次callee、两份源复制、尾值、角色编号和可选诊断全部保留，基础记录保持未写。

## 4. typed owner与caller回收

0xA4资料记录、token和投影字段并入`LegacyBattleGroupAConfigurationState`，继续复用startup `party[index]`的唯一角色owner。32-byte召唤源直接取同一party placement状态，只构造调用期只读值视图，不复制第二份物理全局数组。

action15首帧从共享状态取得召唤索引，以对应组A角色token和`0x0053AF70 + index * 0x20`源token直连typed物化器；窗口token由startup入口保存并沿同一owner传播。旧`0x0046E890` opaque调用生产零次，成功后才进入原phase、坐标和逐帧召唤路径；缺少shared owner时保留分配/清零前缀并阻断后续phase发布。

## 5. 验证状态

纯函数测试覆盖正常三callee顺序、41-dword清零、双份源复制、全部资料字段投影、9-byte非对齐复制、角色别名token、ECX高低word组合、零角色编号诊断，以及分配、角色owner、源记录和基础记录四类首次访问停止。caller回归覆盖action15共享owner直连、实际召唤索引、资料加载参数、旧调用零次、phase仅在成功后推进、固定诊断五参数和缺失owner前缀。

定向测试、AddressSanitizer、Linux core `188/188`和Linux app `194/194`全部通过，源码零warning。inventory生成器连续双跑逐字节一致，正式计数为`175/422 = 166 platform_adapted + 9 assembly_exact + 247 pending_audit`，SHA256为`1f0a60ebecb5d6c5a137cdf799c36d78f0f6372dc7c020c8a7a0374b277fda3d`。原版完整组A对象、0xA4资料、mon.dat加载/动态文字释放、诊断窗口和action15寄存器联合捕获后端缺失，`original_diff_verified`登记为`blocked_runtime_oracle`。
