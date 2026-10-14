# 剧情 VM 共享交互锁 `0x00428D18`

## 结论

`sub_427920` 主分派 opcodes 42/43 共用入口 `0x00428D18`，两条指令物理长度均为 2 字节且没有 operand：

- opcode 42 对完整 32 位 `dword_4A9920` OR `0x00008000`，把受控角色内嵌 action `+0x08` 写零，再调用 `sub_4321E0(action)`；
- opcode 43 只对完整 32 位 `dword_4A9920` AND `0xFFFF7FFF`，不访问角色、不刷新 action；
- 两者最后均推进 2 字节、设置 `ESI=1`，发布 previous opcode 并在同一次 VM 调用中继续。

`dword_4A9920` 不是独立布尔量。它的低 15 位承载 dialog counter，bit15 承载世界交互锁；世界交互 map-event 激活、鼠标方向合成、世界移动门禁、dialog 生命周期及本 handler 必须观察同一个 32 位 owner。SDL runtime 因此以 `world_dialogs_.close.flagged_dialog_counter` 为 canonical owner：剧情 VM 已直接使用该字段，世界交互协调器新增可借用指针，真实 SDL 调用传入同一字段；单元测试未接 app runtime 时仍可回退到 `LegacyWorldInteractionState::global_lock`。

## 唯一汇编边界

opcode 42 分支为 `0x00428D18..0x00428DA1`：

```text
00428D18  cmp word ptr [opcode],2Ah
00428D1E  jnz loc_428DA6
00428D24  mov eax,dword_4AB378       ; controlled role index
00428D29  mov ebx,dword_4A9920
00428D2F  or  bh,80h                 ; set bit15 only
00428D32  lea eax,[eax+eax*2]
00428D35  mov dword_4A9920,ebx
00428D3B  lea eax,[eax+eax*8]
00428D3E  shl eax,3                  ; role stride 0xD8
00428D41  lea edx,dword_4BABE8[eax]  ; embedded action
00428D47  mov dword_4BABF0[eax],0    ; action +0x08
00428D51  push edx
00428D52  call sub_4321E0
00428D5A  test eax,eax
00428D5C  jnz loc_42D1E6
...       nullsub_1("Act Err(Talk:Rmlock)", action fields)
00428DA1  jmp loc_42D1E6
```

`or bh,80h` 等价于对完整 dword OR `0x00008000`，不会截断或覆盖 low15 counter、bit16..31。action 地址计算是 `controlled_index * 0xD8 + role_base + 0x40`，而写零目标 `dword_4BABF0` 正是 action `+0x08` 的完整 u32 `base_variant`。

`sub_4321E0` 返回非零直接进入成功尾；返回零只构造 `Act Err(Talk:Rmlock)` 参数并调用 `nullsub_1`，然后进入同一尾部。诊断无业务 consumer，因此 modern 记录 update/failure 次数但不停止、不回滚已经写入的 lock 或 base variant。

opcode 43 分支为 `0x00428DA6..0x00428DB3`：

```text
00428DA6  mov eax,dword_4A9920
00428DAB  and ah,7Fh                 ; clear bit15 only
00428DAE  mov dword_4A9920,eax
00428DB3  jmp loc_42D1EA
```

因为该入口的 dispatch 值只可能是 42 或 43，`cmp 2Ah / jnz` 的另一分支精确对应 opcode 43；没有隐藏 role/action side effect。

## 共享 continuation

opcode 42 在 helper 返回后先跳 `loc_42D1E6` 恢复被调用约定破坏的局部 `EBX`，随后和 opcode 43 共用 `loc_42D1EA`：

```text
0042D1E6  mov ebx,[var_50]
0042D1EA  add ebx,2
0042D1ED  add word ptr [talk_context+0],2
0042D1F2  mov [var_50],ebx
0042D1F6  mov esi,1
0042D1FB  jmp loc_42B0AE
```

`loc_42B0AE` 进入 common dispatch join，发布 effective opcode 到 `dword_4CF6D8`，然后抓取推进后窗口的下一条。因此 modern 两个 case 均在所有 handler side effect 完成后执行 `instruction_offset += 2`、发布 `previous_opcode` 并 `continue`。action update 返回零也不得 yield 或保留原 IP。

## `dword_4A9920` canonical owner

独立回查该全局的跨模块使用：

- `0x004277E6`：世界 map-event 激活直接赋值 `0x8000`；
- `0x00427866` 及 `0x00427AB9`：世界交互方向/尾部读取同一 dword；
- `0x00427DEC`：dialog 路径直接递增同一 dword；
- `0x004038CB`：世界移动门禁比较完整 dword 是否为零；
- `0x00428D29/0x00428DA6`：本 handler 设置/清除 bit15。

