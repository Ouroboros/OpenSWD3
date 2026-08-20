# 世界地图运行状态清理（`0x00425B50`）

状态：`assembly_exact`、`unit_verified`、`platform_adapted`、
`sdl_runtime_integrated`；尚未 `original_diff_verified`

本文只以 `swd3.exe.lst` 的机器码和指令为行为真值。`sub_425B50` 的完整物理范围是
`0x00425B50..0x00425BDA`，无栈参数，保存 `ESI/EDI`，以 plain `retn` 返回。调用点为
`sub_40F160:0x0040F1C4` 和 `sub_424B90:0x00424ECF`；最后一次 `sub_40DD40` 使正常路径
`EAX` 为 `0xFFFFFFFF`，但两处调用者都忽略该值，不形成业务返回合同。

## 1. 固定物理顺序

函数没有入口门，严格执行以下阶段：

1. 依次把 `dword_4B7948`、`dword_4B794C` 传给 `sub_4885A0`；即使为空也照常调用；
2. 从 `0x004B7930` 起清零 25 个 dword，并把 `dword_49E0C4` 清零；
3. 扫描从 `0x004BABA8` 起的全部 256 个 `0xD8` 角色记录；每项先检查 `+0x38`，非零则
   释放该 payload，随后立即清零当前完整记录，再进入下一项；
4. 从 `0x004B72A0` 起把 54 个 dword（一个完整 `0xD8` 无角色 sentinel）写成
   `0xFFFFFFFF`；
5. 从 `0x004AD490` 起按 `0x21C` 步长调用 `sub_40DD40`，严格重置 72 个对象槽。

角色阶段与 `sub_40F3B0` 不同：`sub_425B50` 清零后不会调用 `sub_40DC00` 重建
`+0x40` action。旧 SDL 新游戏清理误用了 `reset_legacy_world_role_table`，从而留下 action
哨兵；现改为 `clear_legacy_world_role_table`，保留“释放当前 `+0x38` → 立即整项清零”的
物理顺序。

## 2. 现代所有权映射

原 25-dword 地图块和两个裸分配由 `LegacyWorldRenderSession`/`optional` 聚合拥有；reset
通过 RAII 一次释放，无法也无需保留两个裸 `free` 调用。角色 vector 也嵌在该 session
中，因此 SDL 在销毁聚合 owner 前先执行角色 payload/记录 clear；这是避免悬空 span 的
最小平台顺序适配。

原全 `0xFF` 无角色记录由受检空索引/null owner 代替，不向业务层暴露可解引用 sentinel。
72 个 `0x21C` 槽在现代 owner 中拆成 64 个 active slot 与 8 个 party slot；
`LegacyWorldObjectSlot` 默认构造及 `reset_legacy_world_object_slot` 均写满 `0xFF`，而
`LegacyWorldFrameCoordinatorState` 重建同时覆盖这两组。有效状态的释放、清零和后续地图
物化结果不变，因此 closure disposition 为 `platform_adapted`。

## 3. 验证

role-lifecycle UT 独立固定：

- 256 项全部扫描和清零；
- 只对非零 `+0x38` owner 释放 vector 容量；
- 清理后 action 字节保持全零，不继承 `sub_40F3B0` 初始化；
- 超过 256 项的现代 span 在任何写入前隔离。

既有 object-slot UT 固定 `0x21C` 全 `0xFF`，新游戏/世界会话门覆盖 RAII session 释放和
64+8 槽重建。role-lifecycle、role-transfer、new-game transition 与 runtime-session
synthetic/real 五项定向 CTest 通过；Linux core `185/185`、Linux app `191/191`、Windows
LLVM app `191/191` 完整门禁通过，两端应用成功链接且未启动游戏 EXE。原程序释放调用
动态差分仍等待用户 oracle。
