# 剧情 VM 角色地图状态更新 `0x00429B14`

## 结论

`sub_427920` 的 opcode66 固定长 16 字节，把七个 `u16` 参数零扩展后交给 `sub_40D790`。helper按selector命中情况选择：

- 运行角色缺失：只把同GUID MAPS源flags bit7清零；其余六个operand不写源记录；
- 运行角色存在：要求角色位于八个物理party index槽之一，按条件完成整格对齐/空间摘链，清party对象槽，写运行角色七字段，更新MAPS Talk/Path/flags、标记surface，并从party indices/对象槽中移除该角色。

原版VM caller不观察helper返回；执行后固定推进16字节、保持`ESI=0`，经共同出口发布normalized previous66并yield。现代只在真正提前隔离原版越界/死循环/无owner点时typed-stop；helper已完成诊断性路径仍消费并yield。

唯一行为依据是`swd3.exe.lst`机器指令。共享helper已有证据仅用于owner/状态映射，本handler的参数、调用、结果分类、IP/previous/yield与真实资产均重新独立验证。

## 七参数布局

`0x00429B14..0x00429B45`按逆序push构成：

| 偏移 | request字段 |
| --- | --- |
| `+2` | raw role selector |
| `+4` | path data id |
| `+6` | Talk script id |
| `+8` | action id |
| `+10` | base variant |
| `+12` | variant delta |
| `+14` | flags |

每项均先`xor reg,reg`再`mov reg16,[mem]`，所以全部零扩展到32位实参。handler没有`0xFFFF`字段保留规则，也不把`0xFFF0`替换为Talk source GUID；共享lookup仍支持`0xFFFE`受控角色。

全部七项在调用前读完。现代对完整16字节统一边界检查不会移动任何外部副作用。

## 运行角色缺失fallback

`sub_40D790` lookup缺失时扫描MAPS 22-byte角色源，仅对同GUID源执行：

```text
flags &= 0xFF7F
```

path、Talk、action、base、variant和传入flags都不写。GUID缺失只diagnostic并返回；VM caller仍消费、发布previous并yield。

现代在缺失运行角色时用临时party owner调用既有helper，因此不要求post/live party状态。MAPS数据库或payload缺失仍在真实fallback访问点typed-stop。

## 运行角色命中路径

命中角色后，helper：

1. 验证logical party count为1..8；
2. 扫描完整八个物理party role index槽，允许命中logical count之外的陈旧槽；
3. 读取命中party对象槽的path cursor；cursor `< 0x7FFF`且角色坐标非整格时，先清旧surface占用，再按方向表每次移动4直到整格，并从旧空间行摘链；空间miss原版返回值被忽略，后续仍继续；
4. 把完整命中party对象槽写成`0xFF`；
5. 按七个operand写运行角色，其中`path_word_index=0`；
6. MAPS源保留action/base/variant，只写Talk、Path、Path cursor0，并执行`flags &= 0xFF7F`；patch失败只diagnostic，后续继续；
7. 按新角色flags/action标记surface；
8. 从命中物理槽起把后续party indices和对象槽逐项左移到槽6；槽7保留重复尾值；
9. party count减一。

## caller忽略返回与现代分类

原版opcode66无条件忽略`sub_40D790`返回并推进。现代helper的状态分为：

仍消费并yield：

- `ready`；
- `active_role_not_in_physical_party`：原版helper仅diagnostic后返回；
- 运行角色缺失的`maps_patch_failed`：fallback diagnostic；
- 已完成`party_role_removed`的`maps_patch_failed`：原版忽略MAPS helper失败且完成后续效果；
- 已完成`party_role_removed`的`role_spatial_relocation_failed`：原版忽略空间helper返回且完成后续效果。

typed-stop且不发布IP/previous：

- MAPS/party owner缺失；
- controlled role越界、party count越界；
- path方向越界或无法按4对齐；
- surface访问失败；
- 其他在party移除前结束的checked失败。

所有typed-stop保留helper已经完成的地表、坐标、角色、MAPS或party部分效果。

## post/live party状态同步

helper使用：

- post state的party role indices与party count；
- live frame state的party对象槽。

party移除完成后，现代把已移位的live对象槽复制回post state，再把post count发布到live count。live count owner缺失时，helper、live对象槽移位和post对象槽同步均已完成，但IP/previous仍不发布。

运行角色缺失fallback不访问任何party owner。

## IP、previous、yield与边界

正常与diagnostic-consumed路径：

```text
IP += 16
ESI remains 0
common join publishes previous66
yield
```

完整记录位于`0x7FF0`时，MAPS/运行时效果、IP=`0x8000`和previous66完成后直接yield，不进行下一次fetch。截断记录在helper前`operand_out_of_range`。

## 真实资产锁

对`story-vm-talk-linear-records.tsv`全部opcode66 entry逐条回读TALK文件：

- 共100条物理记录/100 probes；
- TALK1/2/3/4分布`48/0/22/30`；
- 全部raw`0x0042`、长度16；
- 19种selector，无`0xFFF0/0xFFFE/0xFFFF`；
- path仅0和3，其中98条为0；
- 七字段均没有`0xFFFF`；flags仅`0xD100/0xD500/0xD000/0xD904`；
- 原始offset、word、长度与probe逐条核验零错误。

真实回放使用`TALK1.DAT@0x00029F5D`：selector9、path0、Talk9、action9、base0、variant7、flags`0xD100`。运行角色与MAPS更新、party移除、post/live count同步完成，IP推进16、previous66发布并yield。

## 测试覆盖

- 四种raw alias的missing-role MAPS fallback，并证明不要求party owner；
- raw`0xFFF0`不替换、`0xFFFE`受控角色独立覆盖；
- 七个高位operand零扩展、运行角色/MAPS/surface写入；
- 八物理槽扫描、indices与对象槽左移、尾槽重复、post/live同步；
- active role不在party的diagnostic-consumed路径；
- MAPS patch失败和空间miss在完成party移除后仍消费；
- 无效方向在surface清除后typed-stop；
- live count owner缺失的后置部分效果；
- MAPS runtime缺失、16字节截断和`0x7FF0`精确尾；
- TALK1真实记录；
- 共享role-map-update普通/真实测试加Story VM三项共5/5通过。

## 双向收敛与分类

从七项push顺序、`sub_40D790`两条主路径、diagnostic返回、party移位、共同出口到现代helper/result分类逐段双向REVIEW，未发现剩余有效域差异。

分类：`platform_adapted`。适配限于checked helper状态、missing-role临时party owner和post/live同步；有效域参数、状态、副作用、diagnostic消费、IP、previous与yield保持原版。
