# 战斗角色动作目标word查询（0x004786E0）

## 1. 范围与边界

权威LST把`sub_4786E0`锁定为`0x004786E0..0x004786E7`共8字节：

```text
004786E0  66 8B 81 A2 29 00 00  mov ax,[ecx+0x29A2]
004786E7  C3                    retn
```

函数只有2条实际指令、0个call、0个分支和1个普通RET；没有外部chunk、中段入口或隐藏异常尾。

## 2. 精确机器语义

入口ECX是角色对象token，EAX、EDX、ESP与flags由caller提供。唯一字段指令读取`actor+0x29A2`的完整16位word，只替换EAX低16位；EAX高16位、ECX、EDX和全部flags保持不变。随后普通RET读取`[ESP]`，把ESP增加4并跳到返回地址。

不得把字段提前零扩展、符号扩展、归一化或布尔化，也不得提前执行caller返回后的`movsx ax`、`cmp ax,0xFFFF`或word发布。例如入口EAX为`0xA5A51234`、字段为`0xCDEF`时，leaf返回EAX必须为`0xA5A5CDEF`。

## 3. 字段访问与canonical owner

完整LST对`actor+0x29A2`共有5处直接访问：基础初始化`0x00478272`写`0xFFFF`，本函数`0x004786E0`读取，`0x00478A7F`写AX，`0x00478B20`写`0xFFFF`，以及待审路径`0x0047D455`写`0xFFFF`。

C++按角色种类复用既有完整actor状态：

- Group-A：`LegacyBattleActionDispatchState::group_a_action_execution[index].action_target`；
- Group-B：`LegacyBattleStartupState::group_b_lifecycle[index].action_execution.action_target`；
- 调试特殊对象：`LegacyBattleDebugHotkeyState::special_actor_action_target.action_target`。

基础初始化直接把canonical action-execution字段写为`0xFFFF`，不再保留第二份`field_29a2`投影。普通默认构造仍保持零值。当前工作包涉及的`0x00478A70`和`0x00478B20`生产调用继续保留为待审generic边界，但其已知`actor+0x29A2`写效果同步到上述canonical owner；没有新增平行actor数组、第二套target缓存或token槽。

全局`word_4A7626`仍由`LegacyBattleActionDispatchState::selected_target_index`独立持有。该全局选择值与每个actor的`+0x29A2`字段不是同一状态，不得合并。

## 4. typed实现与停止点

`query_legacy_battle_actor_action_target()`逐条实现word读取和普通RET，并记录字段token、字段读取、返回地址读取、栈读取、EAX/ECX/EDX、ESP/EIP和flags。

两个真实停止点为：

1. `action_target_read_typed_stop`：字段不可读或actor owner无法解析时停在`0x004786E0`，完整保留入口EAX、ECX、EDX、ESP和flags；
2. `return_address_read_typed_stop`：字段读取成功后停在`0x004786E7`，保留已替换AX的EAX，不读取返回地址、不推进ESP。

正常路径只读取一个字段word和一个返回地址，ESP精确增加4。

## 5. 二十处物理caller

### 5.1 动作分派两处

`0x00454A3D`与`0x00454AE6`均读取固定Group-A首角色，真实返回地址分别为`0x00454A42`和`0x00454AEB`。两处leaf前最后一条改flags指令都是对side word与零的word比较，返回后caller才`movsx ecx,ax`、定位Group-B目标并执行准备、动态数量扫描、首个存活目标发布和公共场景后缀。

实现直接组合typed leaf。字段停止保留已完成的状态指示器与动作前缀，阻断目标准备、扫描、发布和场景后缀。`0x00478A70`发布首个存活目标时同步写Group-A首角色的canonical动作目标。

### 5.2 Group-A frame四处

四个真实返回地址为：

- `0x00456ECE`：空闲角色路径，返回后才`movsx ecx,ax`并查询目标角色回合完成；
- `0x004570D1`：动作启动callee之后，返回后才把AX符号扩展为动作分派目标参数；
- `0x004570F1`：嵌套动作分派返回1之后，返回后先保存完整EAX，再调用`0x00478B20`清动作；
- `0x004571ED`：目标terminal查询返回1之后，返回后才比较AX与`0xFFFF`并按非哨兵目标重置Group-A角色。

四处均复用当前Group-A action-execution owner。第二、三处保留动作启动和嵌套分派的真实EAX/EDX线程；第四处保留terminal回复及其与1比较后的flags。清动作和目标重建的generic写端同步canonical字段。任一停止只保留当前物理caller以前的前缀，阻断对应符号扩展、嵌套分派、清理、terminal后缀或公共尾。

### 5.3 Group-B frame两处

`0x00457E8A`的真实返回地址为`0x00457E8F`。active actor分支继承动作启动callee回复；inactive actor分支继承先前寄存器，并保留`index*24-index`的SUB flags。返回后caller才符号扩展AX并进入对手动作分派。

