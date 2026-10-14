# 战斗角色动作与显示模式设置（0x00478710）

状态：`pending`。

## 1. LST 范围与行为

权威 LST 的 `sub_478710` 为 `0x00478710..0x0047876D`，共 94 字节，
没有子调用、外部 chunk 或隐藏异常尾；下一函数前的对齐从 `0x0047876E` 开始。
入口参数为完整 DWORD，按 `2、13、3、14、6、15` 比较，不能先截断为 WORD。

```text
00478710  mov eax,[esp+4]
00478714  cmp eax,2
00478719  cmp eax,0Dh
0047871E  cmp eax,3
00478723  cmp eax,0Eh
00478728  cmp eax,6
0047872D  cmp eax,0Fh
00478732  mov [ecx+2A6Ch],ax
00478739  mov eax,1
0047873E  retn 4
00478741  cmp eax,6
00478746  or byte ptr [ecx+2A87h],8
0047874F  or byte ptr [ecx+2A87h],40h
00478756  mov [ecx+2A70h],ax
0047875D  mov word ptr [ecx+2A6Ch],0
00478766  mov eax,1
0047876B  retn 4
```

- 默认路径只把参数低 16 位写入动作种类 `+0x2A6C`。
- `2、13、3` 先写显示种类 `+0x2A70`，再清动作种类，不访问模式 byte。
- `6` 先将模式 byte `+0x2A87` 与 `0x08` 按位或；`14、15` 使用 `0x40`。
  随后写显示种类并清动作种类。按位或保留原有其他位。
- 参数读取在字段访问之前，返回地址读取在全部字段写入之后。失败不回滚已提交写入。
- 原版参数读取失败保留入口 EAX；字段失败时 EAX 为完整模式参数；成功及 RET 失败时为 1。
  ECX/EDX 不变，正常 `retn 4` 使 ESP 增加 8。

原版默认出口 flags 来自最后一次 CMP；`2、13、3` 来自命中 CMP；
模式 byte 写入成功后来自 OR。`14、15` 的 OR 失败时仍为额外 `cmp eax,6` 的 flags。
这些是原版机器行为，不是当前叶函数的接口。

## 2. 直接函数与共享数据

`set_legacy_battle_actor_action_mode(view, mode, access)` 接收三个借用字段指针、
完整 DWORD 模式和既有访问失败条件，结果只包含完成或失败状态。
删除叶函数的寄存器式 request/reply、栈与访问轨迹、调用计数、结果数组和 token resolver。
各调用点在实际执行时直接绑定已有数据：

- Group-A 动作：`LegacyBattleActionDispatchState::group_a_action_execution[index].action_kind`。
- Group-A 显示与模式 byte：`LegacyBattleStartupState::party[index].item_effect_application`。
- Group-B 三字段：既有 lifecycle 元素的 `action_composition`。

不复制字段，不新增 actor 数组。默认路径允许显示、模式指针为空；
`2、13、3` 允许模式指针为空。仅在原指令需要访问字段时检查该字段。

六类原有失败状态分别是参数读取、模式读取、模式写入、显示写入、动作写入、返回地址读取失败。
访问顺序与部分写入保持不变；没有增加新的失败位置。
父调用方以 optional 区分未执行和实际结果。尚未迁移的父函数保留其已有返回合同，
仅在需要时从真实前序操作、模式及失败状态得到原版返回值，不再接受伪造的前序 EAX 输入。

## 3. 十一个父函数与四十一处 CALL

以下地址为 LST 追溯坐标，当前接口不再携带这些返回地址。

```text
sub_4539B0  453B15 453C93 453F8B 455053 4551C8 45550C
            4558F9 455951 455AE8 455C1B 455CBB
sub_455D60  456080 45652B
sub_456680  456BA6 456C8E 456E47
sub_4576A0  457C68 457CA0 457E34
sub_45ADF0  45AEEC
sub_466F70  4671F9
sub_469D20  46B550 46B5E5 46B6B7 46B75F 46B87E 46B900 46DCC9
sub_476160  4761BA
sub_4761D0  4761F4 476246
sub_476250  4762C3 4762DA
sub_480220  4803FA 48055A 48058C 48066B 480691 4809F9 480A16 480A53
```

