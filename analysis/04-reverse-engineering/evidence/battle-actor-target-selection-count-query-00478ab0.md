# 战斗角色目标选择计数查询（0x00478AB0）

## 1. 范围、边界与 ABI

权威 LST 把 `sub_478AB0` 锁定为 `0x00478AB0..0x00478AB7`，半开区间为
`0x00478AB0..0x00478AB8`。主体共 8 字节、2 条实际指令、0 个 callee、0 个分支、
0 个局部标签和 1 个普通 `ret`，没有外部 chunk 或中段入口。

```text
0x00478AB0  mov ax, [ecx+0x2A76]
0x00478AB7  ret
```

入口 ECX 是 actor token。函数没有显式参数。字段读取只替换 AX，保留 EAX 高 16 位；
ECX、EDX 和全部算术 flags 保持。普通返回时 RET 读取返回地址，ESP 相对入口增加 4，
EIP 为物理返回地址。

## 2. 原访问顺序与 typed-stop

现代实现保留两个独立物理访问阶段：

1. `count_read_typed_stop`：字段读取失败时，入口 EAX、ECX、EDX、flags、ESP 和 EIP
   全部保持；
2. `return_address_read_typed_stop`：字段读取和 AX 替换已完成，ESP 尚未推进，EIP 停在
   `0x00478AB7`。

正常路径记录字段 token、读取值、一次字段读取、一次返回地址读取、返回寄存器、ESP/EIP、
flags 及物理 CALL 身份。MOV 与 RET 都不修改算术 flags。

现代 C++ 不能合法解引用任意 32-bit actor token，也不能直接制造原 CPU 字段页或栈页
fault。canonical token resolver 和原访问点 typed-stop 是最小平台边界，因此本目标登记为
`platform_adapted`，不能以 owner、span 或测试通过标记为 `assembly_exact`。

## 3. 字段交叉引用与 canonical owner

完整 LST 中 `actor+0x2A76` 的全部直接访问为：

```text
0x00478AA0  inc word ptr [ecx+0x2A76]
0x00478AB0  mov ax, [ecx+0x2A76]
0x00478AF6  mov ax, [ecx+0x2A76]
0x00478B03  mov [ecx+0x2A76], ax
```

本目标复用 Workpack 307 建立的中性字段 `target_selection_count`：

```text
Group-A  action.group_a_action_execution[index].target_selection_count
Group-B  startup.group_b_lifecycle[index].action_execution.target_selection_count
```

不新增第二套 actor、计数缓存或脚本专用镜像。相邻 `sub_478AE0` 的条件递减与清零行为仍
属于后续独立工作包。

## 4. 唯一已关闭父 CALL

完整 LST 只有一个物理 CALL，位于已关闭的脚本分派 `sub_469D20` case 61：

```text
0x0046D429 -> 0x0046D42E
```

trace 记录 CALL 地址、返回地址和 actor token。该 CALL 已从 generic port 删除，本轮没有
延期 caller。

### 4.1 Group-A code 路径

actor word 按无符号值与 7 比较。对 `code > 7`，令
`n = zero_extend(code) - 8`，父函数在 CALL 前形成：

```text
EAX = 3021 * n
ECX = 0x005029D0 + 0x2F34 * n
EDX = zero_extend(low16(dword_53CE94))
flags = SUB32(1008 * n, n)
```

乘法与地址运算按 32 位自然回绕。不能先用 canonical 数组长度拒绝 code；非 canonical
Group-A token 仍须携带完整 EAX/ECX/EDX/flags 进入 leaf，并在真实字段访问点停止。该顺序
也覆盖 EAX 高 16 位非零时的保持合同。

### 4.2 Group-B code 路径

对 `code <= 7`，父函数在 CALL 前形成：

```text
EAX = zero_extend(code)
EDX = 1381 * zero_extend(code)
ECX = 0x00525508 + 0x2B28 * zero_extend(code)
flags = SUB32(24 * zero_extend(code), zero_extend(code))
```

地址 LEA 不修改最后一次 SUB flags。leaf 只替换 AX，并保持 ECX、EDX 与这些 flags。

### 4.3 成功返回与停止后缀

成功返回后父函数执行 `test ax,ax`。零值递增 inner 计数；非零值跳过该递增。随后才可能
推进 outer、更新 `dword_53CE94`、调用 `sub_46E1E0`、改写脚本 cursor、清扫描全局并返回
1。

leaf 任一 typed-stop 都必须抑制 `TEST AX`、inner/outer 循环后缀、可选 page-load CALL、
cursor 更新、全局清零与 case 返回后缀。停止前已经完成的脚本列表扫描和 `word_d` 计数
保持提交。

## 5. 双向追溯与测试范围

LST 到 C++ 已覆盖唯一 16 位读取、AX 低字替换、普通 RET、EAX/ECX/EDX、ESP/EIP、flags
和两个访问阶段。C++ 到 LST 反向追溯覆盖 canonical resolver、请求、trace、唯一物理
caller、无符号 actor code 地址算术、post-call `TEST AX` 及父级 typed-stop；没有无来源
字段写或延期 caller。

汇编独立测试向量覆盖：

- Group-A 与 Group-B canonical owner；
- 字段值 `0/1/0x7FFF/0x8000/0xFFFF`；
- EAX 高 16 位保持、ECX/EDX 和全部入口 flags 保持；
- 字段读取与 RET 两处 typed-stop；
- `0x0046D429 -> 0x0046D42E` 的物理 CALL/return/actor trace；
- Group-A `3021*n`、outer EDX 与 `SUB32(1008*n,n)`；
- Group-B `1381*code` EDX 与 `SUB32(24*code,code)`；
- 非 canonical Group-A code 在 leaf 字段访问点停止；
- 零值触发 page-load、非零值跳过及停止后的完整父级后缀抑制；
- 生产 generic/raw `0x00478AB0` 调用归零。

## 6. 动态差分状态

当前缺少原版完整 Group-A/Group-B actor backing、`+0x2A76` 异常字段页、RET 异常栈页、
寄存器/flags 以及 case 61 caller/callee 联合捕获后端。原版动态差分登记为
`blocked_runtime_oracle`，不得写成 `original_diff_verified`。该阻塞不改变静态 LST 范围、
AX 低字替换、两个访问阶段或唯一物理 CALL 身份。

## 7. 验证与关闭

最终定向战斗测试静默通过；AddressSanitizer/UBSan 与 Linux core 均为 `199/199`，
Linux app 为 `205/205`，连续十轮 Linux core 均为 `199/199`。最终日志扫描没有
OpenSWD3 源码 warning、编译 error、测试失败、sanitizer finding 或 runtime error。

新文件全量 clang-format、旧文件 changed-range clang-format、`git diff --check`、生产
raw/generic 调用、TMP 分类和完整 release diff 审计均通过。未启动原版或 OpenSWD3
游戏程序。

LST 地址摘录 SHA-256 为
`4d42108a6182e1db3d462727e44757f350239d9dc109c45cf76c99f0f7dcbdd1`。inventory 生成器
连续双跑逐字节一致，关闭结果为
`308/422 = 298 platform_adapted + 10 assembly_exact + 114 pending_audit`，inventory
SHA-256 为 `e9336855b4153ed9cfaf00e7541b5ed9f2048b5c1623d122e22a88b0d9a2ce25`。
下一条 `pending_audit` 为 `audit_order=309 / 0x00478AC0 / sub_478AC0`。
