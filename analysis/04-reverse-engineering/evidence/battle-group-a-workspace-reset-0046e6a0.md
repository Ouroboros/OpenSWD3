# 战斗组A角色工作区零化 `0x0046E6A0`

状态：`platform_adapted`。B11实际字段绑定及本页旧2F0E结论的修正见
[队伍入战接线](battle-party-startup-runtime-binding.md)。下述全量数字为历史验证。

## 1. 完整权威范围与ABI

权威LST主体为`0x0046E6A0..0x0046E720`，从proc到endp共46行、26条实际指令、0个call、0个跳转、1个返回点，没有外部`FUNCTION CHUNK`。函数是无栈参数thiscall，保存并恢复EDI；EDX保存this，EAX以xor清零，三个`rep stosd`结束后ECX为零，因此返回寄存器固定为EAX零、ECX零、EDX原this。

## 2. 精确写入集合与顺序

函数先按物理顺序清十二项显式字段：十一项u16位于`+0x2F10..+0x2F24`，访问顺序为`+0x2F1E,+0x2F20,+0x2F22,+0x2F10,+0x2F12,+0x2F1C,+0x2F1A,+0x2F24,+0x2F14,+0x2F18,+0x2F16`；其中在前三项后清一项`+0x2F0C`u32。

随后三个`rep stosd`依次清：

- `+0x2BC8`起的`0xBE`个dword；
- `+0x0AF0`起的`0x4C`个dword；
- `+0x2B24`起的`0x29`个dword。

前后两段恰好组成`+0x2B24..+0x2EBF`连续`0xE7`个dword，但typed实现仍保持先高段、再早期工作区、最后低段的原顺序。`+0x2F0E`属于`+0x2F0C`的DWORD，随之清零；只有`+0x2F26`不在写入集合。

## 3. typed物理owner与caller边界

`LegacyBattleGroupAWorkspaceState`保存早期、后期工作区及显式字段，挂入startup.party。
删除独立2F0E字段，避免与2F0C DWORD相矛盾。临时bindings同时借用实际action、
final_processing、item_effect、particle字段，在原清零位置发布对应写入。
包括七条动作记录、派生字段和缓存物品ID；保留2EF8计数与2F26 tick。
对象地址只作为compat::u32 token返回，不转换为主机指针。

唯一caller为0046E730，已直接组合本函数；核心startup及SDL入战配置均传入
对应的临时字段bindings。workspace写入早于placement复制、来源读取和诊断。
caller随后重建EDX/EAX/ECX，不消费本函数返回寄存器。

## 4. 验证状态

定向测试用非零模式填满全部触及范围，验证三段精确清零、十二项显式清零、相邻word保持、各段计数和固定返回寄存器（旧2F0E保持断言错误，当前已修正）；另通过startup组A角色记录直接调用，验证写入的是startup唯一物理视图而非孤立副本。定向测试、AddressSanitizer、Linux core `188/188`和Linux app `194/194`全部通过，源码零warning；app仅出现既有ALSA开发库CMake提示。

inventory生成器连续双跑逐字节一致，正式计数为`171/422 = 162 platform_adapted + 9 assembly_exact + 251 pending_audit`，SHA256为`d879fea89c0e09f1ae20585691376351145d022eae2143669394d4c03e2c0aa0`。原版完整组A对象、三段物理内存和caller寄存器联合捕获后端缺失，`original_diff_verified`登记为`blocked_runtime_oracle`。
