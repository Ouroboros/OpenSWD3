# 战斗组A角色共享数值双写 `0x0046E870`

状态：`platform_adapted`。完整LST、typed数值pair、startup唯一caller、验证和inventory双生成均已收敛。

## 1. 完整权威范围与ABI

权威LST主体为`0x0046E870..0x0046E880`，从proc到endp共11行、4条实际指令、0个call、0个跳转、1个返回点，没有外部`FUNCTION CHUNK`。函数是thiscall并由callee弹出一个dword参数。

入口把参数读入EAX，按原顺序把同一完整dword写入角色`+0x2EC0`和`+0x2EC4`，随后直接返回。正常返回EAX为输入值、ECX保持原this、EDX完整保持入口值。

## 2. typed owner与停止点

`LegacyBattleGroupAValuePairState`唯一承接两个相邻数值，不改变位型或截断。角色token为零时，typed-stop放在首次`+0x2EC0`写位置；两项旧值和写计数保持，第二项不会被写。

## 3. startup caller回收与相邻寄存器链

唯一caller位于初始队伍资源绑定循环。权威caller在调用前把队伍source索引保存在EDX，用该索引读取输入dword后调用本函数；返回EDX仍是source索引，紧接着进入已关闭共享资源双写。

startup现在以请求中的完整party value直连typed数值双写，并把source索引作为入口EDX；其返回EDX再直接传给`0x0046E850`typed资源pair。旧`apply_party_value`枚举值改为reserved，不平移后续ABI槽。任一typed-stop都会阻断资源与名字绑定及后续角色。

## 4. 验证状态

纯函数测试覆盖两项非零旧值被同一完整dword按序替换、固定EAX/ECX、source索引EDX直传，以及零角色token在首次写停止且不改任一字段。startup回归验证两名初始角色直连、完整party value双写、source索引依次为0/1并进入资源pair、旧value槽零调用且后续绑定继续。定向测试、AddressSanitizer、Linux core `188/188`和Linux app `194/194`全部通过，源码零warning；app仅出现既有ALSA开发库CMake提示。

inventory生成器连续双跑逐字节一致，正式计数为`174/422 = 165 platform_adapted + 9 assembly_exact + 248 pending_audit`，SHA256为`87a1424ffba3d06e6b76fb4cbe99b5841dd4d41e8d34dc8468419fdd2ff441be`。原版组A对象、party value表和caller相邻callee寄存器联合捕获后端缺失，`original_diff_verified`登记为`blocked_runtime_oracle`。
