# 战斗角色运行状态重置（0x00478850）

## 1. 范围与边界

权威 LST 把 `sub_478850` 锁定为 `0x00478850..0x00478A61`，半开区间为
`0x00478850..0x00478A62`。主体共 530 字节、112 条实际指令、1 个 callee、
5 个条件跳转、5 个局部标签、1 个普通 `retn`、1 个 `rep movsd` 和 7 个
`rep stosd`，没有外部 chunk 或中段入口。

唯一 callee 为：

```text
0x00478A4D -> 0x00439070 / sub_439070
```

callee 接收固定上界 140，按原拒绝采样语义返回 `0..139`。本工作包复用现有
`LegacyBattleBoundedRandomPort`，不把它简化成单次 `% 140`，也不改变 secondary
RNG 的消费顺序。

入口 ECX 是 actor token，没有显式参数。函数保存并恢复 EBX、EBP、EDI；只有
`actor+0x2B04 == 1` 时才保存、使用并恢复 ESI。普通返回时 ESP 相对入口增加 4，
EIP 为物理返回地址。函数不执行 `cld`，全部 REP 使用入口 DF。

## 2. 固定前缀与条件字段

入口先保存寄存器并建立 `EBX=actor`、`EAX=0`。随后按 LST 顺序执行 37 项固定
字段访问，不能按字段类别重排。关键写入包括：

- `+0x2A12=0`、`+0x2AAC=0`、`+0x2AB0=0`、`+0x2AB4=0`；
- `+0x2A8C/+0x2A86/+0x2A6C/+0x2A70/+0x2A72/+0x2A7A` 的 word 写；
- `+0x2668=15`、`+0x266C=1`、`+0x2A68=2`、`+0x2A6A=8`；
- `+0x0316/+0x0318` 坐标 word 清零；
- `+0x26CC=0`。

`+0x0D9C` 只读取一个 byte 到 DL，EDX 高 24 位保持入口值。`TEST CL,DL` 只检查
bit 3；置位时额外清零 `+0x2AF0`。随后读取 `+0x0D94` byte 并检查 bit 2；置位时
再次清 `+0x2AF0`，把 `+0x266C` 写为 `0xFFFFFFE0`，并把 `+0x26D0` word 变为
`(old & 0xFFF7) | 0x0200`。

`+0x2B00` 使用完整 dword 严格比较 1。只有等于 1 时最后写
`+0x2AF0=1`，覆盖此前条件清零；0、2、`0xFFFFFFFF` 均不写。

`+0x2B04` 同样使用完整 dword 严格比较 1。只有等于 1 时执行 8 次
`rep movsd`，从 `actor+0x0D70` 复制到 `actor+0x0D50`。DF=0 时地址递增，DF=1
时地址递减；每次严格先读源 dword，再写目标 dword。值为 0、2 或
`0xFFFFFFFF` 时不触碰 ESI，也不复制。

## 3. 八段 REP、重复清零与 RNG

条件复制之后严格执行七段 `rep stosd`：

```text
38 dwords  from actor+0x0338
38 dwords  from actor+0x03D0
38 dwords  from actor+0x0468
38 dwords  from actor+0x0500
304 dwords from actor+0x0630
304 dwords from actor+0x0630
10 dwords  from actor+0x0D90
```

两次 304-dword 清零之间，函数另把 `actor+0x2630..0x263F` 四个 dword 逐项写
为零。两次 304-dword REP 是两个独立、顺序可观察的物理写序列，不能去重。
七段 STOSD 共 770 次 dword 写；加上条件 MOVSD 后共 8 个 REP 段。

最后恢复入口 EDI，并依次把 `+0x2A56/+0x2A5A/+0x2A5E/+0x2A62` 四个 dword
写为 `0xFFFFFFFF`。随后读取 `+0x2AA0` 完整 dword：

- 不等于 1：不调用 RNG，EAX 保留该完整 dword，flags 来自 `CMP value,1`；
- 等于 1：压入 140，在 `0x00478A4D` 调用 `sub_439070`，caller 清参，
  `EAX += 50`，再只把 AX 写回 `+0x2A12`。

