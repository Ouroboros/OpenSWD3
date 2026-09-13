# 战斗角色`+0x26B8`高位设置（0x00478780）

## 1. 范围与边界

权威LST把`sub_478780`锁定为`0x00478780..0x004787B4`共53字节、11条实际指令、0个callee、2个条件分支和1个普通`retn`，没有外部chunk、中段入口或隐藏异常尾：

```text
00478780  test dword ptr [ecx+26C0h],2000000h
0047878A  jnz  short locret_4787B4
0047878C  mov  eax,[ecx+26B8h]
00478792  test eax,80000000h
00478797  jz   short loc_4787A9
00478799  xor  edx,edx
0047879B  mov  [ecx+2A78h],dx
004787A2  mov  [ecx+542h],dx
004787A9  or   eax,80000000h
004787AE  mov  [ecx+26B8h],eax
004787B4  retn
```

入口ECX是角色对象token；函数没有显式栈参数。

## 2. 精确机器语义

执行顺序不得折叠：

1. 对完整`actor+0x26C0` dword执行`TEST 0x02000000`；bit25置位时直接走普通RET，不访问后三个字段。
2. gate未置位时读取完整`actor+0x26B8` dword到EAX，并以`TEST 0x80000000`检查原bit31。
3. 原bit31置位时先执行`xor edx,edx`，再依次把`DX=0`写入`actor+0x2A78`和`actor+0x0542`两个word；第二次写故障必须保留第一次写。
4. 无论原bit31是否置位，都对EAX执行`OR 0x80000000`并把完整dword写回`actor+0x26B8`。
5. 从`[ESP]`读取返回地址，普通RET令ESP增加4。

原bit31已置位时仍执行最终完整dword写回；原bit31未置位时不得触碰两个word。gate早退保持入口EAX和EDX；正常非gate路径以最终`actor+0x26B8`值返回EAX，原bit31置位时EDX为零，否则EDX保持入口值。ECX始终保持角色token。

## 3. Flags与部分提交

`TEST actor+0x26C0`提交逻辑flags；gate早退时这些flags一直保持到RET。字段读取不改flags，因此`actor+0x26B8`读取故障保留第一次TEST结果。第二次TEST提交bit31测试flags；原bit31置位后`XOR EDX,EDX`提交零结果flags，两个word写不改flags。最终OR提交完整EAX结果flags：CF和OF清零，ZF、SF、PF按结果，AF未定义；最终结果bit31恒置位，所以SF恒一、ZF恒零。字段写和RET不改flags。

Typed实现按真实指令地址区分六个停止点：

- `field_26c0_read_typed_stop`：`0x00478780`，入口寄存器、ESP和入口flags保持，零字段访问。
- `field_26b8_read_typed_stop`：`0x0047878C`，保留第一次TEST flags，只提交`+0x26C0`读取。
- `summon_completion_word_write_typed_stop`：`0x0047879B`，EAX保留字段读取值，EDX已清零，保留XOR flags，两个word均未写。
- `special_target_command_cursor_write_typed_stop`：`0x004787A2`，保留`+0x2A78`第一次word写，不提交第二次word和最终dword写。
- `field_26b8_write_typed_stop`：`0x004787AE`，保留已到达的两个word写、OR后的EAX和flags，不提交最终dword写。
- `return_address_read_typed_stop`：`0x004787B4`，保留全部actor字段提交和最终flags，不读取返回地址、不推进ESP。

正常结果记录四字段token、逐项访问顺序、RET栈token、返回地址、完整寄存器、flags及known状态。

## 4. Canonical owner与字段交叉引用

完整LST对四个物理字段共找到132处访问：`+0x26C0`为111次、27个函数；`+0x26B8`为7次、6个函数；`+0x2A78`为13次、4个函数；`+0x0542`只有当前函数一次写入。现有证据不足以给`+0x26C0`完整dword赋予更窄业务名，因此以`LegacyBattleActorField26c0`保持中性命名。

字段只复用既有owner：

- `+0x26C0`：以action-execution为canonical backing，startup party/enemy progress持有绑定视图；copy/move construction共享backing，assignment复制值但不改写destination既有alias。
- `+0x26B8`：复用Workpack 299建立的`field_26b8`。
- `+0x2A78`：复用`LegacyBattleGroupAActionExecutionState::summon_completion_word`。
- `+0x0542`：复用slot 4 `special_target_action_record.command_cursor`。

Group-A解析到`LegacyBattleActionDispatchState::group_a_action_execution[index]`；Group-B解析到`LegacyBattleStartupState::group_b_lifecycle[index].action_execution`。解析时只把对应startup progress的`field_26c0`视图绑定到上述canonical backing，不新增平行actor数组、第二套状态、缓存或token槽。

## 5. 二十七个已关闭物理CALL

机器码扫描得到35处物理CALL。以下27处所属caller已经关闭，并在原控制流位置直接组合typed leaf；括号内为`CALL地址 -> 返回地址`：

