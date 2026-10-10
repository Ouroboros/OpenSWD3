# 战斗角色目标选择计数查询（0x00478AB0）

状态：`platform_adapted`。查询直接借用实际 WORD；脚本父级及后续加载
仍有通用协议，未验收完整战斗接线或原版动态差分。

## 1. 完整 LST 与独立推导

完整主体为 `00478AB0..00478AB7`，半开范围至 `00478AB8`。
共 8 字节、2 条指令，无 callee、分支、外部 chunk 或中段入口。

```text
00478AB0  mov ax, [ecx+2A76h]
00478AB7  ret
```

MOV 只读取 WORD，替换 AX，保持 EAX 高 WORD、ECX、EDX 和 flags。
RET 读取返回地址。字段故障没有读取值；返回故障保留实际读取值。
没有字段写入、清计数、资源操作或业务成功值。

全 LST 唯一直接调用为脚本 case 61 的 `0046D429→0046D42E`。
字段 `+2A76` 的全部直接访问为 `00478AA0`、`00478AB0`、
`00478AF6`、`00478B03`。相邻递增和衰减仍为独立待迁移接口。

本批先独立记录完整叶、唯一调用方和测试向量，再审计现有 C++。
对应记录为 `build/tmp/runtime/target-count-query-direct-lst-analysis.md`。
既有 Workpack 308 测试和机械登记不替代当前语义审查。

## 2. 实际共享字段与语义结果

```text
Group A: action.group_a_action_execution[index].target_selection_count
Group B: startup.group_b_lifecycle[index].action_execution.target_selection_count
```

查询接收实际 `const u16*` 和原访问描述，返回状态与 `optional<u16>`。
无字段读取时值缺席；返回故障时值存在；正常完成时值为实际完整 WORD。
重复查询重新读取字段，不建立查询缓存、第二套角色或写回镜像。

已删除查询专用 owner/view/resolver、地址常量、actor token、寄存器输入
和结果、flags/ESP/EIP、字段及栈读计数、CALL/return/actor 数组、请求
数组、执行器和脚本转发函数。接口不再依赖计数递增协议。
父结果只保留一次实际查询的可选语义结果；缺席表示没有执行。

空指针或不可读字段仍在原字段读取点停止，不制造零值。返回访问描述
保留原 RET 故障边界；没有新增停止。C++ 不模拟任意原地址解引用、
CPU 栈页故障或 SEH，因此保留 `platform_adapted`。

## 3. 唯一调用方的顺序与结果消费

`0046D374..0046D3AD` 每次进入先写扫描 WORD 为 1、数量 WORD 为 0，
再清共享帧门 `4A7B58`。逐个读取脚本 WORD；非 FFFF 项才递增并发布
两个 WORD。读取故障保留已发布的部分扫描前缀，不先合并到局部总数。
`0046D3AF..0046D3C0` 清两个查询游标；零项直接完成，不查询或加载。

每个角色先读取代码，再于 `0046D3D9` 清同一 `action.frame_enabled`，
于 `0046D3E3` 写 `packed_actor_state` 高 WORD，保留其低 WORD。
这两项均在实际查询之前；没有脚本专用帧门或角色镜像。

代码无符号 `0..7` 借用敌方字段；代码 `>7` 零扩展后减 8，借用队员
字段。代码未映射时先保留实际地址计算与共享写入，再到原字段点停止；
不提前新增角色代码错误路径。

```text
Group A: n = zero_extend(code) - 8
EAX = 3021*n; ECX = 005029D0 + 2F34*n; EDX = current WORD cursor
flags = SUB32(1008*n, n)

Group B:
EAX = zero_extend(code); ECX = 00525508 + 2B28*code; EDX = 1381*code
flags = SUB32(24*code, code)
```

未迁移父级在本地保留这些真实前驱。字段故障保持前驱 EAX；返回故障
只替换其低 WORD；两个故障均阻断 TEST、游标递增、加载和清理后缀。
正常消费直接对实际 WORD 执行 `TEST AX` 的零判断，8000、FFFF 均非零。
只有零值增加通过 WORD；扫描游标仍逐项增加，WORD 回绕保持。

