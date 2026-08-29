# 战斗组A角色共享资源双写 `0x0046E850`

状态：`platform_adapted`。完整LST、typed资源pair、startup唯一caller、验证和inventory双生成均已收敛。

## 1. 完整权威范围与ABI

权威LST主体为`0x0046E850..0x0046E860`，从proc到endp共11行、4条实际指令、0个call、0个跳转、1个返回点，没有外部`FUNCTION CHUNK`。函数是thiscall并由callee弹出一个dword参数。

入口把参数读入EAX，按原顺序把同一完整dword写入角色`+0x2EC8`和`+0x2ECC`，随后直接返回。正常返回EAX为资源token、ECX保持原this、EDX完整保持入口陈旧值。

## 2. typed owner与停止点

`LegacyBattleGroupAResourcePairState`唯一承接两个相邻资源token，不把参数转换为主机指针。角色token为零时，typed-stop放在首次`+0x2EC8`写位置；两项旧值、写计数和入口寄存器均保持，第二项不会被写。

## 3. startup caller回收

唯一caller位于初始队伍资源绑定循环：前一项pending value callee返回后，原路径以固定共享资源token调用本函数，再继续pending名字绑定。startup现在捕获前一callee的EDX并直接调用typed双写器；正常路径继续名字绑定，typed-stop则阻断名字及后续角色。

旧`apply_party_palette`枚举值改为reserved，未平移后续枚举；本函数不再经过平台端口。typed结果按角色索引记录两次写和返回寄存器。

## 4. 验证状态

纯函数测试覆盖两项非零旧值被同一token按序替换、固定EAX/ECX、陈旧EDX完整直传，以及零角色token在首次写停止且不改任一字段。startup回归验证两名初始角色直接写入同一资源token、前一value callee的EDX进入typed结果、旧palette槽零调用且后续绑定继续。定向测试、AddressSanitizer、Linux core `188/188`和Linux app `194/194`全部通过，源码零warning；app仅出现既有ALSA开发库CMake提示。

inventory生成器连续双跑逐字节一致，正式计数为`173/422 = 164 platform_adapted + 9 assembly_exact + 249 pending_audit`，SHA256为`39296e047e86652a3e46b68c8bf4ce5496227c435f358a7f6574c6df0c4ea5d8`。原版组A对象、共享资源表和caller前后callee寄存器联合捕获后端缺失，`original_diff_verified`登记为`blocked_runtime_oracle`。
