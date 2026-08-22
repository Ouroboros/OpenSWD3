# 剧情 VM deferred 世界 session 重载 `0x0042C9CE`

状态：`platform_adapted`、`unit_tested`、`real_asset_tested`、`sdl_runtime_integrated`、`external_dependency_tested`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x0042C9CE..0x0042CA39`

opcode：156 / `OP_156_RELOAD_DEFERRED_WORLD_SESSION`

## 1. signed deferred map gate

handler无operand，首先读取`dword_4CAE88`并按signed i32判断。值为零或负数时只调用无业务消费者的`nullsub_1`，不读取受控角色、不调用reload helper，也不改三个deferred字段。现代省略该debug scratch。

值严格大于零时，机器在进入helper前读取受控角色当前action id，然后按以下参数同步重载：

```text
map_id       = deferred_map_id
tile_x       = deferred_map_tile_x
tile_y       = deferred_map_tile_y
action_id    = low_u16(controlled_role.action.action_id)
base_variant = 0
variant      = 1
load_flags   = 1
```

map/X/Y均以完整dword传入`sub_42E790`。已审计的所有deferred map producer只写sign-extended word、零或当前logical-map owner；进入本handler的正值域不超过u16，现代request的u16 map id不丢失合法值。tile X/Y保留完整i32二补码并转换为u32 request，负值不夹取或正规化。

## 2. reload与deferred清理顺序

`sub_42E790`先执行既有五段world-session reload前置清理并置process bit，再调用同步`sub_40C130`。现代复用opcode27/155已接入的`begin_world_session_reload`与`reload_world_session`窄端口。

只有helper成功返回后，机器才严格依次写：

```text
deferred_map_tile_x = -1
deferred_map_tile_y = -1
deferred_map_id = 0
```

因此reload调用期间三个旧值仍可见。checked reload失败保留全部deferred值和helper已完成的前置副作用，但不执行三项清理，不推进IP，不发布previous，也不service audio。

受控角色由VM入口统一验证。positive分支在begin前读取action；nonpositive分支在机器上完全不触及角色表。

## 3. IP、previous、audio与yield

positive成功与nonpositive debug-only分支都进入`loc_42CF67`，推进物理脚本指针与u16 IP 2字节，再跳common join。handler未写ESI，因此两路都发布normalized previous156、执行一次audio maintenance并yield；不是same-call continuation。

记录位于`IP=0x7FFE`时，positive路径仍先完成同步reload与三项deferred清理，再推进到`0x8000`、发布previous、service audio并yield，不fetch后继。

## 4. 资产与验证

完整线性TALK目录锁定opcode156一条物理记录/一个probe：

```text
TALK1.DAT@0x00038E29  9C 00
```

它位于opcode53与下一条opcode52之间。四种raw word在四库的全文件双字节候选计数为：

```text
             009C 409C 809C C09C
TALK1.DAT      37    0    0    0
TALK2.DAT      59    0    0    0
TALK3.DAT       0    0    0    0
TALK4.DAT      10    0    0    0
```

只有上述TALK1位置被线性目录证明为opcode156入口。synthetic覆盖四raw alias、正值最大u16 map、负tile完整dword、reload期间旧值可见、成功后清理、零/负map no-op、checked reload失败及精确窗口尾；真实回放覆盖唯一线性记录。

Story VM synthetic、real及initial-session三项通过。Linux core `186/186`、app `192/192`完整门通过。未启动原版或OpenSWD3游戏EXE。
