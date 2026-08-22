# 剧情 VM 当前世界 session 重载 `0x0042C95B`

状态：`platform_adapted`、`unit_tested`、`asset_absence_verified`、`sdl_runtime_integrated`、`external_dependency_tested`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x0042C95B..0x0042C9CD`

opcode：155 / `OP_155_RELOAD_CURRENT_WORLD_SESSION`

## 1. map 22特殊分支

handler无operand，首先读取当前logical map dword。值为22时只把两个debug字符串传给`nullsub_1`，不读取受控角色，不写deferred map字段，也不调用reload helper。现代省略无消费者debug scratch，随后按正常完成路径推进2字节。

## 2. 非22请求与写入顺序

非22路径按受控角色index直接取当前角色，严格依次执行：

```text
deferred_tile_x = role.world_x >> 4   // logical u32 shift
deferred_tile_y = role.world_y >> 4
deferred_map_id = current_logical_map
reload(
    current_logical_map,
    deferred_tile_x,
    deferred_tile_y,
    low_u16(role.action.action_id),
    base_variant = 0,
    variant_delta = 1,
    load_flags = 1
)
```

三个deferred字段都在helper前提交。受控角色index由VM入口统一验证；本handler没有额外resolver或operand读取。

`sub_42E790`与opcode27共用同步reload边界。现代先调用`begin_world_session_reload`恢复helper的五项清理和process bit，再调用`reload_world_session`。checked reload失败保留三个deferred写和helper前置副作用，但不推进IP、不发布previous、不service audio。

## 3. 完整dword tile与低word持久字段

机器`sub_40C130`将tile X/Y按完整dword左移4初始化受控角色world坐标，同时只把低16位写入MAPS/角色物理word。空间绑定在完整坐标覆盖前已建立，之后不重建；原始陈旧绑定按事实保留。

既有`LegacyWorldLoadRequest`的tile字段原为u16，只能表达opcode27脚本operand。现扩为u32：

- opcode27的u16 operand继续零扩展，合法行为不变；
- MAPS source和preload patch显式截低16；
- 角色物化完成后以完整u32 tile左移4覆盖受控角色world X/Y；
- 不重新执行已经完成的空间绑定。

runtime-session UT独立覆盖高位tile、低16 MAPS写入、完整world坐标和陈旧空间绑定。

## 4. IP、previous、audio与yield

map22 no-op和非22 reload成功都进入`loc_42CF67`，推进物理脚本指针与u16 IP 2字节，再跳common join。handler未写ESI，因此两路都发布normalized previous155、执行一次audio maintenance并yield；不是same-call continuation。

记录可位于`IP=0x7FFE`精确结束窗口。同步reload、IP=`0x8000`、previous、audio和yield均完成，不fetch后继。

## 5. 资产与验证

完整线性TALK目录中opcode155为0条物理记录/0 probes，使用`asset_absence_verified`。四种raw word在四库的全文件双字节候选计数为：

```text
             009B 409B 809B C09B
TALK1.DAT       3    0    0    0
TALK2.DAT       1    0    0    0
TALK3.DAT       0    0    0    0
TALK4.DAT      19    0    0    0
```

这些基础raw字样均非线性指令入口。synthetic覆盖四raw alias、完整dword tile、固定reload参数、map22 no-op、checked reload失败、helper/audio顺序及精确窗口尾。runtime-session synthetic/real两项覆盖共享loader位宽边界。

Story VM synthetic、real及initial-session三项通过；runtime-session synthetic/real两项通过。Linux core `186/186`、app `192/192`完整门通过。未启动原版或OpenSWD3游戏EXE。
