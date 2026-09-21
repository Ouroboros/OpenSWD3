# 战斗角色目标选择计数递增（0x00478AA0）

## 1. 范围、边界与 ABI

权威 LST 把 `sub_478AA0` 锁定为 `0x00478AA0..0x00478AA7`，半开区间为
`0x00478AA0..0x00478AA8`。主体共 8 字节、2 条实际指令、0 个 callee、0 个分支、
0 个局部标签和 1 个普通 `ret`，没有外部 chunk 或中段入口。

```text
0x00478AA0  inc word ptr [ecx+0x2A76]
0x00478AA7  ret
```

入口 ECX 是 actor token。函数没有显式参数。EAX、ECX 和 EDX 完整保持。普通返回时
RET 读取返回地址，ESP 相对入口增加 4，EIP 为物理返回地址。

唯一主体指令对 `actor+0x2A76` 执行 16 位 read-modify-write INC。值从
`0xFFFF` 自然回绕到 `0x0000`，不能改成无界、饱和或带符号加法。INC 更新 OF、SF、
ZF、AF 和 PF，同时保持入口 CF。

## 2. 原访问顺序与 typed-stop

现代实现把同一条物理 INC 拆成读访问和写访问两个 typed 阶段，再建模 RET 栈读取：

1. `count_read_typed_stop`：读取失败时零字段提交，入口寄存器与 flags 保持；
2. `count_write_typed_stop`：保留已观察的旧 word，但不提交新值，也不提交 INC flags；
3. `return_address_read_typed_stop`：字段写入和 INC flags 已提交，ESP 尚未推进。

正常路径记录旧值、新值、字段 token、一次读、一次写、一次返回地址读取、返回寄存器、
ESP/EIP 和完整 flags。写入值始终按 `u16(old+1)` 计算。

现代 C++ 不能合法解引用任意 32-bit actor token，也不能直接制造原 CPU 字段页或栈页
fault。canonical token resolver 和原访问点 typed-stop 是最小平台边界，因此本目标登记为
`platform_adapted`，不能以测试通过、字段 owner 或 span 检查标记为 `assembly_exact`。

## 3. 字段交叉引用与 canonical owner

完整 LST 中 `actor+0x2A76` 的全部直接访问为：

```text
0x00478AA0  inc word ptr [ecx+0x2A76]
0x00478AB0  mov ax, [ecx+0x2A76]
0x00478AF6  mov ax, [ecx+0x2A76]
0x00478B03  mov [ecx+0x2A76], ax
```

相邻 `sub_478AB0` 是普通 word getter。相邻 `sub_478AE0` 只在值大于零时递减该
word，同时独立处理 `+0x2A74` 并清 `+0x2AE0`。两个相邻函数仍属于后续独立工作包，
不扩入本目标。

字段采用中性名称 `target_selection_count`。它加入现有
`LegacyBattleGroupAActionExecutionState`，由以下 canonical 状态共用：

```text
Group-A  action.group_a_action_execution[index].target_selection_count
Group-B  startup.group_b_lifecycle[index].action_execution.target_selection_count
```

命名只表达三个 caller 已证明的行为：Group-B actor 被选中或被累计为 live target 时递增。
不从旧 raw 名称推导更多业务语义，也不建立第二套 actor 数组。

## 4. 三个已关闭父 CALL

完整 LST 只有一个父函数、三个物理 CALL，全部位于已关闭的 `sub_456680`。本轮没有延期
CALL：

```text
0x00456A1F -> 0x00456A24
0x00456CDD -> 0x00456CE2
0x00456D71 -> 0x00456D76
```

trace 逐次记录物理 CALL 地址、返回地址和 actor token。请求按真实执行序号消费，因此
`0x00456A1F` 在 Group-B 循环中重复执行时不会重复使用 request 0。

### 4.1 `0x00456A1F`

Group-B 扫描以 `0x00525508` 为首 token，每轮增加 `0x2B28`。对象非 terminal 且映射
为 `0xFFFFFFFF` 时，父函数先调用 `0x0047C660`，再以 ESI 恢复 ECX 后执行本 CALL。
MOV 不改 flags，因此 leaf 入口 EAX、EDX 和 flags 继承 `0x0047C660` 回复。

