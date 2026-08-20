# 剧情 VM 同文件绝对跳转 handler：0x00428310

状态：`platform_adapted`、`assembly_exact`（有效 I/O 域）、`unit_tested`、`real_asset_tested`、`sdl_runtime_integrated`；原程序动态差分仍为 `blocked_runtime_oracle`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x00428310..0x00428317`

opcode：`15`，C++ 语义常量 `OP_15_JUMP_SAME_FILE_OFFSET`

直接 helper：`sub_42E430`

## 1. 编码与入口

指令固定 6 bytes：

```text
+0 u16 opcode
+2 u32 target_data_offset
```

`mov ecx,[ebx+2]` 直接读取 little-endian、可非对齐的完整 `u32` target；它不做符号扩展，也不把 target 解释成相对位移。公共 fetch 的 `raw_word & 0x3FFF` 使 `000F/400F/800F/C00F` 共用该入口。

少于6 bytes 时现代在原 `u32` 读取危险点返回 `operand_out_of_range`，不 service audio、不改 context offset、不调用 loader、不发布 previous。

## 2. `sub_42E430` 与共享 call-site

入口把 target 放入 ECX 后跳到 `loc_42CCD5`。共享 call-site 把 target 与 context 传给 `sub_42E430`；helper 的顺序是：

1. `_AIL_serve()`；
2. context `+0x14 = target_data_offset`；
3. target 加 `0x200`，在当前已经打开的 TALK 文件内绝对 seek；
4. context `+0x20 = 0`；
5. 从该位置读取最多 `0x8000` bytes 到共享窗口，不预清窗口；
6. 固定返回1。

helper 返回后 caller 重新取得共享窗口首地址，把 `ESI` 固定为1并进入公共 join。公共 join 发布 `previous=15`；由于 `ESI=1`，它直接回到 `loc_427B40`，在同一次 VM 调用中从新窗口 offset0 fetch。它不会在 handler 末尾再次 service audio，也不会 yield。

现代用 `load_data_window(current_file_number, target, window, false)` 复用 typed owner：resource loader 自身按原协议增加物理 `0x200`，不清窗口，短读仍为 ready。成功后同步 `loaded_file_number`/`loaded_data_offset`，发布 `previous_opcode=15` 并 `continue`，因此下一 opcode 的结果覆盖本 handler 的临时 opcode，但 executed count 包含两条。

## 3. I/O 失败平台适配

原 `sub_42E430` 忽略 seek/read 返回值，失败后仍发布 previous 并从可能残留或部分覆盖的共享缓冲继续 fetch。现代不得把失败伪造成成功：loader 返回非 ready 时，在禁止无效窗口 fetch 的 checked 边界返回 `load_failed`。

失败适配保留此前已经发生的原顺序副作用：一次 audio service、context data offset 改为 target、instruction offset 清零、loader 调用以及 `previous_opcode=15`；随后把 `window_loaded` 清为 false，避免读取旧窗口。恢复后的下一 VM 调用会按 context target 重新加载。该差异只约束无效 I/O 域，valid-domain 控制流与 LST 一致。

## 4. 测试与真实资产

synthetic 测试覆盖：四 raw alias、完整 `u32` target、当前 file number、`clear_before_read=false`、audio→load 调用顺序、previous publication、same-call 执行目标窗口 opcode、loader 失败与窗口尾 `0x7FFA`/`0x7FFC` 边界。

`story-vm-talk-linear-records.tsv` 中 opcode15 共 68 条物理记录：

- `TALK1.DAT` 51 条；
- `TALK2.DAT` 3 条；
- `TALK3.DAT` 6 条；
- `TALK4.DAT` 8 条。

全 68 条均为 raw `0x000F`、decoded length 6、entry probe hit 1；共有48个 target，范围 `0x00003F16..0x0004EEF3`，全部 target `+0x200` 落在所属 TALK 文件内。目标首 opcode 分布包含 `21/29/50/52/59/76/91/133/161/1026`。

真实回放使用 `TALK1.DAT@0x8A85` 的 `0F 00 CF 88 00 00`，经真实 resource database 在同文件加载 data offset `0x88CF`；目标窗口首条为 opcode59/sound `0x003A`。单次 VM 调用执行 opcode15 后立即执行 opcode59并 yield，固定 audio、target、IP4、previous15 与 sound request。定向 synthetic、real-suite、initial-session-real-suite CTest 为 3/3；完整 Linux core 186/186、Linux app 192/192、Windows LLVM app 192/192 均以 exit 0 通过，且未启动任何游戏 EXE。

关闭后 workpack 为 11/146。下一行严格是：

```text
0x00428318
opcode 16
```

opcode16 尚未独立审计，常量保持无语义后缀 `OP_16`。
