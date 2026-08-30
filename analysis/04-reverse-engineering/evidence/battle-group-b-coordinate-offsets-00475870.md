# 战斗组B资源坐标增量读取 `0x00475870`

状态：`platform_adapted`。完整LST、typed资源owner、唯一caller、分支边界测试、ASan/Linux门禁和inventory双生成收敛后关闭。

## 1. 完整权威范围与ABI

权威LST主体为`0x00475870..0x0047588F`，从proc到endp共16行、9条实际指令、0个call、0个跳转、0个局部标签和1个返回点，没有外部`FUNCTION CHUNK`。函数是thiscall，入口ECX为组B actor，两个栈参数依次为第一、第二个u16输出指针，callee以`retn 8`弹出参数。

唯一直接caller是最终角色步进`0x0045AA00`的`0x0045ACE3`。caller把两个栈上u16局部量清零后传入，callee完整返回后才分别读回并累加到共享坐标增量；任何callee访问故障都阻断两个caller累加和后续描述符查询。

## 2. 第一项读取与故障顺序

入口第一条指令读取`actor+0x0C`资源token到EAX。actor状态缺失时严格停在该读取点，保留入口EAX、actor ECX和入口陈旧EDX，两个输出均不写。

资源token随后用于读取`resource+0x62`的u16到DX。该指令只覆盖EDX低word，高word保持入口值；资源token为零时停在此读取点，EAX为零、ECX仍为actor、EDX仍为入口值。读取成功后才把第一输出指针载入EAX并写入该u16；第一输出不可访问时保留资源读取和DX低word覆盖，但不写第二输出。

## 3. 第二项读取、部分写与终端寄存器

第一输出成功后，函数重新读取`actor+0x0C`到ECX，再把第二输出指针载入EAX。随后读取`resource+0x8A`到DX低word并写第二输出。第二输出不可访问时第一输出已经写入，EAX为第二输出token，ECX为资源token，EDX高word保持入口值、低word为第二资源值。

普通返回时两个输出分别等于资源`+0x62/+0x8A`的原始u16；EAX为第二输出token，ECX为重读资源token，EDX高word保持入口陈旧值且低word为第二资源值。函数不做符号扩展、不比较数值、不调用任何callee，也不修改actor或资源。

## 4. typed owner与caller回收

实现直接读取`LegacyBattleStartupState::group_b_lifecycle`中的八槽组B元素和164-byte资源块，不建立第二份坐标或资源状态。final-actor在原有效性callee返回1后解析同一actor槽，调用typed读取器；只有完整返回才以u16回绕累加两个输出。owner、actor或资源缺失统一向父级传播`group_b_coordinate_offset_typed_stop`，保留有效性检查前缀并阻断描述符、动作、攻击顺序和终止门后缀。

`0x00475870`生产opaque调用已删除。纯函数测试覆盖actor与资源首次读取、第一输出故障、第二输出故障后的部分写，以及普通双输出与EAX/ECX/EDX终端寄存器；final-actor与组B帧测试覆盖共享owner、u16累加、旧token零调用、typed-stop传播及后续描述符故障顺序。

## 5. 验证与动态阻塞

战斗聚合定向测试覆盖纯函数边界和final-actor/组B帧caller，随完整core门禁通过。独立`./build-asan.sh`经公共`build.sh`完成core AddressSanitizer `188/188`，`./build.sh core`完成`188/188`，`./build.sh app`完成`194/194`；零OpenSWD3源码warning、sanitizer finding或测试失败，app仅保留既有ALSA环境提示。inventory生成器连续双跑逐字节一致，关闭进度为`241/422 = 232 platform_adapted + 9 assembly_exact + 181 pending_audit`，SHA256为`bdb2b3b42020d847ae21bc3f93054154fd8b6cb1e5a51017a7da01ca0c6c94e6`。

当前缺少原版八个组B actor、动态164-byte资源、两个真实栈输出地址、入口陈旧EAX/EDX、caller共享坐标与寄存器联合捕获后端，因此`original_diff_verified`登记为`blocked_runtime_oracle`。
