# 合并玩家道具数量B到A并清除类别标志 `0x0044D4E0`

状态：`platform_adapted`

## 1. LST锁

权威范围为`swd3.exe.lst`的`0x0044D4E0..0x0044D51B`，34行，无callee、无外部FUNCTION CHUNK。直接caller为`0x00446700+0xE5`。

函数从head逐条处理176字节玩家道具记录：

1. `filter_flags &= 0xFFFF7FFF`，只清bit15并保留其余31位。
2. 数量A按u16回绕加数量B。
3. 数量B无条件写0。
4. 合并后的A按i16解释；signed大于99时写99，signed负值或0不做下限夹取。
5. 前进到next，直到null；正常返回EAX=0。

因此`A=0xFFFF,B=1`回绕为0，`A=0x7FFF,B=1`成为`0x8000`并保持，不会被错误夹到99。链表环只在下一次将重复处理同一节点时typed-stop；第一次合并、bit15清除和B清零均保留。

## 2. caller对应

`0x00446700`在多阶段游戏内菜单提交中调用本函数后继续发布选择和转场。现有typed选择记录与克隆流程已经显式持有`first_value`、`second_value`和`filter_flags`，没有裸`0x0044D4E0`地址或merge opaque端口。本函数提供同一176字节记录的独立typed入口。

## 3. 验证

UT覆盖空链、三节点正常链和自环：验证只清bit15、90+20夹99、`0xFFFF+1`回绕0、`0x7FFF+1`保持`0x8000`、所有B清0，以及自环在第一次完整合并后停止。

workpack双生成稳定为`184/227`，SHA256为`981207a0c7913d1becc84dba2d9d1cd7372480bd441b1df7a0556f806dc95a59`；下一项为`0x0044D520`。
