# 战斗角色启动门递增与 latch 发布（0x00478AC0）

## 1. 范围、边界与 ABI

权威 LST 把 `sub_478AC0` 锁定为 `0x00478AC0..0x00478AD1`，半开区间为
`0x00478AC0..0x00478AD2`。主体共 18 字节、3 条实际指令、0 个 callee、0 个分支、
0 个局部标签和 1 个普通 `ret`，没有外部 chunk 或中段入口。

```text
0x00478AC0  inc word ptr [ecx+0x2A74]
0x00478AC7  mov dword ptr [ecx+0x2AE0], 1
0x00478AD1  ret
```

入口 ECX 是 actor token。函数没有显式参数。EAX、ECX 和 EDX 的数值不被三条指令
修改。普通返回时 RET 读取返回地址，ESP 相对入口增加 4，EIP 为物理返回地址。

## 2. 原访问顺序、flags 与 typed-stop

严格物理访问顺序为：

1. 读取 `actor+0x2A74` 的 16 位旧值；
2. 把 16 位递增结果写回 `actor+0x2A74`；
3. 把 dword `1` 写到 `actor+0x2AE0`；
4. RET 从 `[ESP]` 读取返回地址。

`INC word` 按 16 位自然回绕，`0xFFFF` 递增为 `0x0000`。它保留入口 CF，按 16 位
结果更新 OF、SF、ZF、AF 和 PF。后续 MOV 与 RET 不修改这些算术 flags。

现代实现保留四个独立物理访问阶段：

1. `start_gate_read_typed_stop`：没有字段或 flags 提交，入口寄存器、ESP 和 EIP 保持；
2. `start_gate_write_typed_stop`：旧 word 已读取，但新 word 与 INC flags 尚未提交；
3. `start_gate_latch_write_typed_stop`：启动门递增与 INC flags 已提交，latch 尚未写入，
   EIP 停在 `0x00478AC7`；
4. `return_address_read_typed_stop`：两个字段写均已提交，ESP 尚未推进，EIP 停在
   `0x00478AD1`。

正常路径记录两个字段 token、旧值与新值、字段访问次数、返回地址读取、返回寄存器、
ESP/EIP、flags 和物理 CALL 身份。

## 3. 字段交叉引用与 canonical owner

完整 LST 中 `actor+0x2A74` 的全部 9 处直接访问为：

```text
0x0046F8C8  cmp word ptr [esi+0x2A74], bp
0x00473C1B  cmp word ptr [esi+0x2A74], bp
0x004745B8  cmp word ptr [esi+0x2A74], bp
0x004758AD  cmp word ptr [esi+0x2A74], di
0x004786D0  mov ax, [ecx+0x2A74]
0x00478AC0  inc word ptr [ecx+0x2A74]
0x00478AE0  mov ax, [ecx+0x2A74]
0x00478AEF  mov [ecx+0x2A74], ax
0x0047D4ED  mov [esi+0x2A74], bx
```

该字段继续使用现有中性名称 `start_gate`。

完整 LST 中 `actor+0x2AE0` 的全部 5 处直接访问为：

```text
0x0047887B  mov [ebx+0x2AE0], eax
0x00478AC7  mov dword ptr [ecx+0x2AE0], 1
0x00478B0A  mov [ecx+0x2AE0], edx
0x00478B50  mov eax, [ecx+0x2AE0]
0x0047D5CE  mov [esi+0x2AE0], ebx
```

本轮把它以中性名称 `start_gate_latch` 放入同一 action-execution canonical owner：

```text
Group-A  action.group_a_action_execution[index]
Group-B  startup.group_b_lifecycle[index].action_execution
```

runtime reset actor image 继续负责偏移 `0x2AE0` 的导入导出，但原 residual
`field_2ae0` 已删除。没有第二套 actor、启动门或 latch 镜像。

## 4. 五个已关闭父 CALL

完整 LST 只有五个物理 CALL，均位于已关闭父函数：

```text
0x00456FE1 -> 0x00456FE6  sub_456680
0x0045707A -> 0x0045707F  sub_456680
0x0045786B -> 0x00457870  sub_4576A0
0x00457E1C -> 0x00457E21  sub_4576A0
0x0046DD4A -> 0x0046DD4F  sub_469D20
```

五处 CALL 全部在原控制流位置直接组合 typed leaf，本轮没有延期 caller。trace 保留
CALL 地址、返回地址和 actor token；父函数把 CALL 前的 EAX、ECX、EDX 和 flags 传入
leaf，任一 typed-stop 均立即抑制尚未执行的父级后缀。

### 4.1 Group-A frame：0x00456FE1 与 0x0045707A

`0x00456FE1` 位于已选 Group-B 目标路径。正常前缀先对当前 Group-A actor 调用
`0x00478A70`，再从 signed SI 形成 Group-B token。地址算术同时留下
`EAX = 1381*n`、`EDX = 345*n` 和 `SUB32(24*n,n)` flags；成功返回后才比较
`dword_53C010` 并继续 frame 后缀。