所有值为零时 `0046D477..0046D48F` 先把 cursor 推进 `count*2+4`，
读取后续 signed WORD，再执行既有加载。该边界在父级局部重建实际
signed EAX、零扩展 count ECX、原 cursor EDX 和 XOR flags。
加载失败保留已推进 cursor，抑制正常后缀。

正常加载后 `0046D49A` 重读共享数量；完成比较也读取实际通过 WORD。
相等才清数量和两个游标；不等则清两个游标、按当前 cursor 加
`count*2+8`，保留数量。空列表数量和通过 WORD 同为 0，保持 cursor，
不读取或加载终止字后的内容。原不对称分支没有合并。

## 4. 双向追溯与独立向量

叶的两个原访问阶段逐项映射至直接读取和原返回访问状态；接口所有
值都有实际字段来源。无寄存器回声、成功常量、读取计数或无来源写入。
唯一调用方的列表初始化、部分发布、双方选择、访问故障、WORD TEST、
通过与扫描游标、加载前缀和加载后重读均回查上述 LST 地址。

独立测试覆盖：

- WORD 值 0、1、2、1234、7FFF、8000、FFFF；相邻字段保持。
- A2/B3 实际字段独立、索引 0 哨兵、重复读取后的共享修改。
- 字段故障无值、返回故障保留值、空字段原停止边界。
- 脚本代码 0/7/8/17 与五种计数值，重新进入时重置旧扫描状态。
- 空列表不查询、不加载；无 FFFF 时保留逐 WORD 扫描前缀。
- 零队员与非零队员/敌方，完整 WORD 判定与加载差异。
- 两种查询故障保留数量、游标、帧门及真实前驱并阻断后缀。
- 混合列表先读敌方，再遇不可用队员；保留第一项的实际通过进度。
- 不可用代码 8000 保留非零高 WORD 前驱 `05E62198`，没有提前拒绝。
- 加载收到 signed 后续 WORD；回调改变数量与角色后正常后缀重读数量，
  不回写旧角色值或再次清掉后续帧门写入。

## 5. 当前验证

最终 core setup 1/1 通过（12.50秒），ASan setup 1/1 通过（18.73秒），
SDL 应用编译链接通过。三个最终日志无 warning/error、测试失败或
sanitizer finding；七个当前源码/测试 SHA256 与启动验证时身份匹配。

```text
./build.sh core --build-target openswd3_battle_legacy_battle_setup_tests --test --test-regex '^battle\.legacy_battle_setup$'
./build-asan.sh --build-target openswd3_battle_legacy_battle_setup_tests --test --test-regex '^battle\.legacy_battle_setup$'
./build.sh app --build-target openswd3
```

日志为 `build/tmp/runtime/target-count-query-direct-{core,asan,sdl}.log`。
身份为 `target-count-query-direct-validated-source.sha256`。
当前 gate 受管进程 `proc_aa78` 退出0；格式和 whitespace 检查通过。
源代码、完整 LST 与当前独立向量逐项复核，未增加资源或业务停止。

初轮 setup 栈溢出由 ASan 定位到原大型测试函数，尚未进入新增断言；
拆成独立测试函数后全部原断言及新增向量均执行通过。初轮日志保留为
`target-count-query-direct-before-test-split-core.log` 与
`target-count-query-direct-asan-diagnosis.log`，不计入本批通过证据。

历史 Workpack 308 的 199/199、205/205、十轮 core 和旧 inventory
SHA 仅为历史记录，不能证明本批源码。没有运行游戏程序。

## 6. 保留的未完成范围

计数递增、衰减、脚本父级的其他请求/回复、操作表、参数数组、计数及
后续加载 Port 仍待迁移。闭合查询不提升这些接口、B11、Workpack
316/318、完整 SDL 战斗流程或真实资源生产接线。

原版联合 actor backing、字段页/栈页异常、寄存器及 SEH 捕获尚缺，
动态差分仍为 `blocked_runtime_oracle`。静态审查与 UT 不冒充原版差分。
全项目候选范围未核定穷尽；本叶迁移不是全项目完成条件。
