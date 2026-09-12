# 战斗角色启动门word查询（0x004786D0）

## 1. 范围与边界

权威LST把`sub_4786D0`锁定为`0x004786D0..0x004786D7`共8字节：

```text
004786D0  66 8B 81 74 2A 00 00  mov ax,[ecx+0x2A74]
004786D7  C3                    retn
```

函数只有2条实际指令、0个call、0个分支、0个局部标签和1个普通RET；没有外部chunk、中段入口或隐藏异常尾。

## 2. 精确机器语义

入口ECX是角色对象token，EAX、EDX、ESP与flags由caller提供。唯一字段指令读取`actor+0x2A74`的完整16位word，只替换EAX低16位；EAX高16位、ECX、EDX与全部flags保持不变。随后普通RET读取`[ESP]`，把ESP增加4并跳到返回地址。

不得把结果零扩展、符号扩展、布尔化，也不得提前执行frame caller自己的`test ax,ax`或调试caller自己的`and eax,0xFFFF`。例如入口EAX为`0xA5A51234`、字段为`0xCDEF`时，leaf返回EAX必须为`0xA5A5CDEF`。

## 3. 字段访问与唯一owner

完整LST对`actor+0x2A74`共有9处直接访问：4处word比较、2处word读取、2处word写和1处word自增。除本getter外，相邻读写位于组A动作执行、特殊动作、组B动作执行、准备/重置目标及状态发布路径；这些函数继续由各自工作包审计，本工作包不提前关闭。

C++ owner按角色种类复用既有完整actor状态：

- Group-A：`LegacyBattleActionDispatchState::group_a_action_execution[index].start_gate`；
- Group-B：`LegacyBattleStartupState::group_b_lifecycle[index].action_execution.start_gate`。

Group-A frame、Group-B frame和调试叠加层均解析上述owner，没有新增平行actor数组、第二套word缓存或token槽。

## 4. typed实现与停止点

`query_legacy_battle_actor_start_gate()`逐条实现word读和普通RET，并记录字段token、字段读取、返回地址读取、栈读取、EAX/ECX/EDX、ESP/EIP及flags。

两个真实停止点为：

1. `start_gate_read_typed_stop`：字段不可读或actor owner无法解析时停在`0x004786D0`，完整保留入口EAX、ECX、EDX、ESP和flags；
2. `return_address_read_typed_stop`：字段读取成功后停在`0x004786D7`，保留已替换AX的EAX，不读取返回地址、不推进ESP。

正常路径只读取一个字段word和一个返回地址，ESP精确增加4。

## 5. 四处物理caller

### 5.1 Group-A frame（0x004566AD）

Group-A索引按原乘移序列形成`index*0x2F34`对象偏移与角色token；leaf入口EAX保持该偏移，EDX保持caller输入，flags来自`0x0045669A`最后一次`shl eax,2`，真实返回地址为`0x004566B2`。返回后caller才执行`test ax,ax`，并与两个抑制门及全局覆盖门共同形成效果模式，再调用效果模式发布。

C++直接读取Group-A action-execution的`start_gate`，只增加typed调用计数，不增加generic端口计数。typed-stop发生在任何效果模式发布和后续frame副作用之前。

### 5.2 Group-B frame（0x004581FE）

Group-B索引按原算术形成`index*0x565`中间值，再以八倍比例形成`index*0x2B28`对象步长；leaf入口EAX保持`index*0x565`，EDX保持caller值，flags来自`0x004581EA`的`index*24-index`减法，后续LEA不修改flags，真实返回地址为`0x00458203`。返回后caller才执行`test ax,ax`并形成公共效果模式。

C++直接读取Group-B startup lifecycle action-execution的`start_gate`。typed-stop保留此前已完成的完整frame前缀，但阻断效果模式发布和最终公共尾。

### 5.3 调试叠加层Group-B行（0x0045DF67）

相邻动作种类getter返回后，caller先执行`and eax,0xFFFF`，再压入已mask的命令参数并调用本getter。leaf入口EAX是零扩展命令word，EDX是当前解析角色的零扩展level word，flags来自该AND，真实返回地址为`0x0045DF6C`。返回后caller才再次mask AX，并把启动门、命令、生命和level组合为当前行文字。

### 5.4 调试叠加层Group-A行（0x0045DFE2）

相邻动作种类getter返回后同样先mask命令并压栈。leaf入口EAX是零扩展命令word；首行EDX保留此前最后一次Group-B文字绘制或字体reset的callee残值，后续行EDX保留上一行文字绘制callee残值；flags来自命令AND；真实返回地址为`0x0045DFE7`。返回后caller才mask启动门并格式化当前行。

两类调试typed-stop均保留已完成的resolve、生命、level或动作种类前缀以及已压入命令参数，抑制当前行格式化、绘制、剩余角色循环和公共调试后缀。调试端口中原启动门查询ordinal保留为`reserved_query_actor_start_gate`，但生产路径不再调用。

## 6. 双向追溯与验证范围

LST到C++追溯覆盖唯一word读、EAX部分寄存器写、普通RET及四个caller的真实返回地址；C++到LST反向追溯覆盖owner解析、字段/RET两个停点、frame后续TEST和调试后续mask位置。

测试覆盖Group-A/Group-B owner别名、零/一/其它word、EAX高word保留、ECX/EDX/flags保持、普通RET栈、字段/RET停止、两类frame效果模式、两类调试行、首行/后续行寄存器线程、停止前缀/后缀抑制，以及生产`0x004786D0` raw调用归零。

## 7. 动态差分状态

当前缺少原版完整Group-A/Group-B actor、异常字段/栈内存页，以及四处caller的寄存器、flags与SEH联合捕获后端，原版动态差分登记为`blocked_runtime_oracle`。该阻塞不改变8字节leaf和四处静态caller的LST闭环。
