# 剧情 VM 世界 session 同步重载：0x004286C5

状态：`platform_adapted`、`assembly_exact`（有效角色、地图与资源 owner 域）、`unit_tested`、`real_asset_tested`、`sdl_runtime_integrated`、`external_dependency_tested`；原程序动态差分仍为 `blocked_runtime_oracle`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

handler：`0x004286C5..0x00428712`

直接 helper：`sub_42E790`（`0x0042E790..0x0042E845`）、`sub_40C130`、`sub_40D200`。

opcode：`27`，枚举项 `OP_27_RELOAD_WORLD_SESSION`。

## 1. 14-byte handler 与边界修正

```text
+0   u16 raw opcode
+2   u16 logical map id
+4   u16 tile x
+6   u16 tile y
+8   u16 requested action id
+10  u16 requested base variant
+12  u16 requested variant delta
```

`0x004286C5..0x004286F0`按 `+12/+10/+8/+6/+4/+2` 的机器读取顺序把六项分别零扩展，并在最先压入的尾参数位置传 literal `1`。`0x004286F1`调用 `sub_42E790`；返回后同时推进保存的 IP 与窗口指针 14 字节，设置 `ESI=1`，再由 `0x0042870E` 的近跳进入公共 join。因此 `0x0042870E` 是本 handler 的最后一条机器指令，而不是下一个入口；下一个 handler 严格从 `0x00428713` 开始。静态 handler 提取边界据此修正为半开区间 `0x004286C5..0x00428713`。

公共 join 发布 effective opcode 27，并在同一次 VM 调用中继续取下一条。不能把世界重载改成异步请求，也不能在 handler 返回时无条件 yield。

现代 VM 在读取前验证完整 14-byte typed 窗口；截断记录返回 `operand_out_of_range`，不执行 helper 前置副作用。该检查是明确的平台安全边界，不改变有效记录行为。

## 2. `sub_42E790` 的精确前置顺序

`sub_42E790` 的可观察顺序是：

1. `dword_4B7920 = 0`；
2. 从 `dword_4B7CB0` 开始清 `0x80` 个 dword，即完整 `0x200` 字节工作区；
3. `dword_4B7518 = 0`；
4. `dword_4A948C = 0`；
5. `dword_4A9488 = 0`；
6. 对 `dword_4C9A18` OR bit 0；
7. 从 `dword_4AB378` 取得受控角色 index；
8. 依次解析后三个可继承参数；
9. 调用 `sub_40C130`；
10. 只清 `dword_4C9A18` bit 0，返回 `1`。

SDL owner 将这五段清理分别绑定到 camera-y transition、规范化输入记录、camera-x transition、player-y transition 与 player-x transition，并按上述机器顺序写入；随后设置 `kProcessIdleSuppression` bit。`reload_story_world_session` 的 RAII guard 在成功以及每个 checked failure 返回点只清该 bit，不覆盖同一 process word 的其他位。

## 3. 三个 `FFFF` 继承与低 16 位

只有 `+8/+10/+12` 三项独立接受 `0xFFFF`：

- action id 继承受控角色 action `+0x00`；
- base variant 继承 action `+0x08`；
- variant delta 继承 action `+0x34`。

机器以 dword 读取 action 字段后传给 `sub_40C130`；但脚本参数及 loader ABI 均为 16 位有效值。现代端口显式取每个来源字段低 16 位。前三项 map id/tile x/tile y 从不继承；全资产也没有把前三项写成 `FFFF`。

继承在 `begin_world_session_reload` 的清理与 process-bit 发布之后、真实 loader 调用之前发生。受控角色无效时，既有 VM 入口 guard 在 dispatch 前返回 `role_not_found`，所以不会伪造角色零或执行任何 reload 前置副作用。

## 4. `sub_40C130` 的同步 typed owner

opcode 27 的 literal `1` 是 loader flags bit 0。机器 loader 在同一调用中先以旧 world/role owner 执行 `sub_40D200` preload，再释放旧 owner 并重建地图与角色。SDL 适配保留该可见边界：

1. process bit 已置位；
2. load-progress `-1` 在 preload 与旧 owner teardown 前发布；
3. 以旧 role span、旧 MAPS database、72 个 active-object prefix 与受控角色 index 调用 `preload_legacy_world_roles_before_load`；
4. 清旧 TSW loader/cache、role/path owner、音频数组和 world transient owner；
5. 将已消费的 bit0 从传给 `load_legacy_world_runtime_session` 的 flags 中清除，避免重复 preload；
6. 同步装入 `huge.lmf`/CM cache/MAPS，并执行角色 post-materialization；
7. 重绑 role span、受控角色 index、spatial index、surface、camera、story paths、frame state、special-frame loader、音频数组与方向状态；
8. 只有以上全部成功后才向 VM 返回 `true`。

