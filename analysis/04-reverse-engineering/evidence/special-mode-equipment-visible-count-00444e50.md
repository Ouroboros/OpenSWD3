# 装备物品窗口可见项计数 `0x00444E50`

状态：`assembly_exact`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。物理范围`0x00444E50..0x00444E73`，29行，无FUNCTION CHUNK。callers为已关闭`0x00443670`、`0x004437C0`和待审`0x00444E80`。

入口读取窗口链`visible_record_head`，无条件把`visible_record_count`清零。只要当前节点非空且count小于24，就先递增并发布count，再沿offset0 next推进。返回值严格为计数停止处的节点：短链/空链为null，至少25项时为第25项节点；不改链。

43670分页推进与437C0分页后退中的`refresh_equipment_visible_count` opaque端口已删除，均在B9A0重建窗口头之后直接调用本helper。相应不可达的refresh-stop状态及F40映射已删除；两个caller的helper count继续计入本次直接调用。444E80仍独立待审，不提前回收。

UT覆盖30项链计数24并返回第25项、2项短链返回null及空链清除陈旧count；既有分页推进/后退UT继续覆盖窗口头、count、selection与后续文本发布。定向测试通过。

workpack双生成稳定为`111/227`，SHA256均为`b046f9af560be24a113156ed365081d67ee9e7457e18a45db06821aaebc78586`；下一单元`0x00444E80`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
