# 战斗脚本曲线坐标采样 `0x0046E290`

状态：`platform_adapted`。完整LST、typed曲线采样、唯一case39 caller、测试、完整门和inventory双生成均已收敛。

## 1. 完整权威范围与ABI

权威LST主体为`0x0046E290..0x0046E385`，从proc到endp共105行、69条实际指令、5个call、0个跳转和1个返回点，没有外部`FUNCTION CHUNK`。函数是cdecl七参数：float帧值、两个i32输出指针，以及四个连续i16 X/Y控制点指针。

函数按控制点顺序把8个i16分别符号扩展并经x87转为float。先以参数0调用一次已关闭曲线矩阵协调，结果写入死局部；随后把入口frame与double常量`0.05`相乘并舍入为float，生成四项三次B样条基值，再与四个X/Y点相乘求和。

冻结矩阵的四列对应：

```text
[-1/6,  1/2, -1/2, 1/6]
[ 1/2,   -1,    0, 2/3]
[-1/2,  1/2,  1/2, 1/6]
[ 1/6,    0,    0,   0]
```

两个float输出依次调用`0x00489654`，按x87向零`fistp qword`后只取低32位写入输出dword。函数返回最后一次Y转换的完整低dword；正常尾ECX为X输出指针，EDX为Y输出指针。

## 2. typed实现

`sample_legacy_battle_script_curve`冻结权威float常量，按原三次、平方、线性和常量项顺序生成四项基值，再按原X/Y乘加次序输出。中间矩阵和点积以扩展精度计算、每个原float落点显式收窄；NaN、无穷或i64越界沿既有x87兼容规则输出integer-indefinite低dword零。

首个参数0曲线调用只写死局部且没有外部副作用，typed实现不保留该不可观察临时结果。四控制点仍按i16符号域读取，frame来自caller当前u16帧号的i16符号扩展。

## 3. 唯一caller回收

唯一caller是脚本case39六段路径。每个活动帧按当前段选择四个连续控制点，以帧号调用typed曲线采样；结果同时写回原`value_a/value_b` owner和角色坐标工作区。随后根据actor组别调用同一坐标写服务，再递增帧号，超过20时推进段号并清帧号，最后重建角色指标和完整战斗帧。

caller在曲线返回后把ECX/EDX恢复为两个物理输出owner的`compat::u32` token，后续只消费输出低word；token不转换为主机指针。旧`script_finalize`枚举槽保持原数值并改为reserved，生产零调用。

## 4. 验证状态

纯函数测试以四点`(0,0),(0,0),(60,-60),(60,-60)`验证帧0/10/20分别得到`(10,-10)/(30,-30)/(50,-50)`，并验证返回值是Y转换低dword。case39回归从live actor坐标构造路径，确认第一帧直接写typed曲线结果、旧reserved槽零调用且坐标服务调用一次。定向测试、AddressSanitizer、Linux core `188/188`和Linux app `194/194`全部通过，源码零warning；app只有既有ALSA白名单提示。

inventory生成器连续双跑逐字节一致，正式计数为`167/422 = 158 platform_adapted + 9 assembly_exact + 255 pending_audit`，SHA256为`479882f5b376d12d09248f571e0ae3127d3e016f27cef80530fd3f18da50f24a`。原版x87扩展精度、动态局部栈、两个输出指针token和caller联合寄存器缺少捕获后端，`original_diff_verified`登记为`blocked_runtime_oracle`。
