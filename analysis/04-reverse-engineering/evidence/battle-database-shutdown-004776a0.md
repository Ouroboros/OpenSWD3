# 战斗 MON/LEVEL 数据库会话关闭 `0x004776A0`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`。

## 1. 完整 LST 范围

权威函数为 `0x004776A0..0x004776EE`。从 `proc` 到 `endp` 共 42 个物理行、25 条实际指令、2 个静态 `call`、4 个条件跳转、2 个局部标签和 1 个返回点，无外部 `FUNCTION CHUNK`。

唯一直接 caller 位于已关闭的战斗全局重置 `0x0045B630`，调用点为 `0x0045BCF0`。caller 在停止全部 sample 和提交音频 stream 停用之后调用本函数；返回后再清重复全局 dword 与最终 6 dword 区域。

## 2. 状态映射

四个旧全局分别映射到既有唯一 typed owner：

- `dword_53D020`：`LegacyBattleMonDatabaseState::handle`；
- `dword_53CF40`：`LegacyBattleMonDatabaseState::open`；
- `dword_53D024`：`LegacyBattleLevelDatabaseState::handle`；
- `dword_53CF44`：`LegacyBattleLevelDatabaseState::open`。

完整交叉引用确认前一对由 `0x00476A80` 与 `0x00476DB0` 的 `MON.DAT` 读取路径建立和复用，后一对由 `0x00477290` 与 `0x00477400` 的 `LEVEL.DAT` 读取路径建立和复用。本函数不建立影子句柄或第二份会话门。

## 3. 条件关闭顺序

入口立即读取 MON 句柄。仅当句柄同时不等于 `0xFFFFFFFF` 且不等于零时：

1. 以该句柄调用 `CloseHandle`；
2. 等调用返回后才把 MON 句柄写成 `0xFFFFFFFF`。

随后无条件重新读取 LEVEL 句柄，并执行完全相同的双哨兵判断。LEVEL 关闭只能发生在 MON 分支完成或跳过之后，因此两个有效句柄的调用顺序固定为 MON、LEVEL。

`CloseHandle` 的布尔返回值不形成成功门。即使平台边界返回零，只要调用正常返回，对应句柄仍写成 `0xFFFFFFFF`。零句柄和全 1 句柄都不调用平台边界，也不改写原句柄值。

两个句柄分支结束后，函数无条件把 MON 与 LEVEL 的 `open` 会话门都清零。门值不参与是否关闭句柄的判断：入口门为零但句柄有效时仍关闭；入口门非零但句柄为哨兵时仍跳过关闭并在尾部清门。

## 4. 寄存器合同

函数保存并恢复 ESI、EDI，不修改 EBX。入口 EAX 在首条指令被 MON 句柄覆盖，因此不影响结果。

- 每次 `CloseHandle` 的 EAX/ECX/EDX reply 线程到下一段；
- 进入 LEVEL 段时，EAX 无条件被 LEVEL 句柄覆盖；
- LEVEL 调用发生时，正常返回 EAX/ECX/EDX 来自该调用；
- LEVEL 调用跳过时，返回 EAX 为 LEVEL 句柄，ECX/EDX 保留此前 MON 调用 reply 或原入口值。

因此“仅 MON 有效、LEVEL 为全 1”路径最终 EAX 为全 1，而不是 MON 的关闭返回值。typed 结果显式保留该覆盖关系。

## 5. caller 回收

`reset_legacy_battle_globals` 已删除旧 `initialize_post_reset_4776a0` opaque 调用，直接组合 `shutdown_legacy_battle_databases`。`LegacyBattleGlobalResetRuntimePort` 通过虚继承借用现有 MON owner，并新增对既有 LEVEL owner的同一虚基访问；关闭函数本身不持有状态。

caller 把前一音频 stream 停用边界的 EAX/ECX/EDX 作为本函数入口快照。数据库关闭结果在最终 6 dword 清零之前完成，之后 caller 固定返回 EAX 0、ECX 0，并保留数据库关闭后的 EDX。旧枚举槽只保留 reserved 身份，生产路径零调用。

## 6. 测试与动态差分

独立定向测试覆盖：

- 两句柄都有效的固定关闭顺序；
- 第二次调用继承第一次 reply 的 ECX/EDX；
- 零和全 1 两种哨兵的跳过与句柄原值保留；
- 仅 MON 有效和仅 LEVEL 有效；
- `CloseHandle` 返回零后仍写全 1 句柄；
- 入口 `open` 门与句柄有效性相互独立；
- 两门始终在正常尾部清零；
- 最终 EAX/ECX/EDX 覆盖关系。

战斗聚合测试另固定全局重置中的音频 stream→MON 关闭→LEVEL 关闭→最终 6 dword 清零顺序、旧 opaque 槽零调用和 caller 最终寄存器合同。

最终验证为独立数据库关闭测试、战斗聚合caller测试、Linux core `192/192`、完整AddressSanitizer `192/192`和Linux app `198/198`全部通过；最终日志没有OpenSWD3源码warning、测试失败、sanitizer finding或runtime error。新增及触碰C++文件通过clang-format `--dry-run --Werror`，`git diff --check`通过。inventory生成器连续双跑逐字节一致，工作包为`264/422 = 255 platform_adapted + 9 assembly_exact + 158 pending_audit`，SHA256为`6bdb54e72e0a8753fe10311e642484477cf6db4e297fa710958c3117361c89d2`。

当前缺少原版 Win32 句柄表、真实 `CloseHandle` 返回寄存器及已关闭全局重置其余尾部 callee 的联合捕获后端，`original_diff_verified` 登记为 `blocked_runtime_oracle`。
