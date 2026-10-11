# 战斗角色状态高位清除（0x00478770）

## 原版范围

权威源为 `swd3.exe_export_for_ai/swd3.exe.lst`。
`sub_478770` 的完整主体为 `0x00478770..0x0047877A`，共11字节、
2条指令、无子调用、无分支，只有一个普通 RET。

```text
00478770  81 A1 B8 26 00 00 FF FF FF 7F  and dword ptr [ecx+26B8h],7FFFFFFFh
0047877A  C3                             retn
```

读取完整 DWORD，清除 bit31，原样保留低31位并写回同一字段。
即使高位已经为零，也必须经过字段读、字段写和返回地址读取。
字段读失败不写入；字段写失败不提交结果；RET 失败保留已经写入的字段。
没有分配、释放或资源所有权转移。

## 共享数据与当前接口

`clear_legacy_battle_actor_field_26b8_high_bit(u32*, MemoryAccess)`
直接借用共享字段，返回值只包含完成或三类访问失败状态。
已删除叶函数的 token resolver、寄存器请求与回复、地址常量、
字段及栈访问轨迹、调用次数，以及无调用方的 pending clear 转发协议。

字段只有一份存储：

- Group-A：`LegacyBattleActionDispatchState::group_a_action_execution[index].field_26b8`。
- Group-B：`LegacyBattleStartupState::group_b_lifecycle[index].action_execution.field_26b8`。

真实父调用借用 `actor.actor.action_execution->field_26b8`。
置位和查询操作继续访问同一字段，没有新增缓存、镜像数组或 token 槽。
完整 LST 中的字段访问位置为 `00478770`、`0047878C`、`004787AE`、
`004787C0`、`00478CA5`、`0047D51E` 和 `0047E6CF`。
低31位的更具体业务含义尚无充分证据，因此保留中性字段名。

## 唯一实际调用

完整 LST 中只有 `00478CC8` 调用该函数，属于 `sub_478B60`。
调用顺序如下：

```text
00478CA5  test dword ptr [ebp+26B8h],80000000h
00478CAF  jz   loc_478CD4
00478CB3  call sub_47BA80
00478CB8  cmp  eax,ebx
00478CBA  jnz  loc_479845
00478CC0  mov  ecx,ebp
00478CC2  mov  [ebp+2AC4h],edi
00478CC8  call sub_478770
00478CCD  pop  edi
00478CCE  pop  esi
00478CCF  pop  ebp
00478CD0  pop  ebx
00478CD1  retn 4
```

父入口将 EBX 设为1、EDI 设为0。只有高位置位且 `sub_47BA80`
返回1时，先清 `+2AC4`，再调用清高位函数。
非1返回值不会触及这两个字段；叶函数失败保留之前的 `+2AC4` 写入，
阻止父函数从 `00478CCD` 开始的退栈和双侧 frame 后续操作。

当前父实现已在该位置直接调用共享字段函数。结果使用 `optional`，
未执行时不生成成功结果；双侧 frame 直接传播存在的结果，
不再通过 callee 编号查找清高位记录或累计其调用次数。

## 尚未迁移的父接口

`sub_478B60` 及双侧 frame 的完整通用接口仍待迁移。
父函数其他调用仍保留 `physical_calls` 和停止控制。
清高位操作已删除物理调用记录及按通用调用序号停止的人工检查，
保留字段读、字段写和RET实际失败边界；不接受父函数的整体迁移。

原版叶函数保持 EAX、ECX、EDX；成功 RET 令 ESP 增加4。
AND 成功后 CF/OF 清零，ZF/SF/PF取决于结果，AF未定义。
原调用之后没有消费这些 flags 的业务条件。
为保持未迁移父接口的现有报告，父层暂时保留寄存器、失败位置和 flags：
读写失败保留 CMP 的状态，RET 失败保留已完成 AND 的状态。
这些机器状态不再进入叶函数的参数或结果。

## 验证与限制

独立 LST 向量覆盖8种 DWORD 输入、重复清除、
全部8种访问许可组合、空字段、实时重读及不同字段隔离。
父测试覆盖双侧角色的6个失败前缀，以及 readiness 为0、2、FFFFFFFF
时不执行的3个分支；双侧 frame 测试检查成功、未执行及失败后的后缀抑制。
查询测试确认其共享字段能立即观察直接清位结果。

setup和actor_frame_316在core与ASan下均各1/1通过，SDL实际链接通过。
这些门在拆分批次之前已执行完毕；先独立发布显示字段修正，再恢复本批补丁。
恢复后36份源码测试的SHA256与此前通过的完整验证输入逐字节一致，
没有把另一版本的测试结果用于当前源码，也没有重复运行无新增风险的门。
首个成功高位清除用例将剩余通用调用的停止序号设为1，
证明清高位已经不受该人工调用编号检查控制。

验证记录存放于`build/tmp/runtime/published-repair-combined-validation/`，
文件前缀为`action-mode-published-repair-`，后缀为`core.log`、
`callers-core.log`、`asan.log`、`callers-asan.log`和`sdl.log`；
源码身份清单为同前缀的`validated-source.sha256`。
两种setup构建各报告既有outcome-resolution测试的u16到u8转换warning，
该文件未修改；没有测试失败或sanitizer错误。

原版异常内存页、异常栈页及 SEH 联合捕获仍缺失，
动态差分状态为 `blocked_runtime_oracle`。
静态 LST 追溯和宿主测试不等于原版异常动态差分，也不证明整个父调用链已迁移。