此前 modern 世界交互局部 owner 与 dialog counter 分离，会使 opcode 42 设置的锁无法阻止鼠标方向合成，map-event 设置的锁也无法被 opcode 43 清除。真实 SDL runtime 现统一到 `world_dialogs_.close.flagged_dialog_counter`：

1. `coordinate_legacy_world_interaction` 接受可选 `shared_global_lock`；map-event 写与方向门均使用选定引用；
2. SDL 调用传 `&world_dialogs_.close.flagged_dialog_counter`；
3. SDL 世界移动门禁也读取同一字段；
4. 直接模块测试省略该指针时保留局部 fallback，不复制或镜像两份状态。

这不是新增业务状态，而是恢复原版唯一全局 owner。

## Unsafe 点与平台适配

原版 opcode 42 用裸 `dword_4AB378` 计算角色地址；无有效角色时会形成越界地址。modern VM 的 public session boundary 在抓取 opcode 前验证受控角色 owner，因此无效 index 返回 `role_not_found`，opcode/count、lock、IP 和 previous 均保持。opcode 43 在机器内部不读角色，但同样受该既有 typed session 前置条件约束；测试分别锁定两个 opcode 的边界，不能从相邻 handler 继承。

原版 `sub_4321E0` 是进程内 action runtime。modern 通过 `LegacyWorldStoryVmPorts::update_action` 接入；返回零只计入诊断，不改变 continuation。世界交互的共享 owner 用 typed pointer 替代裸进程全局。上述差异均隔离原始裸 owner/地址域，有效 runtime 域中的位宽、顺序、callback 次数、IP、previous 与 same-call 行为保持一致，因此共享 handler 分类为 `platform_adapted`。

两条指令只需当前 opcode word。位于 `0x7FFE` 的完整记录会先完成全部 side effect、推进到 `0x8000`并发布 previous；随后下一次 fetch 才返回 `instruction_out_of_range`。42/43 各自具有独立窗口尾测试。

## 真实资产审计

锁定 inventory 观察到：

| opcode | 物理记录 | entry probes | raw word | 长度 | TALK1/2/3/4 |
| ---: | ---: | ---: | --- | ---: | --- |
| 42 | 84 | 84 | `0x002A`（84/84） | 2（84/84） | 68/15/1/0 |
| 43 | 62 | 62 | `0x002B`（62/62） | 2（62/62） | 52/8/2/0 |

真实回放分别使用：

```text
TALK1.DAT@0x000046EE: 2A 00
TALK1.DAT@0x0000A164: 2B 00
```

四文件raw byte-word候选：

- `0x002A` 为 TALK1/2/3/4=`170/47/19/44`，合计280；`0x402A/0x802A/0xC02A`均为0；
- `0x002B` 为 TALK1/2/3/4=`178/36/19/42`，合计275；`0x402B/0x802B/0xC02B`均为0。

只有inventory证明的84/62条作为指令记录；其余raw候选不扩张为资产入口。

## 测试覆盖

synthetic、integration 与 real 测试覆盖：

- 42/43 各四种raw opcode alias；
- 42完整u32 OR、base variant完整u32清零、一次action update、无audio与same-call continuation；
- action update返回零时保留lock/base写入，只增加failure诊断并继续；
- 43只清bit15，保留low15 counter及bit16..31，不刷新action；
- 42→43同调用组合使用同一owner并依次发布previous；
- 两个opcode各自的invalid controlled-session前置边界；
- 两个opcode各自的`0x7FFE`窗口尾副作用、IP与previous顺序；
- world interaction map-event写入外部shared owner且不镜像fallback；shared bit15阻止方向输入合成；
- SDL interaction调用与世界移动门禁绑定dialog canonical；
- `TALK1.DAT@0x000046EE`和`@0x0000A164`真实记录回放。

定向门运行三轮：首轮联合4/4通过并暴露3个aggregate initializer warning；修正后联合4/4通过且stderr空；补齐opcode43独立边界后VM 3/3通过且stderr空。

## 双向收敛与分类

LST→C++：42/43 shared compare、完整dword bit15位运算、受控角色0xD8步长、action `+0x08`写零、一次action update、零返回纯诊断、两字节推进、previous及same-call continuation均一一映射。

C++→LST：没有新增lock镜像、计数器截断、失败回滚、额外action refresh、audio、yield或operand读取。shared owner参数只恢复原版跨模块唯一全局；typed role/action owner与checked session边界明确记录为平台适配。

```text
assembly_exact;unit_tested;real_asset_tested;platform_adapted;sdl_runtime_integrated
```
