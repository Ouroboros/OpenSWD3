# 战斗角色启动门 latch 查询（0x00478B50）

## 1. 范围、边界与 ABI

权威 LST 把 `sub_478B50` 锁定为 `0x00478B50..0x00478B56`，半开区间为
`0x00478B50..0x00478B57`。主体共 7 字节、2 条实际指令、0 个 callee、0 个分支和
1 个普通 `ret`，没有外部 chunk 或中段入口。`0x00478B57..0x00478B5F` 是对齐填充，
下一函数从 `0x00478B60` 开始。

```text
0x00478B50  mov eax, dword ptr [ecx+0x2AE0]
0x00478B56  ret
```

入口 ECX 是 actor token，没有物理栈参数。字段读取完整覆盖 EAX；ECX、EDX 和算术
flags 保持。正常 RET 从 `[ESP]` 读取返回地址，ESP 增加 4，EIP 转移到物理返回地址。

## 2. 原访问顺序与 typed-stop

严格物理访问顺序只有两步：

1. 在 `0x00478B50` 读取 `actor+0x2AE0` dword；
2. 在 `0x00478B56` 从 `[ESP]` 读取返回地址。

现代 typed 边界保留两个独立停止点：

- `start_gate_latch_read_typed_stop`：字段值尚未提交，EAX、ECX、EDX、flags 和 ESP
  保持入口状态，EIP 为 `0x00478B50`；
- `return_address_read_typed_stop`：完整字段 dword 已提交到 EAX，ESP 尚未推进，EIP
  为 `0x00478B56`。

## 3. 字段交叉引用与 canonical owner

完整 LST 中 `actor+0x2AE0` 的全部五处直接访问为：

```text
0x0047887B  mov [ebx+0x2AE0], eax
0x00478AC7  mov dword ptr [ecx+0x2AE0], 1
0x00478B0A  mov [ecx+0x2AE0], edx
0x00478B50  mov eax, [ecx+0x2AE0]
0x0047D5CE  mov [esi+0x2AE0], ebx
```

Workpacks 309 和 310 已把该字段收敛到 action-execution canonical owner，并命名为
`start_gate_latch`：

```text
Group-A  action.group_a_action_execution[index]
Group-B  startup.group_b_lifecycle[index].action_execution
```

本 getter 复用 `LegacyBattleActorStartGateIncrementView`、
`LegacyBattleActorStartGateIncrementOwners` 和
`resolve_legacy_battle_actor_start_gate_increment`。不建立第二套 actor、启动门或 latch
状态。

## 4. 唯一已关闭父 CALL

完整 LST 只有一个物理 CALL：

```text
0x00457842 -> 0x00457847  sub_4576A0
```

它位于 Group-B frame 的 Group-A 候选扫描。进入循环前 EBX 固定为 1，ESI 指向当前
Group-A actor。只有 terminal 不等于 1、固定 AI dword 不等于 1、blocked 查询不等于
1 时才到达本 CALL。

CALL 前寄存器和 flags 为：

```text
ECX   = 当前 Group-A actor token
EAX   = blocked 查询 EAX
EDX   = blocked 查询 EDX
flags = CMP32(blocked 查询 EAX, 1)
```

leaf 正常返回后，`0x00457847` 重新执行完整 dword `cmp eax,1`。只有字段值精确等于
1 才跳到 `0x00457876` 并跳过候选后缀；`0`、`2`、`0x80000000`、`0xFFFFFFFF`
等所有其他 dword 都继续到 `0x0045784D` 的回合完成查询。不能把字段布尔化。

leaf typed-stop 发生在 caller 比较之前。它保留已经发生的 terminal、AI 与 blocked
查询前缀，阻断回合完成、当前 Group-B actor 空闲查询、control 清理、启动门递增、
phase progress 累加和当前候选的后续处理。

## 5. 双向追溯与测试范围

当前 LST 审计已经覆盖唯一字段读、普通 RET、完整 EAX 覆盖、ECX/EDX/flags 保持、
ESP/EIP、两个物理访问阶段，以及唯一 caller 的完整前后缀。

汇编独立测试向量要求覆盖：

- Group-A 与 Group-B canonical resolver；
- 非法、未对齐和 one-past token；
- 完整 dword `0/1/2/0x7FFFFFFF/0x80000000/0xFFFFFFFF`；
- 字段读停止与 RET 读停止的 partial commit；
- known/unknown flags；
- 非零 request offset；
- `0x00457842 -> 0x00457847` 的物理 trace 和 Group-A actor token；
- 字段精确等于 1 的跳过分支与完整非 1 dword 的继续分支；
- 父级两个 typed-stop 的前缀保留与全部后缀抑制；
- production raw `0x00478B50` 调用归零。

## 6. 分类与动态差分状态

任意 32 位 actor token、`+0x2AE0` 字段页和 RET 栈页无法由现代 C++ 直接合法解引用。
canonical token resolver 与两个原访问点 typed-stop 是最小平台边界，因此目标分类为
`platform_adapted`，不能以 owner、span 或测试通过标记为 `assembly_exact`。

当前缺少原版完整 Group-A/Group-B actor backing、`+0x2AE0` 异常字段页、RET 异常栈页，
以及唯一 caller/callee 的联合寄存器、flags 与 SEH 捕获后端。原版动态差分登记为
`blocked_runtime_oracle`，不得写成 `original_diff_verified`。

## 7. 最终闭环状态

生产 typed leaf 与唯一 Group-B caller 已接入。完整正向与反向追溯未发现未解释差异，
production raw 调用和测试 raw reply 均已归零。定向测试、Linux core `199/199`、
Linux app `205/205`、AddressSanitizer/UBSan `199/199` 和连续十轮 Linux core
`199/199` 全部通过。

inventory 连续双生成逐字节一致，最终状态为
`314/422 = 304 platform_adapted + 10 assembly_exact + 108 pending_audit`，SHA-256 为
`350003b628090d8f7cc4d22b130ee5cd6d9326f2649f4ba5cc27e768f8f29b0a`。新增文件全量
clang-format、历史文件 changed-range clang-format、`git diff --check`、raw 地址、
编译告警、测试失败、sanitizer finding、runtime error 和发布前完整差异 REVIEW 全部
通过。原版动态差分仍仅登记为 `blocked_runtime_oracle`，未启动原版或 OpenSWD3 游戏
程序。本工作包最终分类为 `platform_adapted`。

LST 摘录 SHA-256：

```text
687e3b3c1b5bd0a6a8aa76b60c19cfd8616a1a4995e411bdd11591370f2c512c  leaf.lst
a42a2d496d4c25a5488298f9602ced64eec8deb87e2f1a035d5811b950031a78  caller.lst
```
