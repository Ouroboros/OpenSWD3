# 剧情 VM 玩家物品数量调整 `0x0042BE8A`

状态：`assembly_exact`、`unit_tested`、`real_asset_tested`、`platform_adapted`、`sdl_runtime_integrated`；MON定义装载归后续special-modes owner。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x0042BE8A..0x0042BEFD`

opcode：128

## 1. VM记录与调用顺序

物理记录固定6字节：

```text
+0  u16 opcode
+2  u16 item id
+4  i16 quantity delta
```

机器访问顺序不是字段布局顺序：先以`mov ax,[ebx+4]`读取delta，再以`mov cx,[ebx+2]`读取item id，随后固定调用：

```text
sub_44D2D0(&dword_4A9940, item_id, delta, 0)
```

最后参数0选择helper的quantity-B调整模式。返回节点非空时，handler只以节点`+0x0C`名称和原始signed delta格式化`"%s,%d"`；返回空则格式化`"%d,%d"`。两路结果都只交给`nullsub_1`，没有业务消费者。

handler随后固定推进物理脚本指针与u16 IP各6字节，不设置ESI；common join发布normalized previous128，执行一次audio service并yield。helper返回空、定义加载失败或missing item加非正delta均不改变该流控。

## 2. `sub_44D2D0` mode 0

`dword_4A9940`是无哨兵的玩家普通库存链首。helper从头到尾取第一个`node.item_id == item_id`。

命中节点时，以u16加法提交`quantity_b += delta`，再把结果按i16判断：

```text
result > 99   quantity_b=99, quantity_a=0, return node
1..99         保持两项，进入FFDC规范化并返回node
result <= 0   quantity_b=0，按u16回绕把旧signed result加到quantity_a
```

搬入quantity A后，A按i16 `<=0`会摘链并依次释放动态说明和节点，返回空；A仍为正则保留节点。item id `0xFFDC`在共同返回前把A/B强制为`1/0`，但`B>99`夹值和A耗尽删除发生在该规范化之前。

未命中节点时，signed delta `<=0`直接返回空。正delta先分配并清零`0xB0`节点：普通item调用`sub_476DB0`从`mon.dat`填充`+0x0C`起定义快照和`+0xAC`动态说明；失败时释放未链接节点并返回空。`0xFFDC`不访问MON，复制公共“無”名称并把初始delta强制为1。

创建成功后写item id、`quantity_a=0`、`quantity_b=delta`，并对节点`+0x2C`定义flags置bit15；最后把新节点前插到玩家库存链首。

## 3. 现代owner与适配

现代VM借用既有`LegacyWorldItemListState::player_inventory`，不建立VM私有库存副本。`LegacyWorldItemNode`保持item id、selected count、quantity A/B四个u16、0xA0定义快照和独立说明owner。

现有节点的查找、u16回绕、i16比较、99夹值、A/B搬运、删除、FFDC规范化和链首插入在VM侧直接按helper转写。裸next、手工free及unchecked malloc分别由`std::list`、RAII和`item_allocation_failed` typed-stop隔离。

普通新节点的MON定义通过`load_story_item_definition`窄端口装载。当前SDL已绑定真实玩家库存链，但special-modes模块尚未提供MON定义loader；端口明确返回失败，因而走原helper“释放未链接节点并返回空”的路径，不发布空定义假节点。VM参数解析、item helper和流控不因此延期；真实MON后端留给B9完成。

缺少现代玩家库存binding只在两项operand都读取后的原固定全局调用点返回`runtime_unavailable`。definition failure仍按机器继续+6、previous、audio和yield。

## 4. 资产与验证

线性TALK目录锁定397条物理记录/399 probes，全部raw `0x0080`、长度6：

```text
TALK1  145
TALK2  114
TALK3  112
TALK4   26
```

共214种item id，范围`0x0020..0x041D`。signed delta分布为：`+1` 371条、`-1` 20条、`-2` 3条、`+2/+8/+10`各1条；正值374条、负值23条、零值0条，未观察`0xFFDC`。

四文件raw `0x0080`字样共684处，高位alias `0x4080/0x8080/0xC080`均为0；只有线性目录证明的397条作为资产记录。TALK3两条相邻记录各有两个entry probe，其余记录各一个。

真实回放使用`TALK1.DAT@0x0000764F`：

```text
80 00 CB 03 01 00
```

即item 971增加1。测试定义端口填充可识别快照后，节点前插、quantity B=1、definition flags bit15及说明owner均通过。

synthetic覆盖四raw alias、existing数量增加、`>99`夹值、负delta的A/B搬运、A耗尽删除、i16回绕删除、existing/new `0xFFDC`、定义快照/说明前插、missing非正delta、定义加载失败、缺库存owner、delta-first operand截断及`IP=0x7FFA`精确尾。

Story VM synthetic、real及initial-session三项通过。Linux core 186/186与app 192/192均以exit 0通过。未启动原版或OpenSWD3游戏EXE。
