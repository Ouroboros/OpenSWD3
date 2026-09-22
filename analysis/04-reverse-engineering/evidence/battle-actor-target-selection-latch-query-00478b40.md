# 战斗角色目标选择 latch 查询（0x00478B40）

## 1. 范围、边界与 ABI

权威 LST 把 `sub_478B40` 锁定为 `0x00478B40..0x00478B46`，半开区间为
`0x00478B40..0x00478B47`。主体共 7 字节、2 条实际指令、0 个 callee、0 个分支和
1 个普通 `ret`，没有外部 chunk 或中段入口。`0x00478B47..0x00478B4F` 是下一函数前的
对齐填充。

```text
0x00478B40  mov eax, dword ptr [ecx+0x2AA8]
0x00478B46  ret
```

入口 ECX 是 actor token。正常返回时 EAX 被完整替换为 `actor+0x2AA8` 的 dword，ECX、
EDX 和 arithmetic flags 保持入口值，ESP 增加 4，EIP 为物理返回地址。

## 2. 字段、访问顺序与停止点

严格物理访问顺序为：

1. 从 `actor+0x2AA8` 读取完整 dword 到 EAX；
2. RET 从 `[ESP]` 读取返回地址。

现代实现保留两个独立 typed-stop：

- 字段读不可达时不提交 EAX，ECX、EDX、flags 和 ESP 保持，EIP 停在
  `0x00478B40`；
- RET 读取不可达时 EAX 已提交完整字段值，ESP 不推进，EIP 停在
  `0x00478B46`。

正常返回与两个停止点都不把字段值布尔化。`0`、`1`、`2`、`0x7FFFFFFF`、
`0x80000000` 和 `0xFFFFFFFF` 均按完整 32 位值处理。

## 3. canonical owner

getter 复用 Workpack 312 已收敛的 actor-local canonical 字段和 resolver：

- Group-A：`LegacyBattleStartupState::group_a_runtime_reset[index]`
  的 `target_selection_latch`；
- Group-B：`LegacyBattleStartupState::group_b_lifecycle[index].runtime_reset`
  的 `target_selection_latch`。

getter 与 setter 共用 `LegacyBattleActorTargetSelectionLatchView`、
`LegacyBattleActorTargetSelectionLatchOwners` 和
`resolve_legacy_battle_actor_target_selection_latch`，没有第二套 actor 状态、字段缓存或
解析器。

## 4. 四个已关闭父 CALL

完整 LST 只有四处真实 `E8` CALL；`__thiscall` 原型注释不是物理调用。两个父函数均已
关闭，本工作包没有延期 caller。

```text
sub_456680  0x00456B60 -> 0x00456B65
sub_456680  0x00456F12 -> 0x00456F17
sub_456680  0x00457140 -> 0x00457145
sub_4576A0  0x00457EC3 -> 0x00457EC8
```

### 4.1 `0x00456B60`

Group-A frame 以当前 Group-A actor token 调用 getter。返回后执行完整 dword
`cmp eax,1`；只有值严格等于 1 才进入选择完成后缀，`2` 等其他值走不等分支。
typed-stop 阻断 CMP 和两侧后缀。

### 4.2 `0x00456F12`

Group-A frame 在 action-mode 查询和共享状态前缀后调用 getter。返回后执行
`test eax,eax`，随后的 MOV 覆盖 EAX 但不修改 TEST flags，JNZ 仍由 getter 的完整
零/非零结果决定。测试以 `0x80000000` 证明非零 dword 不能布尔化成其他窄值。typed-stop
保留 caller 前缀并阻断 TEST、MOV 和两侧后缀。

### 4.3 `0x00457140`

Group-A frame 仅在前置 ZF 没有绕过 CALL 时执行 getter。返回后执行完整 dword
`cmp eax,1`；值等于 1 才进入 Group-B gate-decay 后缀。typed-stop 保留此前 action-target
clear 与本地清理前缀，并阻断 CMP 和 gate-decay 后缀。

### 4.4 `0x00457EC3`

