# 战斗角色效果资源槽写入（0x004787D0）

## 1. 范围与边界

权威 LST 把 `sub_4787D0` 锁定为 `0x004787D0..0x004787E8` 共 25
字节、5 条实际指令、0 个 callee、0 个条件分支和 1 个 `retn 4`，没有外部
chunk、中段入口或隐藏异常尾：

```text
004787D0  mov  dx,[esp+4]
004787D5  xor  eax,eax
004787D7  mov  ax,[ecx+2A7Ch]
004787DE  mov  [ecx+eax*2+29C4h],dx
004787E6  retn 4
```

入口 ECX 是角色对象 token；唯一显式栈参数是 `[ESP+4]` 的低 16 位资源值。
`[ESP]` 是物理返回地址。

## 2. 精确机器语义

执行顺序不得折叠：

1. 从 `[ESP+4]` 读取一个 word 到 DX。EDX 高 16 位保持入口值。
2. `xor eax,eax` 把完整 EAX 清零并提交零结果 flags。
3. 从 `actor+0x2A7C` 读取一个 word 到 AX。因为 EAX 已清零，结果是 cursor
   的 u16 零扩展值。
4. 以该 EAX 作为 word 下标，把 DX 写入
   `actor+0x29C4+cursor*2`。函数不修改 cursor。
5. `retn 4` 从 `[ESP]` 读取返回地址，同时弹出返回地址与一个 dword 参数，正常
   返回时 ESP 增加 8。

正常返回时 EAX 是 cursor 的 u16 零扩展值，ECX 保持 actor token，EDX 高 16 位
保持入口值且低 16 位为资源值。EIP 取物理返回地址。

## 3. Flags 与部分提交

第一条 `MOV` 不改 flags。`XOR EAX,EAX` 清 CF/OF，设置 ZF，清 SF，按零结果设置
PF，AF 未定义。后续两条 `MOV` 和 `RETN` 都不改 flags，所以正常返回保留该 XOR
结果。

Typed 实现按真实访问顺序区分四个停止点：

- `argument_read_typed_stop`：停在 `0x004787D0`。入口 EAX、EDX、ESP、EIP 和
  flags 全部保持，零次 actor 访问。
- `cursor_read_typed_stop`：停在 `0x004787D7`。DX 参数读取与 EAX 清零已经提交，
  保留 XOR flags，尚未读取 actor。
- `target_write_typed_stop`：停在 `0x004787DE`。cursor 已读入 EAX，目标 token
  已按原公式形成，但目标 word 尚未写入。
- `return_address_read_typed_stop`：停在 `0x004787E6`。目标 word 已写入，返回
  地址未读取，ESP 未推进，EIP 不伪造为 caller 后缀。

正常结果同时记录参数、cursor、目标 word 和返回地址的物理 token、访问次数、
actor/stack 访问顺序、完整 EAX/ECX/EDX、ESP/EIP、flags 与 known 状态。

## 4. Canonical owner 与平台适配

LST 的角色布局使用 35 个连续 word 槽 `actor+0x29C4..actor+0x2A08`，cursor 位于
`actor+0x2A7C`。现代实现不建立平行数组：

- Group-A 解析到
  `LegacyBattleActionDispatchState::group_a_action_execution[index]` 的
  `effect_resource_slots` 与 `effect_resource_cursor`。
- Group-B 解析到
  `LegacyBattleStartupState::group_b_lifecycle[index].action_execution` 的同一组
  canonical 字段。
- 已经持有单个 Group-A actor 的 caller 直接借用该对象，不复制 owner。

原程序用裸 actor 指针和无边界下标直接读写。现代 C++ 没有可合法表达的相邻任意内存，
因此仅在原参数读取、cursor 读取、目标写入和返回地址读取点执行 typed-stop。cursor
`0..34` 保持原写入；cursor `>=35` 不夹值、不回滚前缀，也不创造扩展存储。这是本工作包
登记为 `platform_adapted` 的唯一 leaf 级差异。

相邻 `0x0047CEC0` 是 cursor/完成状态的独立更新者。每次已恢复 caller 在原调用时点
同步 canonical cursor：参数完整等于 `1` 时执行 u16 加一并在大于 34 时 clamp 到 34；
其他参数执行 u16 减一并保留零下溢为 `0xFFFF`。`sub_4787D0` 自身不吸收这项行为。

## 5. 二十七个已恢复物理 CALL

机器码扫描得到 40 处 `call sub_4787D0`。以下 27 处所属 caller 已关闭，并在原控制流
位置直接组合 typed leaf。括号内为 `CALL 地址 -> 返回地址`：

- `sub_4539B0` 一处：`0x00453B85 -> 0x00453B8A`。
- `sub_4576A0` 一处：`0x0045815A -> 0x0045815F`。
- `sub_4582B0` 两处：`0x00458C83 -> 0x00458C88`、
  `0x00458CC1 -> 0x00458CC6`。
- `sub_458DE0` 六处：`0x0045958C -> 0x00459591`、
  `0x004595DB -> 0x004595E0`、`0x0045971A -> 0x0045971F`、
  `0x00459769 -> 0x0045976E`、`0x00459879 -> 0x0045987E`、
  `0x004598C3 -> 0x004598C8`。
- `sub_45C010` 四处：`0x0045C9E7 -> 0x0045C9EC`、
  `0x0045CB3B -> 0x0045CB40`、`0x0045CD86 -> 0x0045CD8B`、
  `0x0045CF55 -> 0x0045CF5A`。
