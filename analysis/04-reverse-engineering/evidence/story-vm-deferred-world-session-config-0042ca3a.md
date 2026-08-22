# 剧情 VM deferred 世界 session 配置 `0x0042CA3A`

状态：`assembly_exact`、`unit_tested`、`asset_absence_verified`、`sdl_runtime_integrated`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x0042CA3A..0x0042CA7B`

opcode：157 / `OP_157_CONFIGURE_DEFERRED_WORLD_SESSION`

## 1. 记录与map 22特例

物理记录长度固定8字节：

```text
+0  u16 raw opcode
+2  u16 map id
+4  u16 tile x
+6  u16 tile y
```

机器首先读取map word并与22作u16相等比较。恰好等于22时只调用无副作用`nullsub_1`，完全不读取tile X/Y，也不改三个deferred字段；随后仍按物理长度推进8字节。现代省略该纯no-op调用。

因此map22记录即使在窗口中只剩opcode与map两个word，也会成功消费名义8字节，发布previous并继续下一次fetch；尾部tile bytes不是安全预检条件。

## 2. non-22 staged writes

map不等于22时，三个word严格按物理顺序各自sign-extend i16到i32并立即提交：

```text
deferred_map_id     = sign_extend_i16(map)
deferred_map_tile_x = sign_extend_i16(tile_x)
deferred_map_tile_y = sign_extend_i16(tile_y)
```

包括map 0、`0x8000`与`0xFFFF`在内都照常写入，不执行范围、正值或sentinel检查。opcode156后续只对signed map严格大于零时发起reload；本handler不提前应用该谓词。

现代按每个原始读取点检查窗口边界：缺map时无写入；缺X时保留map写；缺Y时保留map与X写。没有整体8字节预检或失败回滚。

## 3. IP、previous与same-call

map22与non22完整路径都进入`loc_4289A8`，物理指针和u16 IP固定增加8字节，设置`ESI=1`后进入common join。两路发布normalized previous157，并在同一次VM调用中继续取下一条；不service audio、不yield。

non22完整记录位于`IP=0x7FF8`时先完成三写、IP=`0x8000`与previous157，再由同调用下一fetch返回越界。map22位于`IP=0x7FFC`且仅有map word时仍推进到`0x8004`并由下一fetch失败。

## 4. 资产与验证

完整线性TALK目录中opcode157为0条物理记录/0 probes，使用`asset_absence_verified`。四种raw word在四库的全文件双字节候选计数为：

```text
             009D 409D 809D C09D
TALK1.DAT      10    0    0    0
TALK2.DAT       1    0    0    0
TALK3.DAT       0    0    0    0
TALK4.DAT       9    0    0    0
```

这些基础raw字样均非线性指令入口。synthetic覆盖四raw alias、map/X/Y的零/正/负符号边界、map22未读tile尾、map/X/Y三级截断、完整精确尾、previous与same-call。

Story VM synthetic、real及initial-session三项通过。Linux core `186/186`、app `192/192`完整门通过。未启动原版或OpenSWD3游戏EXE。
