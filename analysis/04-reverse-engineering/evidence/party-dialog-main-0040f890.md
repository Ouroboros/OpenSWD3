# 共享角色、道具调试对话主过程 `0x0040F890`

状态：`platform_adapted`

## 1. LST锁

权威范围为`swd3.exe.lst`的`0x0040F890..0x004103B8`，共1377行，无外部`FUNCTION CHUNK`。唯一caller为世界玩家输入`0x00402F80`。

现代将Windows消息、通知结构、命令参数、编辑框文字、列表选择和两处陈旧栈值收敛为显式事件snapshot；窗口操作留在窄端口。列头`0x004103C0`、行填充`0x00410600`、整页填充`0x00410730`、队员setter `0x00411030`、玩家道具数量`0x0044D2D0`和低14位查找`0x0044D680`均直接复用已关闭typed入口。

## 2. 初始化与页签通知

`WM_INITDIALOG(0x110)`固定顺序：

1. 插入10页：物品；賽特、妮可、卡瑪、李靖的法術；同四人的屬性；變數。
2. page写0。
3. 建四列，设置list extended style `0x20`，填page0。
4. ID、数量、附加值编辑框长度依次限制为4、8、4。
5. 返回1。

`WM_NOTIFY(0x4E)`只处理code `0xFFFFFDD9(-551)`：读取当前页，按3、2、1、0删除旧列，重建四列并填页；随后依次启用附加值、补足、删除、新增四控件，page>=5时再按同序全部禁用。其他通知不产生副作用。

## 3. 命令

- `0x3E8`：先写关闭latch，再结束对话并隐藏cursor。
- `0x3EB`：只允许page0；按列表索引取节点。signed数量和小于90时先把second写90，只有新signed总和大于90才把first写为`90-second`，因此negative first可保留并使最终总和仍小于90；随后直接填当前四cell。
- `0x3EC`：page<5先申请32字节scratch。page1..4只接受ID 1501..1999并强制数量1、附加值0；page0读取输入。调用玩家数量helper后，page0按第一、第二、第三mask及低ID<=500顺序发布分类更新，低ID第三类可重复。最后严格为填页→清三编辑框→释放scratch。
- `0x3EE`：page0..4按列表索引取节点；目标ID高16来自原`sub_43B9A0`输出指针返回值，低16来自节点ID，现代由`output_pointer_high_word`显式snapshot；数量delta固定`-1024`。helper返回非null才报告删除错误，然后填页。
- `0x3F6`：先申请32字节scratch。page9写64个全局i32；page0..4始终在玩家总库存链按低14位找ID，数量存在时先清first并把signed值仅按`>=90`上夹，page0再发布分类更新；page5..8直接调用17字段setter。成功路径严格为释放scratch→填页→清三编辑框。

## 4. 数字解析与残值

导航callee `0x00430BE0`只接受可选正负号加最多9个十进制数字；非法输入不改目标槽，caller忽略返回值。现代保留：

- ID解析目标初值为消息值`0x111`。
- 更新库存缺失/非法附加值时复用命令`lParam`低32位。
- 全局整数、队员字段及部分数量缺失/非法时复用显式`stale_local_value`。
- 删除命令复用显式旧输出指针高字。

不得用现代“解析失败即返回”清除这些残值路径。

## 5. 原始停止与泄漏前缀

- scratch申请失败立即停止。
- 新增page0的数量helper返回null时先报告插入错误，再在首个记录字段读取点typed-stop，scratch不释放。
- 负页的item head、缺失selected record、无界masked lookup链环、page>=10成员访问和负/大于63全局索引均在原始读取/写入点typed-stop。
- page9索引越界、成员page>=10、成员selector 17以上均保留已申请且未释放的scratch；selector 17以上原版直接返回，不伪造错误状态。
- 新增与直接修改的释放时机不同，不得统一清理。

## 6. 验证

`special_modes.legacy_initial_menu`覆盖初始化10个原Big5页名、四列与三长度；有效/无效页签通知；关闭顺序；negative first补足残值；page0四次分类更新；page1强制数量；删除打包高字；直接修改的negative u16与`lParam`残值；队员setter；全局stale值；越界、记录缺失、selector 17和scratch申请失败前缀。