CALL 成功后父函数才递增当前 Group-A 的独立 progress dword。leaf typed-stop 必须阻断
该 progress 写、循环尾、余下对象、target preparation 和 frame 尾。该 Group-A progress
不是 `actor+0x2A76`，不能合并为同一 owner。

### 4.2 `0x00456CDD`

父函数从第一套 one-based Group-B 选择值 `n` 建立：

```text
EAX = 1381 * n
EDX = 345 * n
ECX = 0x005229E0 + 0x2B28 * n
    = 0x00525508 + 0x2B28 * (n - 1)
flags = SUB32(24 * n, n)
```

CALL 成功后重新读取 `n`，执行保持 CF 的 DEC，压入 `n-1`，再以当前 Group-A actor 调用
`0x00478A70`。leaf 的 EAX/EDX 和 INC 后 CF 进入后续 caller。typed-stop 阻断 DEC、目标
写入、availability 写、门清理和 frame 尾。

### 4.3 `0x00456D71`

第二套 one-based Group-B 选择使用相同地址算术和寄存器合同。CALL 成功后调用
`0x00478A70`，写 availability，执行 Group-A final processing，清选择门并更新 UI。
leaf typed-stop 阻断完整后缀。

三个 caller 均不再通过 generic port 调用 `0x00478AA0`。

## 5. 双向追溯与测试范围

LST 到 C++ 已覆盖唯一 16 位 RMW INC、普通 RET、u16 回绕、EAX/ECX/EDX、ESP/EIP、
INC flags 和三个访问阶段。C++ 到 LST 反向追溯覆盖 resolver、请求、trace、三个物理
caller、重复 CALL 序号及父级 typed-stop；没有无来源的字段写或延期 caller。

汇编独立测试向量覆盖：

- Group-A 与 Group-B canonical owner；
- 旧值 `0/1/0x7FFF/0x8000/0xFFFF`；
- `0x7FFF -> 0x8000` 的 OF/SF/AF/PF；
- `0xFFFF -> 0` 的 16 位回绕与 ZF/AF/PF；
- CF 保持及 EAX/ECX/EDX 不变；
- 字段读、字段写和 RET 三处 typed-stop；
- `0x00456A1F` 循环重复调用和两个 Group-B canonical 字段；
- `0x00456CDD` 与 `0x00456D71` 的真实地址、返回地址、token 和后续
  `0x00478A70` 顺序；
- caller 写停止时目标写入与后缀抑制；
- 生产 generic/raw `0x00478AA0` 调用归零。

## 6. Inventory 生成证明

`analysis/tools/build_battle_workpack.py` 对本目标的唯一登记为
`platform_adapted`，evidence 指向本文。生成器连续运行两次后，第二次输出与第一次保存的
TSV 逐字节一致；结果为 `307/422 = 297 platform_adapted + 10 assembly_exact +
115 pending_audit`。生成后的
`analysis/04-reverse-engineering/inventory/battle-function-workpack.tsv` SHA-256 为：

```text
4d285482fe5a5c218d62e0f07f923a3a1cf6bf8ca2484aafe4d5df05d297b372
```

下一条 `pending_audit` 是 `audit_order=308 / 0x00478AB0 / sub_478AB0`。

## 7. 最终验证

战斗定向测试入口成功返回；Linux core、AddressSanitizer/UBSan 与 Linux app 分别通过
`199/199`、`199/199` 和 `205/205`。随后连续执行十轮完整 Linux core，每轮均为
`199/199`。新文件全量与旧文件 changed-range clang-format Werror、
`git diff --check`、production raw token、inventory 幂等与 SHA、TMP 分类及发布前静态
审计均通过；十二份完整构建日志为零 OpenSWD3 源码 warning、测试失败、sanitizer
finding 或 runtime error。未启动原版或 OpenSWD3 游戏程序。

## 8. 动态差分状态

当前缺少原版完整 Group-A/Group-B actor backing、`+0x2A76` 异常字段页、RET 异常栈页、
INC flags 以及三个 caller/callee 联合寄存器与 SEH 捕获后端。原版动态差分登记为
`blocked_runtime_oracle`，不得写成 `original_diff_verified`。该阻塞不改变静态 LST
范围、16 位回绕、三阶段访问顺序或三个物理 CALL 身份。
