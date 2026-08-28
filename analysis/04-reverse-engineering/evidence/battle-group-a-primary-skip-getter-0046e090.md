# 组A角色主跳过状态读取 `0x0046E090`

状态：`assembly_exact`。完整LST、唯一地址引用、静态不可达证明、回归测试、完整门和inventory双生成均已收敛。

## 1. 完整权威范围

权威LST主体为`0x0046E090..0x0046E096`，从proc到endp共10行、2条实际指令、0个call、0个跳转、0个局部标签和1个返回点，没有外部`FUNCTION CHUNK`。

完整行为只有：

```text
mov eax, [ecx+0x2B00]
retn
```

入口ECX是组A角色对象token对应的this。函数按完整32-bit读取对象`+0x2B00`并原样返回EAX；不掩码、不符号扩展、不修改ECX/EDX，也没有任何内存写入或callee副作用。

## 2. 物理状态owner

`+0x2B00`与相邻`+0x2B04`是既有组A角色结算跳过状态。胜利奖励、帧完成、帧输入、等级推进和成长结果均已按完整dword读取这两个物理槽；typed侧由现有角色结算状态数组承接，不为本叶子创建第二份存储。

## 3. 唯一地址引用与静态不可达证明

全程序没有`call 0x0046E090`。唯一引用是主脚本分派case58内的地址常量：

1. `0x0046D258`先把固定函数地址`0x0046E0A0`装入EDX。
2. `test edx,edx`后在`0x0046D264`执行`jnz 0x0046D2AD`。
3. 该PE代码地址静态非零，因此执行必定在此跳走。
4. 只有不可达的顺序后继才会在`0x0046D266`把本函数固定地址装入ECX并再次测试；其后的目标槽写入和攻击顺序插入同样不可达。

因此该函数只有DATA XREF，没有运行时caller。现代case58对组B actor保持直接推进4字节，不恢复这段静态死块；新增一个未被调用的现代getter反而会制造原程序没有的运行路径。

## 4. 验证状态

新增case58回归将两个脚本word设为组B路径，断言cursor只推进4字节、返回1、无窄端口调用且攻击顺序记录保持不变。定向测试、AddressSanitizer、Linux core `188/188`和Linux app `194/194`全部通过。

inventory生成器连续双跑逐字节一致，正式计数为`162/422 = 154 platform_adapted + 8 assembly_exact + 260 pending_audit`，SHA256为`3a3d2dbd8e5b2f3f13402d674cd43e2dc965b85e18d23acc4048c031f4bdad47`。本函数的唯一引用静态不可达，无需动态oracle；`original_diff_verified`由完整两指令LST与不可达控制流证明关闭。
