# 剧情 VM 玩家物品与角色槽交换 `0x0042C033`

状态：`assembly_exact`、`unit_tested`、`asset_absence_verified`、`platform_adapted`、`sdl_runtime_integrated`；MON定义装载归后续special-modes owner。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x0042C033..0x0042C233`

opcode：132

## 1. 物理记录与分阶段读取

记录固定8字节：

```text
+0  u16 opcode
+2  u16 role group
+4  u16 role slot
+6  u16 player item id
```

机器先零扩展读取role group和role slot，再依次要求`group <= 3`、`slot <= 11`。任一index无效时只进入空诊断，不读取`+6` item id，也不访问任何item owner；随后仍走固定`+8`尾。

两项index有效后才读取item id，并调用：

```text
sub_44D680(&dword_4A9940, item_id)
```

玩家普通库存节点的item id先`AND 0x3FFF`，脚本item id保持完整u16；首个masked match作为source。miss只诊断并消费，不访问角色root。

所有非危险出口都进入`0x004289A4`：物理脚本指针和u16 IP各加8，设置ESI=1，common join发布normalized previous132并同调用继续；没有audio service或yield。

## 2. 角色root选择

source命中后，角色root索引按：

```text
root_index = role_group * 16 + role_slot
```

有效域覆盖`dword_4C8AD0[0..11,16..27,32..43,48..59]`，即四组各前12槽；每组末四槽不由该handler选择。64槽在世界初始化与存档装载时均建立独立`0xB0`哨兵root，并把root `next`强制清零。

机器先分配一个`0xB0`临时节点，随后才读取并解引用选定root。空root因此位于临时分配后的原危险点。

## 3. 临时旧root深拷贝

handler以`rep movsd 0x2C`把选定root完整复制到临时节点，包含next、四个u16字段、`0xA0`定义快照和`+0xAC`说明指针。随后读取旧root说明长度，分配`length+1`字节，把新指针写入临时节点并复制字符串。

这份临时快照后续只有item id参与库存更新；定义快照和说明仍照原顺序被完整复制。两个malloc、root、说明指针及字符串均无null检查，现代只在对应分配/访问点typed-stop。

## 4. source覆盖角色root

handler再以`rep movsd 0x2C`把玩家source节点完整覆盖选定root，然后按顺序写：

```text
root.quantity_a    = 1
root.quantity_b    = 0
root.selected_count = 0
```

接着读取source说明长度，分配独立副本，写入root `+0xAC`并复制字符串。因而root保留source的完整u16 item id与定义快照，但数量规范为`1/0`，临时选择数清零，说明不借用source owner。

原完整raw复制也会把source `next`写入root：旧root child链被丢失，root开始别名玩家source的后继链。原版没有恢复或清零该next；这是明确原始所有权缺陷。

## 5. 两次玩家库存mode0更新

root覆盖完成后，handler固定依次调用：

```text
sub_44D2D0(&dword_4A9940, displaced.item_id, +1, 0)
sub_44D2D0(&dword_4A9940, source.item_id,    -1, 0)
```

第一项把旧root item加回玩家普通库存；第二项从玩家库存扣除source item。两次都使用mode0数量B规则：完整u16回绕、i16判断、上限99、非正B搬入A、A耗尽删除、`0xFFDC`规范化和missing正值的MON定义前插。helper返回值均不参与正常控制流；旧item MON loader miss后仍执行source扣除。

source item id在第一项返回后从原source节点重新读取。若第一项因signed回绕删除了同一source节点，原版在该位置读取已释放内存；现代于此原use-after-free点返回`item_update_failed`，保留root覆盖和已发生的库存删除，不发布IP/previous。

最后机器只对临时`0xB0`本体调用通用delete wrapper，没有释放临时说明。旧root原说明在root覆盖时也没有释放，所以每次完整交换会泄漏两份说明owner；source与新root各自的说明仍独立。

## 6. 现代owner与平台适配

现代VM把`role_item_lists[64]`借用从只读升级为可写，继续与SDL世界item owner共用真实root，不建立VM镜像。玩家链仍使用同一`player_inventory`。

`LegacyWorldItemNode`保留全部可见字段与定义/说明快照。临时节点和说明以RAII深拷贝；选定root固定字段、数量重设、说明深拷贝与两次库存更新保持机器顺序。裸malloc/free、说明泄漏和null/UAF域以`unique_ptr/vector/list`及typed-stop隔离。

原root `next`与玩家tail的交叉所有权无法由两个独立`std::list`安全表达。现代在raw覆盖时释放选定root原独立child owner，并保持新root child为空；不伪造玩家节点共享所有权。当前生产消费者只读取64个root自身，child仅用于总关闭；因此有效root字段、库存数量与后续root存在查询保持一致。该差异明确归类`platform_adapted`，不声称重现原双重释放风险。

普通旧item加回库存继续复用`load_story_item_definition`窄端口。当前SDL MON后端留给B9：loader miss按原helper返回空并继续source扣除，不伪造定义节点。

## 7. 资产与验证

完整线性TALK目录中opcode132为0条物理记录/0 probes，使用`asset_absence_verified`，不伪造真实回放。四文件基础raw `0x0084`字样为`22/35/8/9`，`0x4084/0x8084/0xC084`均为零；这些基础字样都不是线性指令入口。

synthetic覆盖四raw alias乘四个role group的slot11、masked source高位、source后继不别名、旧root child清理、root完整快照/说明/数量规范、旧item新建与existing上限、source扣除、旧item loader miss继续、第一项删除source后的UAF typed-stop、两类invalid index、masked miss、player-before-role owner顺序、缺root、三级operand截断、invalid未读item的partial tail及成功`IP=0x7FF8`精确尾。

Story VM synthetic、real及initial-session三项通过。Linux core 186/186、app 192/192完整门均以exit 0通过。未启动原版或OpenSWD3游戏EXE。
