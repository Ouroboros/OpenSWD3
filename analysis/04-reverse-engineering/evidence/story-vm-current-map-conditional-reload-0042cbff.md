# 剧情 VM 当前地图条件重载 `0x0042CBFF`

状态：`platform_adapted`、`unit_tested`、`real_asset_tested`、`sdl_runtime_integrated`、`external_dependency_tested`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x0042CBFF..0x0042CC34`

opcode：

- 163 / `OP_163_RELOAD_IF_CURRENT_MAP_NOT_EQUAL`
- 164 / `OP_164_RELOAD_IF_CURRENT_MAP_EQUAL`

## 1. signed map operand与反向谓词

两个opcode先读取`+2`的i16 map operand并符号扩展为完整dword，再与current logical map完整dword按位比较。opcode163在不相等时taken；opcode164在相等时taken。四种raw alias都先归一化到对应基础opcode。

```text
opcode 163: reload = signed_map_operand != current_logical_map_dword
opcode 164: reload = signed_map_operand == current_logical_map_dword
```

因此operand `0xFFFF`按`0xFFFFFFFF`比较；current map高16位不被截断。只有taken路径读取`+4`的u32 target。not-taken路径不访问target，即使物理窗口只剩完整opcode与map operand也先完成顺序副作用。

## 2. taken同文件重载

taken路径把unaligned u32 target传给`sub_42E430`。helper严格依次：

1. service audio一次；
2. 把context TALK data offset写为target；
3. 把context IP清零；
4. seek当前TALK文件的`target + 0x200`物理位置；
5. 读取最多`0x8000`字节到共享window，不预清未覆盖尾。

caller不把源IP加8。helper返回后重新取得window起点，发布normalized previous163/164并same-call新窗口；合法路径没有yield。

现代复用既有checked同文件window loader。读取失败时保留此前audio、context target/IP0和previous发布，把window标为未加载并返回typed `load_failed`；不伪造原版忽略文件I/O失败后继续执行共享buffer的非规范失败域。

## 3. not-taken、staged读取与窗口尾

not-taken路径固定推进8字节，发布previous并same-call顺序后继；不service audio，也不调用资源端口。

读取顺序严格为：

```text
map operand -> predicate -> taken-only target -> audio/load
```

operand缺失时不比较；taken target缺失时不audio、不写context、不发布previous。not-taken记录位于`IP=0x7FFC`时只读取到`0x7FFE`的map operand，随后IP推进到`0x8004`并发布previous，再由下一fetch返回typed `instruction_out_of_range`。完整taken记录位于`IP=0x7FF8`时先完成重载，再从新window的IP0继续。

## 4. 真实TALK资产

完整线性目录锁定27条物理记录/27 probes，全部为基础raw、长度8：

```text
           opcode163  opcode164  total
TALK1.DAT      1          6        7
TALK2.DAT      1          0        1
TALK3.DAT      1         13       14
TALK4.DAT      0          5        5
TOTAL          3         24       27
```

operand均为正值，范围22..334；值221出现5次，109出现3次，98/247/92各出现2次，其余各1次。27个target的物理`target + 0x200`均可读，首opcode全部为1026。

代表回放：

- `TALK1.DAT@0x00038DDD`：opcode163、operand22、target`0x00038C3D`；current map21时taken。
- `TALK1.DAT@0x0000BCCD`：opcode164、operand98、target`0x0000BB09`；current map98时taken。

两条记录均audio一次、保持window未覆盖尾、发布对应previous，并同调用执行目标1026后抵达测试中的typed unsupported successor。

## 5. 验证

synthetic覆盖两个opcode的全部四raw alias、正值与`-1` signed operand、current map高16位、taken/not-taken、target未读、operand/target截断、完整与非完整窗口尾、audio/previous/same-call顺序、window尾保留及checked load失败。

真实资产测试覆盖27条入口和target有效性，并回放两种谓词的代表taken记录。Story VM synthetic、real及initial-session三项通过。Linux core `186/186`、app `192/192`完整门通过。未启动原版或OpenSWD3游戏EXE。