主动作分支保留实际前序结果：动作 2 使用 debug 低字节或资源释放结果；
动作 3 使用前序清零值；动作 23 使用物品操作或文字输出结果；
动作 409 使用原阻断值、实际视觉准备结果或更新后的计分地址。
视觉准备条件在进入前取值，避免外部调用修改共享门后改变第二个条件。

动作 7 的 `0x00455507` 作用于敌方目标，`0x00456526` 作用于我方目标；
两侧均按 LST 绑定目标，保留来源角色和相邻角色。对手模式 300 写当前 Group-B 角色。
Group-A frame 保留设置后与 1 比较的控制流以及失败时的原返回值。
Group-B 连续设置 2、6 后仍得到模式 byte `0x88`、动作 0、显示 6；
设置 17 后，下游发布操作从共享字段观察到 17，后续完成重置才改变它。

动作后处理、消息角色循环和脚本中的设置仍在原控制流位置执行。
所有失败阻断原调用后的订单、清理、消息发布或脚本游标后缀。
三个 Group-B 组合函数把实际 optional 结果传回父层；`0x004761B5` 成功后才 OR `0x80`。
脚本的订单插入函数不读取 flags；后续实际 flags 条件使用前重新计算，
SDL 的脚本通用口仅透传该机器字段，不以设置函数的 flags 决定业务分支。
该父层通用接口仍在后续迁移范围内。

`sub_480220` 仍待整体审计。其现有八槽报告仅保留 `executed`、模式和真实入口 EAX，
frame 对已执行项按顺序直接设置相应角色字段；失败阻断后缀，不伪造未执行项。
本批移除了该报告中设置函数的 EDX/flags 负载，但不把八槽父协议或整个 actor-ready 宣称为已迁移。

## 4. 双向复核与定向验证

LST 到实现核对完整 DWORD 分支、三个共享字段、写入次序、全部失败出口及 41 处调用位置；
实现到 LST 核对每个实际分支、字段绑定、前序值、optional 传播和失败后缀。

叶测试包含 28 组模式（含 0–16、300、符号位、截断边界和特殊值的高位变体），
每组遍历全部 256 个初始模式 byte；另有 19 组访问失败或未使用字段条件，
以及空借用字段、实时重读和两角色隔离测试。
调用方测试保留组合函数、消息、后处理、脚本及双侧 frame 的实际状态断言，
增加动作 7 目标隔离、八槽有序设置及 RET 失败后缀、动作 2/409 的 15 组前序失败向量。
删除旧寄存器和调用数组断言，不用它们代替真实状态断言。

最终验证通过，日志位于 `build/tmp/runtime/`：

- `action-mode-direct-core.log`：core 的 `battle.legacy_battle_setup`，1/1。
- `action-mode-direct-callers-core.log`：core 的 `battle.actor_frame_316`，1/1。
- `action-mode-direct-asan.log`：ASan 的 `battle.legacy_battle_setup`，1/1。
- `action-mode-direct-callers-asan.log`：ASan 的 `battle.actor_frame_316`，1/1。
- `action-mode-direct-sdl.log`：SDL `openswd3` 实际链接成功。
- `action-mode-direct-gate-input-check.log`、`action-mode-direct-gate-output-check.log`：
  36 个源码/测试文件在验证前后保持相同 SHA-256。
- `action-mode-direct-validated-source.sha256`：最终验证文件清单。

最终执行脚本为 `run-action-mode-direct-final-gates.sh`，受管运行退出 0。
没有执行全量 core/app 测试，也没有启动游戏程序。

## 5. 未关闭边界

本批只关闭动作/显示设置叶协议及其调用点迁移。十一个父函数中的其他通用调用、
actor-ready 八槽报告和资源边界继续保留在全项目待迁移集合。
工作包 298 的分类为 `pending`，不提高 B10 或父工作包的完成数。

原版动态差分仍为 `pending`：缺少完整 actor、异常参数/字段/栈页和联合 SEH 捕获。
当前静态 LST 及定向测试证据不能冒充原版动态验证或全项目 INVOKE 清理完成。
