# 组A角色次跳过状态读取 `0x0046E0A0`

状态：`assembly_exact`。完整LST、唯一地址引用、caller寄存器消费、回归测试、完整门和inventory双生成均已收敛。

## 1. 完整权威范围

权威LST主体为`0x0046E0A0..0x0046E0A6`，从proc到endp共10行、2条实际指令、0个call、0个跳转、0个局部标签和1个返回点，没有外部`FUNCTION CHUNK`。

完整行为只有：

```text
mov eax, [ecx+0x2B04]
retn
```

入口ECX是组A角色对象token对应的this。函数按完整32-bit读取对象`+0x2B04`并原样返回EAX；不掩码、不符号扩展、不修改ECX/EDX，也没有内存写入或callee副作用。

## 2. 物理状态owner

`+0x2B04`与前一叶子的`+0x2B00`共同组成既有组A角色结算跳过状态。胜利奖励、帧完成、帧输入、等级推进、成长结果和脚本镜像路径均已使用该完整dword；typed侧继续复用现有角色结算状态，不建立第二份字段存储。

## 3. 唯一地址引用与静态不可达证明

全程序没有`call 0x0046E0A0`。唯一DATA XREF位于主脚本分派case58的组B路径：

1. `0x0046D258`把固定函数地址装入EDX。
2. `0x0046D25D`执行`test edx,edx`。
3. 固定PE代码地址静态非零，因此`0x0046D264`的`jnz 0x0046D2AD`必然跳转。
4. 该跳转越过前一叶子地址加载、目标槽写入和攻击顺序插入死块；本叶子自身从不被调用。

与前一叶子不同，本地址常量的加载和测试确实执行，因此原函数返回前EDX保持该固定地址。typed case58以`compat::u32` token恢复这个可观察寄存器值，但不把token转换为主机指针，也不恢复静态不可达调用块。

## 4. 验证状态

case58回归断言组B路径返回1、cursor推进4字节、EDX保持固定getter token、无窄端口调用且攻击顺序记录不变。定向测试、AddressSanitizer、Linux core `188/188`和Linux app `194/194`全部通过，源码零warning。

inventory生成器连续双跑逐字节一致，正式计数为`163/422 = 154 platform_adapted + 9 assembly_exact + 259 pending_audit`，SHA256为`af7a8f6f695cd1a78feaa4bb3ad950c6615bcdeb3a40edc7069faecd3340072f`。本函数不被调用，`original_diff_verified`由完整两指令LST、静态不可达控制流和caller EDX token回归证明关闭。