- `sub_4539B0`三处：`0x004541BE -> 0x004541C3`、`0x00454469 -> 0x0045446E`、`0x00454D81 -> 0x00454D86`。
- `sub_4576A0`一处：`0x00458161 -> 0x00458166`。
- `sub_4582B0`两处：`0x00458BAA -> 0x00458BAF`、`0x00458BD7 -> 0x00458BDC`。
- `sub_458DE0`六处：`0x00459404 -> 0x00459409`、`0x00459438 -> 0x0045943D`、`0x00459451 -> 0x00459456`、`0x00459513 -> 0x00459518`、`0x004596B0 -> 0x004596B5`、`0x004597F1 -> 0x004597F6`。
- `sub_469D20`一处：`0x0046DAD8 -> 0x0046DADD`。
- `sub_46F8C0`一处：`0x0046FB7B -> 0x0046FB80`。
- `sub_4728E0`一处：`0x00472B65 -> 0x00472B6A`。
- `sub_4731A0`一处：`0x004734A3 -> 0x004734A8`。
- `sub_4735B0`一处：`0x00473A94 -> 0x00473A99`。
- `sub_473C10`一处：`0x00473E0F -> 0x00473E14`。
- `sub_4745B0`两处：`0x0047470B -> 0x00474710`、`0x00474943 -> 0x00474948`。
- `sub_474FC0`一处：`0x00475035 -> 0x0047503A`。
- `sub_4751C0`一处：`0x0047548F -> 0x00475494`。
- `sub_4758A0`五处：`0x00475B19 -> 0x00475B1E`、`0x00475B98 -> 0x00475B9D`、`0x00475BE2 -> 0x00475BE7`、`0x00475DD6 -> 0x00475DDB`、`0x00475E2C -> 0x00475E31`。

每个站点独立保存物理返回地址；共享C++ helper只减少样板，不折叠物理CALL身份。caller按LST线程调用前EAX、EDX、ECX actor token、最后一条算术或TEST/CMP产生的flags及known状态。条件站点只在原bit、callee返回值、动作阶段或循环门真实成立时追加trace；leaf typed-stop立即抑制原返回地址后的字段清理、坐标、效果、资源、循环或父级后缀。嵌套目标效果的`0x00475035`保持在外层动作400/动作4自身站点之前的真实动态顺序，未到达的外层CALL不伪造trace。

## 6. 八个延期物理CALL

以下8处CALL所属父函数仍是后续工作包，当前只登记精确延期边界，不把未审父函数复制到现代实现：

- `sub_478B60`：`0x00478BD6 -> 0x00478BDB`。
- `sub_47E5C0`：`0x0047E611 -> 0x0047E616`。
- `sub_47FC40`：`0x00480058 -> 0x0048005D`。
- `sub_481010`：`0x004811E6 -> 0x004811EB`。
- `sub_481A40`两处：`0x00481BC3 -> 0x00481BC8`、`0x00481C22 -> 0x00481C27`。
- `sub_4838D0`：`0x00483B0D -> 0x00483B12`。
- `sub_483DB0`：`0x00483F61 -> 0x00483F66`。

Workpack 300只对当前已有窄reply的`sub_478B60`补充`executed`、actor token、入口EAX/EDX、flags与known状态。Group-A/Group-B frame仅在reply明确表示`0x00478BD6`真实执行时组合typed leaf，使用固定返回地址`0x00478BDB`；未执行reply不得产生副作用。reply同时保留第二个pending high-bit-set槽，供同一未审调用中出现第二次物理CALL时保持独立身份。

Workpack 315必须把该leaf直接放回`actor+0x2B10`写入之后、`0x00478BDB`无条件跳转之前；父函数内typed-stop必须阻断真实后缀。其余七个延期caller由各自工作包按同一typed接口恢复入口寄存器、flags、条件门和后缀，不以当前工作包伪装关闭。

## 7. 双向追溯与验证范围

LST到C++追溯覆盖53字节完整边界、两次TEST、条件双word清零、无条件最终OR/write、普通RET、六个故障点、部分提交、寄存器和flags。C++到LST反向追溯覆盖四个canonical字段、27个已关闭CALL的独立返回地址、8个延期CALL身份、`sub_478B60`窄reply、第二pending槽、嵌套trace顺序及caller后缀抑制。生产源码对`0x00478780`只保留typed地址常量和reserved枚举，已关闭路径不再执行generic opaque调用。

测试覆盖gate早退、原bit31清/置两分支、两个word写顺序、最终dword写、六类typed-stop、部分提交、EAX/ECX/EDX、ESP/EIP、flags、Group-A/Group-B owner alias、27+8 caller分类、`sub_478B60`执行/未执行窄reply、第二pending槽、现代frame typed-stop后缀及全部生产caller trace。ASan还暴露聚合战斗测试中历史巨型frame-coordinator栈帧；仅测试侧把大型state、port、fixture和result放到堆，不改变生产行为。

## 8. 动态差分状态

当前缺少原版完整Group-A/Group-B actor、四字段异常内存页、RET异常栈页，以及35处caller的联合寄存器、flags与SEH捕获后端，原版动态差分登记为`blocked_runtime_oracle`。该阻塞不改变53字节leaf、35处物理CALL身份和27/8静态关闭边界。
