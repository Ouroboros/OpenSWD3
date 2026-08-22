# 剧情 VM 队伍物品资格追加 `0x0042BF72`

状态：`assembly_exact`、`unit_tested`、`asset_absence_verified`、`platform_adapted`、`sdl_runtime_integrated`；MON定义装载归后续special-modes owner。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x0042BF72..0x0042C032`

opcode：131

## 1. 物理记录与入口顺序

记录固定6字节：

```text
+0  u16 opcode
+2  u16 party item-list index
+4  u16 item id
```

机器先零扩展读取两项operand，再判断index。index `0..3`选择`dword_4A9490[4]`的一条必需哨兵链；index `>=4`只进入空诊断，随后仍固定消费6字节。

所有正常出口都经`0x00429F61`共享尾：物理脚本指针与u16 IP各加6，设置ESI=1，common join发布normalized previous131并同调用继续。handler没有audio service或yield。

## 2. 已存在短路

有效index先把所选哨兵root直接传给`sub_44D680`。helper从`sentinel->next`开始，不比较哨兵自身；每个普通节点的`item_id`先`AND 0x3FFF`，再与脚本完整u16 item id比较。

首个masked match存在时，handler立即走共享+6尾，不执行数量upsert、definition flag清除、资格检查或MON装载。因此节点`0xC123`会让脚本`0x0123`短路；脚本`0xC123`不会被该预查命中。

## 3. 缺失时的mode1 upsert

masked预查miss后，机器调用：

```text
sub_44D2D0(selected_sentinel, item_id, 1, 1)
```

传入哨兵本身使helper从`sentinel->next`执行完整u16 item id查找，并把新节点前插到该位置。

### existing exact id

mode1对`quantity_a`执行u16加一，再按i16判断：

```text
result > 99   quantity_a=99, quantity_b=0
1..99         保持
result <= 0   quantity_a=0；quantity_b按i16 >0时保留，否则删除节点并返回空
```

保留的`0xFFDC`节点再强制为`quantity_a=1, quantity_b=0`。因此脚本高位item id可以绕过masked预查，却由exact upsert命中并增加现有数量。

### new id

正增量固定为1。普通item分配并清零`0xB0`节点，调用`sub_476DB0`填充MON定义快照和说明；失败释放未链接节点并返回空。`0xFFDC`不访问MON，复制公共“無”名称并使用数量1。新节点写完整item id、`quantity_a=1`、`quantity_b=0`后前插；mode1本身不置definition flags bit15。

helper返回空后原handler会立即解引用`[ESI+0x2C]`；现代只在该原危险点返回`item_update_failed`，保留helper已经完成的删除或loader failure。

## 4. flag与资格门

成功取得节点后，handler读取节点`+0x2C`完整dword并只清AH bit7，即definition flags bit15 / definition snapshot字节`+0x21`的`0x80`。

随后零扩展读取节点`+0x46`的u16资格word。按index生成：

```text
index 0  0x8000
index 1  0x4000
index 2  0x2000
index 3  0x1000
```

对应bit已置时保留upsert结果并消费。bit未置时只执行空诊断，再调用：

```text
sub_44D0F0(selected_sentinel, returned_item_id, -1, 0)
```

该调用不是简单擦除returned节点。它先从链头扫描完整id且definition bit15仍置位的首节点；只有全miss才扫描bit15已清的首节点。命中后对`quantity_a`执行u16减一和signed判断：

- `>99`夹为99；
- 正值保留；
- 零删除并返回；
- 第一段flagged节点结果为负时，删除后继续第二段unflagged扫描；
- 第二段非正值删除后结束；
- 保留的`0xFFDC`把quantity A重设为1。

正常新节点的quantity A为1且bit15已清，所以不合格时第二段将其减到零并删除，净效果是不追加。重复/高位exact节点仍保留上述两段扫描、数量和flag副作用。

## 5. 现代owner与平台适配

现代VM新增可写借用`LegacyWorldItemListState::party_item_lists[4]`，继续使用既有哨兵与`std::list<LegacyWorldItemNode>`，不复制链或定义快照。

index合法后才访问owner。缺runtime binding或所选必需root时，在原root首次解引用点返回`runtime_unavailable`；invalid index即使owner缺失仍按机器消费。裸next、malloc/free和说明指针由list/vector RAII替代，合法域的首匹配、前插、数量回绕、定义修改和两段减一顺序不变。

普通新节点继续复用opcode128建立的`load_story_item_definition`窄端口。当前SDL的MON后端延期到B9，明确返回loader failure；opcode131因此在原空返回解引用点typed-stop，不发布伪定义节点。已有队伍item、invalid index和不需MON的路径仍使用真实owner运行。

## 6. 资产与验证

完整线性TALK目录中opcode131为0条物理记录/0 probes，使用`asset_absence_verified`，不伪造真实回放。四文件基础raw `0x0083`字样为`31/35/11/9`，另有TALK1单个`0x4083`；均不是线性指令入口。`0x8083/0xC083`原始字样为零。

synthetic覆盖四raw alias乘四个index资格mask、masked高位已有节点短路、invalid `4/FFFF`、invalid仍先读item、合格新建/快照/说明、普通与FFDC不合格净删除、高位exact upsert、add-then-decrement、flagged负结果删除后继续unflagged扫描、mode1 i16回绕保留/删除、signed 99夹值、definition loader failure、runtime/root缺失、两级operand截断和`IP=0x7FFA`精确尾。

Story VM synthetic、real及initial-session三项通过；Linux core 186/186与app 192/192完整门通过。未启动原版或OpenSWD3游戏EXE。
