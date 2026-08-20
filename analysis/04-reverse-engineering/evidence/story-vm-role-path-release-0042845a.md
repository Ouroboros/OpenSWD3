# 剧情 VM 角色路径释放 handler：0x0042845A

状态：`platform_adapted`、`assembly_exact`（有效内存/owner 域）、`unit_tested`、`real_asset_tested`、`sdl_runtime_integrated`；原程序动态差分仍为 `blocked_runtime_oracle`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x0042845A..0x004284C1`

opcode：`18`，C++ 语义常量 `OP_18_RELEASE_ROLE_PATH`

直接 helper：`sub_40C0D0`、`sub_42D920`

## 1. 编码、selector 与危险点

逻辑指令长度为4 bytes：

```text
+0 u16 opcode
+2 u16 role_selector
```

公共 fetch 的 `raw_word & 0x3FFF` 使 `0012/4012/8012/C012` 共用该入口。handler 把 selector 原样传给 `sub_40C0D0`，不做 `0xFFF0` source 替换；`0xFFFE` 仍由 helper 解析为 controlled role。

ordinary miss 会让 resolver output 成为 `0xFFFFFFFF`。原 handler 忽略 resolver bool，并把 output 传给 `sub_42D920`；该 helper 在 `0x0042D947` 首先按 role index 读取 role `+0x10`，因此 miss 在这里发生原始危险访问。现代保留 resolver output，并只在该 role dereference 点前 checked-stop `role_not_found`；invalid controlled index 由 VM 入口既有安全边界停止。

## 2. `sub_42D920` 返回合同

helper 先读取 role `+0x10` 的 bit31 path-ownership flag：

- bit31 已清：立即返回1，不读取 active slots，也不需要 pathfinding runtime；
- bit31 置位：扫描恰好72个 active-object slot，每槽 `0x21C` bytes；
- 匹配条件为 slot `+0x00` role index 相等，且 slot `+0x1B` low nibble 大于1；
- 72槽均不匹配时返回0；
- 匹配后，slot `+0x08` saved role 为 `0xFFFF` 时清空该 type>1 slot并返回1；
- saved role 非 `0xFFFF` 时恢复 slot 的 role/cursor/destination/type1 状态，并按保存目标重建路径，随后返回1。

现代 `complete_legacy_world_story_path` 原先在 helper 入口要求全部 pathfinding/camera/movement owner，早于原程序实际使用。本轮把检查延迟到 saved chained path 真正重建前：bit31 clear、无匹配槽、以及无 chained path 的 slot 清空只依赖 role 与72槽；只有 chained path 才要求 node pool、地图尺寸和 surface。这是 typed owner 的最小平台适配。

## 3. handler 两条路径与公共 join

`sub_42D920` 返回后，handler 无条件：

1. 把 role `+0x10` 与 `0x7FFFFFFF`，清 bit31；
2. 把 role record base `+0x84` 的 `u16` 置0，对应 `LegacyActionRecord::wait_remaining`（`0x4BAC2C - 0x4BABA8 = 0x84`）；
3. 设置 continuation 并进入公共 join。

helper 返回非0时，`0x0042847E..0x00428486` 同时把 context IP 与指令指针加4；公共 join 发布 previous18，并在同一次 VM 调用取下一 opcode。

helper 返回0时不推进 IP，但仍先清 bit31、清 wait 并发布 previous18。公共 join 同调用再次取到 opcode18；第二次 helper 因 bit31 已清立即返回1，于是推进4。故有效状态下 zero-return 路径恰好重执行一次，不会形成无限 busy loop。旧 C++ 缺少公共 join 的 previous18 发布，本轮已修复。

modern owner 缺失或 chained-path owner 失败只在原 helper 实际需要该 owner 的点 checked-stop；已清 bit31或无匹配槽不被无关 runtime 依赖阻塞。failure 不伪造清 flag、清 wait 或推进成功。

## 4. 测试与真实资产

synthetic 测试独立覆盖：四 raw alias、最小 owner 下 type2 slot 完成、direct helper 的 bit31-clear immediate return、bit31 已清且无 runtime、无槽 zero-return 单次重试、type1 不匹配、ordinary miss output `FFFFFFFF`、`0xFFF0` 不替换、invalid controlled-role 入口、匹配槽但 owner 缺失、chained-path owner failure，以及窗口 `0x7FFE` 短 selector。

`story-vm-talk-linear-records.tsv` 中 opcode18 共301条物理记录：`TALK1.DAT` 134条、`TALK2.DAT` 84条、`TALK3.DAT` 37条、`TALK4.DAT` 46条。全部 raw `0x0012`、decoded length 4、entry probe hit 1，共91个 selector。

真实回放使用：

```text
TALK1.DAT@0x00054136
12 00 0B 00 6F 00
selector = 0x000B
next opcode = 111
```

构造 GUID 11、bit31 已清且 wait 非0的 role，不提供 story-path runtime。opcode18 依原 helper immediate-return 路径清 wait、推进4、发布 previous18，并在同一次 VM 调用取到尚未恢复的 opcode111 后以 `unsupported_opcode` 停止。

定向 story-path owner、synthetic、real-suite、initial-session-real-suite CTest 为4/4；完整 Linux core 186/186、Linux app 192/192 均以 exit 0 通过。生成器 Python `py_compile` 通过且两次重跑幂等；两套真实 CMake 均已编译本轮全部 C++ 改动。按执行计划 v256，小 handler 不运行 Windows；Windows LLVM app 只在剧情 VM P3 大阶段完成时统一编译、集中修复。未启动任何游戏 EXE。

关闭后 workpack 为14/146。下一行严格是：

```text
0x004284C2
opcode 19
```

opcode19 尚未独立审计，常量保持无语义后缀 `OP_19`。
