# 剧情 VM 设置角色状态位 `0x0400` `0x00429C37`

## 结论

`sub_427920` 的 opcode69 固定长4字节。它先读取`+2` raw selector；值为`0xFFF0`时替换为Talk context source GUID，再调用共享角色lookup：

- 命中运行角色：对完整32位flags执行`flags |= 0x00000400`；
- 缺失运行角色：调用`sub_40D460`，只提交`flags_or_mask=0x0400`和`flags_and_mask=0xFFFF`，其它字段及logical map均为`0xFFFF`。

两路都推进4字节、保持`ESI=0`，共同出口发布normalized previous69后yield。MAPS GUID缺失只diagnostic，VM继续消费。

唯一行为依据是`swd3.exe.lst`机器指令。虽然入口紧邻opcode68，本handler的OR语义和fallback参数已独立恢复与验证。

## selector合同

`0x00429C37..0x00429C4A`：

1. 读取raw`u16 selector`；
2. 若为`0xFFF0`，以Talk context source GUID覆盖；
3. lookup和missing fallback都使用覆盖后的selector。

若替换结果为`0xFFFE`，共享lookup继续解析为受控角色。运行角色缺失时不会把raw`0xFFF0`写入MAPS请求。

## 运行角色命中

原版读取角色`+0x10`完整dword flags，执行：

```text
or CH, 0x04
```

CH bit2对应完整flags bit10`0x00000400`，等价于：

```text
flags |= 0x00000400
```

高16位及其它31位全部保留。handler不修改角色动作、坐标、Talk、Path、surface或空间链。

## MAPS fallback十一个参数

缺失角色时`sub_40D460`参数为：

```text
guid             = resolved selector
action_id        = FFFF
base_variant     = FFFF
variant_delta    = FFFF
tile_x           = FFFF
tile_y           = FFFF
talk_script_id   = FFFF
path_data_id     = FFFF
flags_or_mask    = 0400
flags_and_mask   = FFFF
logical_map_id   = FFFF
```

helper只对源flags OR`0x0400`。GUID缺失时diagnostic返回0；opcode69不检查返回，仍推进、发布previous并yield。

## IP、previous与边界

- 完整记录：IP加4、previous69发布、yield；
- selector截断：在lookup和flags/MAPS效果前`operand_out_of_range`；
- 完整missing记录位于`0x7FFC`：MAPS请求完成，IP到`0x8000`，previous69发布后直接yield，不进行下一fetch。

## 真实资产锁

对`story-vm-talk-linear-records.tsv`全部opcode69 entry逐条回读：

- 103条物理记录/103 probes；
- TALK1/2/3/4分布`22/3/7/71`；
- 全部raw`0x0045`、长度4；
- 16种selector，无`0xFFF0/0xFFFE/0xFFFF`；
- GUID1占82条，其余常见为10/36/34/38/33；
- 原始offset、word、长度与probe逐条核验零错误。

真实回放使用`TALK1.DAT@0x0000CA01`，selector为GUID1；完整flags`0xA5A50001`变为`0xA5A50401`，IP推进4、previous69发布并yield。

## 测试覆盖

- 四种raw alias与完整32位flags保留；
- `0xFFF0`命中source角色；
- `0xFFFE`受控角色lookup；
- 直接missing selector的完整MAPS请求11字段；
- `0xFFF0` missing时fallback使用替换后的GUID；
- selector截断与`0x7FFC`missing exact tail；
- TALK1 GUID1真实记录；
- 剧情VM三项测试通过。

## 双向收敛与分类

实现从selector替换、lookup、CH OR、MAPS十一参数、共享+4尾到previous/yield逐项反向映射，未发现剩余有效域差异。

分类：`platform_adapted`。适配仅为角色lookup/owner和MAPS窄端口的checked隔离；有效域位宽、fallback GUID、参数、IP、previous与yield保持原版。
