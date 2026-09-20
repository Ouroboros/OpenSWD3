# 战斗角色演出激活（0x004787F0）

## 1. 范围与边界

权威 LST 把 `sub_4787F0` 锁定为 `0x004787F0..0x0047882D` 共 62
字节、15 条实际指令、0 个 callee、3 个条件分支和 1 个 `retn 4`，没有
外部 chunk、中段入口或隐藏异常尾：

```text
004787F0  mov  edx,[esp+4]
004787F4  mov  eax,1
004787F9  mov  [ecx+2AB8h],edx
004787FF  mov  dl,[ecx+2A94h]
00478805  test dl,dl
00478807  mov  [ecx+2ABCh],eax
0047880D  jnz 00478816
0047880F  mov  byte [ecx+2A94h],6
00478816  cmp  [ecx+2AA0h],eax
0047881C  jnz 0047882B
0047881E  mov  ecx,[ecx+4]
00478821  test ecx,ecx
00478823  jz   0047882B
00478825  mov  word [ecx+4],0
0047882B  retn 4
```

入口 ECX 是角色对象 token；唯一显式栈参数是 `[ESP+4]` 的完整 dword。
`[ESP]` 是物理返回地址。

## 2. 精确机器语义

执行顺序不得折叠：

1. 从 `[ESP+4]` 读取完整 dword 参数到 EDX。
2. 把完整 EAX 设为 1；两条 MOV 均不修改入口 flags。
3. 把完整参数写入 actor `+0x2AB8`。
4. 只用 actor `+0x2A94` byte 覆盖 DL，EDX 高 24 位保持参数高 24 位。
5. 对原 marker byte 执行 `TEST DL,DL`。
6. 无条件把 actor `+0x2ABC` 完整 dword 写为 1。
7. 原 marker 为零时把 actor `+0x2A94` byte 写为 6；非零时不写。
8. 把 actor `+0x2AA0` 完整 dword 与 1 比较。
9. 只有完整值等于 1 时，才读取 actor `+0x04` 的 live-record token。
10. 只有 token 非零时，才把 `[token+4]` 的低 word 清零；高 word 保持。
11. 从 `[ESP]` 读取物理返回地址并执行 `retn 4`，正常 ESP 增加 8。

正常返回时 EAX 恒为 1，EDX 为
`(argument & 0xFFFFFF00) | original_marker`。即使 marker 随后从 0 写成 6，
返回 DL 仍是原始 0。

`actor+0x2AA0 != 1` 时，ECX 保持 actor token，flags 来自完整 dword
`CMP field,1`。值等于 1 且 live-record token 为零时，ECX 为 0，flags 来自
`TEST ECX,ECX` 的零结果；token 非零时 ECX 为该 token，flags 来自非零 TEST。
末尾 word MOV 和 RET 不修改 flags。

## 3. 九个访问点与部分提交

Typed 实现按真实访问顺序区分九个停止点：

- `argument_read_typed_stop`：停在 `0x004787F0`。入口寄存器、flags、ESP 和
  EIP 全部保持。
- `special_ready_write_typed_stop`：停在 `0x004787F9`。EDX 已读取完整参数，
  EAX 已为 1，入口 flags 保持。
- `marker_read_typed_stop`：停在 `0x004787FF`。`+0x2AB8` 已提交。
- `presentation_enabled_write_typed_stop`：停在 `0x00478807`。DL 已替换为
  原 marker，TEST flags 已提交。
- `marker_write_typed_stop`：停在 `0x0047880F`。`+0x2ABC=1` 已提交，原
  marker 为零的 TEST flags 保持。
- `source_runtime_value_read_typed_stop`：停在 `0x00478816`。此前 dword/byte
  写入保持，CMP flags 尚未提交。
- `live_record_token_read_typed_stop`：停在 `0x0047881E`。相等 CMP flags 已
  提交，ECX 仍是 actor token。
- `live_record_value_write_typed_stop`：停在 `0x00478825`。ECX 已替换为
  live-record token，非零 TEST flags 已提交。
- `return_address_read_typed_stop`：停在 `0x0047882B`。全部到达写入保持，
  ESP 不推进，EIP 不伪造为 caller 后缀。

正常结果同时记录参数与返回地址两次 stack 读取、七项 actor 访问顺序、完整
EAX/ECX/EDX、ESP/EIP、flags 及其 known 状态。

## 4. Canonical owner 与平台适配

现代实现不建立平行 actor 数组，各字段直接绑定既有生命周期 owner：

- Group-A `+0x2AB8` 与 `+0x2ABC` 绑定
  `startup.party[index].progress.special_ready/presentation_enabled`；
  `+0x2A94` 绑定 `base_initialization.field_2a94`；`+0x2AA0`、`+0x04`
  token 与记录值绑定 `configuration`。
