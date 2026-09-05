# 战斗组A角色配置 `0x0046E730`

状态：`platform_adapted`。完整LST、typed配置状态、两类callee、startup唯一caller、验证和inventory双生成均已收敛。

## 1. 完整权威范围与ABI

权威LST主体为`0x0046E730..0x0046E841`，从proc到endp共122行、80条实际指令、2个call、8个跳转、1个返回点，没有外部`FUNCTION CHUNK`。函数是thiscall并由callee弹出三个dword参数，保存EBX/ESI/EDI。

三个参数依次是56-byte源记录token、96-byte步长辅助记录token和32-byte placement token。正常尾固定返回EAX为角色内基础记录token、ECX为源记录token；EDX低word来自角色内记录`+0x12`，高word来自placement token，若进入诊断则改取诊断callee返回EDX的高word。

## 2. 复制与发布顺序

函数首先直接调用已关闭的工作区零化器。随后从placement复制8个dword到角色`+0x0D50`，再从该角色副本复制8个dword到`+0x0D70`，并把placement末dword另写`+0x2AA0`。

接着把源记录14个dword复制到角色首token指向的基础记录。复制完成后才把源记录token写入角色`+4`、辅助记录token写入`+8`，并从角色副本`+0x1E`读取byte发布到角色`+0x2A93`。因此后续对源记录的夹值不会反向改变本次角色副本。

placement `+0x14`的u16同时写入角色`+0x2A0C`；值为零时，以窗口token、固定文字token、flags零、固定源文件token和行号调用公共诊断边界。诊断完成后继续执行，不中断配置。

## 3. 尾部字段复制与signed夹值

角色基础记录内部先把u16 `+0x10`复制到`+0x26`，再把u16 `+0x12`复制到`+0x28`。随后以常量9999按`+0x0A,+0x04,+0x0C,+0x06,+0x0E,+0x08`顺序检查源记录六项u16；比较使用signed `jle`，因此负i16保持原值，仅signed大于9999的值被写成9999。

最后先清角色`+0x2AB8`，再测试角色副本`+0x25`的bit7；置位时把该dword改写为1。typed实现复用`LegacyBattleActorProgressState::special_ready`，不创建第二份完成状态。

## 4. typed owner、停止点与callee

`LegacyBattleGroupAConfigurationState`承接角色内14-dword基础记录、两份placement副本、两个源token和发布字段；四份可写源记录由startup状态唯一持有。placement的角色编号、坐标和active继续复用startup组A角色记录，调用时只构造只读32-byte值视图。

placement token、源记录token或角色基础记录token为零时，typed-stop分别放在原始首次placement读、源读或目标写位置。工作区零化和此前完成的placement复制保持不回滚。

`0x0046E6A0`已直接组合，旧地址不再经过端口。`0x00431150`是尚无通用typed实现的公共诊断边界，仅保留窄诊断port，并完整传播其EDX到后续寄存器链。

## 5. startup caller回收与验证状态

startup唯一caller不再调用`configure_party_actor`旧业务槽；原枚举值改为reserved，新诊断服务追加到枚举尾。caller把实际组A actor token、四份源记录、角色placement owner和窗口token传给typed配置器；typed-stop会阻断后续角色mode查询，并保留此前startup副作用。

纯函数测试覆盖非零placement双复制、14-dword先复制后源夹值、负i16保留、内部word复制、特殊bit、正常返回寄存器、零placement诊断时序/参数/陈旧EDX高word及三类首次访问typed-stop。startup回归覆盖双角色直连、旧槽零调用、源token、镜像后placement、工作区直接组合、夹值不污染角色副本和零角色诊断转发。定向测试、AddressSanitizer、Linux core `188/188`和Linux app `194/194`全部通过，源码零warning；app仅出现既有ALSA开发库CMake提示。

inventory生成器连续双跑逐字节一致，正式计数为`172/422 = 163 platform_adapted + 9 assembly_exact + 250 pending_audit`，SHA256为`1e8eb49e2803ef138da7b3b4a23e16e85967aa751360dfbf5056e41f9b9d5ac0`。原版完整组A对象、源/placement记录、窗口、诊断callee和caller寄存器联合捕获后端缺失，`original_diff_verified`登记为`blocked_runtime_oracle`。