RNG 正常结果范围为 50..189。ECX/EDX 保留 callee 返回 residue，不能恢复成
调用前值；非 RNG 路径最终 `EDX=actor+0x630`、`ECX=0xFFFFFFFF`。

## 4. Canonical owner 与共享 backing

实现不建立第二套完整 actor。`LegacyBattleActorRuntimeResetView` 只在调用时把现有
canonical owner 物化成瞬时 `0x2B18` byte image；每个成功物理写立即同步回所有
重叠 owner，因此后续读取、重复 REP、同址覆盖和 typed-stop 部分提交都观察到同一
backing。

Group-A 复用：

- startup party 的 progress、coordinates、base initialization、configuration、
  final processing 和 item-effect owner；
- action-dispatch 的 action execution、18 条 action record 与坐标 alias；
- `LegacyBattleStartupState::group_a_runtime_reset` 只保存尚无既有 owner 的 residual
  byte 区与标量，并由 heap owner 持有，避免扩大聚合测试栈帧。

Group-B 复用 startup enemy progress，以及 lifecycle 中的 base initialization、
action execution、action configuration、action composition、action record 和
coordinates；未建模 residual 位于每个
`LegacyBattleActorGroupBElementState::runtime_reset`。

写入重叠 canonical 字段时同步所有已知 alias，例如 `+0x2A12` 同步 progress 与
completion delay，`+0x2A8C` 同步 action execution 与 Group-B composition，
`+0x2B0C` 同步 residual 与 Group-A final processing，坐标复制同时同步 startup 与
action owner。

## 5. 物理访问、flags 与 typed-stop

最大路径包含 55 条显式内存访问指令、8-dword MOVSD 的 16 个物理访问、770 个
STOSD 写和 11 个栈访问，共 852 个物理访问事件。实现按访问 ordinal 区分：

- `actor_read_typed_stop`；
- `actor_write_typed_stop`；
- `stack_read_typed_stop`；
- `stack_write_typed_stop`；
- `random_call_typed_stop`。

失败访问本身不提交，此前写入、寄存器、flags、DF、ESP/EIP 和 REP 迭代保持。
MOVSD 的源读与目标写是两个不同停止点；STOSD 失败时当前 dword 不写，EDI/ECX 不
推进。第二次 304-dword REP 中段停止时，第一次 304-dword 清零和本次已完成前缀均
保留。RET 读取失败时全部 actor 写入已提交，但父 caller 后缀不得执行。

现代 C++ 不能合法解引用任意 32-bit actor/stack token，也不能制造真实 CPU page
fault。非法、非对齐或越界 actor token 不在函数入口提前拒绝；prologue 栈写仍先执行，
随后在第一个真实 actor 访问点停止。该差异属于原访问点的最小平台适配，因此本工作包
登记为 `platform_adapted`，不能以 owner、span 或测试通过标记为 `assembly_exact`。

## 6. 17 个已关闭父 CALL

完整 LST 有 9 个父函数、22 个物理 CALL。以下 7 个已关闭父函数中的 17 个 CALL
全部在原控制流位置直接组合 typed leaf：

```text
sub_4527E0  0x00452F93 -> 0x00452F98
             0x00453050 -> 0x00453055
sub_4539B0  0x00454FAF -> 0x00454FB4
sub_456680  0x004567D8 -> 0x004567DD
             0x00456965 -> 0x0045696A
             0x00456989 -> 0x0045698E
             0x004572AB -> 0x004572B0
sub_4576A0  0x00457791 -> 0x00457796
             0x004577CC -> 0x004577D1
             0x00457F23 -> 0x00457F28
             0x00457FD0 -> 0x00457FD5
             0x0045802C -> 0x00458031
sub_45ADF0  0x0045AE1D -> 0x0045AE22
             0x0045AEF6 -> 0x0045AEFB
sub_45D8F0  0x0045DC27 -> 0x0045DC2C
             0x0045DC58 -> 0x0045DC5D
sub_466F70  0x00467139 -> 0x0046713E
```

