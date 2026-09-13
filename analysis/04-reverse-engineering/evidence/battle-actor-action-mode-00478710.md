# 战斗角色动作与显示模式设置（0x00478710）

## 1. 范围与边界

权威LST把`sub_478710`锁定为`0x00478710..0x0047876D`共94字节，最后一条指令从`0x0047876B`开始，下一函数前的对齐从`0x0047876E`开始。函数没有callee、外部chunk、中段入口或隐藏异常尾；控制流包含六次入口CMP、六个条件跳转、一次额外CMP、一个条件跳转、一个无条件跳转和两个`retn 4`出口。

关键指令为：

```text
00478710  mov eax,[esp+4]
00478714  cmp eax,2
00478719  cmp eax,0Dh
0047871E  cmp eax,3
00478723  cmp eax,0Eh
00478728  cmp eax,6
0047872D  cmp eax,0Fh
00478732  mov [ecx+2A6Ch],ax
00478739  mov eax,1
0047873E  retn 4
00478741  cmp eax,6
00478746  or byte ptr [ecx+2A87h],8
0047874F  or byte ptr [ecx+2A87h],40h
00478756  mov [ecx+2A70h],ax
0047875D  mov word ptr [ecx+2A6Ch],0
00478766  mov eax,1
0047876B  retn 4
```

## 2. 精确机器语义

入口ECX是角色对象token；参数是`[ESP+4]`中的完整32位dword。六次入口比较严格按`2、13、3、14、6、15`顺序执行，不得先截断为word。

- 默认路径：参数不等于上述六值时，把参数低16位写入`actor+0x2A6C`，不改`actor+0x2A70`和`actor+0x2A87`。
- 模式`2、13、3`：把参数低16位写入`actor+0x2A70`，再把`actor+0x2A6C`清零；不访问模式byte。
- 模式`14、15`：先执行`cmp eax,6`，再对`actor+0x2A87`原值执行OR `0x40`，随后写显示word并清动作word。
- 模式`6`：直接对`actor+0x2A87`原值执行OR `0x08`，随后写显示word并清动作word。

所有成功分支都在RET前执行`mov eax,1`。ECX和EDX不变。两个出口均为`retn 4`：先读取`[ESP]`返回地址，再同时弹出返回地址和一个dword参数，因此成功后ESP增加8。

flags严格保留最后一条实际改flags指令：

- 默认路径来自`cmp parameter,15`；
- `2、13、3`分别来自命中的对应CMP；
- `14、15、6`成功路径来自对应OR，CF/OF清零，AF未定义；
- `14、15`在OR读写发生fault时，flags仍来自额外的`cmp eax,6`，而不是入口命中CMP或未完成的OR。

## 3. 字段与canonical owner

本函数访问三个既有actor字段：

- `actor+0x2A6C`：动作种类word；
- `actor+0x2A70`：显示种类word；
- `actor+0x2A87`：模式flags byte。

C++复用已有canonical owner：

- Group-A动作word：`LegacyBattleActionDispatchState::group_a_action_execution[index].action_kind`；
- Group-A显示word与模式byte：`LegacyBattleStartupState::party[index].item_effect_application`；
- Group-B三个字段：`LegacyBattleStartupState::group_b_lifecycle[index].action_composition`。

这与前序动作种类、显示种类及startup/lifecycle恢复使用同一owner。没有新增平行actor数组、第二套模式状态或token槽。SDL脚本入口把既有startup owner与同一脚本会话动作分派owner共同绑定；重置时两者同时回到默认状态。

## 4. typed实现与停止点

`set_legacy_battle_actor_action_mode()`按指令顺序记录参数栈读取、actor字段访问、返回地址栈读取、EAX/ECX/EDX、ESP/EIP和flags。真实停止点为：

1. `argument_read_typed_stop`：停在`0x00478710`，完整保留入口寄存器、ESP和flags；
2. `mode_flags_read_typed_stop`：停在`0x00478746`或`0x0047874F`，未提交OR；
3. `mode_flags_write_typed_stop`：同样停在OR指令，保留已读取前缀但不提交OR；
4. `display_kind_write_typed_stop`：停在`0x00478756`，保留已完成的模式byte OR；
5. `action_kind_write_typed_stop`：默认路径停在`0x00478732`；特殊路径停在`0x0047875D`并保留显示word及可能的模式byte写入；
6. `return_address_read_typed_stop`：停在`0x0047873E`或`0x0047876B`，保留全部actor写入和`EAX=1`，但不读取返回地址、不推进ESP。

参数读取发生在任何actor访问前。特殊路径的访问顺序固定为模式byte读、模式byte写、显示word写、动作word写；模式`2、13、3`跳过前两项。RET读取始终最后发生。

## 5. 十一个caller函数与四十一处物理CALL

机器码逐条扫描得到11个caller函数、41条真实`call sub_478710`指令，分布为`11+2+3+3+1+1+7+1+2+2+8`。共享C++组合helper只消除重复样板，不合并物理CALL身份；每次调用保留独立返回地址、入口EAX/ECX/EDX、flags来源和typed-stop后缀。

### 5.1 主动作分派十一处

`sub_4539B0`的真实返回地址为：

