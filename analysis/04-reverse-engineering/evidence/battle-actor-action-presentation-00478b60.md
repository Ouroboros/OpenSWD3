# 战斗角色行动表现主流程（0x00478B60）

## 1. 范围、边界与 ABI

权威 LST 把 `sub_478B60` 锁定为 `0x00478B60..0x00479849`，末条
`retn 4` 占 3 字节，半开区间为 `0x00478B60..0x0047984C`。主体共有 805 条实际
指令、31 个静态 CALL、119 个跳转和六个 `retn 4`，没有外部 chunk 或中段入口。

入口物理栈参数为 actor token 与 effect dword。prologue 保存 EBP、EDI、ESI、EBX，
`0x00478B71` 无条件把 effect dword 写入 `actor+0x2AF4`。六个出口均恢复四个保存寄存器
并执行 `retn 4`；正常返回时 ESP 相对入口增加 8，返回地址读取失败时 ESP 不推进。

六个出口地址为：

```text
0x00478CD1
0x0047914C
0x0047915F
0x00479184
0x00479198
0x00479849
```

## 2. canonical owner 与共享 actor image

实现复用 Workpack 309 的 runtime-reset actor image，并把大小扩展到 `0x2B1C`，覆盖本函数
最高访问字段 `+0x2B18`。Group-A 与 Group-B token 均解析到既有 canonical owner：

```text
Group-A  action.group_a_action_execution[index]
         startup.group_a_runtime_reset[index]
         startup.party[index] 及既有共享 owner
Group-B  startup.group_b_lifecycle[index].action_execution
         startup.group_b_lifecycle[index].runtime_reset
         startup.enemies[index] 及既有共享 owner
```

物化和同步层只表达原内存重叠，不建立第二套 actor、action record、坐标、progress、资源或
runtime-reset 状态。每次原读写都有独立物理地址；写入一旦成功便立即同步 canonical owner，
typed stop 保留已提交前缀。

## 3. 坐标模式与入口前半段

`actor+0x26D6` 的 word 决定坐标模式：

- mode 3：Y 以 word 加 30，先发生 16 位回绕，再作 signed word 比较；达到 alternate Y 时
  清 completion/mode words，并在 `0x00478BD6` 组合 typed high-bit-set child。
- mode 2：Y 以 word 加 `0xFFF6`；signed 结果小于等于零时清 Y 与两个 mode words。
- mode 1：`0x00478C07` 调 `sub_439070(2)`，只在 AX 等于 1 时移动 X；方向由
  `source_runtime==1 ? mirror!=0 : mirror==0` 决定。
- 其他 mode：不执行三类坐标动作。

只有进入 mode 1 路径时，`0x00478C45` 才把 `actor+0x26D6` 的 completion word 无条件递减一；其他 mode 从 `0x00478C4C` 继续。

`0x00478C6A..0x00478CA5` 保留 source-runtime、motion、scene 与 script gate 的原短路顺序。
`actor+0x26B8` bit31 置位时，`0x00478CB3` 查询 `sub_47BA80`；只有完整 EAX 等于 1 才在
`0x00478CC8` 组合 typed high-bit-clear child，并从 `0x00478CD1` 返回。查询结果不是 1 时走
公共尾部且不清字段。`actor+0x2AAC` 只在完整 dword 精确等于 1 时提前返回。

## 4. action record 选择与 snapshot

函数按 LST 顺序重置 frame-source action record，并从 profile、source-runtime、override、
presentation kind 和 variant override 选择 `base_variant`。可观察值包括：

```text
0x24  0x26  0x2C  0x2D  0x31  0x33  0x48  0x49  0x4A  0x4B
```

`actor+0x2A0E` 是完整 word override，不能布尔化或截成 byte。source-runtime live record 的
`+4` 与 `+0xA` word 保留原可访问性、读取顺序和阈值运算。

snapshot 阶段保留：

- `actor+0x02FA` bit 3 选择 display kind，并清 source word；
- `actor+0x0D94` bit 2 复制完整 action record，并置 mode bit 3；
- action kind、资源 token、帧索引、坐标与重叠 word/dword 的原写入顺序。

## 5. update、绘制与六个返回路径

第一记录 update 位于 `0x00478FD1`。EAX 为零时直接走公共尾部；非零时在
`0x00478FF1` 查询资源并写回 canonical render source。
最终暂存审计将 `0x0047902A` 读提前到 `0x00479035` 写之前、将 `0x00479276` 写提前到
`0x0047927C` 资源读之前，并在 bit31/mode-bit-2 绘制分支分别将 `0x00479479`、
`0x004794E4` 读放在 `0x0047947F`、`0x004794EA` 写之前。对应 fault-ordinal
回归核对了地址、相邻访问序数和后续资源读取停止时已即时提交的 canonical 写入。

四个 call-then-return 路径为：

```text
mode bit 3              0x00479143 sub_47F3C0 -> 0x0047914C
mode bit 9              0x00479156 sub_47F580 -> 0x0047915F
mode bit 7, kind 0      0x0047917B sub_47F710 -> 0x00479184
mode bit 10, kind 0x1C  0x0047918F sub_47F710 -> 0x00479198
```

其余主绘制分支保留 `actor+0x26C0` bits 31/25/26、mode bit 2、overlay enable 和 action
kind 2 的原条件。bit26 的 `sub_47CC60` 仍是精确延期的 pending child，不伪装为已关闭。

`0x004795EF` 的 `sub_4507A0` 已组合 typed ten-place decimal child。其 typed stop 保留
物理 CALL 和 decimal 前缀并阻断后缀。sample 分支保留 `0x0047960B sub_485610`、
`0x00479634 sub_485650`、signed X `<320` 的 `-16/+16` pan 参数，以及 sample word 清零。

