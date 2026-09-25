# 战斗角色目标选择 latch 设置（0x00478B30）

## 1. 范围、边界与 ABI

权威 LST 把 `sub_478B30` 锁定为 `0x00478B30..0x00478B3A`，半开区间为
`0x00478B30..0x00478B3B`。主体共 11 字节、2 条实际指令、0 个 callee、0 个分支和
1 个普通 `ret`，没有外部 chunk 或中段入口。`0x00478B3B..0x00478B3F` 是下一函数前的
对齐填充。

```text
0x00478B30  mov dword ptr [ecx+0x2AA8], 1
0x00478B3A  ret
```

入口 ECX 是 actor token，函数没有显式参数。字段写和 RET 都不修改 EAX、ECX、EDX 或
算术 flags。正常返回时 ESP 相对入口增加 4，EIP 为物理返回地址。

## 2. 字段、访问顺序与停止点

唯一业务写入是把 `actor+0x2AA8` 的完整 dword 无条件替换为 `1`。字段原值为 `0`、`1`、
`2`、`0x12345678`、`0x80000000` 或 `0xFFFFFFFF` 都走同一路径，不存在读取、比较、条件
跳过、符号扩展或算术回绕。

严格物理访问顺序为：

1. 向 `actor+0x2AA8` 写入 dword `1`；
2. RET 从 `[ESP]` 读取返回地址。

现代实现保留两个独立 typed-stop：

- 字段写不可达时不提交字段，EIP 停在 `0x00478B30`；
- RET 读取不可达时字段已经提交，ESP 不推进，EIP 停在 `0x00478B3A`。

两种停止都保持入口 EAX、ECX、EDX 和 flags。只有 RET 成功时才记录返回地址读取、推进
ESP 并把 EIP 改为调用点返回地址。

## 3. canonical owner

完整字段交叉引用和相邻 getter `sub_478B40` 证明 `actor+0x2AA8` 是 actor-local dword。
`sub_47D350` 的 `0x0047D580` 指令会直接把该字段写零，因此它不属于全局
`action_execution_active`。

实现把原无语义的 `LegacyBattleActorRuntimeResetState::field_2aa8` 收敛为
`target_selection_latch`，并继续由既有 runtime-reset residual owner 承载：

- Group-A：`LegacyBattleStartupState::group_a_runtime_reset[index]`；
- Group-B：`LegacyBattleStartupState::group_b_lifecycle[index].runtime_reset`。

专用 resolver 只解析本函数实际访问的单个 dword，不要求完整 runtime-reset view 的 progress、
action、coordinates 等无关 owner，也没有建立第二套 actor 状态。runtime-reset 物化和回写仍
在偏移 `0x2AA8` 使用同一 canonical 字段。

## 4. 三个已关闭父 CALL

完整 LST 只有三处真实 `E8` CALL，三个父函数均已关闭，本工作包没有延期 caller：

```text
sub_4539B0  0x00454BAE -> 0x00454BB3
sub_456680  0x00456B51 -> 0x00456B56
sub_4576A0  0x004578FB -> 0x00457900
```

### 4.1 Action dispatch

`0x00454BA7` 先调用 `sub_4707B0`，随后把当前 Group-A actor token 放入 ECX，并在
`0x00454BAE` 调用目标。typed leaf 的入口 EAX、EDX 与 flags 来自紧邻 scene publish
回复。只有 leaf 正常返回后才设置 action runtime 高字节 bit 7；字段写或 RET typed-stop
均保留 scene publication 前缀并阻断该 OR 后缀。

### 4.2 Group-A frame

两条候选扫描路径都保存实际命中候选的 terminal reply。`0x00456B51` 的入口 EAX、EDX
来自该 reply，flags 精确来自 `cmp eax,1` 的 32 位减法结果。leaf 正常返回后才把选中索引
压栈并调用 `0x00478A70`；typed-stop 保留 target-ready 前缀，阻断 target selection 与
后续 selection-complete 处理。

