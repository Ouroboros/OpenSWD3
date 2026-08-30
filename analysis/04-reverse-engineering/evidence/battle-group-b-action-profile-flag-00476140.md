# 战斗组B行动资料标记判定 `0x00476140`

状态：`platform_adapted`。完整LST、唯一owner、caller陈旧寄存器、双向基本块追溯、定向/ASan/Linux门禁和inventory双生成收敛后关闭。

## 1. 完整权威范围与ABI

权威LST主体为`0x00476140..0x0047615E`，从proc到endp共17行、8条实际指令、0个call、1个跳转、1个局部标签和2个返回点，没有外部`FUNCTION CHUNK`。函数是thiscall：ECX为当前组B行动者，无栈参数，以普通`retn`返回。唯一caller是组B逐帧函数`0x004576A0`的`0x00457B8C`。

入口第一条指令就读取actor `+0x0D98`完整dword。modern actor映射缺失因此只在该真实访问点typed-stop，保留caller传入的陈旧EAX、actor ECX token和陈旧EDX。正常返回只覆盖EAX；ECX保持actor token，EDX保持入口值。

## 2. 两级标记优先级

函数先对actor `+0x0D98`完整dword测试`0x10000000`。该bit置位时直接把EAX写1并返回，不再读取fallback dword。

主bit未置位时，读取actor `+0x0D94`完整dword，逻辑右移12位后与1，返回原bit12的0或1。位操作是无符号完整dword，不改为byte测试，不把两个条件合并为通用布尔字段，也不读取其他相邻位。

这两个dword分别对应组B行动配置40-byte `profile_buffer`的偏移`0x08`与`0x04`。它们继续由`LegacyBattleGroupBActionConfigurationState::profile_buffer`唯一持有；实现不新增平行字段，也不误用组A执行状态中同物理地址的抽象字段。

## 3. 唯一caller与陈旧寄存器

普通packed-status路径先调用状态序列查询。查询结果不等于1时，`0x00476140`入口EAX/EDX直接继承该callee。

状态序列结果等于1时，caller先发布当前actor索引、清状态杂项、调用特殊行动查询并可置latch；随后无条件把EAX改为当前组B文字token。文字不存在时，EDX继承特殊行动callee；文字存在时，文字消息callee完成后的EAX/EDX成为本函数入口陈旧寄存器。文字消息typed-stop发生在本函数之前，不进入本函数。

modern caller显式线程上述EAX/EDX来源，再直连typed标记判定。返回1时才把行动side写1并把目标索引改为当前组B索引；返回0时两者保持原值。无论正常返回0或1，caller随后都发布当前actor索引并调用phase查询。actor typed-stop保留状态序列、特殊行动、latch、文字和此前actor索引副作用，阻断side/目标发布、第二次actor索引发布、phase查询及后缀。生产源码不再包含`0x00476140`整函数token。

## 4. 测试与验证

纯函数测试覆盖首次actor访问stop的EAX/ECX/EDX、主bit优先、fallback bit12、组B profile唯一owner、无关位返回0，以及正常ECX/EDX保持。frame测试覆盖主bit返回1后的side/自身索引发布、状态序列非1时actor stop保留callee EAX并阻断后缀，以及状态序列为1时保留清状态、latch、首次actor索引和文字token陈旧EAX。所有caller测试同时验证旧opaque调用次数为零。

最终`./build-asan.sh`、`./build.sh core`和`./build.sh app`分别完成AddressSanitizer core `188/188`、Linux core `188/188`与Linux app `194/194`；零OpenSWD3源码warning、sanitizer finding或测试失败。inventory生成器连续双跑逐字节一致，关闭进度为`244/422 = 235 platform_adapted + 9 assembly_exact + 178 pending_audit`，SHA256为`d83e15d36d75d20d8e0f2c64e897df347e511b7a820fd48baac6a1863fd4dc6e`。

当前缺少原版八个组B完整actor、动态40-byte profile、状态序列/特殊行动/文字callee及唯一caller寄存器联合捕获后端，因此`original_diff_verified`登记为`blocked_runtime_oracle`。