- `sub_45D690` 五处：`0x0045D6C7 -> 0x0045D6CC`、
  `0x0045D72F -> 0x0045D734`、`0x0045D744 -> 0x0045D749`、
  `0x0045D7B5 -> 0x0045D7BA`、`0x0045D7CA -> 0x0045D7CF`。
- `sub_469D20` 一处：`0x0046DB01 -> 0x0046DB06`。
- `sub_46EE60` 三处：`0x0046EEDF -> 0x0046EEE4`、
  `0x0046EF72 -> 0x0046EF77`、`0x0046EFFE -> 0x0046F003`。
- `sub_4731A0` 两处：`0x00473508 -> 0x0047350D`、
  `0x00473533 -> 0x00473538`。
- `sub_4758A0` 两处：`0x00475BEE -> 0x00475BF3`、
  `0x00475E25 -> 0x00475E2A`。

每个站点独立保存 CALL 与返回地址身份、actor token、入口 EAX/EDX、前驱 flags 和
known 状态。调用只在原门真实到达时追加 trace。leaf typed-stop 立即阻断原返回地址后的
资源处理、cursor 更新、字段清理、循环推进或父级后缀。

`sub_4582B0` 的两处 actor token 都来自保存原 `arg_0` 的 EDI；奖励发布使用的
`arg_4` object token 不是该资源槽 backing。`sub_4758A0` 的两个 Group-B 直接 actor
站点使用当前 actor 自身 backing，不借用 dispatch/startup 中其他 actor。

## 6. 十三个精确延期物理 CALL

以下 13 处 CALL 所属父函数仍是后续工作包。当前只登记物理地址和返回地址，不猜测未审
父函数的门、寄存器、flags、后缀或 owner：

- `sub_47E5C0` 两处：`0x0047E5D2 -> 0x0047E5D7`、
  `0x0047E5E8 -> 0x0047E5ED`。
- `sub_481010` 八处：`0x00481061 -> 0x00481066`、
  `0x0048130D -> 0x00481312`、`0x004818D2 -> 0x004818D7`、
  `0x004818F9 -> 0x004818FE`、`0x00481924 -> 0x00481929`、
  `0x0048194F -> 0x00481954`、`0x004819D1 -> 0x004819D6`、
  `0x004819E5 -> 0x004819EA`。
- `sub_481A40` 三处：`0x00481A6D -> 0x00481A72`、
  `0x00481EF8 -> 0x00481EFD`、`0x00481F19 -> 0x00481F1E`。

这些 caller 在各自工作包关闭时必须直接组合本 typed leaf，并恢复各物理站点的入口机器
状态、条件门、部分提交和返回后缀。延期站点不计入当前 27 个生产接入点，也不因地址清单
存在而伪装父函数已关闭。

## 7. 双向追溯与测试范围

LST 到 C++ 追溯覆盖 25 字节完整边界、word 参数读取、部分 EDX、XOR EAX、u16 cursor
零扩展、word 下标写入、`RETN 4`、四个故障点、部分提交、寄存器和 flags。C++ 到 LST
反向追溯覆盖 35 槽 canonical owner、Group-A/Group-B 解析、40 组 CALL/返回地址、27 个
已恢复站点、13 个延期站点、嵌套 trace、typed-stop 后缀抑制，以及每次
`0x0047CEC0` 后的 cursor 同步。

生产源码对 `0x004787D0` 只保留 typed leaf 地址身份和 script-dispatch reserved 地址
枚举；已关闭路径不再通过 generic opaque port 执行该函数。

定向测试覆盖：

- Group-A/Group-B canonical backing、非法 token、reset；
- cursor `33 -> 34`、34 clamp、普通减一和 `0 -> 0xFFFF` 下溢；
- 正常 slot 写、部分寄存器、两次 stack 读取、ESP/EIP、XOR flags；
- 四类 typed-stop 与每个停止点的已提交前缀；
- slot 34 有效、slot 35 在原目标写点停止；
- 40 组物理 CALL/返回地址恒为 `return = call + 5`；
- 27 个已恢复 caller 的到达门、trace、canonical slot、cursor 同步与父级后缀抑制。

最终验证结果：

- Linux core 完整门通过 `199/199`；
- AddressSanitizer/UBSan 完整门通过 `199/199`，无 sanitizer finding 或 runtime error；
- Linux app 完整门通过 `205/205`；
- 连续十轮 Linux core 均通过 `199/199`，且每轮
  `battle.legacy_battle_setup` 均通过；
- 新文件全量和历史文件 changed-range clang-format 均通过；
- 十轮 core 日志无 OpenSWD3 源码 warning、测试失败、sanitizer finding 或 runtime
  error；
- inventory 生成器连续双跑逐字节一致，最终计数为
  `302/422 = 292 platform_adapted + 10 assembly_exact + 120 pending_audit`，
  SHA-256 为
  `adeeb9eba674ec671ec51253acfda91f35d43be15135826a004f574400fab93e`；
- TMP 终检发现的三个历史 OpenSWD3 instance-lock 已迁入
  `build/tmp/migrated-system-tmp/workpack302-20260921/`，迁移后系统临时目录项目产物为零；
- 未启动原版或 OpenSWD3 游戏程序。

## 8. 动态差分状态

当前缺少原版完整 Group-A/Group-B actor、越界 actor 页、异常栈页，以及 40 处 caller
联合寄存器、flags 与 SEH 捕获后端。原版动态差分登记为
`blocked_runtime_oracle`，不得写成 `original_diff_verified`。该阻塞不改变 25 字节 leaf、
40 处物理 CALL 身份和 27/13 静态边界。
