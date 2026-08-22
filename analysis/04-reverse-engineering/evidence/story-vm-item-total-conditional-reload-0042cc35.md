# 剧情 VM 物品总量条件重载 `0x0042CC35`

状态：`platform_adapted`、`unit_tested`、`real_asset_tested`、`asset_absence_verified`、`sdl_runtime_integrated`、`external_dependency_tested`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x0042CC35..0x0042CCF6`

opcode：

- 165 / `OP_165_RELOAD_IF_ITEM_TOTAL_AT_LEAST`
- 166 / `OP_166_RELOAD_IF_ITEM_TOTAL_AT_MOST`

## 1. 两类item owner与signed总和

handler先读取`+2`的u16 item id，然后严格按顺序查询两个owner：

1. `sub_44D680(&dword_4A9940, item_id)`遍历玩家inventory链，节点item id先AND `0x3FFF`再比较；
2. `sub_44D650(item_id)`依次扫描64个角色item root指针，只比较每个sentinel root的完整u16 item id，不遍历root后的linked nodes。

每个命中节点都把raw `+8`和`+10`两个word分别按i16符号扩展后加入i32总和；现代字段对应`quantity_a`和`quantity_b`。raw `+6 selected_count`完全不参与。玩家链即使命中，角色root扫描仍继续；角色root在首个匹配处立即返回，不访问之后的slot。

原版角色root helper无条件解引用每个slot指针。现代optional root在同一扫描位置typed-stop：匹配发生在缺root之前时保持原始短路；缺root先出现时不越过该危险点寻找后续匹配。

## 2. threshold谓词与零总和特例

两个owner查询完成后，handler无条件读取`+4`的signed i16 threshold。总和非零时：

```text
opcode 165: reload = total >= threshold
opcode 166: reload = total <= threshold
```

总和恰好为零时不执行上述普通比较，而保留机器的独立分支：

```text
opcode 165: reload = false
opcode 166: reload = true
```

因此opcode165在`total=0, threshold=-32768`时仍sequential；opcode166在同一输入下仍reload。总和可由未命中产生，也可由多个signed quantity相消产生，行为相同。

只有taken路径读取`+6`的u32 same-file target。not-taken不访问target，固定推进10字节，发布normalized previous165/166并same-call顺序后继；不service audio。

## 3. taken重载、失败与窗口尾

taken路径复用`sub_42E430`：audio一次，写context TALK data offset为target，IP归零，seek当前TALK文件`target + 0x200`并读取共享`0x8000`窗口，不预清未覆盖尾。caller不推进源IP，helper返回后发布previous并same-call新窗口。

现代checked读取失败保留audio、context target/IP0和previous发布，把window标为未加载并typed `load_failed`；不重现原版忽略I/O失败后继续使用共享buffer的非规范失败域。

读取顺序严格为：

```text
item id -> player query -> role-root query -> threshold -> predicate
        -> taken-only target -> audio/load
```

threshold截断发生在两个查询之后。taken target截断时不audio、不写context、不发布previous。not-taken记录位于`IP=0x7FFA`时可只读到threshold，随后IP推进到`0x8004`并发布previous，再由下一fetch typed-stop。完整taken记录位于`IP=0x7FF6`时先完成重载，再从新window IP0继续。

## 4. 真实TALK资产

完整线性目录锁定opcode165三条物理记录/三 probes，全部位于TALK4、基础raw、长度10；opcode166为零条线性记录，以`asset_absence_verified`记录：

```text
file offset  item  threshold  target      target first opcode
0x00004F16   795       1      0x00004D26       165
0x00005470   798       1      0x000052EF      1026
0x000057B5   798       1      0x00005662      1026
```

三个target的物理`target + 0x200`均可读。`TALK4.DAT@0x00005470`代表回放给玩家masked item798总和1，在threshold1等号边界taken；同文件加载target `0x000052EF`，audio一次、保持window未覆盖尾、发布previous165并same-call目标1026。

## 5. 验证

synthetic覆盖两个opcode全部四raw alias、玩家masked ID、角色root完整ID、linked-node忽略、两个owner同时命中、signed正负quantity与threshold、等号边界、非零taken/not-taken、命中相消后的零总和特例、selected-count未读语义、owner/root危险点和首匹配短路、threshold/target分阶段截断、taken-only target、完整与非完整窗口尾、audio/previous/same-call顺序、window尾保留及checked load失败。

真实资产测试覆盖三条opcode165入口和target有效性，并回放代表taken记录；opcode166以零线性记录和完整synthetic覆盖闭环。Story VM synthetic、real及initial-session三项通过。Linux core完整门186/186、app完整门192/192通过。未启动原版或OpenSWD3游戏EXE。
