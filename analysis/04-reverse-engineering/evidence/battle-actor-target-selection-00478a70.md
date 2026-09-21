# 战斗角色目标选择状态写入（0x00478A70）

## 1. 范围、边界与 ABI

权威 LST 把 `sub_478A70` 锁定为 `0x00478A70..0x00478A91`，半开区间为
`0x00478A70..0x00478A92`。主体共 34 字节、5 条实际指令、0 个 callee、0 个分支、
0 个局部标签和 1 个 `retn 4`，没有外部 chunk 或中段入口。

入口 ECX 是 actor token。唯一显式参数位于 `[ESP+4]`，函数只读取其低 16 位到 AX。
普通返回时 `retn 4` 同时移除返回地址和 4 字节参数槽，因此 ESP 相对入口增加 8，EIP
为物理返回地址。

第一条 MOV 只替换 EAX 低 word，保留高 16 位。ECX 保持 actor token，EDX 完整保持
入口值。全部五条指令均不修改 arithmetic flags，因此完整返回和任一访问点停止都保留
相应入口 flags。

## 2. 固定写入顺序

函数没有分支，严格执行：

```text
0x00478A70  AX <- word [ESP+4]
0x00478A75  dword [ECX+0x2AB4] <- 1
0x00478A7F  word  [ECX+0x29A2] <- AX
0x00478A86  word  [ECX+0x2A12] <- 0
0x00478A8F  retn 4
```

可观察结果为：

- 把 actor 空闲状态 latch 写为完整 dword 1；
- 把参数低 word 写入 actor action target；
- 只清 actor progress 的低 word，高 16 位保持；
- 参数 `0/1/0x7FFF/0x8000/0xFFFF` 均不做符号转换或范围夹值。

写入顺序不可重排。后续访问失败时，前面已完成的 actor 写入继续保留。

## 3. Canonical owner 与 typed-stop

实现不新增第二套 actor 状态。resolver 直接借用现有 owner：

```text
Group-A +0x2AB4  group_a_action_execution[index].idle_state_latch
Group-A +0x29A2  group_a_action_execution[index].action_target
Group-A +0x2A12  startup.party[index].progress.progress low word
Group-B +0x2AB4  group_b_lifecycle[index].action_execution.idle_state_latch
Group-B +0x29A2  group_b_lifecycle[index].action_execution.action_target
Group-B +0x2A12  startup.enemies[index].progress.progress low word
```

leaf 按五个真实访问阶段分别提供：

- `argument_read_typed_stop`；
- `idle_state_write_typed_stop`；
- `action_target_write_typed_stop`；
- `progress_write_typed_stop`；
- `return_address_read_typed_stop`。

失败访问本身不提交，之前已完成的写入、EAX 低 word、ECX、EDX、ESP、EIP 与 flags
保持。RET 读取停止时三个 actor 写入已全部提交，但 ESP 不推进，父 caller 后缀不执行。

现代 C++ 不能合法解引用任意 32-bit actor 或栈 token，也不能制造原 CPU page fault。
非法或非 canonical actor token 不在入口统一拒绝，而在第一个真实 actor 写入点停止。该
差异属于原访问点的最小平台适配，因此本目标登记为 `platform_adapted`，不能以 typed
owner、测试通过或 span 检查标记为 `assembly_exact`。

## 4. 二十个已关闭父 CALL

完整 LST 有 6 个父函数、20 个物理 CALL。六个父函数均已关闭，因此本轮没有延期 CALL：

```text
sub_4539B0  0x00454B8D -> 0x00454B92
sub_456680  0x00456B59 -> 0x00456B5E
             0x00456CEC -> 0x00456CF1
             0x00456D80 -> 0x00456D85
             0x00456E5A -> 0x00456E5F
             0x00456FC1 -> 0x00456FC6
sub_4576A0  0x00457903 -> 0x00457908
             0x00457925 -> 0x0045792A
             0x00457C18 -> 0x00457C1D
             0x00457D72 -> 0x00457D77
             0x00457DFB -> 0x00457E00
             0x00457E26 -> 0x00457E2B
sub_45ADF0  0x0045AF7D -> 0x0045AF82
sub_466F70  0x004671C9 -> 0x004671CE
sub_469D20  0x0046B522 -> 0x0046B527
             0x0046B5B9 -> 0x0046B5BE
             0x0046B733 -> 0x0046B738
             0x0046B850 -> 0x0046B855
             0x0046B8D4 -> 0x0046B8D9
             0x0046DC9C -> 0x0046DCA1
```

每个 caller 直接组合 typed leaf，并记录物理 call/return 地址、actor token、参数低
word、入口 EAX/EDX、flags 和结果。leaf 任一停止时，父函数立即返回对应 typed-stop，
保留 CALL 前父状态并阻断 CALL 后缀。

## 5. 六类 caller 接线

### 5.1 主动作分派

`0x00454B8D` 传入当前选中目标 word。Group-A 与 Group-B 的现代 C++ 分支仍记录同一
物理 CALL 身份。前置 terminal query 的 EAX/EDX 与逻辑 flags 进入 leaf；成功后不再由
父级重复写 action target。