```text
0x00453B15  0x00453C93  0x00453F8B
0x00455053  0x004551C8  0x0045550C
0x004558F9  0x00455951  0x00455AE8
0x00455C1B  0x00455CBB
```

这些调用覆盖framebuffer清零后的模式300、扫描结束后的模式300、动作完成后的模式0，以及条件特殊动作分支。实现线程化真实前序callee回复或地址算术结果，并保留REP清零前的AND、XOR、CMP、SUB、ADD或callee flags。任一typed-stop只保留当前CALL前缀，阻断返回后的扫描、订单移除、清理、退却提交或公共尾。

### 5.2 对手动作分派两处

`sub_455D60`的返回地址为`0x00456080`和`0x0045652B`，分别设置模式300和模式0。两处都直接组合Group-A canonical owner；后者保留动作完成查询返回值与`xor ebp,ebp` flags。生产端口不再接收`0x00478710`。

### 5.3 Group-A frame三处

`sub_456680`的返回地址为`0x00456BA6`、`0x00456C8E`和`0x00456E47`。模式来自既有delay/action状态，入口寄存器分别保留索引、模式值、选择完成回复或presentation回复；flags保留对应actor状态比较或callee结果。typed-stop阻断当前返回值检查、后续动作分派、清理与公共frame尾。

### 5.4 Group-B frame三处

`sub_4576A0`的返回地址为`0x00457C68`、`0x00457CA0`和`0x00457E34`，分别覆盖状态位触发的模式2、模式6和选择发布后的模式17。三个字段统一落到Group-B lifecycle的`action_composition`。连续模式2再模式6时最终为`mode_flags=0x88`、`action_kind=0`、`display_kind=6`，并保持两条CALL的独立结果顺序。

### 5.5 动作后处理与消息阶段

`sub_45ADF0`在`0x0045AEE7`调用并返回到`0x0045AEEC`，使用目标reset回复设置模式0；typed-stop阻断availability查询及后续suffix。

`sub_466F70`在`0x004671F4`调用并返回到`0x004671F9`。同一物理CALL可在角色循环中多次执行；每次以当前Group-A索引形成EAX/EDX地址算术结果，并逐次保留相同物理返回地址。调用成功后caller才发布消息阶段状态。

### 5.6 战斗脚本分派七处

`sub_469D20`的真实返回地址为：

```text
0x0046B550  0x0046B5E5  0x0046B6B7  0x0046B75F
0x0046B87E  0x0046B900  0x0046DCC9
```

七处按Group-A/Group-B token规则绑定同一startup/action owner，覆盖脚本动作模式11、12和6。入口地址算术及SUB flags不被C++语句合流抹除。旧枚举槽改名为`reserved_actor_action_mode`，地址与ordinal保留，但生产路径零调用。RET typed-stop阻断脚本workspace清理、frame gate、目标发布和游标推进等caller suffix。

### 5.7 三个Group-B动作组合helper共五处

`sub_476160`在`0x004761B5`调用并返回到`0x004761BA`；成功后caller才OR模式byte `0x80`。

`sub_4761D0`的返回地址为`0x004761F4`和`0x00476246`，分别覆盖预载profile的模式2与MON profile加载后的模式1。前者保留调用前OR `0x80`的flags，后者保留derived word ADD flags。

`sub_476250`的返回地址为`0x004762C3`和`0x004762DA`，分别覆盖profile bit触发的模式2与普通模式1。三个helper都接收上层fault模板，把实际CALL结果回卷到frame、动作分派或脚本分派的外层结果；typed-stop不会被helper边界吞掉。

### 5.8 actor-ready内部八处

`sub_480220`内部八个真实返回地址为：

```text
0x004803FA  0x0048055A  0x0048058C  0x0048066B
0x00480691  0x004809F9  0x00480A16  0x00480A53
```

当前`0x00480220`仍是待审窄边界，因此其reply显式发布最多八个已执行CALL的模式、入口寄存器和flags；Group-A/Group-B frame只对`executed`项逐个组合typed leaf，并按固定槽恢复真实返回地址。实现不伪造未执行分支，也不把八条CALL折叠为一个虚拟调用。任一typed-stop阻断actor-ready返回后的当前frame后缀。

## 6. 双向追溯与验证范围

LST到C++追溯覆盖94字节完整边界、六次入口CMP、额外`cmp eax,6`、两个OR、两个word写、两个`mov eax,1`、两个`retn 4`、三个字段owner以及41条机器码CALL。C++到LST反向追溯覆盖参数和RET栈、字段访问顺序、部分word写、mode byte RMW、成功与fault flags、11个caller函数、41个返回地址、嵌套helper结果回卷及caller后缀抑制。

测试覆盖Group-A/Group-B owner别名、32位比较与word截断、默认值、六个特殊值、模式byte旧bit保留、两种RET出口、EAX/ECX/EDX、ESP、stack token、最终flags、全部字段/RET停止点、41个物理返回地址、直接caller、嵌套helper、actor-ready八槽、生产raw调用归零及关键caller后缀。

## 7. 动态差分状态

当前缺少原版完整Group-A/Group-B actor、异常参数/字段/栈内存页，以及41处caller的联合寄存器、flags与SEH捕获后端，原版动态差分登记为`blocked_runtime_oracle`。该阻塞不改变94字节leaf和41处静态caller的LST闭环。