source-runtime/action-kind-6 保留第二记录 update、lookup、draw、sample；additional kind/index
非零时保留附加 lookup 与 draw。正常公共尾部顺序固定为：

```text
0x00479801 sub_480AE0
0x00479808 sub_480D40
0x00479840 sub_47E650
0x00479849 retn 4
```

## 6. 31 个物理 CALL

完整 LST 与生产源码的 CALL 地址逐项一致：

```text
0x00478BD6 sub_478780
0x00478C07 sub_439070
0x00478CB3 sub_47BA80
0x00478CC8 sub_478770
0x00478D13 nullsub_1 = 0x0044A240
0x00478FD1 sub_4321E0
0x00478FF1 sub_4315D0
0x00479143 sub_47F3C0
0x00479156 sub_47F580
0x0047917B sub_47F710
0x0047918F sub_47F710
0x004792A9 sub_4170E0
0x0047931F sub_417050
0x0047938F sub_417050
0x00479444 sub_4170E0
0x00479461 sub_47CC60
0x004794C2 sub_4170E0
0x0047952D sub_4170E0
0x004795AB sub_4170E0
0x004795EF sub_4507A0
0x0047960B sub_485610
0x00479634 sub_485650
0x00479675 sub_4321E0
0x00479695 sub_4315D0
0x00479706 sub_4170E0
0x0047971A sub_485610
0x00479785 sub_4315D0
0x004797F7 sub_4170E0
0x00479801 sub_480AE0
0x00479808 sub_480D40
0x00479840 sub_47E650
```

机械审计保存于 `build/workpack315/call-identity-audit.txt`，结果为
`expected=31 / actual=31 / mismatches=0`。所有 pending child 保留物理 CALL/return 地址、
callee token、参数顺序、回复寄存器与 trace 顺序。

## 7. 两个真实 caller

完整 LST 只有两个 caller：

```text
Group-A  0x004566DB -> 0x004566E0
Group-B  0x0045822C -> 0x00458231
```

两个 caller 均在原控制流位置直接组合 typed leaf，传入 canonical actor token 和完整
boolean-derived effect stack argument。测试分别覆盖 effect 0 和 1。父级在判断 typed stop 前先
合并 leaf 已完成的 physical CALL、high-bit-set、high-bit-clear 和 decimal trace；停止后不执行
原本不可到达的 frame suffix。

生产源码没有 raw generic `0x00478B60` 调用，测试没有
`port.push(0x00478B60U, ...)` reply。

## 8. typed-stop 与测试覆盖

独立停止点覆盖：

- actor/read/write/resource/nested-record 的每个 normal-path access ordinal；
- normal path 的每个 pending CALL ordinal；
- typed high-bit-set、typed high-bit-clear 与 typed decimal child；
- 六个 `retn 4` 的返回地址读取；
- 两个父 caller 的 typed-stop prefix merge 与 suffix suppression。

汇编独立向量完整覆盖：canonical resolver、入口 effect 写、mode 1/2/3、AX 双侧、signed
阈值、16 位回绕、提前 gate、high-bit query/clear、exact completion latch、十种 action value、
full-word override、snapshot、update zero/nonzero、四个立即返回、bit31/25/26、mode bit2、
overlay、action kind 2、decimal child、sample pan 双侧、第二记录、附加记录、normal tail 和两个
真实 caller。

## 9. 分类与动态差分状态

任意 32 位 actor token、最高到 `+0x2B18` 的字段页、live nested record、31 个 callee 和
六个 RET 栈页无法由现代 C++ 直接合法解引用或调用。canonical resolver、共享 actor image、
原访问点 typed-stop、closed-child 组合与 pending CALL trace 是最小平台边界，因此最终目标
分类为 `platform_adapted`，不能以 typed owner、span、测试或 CALL 数量标成
`assembly_exact`。

当前缺少原版完整 Group-A/Group-B actor backing、异常字段页、nested-record 页、六个异常
RET 栈页，以及两个 caller 与 callee 的联合寄存器、flags 和 SEH 捕获后端。原版动态差分登记为
`blocked_runtime_oracle`，不得写成 `original_diff_verified`。未启动原版或 OpenSWD3 游戏程序。

## 10. 最终闭环状态

生产 typed leaf、两个真实 caller、closed-child trace merge 与全部汇编独立测试已经完成。
最终验证结果：

- battle aggregate 定向测试通过；
- Linux core `199/199`；
- Linux app `205/205`；
- AddressSanitizer/UBSan `199/199`；
- 连续十轮 Linux core `10/10`，每轮 `199/199`；
- 31 个物理 CALL 地址 `expected=31 / actual=31 / mismatches=0`；
- inventory 双生成逐字节一致；
- inventory 为 `315/422 = 305 platform_adapted + 10 assembly_exact + 107 pending_audit`；
- inventory SHA-256 为
  `8f70033463b588e8a32cfdea2784c7b3142872a27a64d49bedcf51c8696858c6`；
- 新文件全量 clang-format、历史文件 changed-range clang-format 与 `git diff --check`
  通过；
- 编译告警、测试失败、sanitizer finding、runtime error、production raw 调用与测试 raw
  reply 均为零；
- 未启动原版或 OpenSWD3 游戏程序。

最终双向 REVIEW 未发现未映射 LST 行为，也未发现无 LST 地址或未登记平台边界的 C++
行为。本工作包最终分类为 `platform_adapted`；原版动态差分仍仅登记为
`blocked_runtime_oracle`。

LST 摘录 SHA-256：

```text
6afcf694a7280fce989df8fb73739b48ee468aa20aacb7741a34d762d740d532  function.lst
374788fd2a4ffe12d611d3d1ade9fcf4a9721015a2df0b4355482437b690e883  callers.lst
```
