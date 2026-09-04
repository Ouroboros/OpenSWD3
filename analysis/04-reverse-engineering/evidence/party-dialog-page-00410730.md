# 角色、道具与全局整数对话整页填充 `0x00410730`

状态：`platform_adapted`

## 1. LST锁

权威范围为`swd3.exe.lst`的`0x00410730..0x00411016`，共1030行，无外部`FUNCTION CHUNK`。唯一caller为共享Windows对话过程`0x0040F890`。

入口固定先发送`0x1009`清空列表。现代直接复用已关闭的队伍字段getter `0x004112B0`和旧行替换`0x00410490`；第一类派生值在原位置直接组合已关闭固定键曲线查询`0x004779F0`，第二类继续由窄B10 pair端口表达，第三类两个物理站点直接组合已关闭固定键计数查询`0x00477800`。

## 2. page 0..4：道具链

page按signed域选择五个head；negative page在head读取点typed-stop。链逐节点、不设数量上限，现代只在即将重复读取已访问节点时停止。

每行固定传给旧行替换：

- 名称：记录显示名。
- 数量：`i16(first) + i16(second)`。
- 编号：完整u16道具ID。
- 附加值/分母：page0计算，page1..4固定`-1/-1`从而显示空串。

page0按固定覆盖顺序处理：

1. raw `+0x20` flags匹配第一mask：固定键曲线查询，分母0。
2. 匹配第二mask：pair查询，返回第一项作分母、第二项作附加值。
3. 匹配第三mask：第三查询，分母`-1`。
4. ID非零且无符号`<=500`：无条件再次执行第三查询，覆盖此前结果，分母`-1`。

每次flags匹配都先把按位与结果的bit15清掉再与原mask精确比较；mask本身不清bit15。第一类固定曲线、第二类pair query或第三类固定数量链在原访问点停止时均不绘制当前行；低ID最终覆盖仍会按LST执行第二次固定计数查询。

## 3. page 5..8：四名队员字段

member index为`page-5`，固定填17行。名称使用原Big5：生命、魔法、體力、大生命、大魔法、大體力、力量、耐力、智慧、速度、默契、反應力、運氣、閃躲、經驗、總經驗、等級。

每行quantity直接调用17字段getter；number为selector 0..16；added value与denominator均0。因分母0残值合同，第四列最终显示十进制0，而不是`0%`。

page>=10且不等于9会在四项记录索引读取点typed-stop；列表清空已完成。

## 4. page 9：64个全局整数

固定遍历64个完整i32：row即索引，quantity为对应bit pattern按`%d`解释，number为索引，added value `-1`。

64个8字节名称槽严格复原：

- row0：原Big5“錢”。
- row11：`Cmpy1`。
- row12：`Cmpy2`。
- 其他row：runtime单字节label；label为0时为空串。

不得现代化为`Var01`等自造名称。

## 5. 验证

`special_modes.legacy_initial_menu`覆盖17字段页的原文字和signed/unsigned值、64整数页特殊名称、page0第一类固定曲线直连、第三类两个物理站点的固定计数直连、两节点查询覆盖与低ID最终覆盖、两类固定链typed-stop前缀、page1空附加值、invalid member page、链自环、pair query不可用和清空消息不可用前缀。
