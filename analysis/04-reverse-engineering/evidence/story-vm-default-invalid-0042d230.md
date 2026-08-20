# 剧情 VM 默认非法 handler：0x0042D230

状态：`platform_adapted`、`assembly_exact`（有效内存域）、`unit_tested`、`sdl_runtime_integrated`；原程序动态差分仍为 `blocked_runtime_oracle`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x0042D230..0x0042D24D`

显式工作包 opcode：`0`

同入口默认范围：`194..1023`、`1027..16382`

## 1. 分派边界

取指在 `0x00427B40` 读取 u16，并于 `0x00427B4F` 执行：

```text
effective_opcode = raw_word & 0x3FFF
```

因此 opcode 0 的四个 raw alias `0000/4000/8000/C000` 都进入本组。

默认范围由两段原算术得到：

- `0x00427B7C..0x00427B82`：opcode 0 的 `(opcode-1)` 无符号越界进入 default；
- `0x0042AD92..0x0042ADAA`：`194..1023` 超过 94 项次表；
- `0x0042D219..0x0042D22E`：排除 1025、1026、16383 后，`1027..16382` 进入 default。

`1..193`、1024、1025 是原版显式 handler/special，即使现代尚未实现，也不得误走本组；1026 与 16383 已有独立 modern case。

## 2. handler 指令顺序

`0x0042D230..0x0042D249` 的顺序固定为：

1. `push 0`；
2. `MessageBeep(0)`；
3. 读取 `[0x004CF6D8]` 的上一次有效 opcode；
4. 读取本轮在 fetch 阶段保存的当前有效 opcode；
5. 把 previous/current 与固定格式/缓冲区送入 `nullsub_1` 诊断路径；
6. 跳入 `0x0042B09D`，随后到公共 join。

`MessageBeep` 返回值不读取。`nullsub_1` 没有业务副作用，因此现代不伪造日志 callback；`LegacyWorldStoryVmResult` 只暴露 diagnostic count/current/previous，供测试与诊断观察。

## 3. 本组经过的公共尾

默认入口的两个 continue 来源在 fetch 时都为零：

```text
ESI = 0
local_var_28 = 0
```

`0x0042B0AE..0x0042B0C8`：

1. 读取当前有效 opcode；
2. 计算 `local_var_28 | ESI`；
3. 把当前 opcode 写回 `[0x004CF6D8]`；
4. 因结果为零，跳到 `0x0042D4D7`；
5. 直接调用一次 `_AIL_serve@0`；
6. `EAX = 1`，从 `0x0042D4F3` 返回。

本组不修改 `[state + 0x20]` 或当前机器指针，所以后续帧会再次读取同一 raw word。不能把非法值改成 NOP、自动跳两字节、异常终止或同帧 busy loop。

其他 handler 是否以及何时经过同一公共 join 仍按其各自 P2 行审计；本组只关闭 default 路径对 previous/audio/yield 的使用，不据此把 `common_join` runtime path 整体标记完成。

## 4. 现代实现

现代 `step_legacy_world_story_vm` 增加原默认域谓词：

```text
opcode == 0
or 194 <= opcode <= 1023
or 1027 <= opcode <= 16382
```

只有这些值执行：

```text
ports.beep()
result.invalid_opcode_{current,previous}
state.previous_opcode = current
ports.service_audio()
status = yielded
return
```

`state.previous_opcode` 是原 32 位 `[0x004CF6D8]` owner，默认初始化为零；`sub_40E0B0` 没有该全局 xref，所以 `initialize_legacy_world_story_vm` 不重置它。SDL beep 保持与现有平台 beep owner 一致的 no-op 适配；`service_audio()` 复用实际 `LegacyAudioMaintenancePorts`，对应原直接 `_AIL_serve`。

现代受检 0x8000-byte window 在取指前隔离越界；合法域内的 handler 顺序、IP 不推进、previous 写回和 audio service 不变，因此分类为 `platform_adapted`。

## 5. 测试向量

`legacy_world_story_vm_test.cpp` 独立固定：

- opcode 0 的四个 raw alias `0000/4000/8000/C000`；
- 两个默认范围的四个边界 `194/1023/1027/16382`；
- beep → audio 外部可见顺序；
- diagnostic previous 在写回前读取；
- 连续两帧 `0x55 → 194 → 1023` 的 previous rollover；
- IP 始终不推进、每次只执行一条、每次 audio service 一次；
- `1/12/1024/1025` 这些显式但尚未实现的值继续返回 `unsupported_opcode`，不 beep、不 service、不更新 previous；
- VM 重初始化不清 `previous_opcode`。

story VM synthetic、real 与 initial-session-real 三项定向 CTest 为 3/3。最终完整门禁为 Linux core 186/186、Linux app 192/192、Windows LLVM app 192/192；三个构建测试进程均 lifecycle exit 0，且没有启动原版或 OpenSWD3 游戏 EXE。

## 6. 停止线

本组关闭后 workpack 为 1/146；下一组严格是共享入口：

```text
0x00427B8F
opcodes 1-6,89-90
```

该组包含八个变体和部分现代 case，必须整体重新审计，不能只延续当前已实现的 opcode 6/89。