每个 caller 保存独立 call/return 地址、actor token、入口 EAX/EDX、同一 RNG owner
和 typed trace。leaf 任一停止时，父函数立即返回 typed-stop，保留 CALL 前父状态并
抑制 CALL 后缀。Group-A frame 的 nested post-action trace 按实际执行顺序并回外层；
request base offset 使用外层已消费 CALL 数，不能从 request[0] 重新开始。

frame coordinator 的 debug、selection 和 message 继续共享同一个
`SecondaryRngBoundedAdapter`，因此同一帧 secondary RNG 状态和消费顺序不分叉。
生产源码不再通过 generic/raw port 调用 `0x00478850`。

## 7. 五个精确延期 CALL

以下两个父函数仍为自身 inventory 的 `pending_audit`，本工作包不提前猜测其完整
控制流：

```text
sub_479850  0x004798F7  0x0047B723  0x0047B81A  0x0047B9A7
sub_47E880  0x0047E8B3
```

这 5 个 CALL 保留在 caller 审计结果中，分别延期到 `audit_order=316 /
0x00479850` 和 `audit_order=367 / 0x0047E880` 的父函数闭环。延期不计入本轮 17 个
已回收 CALL，也不把父函数误标为关闭。

## 8. 双向追溯与测试范围

LST 到 C++ 已覆盖完整 112 条指令、5 个条件跳转、8 个 REP、唯一 RNG CALL、寄存器
保存/恢复、flags、DF、ESP/EIP 和全部退出。C++ 到 LST 反向追溯覆盖瞬时 byte image、
canonical alias 同步、五类 typed-stop、17 个真实 caller 和 5 个延期 caller；没有
无法反查到 LST 或已登记平台适配的生产行为。

测试覆盖：

- Group-A/Group-B canonical owner 与非法 actor token；
- `+0x0D9C` bit 3、`+0x0D94` bit 2；
- `+0x2B00/+0x2B04/+0x2AA0` 的 `0/1/2/0xFFFFFFFF`；
- DF=0/1、8 个 REP 计数、两次 304-dword 清零及第二次中段 partial commit；
- 最大路径 852 个访问 ordinal 全扫，四类 actor/stack stop 均实际命中；
- RNG 结果 0/139、固定上界 140、调用次数、返回 residue 与随机 CALL stop；
- prologue 首个栈写、非法 actor 首字段写、RNG CALL、epilogue pop 和 RET stop；
- 17 个已关闭父 CALL 的物理 call/return 地址顺序；
- action-dispatch、debug hotkeys、message phase、post-action 的父级 typed-stop 后缀抑制；
- nested request offset、trace 合并和 generic/raw `0x00478850` 调用归零。

最终验证通过 Linux core `199/199`、AddressSanitizer/UBSan `199/199`、Linux app
`205/205`，以及连续十轮完整 Linux core，每轮均为 `199/199`。新文件全量与旧文件
changed-range clang-format Werror、`git diff --check`、17 地址唯一性、production raw
调用归零、构建注册、TMP 分类和 unstaged release audit 均通过；最终日志零
OpenSWD3 源码 warning、测试失败、sanitizer finding 或 runtime error。未启动原版或
OpenSWD3 游戏程序。

inventory 生成器连续双跑逐字节一致；当前计数为
`305/422 = 295 platform_adapted + 10 assembly_exact + 117 pending_audit`，
SHA-256 为
`6d2e8fc6ef51963ba50ab33c8d439dd34f0eb1f29f8e62fa9e782e098feec353`。

## 9. 动态差分状态

当前缺少原版完整 Group-A/Group-B actor backing、入口 DF=1、异常字段页、REP 中段
异常页、异常栈页、RNG 联合状态及 22 个 caller 的寄存器/flags/SEH 捕获后端。原版
动态差分登记为 `blocked_runtime_oracle`，不得写成 `original_diff_verified`。该阻塞
不改变当前静态 LST 收敛、17 个已关闭 CALL 身份和 5 个延期边界。
