# 按编号增减共享道具记录、夹到99并在归零时释放 `0x0044D0F0`

状态：`platform_adapted`

## 1. LST锁

权威范围为`swd3.exe.lst`的`0x0044D0F0..0x0044D2C3`，257行，无外部FUNCTION CHUNK。直接caller为剧情VM opcode 131路径`0x0042C026`；callee为两处176字节分配、一次名称加载和六处共享释放调用点。

参数为链表head、完整32位记录ID、signed i16增量和signed i16类别。记录字段对应：`+4`低16记录ID、`+8`数量、`+2C` bit15类别、`+AC`名称owner。

## 2. 两类查找和数量结果

- 类别等于1或增量为负时，先找ID低16相等且bit15置位的记录。
- 类别等于0或第一阶段留下负数时，再从head找ID低16相等且bit15清零的记录。
- 数量按u16回绕相加，再按i16解释；signed大于99时写99。
- 正数记录保留并返回；ID`0xFFDC`的保留记录无条件把数量重写为1。
- flagged记录结果等于0时解除并释放后立即返回；结果为负时解除并释放，再把该负数作为第二类记录的增量。
- unflagged记录结果小于等于0时解除并释放，返回null。
- 负数在两类均未找到时不分配，返回null。

解除顺序严格为先改前驱/head，再释放`+AC`名称owner，最后释放176字节记录。链表环只在下一次将重复读取同一节点时typed-stop；第一类和第二类分别报告。

## 3. 新建

找不到且残值非负时申请176字节并清零：

- ID低16为`0xFFDC`时先把残值强制为1，复制固定缺省名称，不调用名称loader。
- 普通ID以完整32位ID调用名称loader；失败时先释放名称owner，再释放记录，返回null。
- 写入ID低16；类别精确等于1时OR bit15，其他类别保持清零。
- 写入数量低16并前插head。

分配null只在原memset点typed-stop；FFDC数量强制1等此前副作用保留。

## 4. 剧情VM caller

原caller`0x0042C026`固定传入增量-1、类别0，用于opcode 131限制不满足时撤回刚加入的队员道具。现有`decrement_party_item`已以`LegacyWorldItemNode`专用列表表达同一顺序：负数先查bit15记录，结果非负立即结束，负数删除后继续查bit15清零记录，FFDC保留数量1。该路径没有`0x0044D0F0` opaque端口；两种typed记录结构保持模块隔离。

## 5. 验证

UT覆盖flagged夹99、负数先查flagged再更新unflagged、flagged负数删除后以残值继续、两类归零/负数删除、负数未找到、FFDC存量强制1、普通flagged新建、FFDC新建、分配停止、名称加载失败双释放及两类链环停止。

workpack双生成稳定为`182/227`，SHA256为`18dcd73a6a942eb33d451a439fd3b8ac0c6d3549393b15300b5b507f9c606fec`；下一项为`0x0044D2D0`。