Group-B frame 先读取并清空 action target，再以同一 Group-B actor token 调用 getter。
返回后执行完整 dword `cmp eax,1`；值等于 1 才进入 Group-A gate-decay 后缀。typed-stop
保留已提交的 action-target clear，并阻断比较、衰减和 runtime-reset 后缀。

四处 caller 都记录物理 CALL、return 和 actor token。生产路径不再通过 generic port 调用
`0x00478B40`。

## 5. request、trace 与嵌套边界

request 与 trace 使用惰性 heap-backed array，避免把大型聚合测试状态压入栈帧。leaf call
wrapper 以 `request_offset + trace.calls` 选择请求，并有非零 offset 的直接测试。

Group-A 与 Group-B frame 会在进入 nested dispatcher 前转发已消费 call 数，并支持合并
nested trace。机械搜索确认当前 getter 仅由本节四个 frame CALL 消费；现有 nested action 和
opponent dispatcher 不调用该 getter，因此不存在可执行的 nested getter request 消费路径。
本工作包保留前向接线，但不伪造不存在的动态调用或测试。

## 6. 双向追溯与测试范围

LST 到 C++ 已覆盖完整 dword 字段读、普通 RET、两个真实访问点、EAX 完整覆盖、ECX/EDX
与 flags 保持、ESP/EIP 和 partial commit。C++ 到 LST 反向追溯覆盖 canonical resolver、
惰性 request/trace、显式 request offset、四个物理 caller 和父级后缀抑制，没有无来源业务
分支或状态写。

汇编独立测试覆盖：

- Group-A、Group-B、非法、未对齐和越界 token；
- 六组完整 dword 值；
- 字段读停止、RET 停止及不同提交前缀；
- EAX、ECX、EDX、flags、known 状态、ESP、EIP 和物理返回地址；
- 非零 request offset；
- 四组 CALL/return/actor 物理身份；
- 四个父 caller 的等于、不等于、零、非零与 typed-stop 后缀抑制；
- `0x00457EC3` 停止时保留 action-target clear；
- production generic/raw `0x00478B40` 调用归零。

最后一轮完整正向与反向追溯没有产生新的未解释差异。

## 7. 分类与动态差分状态

任意 32 位 actor token、字段页和 RET 栈页无法由现代 C++ 直接合法解引用。canonical token
resolver、原访问点 typed-stop、惰性 request/trace 和显式 request offset 是最小平台边界，
因此本目标登记为 `platform_adapted`，不能以 owner、span、测试通过或 typed-stop 单独标记为
`assembly_exact`。

当前缺少原版完整 Group-A/Group-B actor backing、`+0x2AA8` 异常字段页、RET 异常栈页，
以及四个 caller/callee 的联合寄存器、flags 与 SEH 捕获后端。原版动态差分登记为
`blocked_runtime_oracle`，不得写成 `original_diff_verified`。

## 8. 验证与关闭状态

- 定向 `openswd3_battle_legacy_battle_setup_tests` 退出码为零；
- Linux core `199/199`、Linux app `205/205`、ASan/UBSan core `199/199` 全部通过；
- ASan/UBSan 日志为零 sanitizer finding，core、app 与 sanitizer 日志为零 warning、error
  和测试失败；
- 连续十轮串行 Linux core 均为 `199/199`；
- LST 摘录 SHA-256 分别为
  `4000e3dac0c87c45bcc041df3c7d406024a6fb0aa157968f1edaca30088dd98d` 和
  `c54a5f8038f3eff8ded6a8f08071dfde7db1a71bc72d0cfc3ac71002bc700c39`；
- inventory 连续两次生成逐字节一致，结果为
  `313/422 = 303 platform_adapted + 10 assembly_exact + 109 pending_audit`，SHA-256 为
  `2a55f9cf35d6a4c01024b6e95ddc4c2344a9f4e7e9fd34eece7275514894f879`；
- 新文件全量 clang-format、旧文件 changed-range clang-format、`git diff --check` 和
  release 静态审计全部通过；
- 未启动原版或 OpenSWD3 游戏程序。

Workpack 313 以 `platform_adapted` 关闭。下一条 inventory 游标为
`audit_order=314 / 0x00478B50 / sub_478B50`。
