# 战斗角色显示种类word查询（0x004786C0）

## 1. 范围与边界

权威LST把`sub_4786C0`锁定为`0x004786C0..0x004786C7`共8字节：

```text
004786C0  66 8B 81 70 2A 00 00  mov ax,[ecx+0x2A70]
004786C7  C3                    retn
```

函数只有2条实际指令、0个call、0个分支、0个局部标签和1个普通RET；没有外部chunk、中段入口或隐藏异常尾。

## 2. 精确机器语义

入口ECX是角色对象token，EAX、EDX、ESP与flags由caller提供。唯一字段指令读取`actor+0x2A70`的完整16位word，只替换EAX低16位；EAX高16位、ECX、EDX与全部flags保持不变。随后普通RET读取`[ESP]`，把ESP增加4并跳到返回地址。

不得把结果零扩展、符号扩展、布尔化，也不得提前执行caller自己的`and eax,0xFFFF`。例如入口EAX为`0xA5A51234`、字段为`0xCDEF`时，leaf返回EAX必须为`0xA5A5CDEF`。

## 3. 字段访问与唯一owner

完整LST对`actor+0x2A70`共有13处直接访问：8处word写和5处word读。当前leaf是其中唯一独立getter；其余写入或读取位于Group-A效果应用、最终处理、actor初始化及后续角色流程，本工作包不提前关闭相邻函数。

C++ owner按角色种类直接复用既有startup actor状态：

- Group-A：`LegacyBattleStartupState::party[index].item_effect_application.display_kind`；
- Group-B：`LegacyBattleStartupState::group_b_lifecycle[index].action_composition.display_kind`。

resolver只返回上述owner中的word地址。动作分派状态、结果和caller局部没有新增平行word数组、第二套display缓存或token槽。

## 4. typed实现与停止点

`query_legacy_battle_actor_display_kind()`逐条实现word读和普通RET，并记录字段token、字段读取、返回地址读取、栈读取、EAX/ECX/EDX、ESP/EIP及flags。

两个真实停止点为：

1. `display_kind_read_typed_stop`：字段不可读或startup actor owner无法解析时停在`0x004786C0`，完整保留入口EAX、ECX、EDX、ESP和flags；
2. `return_address_read_typed_stop`：字段读取成功后停在`0x004786C7`，保留已替换AX的EAX，不读取返回地址、不推进ESP。

正常路径只读取一个字段word和一个返回地址，ESP精确增加4。

## 5. 唯一物理caller（0x00453A0E）

主动作分派先通过`0x004786B0`取得Group-A actor动作种类并只保留低word，再调用`0x0047CE80`查询角色终止状态。完整返回EAX等于1时立即离开；只有终止查询不等于1且动作种类为零时，才执行`test edi,edi`、把同一Group-A actor token写入ECX并在`0x00453A0E`调用本leaf。真实返回地址为`0x00453A13`。

leaf入口EAX与EDX沿用`0x0047CE80`的真实回复，ECX为actor token，flags来自零值`test edi,edi`。返回后caller才执行`and eax,0xFFFF`并把结果写入局部动作槽；低word为零时返回1，非零时进入后续动作switch。字段或RET typed-stop保留已完成的动作种类查询和终止查询副作用，阻断局部写入、switch与全部动作后缀。

生产路径直接组合typed leaf，不再通过generic端口调用`0x004786C0`。

## 6. 双向追溯与验证范围

LST到C++追溯覆盖唯一word读、EAX部分寄存器写、普通RET、唯一caller的前置条件、寄存器与flags线程、真实返回地址和post-call mask；C++到LST反向追溯覆盖两类owner解析、字段/RET两个停点和caller后缀抑制。

测试覆盖Group-A/Group-B owner别名、零/一/其它word、EAX高word保留、ECX/EDX/flags保持、普通RET栈、字段/RET停止、真实返回地址、前序callee寄存器线程、零值早退、非零switch及生产`0x004786C0` raw调用归零。

## 7. 动态差分状态

当前缺少原版完整Group-A/Group-B actor、异常字段/栈内存页，以及唯一caller的寄存器、flags与SEH联合捕获后端，原版动态差分登记为`blocked_runtime_oracle`。该阻塞不改变8字节leaf和唯一静态caller的LST闭环。
