# 剧情 VM 角色头顶标记清除 `0x00429D0F`

## 结论

opcode72固定长4字节，原样读取`+2` selector，不替换`0xFFF0`。它调用`sub_40C0D0`查找角色：命中时把角色`field_3c`写零；缺失时不写任何角色状态。两路都推进4、保持`ESI=0`，经公共出口发布normalized previous72并yield。

现有C++的lookup与清零正确，但错误地同调用continue且漏发previous；现已按LST恢复跨帧行为。

## LST收敛

- `00429D0F mov dx,[ebx+2]`：raw selector；
- `00429D19 call sub_40C0D0`：共享lookup，因此`0xFFFE`仍可指向受控角色；
- `00429D21 test eax,eax`后缺失直接跳共享`+4`尾；
- 命中按角色索引写`dword_4BABE4[index*0xD8]=0`，即`field_3c=0`；
- `0042C7E6`共享尾推进4并令`ESI=0`；
- `0042B0AE`公共join发布previous并yield。

因此literal`0xFFF0`若没有同GUID角色会静默消费，不会替换talk-context GUID。`0x7FFC`完整记录可先完成清零、IP=`0x8000`、previous72，再直接yield；`0x7FFE`只有opcode时在lookup前typed-stop，IP与previous不变。

## 真实资产锁

- 329条物理记录、336个entry probes；
- TALK1/2/3/4分布`126/21/138/44`；
- 全部raw`0x0048`、长度4；
- 53种selector，无`0xFFF0/0xFFFE`；
- 原始offset、word、长度逐条核验零错误。

真实回放使用`TALK1.DAT@0x00004BA7`：selector GUID191；预置非零`field_3c`后清零、推进4、发布previous72并yield。

## 测试覆盖

- 四raw alias命中清零；
- literal`0xFFF0`缺失静默消费且不改角色；
- `0xFFFE`受控角色；
- selector截断typed-stop；
- `0x7FFC`完整精确尾；
- TALK1真实记录；
- story100所有opcode72显式帧边界；
- 剧情VM三项测试通过。

## 分类

分类：`assembly_exact`。有效域内lookup、写入、缺失、IP、previous与yield均直接对应汇编，无新增平台替代。
