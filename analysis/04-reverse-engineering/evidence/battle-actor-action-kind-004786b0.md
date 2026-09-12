# 战斗角色动作种类word查询（0x004786B0）

## 1. 范围与边界

权威LST把`sub_4786B0`锁定为`0x004786B0..0x004786B7`共8字节：

```text
004786B0  66 8B 81 6C 2A 00 00  mov ax,[ecx+0x2A6C]
004786B7  C3                    retn
```

函数只有2条实际指令、0个call、0个分支、0个局部标签和1个普通RET；没有外部chunk、中段入口或隐藏异常尾。

## 2. 精确机器语义

入口ECX是角色对象token，EAX、EDX、ESP与flags由caller提供。唯一字段指令读取`actor+0x2A6C`的完整16位word，只替换EAX低16位；EAX高16位、ECX、EDX与全部flags保持不变。随后普通RET读取`[ESP]`，把ESP增加4并跳到返回地址。

不得把结果零扩展、符号扩展、布尔化，也不得提前执行caller自己的`and eax,0xFFFF`。例如入口EAX为`0xA5A51234`、字段为`0xCDEF`时，leaf返回EAX必须为`0xA5A5CDEF`。

## 3. 字段访问与唯一owner

完整LST对`actor+0x2A6C`共有47处直接访问：27处word写、8处寄存器读取和12处word比较。初始化器`0x004782CD`写零；动作执行、AI、目标和模式路径继续在其各自后续工作包中写或比较同一word，本工作包不提前关闭这些相邻函数。

C++ owner按角色种类复用既有完整actor状态：

- Group-A：`LegacyBattleActionDispatchState::group_a_action_execution[index].action_kind`；
- Group-B：`LegacyBattleStartupState::group_b_lifecycle[index].action_composition.action_kind`。

`LegacyBattleActorBaseInitializationOwner`的Group-A公共初始化也直接写`action_execution.action_kind`，不再保留并列word成员。Group-B公共初始化继续把`action_composition.action_kind`引用传入同一初始化器。动作分派、对手分派和调试叠加层均只解析上述owner，没有新增平行actor数组、第二套word缓存或token槽。

## 4. typed实现与停止点

`query_legacy_battle_actor_action_kind()`逐条实现word读和普通RET，并记录字段token、字段读取、返回地址读取、栈读取、EAX/ECX/EDX、ESP/EIP及flags。

两个真实停止点为：

1. `action_kind_read_typed_stop`：字段不可读或actor owner无法解析时停在`0x004786B0`，完整保留入口EAX、ECX、EDX、ESP和flags；
2. `return_address_read_typed_stop`：字段读取成功后停在`0x004786B7`，保留已替换AX的EAX，不读取返回地址、不推进ESP。

正常路径只读取一个字段word和一个返回地址，ESP精确增加4。

## 5. 四处物理caller

### 5.1 主动作分派（0x004539E6）

Group-A索引按原乘移序列形成角色token；leaf入口EAX为`index*0xBCD`，flags来自最后一次`index*0x3F0-index`的SUB，EDX保持caller输入，真实返回地址为`0x004539EB`。返回后才执行`mov di,ax`和`and edi,0xFFFF`，随后查询角色终止状态并进入主动作/fallback分派。typed-stop阻断终止查询与全部动作switch后缀。

### 5.2 对手动作分派（0x00455D97）

Group-B索引按原步长计算角色token；leaf入口EAX为`index*0x565`，flags来自最后一次`index*24-index`的SUB，EDX保持caller输入，真实返回地址为`0x00455D9C`。返回后caller才执行`and eax,0xFFFF`并按动作1..17、100、200、300及default分派。typed-stop阻断switch及全部动作副作用。

### 5.3 调试叠加层Group-B行（0x0045DF5A）

当前Group-B对象先解析profile对象并读取其`+0x54`level word；`xor edx,edx`后只把DX替换为该word，并在调用前压入栈作为相邻getter的后续参数。leaf入口EAX保留resolved profile token，EDX为零扩展level，flags为XOR零结果，真实返回地址为`0x0045DF5F`。返回后才零扩展AX并调用相邻lock getter；typed-stop保留resolve、vitality与level读取前缀，抑制当前行lock、格式化、绘制和剩余循环。

### 5.4 调试叠加层Group-A行（0x0045DFD5）

首个Group-A行的leaf入口EAX保留前一Group-B count读取值，EDX保留此前最后一次文字绘制或字体reset的callee残值，flags来自Group-A count的TEST；第二行起EAX来自上轮末尾重载的Group-A count、EDX来自上一行文字绘制callee、flags来自`CMP completed_rows,count`。真实返回地址为`0x0045DFDA`。返回后才零扩展AX并调用相邻lock getter。typed-stop抑制当前行与剩余Group-A及公共叠加层后缀。

调试端口中原动作查询ordinal保留为`reserved_query_actor_command`，但生产路径不再调用。

## 6. 双向追溯与验证范围

LST到C++追溯覆盖唯一word读、EAX部分寄存器写、普通RET及四个caller的真实返回地址；C++到LST反向追溯覆盖owner解析、字段/RET两个停点和caller后续零扩展位置。

测试覆盖Group-A/Group-B owner别名、零/一/其它word、EAX高word保留、ECX/EDX/flags保持、普通RET栈、字段/RET停止、主/对手switch、两类调试行格式化前缀与后缀抑制，以及生产`0x004786B0` raw调用归零。

## 7. 动态差分状态

当前缺少原版完整Group-A/Group-B actor、异常字段/栈内存页，以及四处caller的寄存器、flags与SEH联合捕获后端，原版动态差分登记为`blocked_runtime_oracle`。该阻塞不改变8字节leaf和四处静态caller的LST闭环。