### 4.3 Group-B frame

候选重建路径在 `0x004578FB` 以当前 Group-B actor token 调用目标。入口 EAX、EDX 来自
命中的 Group-A terminal reply，flags 来自 `cmp eax,1`。leaf 正常返回后才调用
`0x00478A70` 并写 action pending；typed-stop 保留 target-ready 前缀，阻断 target write
和 pending 后缀。

Group-A nested action dispatch 会把父结果已经消费的 call 数叠加到 request offset，并按物理
顺序把 nested CALL、return 与 actor token trace 合并回外层结果。测试用非零初始 offset、
独立 request 内容和真实 action-22 前置资源证明该路径没有回用错误 request。

## 5. 双向追溯与测试范围

LST 到 C++ 已覆盖无条件 dword 写、普通 RET、两个真实访问点、EAX/ECX/EDX 与 flags
保持、ESP/EIP 和 partial commit。C++ 到 LST 反向追溯覆盖 canonical resolver、惰性
heap-backed request/trace、request offset、三个物理 caller 和父级后缀抑制；没有无来源业务
分支或状态写。

汇编独立测试覆盖：

- Group-A、Group-B 与非法 token resolver；
- 六种 dword 初值都无条件写为 `1`；
- 字段写停止与 RET 停止的不同提交前缀；
- EAX、ECX、EDX、flags、ESP、EIP 与物理返回地址；
- 显式 request offset；
- 三组 CALL/return/actor 物理身份；
- action dispatch、Group-A 与 Group-B 的 typed-stop 和后缀抑制；
- Group-A nested request offset 与 trace 合并；
- production generic/raw `0x00478B30` 调用归零。

最后一轮完整正向与反向追溯没有产生新的未解释差异。

## 6. 分类与动态差分状态

任意 32 位 actor token、字段页和 RET 栈页无法由现代 C++ 直接合法解引用。canonical token
resolver、原访问点 typed-stop、惰性 request/trace 和显式 request offset 是最小平台边界，
因此本目标登记为 `platform_adapted`，不能以 owner、span、测试通过或 typed-stop 单独标记为
`assembly_exact`。

当前缺少原版完整 Group-A/Group-B actor backing、`+0x2AA8` 异常字段页、RET 异常栈页，
以及三个 caller/callee 的联合寄存器、flags 与 SEH 捕获后端。原版动态差分登记为
`blocked_runtime_oracle`，不得写成 `original_diff_verified`。

## 7. 验证与关闭状态

- 定向 `openswd3_battle_legacy_battle_setup_tests` 退出码为零；
- Linux core `199/199`、Linux app `205/205`、ASan/UBSan core `199/199` 全部通过；
- ASan/UBSan 日志为零 sanitizer finding，core、app 与 sanitizer 构建日志为零 warning 和
  error；
- 连续十轮串行 Linux core 均为 `199/199`，十个独立日志的失败与告警计数为零；
- 新 header、source、UT 全量 clang-format 审计和旧文件 changed-range clang-format 已通过；
- `git diff --check`、production raw address、旧 `field_2aa8`、inventory 双生成和未跟踪物料
  分类审计已通过；
- inventory 为 `312/422 = 302 platform_adapted + 10 assembly_exact + 110 pending_audit`，
  SHA-256 为 `bf69e0e06746fc36650e43f0edea4b3deb5b4b0885026b976e09246f6559dd9d`；
- LST 摘录哈希分别为
  `0b0537c4616044f432e86bfdc7eb666f252873efa15667cb8cdd46ceb352145d` 和
  `949d2e165157b2bb241a5c7ffdf3d1c47198bbad2da107a8b5e0fa2f618c9379`；
- 未启动原版或 OpenSWD3 游戏程序。

Workpack 312 以 `platform_adapted` 关闭。下一条 inventory 游标为
`audit_order=313 / 0x00478B40 / sub_478B40`。