### 5.2 Group-A frame

五处 CALL 保留 SI 或递减循环值形成的参数、actor token、到达 CALL 时 EAX/EDX 与
flags。`0x00456FC1` 继承 reset-target 回复的 EAX/EDX；其前置 CMP 只更新 flags。
Group-A 自身调用、nested action dispatch 和 nested post-action 共用全局 request offset，
避免嵌套调用重新消费 request[0]。nested trace 按真实执行顺序并回外层。

### 5.3 Group-B frame

六处 CALL 覆盖普通选择、phase-side、负状态、profile 与状态动作路径。
`0x00457DFB` 的两个现代分支共享同一物理地址，不能虚构成两个 CALL。profile 状态动作在
`0x00457E26` 写入目标后以 action kind 17 执行；同帧完成后原 `0x0045802C` actor reset
会再次把 canonical action kind 清零，测试分别验证中间写入和最终清理。

### 5.4 Post-action 与 message phase

`0x0045AF7D` 位于 post-action 后缀全局写入之前；停止时后缀不执行。
`0x004671C9` 位于 active actor 提交与 actor action mode 配置之间，参数为零；停止时保留
active actor 前缀并阻断后续配置。

### 5.5 Script dispatch

六处 CALL 分别属于 case 11、21、26 与 78：

- case 11 Group-A：`0x0046B522`，参数 0；
- case 11 Group-B：`0x0046B5B9`，参数 0；
- case 21 Group-A：权威 LST 不存在本 CALL，现代实现不得虚构；
- case 21 Group-B：`0x0046B733`，参数为角色编号；
- case 26 Group-A：`0x0046B850`，参数 0；
- case 26 Group-B：`0x0046B8D4`，参数 0；
- case 78 Group-A：`0x0046DC9C`，参数 0。

成功后执行各自原 action mode、cursor、frame 或 idle 后缀；typed-stop 只保留 CALL 前已
提交状态。旧 `pending_478a70` 枚举已删除，生产源码不再通过 generic/raw port 调用目标
地址。

## 6. Trace、请求与嵌套顺序

公共 trace 容量为 32，逐项保存 call 地址、return 地址、actor token 与参数。请求列表
可覆盖每个物理 CALL 的入口 ESP、五类访问可达性和入口寄存器；显式 caller 参数始终覆盖
请求模板中的 call/return、actor、参数、EAX、EDX 与 flags。

`request_offset + trace.calls` 决定当前请求索引。action、Group-A frame、Group-B frame、
post-action、message phase 和 script dispatch 均把嵌套已消费数量传播到子调用，并在返回
时合并 trace；不同父层不能重复使用同一个 fault request。

## 7. 双向追溯与测试范围

LST 到 C++ 已覆盖全部 5 条指令、3 个 actor 写、`retn 4`、EAX 低 word、ECX/EDX、
flags、ESP/EIP 和 5 个访问阶段。C++ 到 LST 反向追溯覆盖 canonical resolver、访问
trace、五类 typed-stop、20 个物理 caller 和全局 request offset；没有无来源的生产状态
写入或延期 CALL。

测试覆盖：

- Group-A/Group-B canonical owner；
- 参数 `0/1/0x7FFF/0x8000/0xFFFF`；
- EAX 高 word、ECX/EDX、flags、ESP/EIP 与五项访问 trace；
- 五个访问点逐一停止及 prefix partial commit；
- 二十组 call/return 地址常量与关键 caller 路径；
- action、Group-A/Group-B frame、post-action、message phase 和 script dispatch 后缀抑制；
- script case 11/21/26/78 的真实分支、参数和 cursor/state 结果；
- profile action 17 的中间 action-kind 写入与后续 `0x0045802C` reset；
- generic/raw `0x00478A70` 调用归零。

当前源码已通过 Linux core `199/199`、AddressSanitizer/UBSan `199/199`、Linux app
`205/205` 与连续十轮 core `199/199`。新文件全量和旧文件 changed-range clang-format、
`git diff --check`、退役 raw token、二十个物理 CALL 地址、构建日志 warning/test/sanitizer
扫描、TMP 分类、inventory 连续双生成及 staged/unstaged release audit 全部通过。未启动
原版或 OpenSWD3 游戏程序。

inventory 为 `306/422 = 296 platform_adapted + 10 assembly_exact + 116 pending_audit`，
SHA-256 为 `b7a3c5db9f5df536b2800e1dc4c5016587ea8e503376cafd12fddc0afb8d186d`。

## 8. 动态差分状态

当前缺少原版完整 Group-A/Group-B actor backing、异常参数栈、三个 actor 字段页、RET
栈页，以及二十处 caller 的寄存器、flags 与 SEH 联合捕获后端。原版动态差分登记为
`blocked_runtime_oracle`，不得写成 `original_diff_verified`。该阻塞不改变当前静态 LST
边界、五阶段访问顺序或二十个已关闭 CALL 身份。
