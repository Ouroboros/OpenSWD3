# 剧情 VM 队伍成员字段条件重载 `0x0042D05C`

状态：`platform_adapted`、`unit_tested`、`asset_absence_verified`、`sdl_runtime_integrated`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x0042D05C..0x0042D0D3`，getter `0x004112B0..0x0041144B`，same-file loader `0x0042E430..0x0042E47B`

opcodes：186–187

## 1. 记录与getter调用

两个opcode共用固定10字节布局：

```text
+2  i16 field selector
+4  u16 threshold
+6  u32 same-file target（taken-only读取）
```

机器先读取signed selector。`selector > 16`时不调用getter，不读threshold/target，不推进IP；只发布previous、service audio一次并yield，因此下一帧重试同一记录。

`selector <= 16`时固定调用`sub_4112B0(selector, 1)`，即只读取四项0x38 party-member记录中的第二项。negative selector通过外层signed门，但getter内部按unsigned switch落到default并返回0，不发生负索引访问。

getter的17项返回规则：

```text
0..5   record +0x04..+0x0E，i16 sign-extend
6..13  record +0x10..+0x1E，u16 zero-extend
14     record +0x20，完整i32 bit pattern
15     record +0x00，完整i32 bit pattern
16     record +0x2C，u8 zero-extend
other  0
```

现代把既有`LegacyWorldStoryPartyMemberResources`补全为同一0x38记录的中性语义owner。opcode134继续访问原三组current/limit和transient字段；186–187读取完整17项视图，不建立副本。B10/B11仍负责后续真实记录加载与持久化，当前工作包不伪造资产值。

## 2. signed条件与target时点

getter返回后才读取zero-extended u16 threshold，并执行signed i32比较：

- opcode186：`value >= threshold` taken；
- opcode187：`value <= threshold` taken。

not-taken不读取target，固定把逻辑IP加10，发布previous并same-call，无audio。即使物理窗口只剩opcode、selector和threshold六字节，not-taken仍提交IP到越过`0x8000`的位置，然后由下一fetch报告窗口越界。

taken才读取`+6 u32 target`并调用same-file loader。loader内部先service audio一次、发布目标TALK offset与IP0并读取新窗口；返回handler后common join发布previous，再service audio一次并yield。因此taken不在同一VM调用执行目标首指令，direct audio总数精确为2。

checked loader失败是现代平台边界：目标offset/IP0和第一次audio已经提交；handler仍按原调用返回后的顺序发布previous并执行第二次audio，再返回`load_failed`。target截断则停止在loader前，不发布previous、不audio。

## 3. 资产锁与验证

完整线性TALK目录中opcodes186/187均为0条，使用`asset_absence_verified`。全文件双字节候选总计：

```text
00BA=13  40BA=0  80BA=0  C0BA=33
00BB=27  40BB=3  80BB=0  C0BB=5
```

这些候选均为operand、文本或其他非线性入口字节，不能冒充真实record。

synthetic覆盖两opcode四raw alias、全部17 selector映射、固定第二项owner、0–5 sign extension、6–13 zero extension、两个完整i32字段、u8字段、signed GE/LE及equality、negative selector default0、high selector不读后项并audio-yield重试、threshold/target分阶段截断、not-taken未读target与十字节same-call、taken精确尾、双audio顺序及checked load失败。Story VM synthetic/real/initial-session 3/3通过，SDL app编译通过；Linux完整门core 186/186、app 192/192通过。未启动原版或OpenSWD3游戏EXE。
