# 战斗组A角色工作区零化 `0x0046E6A0`

状态：`platform_adapted`。完整LST、typed物理视图、唯一caller边界、验证和inventory双生成均已收敛。

## 1. 完整权威范围与ABI

权威LST主体为`0x0046E6A0..0x0046E720`，从proc到endp共46行、26条实际指令、0个call、0个跳转、1个返回点，没有外部`FUNCTION CHUNK`。函数是无栈参数thiscall，保存并恢复EDI；EDX保存this，EAX以xor清零，三个`rep stosd`结束后ECX为零，因此返回寄存器固定为EAX零、ECX零、EDX原this。

## 2. 精确写入集合与顺序

函数先按物理顺序清十二项显式字段：十一项u16位于`+0x2F10..+0x2F24`，访问顺序为`+0x2F1E,+0x2F20,+0x2F22,+0x2F10,+0x2F12,+0x2F1C,+0x2F1A,+0x2F24,+0x2F14,+0x2F18,+0x2F16`；其中在前三项后清一项`+0x2F0C`u32。

随后三个`rep stosd`依次清：

- `+0x2BC8`起的`0xBE`个dword；
- `+0x0AF0`起的`0x4C`个dword；
- `+0x2B24`起的`0x29`个dword。

前后两段恰好组成`+0x2B24..+0x2EBF`连续`0xE7`个dword，但typed实现仍保持先高段、再早期工作区、最后低段的原顺序。`+0x2F0E`和`+0x2F26`不在写入集合，不扩大清零范围。

## 3. typed物理owner与caller边界

`LegacyBattleGroupAWorkspaceState`以`0x4C`个早期dword、`0xE7`个后期dword、显式u32和十一项尾u16承接完整写集，并保留两个相邻未写word用于回归。该状态挂入`LegacyBattleStartupState::party`的每个组A角色记录，作为后续构造、帧处理和关闭路径可复用的唯一物理视图；对象地址只作为`compat::u32` token返回，不转换为主机指针。

唯一caller是尚未审计的`0x0046E730`，它在保存EBX/ESI/EDI并把this移入EBX后立即调用本函数，随后完全重建EDX/EAX/ECX，因此不消费返回寄存器。当前已关闭startup只通过`configure_party_actor`窄端口隔离整个caller；本项不穿透或伪造caller余下复制、字段夹值和诊断行为。第172项审计caller时必须直接组合本typed零化器并删除对应内部opaque边界。

## 4. 验证状态

定向测试用非零模式填满全部触及范围，验证三段精确清零、十二项显式清零、两个相邻word保持、各段计数和固定返回寄存器；另通过startup组A角色记录直接调用，验证写入的是startup唯一物理视图而非孤立副本。定向测试、AddressSanitizer、Linux core `188/188`和Linux app `194/194`全部通过，源码零warning；app仅出现既有ALSA开发库CMake提示。

inventory生成器连续双跑逐字节一致，正式计数为`171/422 = 162 platform_adapted + 9 assembly_exact + 251 pending_audit`，SHA256为`d879fea89c0e09f1ae20585691376351145d022eae2143669394d4c03e2c0aa0`。原版完整组A对象、三段物理内存和caller寄存器联合捕获后端缺失，`original_diff_verified`登记为`blocked_runtime_oracle`。