- Group-B 对应字段绑定
  `startup.group_b_lifecycle[index].action_configuration`、
  `base_initialization.field_2a94`、`live_record_token` 与
  `live_record_value_04`。
- Group-B action configuration 每次重配时同步更新同一 live-record backing，
  不保留第二套影子值。

原程序允许裸 actor 指针、任意 `actor+0x04` token 和异常内存页。现代 C++ 只能对
已解析的 Group-A/Group-B owner 和已登记 live record 执行合法访问，因此固定 token
解析失败或访问不可达时，只在对应原访问点 typed-stop；不前移门、不回滚已提交前缀、
不伪造相邻内存。这是本工作包登记为 `platform_adapted` 的差异。

## 5. 七个物理 CALL

完整 LST 只有七处 `call sub_4787F0`，全部所属父函数已经关闭，并在原控制流位置
直接组合 typed leaf：

- `sub_4539B0`：`0x004540E4 -> 0x004540E9`、
  `0x004546B5 -> 0x004546BA`、`0x0045472E -> 0x00454733`。
- `sub_455D60`：`0x00456286 -> 0x0045628B`、
  `0x00456460 -> 0x00456465`。
- `sub_469D20`：`0x0046AF55 -> 0x0046AF5A`、
  `0x0046AFA8 -> 0x0046AFAD`。

七处参数均为完整 dword 1。动作 6 的 `0x004540E4` 和 `0x004546B5` 分属
目标阶段启动与完成路径，按各自真实 phase 到达；动作 33 的 `0x0045472E` 使用
Group-B 目标 actor。对手动作 17 的 `0x00456286` 使用当前 Group-B actor；对手动作
6 的 `0x00456460` 使用 Group-A 目标 actor。脚本 opcode 10 的两个分支分别使用
Group-A 与 Group-B actor。

每个站点独立保存 CALL/返回地址、actor token、入口 EAX/EDX、前驱 flags 和 known
状态。脚本两个 caller 还按 LST 恢复进入 CALL 前的乘法/减法寄存器残值：Group-A
使用 `1007/3021`，Group-B 使用 `1381/345`。leaf typed-stop 立即阻断各父函数
原返回地址后的目标发布、frame 刷新、workspace/终止状态、脚本 selection gate、frame
调用与 cursor 推进。

生产源码对 `0x004787F0` 只保留 typed leaf 地址身份和脚本 reserved 枚举；七个已
关闭路径不再通过 generic opaque port 执行该函数。

## 6. 双向追溯与测试范围

LST 到 C++ 追溯覆盖完整 62 字节、完整 dword 参数与字段比较、DL 局部覆盖、原 marker
保留、两次 TEST、CMP、条件 byte 写、live-record word 写和 `RETN 4`。C++ 到 LST
反向追溯覆盖 Group-A/Group-B canonical owner、九个真实访问点、七组 CALL/返回地址、
父级 typed-stop 后缀抑制及 production raw-call 归零。

定向测试覆盖：

- Group-A/Group-B canonical owner 与非法 token；
- 参数 `0/1/0x12345678/0xFFFFFFFF` 和 marker `0/1/0x80/0xFF`；
- `+0x2AA0` 的 `0/1/2/0xFFFFFFFF` 完整 dword 比较；
- live-record token 零/非零及只清低 word；
- 九个 typed-stop 的寄存器、flags、ESP/EIP、访问计数与部分提交；
- 七组物理 CALL/返回地址恒为 `return = call + 5`；
- 动作、对手动作和脚本三类父 caller 的正常路径、canonical backing、raw 调用为零，
  以及 `presentation_enabled_write_typed_stop` 后的完整后缀抑制。

最终验证结果：

- Linux core 完整门通过 `199/199`；
- AddressSanitizer/UBSan 完整门通过 `199/199`，无 sanitizer finding 或 runtime error；
- Linux app 完整门通过 `205/205`；
- 连续十轮 Linux core 均通过 `199/199`；
- 新文件全量和历史文件 changed-range clang-format 通过；
- inventory 生成器连续双跑逐字节一致，最终为
  `303/422 = 293 platform_adapted + 10 assembly_exact + 119 pending_audit`，
  SHA-256 为
  `5f746280df63b04b8f04a04b139170ee47fb6fc9f25e5e46aa5f89c6dea699cd`；
- 未启动原版或 OpenSWD3 游戏程序。

## 7. 动态差分状态

当前缺少原版完整 Group-A/Group-B actor、九个异常内存访问、异常栈页，以及七处
caller 联合寄存器、flags 与 SEH 捕获后端。原版动态差分登记为
`blocked_runtime_oracle`，不得写成 `original_diff_verified`。该阻塞不改变 62 字节
leaf、七处物理 CALL 身份与当前静态收敛结论。
