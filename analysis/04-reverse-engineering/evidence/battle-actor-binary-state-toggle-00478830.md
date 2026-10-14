# 战斗角色二值状态切换（0x00478830）

## 1. 范围与边界

权威 LST 把 `sub_478830` 锁定为 `0x00478830..0x00478843` 共 20
字节、6 条实际指令、0 个 callee、0 个跳转和 1 个 `retn 4`，没有
外部 chunk、中段入口或隐藏尾部：

```text
00478830  mov  edx,[ecx+2AF8h]
00478836  xor  eax,eax
00478838  test edx,edx
0047883A  setz al
0047883D  mov  [ecx+2AF8h],eax
00478843  retn 4
```

入口 ECX 是 actor token。栈上有一个四字节参数，但函数从不读取参数值；
`retn 4` 仍同时消费返回地址和参数槽。

## 2. 精确机器语义

执行顺序不得折叠：

1. 从 actor `+0x2AF8` 读取完整 dword 到 EDX。
2. `xor eax,eax` 清完整 EAX。
3. 对旧 dword 执行完整 32 位 `TEST EDX,EDX`。
4. 旧值严格为零时 `setz al` 得到 EAX=1；任意非零值都得到 EAX=0。
5. 把完整 EAX 写回 actor `+0x2AF8`。
6. 从 `[ESP]` 读取返回地址并执行 `retn 4`。

这不是按位翻转。它把零规范化为 1，把任何非零 dword 规范化为 0。
正常返回时 EAX 是新值，EDX 是完整旧值，ECX 保持 actor token，flags 来自
`TEST EDX,EDX`，ESP 相对入口增加 8，EIP 为物理返回地址。显式参数值不进入
任何寄存器或内存读；结果只由旧字段值决定。

## 3. 三个访问点与部分提交

Typed 实现按真实访问顺序区分三个停止点：

- `value_read_typed_stop`：停在 `0x00478830`。字段、入口 EAX/EDX、flags、
  ESP 均保持，ECX 为 actor token。
- `value_write_typed_stop`：停在 `0x0047883D`。EDX 已取得旧 dword，EAX 已为
  0/1，TEST flags 已提交，字段尚未改写。
- `return_address_read_typed_stop`：停在 `0x00478843`。字段写入已提交；
  EAX/EDX/ECX 与 TEST flags 保持，ESP 未推进，父 caller 后缀不得执行。

正常结果记录字段读/写两项 actor access，只记录返回地址一次 stack 读取；未读取的
参数槽没有伪造 stack read，但正常 `RETN 4` 仍以 ESP+8 表达其消费。

## 4. Canonical owner 与平台适配

现代实现不建立平行 actor 数组：

- Group-A `+0x2AF8` 绑定
  `startup.party[index].progress.script_binary_state`；
- Group-B `+0x2AF8` 绑定
  `startup.group_b_lifecycle[index].action_configuration.script_binary_state`。

原程序可对任意裸 actor 指针访问 `+0x2AF8`。现代 C++ 只能解析已登记的
Group-A/Group-B 生命周期 owner；固定 token 非对齐、越界或 owner 不存在时，在
原字段读点 typed-stop，不提前加 actor-code 范围门、不回滚 caller 已发布前缀，也不
伪造邻接内存。因此本工作包登记为 `platform_adapted`。

## 5. 两个物理 CALL 与 opcode 29

完整 LST 只有脚本分派 `sub_469D20` opcode 29 的两处 CALL：

- Group-A：`0x0046B04E -> 0x0046B053`；
- Group-B：`0x0046B072 -> 0x0046B077`。

共同前缀先读取 u16 actor code、压入固定参数 1，并无条件把 actor code 发布到
共享 word。code `<=7` 走 Group-B，code `>7` 走 Group-A；地址算术分别为：

- Group-A：`0x005029D0 + 0x2F34 * (u16(code) - 8)`；
- Group-B：`0x00525508 + 0x2B28 * u16(code)`。

实现直接复用既有 `script_actor_address` 的 LST 算术。Group-A CALL 前 EAX 为
`1007*(code-8)`、ECX 为 actor token、EDX 保持 caller 当前值；Group-B CALL 前
EAX 为 `1381*code`、EDX 为 `345*code`、ECX 为 actor token。两支 flags 都保留
各自最后一次 SUB。CALL trace 独立保存 call/return、固定参数 1、actor token、
寄存器、flags、三个 typed-stop 与 leaf 部分提交。

Group-B leaf 完整返回后才递增共享 byte `0x0053BF02`，按 u8 回绕。两支随后共同把
script cursor 加 4、清 selected actor code word，并令父函数返回 EAX=1。leaf 任一
停止都保留已执行的 actor-code 发布与地址算术前缀，阻断 Group-B byte 递增和共同
后缀。code 18 与 `0xFFFF` 不提前拒绝：二者都先走 Group-A 地址算术与物理 CALL，
再在 leaf 原字段读点停止。

生产源码只保留 `0x00478830` typed leaf 地址身份和脚本 reserved 枚举；generic/raw
port 调用数为零。

## 6. 双向追溯与测试范围

LST 到 C++ 追溯覆盖完整 20 字节、完整 dword 读写、XOR、TEST、SETZ、未读取参数和
`RETN 4`。C++ 到 LST 反向追溯覆盖两个 canonical owner、三个真实访问点、两组
CALL/返回地址、actor-code 发布、Group-B byte 回绕、共同后缀和父级后缀抑制；没有
无法反查到 LST 或已登记平台适配的生产行为。

测试覆盖：

- Group-A/Group-B canonical owner 与非法 token；
- 旧值 `0/1/2/0x00010000/0x80000000/0xFFFFFFFF`；
- 完整 EAX/ECX/EDX、TEST flags、ESP/EIP、actor/stack access 与未读取参数；
- 字段读、字段写和返回地址读取三个 typed-stop 的部分提交；
- code `0/7/8/17` 两组合法端点、两次执行 `0->1->0`；
- Group-B byte 从 `0xFF` 回绕为 0；
- code 18 与 `0xFFFF` 在原字段读点停止；
- Group-A 写停止与 Group-B RET 停止后的完整父级后缀抑制；
- 两组物理 CALL/返回地址及 production raw 调用为零。

最终验证已通过补齐向量后的 Linux core `199/199`、AddressSanitizer/UBSan
`199/199`、Linux app `205/205`，以及连续十轮完整 Linux core，每轮均为
`199/199`。新文件全量与触碰 C++ 文件通过 clang-format Werror，最终日志零源码
warning、测试失败、sanitizer finding 或 runtime error；TMP 分类、production raw
调用归零和 unstaged release audit 均通过，未启动原版或 OpenSWD3 游戏程序。

inventory 生成器连续双跑逐字节一致；当前计数为
`304/422 = 294 platform_adapted + 10 assembly_exact + 118 pending_audit`，
SHA-256 为
`17532962784376fa50d3dac230c609ed9b30309425db0a7f92628acd75b3bd55`。

## 7. 动态差分状态

当前缺少原版完整 Group-A/Group-B actor、三个异常内存访问，以及两处 caller 联合
寄存器、flags 与 SEH 捕获后端。原版动态差分登记为 `blocked_runtime_oracle`，不得
写成 `original_diff_verified`。该阻塞不改变 20 字节 leaf、两处物理 CALL 身份和
当前静态收敛结论。
