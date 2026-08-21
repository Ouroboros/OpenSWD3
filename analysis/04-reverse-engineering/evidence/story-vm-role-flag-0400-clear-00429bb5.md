# 剧情 VM 清除角色状态位 `0x0400` `0x00429BB5`

## 结论

`sub_427920` 的 opcode68 固定长4字节。它先读取`+2` raw selector；值为`0xFFF0`时替换为Talk context source GUID，再调用共享角色lookup：

- 命中运行角色：对完整32位flags执行`flags &= 0xFFFFFBFF`；
- 缺失运行角色：调用`sub_40D460`，只提交`flags_or_mask=0`和`flags_and_mask=0xFBFF`，其它字段及logical map均为`0xFFFF`。

两路都推进4字节、保持`ESI=0`，共同出口发布normalized previous68后yield。MAPS GUID缺失只diagnostic，VM继续消费。

唯一行为依据是`swd3.exe.lst`机器指令。本handler不从相邻opcode69继承完成状态。

## selector合同

`0x00429BB5..0x00429BC8`：

1. 读取raw `u16 selector`；
2. 若等于`0xFFF0`，用Talk context `+0x24` source GUID覆盖selector；
3. 把**覆盖后的selector**交给`sub_40C0D0`。

因此运行角色缺失时，MAPS fallback也使用替换后的source GUID，而不是raw `0xFFF0`。若替换结果为共享`0xFFFE`受控角色selector，lookup继续按共享helper规则解析。

## 运行角色命中

命中角色索引后，原版读取角色`+0x10`完整dword flags，并执行：

```text
and CH, 0xFB
```

CH bit2对应完整flags bit10 `0x00000400`，所以等价于：

```text
flags &= 0xFFFFFBFF
```

高16位及其它31位全部保留。handler不触碰角色动作、坐标、Talk、Path、surface或空间链。

## MAPS fallback十一个参数

缺失角色时，`sub_40D460`参数为：

```text
guid             = resolved selector
action_id        = FFFF
base_variant     = FFFF
variant_delta    = FFFF
tile_x           = FFFF
tile_y           = FFFF
talk_script_id   = FFFF
path_data_id     = FFFF
flags_or_mask    = 0000
flags_and_mask   = FBFF
logical_map_id   = FFFF
```

helper只执行源flags AND `0xFBFF`。GUID缺失时helper自身diagnostic并返回0；opcode68不检查返回，仍推进、发布previous并yield。

现代复用现有`patch_role_source`窄端口，保留请求字段和diagnostic-only调用合同。

## IP、previous与边界

- 完整记录：IP加4、previous68发布、yield；
- selector截断：在lookup和任何flags/MAPS效果前`operand_out_of_range`；
- 完整记录位于`0x7FFC`：角色或MAPS效果完成，IP到`0x8000`，previous68发布后直接yield；`ESI=0`所以不进行下一fetch。

## 真实资产锁

对`story-vm-talk-linear-records.tsv`全部opcode68 entry逐条回读：

- 78条物理记录/78 probes；
- TALK1/2/3/4分布`24/6/7/41`；
- 全部raw`0x0044`、长度4；
- 17种selector，无`0xFFF0/0xFFFE/0xFFFF`；
- GUID1占50条，其余常见为4/9/10；
- 原始offset、word、长度与probe逐条核验零错误。

真实回放使用`TALK1.DAT@0x00009795`，selector为GUID1；完整flags`0xA5A5FFFF`变为`0xA5A5FBFF`，IP推进4、previous68发布并yield。

## 测试覆盖

- 四种raw alias与完整32位flags保留；
- `0xFFF0`命中source角色；
- `0xFFFE`受控角色lookup；
- 直接missing selector的完整MAPS请求11字段；
- `0xFFF0` missing时fallback使用替换后的GUID；
- selector截断和`0x7FFC`missing exact tail；
- TALK1 GUID1真实记录；
- 剧情VM三项测试通过。

## 双向收敛与分类

实现从selector替换、lookup、CH位掩码、MAPS十一参数、共享+4尾到previous/yield逐项反向映射，未发现剩余有效域差异。

分类：`platform_adapted`。适配仅为角色lookup/owner和MAPS窄端口的checked隔离；有效域位宽、fallback GUID、参数、IP、previous与yield保持原版。