`sub_40C130` 的两个后物化输入也接入真实 typed owner，而不是常量：

- `dword_4C8BE0`位于与 deferred map 字段相邻的持久存档块；现代 `LegacyWorldStoryVmState::guid_one_action_override` 由 VM 初始化归零，并在旧 map 为 22、新 map 非 22、GUID 1 且值非零时提供 action override；
- `dword_4A9940`是无哨兵 player-inventory 链首，节点 `+4` 是 `item_id`；SDL 通过 `LegacyWorldItemListState::player_inventory` 查询 `0x0192`，保持 maps 6/8/200 的 action `0x60 -> 0x5F` 条件。

## 5. 同调用重绑、连续 reload 与失败生命周期

VM 的当前 `roles` span 和外层 frame 引用原先指向 active session，不能在 port 回调中直接销毁。SDL 使用 `pending_story_world_session_`：loader 成功后先把 VM-local span/runtime 指针重绑到 pending session；解释器返回、旧外层引用离开作用域后，再原子移动到 `active_world_session_`。因此 opcode 27 后紧邻的 opcode 10 等指令会在同一次解释器调用中看到新角色，而不是旧角色或延迟一帧的 world。

连续两个 opcode 27 时，第二次以已有 pending session 作为 source；后续 role-source patch 也优先选择 pending database。替换 pending session 前不再访问旧 span，替换后立即重绑 VM-local owner。

checked failure 分成两个阶段：

- progress/preload 等不可逆 teardown 之前失败：保留旧 active world，IP 与 previous opcode 不发布；
- 旧 owner teardown 开始之后失败：设置 fatal-session 标记。VM 仍返回 `world_session_load_failed` 且不发布 IP/previous；离开持有旧引用的作用域后，frame 丢弃 active/pending 失效 session并停止运行，绝不再索引已清空 roles。

若 reload 已成功而后续同调用 opcode 失败，pending world 仍在 VM 返回后提交，保留已经完成的同步世界副作用。

## 6. synthetic 与真实资产

synthetic 覆盖：

- 四个 raw alias：`001B/401B/801B/C01B`；
- 六项直接 operand 与 selected GUID；
- 三项 `FFFF` 的低 16 位继承；
- 成功后 role span/runtime 重绑；
- 同调用 opcode 10 修改 replacement role；
- loader failure 不发布 IP/previous；
- 无效受控角色在 begin 前拒绝；
- 14-byte 截断记录完全无副作用。

全资产静态反查得到 647 条物理记录：TALK1/2/3/4 分别为 360/6/77/204；全部 raw `0x001B`、长度 14、entry probe hit 为 1。前三项 `FFFF` 命中均为零。后三项各只有一次 `FFFF`，且是同一条：

```text
TALK3.DAT@0x00016095
1B 00 A1 00 17 00 16 00 FF FF FF FF FF FF
(logical_map=161, tile_x=23, tile_y=22, inherit, inherit, inherit)
```

真实回放预置 action dword 为 `0x12345/0x23456/0x34567`，验证请求低 16 位为 `0x2345/0x3456/0x4567`，selected GUID 保持 `0x00F8`，IP 推进 14 后同调用继续到中性停止 opcode。

## 7. 验证与关闭边界

定向 synthetic、real-suite、initial-session-real-suite CTest 为 3/3。最终 Linux core 186/186、Linux app 192/192 均以 exit 0 通过；SDL opcode27 owner 和 fatal-session 路径由 app target 真实编译链接，增量链接输出无编译器 warning。全量 app 配置只报告宿主未安装 ALSA development library 的既有 CMake capability warning，不影响 SDL 静态后端、app 链接或 192 项测试。

两份剧情 VM 生成器 `py_compile` 通过；连续两次生成六个 P1/P2 inventory 文件的 SHA-256 逐项一致。最终生成器锁定 198 个显式 opcode、146 个 handler、25 个共享入口、66 个现代 case label，closure 为 22/146。

按执行计划 v264，小 handler checkpoint 不运行 Windows；Windows LLVM app 留到剧情 VM P3 大阶段统一验证。未启动原版或 OpenSWD3 游戏 EXE。

下一行严格是：

```text
0x00428713
opcode 28
```

opcode28 尚未独立审计；现有 C++ 与导航语义不得继承完成状态。
