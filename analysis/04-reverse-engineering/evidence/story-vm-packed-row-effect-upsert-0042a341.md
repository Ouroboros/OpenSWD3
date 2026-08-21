# 剧情 VM framebuffer 区域效果创建 `0x0042A341`

状态：`platform_adapted`、`unit_tested`、`real_asset_tested`、`sdl_runtime_integrated`；原程序动态差分仍为`blocked_runtime_oracle`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x0042A341..0x0042A547`

opcode：`83`

## 1. ID与删除顺序

handler先读`+2 u16 id`。id `>=256`只调用诊断nullsub，随后直接走共享+16尾；它不访问效果链、不读其余operand、不分配。

合法id访问`dword_4BAB9C`并删除所有`(node.mode & 0xFF)==id`节点。每个节点严格按`row_offsets(+0x0C)`、`row_lengths(+0x10)`、节点本身顺序释放。删除在新节点分配和所有后续operand读取前已提交；后续截断、矩形失败或typed分配失败都不能恢复旧节点。

## 2. 节点与staged operand

删除完成后原版分配0x18节点，随后按实际访问顺序读取：

```text
+4  u16 color/style -> node+0x0A word
+8  word X          -> 清bit0后写+0x00，按i16
+10 word Y          -> 清bit0后写+0x02，按i16
+12 word width      -> 清bit0后写+0x04，按i16
+14 word height     -> 清bit0后写+0x06，按i16
+6  u16 mode        -> 在两条行数组分配后才读取
```

`+2 id`写入node+8低字节；高位mode随后OR入同一word。现代`LegacyPackedRowEffect::mode`保留此packed布局。

测试覆盖0..6个operand可用的七个截断点：未读到id时旧节点保留；一旦合法id可读，同ID节点已删除，且IP/previous保持失败前值。

## 3. 矩形门控与原始宽高边界

门控只有：

```text
X >= 0
Y >= 0
signed(X) + signed(width)  <= 640
signed(Y) + signed(height) <= 480
```

没有`width>0`或`height>0`条件。负/零宽高可通过；测试保留零尺寸与`-2/-2`节点。负height在原版会把`2*height+8`作为unchecked分配大小；现代不为`row_count<=0`建立vector，避免巨大host分配，但保留节点字段、空行数组、链接和流控，属于平台适配。

矩形失败发生在旧节点删除和新0x18节点分配之后、行数组分配之前；原版释放新节点、诊断并+16继续。现代临时list销毁新节点，保留相同可见副作用。

## 4. 两级数组与mode初始化

合法矩形依次分配两块`2*height+8`内存，分别写入+0x0C/+0x10；原版不检查任一分配。现代正row count按行数依次resize两个vector，`bad_alloc/length_error`映射`packed_row_effect_allocation_failed`，并销毁未链接临时节点；此前同ID删除仍保留。

每行length初值固定2。offset和高mode为：

| `+6` | high mode | row offset |
| --- | ---: | ---: |
| 1 | `0x4000` | `width-2`按word wrapping |
| 2 | `0x0800` | 0 |
| 其他 | `0x8000` | 0 |

完成后节点前插，推进16、common join发布normalized previous83并same-call继续。现代复用既有packed-row逐帧更新、场景重置和退出释放生命周期。

## 5. alias、资产与测试

四raw alias `0053/4053/8053/C053`均归一为83；`0x7FF0`精确尾在创建/前插/IP/previous完成后才由下一fetch失败。

线性TALK目录含1879条物理记录/1879 probes：

```text
TALK1.DAT 485
TALK2.DAT 337
TALK3.DAT 396
TALK4.DAT 661
```

全部raw `0x0053`、长度16、id<256、X/Y非负、width/height为正。mode分布：1为935条、0为938条、11为6条；真实资产无mode2。real CTest回放`TALK1.DAT@0x000060C2`：id5/color9/mode1/X130/Y360/width382/height111，清bit0后建立110行、offset380、length2的`0x4005`节点并替换旧id5。

synthetic还覆盖三mode、odd清位、全部同ID删除、invalid-ID早消费、四种矩形失败、非正尺寸、owner缺失和same-call。剧情VM三项为3/3。

分类：`platform_adapted`。合法域删除/读取/分配/初始化/链接顺序、字段位宽、+16、previous和same-call保持；裸节点/数组/全局链改为typed list/vector owner，并对unchecked分配和负height巨大分配做typed收敛。
