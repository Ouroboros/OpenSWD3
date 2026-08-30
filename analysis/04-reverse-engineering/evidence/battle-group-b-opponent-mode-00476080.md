# 战斗组B对手模式选择 `0x00476080`

状态：`platform_adapted`。完整LST、RNG边界补证、唯一owner、caller回收、分支与故障顺序测试、ASan/Linux门禁和inventory双生成收敛后关闭。

## 1. 完整权威范围与ABI

权威LST主体为`0x00476080..0x00476138`，从proc到endp共102行、73条实际指令、1个call、14个跳转、5个局部标签和3个返回点，没有外部`FUNCTION CHUNK`。函数是thiscall：ECX为组B行动者，无栈参数，以普通`retn`返回。唯一callee为`0x00439070`，固定参数10；唯一caller是组B逐帧函数`0x004576A0`的`0x004579C1`。

唯一caller先选出一个组A目标并确认其非终止，再以当前组B行动者调用本函数。返回1时caller把行动side写1并把目标改为source自身；返回0时保留先前组A目标。typed-stop发生时阻断这两项发布及`selection_initialized`后缀。

## 2. 随机分类与寄存器

入口无条件调用`0x00439070(10)`。其完整LST补证表明正常返回时EAX与EDX都为随机余数，取值域为0至9，ECX为secondary RNG游标。函数把完整EAX复制到ESI，但分类只比较SI：

- SI无符号不大于4时，把完整ESI清零；
- SI在5至9时，把完整ESI写1；
- 其他低word保留完整随机值，后续既不命中模式0也不命中模式1。

后者虽超出已审RNG正常值域，仍由测试保留其机器分支，不按现代接口假定删除。modern端直接调用集中`LegacyBattleBoundedRandomPort`，不重新引入整函数或RNG opaque token。

## 3. 阈值与资源访问

随机之后依次读取actor `+0x0C`资源token、actor `+0x26B4`完整dword，再首次访问资源`+0x64`。modern actor映射缺失在随机之后typed-stop；资源token为零则在`+0x64`首次访问点typed-stop，并保留随机消耗、EAX timing、ECX零及EDX随机值。

资源有效时先把`+0x64`按i16符号扩展为默认阈值；actor `+0x26B4`非零时以该完整dword的i32解释覆盖阈值。资源`+0x4C`按i32除3并向零截断。只有阈值严格小于该商才继续；相等或更大均返回0。正常出口保留ECX资源token与EDX商。

## 4. 两种模式与门顺序

随机分类为0时，按固定顺序要求：

1. 资源`+0x7C`低word非零；
2. actor `+0x26D1`低bit为零，即typed `retreat_ready_flags & 0x0100`为零；
3. 资源`+0x8E`低byte非零。

全部满足才把actor `+0x2A9C`低byte写1并返回1。随机分类为1时改查资源`+0x80`低word，后两项门相同；全部满足时写2并返回1。任一门失败、阈值拒绝或分类不是0/1都返回0，且保留原`+0x2A9C`内容。

`+0x26B4`继续复用行动配置的`timing_value`，资源字段继续复用组B生命周期164-byte资源owner，`+0x26D1`复用执行状态的`retreat_ready_flags`高byte。新增`opponent_mode`位于同一`LegacyBattleGroupAActionExecutionState`唯一owner，不建立frame或dispatch平行副本。

## 5. caller回收与验证

组Bframe唯一caller改为`select_legacy_battle_group_b_opponent_mode` typed直连。caller在映射组B lifecycle后仍由callee先消费一次bound 10随机；结果1保持原side与self-target发布，结果0保持组A目标，actor/resource typed-stop保留随机及此前目标选择副作用并阻断全部caller后缀。生产源码不再包含`0x00476080`整函数token。

纯函数测试覆盖actor首次访问stop、资源首次访问stop、低word0至4与5至9分类、超域低word完整保留、signed负值除3、完整timing override、相等拒绝、`+0x7C/+0x80`双资源门、`+0x26D1`门、`+0x8E`门、原mode保留、两种成功写入及EAX/ECX/EDX结果。frame测试覆盖成功side/self-target发布、旧opaque零调用，以及资源stop阻断`selection_initialized`后缀。

最终`./build-asan.sh`、`./build.sh core`和`./build.sh app`分别完成AddressSanitizer core `188/188`、Linux core `188/188`与Linux app `194/194`；零OpenSWD3源码warning、sanitizer finding或测试失败。inventory生成器连续双跑逐字节一致，关闭进度为`243/422 = 234 platform_adapted + 9 assembly_exact + 179 pending_audit`，SHA256为`dce5ffe3646bc8cfcbc89c98cf946df9d3345eb9e36d3b8a3ea1ed03bb32f372`。

当前缺少原版八个组B完整actor、动态164-byte资源、secondary RNG游标、唯一caller局部与寄存器联合捕获后端，因此`original_diff_verified`登记为`blocked_runtime_oracle`。
