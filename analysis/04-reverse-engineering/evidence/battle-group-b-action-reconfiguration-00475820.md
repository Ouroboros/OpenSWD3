# 战斗组B行动资源重配置 `0x00475820`

状态：`platform_adapted`。完整LST、typed状态、唯一caller、定向/ASan/Linux验证和inventory双生成收敛后关闭。

## 1. 完整权威范围与ABI

权威LST主体为`0x00475820..0x00475868`，从proc到endp共35行、24条实际指令、3个call、0个跳转、0个局部标签和1个返回点，没有外部`FUNCTION CHUNK`。函数是thiscall，入口ECX为组B actor，唯一栈参数为资源定义值，callee以`retn 4`弹出参数。唯一直接caller是战斗脚本分派case 80的`0x0046DDD1`。

入口先把参数载入EAX、保存ESI并令ESI等于actor，再把定义参数压栈；首次actor访问是读取`actor+0x0C`资源token。actor缺失时只能在该读取点停止，保留EAX定义参数、ECX actor token、入口EDX以及此前栈副作用。

## 2. 资源加载与首次访问

资源加载callee `0x00476DB0`的完整边界为：

- 两个栈参数依次形成资源token与定义参数；
- EAX为定义参数；
- ECX为资源token；
- EDX保持caller进入本函数时的陈旧值。

callee返回后重新读取`actor+0x0C`，首次真实资源访问是`movsx edx, word ptr [resource+0x64]`。资源token为零时严格停在该word读取点：资源加载callee及其副作用已经发生，EAX为零，ECX/EDX保留callee返回值，后续资源写、mode发布、profile与释放均不执行。

资源可用时，`resource+0x64`按i16符号扩展为i32并完整写到`resource+0x4C`。本函数不写actor`+0x26B4`；modern typed owner只更新164-byte资源块中的`+0x4C`，不得与上一函数的actor时间字段联动。随后读取`resource+0x90`低byte写actor`+0x2A93`。

## 3. profile与释放ABI

资源mode通过`mov cl`只覆盖资源加载callee返回ECX的低8位，高24位保持陈旧。资源`+0x60`通过`mov dx`只覆盖符号扩展EDX的低16位，高16位保持`resource+0x64`符号扩展结果。随后以actor`+0x0D90`进入`0x00476A80`：

- 两个栈参数为actor`+0x0D90`和组合后的EDX；
- EAX为actor`+0x0D90`；
- ECX为资源加载callee返回ECX高24位与资源mode低8位组合；
- EDX为符号扩展结果高16位与资源`+0x60`低16位组合。

profile callee正常返回后重新读取资源token，以该token覆盖ECX并调用`0x00478220`；EAX和EDX继承profile返回值，唯一栈参数为资源token。正常终端EAX/ECX/EDX完整继承释放callee。profile与释放typed-stop分别停在各自call边界，保留此前资源`+0x4C`、actor mode、profile发布和寄存器snapshot。

## 4. 唯一caller与typed owner

脚本case 80在`0x0046DD9F..0x0046DDED`先把脚本`+2` actor写入共享高word，再把脚本`+4`按i16符号扩展写入共享定义值。原乘法链把actor索引转换为`0x00525508 + actor*0x2B28`，并在调用前留下EDX=`actor*345`。typed caller按相同值调用本函数，复用`LegacyBattleStartupState::group_b_lifecycle`惰性八槽owner；actor token、资源token、164-byte资源块、profile和mode均不建立第二份物理状态。

顶层`0x00475820` opaque调用已删除，枚举数值仅保留reserved槽。三个未审内部callee分别以`0x00476DB0`、`0x00476A80`和`0x00478220`窄调用记录保留，脚本port仍能观察参数与寄存器。完成后caller把脚本cursor加6、EAX固定为1，并以原prologue保存值恢复ECX；EDX保留释放callee终值。任一callee typed-stop均阻断cursor推进和caller epilogue恢复，状态登记为closed-callee stop。

## 5. 验证与动态阻塞

纯函数回归覆盖actor与资源首次访问stop、资源加载/profile/释放三个callee stop、i16符号扩展、资源`+0x4C`写入、actor`+0x26B4`保持、mode低byte发布、陈旧CL/DX组合、profile与释放ABI及普通终端寄存器。脚本回归覆盖actor乘法地址、入口EDX、三个callee trace、共享八槽owner、cursor加6、正常ECX恢复及callee stop阻断后缀。验证为定向CTest 1/1、完整core AddressSanitizer 188/188、Linux core 188/188和Linux app 194/194全部通过；app只保留SDL上游缺少ALSA开发库的既有环境提示，无OpenSWD3源码warning或sanitizer finding。

原版八个组B完整对象、动态资源字节、profile buffer、三个callee副作用、脚本case 80共享全局及caller寄存器缺少联合捕获后端，因此`original_diff_verified`登记为`blocked_runtime_oracle`。