`0x0045707A` 位于 Group-B 候选循环。父级对同一候选执行两次 `0x0047CE80`；第一次
结果不等于 1 且第二次结果为 0 时才调用本 leaf。入口 EAX/EDX 来自第二次查询，flags
来自 `TEST EAX,EAX`；成功后才把 EBP 置 1 并推进循环。

### 4.2 Group-B frame：0x0045786B 与 0x00457E1C

`0x0045786B` 位于 Group-A 候选扫描。父级先完成排除、状态、完成和空闲检查，再调用
`0x0047C660`；本 leaf 接收该 callee 的 EAX/EDX/flags。成功后才递增
`dword_53BD7C` 并推进扫描。

`0x00457E1C` 位于已选 Group-A actor 路径。父级先把 `word ptr dword_53BCFC` 传给当前
Group-B actor 的 `0x00478A70`，再从完整 dword index 形成 Group-A token。地址算术留下
`EAX = 1007*n`、`EDX = 3021*n` 和 `SUB32(1008*n,n)` flags；成功后才跳到共同 frame
后缀。

### 4.3 Script dispatch：0x0046DD4A

脚本 case 78 以已发布 actor code 形成 Group-B token：

```text
EAX = 1381 * actor_code
EDX = 345 * actor_code
ECX = 0x005229E0 + 0x2B28 * actor_code
flags = SUB32(24 * actor_code, actor_code)
```

乘法和地址运算按 32 位自然回绕。成功后才清 `dword_53C02C` 与 `word_53CE98` 并进入
共同脚本后缀。typed-stop 保留 CALL 前已经提交的列表清零、共享 gate 和其他前缀写入，
但阻断这两个后缀写、selected actor reset、cursor 推进与 case 返回后缀。

## 5. 双向追溯与测试范围

LST 到 C++ 已覆盖唯一 16 位读改写、dword latch 写、普通 RET、16 位回绕、
EAX/ECX/EDX、ESP/EIP、INC flags 和四个访问阶段。C++ 到 LST 反向追溯覆盖 canonical
resolver、请求、trace、五个物理 caller、三类父函数中的地址算术和 typed-stop；没有无来源
字段写或延期 caller。

汇编独立测试向量覆盖：

- Group-A 与 Group-B canonical owner；
- `start_gate` 旧值 `0/1/0x7FFF/0x8000/0xFFFF`；
- `0x7FFF -> 0x8000` 和 `0xFFFF -> 0` 的 INC16 flags；
- CF、EAX、ECX、EDX 保持；
- 四个 typed-stop 阶段及其 partial commit；
- RET 成功时 ESP 增加 4、EIP 等于物理返回地址；
- 五个 CALL 的地址、返回地址、actor token 和真实执行顺序；
- Group-A 选定目标与候选扫描、Group-B 候选扫描与选定 actor、脚本 case 78；
- leaf typed-stop 后各父函数完整后缀抑制；
- production generic/raw `0x00478AC0` 调用归零。

## 6. 分类与动态差分状态

任意 32 位 actor token、两个字段页和 RET 栈页无法由现代 C++ 直接合法解引用。canonical
token resolver 和原访问点 typed-stop 是最小平台边界，因此本目标登记为
`platform_adapted`，不能以 owner、span 或测试通过标记为 `assembly_exact`。

当前缺少原版完整 Group-A/Group-B actor backing、`+0x2A74` 读写异常字段页、
`+0x2AE0` 写异常字段页、RET 异常栈页，以及五个 caller/callee 的联合寄存器、flags 与
SEH 捕获后端。原版动态差分登记为 `blocked_runtime_oracle`，不得写成
`original_diff_verified`。

## 7. 验证与关闭

最终定向战斗测试静默通过；格式化后的 AddressSanitizer/UBSan 与 Linux core 均为
`199/199`，Linux app 为 `205/205`，连续十轮 Linux core 均为 `199/199`。最终日志扫描
没有 OpenSWD3 源码 warning、编译 error、测试失败、sanitizer finding 或 runtime error。

新文件全量 clang-format、旧文件 changed-range clang-format、`git diff --check`、生产
raw/generic 调用、TMP 分类和完整 release diff 审计均通过。未启动原版或 OpenSWD3
游戏程序。

LST 地址摘录 SHA-256 为
`d3bae325e81346b380595357d05bc3e13e436593549c1501389f62b4ac9af4a1`。inventory 生成器
连续双跑逐字节一致，关闭结果为
`309/422 = 299 platform_adapted + 10 assembly_exact + 113 pending_audit`，inventory
SHA-256 为 `68a4b609093e1fcc352040de93133a266e3eb780b43e98aa34997e73ef9d5257`。
下一条 `pending_audit` 为 `audit_order=310 / 0x00478AE0 / sub_478AE0`。
