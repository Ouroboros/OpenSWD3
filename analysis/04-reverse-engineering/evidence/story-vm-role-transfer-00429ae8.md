# 剧情 VM 角色转入队伍 `0x00429AE8`

## 结论

`sub_427920` 的 opcode65 是 `sub_40D610` 的剧情入口。它读取一个原始 `u16` 角色选择器，调用 `sub_40C0D0` 查找运行角色；命中才执行完整角色转入队伍流程，缺失则静默消费。两路都推进 4 字节、保持 `ESI=0`，经共同出口发布 normalized previous65 后 yield。

handler 不把 `0xFFF0` 替换为 Talk source GUID；它只继承共享 lookup helper 对 `0xFFFE` 受控角色选择器的处理。真实 opcode65 资产中没有 `0xFFF0/0xFFFE/0xFFFF`。

`sub_40D610` 已由 [`world-role-transfer-0040d610-0040d785.md`](world-role-transfer-0040d610-0040d785.md) 独立闭环。本证据不继承其完成状态，而是重新证明 opcode65 的 selector、调用条件、owner 接线、helper 结果传播、IP、previous 与 yield。

## 主 handler

`0x00429AE8..0x00429B0F`：

1. 从 `current+2` 读取 raw `u16` selector；
2. 调用 `sub_40C0D0(selector, &role_index)`；
3. lookup 返回 0 时不调用 `sub_40D610`；
4. lookup 返回非零时调用 `sub_40D610(role_index)`；
5. 两路进入共享 `+4` 尾 `0x0042C7E6`；
6. IP 加 4，`ESI` 保持 0，共同出口发布 previous65 后 yield。

没有 next-opcode lookahead、没有 `0xFFF0` 特判，也不观察 `sub_40D610` 的原版返回寄存器。

## `sub_40D610` 复用合同

命中角色后，共享 helper 的有效域顺序为：

- `path_data_id == 0`：跳过 72 个活动对象槽、MAPS、地表与空间链，直接执行共同队伍追加；
- `path_data_id != 0` 但 72 槽无匹配角色索引：同样直接队伍追加；
- 命中活动对象且角色坐标非整格：先清旧地表占用，按对象 path cursor 的方向字节每次移动 4，直到 X/Y 低四位为 0，再从旧空间行摘链；
- 命中活动对象后，用 `sub_40D460` 对同 GUID MAPS 源 OR flags `0x0080`，随后以 `sub_40DD40` 把完整 `0x21C` 对象槽写为 `0xFF`；
- 最后把 role index 写到当前 party count 槽，清对应 party 对象槽，清角色 Talk id，把 flags bit14 清零并置 bit7，然后 party count 加一。

原版队伍上限和多个裸 owner 不检查。现代共享 helper在容量、对象槽、方向、surface、spatial、MAPS patch等无效域返回精确 `LegacyWorldRoleTransferStatus`；opcode65映射为`role_transfer_failed`并保留已完成的前置副作用，IP/previous不发布。

## MAPS nullable owner时点

为让Story VM在原版真实MAPS访问点才检查owner，共享helper新增nullable `LegacyMapsWorldDatabase*` overload；原有引用overload仍保留并转发。

- path为0或无匹配活动对象时，helper不访问MAPS owner，nullptr不会阻止共同队伍追加；
- 命中活动对象时，非整格对齐、地表和空间副作用先执行；到MAPS patch点才把nullptr报告为`role_source_patch_failed`；
- MAPS失败发生在对象槽重置、队伍追加、IP和previous之前。

这保持了原版访问顺序，同时隔离无效平台owner。

## post状态与live frame状态

SDL加载世界时把`LegacyWorldRolePostMaterializationState`中的party count/对象槽复制到`world_frame_state_`。运行期间：

- post state继续权威持有party role indices、party count和转移统计；
- live frame state的party count/对象槽由帧协调器直接消费。

因此opcode65在共享helper成功后按原版顺序同步：

1. 先把新增party对象槽从post state复制到live frame槽；
2. 再发布live party count。

role indices仍由post state权威持有。live槽owner缺失时，共享helper的角色/队伍副作用已经完成，但IP/previous尚未发布；live count也保持旧值。有效SDL路径同时更新两侧，避免只写加载期副本。

## 失败、边界与精确尾

- selector word不可读：在lookup和所有转移状态前`operand_out_of_range`；
- 角色缺失：不要求任何transfer/live owner，推进4、发布previous并yield；
- transfer state缺失：角色命中后、helper前`runtime_unavailable`；
- helper typed失败：保留helper内部顺序，IP/previous不变；
- live槽/count缺失：保留helper已完成效果和已完成的前置live同步；
- 完整缺失角色记录位于`0x7FFC`时，IP可推进到`0x8000`并直接yield；因为`ESI=0`，不会继续取指。

## 真实资产锁

对`story-vm-talk-linear-records.tsv`的全部opcode65 entry逐条回读原始TALK文件：

- 共109条物理记录、110个entry probes；
- TALK1/2/3/4分布`52/1/21/35`；
- 全部raw`0x0041`、长度4；108条probe=1，一条`TALK3.DAT@0x0001E40D`为probe=2；
- 19种selector，无`0xFFF0/0xFFFE/0xFFFF`；最常见GUID为9/3/10，分别19/16/16条；
- 原始offset、word与长度逐条核验零错误。

真实回放使用`TALK1.DAT@0x0000FE4F`，selector为GUID 3。path为0的角色直接写入post/live party bookkeeping，Talk清零、flags由`0x4082`变为`0x82`，IP推进4、previous65发布并yield。

## 测试覆盖

- 四种raw alias；
- path为0的共同追加、Talk/flags、post role index/count、live槽/count同步；
- raw`0xFFF0`不替换且缺失静默消费；
- `0xFFFE`受控角色lookup；
- aligned活动对象的MAPS flags OR、完整对象槽清空与最终追加；
- nullable MAPS owner在patch点失败，保持对象槽与队伍状态；
- live party槽owner缺失发生在helper成功之后，保留helper效果但阻止publication；
- party count已满的typed边界；
- selector截断与`0x7FFC`缺失角色精确尾；
- TALK1真实GUID 3记录；
- 共享role-transfer单元测试加Story VM三项测试共4/4通过。

## 双向收敛与分类

REVIEW发现并修正一处SDL集成差异：只写`world.role_post_materialization.party_object_slots`会遗漏live `world_frame_state_.party_object_slots`；现已按对象槽后count的顺序同步。再次从VM case、共享helper、SDL owner和测试反向映射到LST后，未发现剩余有效域差异。

分类：`platform_adapted`。适配仅限nullable MAPS owner、checked helper失败和post/live owner同步；有效域selector、helper顺序、队伍状态、IP、previous与yield保持原版语义。