`0x00457EAE`的真实返回地址为`0x00457EB3`，入口EAX/EDX继承嵌套对手动作分派，flags来自其返回值与成功值1的比较。返回后caller才保存目标、调用`0x00478B20`并进入完成后缀。两处均读取startup Group-B lifecycle owner；清动作同步写canonical `0xFFFF`。

### 5.4 动作后处理一处

`0x0045AE4C`逐个读取Group-A角色，真实返回地址为`0x0045AE51`。入口EAX为当前扫描索引，ECX为角色token，EDX线程化前一reset或目标发布callee回复，flags来自扫描索引与排除索引的比较。返回后caller才`movsx esi,ax`、测试符号位、比较全局选择值，并执行候选Group-B扫描、清动作、重置和新目标发布。

实现保留独立全局选择值与actor字段。字段停止保留入口reset及此前循环副作用，阻断当前caller后缀和剩余公共清理；`0x00478A70/0x00478B20`写端同步当前Group-A canonical动作目标。

### 5.5 角色优先级两处

Group-B和Group-A分支的真实返回地址分别为`0x0045B2F2`和`0x0045B322`。前者ECX按`0x00525508 + index*0x2B28`形成，后者按`0x005029D0 + (index-8)*0x2F34`形成；leaf保留地址算术后的EAX、caller EDX和flags。返回后caller才符号扩展AX并参与metric与同组顺序计算。

实现直接借用startup/action owner，不增加generic端口调用。typed-stop发生在对应metric和顺序发布前。

### 5.6 效果协调器八处

八个物理返回地址依次为：

```text
0x0045C064  0x0045C0D1  0x0045C193  0x0045C1FE
0x0045C366  0x0045C458  0x0045CA7A  0x0045CBC5
```

前四类分别覆盖当前Group-A首读及其群体效果回读、当前Group-B首读及其群体效果回读；后四类覆盖两侧单体效果的Group-B/Group-A目标回读。首读flags来自角色索引地址算术；单体效果回读保留子效果返回寄存器和与1比较的flags；需要先执行Group-A奖励复制的两条路径保留该typed callee的EAX/EDX及caller提供的flags。

现代实现虽把路径组织为单体/群体helper，仍以八槽request和逐次结果保留每个物理caller身份，不把八处调用折叠成四个虚拟地址。停止保留首读、子效果或奖励复制前缀，阻断当前发布、反馈、framebuffer、奖励、剩余扫描及公共清理。

### 5.7 调试热键一处

`0x0045DBE3`读取固定特殊actor token `0x004E80FC`，真实返回地址为`0x0045DBE8`。入口EAX、EDX和flags继承角色优先级更新后的caller状态；返回后才符号扩展AX、定位Group-B目标并执行重置和action-block清理。

调试状态保留专属特殊actor canonical字段；旧查询ordinal改名为`reserved_query_special_action_target`并保持生产零调用。typed-stop保留C键重定向与优先级前缀，阻断目标重置和公共清理。

## 6. 双向追溯与验证范围

LST到C++追溯覆盖唯一word读、EAX局部寄存器写、普通RET、5处字段直接访问和20个真实物理caller。C++到LST反向追溯覆盖owner解析、字段/RET两个停止点、每类caller的入口寄存器与flags来源、返回地址、post-call符号扩展/比较/word发布，以及当前工作包涉及的generic setter写效果。

测试覆盖三类owner别名、零/一/`0xFFFF`/其它word、EAX高word、ECX/EDX/flags保持、普通RET栈、字段/RET停止、20个真实返回地址、七类caller后续语义、首轮/后续轮寄存器线程、停止前缀/后缀抑制，以及生产`0x004786E0` raw调用归零。

## 7. 正式验证

- 定向测试覆盖typed leaf、三类owner、七类caller、20个真实返回地址、寄存器与flags线程、canonical写入、setter同步以及typed-stop前缀/后缀。
- `./build-asan.sh --test`：`199/199`。
- `./build.sh core --test`：`199/199`。
- `./build.sh app --test`：`205/205`。
- 连续10轮`./build.sh core --test`：每轮`199/199`。
- inventory双生成稳定：`296/422 = 286 platform_adapted + 10 assembly_exact + 126 pending_audit`，SHA-256为`394486f01599ad2d0a0f3051079e8bcae840da84bf7cb21c24c814a2f9e2dcb8`。

## 8. 动态差分状态

当前缺少原版完整Group-A/Group-B及调试特殊actor、异常字段/栈内存页，以及20处caller的联合寄存器、flags与SEH捕获后端，原版动态差分登记为`blocked_runtime_oracle`。该阻塞不改变8字节leaf、5处字段访问和20处静态caller的LST闭环。
