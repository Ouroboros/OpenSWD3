# 按三种操作更新玩家道具数量A/B并在两者耗尽时删除 `0x0044D2D0`

状态：`platform_adapted`

## 1. LST锁

权威范围为`swd3.exe.lst`的`0x0044D2D0..0x0044D4D8`，275行，无外部FUNCTION CHUNK。12个直接caller分布于世界控制、全局初始化/清理、剧情VM、炼妖、护驾、游戏内菜单、选择记录清理/返回及后续道具函数；callee为一次176字节分配、一次名称加载和四处共享释放调用点。

函数按ID低16查第一条记录。记录数量A为`+8`，数量B为`+A`，名称owner为`+AC`。

## 2. 三种操作

- 操作0：B按u16回绕加delta。signed B大于99时写B=99、A=0；signed B小于等于0时先写B=0，再把该signed残值加到A；signed A小于等于0时删除。
- 操作1：A按u16回绕加delta。signed A大于99时写A=99、B=0；signed A小于等于0时写A=0，仅当signed B也小于等于0时删除。
- 操作2：B按u16回绕加delta。signed B大于99时写B=99、A=0；signed B小于等于0时写B=0，但只在A精确等于0时删除。A为`0xFFFF`等signed负值时仍保留。
- 其他操作：存量记录不改A/B。

保留记录ID为`0xFFDC`时，任何未删除路径最终强制A=1、B=0。

删除顺序为先改head/前驱，再释放名称owner，最后释放记录。ID查找链环只在将重复读取节点时typed-stop。

## 3. 缺失记录

找不到时，delta signed小于等于0直接返回null。正delta先分配并清零，之后才判断FFDC：

- FFDC把初值强制为1并复制固定名称。
- 普通ID以完整32位ID加载名称；失败时双释放并返回null。
- 操作0或2写B=初值、A=0并OR bit15。
- 操作1写A=初值、B=0，不置bit15。
- 其他操作仍前插一条A/B均为0的记录。

分配null在原memset点typed-stop；与`0x0044D0F0`不同，FFDC数量强制发生在成功分配之后。

## 4. caller对应

剧情VM opcode128的`adjust_player_item_quantity`是操作0的`LegacyWorldItemNode`专用实现，保留B负残值转入A、夹99、双数量删除和FFDC纠正。opcode131的`decrement_party_item`对应带类别的`0x0044D0F0`路径。其他世界与special_modes caller已由各自typed状态函数持有同字段，不保留裸`0x0044D2D0`调用地址。

## 5. 验证

UT覆盖操作0残值转入A及删除、操作1夹99与B正值保留、操作2对A精确零/负值的差异、其他操作FFDC纠正、缺失非正delta、操作0/1/其他的新建字段、分配停止和链环停止。

workpack双生成稳定为`183/227`，SHA256为`95711d8109694baa036473b052dab68f783da7fa85bf7cd06914c8deb2d1653c`；下一项为`0x0044D4E0`。
