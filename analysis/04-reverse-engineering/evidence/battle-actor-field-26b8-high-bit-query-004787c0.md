# 战斗角色`+0x26B8`高位查询（0x004787C0）

## 1. 范围与边界

权威LST把`sub_4787C0`锁定为`0x004787C0..0x004787C9`共10字节、3条实际指令、0个callee、0个分支和1个普通`retn`，没有外部chunk、中段入口或隐藏异常尾：

```text
004787C0  mov eax,[ecx+26B8h]
004787C6  shr eax,1Fh
004787C9  retn
```

入口ECX是角色对象token；函数没有显式栈参数。

## 2. 精确机器语义

函数必须按真实指令顺序执行：

1. 从`actor+0x26B8`读取完整dword到EAX；
2. 对完整EAX执行`SHR 31`；
3. 返回原dword bit31的零或一；
4. 从`[ESP]`读取返回地址，普通RET令ESP增加4。

字段读取覆盖完整EAX，不保留入口EAX高位。ECX保持actor token，EDX保持入口值。MOV和RET不改变flags。

`SHR 31`的CF取原值bit30；结果只能为零或一，因此SF恒零，ZF按结果是否为零，PF在结果零时置位、结果一时清除。AF未定义；计数大于一时OF未定义。typed结果分别保留AF和OF的definedness，不把未定义位伪造为已定义机器状态。

## 3. 故障与部分提交

`query_legacy_battle_actor_field_26b8_high_bit()`保留两个真实停止点：

1. `field_read_typed_stop`：停在`0x004787C0`，字段尚未读取，入口EAX、ECX、EDX、ESP、flags及OF definedness全部保持；
2. `return_address_read_typed_stop`：停在`0x004787C9`，字段读取和`SHR 31`结果已经提交，但不读取返回地址、不推进ESP。

正常返回记录字段token、字段读取、RET栈token和返回地址；EIP为调用者返回地址。实现不把完整dword读取折叠为独立布尔缓存。

## 4. Canonical owner与字段交叉引用

完整LST对`actor+0x26B8`只有7次访问：

- `0x00478770`：清除bit31；
- `0x0047878C`和`0x004787AE`：读取并设置bit31；
- `0x004787C0`：当前函数读取并查询bit31；
- `0x00478CA5`：以TEST检查bit31；
- `0x0047D51E`：基础初始化写零；
- `0x0047E6CF`：读取并以TEST决定后续绘制门。

查询直接复用Workpacks 299–300建立的中性`field_26b8` backing：

- Group-A：`LegacyBattleActionDispatchState::group_a_action_execution[index].field_26b8`；
- Group-B：`LegacyBattleStartupState::group_b_lifecycle[index].action_execution.field_26b8`。

query、high-bit clear和high-bit set resolver解析到同一物理地址。没有新增布尔缓存、平行actor数组、第二套字段或token槽。

## 5. 唯一物理caller

机器码全局扫描只有一条`call sub_4787C0`，位于已关闭的`sub_453200`：

```text
004533EB  mov  eax,dword_53BD54
004533F0  lea  ecx,[eax-8]
004533F3  mov  eax,ecx
004533F5  shl  eax,6
004533F8  sub  eax,ecx
004533FA  shl  eax,4
004533FD  sub  eax,ecx
004533FF  lea  ecx,[eax+eax*2]
00453402  lea  ecx,ds:5029D0h[ecx*4]
00453409  call sub_4787C0
0045340E  test eax,eax
00453410  jnz  loc_453434
```

真实CALL地址为`0x00453409`，返回地址为`0x0045340E`。令`index=dword_53BD54-8`，调用前线程为：

```text
EAX = ((((index << 6) - index) << 4) - index) = 1007 * index
ECX = 0x005029D0 + 0x2F34 * index
EDX = sub_42E850返回后未再改写的残值
```

两个LEA不改flags，因此CALL入口flags来自`0x004533FD`最后一次32位SUB，其left为`1008*index`、right为`index`。modern caller按相同两次SHL和两次SUB顺序计算EAX，并显式保留最后SUB flags与九宫格callee后的EDX snapshot。

leaf正常返回后，caller对完整EAX执行`TEST EAX,EAX`。结果一时按`JNZ`直接汇入`0x00453434`；结果零时才读取角色坐标并调用独立动作帧。TEST重置CF/OF、按零或一结果提交ZF/SF/PF并令AF未定义。leaf typed-stop保留面板动作更新和九宫格绘制前缀，阻断TEST、JNZ、独立动作帧、HUD及其余frame后缀。

原`actor_ready_query` generic端口槽保留ordinal并改为reserved名称，生产路径零调用。typed leaf在原控制流位置直接组合，物理CALL地址、返回地址、入口寄存器和flags不因共享实现而折叠。

## 6. 双向追溯与验证范围

LST到C++追溯覆盖10字节完整边界、完整dword读取、`SHR 31`、CF/PF/ZF/SF、AF/OF definedness、普通RET、两个故障点、EAX/ECX/EDX和ESP/EIP。C++到LST反向追溯覆盖Group-A/Group-B canonical owner、唯一CALL的两次SHL/两次SUB线程、九宫格后EDX、真实返回地址、post-call TEST/JNZ、独立动作帧条件与typed-stop后缀抑制。

定向测试覆盖原bit31清除和置位、原bit30清除和置位、完整返回寄存器、字段与栈访问、字段读取fault、RET fault、clear/set/query owner别名，以及唯一caller的入口EAX/EDX/SUB flags、返回地址、TEST两分支、reserved端口零调用和frame后缀阻断。

## 7. 动态差分状态

当前缺少原版完整Group-A actor、`actor+0x26B8`异常内存页、RET异常栈页，以及唯一caller联合寄存器、flags与SEH捕获后端，原版动态差分登记为`blocked_runtime_oracle`。该阻塞不改变10字节leaf和唯一静态caller的LST闭环。
